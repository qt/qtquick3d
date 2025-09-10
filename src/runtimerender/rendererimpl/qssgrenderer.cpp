// Copyright (C) 2008-2012 NVIDIA Corporation.
// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qssgrenderer_p.h"

#include <QtQuick3DRuntimeRender/private/qssgrenderitem2d_p.h>
#include "../qssgrendercontextcore.h"
#include <QtQuick3DRuntimeRender/private/qssgrendercamera_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderlight_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderimage_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderbuffermanager_p.h>
#include "../qssgrendercontextcore.h"
#include <QtQuick3DRuntimeRender/private/qssgrendereffect_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrhicustommaterialsystem_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercodegenerator_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderdefaultmaterialshadergenerator_p.h>
#include <QtQuick3DRuntimeRender/private/qssgperframeallocator_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrhiquadrenderer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendertexturedata_p.h>
#include <QtQuick3DRuntimeRender/private/qssglayerrenderdata_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrhiparticles_p.h>
#include <QtQuick3DRuntimeRender/private/qssgvertexpipelineimpl_p.h>
#include "../qssgshadermapkey_p.h"
#include "../qssgrenderpickresult_p.h"

#include <QtQuick3DUtils/private/qquick3dprofiler_p.h>
#include <QtQuick3DUtils/private/qssgdataref_p.h>
#include <QtQuick3DUtils/private/qssgutils_p.h>
#include <QtQuick3DUtils/private/qssgassert_p.h>
#include <qtquick3d_tracepoints_p.h>

#include <QtCore/QMutexLocker>
#include <QtCore/QBitArray>

#include <cstdlib>
#include <algorithm>
#include <limits>

/*
    Rendering is done is several steps, these are:

    1. \l{QSSGRenderer::beginFrame(){beginFrame()} - set's up the renderer to start a new frame.

    2. Now that the renderer is reset, values for the \l{QSSGRenderer::setViewport}{viewport}, \l{QSSGRenderer::setDpr}{dpr},
    \l{QSSGRenderer::setScissorRect}{scissorRect} etc. should be updated.

    3. \l{QSSGRenderer::prepareLayerForRender()} - At this stage the scene tree will be traversed
    and state for the renderer needed to render gets collected. This includes, but is not limited to,
    calculating global transforms, loading of meshes, preparing materials and setting up the rendering
    steps needed for the frame (opaque and transparent pass etc.)
    If the there are custom \l{QQuick3DRenderExtension}{render extensions} added to to \l{View3D::extensions}{View3D}
    then they will get their first chance to modify or react to the collected data here.
    If the users have implemented the virtual function \l{QSSGRenderExtension::prepareData()}{prepareData} it will be
    called after all active nodes have been collected and had their global data updated, but before any mesh or material
    has been loaded.

    4. \l{QSSGRenderer::rhiPrepare()} - Starts rendering necessary sub-scenes and prepare resources.
    Sub-scenes, or sub-passes that are to be done in full, will be done at this stage.

    5. \l{QSSGRenderer::rhiRender()} - Renders the scene to the main target.

    6. \l{QSSGRenderer::endFrame()} - Marks the frame as done and cleans-up dirty states and
    uneeded resources.
*/

QT_BEGIN_NAMESPACE

struct QSSGRenderableImage;
struct QSSGSubsetRenderable;

void QSSGRenderer::releaseCachedResources()
{
    m_rhiQuadRenderer.reset();
    m_rhiCubeRenderer.reset();
}

QSSGRenderer::QSSGRenderer() = default;

QSSGRenderer::~QSSGRenderer()
{
    m_contextInterface = nullptr;
    releaseCachedResources();
}

void QSSGRenderer::cleanupUnreferencedBuffers(QSSGRenderLayer *inLayer)
{
    // Now check for unreferenced buffers and release them if necessary
    m_contextInterface->bufferManager()->cleanupUnreferencedBuffers(m_frameCount, inLayer);
}

void QSSGRenderer::resetResourceCounters(QSSGRenderLayer *inLayer)
{
    m_contextInterface->bufferManager()->resetUsageCounters(m_frameCount, inLayer);
}

