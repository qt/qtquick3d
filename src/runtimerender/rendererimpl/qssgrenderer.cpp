// Copyright (C) 2008-2012 NVIDIA Corporation.
// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtQuick3DRuntimeRender/private/qssgrenderitem2d_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercontextcore_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercamera_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderlight_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderimage_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderbuffermanager_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercontextcore_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendereffect_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrhicustommaterialsystem_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercodegenerator_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderdefaultmaterialshadergenerator_p.h>
#include <QtQuick3DRuntimeRender/private/qssgperframeallocator_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrhiquadrenderer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendertexturedata_p.h>
#include <QtQuick3DRuntimeRender/private/qssglayerrenderdata_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrhiparticles_p.h>

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

QT_BEGIN_NAMESPACE

struct QSSGRenderableImage;
struct QSSGSubsetRenderable;

void QSSGRenderer::releaseCachedResources()
{
    delete m_rhiQuadRenderer;
    m_rhiQuadRenderer = nullptr;
    delete m_rhiCubeRenderer;
    m_rhiCubeRenderer = nullptr;
}

QSSGRenderer::QSSGRenderer() = default;

QSSGRenderer::~QSSGRenderer()
{
    m_contextInterface = nullptr;
    releaseCachedResources();
}

void QSSGRenderer::setRenderContextInterface(QSSGRenderContextInterface *ctx)
{
    m_contextInterface = ctx;
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
    QSSG_ASSERT(theRenderData && theRenderData->camera, return);

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
    QSSG_ASSERT(theRenderData && theRenderData->camera, return);
    if (theRenderData->layerPrepResult.isLayerVisible()) {
        QSSG_ASSERT(theRenderData->camera, return);
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
    const auto &rhi = rci.rhiContext();
    if (!rhi->isValid())
        return;

    const auto &bufferManager = rci.bufferManager();

    for (const auto &resource : resources) {
        if (resource->type == QSSGRenderGraphObject::Type::Geometry) {
            auto geometry = static_cast<QSSGRenderGeometry*>(resource);
            bufferManager->releaseGeometry(geometry);
        } else if (resource->type == QSSGRenderGraphObject::Type::Model) {
            auto model = static_cast<QSSGRenderModel*>(resource);
            rhi->cleanupDrawCallData(model);
        } else if (resource->type == QSSGRenderGraphObject::Type::TextureData) {
            auto textureData = static_cast<QSSGRenderTextureData *>(resource);
            bufferManager->releaseTextureData(textureData);
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


QSSGRhiShaderPipelinePtr QSSGRenderer::generateRhiShaderPipelineImpl(QSSGSubsetRenderable &renderable,
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

QSSGRhiShaderPipelinePtr QSSGRenderer::generateRhiShaderPipeline(QSSGSubsetRenderable &inRenderable,
                                                                 const QSSGShaderFeatures &inFeatureSet)
{
    const auto &theCache = m_contextInterface->shaderCache();
    const auto &shaderProgramGenerator = contextInterface()->shaderProgramGenerator();
    const auto &shaderLibraryManager = contextInterface()->shaderLibraryManager();
    return generateRhiShaderPipelineImpl(inRenderable, *shaderLibraryManager, *theCache, *shaderProgramGenerator, m_currentLayer->defaultMaterialShaderKeyProperties, inFeatureSet, m_generatedShaderString);
}

void QSSGRenderer::beginFrame(QSSGRenderLayer *layer)
{
    QSSGRHICTX_STAT(m_contextInterface->rhiContext().get(), start(layer));
}

void QSSGRenderer::endFrame(QSSGRenderLayer *layer)
{
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

    QSSGRHICTX_STAT(m_contextInterface->rhiContext().get(), stop(layer));
}

QSSGRenderer::PickResultList QSSGRenderer::syncPickAll(const QSSGRenderLayer &layer,
                                                       QSSGBufferManager &bufferManager,
                                                       const QSSGRenderRay &ray)
{
    PickResultList pickResults;
    Q_ASSERT(layer.getGlobalState(QSSGRenderNode::GlobalState::Active));
    getLayerHitObjectList(layer, bufferManager, ray, m_globalPickingEnabled, pickResults);
    // Things are rendered in a particular order and we need to respect that ordering.
    std::stable_sort(pickResults.begin(), pickResults.end(), [](const QSSGRenderPickResult &lhs, const QSSGRenderPickResult &rhs) {
        return lhs.m_distanceSq < rhs.m_distanceSq;
    });
    return pickResults;
}

QSSGRenderPickResult QSSGRenderer::syncPick(const QSSGRenderLayer &layer,
                                            QSSGBufferManager &bufferManager,
                                            const QSSGRenderRay &ray,
                                            QSSGRenderNode *target)
{
    static const auto processResults = [](PickResultList &pickResults) {
        if (pickResults.empty())
            return QSSGPickResultProcessResult();
        // Things are rendered in a particular order and we need to respect that ordering.
        std::stable_sort(pickResults.begin(), pickResults.end(), [](const QSSGRenderPickResult &lhs, const QSSGRenderPickResult &rhs) {
            return lhs.m_distanceSq < rhs.m_distanceSq;
        });
        return QSSGPickResultProcessResult{ pickResults.at(0), true };
    };

    Q_ASSERT(layer.getGlobalState(QSSGRenderNode::GlobalState::Active));
    PickResultList pickResults;
    if (target) {
        // Pick against only one target
        intersectRayWithSubsetRenderable(bufferManager, ray, *target, pickResults);
        return processResults(pickResults);
    } else {
        getLayerHitObjectList(layer, bufferManager, ray, m_globalPickingEnabled, pickResults);
        QSSGPickResultProcessResult retval = processResults(pickResults);
        if (retval.m_wasPickConsumed)
            return retval;
    }

    return QSSGPickResultProcessResult();
}

bool QSSGRenderer::isGlobalPickingEnabled() const
{
    return m_globalPickingEnabled;
}

void QSSGRenderer::setGlobalPickingEnabled(bool isEnabled)
{
    m_globalPickingEnabled = isEnabled;
}

QSSGRhiQuadRenderer *QSSGRenderer::rhiQuadRenderer()
{
    if (!m_contextInterface->rhiContext()->isValid())
        return nullptr;

    if (!m_rhiQuadRenderer)
        m_rhiQuadRenderer = new QSSGRhiQuadRenderer;

    return m_rhiQuadRenderer;
}

QSSGRhiCubeRenderer *QSSGRenderer::rhiCubeRenderer()
{
    if (!m_contextInterface->rhiContext()->isValid())
        return nullptr;

    if (!m_rhiCubeRenderer)
        m_rhiCubeRenderer = new QSSGRhiCubeRenderer;

    return m_rhiCubeRenderer;

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

void QSSGRenderer::getLayerHitObjectList(const QSSGRenderLayer &layer,
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

void QSSGRenderer::intersectRayWithSubsetRenderable(QSSGBufferManager &bufferManager,
                                                    const QSSGRenderRay &inRay,
                                                    const QSSGRenderNode &node,
                                                    QSSGRenderer::PickResultList &outIntersectionResultList)
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

void QSSGRenderer::intersectRayWithItem2D(const QSSGRenderRay &inRay, const QSSGRenderItem2D &item2D, QSSGRenderer::PickResultList &outIntersectionResultList)
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
                                                                       normal });
        }
    }
}

QSSGRhiShaderPipelinePtr QSSGRenderer::getShaderPipelineForDefaultMaterial(QSSGSubsetRenderable &inRenderable,
                                                                           const QSSGShaderFeatures &inFeatureSet)
{
    if (Q_UNLIKELY(m_currentLayer == nullptr)) {
        Q_ASSERT(false);
        return nullptr;
    }

    // This function is the main entry point for retrieving the shaders for a
    // default material, and is called for every material for every model in
    // every frame. Therefore, like with custom materials, employ a first level
    // cache (a simple hash table), with a key that's quick to
    // generate/hash/compare. Even though there are other levels of caching in
    // the components that get invoked from here, those may not be suitable
    // performance wise. So bail out right here as soon as possible.

    QElapsedTimer timer;
    timer.start();

    QSSGRhiShaderPipelinePtr shaderPipeline;

    // This just references inFeatureSet and inRenderable.shaderDescription -
    // cheap to construct and is good enough for the find()
    QSSGShaderMapKey skey = QSSGShaderMapKey(QByteArray(),
                                             inFeatureSet,
                                             inRenderable.shaderDescription);
    auto it = m_shaderMap.find(skey);
    if (it == m_shaderMap.end()) {
        Q_TRACE_SCOPE(QSSG_generateShader);
        Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DGenerateShader);
        shaderPipeline = generateRhiShaderPipeline(inRenderable, inFeatureSet);
        Q_QUICK3D_PROFILE_END_WITH_ID(QQuick3DProfiler::Quick3DGenerateShader, 0, inRenderable.material.profilingId);
        // make skey useable as a key for the QHash (makes a copy of the materialKey, instead of just referencing)
        skey.detach();
        // insert it no matter what, no point in trying over and over again
        m_shaderMap.insert(skey, shaderPipeline);
    } else {
        shaderPipeline = it.value();
    }

    if (shaderPipeline != nullptr) {
        if (m_currentLayer && m_currentLayer->camera) {
            if (!m_currentLayer->cameraData.has_value())
                [[maybe_unused]] const auto cd = m_currentLayer->getCachedCameraData();
        }
    }

    m_contextInterface->rhiContext()->stats().registerMaterialShaderGenerationTime(timer.elapsed());

    return shaderPipeline;
}

QSSGLayerGlobalRenderProperties QSSGRenderer::getLayerGlobalRenderProperties()
{
    QSSGLayerRenderData &theData = *m_currentLayer;
    const QSSGRenderLayer &theLayer = theData.layer;
    if (!theData.cameraData.has_value() && theData.camera) // NOTE: Ensure we have a valid value!
        [[maybe_unused]] const auto cd = theData.getCachedCameraData();

    bool isYUpInFramebuffer = true;
    bool isYUpInNDC = true;
    bool isClipDepthZeroToOne = true;
    if (m_contextInterface->rhiContext()->isValid()) {
        QRhi *rhi = m_contextInterface->rhiContext()->rhi();
        isYUpInFramebuffer = rhi->isYUpInFramebuffer();
        isYUpInNDC = rhi->isYUpInNDC();
        isClipDepthZeroToOne = rhi->isClipDepthZeroToOne();
    }

    const QSSGRhiRenderableTexture *depthTexture = theData.getRenderResult(QSSGFrameData::RenderResult::DepthTexture);
    const QSSGRhiRenderableTexture *ssaoTexture = theData.getRenderResult(QSSGFrameData::RenderResult::AoTexture);
    const QSSGRhiRenderableTexture *screenTexture = theData.getRenderResult(QSSGFrameData::RenderResult::ScreenTexture);

    return QSSGLayerGlobalRenderProperties{ theLayer,
                                              *theData.camera,
                                              theData.cameraData.value(), // ensured/checked further up in this function
                                              theData.getShadowMapManager().get(),
                                              depthTexture->texture,
                                              ssaoTexture->texture,
                                              screenTexture->texture,
                                              theLayer.lightProbe,
                                              theLayer.probeHorizon,
                                              theLayer.probeExposure,
                                              theLayer.probeOrientation,
                                              isYUpInFramebuffer,
                                              isYUpInNDC,
                                              isClipDepthZeroToOne};
}

void QSSGRenderer::setTonemapFeatures(QSSGShaderFeatures &features, QSSGRenderLayer::TonemapMode tonemapMode)
{
    features.set(QSSGShaderFeatures::Feature::LinearTonemapping,
                 tonemapMode == QSSGRenderLayer::TonemapMode::Linear);
    features.set(QSSGShaderFeatures::Feature::AcesTonemapping,
                 tonemapMode == QSSGRenderLayer::TonemapMode::Aces);
    features.set(QSSGShaderFeatures::Feature::HejlDawsonTonemapping,
                 tonemapMode == QSSGRenderLayer::TonemapMode::HejlDawson);
    features.set(QSSGShaderFeatures::Feature::FilmicTonemapping,
                 tonemapMode == QSSGRenderLayer::TonemapMode::Filmic);
}

QT_END_NAMESPACE
