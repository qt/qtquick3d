/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtQuick3DRuntimeRender/private/qssgrenderer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercontextcore_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercamera_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderlight_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderimage_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderbuffermanager_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercontextcore_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendereffect_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderresourcemanager_p.h>
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

#ifdef Q_CC_MSVC
#pragma warning(disable : 4355)
#endif

// Quick tests you can run to find performance problems

//#define QSSG_RENDER_DISABLE_HARDWARE_BLENDING 1
//#define QSSG_RENDER_DISABLE_LIGHTING 1
//#define QSSG_RENDER_DISABLE_TEXTURING 1
//#define QSSG_RENDER_DISABLE_TRANSPARENCY 1
//#define QSSG_RENDER_DISABLE_FRUSTUM_CULLING 1

// If you are fillrate bound then sorting opaque objects can help in some circumstances
//#define QSSG_RENDER_DISABLE_OPAQUE_SORT 1

QT_BEGIN_NAMESPACE

struct QSSGRenderableImage;
struct QSSGSubsetRenderable;

void QSSGRenderer::releaseResources()
{
    delete m_rhiQuadRenderer; // TODO: pointer to incomplete type!
    m_rhiQuadRenderer = nullptr;
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

bool QSSGRenderer::prepareLayerForRender(QSSGRenderLayer &inLayer,
                                         const QSize &surfaceSize)
{
    QSSGLayerRenderData *theRenderData = getOrCreateLayerRenderData(inLayer);
    Q_ASSERT(theRenderData);
    theRenderData->prepareForRender(surfaceSize);
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
        } else if (resource->type == QSSGRenderGraphObject::Type::Image) {
            auto image = static_cast<QSSGRenderImage*>(resource);
            if (!image->m_qsgTexture) {
                bufferManager->removeImageReference(image->m_imagePath, image);
            }
        } else if (resource->type == QSSGRenderGraphObject::Type::Model) {
            auto model = static_cast<QSSGRenderModel*>(resource);
            if (!model->geometry)
                bufferManager->removeMeshReference(model->meshPath, model);
            else // Models with geometry should be cleaned up here
                rhi->cleanupDrawCallData(model);
        } else if (resource->type == QSSGRenderGraphObject::Type::TextureData) {
            auto textureData = static_cast<QSSGRenderTextureData *>(resource);
            bufferManager->releaseTextureData(textureData);
        }

        // ### There might be more types that need to be supported

        delete resource;
    }
    // Now check for unreferenced buffers and release them if necessary
    bufferManager->cleanupUnreferencedBuffers();
    resources.clear();
}

QSSGRenderLayer *QSSGRenderer::layerForNode(const QSSGRenderNode &inNode) const
{
    if (inNode.type == QSSGRenderGraphObject::Type::Layer)
        return &const_cast<QSSGRenderLayer &>(static_cast<const QSSGRenderLayer &>(inNode));

    if (inNode.parent)
        return layerForNode(*inNode.parent);

    return nullptr;
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

void QSSGRenderer::removeLastFrameLayer(QSSGLayerRenderPreparationData *layerData)
{
    m_lastFrameLayers.removeAll(layerData);
}

static QByteArray logPrefix() { return QByteArrayLiteral("mesh default material pipeline-- "); }


QSSGRef<QSSGRhiShaderPipeline> QSSGRenderer::generateRhiShaderPipelineImpl(QSSGSubsetRenderable &renderable,
                                                                           const QSSGRef<QSSGShaderLibraryManager> &shaderLibraryManager,
                                                                           const QSSGRef<QSSGShaderCache> &shaderCache,
                                                                           const QSSGRef<QSSGProgramGenerator> &shaderProgramGenerator,
                                                                           QSSGShaderDefaultMaterialKeyProperties &shaderKeyProperties,
                                                                           const ShaderFeatureSetList &featureSet,
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
                                        renderable.defaultMaterial().adapter,
                                        renderable.boneGlobals,
                                        renderable.boneNormals,
                                        renderable.morphWeights);

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
                                                                       const ShaderFeatureSetList &inFeatureSet)
{
    const QSSGRef<QSSGShaderCache> &theCache = m_contextInterface->shaderCache();
    const auto &shaderProgramGenerator = contextInterface()->shaderProgramGenerator();
    const auto &shaderLibraryManager = contextInterface()->shaderLibraryManager();
    return generateRhiShaderPipelineImpl(inRenderable, shaderLibraryManager, theCache, shaderProgramGenerator, m_defaultMaterialShaderKeyProperties, inFeatureSet, m_generatedShaderString);
}

void QSSGRenderer::beginFrame()
{
    for (int idx = 0, end = m_lastFrameLayers.size(); idx < end; ++idx)
        m_lastFrameLayers[idx]->resetForFrame();
    m_lastFrameLayers.clear();

    QSSGRHICTX_STAT(m_contextInterface->rhiContext().data(), start(this));
}