bool QSSGRenderer::prepareLayerForRender(QSSGRenderLayer &inLayer)
{
    QSSGLayerRenderData *theRenderData = getOrCreateLayerRenderData(inLayer);
    Q_ASSERT(theRenderData);
    beginLayerRender(*theRenderData);
    theRenderData->resetForFrame();
    theRenderData->prepareForRender();
    endLayerRender();
    return theRenderData->layerPrepResult.flags.wasDirty();
}

// Phase 1: prepare. Called when the renderpass is not yet started on the command buffer.
void QSSGRenderer::rhiPrepare(QSSGRenderLayer &inLayer)
{
    QSSGLayerRenderData *theRenderData = getOrCreateLayerRenderData(inLayer);
    QSSG_ASSERT(theRenderData && !theRenderData->renderedCameras.isEmpty(), return);

    const auto layerPrepResult = theRenderData->layerPrepResult;
    if (layerPrepResult.isLayerVisible()) {
        ///
        QSSGRhiContext *rhiCtx = contextInterface()->rhiContext().get();
        QSSG_ASSERT(rhiCtx->isValid() && rhiCtx->rhi()->isRecordingFrame(), return);
        theRenderData->maybeBakeLightmap();
        beginLayerRender(*theRenderData);
        // Process active passes. "PreMain" passes are individual passes
        // that does can and should be done in the rhi prepare phase.
        // It is assumed that passes are sorted in the list with regards to
        // execution order.
        const auto &activePasses = theRenderData->activePasses;
        for (const auto &pass : activePasses) {
            pass->renderPrep(*this, *theRenderData);
            if (pass->passType() == QSSGRenderPass::Type::Standalone)
                pass->renderPass(*this);
        }

        endLayerRender();
    }
}

// Phase 2: render. Called within an active renderpass on the command buffer.
void QSSGRenderer::rhiRender(QSSGRenderLayer &inLayer)
{
    QSSGLayerRenderData *theRenderData = getOrCreateLayerRenderData(inLayer);
    QSSG_ASSERT(theRenderData && !theRenderData->renderedCameras.isEmpty(), return);
    if (theRenderData->layerPrepResult.isLayerVisible()) {
        beginLayerRender(*theRenderData);
        const auto &activePasses = theRenderData->activePasses;
        for (const auto &pass : activePasses) {
            if (pass->passType() == QSSGRenderPass::Type::Main || pass->passType() == QSSGRenderPass::Type::Extension)
                pass->renderPass(*this);
        }
        endLayerRender();
    }
}

template<typename Container>
static void cleanupResourcesImpl(const QSSGRenderContextInterface &rci, const Container &resources)
{
    const auto &rhiCtx = rci.rhiContext();
    if (!rhiCtx->isValid())
        return;

    const auto &bufferManager = rci.bufferManager();

    for (const auto &resource : resources) {
        if (resource->type == QSSGRenderGraphObject::Type::Geometry) {
            auto geometry = static_cast<QSSGRenderGeometry*>(resource);
            bufferManager->releaseGeometry(geometry);
        } else if (resource->type == QSSGRenderGraphObject::Type::Model) {
            auto model = static_cast<QSSGRenderModel*>(resource);
            QSSGRhiContextPrivate::get(rhiCtx.get())->cleanupDrawCallData(model);
            delete model->particleBuffer;
        } else if (resource->type == QSSGRenderGraphObject::Type::TextureData || resource->type == QSSGRenderGraphObject::Type::Skin) {
            static_assert(std::is_base_of_v<QSSGRenderTextureData, QSSGRenderSkin>, "QSSGRenderSkin is expected to be a QSSGRenderTextureData type!");
            auto textureData = static_cast<QSSGRenderTextureData *>(resource);
            bufferManager->releaseTextureData(textureData);
        } else if (resource->type == QSSGRenderGraphObject::Type::RenderExtension) {
            auto *rext = static_cast<QSSGRenderExtension *>(resource);
            bufferManager->releaseExtensionResult(*rext);
        } else if (resource->type == QSSGRenderGraphObject::Type::ModelInstance) {
            auto *rhiCtxD = QSSGRhiContextPrivate::get(rhiCtx.get());
            auto *table = static_cast<QSSGRenderInstanceTable *>(resource);
            rhiCtxD->releaseInstanceBuffer(table);
        }

        // ### There might be more types that need to be supported

        delete resource;
    }
}

