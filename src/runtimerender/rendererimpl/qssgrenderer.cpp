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

#include <QtQuick3DUtils/private/qssgdataref_p.h>
#include <QtQuick3DUtils/private/qssgutils_p.h>

#include <cstdlib>
#include <algorithm>

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
                                        renderable.material.adapter,
                                        renderable.boneGlobals,
                                        renderable.boneNormals);

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
    for (auto *matObj : qAsConst(m_materialClearDirty)) {
        if (matObj->type == QSSGRenderGraphObject::Type::CustomMaterial)
            static_cast<QSSGRenderCustomMaterial *>(matObj)->updateDirtyForFrame();
        else if (matObj->type == QSSGRenderGraphObject::Type::DefaultMaterial)
            static_cast<QSSGRenderDefaultMaterial *>(matObj)->dirty.updateDirtyForFrame();
    }
    m_materialClearDirty.clear();
}

void QSSGRenderer::endFrame()
{
}

inline bool pickResultLessThan(const QSSGRenderPickResult &lhs, const QSSGRenderPickResult &rhs)
{
    return lhs.m_cameraDistanceSq < rhs.m_cameraDistanceSq;
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

QSSGRenderPickResult QSSGRenderer::pick(QSSGRenderLayer &inLayer,
                                        const QVector2D &inViewportDimensions,
                                        const QVector2D &inMouseCoords,
                                        bool inPickEverything)
{
    m_lastPickResults.clear();

    if (inLayer.flags.testFlag(QSSGRenderLayer::Flag::Active)) {
        if (auto renderData = inLayer.renderData) {
            m_lastPickResults.clear();
            getLayerHitObjectList(*renderData, inViewportDimensions, inMouseCoords, inPickEverything, m_lastPickResults);
            QSSGPickResultProcessResult retval(processPickResultList(inPickEverything));
            if (retval.m_wasPickConsumed)
                return retval;
        }
    }

    return QSSGRenderPickResult();
}

QSSGRenderPickResult QSSGRenderer::syncPick(const QSSGRenderLayer &layer,
                                                const QSSGRef<QSSGBufferManager> &bufferManager,
                                                const QVector2D &inViewportDimensions,
                                                const QVector2D &inMouseCoords)
{
    using PickResultList = QVarLengthArray<QSSGRenderPickResult, 20>; // Lets assume most items are filtered out already
    static const auto processResults = [](PickResultList &pickResults) {
        if (pickResults.empty())
            return QSSGPickResultProcessResult();
        // Things are rendered in a particular order and we need to respect that ordering.
        std::stable_sort(pickResults.begin(), pickResults.end(), [](const QSSGRenderPickResult &lhs, const QSSGRenderPickResult &rhs) {
            return lhs.m_cameraDistanceSq < rhs.m_cameraDistanceSq;
        });
        return QSSGPickResultProcessResult{ pickResults.at(0), true };
    };

    PickResultList pickResults;
    if (layer.flags.testFlag(QSSGRenderLayer::Flag::Active)) {
        getLayerHitObjectList(layer, bufferManager, inViewportDimensions, inMouseCoords, false, pickResults);
        QSSGPickResultProcessResult retval = processResults(pickResults);
        if (retval.m_wasPickConsumed)
            return retval;
    }

    return QSSGPickResultProcessResult();
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

//void QSSGRenderer::prepareImageForIbl(QSSGRenderImage &inImage)
//{
//    if (inImage.m_textureData.m_texture && inImage.m_textureData.m_texture->numMipmaps() < 1)
//        inImage.m_textureData.m_texture->generateMipmaps();
//}

QSSGOption<QVector2D> QSSGRenderer::getLayerMouseCoords(QSSGLayerRenderData &inLayerRenderData,
                                                                const QVector2D &inMouseCoords,
                                                                const QVector2D &inViewportDimensions,
                                                                bool forceImageIntersect) const
{
    if (inLayerRenderData.layerPrepResult.hasValue()) {
        const auto viewport = inLayerRenderData.layerPrepResult->viewport();
        return QSSGLayerRenderHelper::layerMouseCoords(viewport, inMouseCoords, inViewportDimensions, forceImageIntersect);
    }
    return QSSGEmpty();
}

bool QSSGRenderer::rendererRequestsFrames() const
{
    return m_progressiveAARenderRequest;
}

void QSSGRenderer::getLayerHitObjectList(QSSGLayerRenderData &inLayerRenderData,
                                               const QVector2D &inViewportDimensions,
                                               const QVector2D &inPresCoords,
                                               bool inPickEverything,
                                               TPickResultArray &outIntersectionResult)
{
    // This function assumes the layer was rendered to the scene itself. There is another
    // function for completely offscreen layers that don't get rendered to the scene.
    bool wasRenderToTarget(inLayerRenderData.layer.flags.testFlag(QSSGRenderLayer::Flag::LayerRenderToTarget));
    if (wasRenderToTarget && inLayerRenderData.camera != nullptr) {
        QSSGOption<QSSGRenderRay> theHitRay;
        if (inLayerRenderData.layerPrepResult.hasValue()) {
            const auto camera = inLayerRenderData.layerPrepResult->camera();
            const auto viewport = inLayerRenderData.layerPrepResult->viewport();
            theHitRay = QSSGLayerRenderHelper::pickRay(*camera, viewport, inPresCoords, inViewportDimensions, false);
        }

        if (theHitRay.hasValue()) {
            // Scale the mouse coords to change them into the camera's numerical space.
            QSSGRenderRay thePickRay = *theHitRay;
            for (int idx = inLayerRenderData.opaqueObjects.size(), end = 0; idx > end; --idx) {
                QSSGRenderableObject *theRenderableObject = inLayerRenderData.opaqueObjects.at(idx - 1).obj;
                if (inPickEverything || theRenderableObject->renderableFlags.isPickable())
                    intersectRayWithSubsetRenderable(thePickRay, *theRenderableObject, outIntersectionResult);
            }
            for (int idx = inLayerRenderData.transparentObjects.size(), end = 0; idx > end; --idx) {
                QSSGRenderableObject *theRenderableObject = inLayerRenderData.transparentObjects.at(idx - 1).obj;
                if (inPickEverything || theRenderableObject->renderableFlags.isPickable())
                    intersectRayWithSubsetRenderable(thePickRay, *theRenderableObject, outIntersectionResult);
            }
        }
    }
}

using RenderableList = QVarLengthArray<const QSSGRenderNode *>;
static void dfs(const QSSGRenderNode &node, RenderableList &renderables)
{
    if (node.isRenderableType())
        renderables.push_back(&node);

    for (const auto &child : node.children)
        dfs(child, renderables);
}

void QSSGRenderer::getLayerHitObjectList(const QSSGRenderLayer &layer,
                                             const QSSGRef<QSSGBufferManager> &bufferManager,
                                             const QVector2D &inViewportDimensions,
                                             const QVector2D &inPresCoords,
                                             bool inPickEverything,
                                             PickResultList &outIntersectionResult)
{

    // This function assumes the layer was rendered to the scene itself. There is another
    // function for completely offscreen layers that don't get rendered to the scene.
    const bool wasRenderToTarget(layer.flags.testFlag(QSSGRenderLayer::Flag::LayerRenderToTarget));
    if (wasRenderToTarget && layer.renderedCamera != nullptr) {
        const auto camera = layer.renderedCamera;
        // TODO: Need to make sure we get the right Viewport rect here.
        const auto viewport = QRectF(QPointF(), QSizeF(qreal(inViewportDimensions.x()), qreal(inViewportDimensions.y())));
        const QSSGOption<QSSGRenderRay> hitRay = QSSGLayerRenderHelper::pickRay(*camera, viewport, inPresCoords, inViewportDimensions, false);
        if (hitRay.hasValue()) {
            // Scale the mouse coords to change them into the camera's numerical space.
            RenderableList renderables;
            for (const auto &childNode : layer.children)
                dfs(childNode, renderables);

            for (int idx = renderables.size(), end = 0; idx > end; --idx) {
                const auto &pickableObject = renderables.at(idx - 1);
                if (inPickEverything || pickableObject->flags.testFlag(QSSGRenderNode::Flag::LocallyPickable))
                    intersectRayWithSubsetRenderable(bufferManager, *hitRay, *pickableObject, outIntersectionResult);
            }
        }
    }
}

void QSSGRenderer::intersectRayWithSubsetRenderable(const QSSGRef<QSSGBufferManager> &bufferManager,
                                                        const QSSGRenderRay &inRay,
                                                        const QSSGRenderNode &node,
                                                        QSSGRenderer::PickResultList &outIntersectionResultList)
{
    if (node.type != QSSGRenderGraphObject::Type::Model)
        return;

    const QSSGRenderModel &model = static_cast<const QSSGRenderModel &>(node);

    // TODO: Technically we should have some guard here, as the meshes are usually loaded on a different thread,
    // so this isn't really nice (assumes all meshes are loaded before picking and none are removed, which currently should be the case).
    auto mesh = bufferManager->getMesh(model.meshPath);
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
        }
    }

    if (!intersectionResult.intersects)
        return;

    outIntersectionResultList.push_back(
                                QSSGRenderPickResult(model,
                                                     intersectionResult.rayLengthSquared,
                                                     intersectionResult.relXY,
                                                     intersectionResult.scenePosition));
}

