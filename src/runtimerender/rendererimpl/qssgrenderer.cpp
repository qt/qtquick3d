// Copyright (C) 2008-2012 NVIDIA Corporation.
// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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

#include <QtQuick3DUtils/private/qssgdataref_p.h>
#include <QtQuick3DUtils/private/qssgutils_p.h>

#include <QtCore/QMutexLocker>

#include <cstdlib>
#include <algorithm>
#include <limits>

QT_BEGIN_NAMESPACE

struct QSSGRenderableImage;
struct QSSGSubsetRenderable;

void QSSGRenderer::releaseResources()
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
    releaseResources();
}

void QSSGRenderer::setRenderContextInterface(QSSGRenderContextInterface *ctx)
{
    m_contextInterface = ctx;
}

bool QSSGRenderer::prepareLayerForRender(QSSGRenderLayer &inLayer)
{
    QSSGLayerRenderData *theRenderData = getOrCreateLayerRenderData(inLayer);
    Q_ASSERT(theRenderData);
    theRenderData->resetForFrame();
    theRenderData->prepareForRender();
    return theRenderData->layerPrepResult->flags.wasDirty();
}

void QSSGRenderer::rhiPrepare(QSSGRenderLayer &inLayer)
{
    QSSGLayerRenderData *theRenderData = getOrCreateLayerRenderData(inLayer);
    Q_ASSERT(theRenderData);
    if (theRenderData->layerPrepResult->isLayerVisible())
        theRenderData->rhiPrepare();
}

void QSSGRenderer::rhiRender(QSSGRenderLayer &inLayer)
{
    QSSGLayerRenderData *theRenderData = getOrCreateLayerRenderData(inLayer);
    Q_ASSERT(theRenderData);
    if (theRenderData->layerPrepResult->isLayerVisible())
        theRenderData->rhiRender();
}

void QSSGRenderer::cleanupResources(QList<QSSGRenderGraphObject *> &resources)
{
    const auto &rhi = contextInterface()->rhiContext();
    if (!rhi->isValid())
        return;

    const auto &bufferManager = contextInterface()->bufferManager();

    for (auto resource : resources) {
        if (resource->type == QSSGRenderGraphObject::Type::Geometry) {
            auto geometry = static_cast<QSSGRenderGeometry*>(resource);
            bufferManager->releaseGeometry(geometry);
        } else if (resource->type == QSSGRenderGraphObject::Type::Model) {
            auto model = static_cast<QSSGRenderModel*>(resource);
            // release the texture for skinning before remove a model
            if (model->boneTexture)
                rhi->releaseTexture(model->boneTexture);
            rhi->cleanupDrawCallData(model);
        } else if (resource->type == QSSGRenderGraphObject::Type::TextureData) {
            auto textureData = static_cast<QSSGRenderTextureData *>(resource);
            bufferManager->releaseTextureData(textureData);
        }

        // ### There might be more types that need to be supported

        delete resource;
    }
    resources.clear();
}

QSSGLayerRenderData *QSSGRenderer::getOrCreateLayerRenderData(QSSGRenderLayer &layer)
{
    if (layer.renderData == nullptr)
        layer.renderData = new QSSGLayerRenderData(layer, this);

    return layer.renderData;
}

void QSSGRenderer::addMaterialDirtyClear(QSSGRenderGraphObject *material)
{
    m_materialClearDirty.insert(material);
}

static QByteArray logPrefix() { return QByteArrayLiteral("mesh default material pipeline-- "); }