void QSSGRenderer::cleanupResources(QList<QSSGRenderGraphObject *> &resources)
{
    cleanupResourcesImpl(*m_contextInterface, resources);
    resources.clear();
}

void QSSGRenderer::cleanupResources(QSet<QSSGRenderGraphObject *> &resources)
{
    cleanupResourcesImpl(*m_contextInterface, resources);
    resources.clear();
}

QSSGLayerRenderData *QSSGRenderer::getOrCreateLayerRenderData(QSSGRenderLayer &layer)
{
    if (layer.renderData == nullptr)
        layer.renderData = new QSSGLayerRenderData(layer, *this);

    return layer.renderData;
}

void QSSGRenderer::addMaterialDirtyClear(QSSGRenderGraphObject *material)
{
    m_materialClearDirty.insert(material);
}

static QByteArray logPrefix() { return QByteArrayLiteral("mesh default material pipeline-- "); }


QSSGRhiShaderPipelinePtr QSSGRendererPrivate::generateRhiShaderPipelineImpl(QSSGSubsetRenderable &renderable,
                                                                            QSSGShaderLibraryManager &shaderLibraryManager,
                                                                            QSSGShaderCache &shaderCache,
                                                                            QSSGProgramGenerator &shaderProgramGenerator,
                                                                            const QSSGShaderDefaultMaterialKeyProperties &shaderKeyProperties,
                                                                            const QSSGShaderFeatures &featureSet,
                                                                            QByteArray &shaderString)
{
    shaderString = logPrefix();
    QSSGShaderDefaultMaterialKey theKey(renderable.shaderDescription);

    // This is not a cheap operation. This function assumes that it will not be
    // hit for every material for every model in every frame (except of course
    // for materials that got changed). In practice this is ensured by the
    // cheaper-to-lookup cache in getShaderPipelineForDefaultMaterial().
    theKey.toString(shaderString, shaderKeyProperties);

    // Check the in-memory, per-QSSGShaderCache (and so per-QQuickWindow)
    // runtime cache. That may get cleared upon an explicit call to
    // QQuickWindow::releaseResources(), but will otherwise store all
    // encountered shader pipelines in any View3D in the window.
    if (const auto &maybePipeline = shaderCache.tryGetRhiShaderPipeline(shaderString, featureSet))
        return maybePipeline;

    // Check if there's a pre-built (offline generated) shader for available.
    const QByteArray qsbcKey = QQsbCollection::EntryDesc::generateSha(shaderString, QQsbCollection::toFeatureSet(featureSet));
    const QQsbCollection::EntryMap &pregenEntries = shaderLibraryManager.m_preGeneratedShaderEntries;
    if (!pregenEntries.isEmpty()) {
        const auto foundIt = pregenEntries.constFind(QQsbCollection::Entry(qsbcKey));
        if (foundIt != pregenEntries.cend())
            return shaderCache.newPipelineFromPregenerated(shaderString, featureSet, *foundIt, renderable.material);
    }

    // Try the persistent (disk-based) cache then.
    if (const auto &maybePipeline = shaderCache.tryNewPipelineFromPersistentCache(qsbcKey, shaderString, featureSet))
        return maybePipeline;

    // Otherwise, build new shader code and run the resulting shaders through
    // the shader conditioning pipeline.
    const auto &material = static_cast<const QSSGRenderDefaultMaterial &>(renderable.getMaterial());
    QSSGMaterialVertexPipeline vertexPipeline(shaderProgramGenerator,
                                              shaderKeyProperties,
                                              material.adapter);

    return QSSGMaterialShaderGenerator::generateMaterialRhiShader(logPrefix(),
                                                                  vertexPipeline,
                                                                  renderable.shaderDescription,
                                                                  shaderKeyProperties,
                                                                  featureSet,
                                                                  renderable.material,
                                                                  renderable.lights,
                                                                  renderable.firstImage,
                                                                  shaderLibraryManager,
                                                                  shaderCache);
}