void QSSGRenderer::endFrame()
{
    // We need to do this endFrame(), as the material nodes might not exist after this!
    for (auto *matObj : qAsConst(m_materialClearDirty)) {
        if (matObj->type == QSSGRenderGraphObject::Type::CustomMaterial) {
            static_cast<QSSGRenderCustomMaterial *>(matObj)->updateDirtyForFrame();
        } else if (matObj->type == QSSGRenderGraphObject::Type::DefaultMaterial
                   || matObj->type == QSSGRenderGraphObject::Type::PrincipledMaterial) {
            static_cast<QSSGRenderDefaultMaterial *>(matObj)->dirty.updateDirtyForFrame();
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
    if (layer.flags.testFlag(QSSGRenderLayer::Flag::Active)) {
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
    if (layer.flags.testFlag(QSSGRenderLayer::Flag::Active)) {
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

inline bool pickResultLessThan(const QSSGRenderPickResult &lhs, const QSSGRenderPickResult &rhs)
{
    return lhs.m_distanceSq < rhs.m_distanceSq;
}

void QSSGRenderer::setGlobalPickingEnabled(bool isEnabled)
{
    m_globalPickingEnabled = isEnabled;
}

QSSGPickResultProcessResult QSSGRenderer::processPickResultList(bool inPickEverything)
{
    Q_UNUSED(inPickEverything);
    if (m_lastPickResults.empty())
        return QSSGPickResultProcessResult();
    // Things are rendered in a particular order and we need to respect that ordering.
    std::stable_sort(m_lastPickResults.begin(), m_lastPickResults.end(), pickResultLessThan);

    // We need to pick against sub objects basically somewhat recursively
    // but if we don't hit any sub objects and the parent isn't pickable then
    // we need to move onto the next item in the list.
    // We need to keep in mind that theQuery->Pick will enter this method in a later
    // stack frame so *if* we get to sub objects we need to pick against them but if the pick
    // completely misses *and* the parent object locally pickable is false then we need to move
    // onto the next object.

    const int numToCopy = m_lastPickResults.size();
    Q_ASSERT(numToCopy >= 0);
    size_t numCopyBytes = size_t(numToCopy) * sizeof(QSSGRenderPickResult);
    QSSGRenderPickResult *thePickResults = reinterpret_cast<QSSGRenderPickResult *>(
            m_contextInterface->perFrameAllocator().allocate(numCopyBytes));
    ::memcpy(thePickResults, m_lastPickResults.data(), numCopyBytes);
    m_lastPickResults.clear();
    QSSGPickResultProcessResult thePickResult(thePickResults[0]);
    return thePickResult;
}

QSSGRhiQuadRenderer *QSSGRenderer::rhiQuadRenderer()
{
    if (!m_contextInterface->rhiContext()->isValid())
        return nullptr;

    if (!m_rhiQuadRenderer)
        m_rhiQuadRenderer = new QSSGRhiQuadRenderer;

    return m_rhiQuadRenderer;
}

void QSSGRenderer::layerNeedsFrameClear(QSSGLayerRenderData &inLayer)
{
    m_lastFrameLayers.push_back(&inLayer);
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
        if (inPickEverything || pickableObject->flags.testFlag(QSSGRenderNode::Flag::LocallyPickable))
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
    auto mesh = bufferManager->getMesh(model.meshPath);
    if (!mesh) {
        // Check if there is custom geometry before bailing out
        if (model.geometry)
            mesh = bufferManager->getMesh(model.geometry);

        // If there is still no geometry bail out
        if (!mesh)
            return;
    }

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
                for (const auto &subMeshResult : qAsConst(results)) {
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

    outIntersectionResultList.push_back(
                QSSGRenderPickResult(model,
                                     intersectionResult.rayLengthSquared,
                                     intersectionResult.relXY,
                                     intersectionResult.scenePosition,
                                     intersectionResult.localPosition,
                                     intersectionResult.faceNormal,
                                     resultSubset));
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
            outIntersectionResultList.push_back(QSSGRenderPickResult(item2D,
                                                                     intersectionTime * intersectionTime,
                                                                     qmlCoordinate,
                                                                     intersectionPoint,
                                                                     localIntersectionPoint,
                                                                     normal));
        }
    }
}

QSSGRef<QSSGRhiShaderPipeline> QSSGRenderer::getRhiShaders(QSSGSubsetRenderable &inRenderable,
                                                           const ShaderFeatureSetList &inFeatureSet)
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
        shaderPipeline = generateRhiShaderPipeline(inRenderable, inFeatureSet);
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
                                              theData.m_rhiDepthTexture.texture,
                                              theData.m_rhiAoTexture.texture,
                                              theData.m_rhiScreenTexture.texture,
                                              theLayer.lightProbe,
                                              theLayer.probeHorizon,
                                              theLayer.probeExposure,
                                              theLayer.probeOrientation,
                                              isYUpInFramebuffer,
                                              isYUpInNDC,
                                              isClipDepthZeroToOne};
}

const QSSGRef<QSSGProgramGenerator> &QSSGRenderer::getProgramGenerator()
{
    return m_contextInterface->shaderProgramGenerator();
}

QSSGRenderPickResult::QSSGRenderPickResult(const QSSGRenderGraphObject &inHitObject,
                                           float inCameraDistance,
                                           const QVector2D &inLocalUVCoords,
                                           const QVector3D &scenePosition,
                                           const QVector3D &inLocalPosition,
                                           const QVector3D &faceNormal,
                                           int subset)
    : m_hitObject(&inHitObject)
    , m_distanceSq(inCameraDistance)
    , m_localUVCoords(inLocalUVCoords)
    , m_scenePosition(scenePosition)
    , m_localPosition(inLocalPosition)
    , m_faceNormal(faceNormal.normalized())
    , m_subset(subset)
{
}

QT_END_NAMESPACE