QSSGRef<QSSGRhiShaderPipeline> QSSGRenderer::generateRhiShaderPipelineImpl(QSSGSubsetRenderable &renderable,
                                                                           const QSSGRef<QSSGShaderLibraryManager> &shaderLibraryManager,
                                                                           const QSSGRef<QSSGShaderCache> &shaderCache,
                                                                           const QSSGRef<QSSGProgramGenerator> &shaderProgramGenerator,
                                                                           QSSGShaderDefaultMaterialKeyProperties &shaderKeyProperties,
                                                                           const QSSGShaderFeatures &featureSet,
                                                                           QByteArray &shaderString)
{
    shaderString = logPrefix();
    QSSGShaderDefaultMaterialKey theKey(renderable.shaderDescription);

    // This is not a cheap operation. This function assumes that it will not be
    // hit for every material for every model in every frame (except of course
    // for materials that got changed).
    theKey.toString(shaderString, shaderKeyProperties);

    // Check if there's a pre-built shader for available for this shader
    const auto key = QSSGShaderCacheKey::hashString(shaderString, featureSet);
    const auto hkey = QSSGShaderCacheKey::generateHashCode(shaderString, featureSet);
    const auto &shaderEntries = shaderLibraryManager->m_shaderEntries;
    const auto foundIt = shaderEntries.constFind(QQsbCollection::Entry{hkey});
    if (foundIt != shaderEntries.cend())
        return shaderCache->loadGeneratedShader(key, *foundIt);

    const QSSGRef<QSSGRhiShaderPipeline> &cachedShaders = shaderCache->getRhiShaderPipeline(shaderString, featureSet);
    if (cachedShaders)
        return cachedShaders;

    QSSGMaterialVertexPipeline pipeline(shaderProgramGenerator,
                                        shaderKeyProperties,
                                        renderable.defaultMaterial().adapter);

    return QSSGMaterialShaderGenerator::generateMaterialRhiShader(logPrefix(),
                                                                  pipeline,
                                                                  renderable.shaderDescription,
                                                                  shaderKeyProperties,
                                                                  featureSet,
                                                                  renderable.material,
                                                                  renderable.lights,
                                                                  renderable.firstImage,
                                                                  shaderLibraryManager,
                                                                  shaderCache);
}

QSSGRef<QSSGRhiShaderPipeline> QSSGRenderer::generateRhiShaderPipeline(QSSGSubsetRenderable &inRenderable,
                                                                       const QSSGShaderFeatures &inFeatureSet)
{
    const QSSGRef<QSSGShaderCache> &theCache = m_contextInterface->shaderCache();
    const auto &shaderProgramGenerator = contextInterface()->shaderProgramGenerator();
    const auto &shaderLibraryManager = contextInterface()->shaderLibraryManager();
    return generateRhiShaderPipelineImpl(inRenderable, shaderLibraryManager, theCache, shaderProgramGenerator, m_defaultMaterialShaderKeyProperties, inFeatureSet, m_generatedShaderString);
}

void QSSGRenderer::beginFrame()
{
    QSSGRHICTX_STAT(m_contextInterface->rhiContext().data(), start(this));
}

void QSSGRenderer::endFrame()
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

    QSSGRHICTX_STAT(m_contextInterface->rhiContext().data(), stop());
}

QSSGRenderer::PickResultList QSSGRenderer::syncPickAll(const QSSGRenderLayer &layer,
                                                       const QSSGRef<QSSGBufferManager> &bufferManager,
                                                       const QSSGRenderRay &ray)
{
    PickResultList pickResults;
    if (layer.getLocalState(QSSGRenderLayer::LocalState::Active)) {
        getLayerHitObjectList(layer, bufferManager, ray, m_globalPickingEnabled, pickResults);
        // Things are rendered in a particular order and we need to respect that ordering.
        std::stable_sort(pickResults.begin(), pickResults.end(), [](const QSSGRenderPickResult &lhs, const QSSGRenderPickResult &rhs) {
            return lhs.m_distanceSq < rhs.m_distanceSq;
        });
    }
    return pickResults;
}