QSSGRhiShaderPipelinePtr QSSGRendererPrivate::generateRhiShaderPipeline(QSSGRenderer &renderer,
                                                                        QSSGSubsetRenderable &inRenderable,
                                                                        const QSSGShaderFeatures &inFeatureSet)
{
    auto *currentLayer = renderer.m_currentLayer;
    auto &generatedShaderString = currentLayer->generatedShaderString;
    const auto &m_contextInterface = renderer.m_contextInterface;
    const auto &theCache = m_contextInterface->shaderCache();
    const auto &shaderProgramGenerator = m_contextInterface->shaderProgramGenerator();
    const auto &shaderLibraryManager = m_contextInterface->shaderLibraryManager();
    return QSSGRendererPrivate::generateRhiShaderPipelineImpl(inRenderable, *shaderLibraryManager, *theCache, *shaderProgramGenerator, currentLayer->defaultMaterialShaderKeyProperties, inFeatureSet, generatedShaderString);
}

void QSSGRenderer::beginFrame(QSSGRenderLayer &layer, bool allowRecursion)
{
    const bool executeBeginFrame = !(allowRecursion && (m_activeFrameRef++ != 0));
    if (executeBeginFrame) {
        m_contextInterface->perFrameAllocator()->reset();
        QSSGRHICTX_STAT(m_contextInterface->rhiContext().get(), start(&layer));
        resetResourceCounters(&layer);
    }
}

bool QSSGRenderer::endFrame(QSSGRenderLayer &layer, bool allowRecursion)
{
    const bool executeEndFrame = !(allowRecursion && (--m_activeFrameRef != 0));
    if (executeEndFrame) {
        cleanupUnreferencedBuffers(&layer);

               // We need to do this endFrame(), as the material nodes might not exist after this!
        for (auto *matObj : std::as_const(m_materialClearDirty)) {
            if (matObj->type == QSSGRenderGraphObject::Type::CustomMaterial) {
                static_cast<QSSGRenderCustomMaterial *>(matObj)->clearDirty();
            } else if (matObj->type == QSSGRenderGraphObject::Type::DefaultMaterial ||
                       matObj->type == QSSGRenderGraphObject::Type::PrincipledMaterial ||
                       matObj->type == QSSGRenderGraphObject::Type::SpecularGlossyMaterial) {
                static_cast<QSSGRenderDefaultMaterial *>(matObj)->clearDirty();
            }
        }
        m_materialClearDirty.clear();

        QSSGRHICTX_STAT(m_contextInterface->rhiContext().get(), stop(&layer));

        ++m_frameCount;
    }

    return executeEndFrame;
}

QSSGRendererPrivate::PickResultList QSSGRendererPrivate::syncPickAll(const QSSGRenderContextInterface &ctx,
                                                                     const QSSGRenderLayer &layer,
                                                                     const QSSGRenderRay &ray)
{
    const auto &bufferManager = ctx.bufferManager();
    const bool isGlobalPickingEnabled = QSSGRendererPrivate::isGlobalPickingEnabled(*ctx.renderer());
    PickResultList pickResults;
    Q_ASSERT(layer.getGlobalState(QSSGRenderNode::GlobalState::Active));
    getLayerHitObjectList(layer, *bufferManager, ray, isGlobalPickingEnabled, pickResults);
    // Things are rendered in a particular order and we need to respect that ordering.
    std::stable_sort(pickResults.begin(), pickResults.end(), [](const QSSGRenderPickResult &lhs, const QSSGRenderPickResult &rhs) {
        return lhs.m_distanceSq < rhs.m_distanceSq;
    });
    return pickResults;
}