void QSSGRenderer::intersectRayWithSubsetRenderable(const QSSGRenderRay &inRay,
                                                        QSSGRenderableObject &inRenderableObject,
                                                        TPickResultArray &outIntersectionResultList)
{
    QSSGRenderRay::IntersectionResult intersectionResult = QSSGRenderRay::intersectWithAABB(inRenderableObject.globalTransform, inRenderableObject.bounds, inRay);
    if (!intersectionResult.intersects)
        return;

    // Leave the coordinates relative for right now.
    const QSSGRenderGraphObject *thePickObject = nullptr;
    if (inRenderableObject.renderableFlags.isDefaultMaterialMeshSubset())
        thePickObject = &static_cast<QSSGSubsetRenderable *>(&inRenderableObject)->modelContext.model;
    else if (inRenderableObject.renderableFlags.isCustomMaterialMeshSubset())
        thePickObject = &static_cast<QSSGCustomMaterialRenderable *>(&inRenderableObject)->modelContext.model;

    if (thePickObject != nullptr) {
        outIntersectionResultList.push_back(
                                    QSSGRenderPickResult(*thePickObject,
                                                         intersectionResult.rayLengthSquared,
                                                         intersectionResult.relXY,
                                                         intersectionResult.scenePosition));
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

    const bool isYUpInFramebuffer = m_contextInterface->rhiContext()->isValid()
            ? m_contextInterface->rhiContext()->rhi()->isYUpInFramebuffer()
            : true;
    const bool isClipDepthZeroToOne = m_contextInterface->rhiContext()->isValid()
            ? m_contextInterface->rhiContext()->rhi()->isClipDepthZeroToOne()
            : true;

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
                                              isClipDepthZeroToOne};
}

const QSSGRef<QSSGProgramGenerator> &QSSGRenderer::getProgramGenerator()
{
    return m_contextInterface->shaderProgramGenerator();
}

QSSGOption<QVector2D> QSSGRenderer::getLayerMouseCoords(QSSGRenderLayer &inLayer,
                                                                const QVector2D &inMouseCoords,
                                                                const QVector2D &inViewportDimensions,
                                                                bool forceImageIntersect) const
{
    QSSGLayerRenderData *theData = const_cast<QSSGRenderer &>(*this).getOrCreateLayerRenderData(inLayer);
    Q_ASSERT(theData);
    return getLayerMouseCoords(*theData, inMouseCoords, inViewportDimensions, forceImageIntersect);
}

QSSGRenderPickResult::QSSGRenderPickResult(const QSSGRenderGraphObject &inHitObject,
                                           float inCameraDistance,
                                           const QVector2D &inLocalUVCoords,
                                           const QVector3D &scenePosition)
    : m_hitObject(&inHitObject)
    , m_cameraDistanceSq(inCameraDistance)
    , m_localUVCoords(inLocalUVCoords)
    , m_scenePosition(scenePosition)
{
}

QT_END_NAMESPACE