QSSGRenderPickResult QSSGRenderer::syncPick(const QSSGRenderLayer &layer,
                                            const QSSGRef<QSSGBufferManager> &bufferManager,
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

    PickResultList pickResults;
    if (layer.getLocalState(QSSGRenderLayer::LocalState::Active)) {
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

void QSSGRenderer::beginLayerDepthPassRender(QSSGLayerRenderData &inLayer)
{
    m_currentLayer = &inLayer;
}

void QSSGRenderer::endLayerDepthPassRender()
{
    m_currentLayer = nullptr;
}

void QSSGRenderer::beginLayerRender(QSSGLayerRenderData &inLayer)
{
    m_currentLayer = &inLayer;
}
void QSSGRenderer::endLayerRender()
{
    m_currentLayer = nullptr;
}

bool QSSGRenderer::rendererRequestsFrames() const
{
    return m_progressiveAARenderRequest;
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
                                         const QSSGRef<QSSGBufferManager> &bufferManager,
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

void QSSGRenderer::intersectRayWithSubsetRenderable(const QSSGRef<QSSGBufferManager> &bufferManager,
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
    QMutexLocker mutexLocker(bufferManager->meshUpdateMutex());
    auto mesh = bufferManager->getMeshForPicking(model);
    if (!mesh)
        return;

    const auto &globalTransform = model.globalTransform;
    auto rayData = QSSGRenderRay::createRayData(globalTransform, inRay);
    const auto &subMeshes = mesh->subsets;
    QSSGBounds3 modelBounds;
    for (const auto &subMesh : subMeshes)
        modelBounds.include(subMesh.bounds);

    if (modelBounds.isEmpty())
        return;

    auto hit = QSSGRenderRay::intersectWithAABBv2(rayData, modelBounds);

    // If we don't intersect with the model at all, then there's no need to go furher down!
    if (!hit.intersects())
        return;

    // Check each submesh to find the closest intersection point
    float minRayLength = std::numeric_limits<float>::max();
    QSSGRenderRay::IntersectionResult intersectionResult;
    QVector<QSSGRenderRay::IntersectionResult> results;

    int subset = 0;
    int resultSubset = 0;
    for (const auto &subMesh : subMeshes) {
        QSSGRenderRay::IntersectionResult result;
        if (subMesh.bvhRoot) {
            hit = QSSGRenderRay::intersectWithAABBv2(rayData, subMesh.bvhRoot->boundingData);
            if (hit.intersects()) {
                results.clear();
                inRay.intersectWithBVH(rayData, subMesh.bvhRoot, mesh, results);
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

    if (!intersectionResult.intersects)
        return;

    outIntersectionResultList.push_back(QSSGRenderPickResult { &model,
                                                               intersectionResult.rayLengthSquared,
                                                               intersectionResult.relXY,
                                                               intersectionResult.scenePosition,
                                                               intersectionResult.localPosition,
                                                               intersectionResult.faceNormal,
                                                               resultSubset });
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
            const QVector3D localIntersectionPoint = mat44::transform(inverseGlobalTransform, intersectionPoint);
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

QSSGRef<QSSGRhiShaderPipeline> QSSGRenderer::getRhiShaders(QSSGSubsetRenderable &inRenderable,
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

    QSSGRef<QSSGRhiShaderPipeline> shaderPipeline;

    // This just references inFeatureSet and inRenderable.shaderDescription -
    // cheap to construct and is good enough for the find()
    QSSGShaderMapKey skey = QSSGShaderMapKey(QByteArray(),
                                             inFeatureSet,
                                             inRenderable.shaderDescription);
    auto it = m_shaderMap.find(skey);
    if (it == m_shaderMap.end()) {
        Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DGenerateShader);
        shaderPipeline = generateRhiShaderPipeline(inRenderable, inFeatureSet);
        Q_QUICK3D_PROFILE_END(QQuick3DProfiler::Quick3DGenerateShader);
        // make skey useable as a key for the QHash (makes copies of materialKey and featureSet, instead of just referencing)
        skey.detach();
        // insert it no matter what, no point in trying over and over again
        m_shaderMap.insert(skey, shaderPipeline);
    } else {
        shaderPipeline = it.value();
    }

    if (!shaderPipeline.isNull()) {
        if (m_currentLayer && m_currentLayer->camera) {
            QSSGRenderCamera &theCamera(*m_currentLayer->camera);
            if (!m_currentLayer->cameraDirection.hasValue())
                m_currentLayer->cameraDirection = theCamera.getScalingCorrectDirection();
        }
    }
    return shaderPipeline;
}

QSSGLayerGlobalRenderProperties QSSGRenderer::getLayerGlobalRenderProperties()
{
    QSSGLayerRenderData &theData = *m_currentLayer;
    const QSSGRenderLayer &theLayer = theData.layer;
    if (!theData.cameraDirection.hasValue())
        theData.cameraDirection = theData.camera->getScalingCorrectDirection();

    bool isYUpInFramebuffer = true;
    bool isYUpInNDC = true;
    bool isClipDepthZeroToOne = true;
    if (m_contextInterface->rhiContext()->isValid()) {
        QRhi *rhi = m_contextInterface->rhiContext()->rhi();
        isYUpInFramebuffer = rhi->isYUpInFramebuffer();
        isYUpInNDC = rhi->isYUpInNDC();
        isClipDepthZeroToOne = rhi->isClipDepthZeroToOne();
    }

    return QSSGLayerGlobalRenderProperties{ theLayer,
                                              *theData.camera,
                                              *theData.cameraDirection,
                                              theData.shadowMapManager,
                                              theData.rhiDepthTexture.texture,
                                              theData.rhiAoTexture.texture,
                                              theData.rhiScreenTexture.texture,
                                              theLayer.lightProbe,
                                              theLayer.probeHorizon,
                                              theLayer.probeExposure,
                                              theLayer.probeOrientation,
                                              isYUpInFramebuffer,
                                              isYUpInNDC,
                                              isClipDepthZeroToOne};
}

QT_END_NAMESPACE