QSSGRendererPrivate::PickResultList QSSGRendererPrivate::syncPick(const QSSGRenderContextInterface &ctx,
                                                                  const QSSGRenderLayer &layer,
                                                                  const QSSGRenderRay &ray,
                                                                  QSSGRenderNode *target)
{
    const auto &bufferManager = ctx.bufferManager();
    const bool isGlobalPickingEnabled = QSSGRendererPrivate::isGlobalPickingEnabled(*ctx.renderer());

    Q_ASSERT(layer.getGlobalState(QSSGRenderNode::GlobalState::Active));
    PickResultList pickResults;
    if (target)
        intersectRayWithSubsetRenderable(*bufferManager, ray, *target, pickResults);
    else
        getLayerHitObjectList(layer, *bufferManager, ray, isGlobalPickingEnabled, pickResults);

    std::stable_sort(pickResults.begin(), pickResults.end(), [](const QSSGRenderPickResult &lhs, const QSSGRenderPickResult &rhs) {
        return lhs.m_distanceSq < rhs.m_distanceSq;
    });
    return pickResults;
}

QSSGRendererPrivate::PickResultList QSSGRendererPrivate::syncPickSubset(const QSSGRenderLayer &layer,
                                                                        QSSGBufferManager &bufferManager,
                                                                        const QSSGRenderRay &ray,
                                                                        QVarLengthArray<QSSGRenderNode*> subset)
{
    QSSGRendererPrivate::PickResultList pickResults;
    Q_ASSERT(layer.getGlobalState(QSSGRenderNode::GlobalState::Active));

    for (auto target : subset)
        intersectRayWithSubsetRenderable(bufferManager, ray, *target, pickResults);

    std::stable_sort(pickResults.begin(), pickResults.end(), [](const QSSGRenderPickResult &lhs, const QSSGRenderPickResult &rhs) {
        return lhs.m_distanceSq < rhs.m_distanceSq;
    });
    return pickResults;
}

void QSSGRendererPrivate::setGlobalPickingEnabled(QSSGRenderer &renderer, bool isEnabled)
{
    renderer.m_globalPickingEnabled = isEnabled;
}

void QSSGRendererPrivate::setRenderContextInterface(QSSGRenderer &renderer, QSSGRenderContextInterface *ctx)
{
    renderer.m_contextInterface = ctx;
}

const std::unique_ptr<QSSGRhiQuadRenderer> &QSSGRenderer::rhiQuadRenderer() const
{
    if (!m_rhiQuadRenderer)
        m_rhiQuadRenderer = std::make_unique<QSSGRhiQuadRenderer>();

    return m_rhiQuadRenderer;
}

const std::unique_ptr<QSSGRhiCubeRenderer> &QSSGRenderer::rhiCubeRenderer() const
{
    if (!m_rhiCubeRenderer)
        m_rhiCubeRenderer = std::make_unique<QSSGRhiCubeRenderer>();

    return m_rhiCubeRenderer;

}

void QSSGRenderer::beginSubLayerRender(QSSGLayerRenderData &inLayer)
{
    inLayer.saveRenderState(*this);
    m_currentLayer = nullptr;
}

void QSSGRenderer::endSubLayerRender(QSSGLayerRenderData &inLayer)
{
    inLayer.restoreRenderState(*this);
    m_currentLayer = &inLayer;
}

void QSSGRenderer::beginLayerRender(QSSGLayerRenderData &inLayer)
{
    m_currentLayer = &inLayer;
}
void QSSGRenderer::endLayerRender()
{
    m_currentLayer = nullptr;
}

using RenderableList = QVarLengthArray<const QSSGRenderNode *>;
static void dfs(const QSSGRenderNode &node, RenderableList &renderables)
{
    if (QSSGRenderGraphObject::isRenderable(node.type))
        renderables.push_back(&node);

    for (const auto &child : node.children)
        dfs(child, renderables);
}

void QSSGRendererPrivate::getLayerHitObjectList(const QSSGRenderLayer &layer,
                                                QSSGBufferManager &bufferManager,
                                                const QSSGRenderRay &ray,
                                                bool inPickEverything,
                                                PickResultList &outIntersectionResult)
{
    RenderableList renderables;
    for (const auto &childNode : layer.children)
        dfs(childNode, renderables);

    for (int idx = renderables.size() - 1; idx >= 0; --idx) {
        const auto &pickableObject = renderables.at(idx);
        if (inPickEverything || pickableObject->getLocalState(QSSGRenderNode::LocalState::Pickable))
            intersectRayWithSubsetRenderable(bufferManager, ray, *pickableObject, outIntersectionResult);
    }
}

void QSSGRendererPrivate::intersectRayWithSubsetRenderable(QSSGBufferManager &bufferManager,
                                                           const QSSGRenderRay &inRay,
                                                           const QSSGRenderNode &node,
                                                           PickResultList &outIntersectionResultList)
{
    // Item2D's requires special handling
    if (node.type == QSSGRenderGraphObject::Type::Item2D) {
        const QSSGRenderItem2D &item2D = static_cast<const QSSGRenderItem2D &>(node);
        intersectRayWithItem2D(inRay, item2D, outIntersectionResultList);
        return;
    }

    if (node.type != QSSGRenderGraphObject::Type::Model)
        return;

    const QSSGRenderModel &model = static_cast<const QSSGRenderModel &>(node);

    // We have to have a guard here, as the meshes are usually loaded on the render thread,
    // and we assume all meshes are loaded before picking and none are removed, which
    // is usually true, except for custom geometry which can be updated at any time. So this
    // guard should really only be locked whenever a custom geometry buffer is being updated
    // on the render thread.  Still naughty though because this can block the render thread.
    QMutexLocker mutexLocker(bufferManager.meshUpdateMutex());
    auto mesh = bufferManager.getMeshForPicking(model);
    if (!mesh)
        return;

    const auto &subMeshes = mesh->subsets;
    QSSGBounds3 modelBounds;
    for (const auto &subMesh : subMeshes)
        modelBounds.include(subMesh.bounds);

    if (modelBounds.isEmpty())
        return;

    const bool instancing = model.instancing(); // && instancePickingEnabled
    int instanceCount = instancing ? model.instanceTable->count() : 1;

    for (int instanceIndex = 0; instanceIndex < instanceCount; ++instanceIndex) {

        QMatrix4x4 modelTransform;
        if (instancing) {
            modelTransform = model.globalInstanceTransform * model.instanceTable->getTransform(instanceIndex) * model.localInstanceTransform;
        } else {
            modelTransform = model.globalTransform;
        }
        auto rayData = QSSGRenderRay::createRayData(modelTransform, inRay);

        auto hit = QSSGRenderRay::intersectWithAABBv2(rayData, modelBounds);

        // If we don't intersect with the model at all, then there's no need to go furher down!
        if (!hit.intersects())
            continue;

        // Check each submesh to find the closest intersection point
        float minRayLength = std::numeric_limits<float>::max();
        QSSGRenderRay::IntersectionResult intersectionResult;
        QVector<QSSGRenderRay::IntersectionResult> results;

        int subset = 0;
        int resultSubset = 0;
        for (const auto &subMesh : subMeshes) {
            QSSGRenderRay::IntersectionResult result;
            if (!subMesh.bvhRoot.isNull()) {
                hit = QSSGRenderRay::intersectWithAABBv2(rayData, subMesh.bvhRoot->boundingData);
                if (hit.intersects()) {
                    results.clear();
                    inRay.intersectWithBVH(rayData, static_cast<const QSSGMeshBVHNode *>(subMesh.bvhRoot), mesh, results);
                    float subMeshMinRayLength = std::numeric_limits<float>::max();
                    for (const auto &subMeshResult : std::as_const(results)) {
                        if (subMeshResult.rayLengthSquared < subMeshMinRayLength) {
                            result = subMeshResult;
                            subMeshMinRayLength = result.rayLengthSquared;
                        }
                    }
                }
            } else {
                hit = QSSGRenderRay::intersectWithAABBv2(rayData, subMesh.bounds);
                if (hit.intersects())
                    result = QSSGRenderRay::createIntersectionResult(rayData, hit);
            }
            if (result.intersects && result.rayLengthSquared < minRayLength) {
                intersectionResult = result;
                minRayLength = intersectionResult.rayLengthSquared;
                resultSubset = subset;
            }
            subset++;
        }

        if (intersectionResult.intersects)
            outIntersectionResultList.push_back(QSSGRenderPickResult { &model,
                                                                       intersectionResult.rayLengthSquared,
                                                                       intersectionResult.relXY,
                                                                       intersectionResult.scenePosition,
                                                                       intersectionResult.localPosition,
                                                                       intersectionResult.faceNormal,
                                                                       resultSubset,
                                                                       instanceIndex
                                                });
    }
}

void QSSGRendererPrivate::intersectRayWithItem2D(const QSSGRenderRay &inRay, const QSSGRenderItem2D &item2D, PickResultList &outIntersectionResultList)
{
    // Get the plane (and normal) that the item 2D is on
    const QVector3D p0 = item2D.getGlobalPos();
    const QVector3D normal  = -item2D.getDirection();

    const float d = QVector3D::dotProduct(inRay.direction, normal);
    float intersectionTime = 0;
    if (d > 1e-6f) {
        const QVector3D p0l0 = p0 - inRay.origin;
        intersectionTime = QVector3D::dotProduct(p0l0, normal) / d;
        if (intersectionTime >= 0) {
            // Intersection
            const QVector3D intersectionPoint = inRay.origin + inRay.direction * intersectionTime;
            const QMatrix4x4 inverseGlobalTransform = item2D.globalTransform.inverted();
            const QVector3D localIntersectionPoint = QSSGUtils::mat44::transform(inverseGlobalTransform, intersectionPoint);
            const QVector2D qmlCoordinate(localIntersectionPoint.x(), -localIntersectionPoint.y());
            outIntersectionResultList.push_back(QSSGRenderPickResult { &item2D,
                                                                       intersectionTime * intersectionTime,
                                                                       qmlCoordinate,
                                                                       intersectionPoint,
                                                                       localIntersectionPoint,
                                                                       -normal });
        }
    }
}

QSSGRhiShaderPipelinePtr QSSGRendererPrivate::getShaderPipelineForDefaultMaterial(QSSGRenderer &renderer,
                                                                                  QSSGSubsetRenderable &inRenderable,
                                                                                  const QSSGShaderFeatures &inFeatureSet)
{
    auto *m_currentLayer = renderer.m_currentLayer;
    QSSG_ASSERT(m_currentLayer != nullptr, return {});

    // This function is the main entry point for retrieving the shaders for a
    // default material, and is called for every material for every model in
    // every frame. Therefore, like with custom materials, employ a first level
    // cache (a simple hash table), with a key that's quick to
    // generate/hash/compare. Even though there are other levels of caching in
    // the components that get invoked from here, those may not be suitable
    // performance wise. So bail out right here as soon as possible.
    auto &shaderMap = m_currentLayer->shaderMap;

    QElapsedTimer timer;
    timer.start();

    QSSGRhiShaderPipelinePtr shaderPipeline;

    // This just references inFeatureSet and inRenderable.shaderDescription -
    // cheap to construct and is good enough for the find()
    QSSGShaderMapKey skey = QSSGShaderMapKey(QByteArray(),
                                             inFeatureSet,
                                             inRenderable.shaderDescription);
    auto it = shaderMap.find(skey);
    if (it == shaderMap.end()) {
        Q_TRACE_SCOPE(QSSG_generateShader);
        Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DGenerateShader);
        shaderPipeline = QSSGRendererPrivate::generateRhiShaderPipeline(renderer, inRenderable, inFeatureSet);
        Q_QUICK3D_PROFILE_END_WITH_ID(QQuick3DProfiler::Quick3DGenerateShader, 0, inRenderable.material.profilingId);
        // make skey useable as a key for the QHash (makes a copy of the materialKey, instead of just referencing)
        skey.detach();
        // insert it no matter what, no point in trying over and over again
        shaderMap.insert(skey, shaderPipeline);
    } else {
        shaderPipeline = it.value();
    }

    if (shaderPipeline != nullptr) {
        if (m_currentLayer && !m_currentLayer->renderedCameras.isEmpty())
            m_currentLayer->ensureCachedCameraDatas();
    }

    const auto &rhiContext = renderer.m_contextInterface->rhiContext();
    QSSGRhiContextStats::get(*rhiContext).registerMaterialShaderGenerationTime(timer.elapsed());

    return shaderPipeline;
}

QT_END_NAMESPACE
