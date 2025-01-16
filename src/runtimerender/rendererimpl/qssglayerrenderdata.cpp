// Copyright (C) 2008-2012 NVIDIA Corporation.
// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qssglayerrenderdata_p.h"

#include <QtQuick3DRuntimeRender/private/qssgrenderer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderlight_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrhicustommaterialsystem_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrhiquadrenderer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrhiparticles_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderlayer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendereffect_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercamera_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderskeleton_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderjoint_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendermorphtarget_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderparticles_p.h>
#include "../qssgrendercontextcore.h"
#include <QtQuick3DRuntimeRender/private/qssgrenderbuffermanager_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercache_p.h>
#include <QtQuick3DRuntimeRender/private/qssgperframeallocator_p.h>
#include <QtQuick3DRuntimeRender/private/qssgruntimerenderlogging_p.h>
#include <QtQuick3DRuntimeRender/private/qssglightmapper_p.h>
#include <QtQuick3DRuntimeRender/private/qssgdebugdrawsystem_p.h>

#include <QtQuick3DUtils/private/qssgutils_p.h>
#include <QtQuick3DUtils/private/qssgassert_p.h>

#include <QtQuick/private/qsgtexture_p.h>
#include <QtQuick/private/qsgrenderer_p.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QBitArray>
#include <array>

#include "qssgrenderpass_p.h"
#include "rendererimpl/qssgrenderhelpers_p.h"

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(lcQuick3DRender, "qt.quick3d.render");

#define POS4BONETRANS(x)    (sizeof(float) * 16 * (x) * 2)
#define POS4BONENORM(x)     (sizeof(float) * 16 * ((x) * 2 + 1))
#define BONEDATASIZE4ID(x)  POS4BONETRANS(x + 1)

static bool checkParticleSupport(QRhi *rhi)
{
    QSSG_ASSERT(rhi, return false);

    bool ret = true;
    const bool supportRgba32f = rhi->isTextureFormatSupported(QRhiTexture::RGBA32F);
    const bool supportRgba16f = rhi->isTextureFormatSupported(QRhiTexture::RGBA16F);
    if (!supportRgba32f && !supportRgba16f) {
        static bool warningShown = false;
        if (!warningShown) {
            qWarning () << "Particles not supported due to missing RGBA32F and RGBA16F texture format support";
            warningShown = true;
        }
        ret = false;
    }

    return ret;
}

// These are meant to be pixel offsets, so you need to divide them by the width/height
// of the layer respectively.
static const QVector2D s_ProgressiveAAVertexOffsets[QSSGLayerRenderData::MAX_AA_LEVELS] = {
    QVector2D(-0.170840f, -0.553840f), // 1x
    QVector2D(0.162960f, -0.319340f), // 2x
    QVector2D(0.360260f, -0.245840f), // 3x
    QVector2D(-0.561340f, -0.149540f), // 4x
    QVector2D(0.249460f, 0.453460f), // 5x
    QVector2D(-0.336340f, 0.378260f), // 6x
    QVector2D(0.340000f, 0.166260f), // 7x
    QVector2D(0.235760f, 0.527760f), // 8x
};

qsizetype QSSGLayerRenderData::frustumCulling(const QSSGClippingFrustum &clipFrustum, const QSSGRenderableObjectList &renderables, QSSGRenderableObjectList &visibleRenderables)
{
    QSSG_ASSERT(visibleRenderables.isEmpty(), visibleRenderables.clear());
    visibleRenderables.reserve(renderables.size());
    for (quint32 end = renderables.size(), idx = quint32(0); idx != end; ++idx) {
        auto handle = renderables.at(idx);
        const auto &b = handle.obj->globalBounds;
        if (clipFrustum.intersectsWith(b))
            visibleRenderables.push_back(handle);
    }

    return visibleRenderables.size();
}

qsizetype QSSGLayerRenderData::frustumCullingInline(const QSSGClippingFrustum &clipFrustum, QSSGRenderableObjectList &renderables)
{
    const qint32 end = renderables.size();
    qint32 front = 0;
    qint32 back = end - 1;

    while (front <= back) {
        const auto &b = renderables.at(front).obj->globalBounds;
        if (clipFrustum.intersectsWith(b))
            ++front;
        else
            renderables.swapItemsAt(front, back--);
    }

    return back + 1;
}

[[nodiscard]] constexpr static inline bool nearestToFurthestCompare(const QSSGRenderableObjectHandle &lhs, const QSSGRenderableObjectHandle &rhs) noexcept
{
    return lhs.cameraDistanceSq < rhs.cameraDistanceSq;
}

[[nodiscard]] constexpr static inline bool furthestToNearestCompare(const QSSGRenderableObjectHandle &lhs, const QSSGRenderableObjectHandle &rhs) noexcept
{
    return lhs.cameraDistanceSq > rhs.cameraDistanceSq;
}

static void collectBoneTransforms(QSSGRenderNode *node, QSSGRenderSkeleton *skeletonNode, const QVector<QMatrix4x4> &poses)
{
    if (node->type == QSSGRenderGraphObject::Type::Joint) {
        QSSGRenderJoint *jointNode = static_cast<QSSGRenderJoint *>(node);
        jointNode->calculateGlobalVariables();
        QMatrix4x4 globalTrans = jointNode->globalTransform;
        // if user doesn't give the inverseBindPose, identity matrices are used.
        if (poses.size() > jointNode->index)
            globalTrans *= poses[jointNode->index];
        memcpy(skeletonNode->boneData.data() + POS4BONETRANS(jointNode->index),
               reinterpret_cast<const void *>(globalTrans.constData()),
               sizeof(float) * 16);
        // only upper 3x3 is meaningful
        memcpy(skeletonNode->boneData.data() + POS4BONENORM(jointNode->index),
               reinterpret_cast<const void *>(QMatrix4x4(globalTrans.normalMatrix()).constData()),
               sizeof(float) * 11);
    } else {
        skeletonNode->containsNonJointNodes = true;
    }
    for (auto &child : node->children)
        collectBoneTransforms(&child, skeletonNode, poses);
}

static bool hasDirtyNonJointNodes(QSSGRenderNode *node, bool &hasChildJoints)
{
    if (!node)
        return false;
    // we might be non-joint dirty node, but if we do not have child joints we need to return false
    // Note! The frontend clears TransformDirty. Use dirty instead.
    bool dirtyNonJoint = ((node->type != QSSGRenderGraphObject::Type::Joint)
                                              && node->isDirty());

    // Tell our parent we are joint
    if (node->type == QSSGRenderGraphObject::Type::Joint)
        hasChildJoints = true;
    bool nodeHasChildJoints = false;
    for (auto &child : node->children) {
        bool ret = hasDirtyNonJointNodes(&child, nodeHasChildJoints);
        // return if we have child joints and non-joint dirty nodes, else check other children
        hasChildJoints |= nodeHasChildJoints;
        if (ret && nodeHasChildJoints)
            return true;
    }
    // return true if we have child joints and we are dirty non-joint
    hasChildJoints |= nodeHasChildJoints;
    return dirtyNonJoint && nodeHasChildJoints;
}

template<typename T, typename V>
inline void collectNode(V node, QVector<T> &dst, int &dstPos)
{
    if (dstPos < dst.size())
        dst[dstPos] = node;
    else
        dst.push_back(node);

    ++dstPos;
}
template <typename T, typename V>
static inline void collectNodeFront(V node, QVector<T> &dst, int &dstPos)
{
    if (dstPos < dst.size())
        dst[dst.size() - dstPos - 1] = node;
    else
        dst.push_front(node);

    ++dstPos;
}

#define MAX_MORPH_TARGET 8
#define MAX_MORPH_TARGET_INDEX_SUPPORTS_NORMALS 3
#define MAX_MORPH_TARGET_INDEX_SUPPORTS_TANGENTS 1

static bool maybeQueueNodeForRender(QSSGRenderNode &inNode,
                                    QVector<QSSGRenderableNodeEntry> &outRenderableModels,
                                    int &ioRenderableModelsCount,
                                    QVector<QSSGRenderableNodeEntry> &outRenderableParticles,
                                    int &ioRenderableParticlesCount,
                                    QVector<QSSGRenderItem2D *> &outRenderableItem2Ds,
                                    int &ioRenderableItem2DsCount,
                                    QVector<QSSGRenderCamera *> &outCameras,
                                    int &ioCameraCount,
                                    QVector<QSSGRenderLight *> &outLights,
                                    int &ioLightCount,
                                    QVector<QSSGRenderReflectionProbe *> &outReflectionProbes,
                                    int &ioReflectionProbeCount,
                                    quint32 &ioDFSIndex)
{
    bool wasDirty = inNode.isDirty(QSSGRenderNode::DirtyFlag::GlobalValuesDirty) && inNode.calculateGlobalVariables();
    if (inNode.getGlobalState(QSSGRenderNode::GlobalState::Active)) {
        ++ioDFSIndex;
        inNode.dfsIndex = ioDFSIndex;
        if (QSSGRenderGraphObject::isRenderable(inNode.type)) {
            if (inNode.type == QSSGRenderNode::Type::Model)
                collectNode(QSSGRenderableNodeEntry(inNode), outRenderableModels, ioRenderableModelsCount);
            else if (inNode.type == QSSGRenderNode::Type::Particles)
                collectNode(QSSGRenderableNodeEntry(inNode), outRenderableParticles, ioRenderableParticlesCount);
            else if (inNode.type == QSSGRenderNode::Type::Item2D) // Pushing front to keep item order inside QML file
                collectNodeFront(static_cast<QSSGRenderItem2D *>(&inNode), outRenderableItem2Ds, ioRenderableItem2DsCount);
        } else if (QSSGRenderGraphObject::isCamera(inNode.type)) {
            collectNode(static_cast<QSSGRenderCamera *>(&inNode), outCameras, ioCameraCount);
        } else if (QSSGRenderGraphObject::isLight(inNode.type)) {
            if (auto &light = static_cast<QSSGRenderLight &>(inNode); light.isEnabled())
                collectNode(&light, outLights, ioLightCount);
        } else if (inNode.type == QSSGRenderGraphObject::Type::ReflectionProbe) {
            collectNode(static_cast<QSSGRenderReflectionProbe *>(&inNode), outReflectionProbes, ioReflectionProbeCount);
        }

        for (auto &theChild : inNode.children)
            wasDirty |= maybeQueueNodeForRender(theChild,
                                                outRenderableModels,
                                                ioRenderableModelsCount,
                                                outRenderableParticles,
                                                ioRenderableParticlesCount,
                                                outRenderableItem2Ds,
                                                ioRenderableItem2DsCount,
                                                outCameras,
                                                ioCameraCount,
                                                outLights,
                                                ioLightCount,
                                                outReflectionProbes,
                                                ioReflectionProbeCount,
                                                ioDFSIndex);
    }
    return wasDirty;
}

QSSGDefaultMaterialPreparationResult::QSSGDefaultMaterialPreparationResult(QSSGShaderDefaultMaterialKey inKey)
    : firstImage(nullptr), opacity(1.0f), materialKey(inKey), dirty(false)
{
}

static QSSGRenderCameraData getCameraDataImpl(const QSSGRenderCamera *camera)
{
    QSSGRenderCameraData ret;
    if (camera) {
        // Calculate viewProjection and clippingFrustum for Render Camera
        QMatrix4x4 viewProjection(Qt::Uninitialized);
        camera->calculateViewProjectionMatrix(viewProjection);
        std::optional<QSSGClippingFrustum> clippingFrustum;
        if (camera->enableFrustumClipping) {
            QSSGClipPlane nearPlane;
            QMatrix3x3 theUpper33(camera->globalTransform.normalMatrix());
            QVector3D dir(QSSGUtils::mat33::transform(theUpper33, QVector3D(0, 0, -1)));
            dir.normalize();
            nearPlane.normal = dir;
            QVector3D theGlobalPos = camera->getGlobalPos() + camera->clipNear * dir;
            nearPlane.d = -(QVector3D::dotProduct(dir, theGlobalPos));
            // the near plane's bbox edges are calculated in the clipping frustum's
            // constructor.
            clippingFrustum = QSSGClippingFrustum{viewProjection, nearPlane};
        }
        ret = { viewProjection, clippingFrustum, camera->getScalingCorrectDirection(), camera->getGlobalPos() };
    }

    return ret;
}

// Returns the cached data for the active render camera(s) (if any)
const QSSGRenderCameraDataList &QSSGLayerRenderData::getCachedCameraDatas()
{
    ensureCachedCameraDatas();
    return *renderedCameraData;
}

void QSSGLayerRenderData::ensureCachedCameraDatas()
{
    if (renderedCameraData.has_value())
        return;

    QSSGRenderCameraDataList cameraData;
    for (QSSGRenderCamera *cam : std::as_const(renderedCameras))
        cameraData.append(getCameraDataImpl(cam));
    renderedCameraData = std::move(cameraData);
}

[[nodiscard]] static inline float getCameraDistanceSq(const QSSGRenderableObject &obj,
                                                      const QSSGRenderCameraData &camera) noexcept
{
    const QVector3D difference = obj.worldCenterPoint - camera.position;
    return QVector3D::dotProduct(difference, camera.direction) + obj.depthBiasSq;
}

// Per-frame cache of renderable objects post-sort.
const QVector<QSSGRenderableObjectHandle> &QSSGLayerRenderData::getSortedOpaqueRenderableObjects(const QSSGRenderCamera &camera, size_t index)
{
    index = index * size_t(index < opaqueObjectStore.size());
    auto &sortedOpaqueObjects = sortedOpaqueObjectCache[index][&camera];
    if (!sortedOpaqueObjects.empty())
        return sortedOpaqueObjects;

    if (layer.layerFlags.testFlag(QSSGRenderLayer::LayerFlag::EnableDepthTest))
        sortedOpaqueObjects = std::as_const(opaqueObjectStore)[index];

    const auto &clippingFrustum = getCameraRenderData(&camera).clippingFrustum;
    if (clippingFrustum.has_value()) { // Frustum culling
        const auto visibleObjects = QSSGLayerRenderData::frustumCullingInline(clippingFrustum.value(), sortedOpaqueObjects);
        sortedOpaqueObjects.resize(visibleObjects);
    }

    // Render nearest to furthest objects
    std::sort(sortedOpaqueObjects.begin(), sortedOpaqueObjects.end(), nearestToFurthestCompare);

    return sortedOpaqueObjects;
}

// If layer depth test is false, this may also contain opaque objects.
const QVector<QSSGRenderableObjectHandle> &QSSGLayerRenderData::getSortedTransparentRenderableObjects(const QSSGRenderCamera &camera, size_t index)
{
    index = index * size_t(index < transparentObjectStore.size());
    auto &sortedTransparentObjects = sortedTransparentObjectCache[index][&camera];

    if (!sortedTransparentObjects.empty())
        return sortedTransparentObjects;

    sortedTransparentObjects = std::as_const(transparentObjectStore)[index];

    if (!layer.layerFlags.testFlag(QSSGRenderLayer::LayerFlag::EnableDepthTest)) {
        const auto &opaqueObjects = std::as_const(opaqueObjectStore)[index];
        sortedTransparentObjects.append(opaqueObjects);
    }

    const auto &clippingFrustum = getCameraRenderData(&camera).clippingFrustum;
    if (clippingFrustum.has_value()) { // Frustum culling
        const auto visibleObjects = QSSGLayerRenderData::frustumCullingInline(clippingFrustum.value(), sortedTransparentObjects);
        sortedTransparentObjects.resize(visibleObjects);
    }

    // render furthest to nearest.
    std::sort(sortedTransparentObjects.begin(), sortedTransparentObjects.end(), furthestToNearestCompare);

    return sortedTransparentObjects;
}

const QVector<QSSGRenderableObjectHandle> &QSSGLayerRenderData::getSortedScreenTextureRenderableObjects(const QSSGRenderCamera &camera, size_t index)
{
    index = index * size_t(index < screenTextureObjectStore.size());
    const auto &screenTextureObjects = std::as_const(screenTextureObjectStore)[index];
    auto &renderedScreenTextureObjects = sortedScreenTextureObjectCache[index][&camera];

    if (!renderedScreenTextureObjects.empty())
        return renderedScreenTextureObjects;
    renderedScreenTextureObjects = screenTextureObjects;
    if (!renderedScreenTextureObjects.empty()) {
        // render furthest to nearest.
        std::sort(renderedScreenTextureObjects.begin(), renderedScreenTextureObjects.end(), furthestToNearestCompare);
    }
    return renderedScreenTextureObjects;
}

const QVector<QSSGBakedLightingModel> &QSSGLayerRenderData::getSortedBakedLightingModels()
{
    if (!renderedBakedLightingModels.empty() || renderedCameras.isEmpty())
        return renderedBakedLightingModels;
    if (layer.layerFlags.testFlag(QSSGRenderLayer::LayerFlag::EnableDepthTest) && !bakedLightingModels.empty()) {
        renderedBakedLightingModels = bakedLightingModels;
        for (QSSGBakedLightingModel &lm : renderedBakedLightingModels) {
            // sort nearest to furthest (front to back)
            std::sort(lm.renderables.begin(), lm.renderables.end(), nearestToFurthestCompare);
        }
    }
    return renderedBakedLightingModels;
}

const QSSGLayerRenderData::RenderableItem2DEntries &QSSGLayerRenderData::getRenderableItem2Ds()
{
    if (!renderedItem2Ds.isEmpty() || renderedCameras.isEmpty())
        return renderedItem2Ds;

    renderedItem2Ds = renderableItem2Ds;

    if (!renderedItem2Ds.isEmpty()) {
        const QSSGRenderCameraDataList &cameraDatas(getCachedCameraDatas());
        // with multiview this means using the left eye camera
        const QSSGRenderCameraData &cameraDirectionAndPosition(cameraDatas[0]);
        const QVector3D &cameraDirection = cameraDirectionAndPosition.direction;
        const QVector3D &cameraPosition = cameraDirectionAndPosition.position;

        const auto isItemNodeDistanceGreatThan = [cameraDirection, cameraPosition]
                (const QSSGRenderItem2D *lhs, const QSSGRenderItem2D *rhs) {
            if (!lhs->parent || !rhs->parent)
                return false;
            const QVector3D lhsDifference = lhs->parent->getGlobalPos() - cameraPosition;
            const float lhsCameraDistanceSq = QVector3D::dotProduct(lhsDifference, cameraDirection);
            const QVector3D rhsDifference = rhs->parent->getGlobalPos() - cameraPosition;
            const float rhsCameraDistanceSq = QVector3D::dotProduct(rhsDifference, cameraDirection);
            return lhsCameraDistanceSq > rhsCameraDistanceSq;
        };

        const auto isItemZOrderLessThan = []
                (const QSSGRenderItem2D *lhs, const QSSGRenderItem2D *rhs) {
            if (lhs->parent && rhs->parent && lhs->parent == rhs->parent) {
                // Same parent nodes, so sort with item z-ordering
                return lhs->zOrder < rhs->zOrder;
            }
            return false;
        };

        // Render furthest to nearest items (parent nodes).
        std::stable_sort(renderedItem2Ds.begin(), renderedItem2Ds.end(), isItemNodeDistanceGreatThan);
        // Render items inside same node by item z-order.
        // Note: stable_sort so item order in QML file is respected.
        std::stable_sort(renderedItem2Ds.begin(), renderedItem2Ds.end(), isItemZOrderLessThan);
    }

    return renderedItem2Ds;
}

// Depth Write List
void QSSGLayerRenderData::updateSortedDepthObjectsListImp(const QSSGRenderCamera &camera, size_t index)
{
    auto &depthWriteObjects = sortedDepthWriteCache[index][&camera];
    auto &depthPrepassObjects = sortedOpaqueDepthPrepassCache[index][&camera];

    if (!depthWriteObjects.isEmpty() || !depthPrepassObjects.isEmpty())
        return;

    if (layer.layerFlags.testFlag(QSSGRenderLayer::LayerFlag::EnableDepthTest)) {
        if (hasDepthWriteObjects || (depthPrepassObjectsState & DepthPrepassObjectStateT(DepthPrepassObject::Opaque)) != 0) {
            const auto &sortedOpaqueObjects = getSortedOpaqueRenderableObjects(camera, index); // front to back
            for (const auto &opaqueObject : sortedOpaqueObjects) {
                const auto depthMode = opaqueObject.obj->depthWriteMode;
                if (depthMode == QSSGDepthDrawMode::Always || depthMode == QSSGDepthDrawMode::OpaqueOnly)
                    depthWriteObjects.append(opaqueObject);
                else if (depthMode == QSSGDepthDrawMode::OpaquePrePass)
                    depthPrepassObjects.append(opaqueObject);
            }
        }
        if (hasDepthWriteObjects || (depthPrepassObjectsState & DepthPrepassObjectStateT(DepthPrepassObject::Transparent)) != 0) {
            const auto &sortedTransparentObjects = getSortedTransparentRenderableObjects(camera, index); // back to front
            for (const auto &transparentObject : sortedTransparentObjects) {
                const auto depthMode = transparentObject.obj->depthWriteMode;
                if (depthMode == QSSGDepthDrawMode::Always)
                    depthWriteObjects.append(transparentObject);
                else if (depthMode == QSSGDepthDrawMode::OpaquePrePass)
                    depthPrepassObjects.append(transparentObject);
            }
        }
        if (hasDepthWriteObjects || (depthPrepassObjectsState & DepthPrepassObjectStateT(DepthPrepassObject::ScreenTexture)) != 0) {
            const auto &sortedScreenTextureObjects = getSortedScreenTextureRenderableObjects(camera, index); // back to front
            for (const auto &screenTextureObject : sortedScreenTextureObjects) {
                const auto depthMode = screenTextureObject.obj->depthWriteMode;
                if (depthMode == QSSGDepthDrawMode::Always || depthMode == QSSGDepthDrawMode::OpaqueOnly)
                    depthWriteObjects.append(screenTextureObject);
                else if (depthMode == QSSGDepthDrawMode::OpaquePrePass)
                    depthPrepassObjects.append(screenTextureObject);
            }
        }
    }
}

const std::unique_ptr<QSSGPerFrameAllocator> &QSSGLayerRenderData::perFrameAllocator(QSSGRenderContextInterface &ctx)
{
    return ctx.perFrameAllocator();
}

static constexpr quint16 PREP_CTX_INDEX_MASK = 0xffff;
static constexpr QSSGPrepContextId createPrepId(size_t index, quint32 frame) { return QSSGPrepContextId { ((quint64(frame) << 32) | index ) * quint64(index <= std::numeric_limits<quint16>::max()) }; }
static constexpr size_t getPrepContextIndex(QSSGPrepContextId id) { return (static_cast<quint64>(id) & PREP_CTX_INDEX_MASK); }
static constexpr bool verifyPrepContext(QSSGPrepContextId id, const QSSGRenderer &renderer) { return (getPrepContextIndex(id) > 0) && ((static_cast<quint64>(id) >> 32) == renderer.frameCount()); }

QSSGPrepContextId QSSGLayerRenderData::getOrCreateExtensionContext(const QSSGRenderExtension &ext, QSSGRenderCamera *camera, quint32 slot)
{
    const auto frame = renderer->frameCount();
    const auto index = extContexts.size();
    // Sanity check... Shouldn't get anywhere close to the max in real world usage (unless somethings broken).
    QSSG_ASSERT_X(index < PREP_CTX_INDEX_MASK - 1, "Reached maximum entries!", return QSSGPrepContextId::Invalid);
    auto it = std::find_if(extContexts.cbegin(), extContexts.cend(), [&ext, slot](const ExtensionContext &e){ return (e.owner == &ext) && (e.slot == slot); });
    if (it == extContexts.cend()) {
        extContexts.push_back(ExtensionContext{ ext, camera, index, slot });
        it = extContexts.cbegin() + index;
        renderableModelStore.emplace_back();
        modelContextStore.emplace_back();
        renderableObjectStore.emplace_back();
        screenTextureObjectStore.emplace_back();
        opaqueObjectStore.emplace_back();
        transparentObjectStore.emplace_back();
        sortedOpaqueObjectCache.emplace_back();
        sortedTransparentObjectCache.emplace_back();
        sortedScreenTextureObjectCache.emplace_back();
        sortedOpaqueDepthPrepassCache.emplace_back();
        sortedDepthWriteCache.emplace_back();
        QSSG_ASSERT(renderableModelStore.size() == extContexts.size(), renderableModelStore.resize(extContexts.size()));
        QSSG_ASSERT(modelContextStore.size() == extContexts.size(), modelContextStore.resize(extContexts.size()));
        QSSG_ASSERT(renderableObjectStore.size() == extContexts.size(), renderableObjectStore.resize(extContexts.size()));
        QSSG_ASSERT(screenTextureObjectStore.size() == extContexts.size(), screenTextureObjectStore.resize(extContexts.size()));
        QSSG_ASSERT(opaqueObjectStore.size() == extContexts.size(), opaqueObjectStore.resize(extContexts.size()));
        QSSG_ASSERT(transparentObjectStore.size() == extContexts.size(), transparentObjectStore.resize(extContexts.size()));
        QSSG_ASSERT(sortedOpaqueObjectCache.size() == extContexts.size(), sortedOpaqueObjectCache.resize(extContexts.size()));
        QSSG_ASSERT(sortedTransparentObjectCache.size() == extContexts.size(), sortedTransparentObjectCache.resize(extContexts.size()));
        QSSG_ASSERT(sortedScreenTextureObjectCache.size() == extContexts.size(), sortedScreenTextureObjectCache.resize(extContexts.size()));
        QSSG_ASSERT(sortedOpaqueDepthPrepassCache.size() == extContexts.size(), sortedOpaqueDepthPrepassCache.resize(extContexts.size()));
        QSSG_ASSERT(sortedDepthWriteCache.size() == extContexts.size(), sortedDepthWriteCache.resize(extContexts.size()));
    }

    return createPrepId(it->index, frame);
}

static void createRenderablesHelper(QSSGLayerRenderData &layer, const QSSGRenderNode::ChildList &children, QSSGLayerRenderData::RenderableNodeEntries &renderables, QSSGRenderHelpers::CreateFlags createFlags)
{
    const bool steal = ((createFlags & QSSGRenderHelpers::CreateFlag::Steal) != 0);
    for (auto &chld : children) {
        if (chld.type == QSSGRenderGraphObject::Type::Model) {
            const auto &renderModel = static_cast<const QSSGRenderModel &>(chld);
            auto &renderableModels = layer.renderableModels;
            if (auto it = std::find_if(renderableModels.cbegin(), renderableModels.cend(), [&renderModel](const QSSGRenderableNodeEntry &e) { return (e.node == &renderModel); }); it != renderableModels.cend()) {
                renderables.emplace_back(*it);
                if (steal)
                    renderableModels.erase(it);
            }
        }

        createRenderablesHelper(layer, chld.children, renderables, createFlags);
    }
}

QSSGRenderablesId QSSGLayerRenderData::createRenderables(QSSGPrepContextId prepId, const QList<QSSGNodeId> &nodes, QSSGRenderHelpers::CreateFlags createFlags)
{
    QSSG_ASSERT_X(verifyPrepContext(prepId, *renderer), "Expired or invalid prep id", return {});

    const size_t index = getPrepContextIndex(prepId);
    QSSG_ASSERT(index < renderableModelStore.size(), return {});

    auto &renderables = renderableModelStore[index];
    if (renderables.size() != 0) {
        qWarning() << "Renderables already created for this context - Previous renderables will be overwritten";
        renderables.clear();
    }

    renderables.reserve(nodes.size());

    // We now create the renderable node entries for all the models.
    // NOTE: The nodes are not complete at this point...
    const bool steal = ((createFlags & QSSGRenderHelpers::CreateFlag::Steal) != 0);
    for (const auto &nodeId : nodes) {
        auto *node = QSSGRenderGraphObjectUtils::getNode<QSSGRenderNode>(nodeId);
        if (node && node->type == QSSGRenderGraphObject::Type::Model) {
            auto *renderModel = static_cast<QSSGRenderModel *>(node);
            // NOTE: Not ideal.
            if (auto it = std::find_if(renderableModels.cbegin(), renderableModels.cend(), [renderModel](const QSSGRenderableNodeEntry &e) { return (e.node == renderModel); }); it != renderableModels.cend()) {
                auto &inserted = renderables.emplace_back(*it);
                inserted.overridden = {};
                if (steal)
                    it->overridden |= QSSGRenderableNodeEntry::Overridden::Disabled;
            } else {
                renderables.emplace_back(*renderModel);
            }
        }

        if (node && ((createFlags & QSSGRenderHelpers::CreateFlag::Recurse) != 0)) {
            const auto &children = node->children;
            createRenderablesHelper(*this, children, renderables, createFlags);
        }
    }

    return (renderables.size() != 0) ? static_cast<QSSGRenderablesId>(prepId) : QSSGRenderablesId{ QSSGRenderablesId::Invalid };
}

void QSSGLayerRenderData::setGlobalTransform(QSSGRenderablesId renderablesId, const QSSGRenderModel &model, const QMatrix4x4 &globalTransform)
{
    const auto prepId = static_cast<QSSGPrepContextId>(renderablesId);
    QSSG_ASSERT_X(verifyPrepContext(prepId, *renderer), "Expired or invalid renderables id", return);
    const size_t index = getPrepContextIndex(prepId);
    QSSG_ASSERT_X(index < renderableModelStore.size(), "Missing call to createRenderables()?", return);

    auto &renderables = renderableModelStore[index];
    auto it = std::find_if(renderables.cbegin(), renderables.cend(), [&model](const QSSGRenderableNodeEntry &e) { return e.node == &model; });
    if (it != renderables.cend()) {
        it->globalTransform = globalTransform;
        it->overridden |= QSSGRenderableNodeEntry::Overridden::GlobalTransform;
    }
}

QMatrix4x4 QSSGLayerRenderData::getGlobalTransform(QSSGPrepContextId prepId, const QSSGRenderModel &model)
{
    QSSG_ASSERT_X(verifyPrepContext(prepId, *renderer), "Expired or invalid prep id", return {});
    const size_t index = getPrepContextIndex(prepId);
    QSSG_ASSERT_X(index < renderableModelStore.size(), "Missing call to createRenderables()?", return {});

    QMatrix4x4 ret = model.globalTransform;
    auto &renderables = renderableModelStore[index];
    auto it = std::find_if(renderables.cbegin(), renderables.cend(), [&model](const QSSGRenderableNodeEntry &e) { return e.node == &model; });
    if (it != renderables.cend() && (it->overridden & QSSGRenderableNodeEntry::Overridden::GlobalTransform))
        ret = it->globalTransform;

    return ret;
}

void QSSGLayerRenderData::setGlobalOpacity(QSSGRenderablesId renderablesId, const QSSGRenderModel &model, float opacity)
{
    const auto prepId = static_cast<QSSGPrepContextId>(renderablesId);
    QSSG_ASSERT_X(verifyPrepContext(prepId, *renderer), "Expired or invalid renderables id", return);
    const size_t index = getPrepContextIndex(prepId);
    QSSG_ASSERT_X(index < renderableModelStore.size(), "Missing call to createRenderables()?", return);

    auto &renderables = renderableModelStore[index];
    auto it = std::find_if(renderables.cbegin(), renderables.cend(), [&model](const QSSGRenderableNodeEntry &e) { return e.node == &model; });
    if (it != renderables.cend()) {
        it->globalOpacity = opacity;
        it->overridden |= QSSGRenderableNodeEntry::Overridden::GlobalOpacity;
    }
}

float QSSGLayerRenderData::getGlobalOpacity(QSSGPrepContextId prepId, const QSSGRenderModel &model)
{
    QSSG_ASSERT_X(verifyPrepContext(prepId, *renderer), "Expired or invalid prep id", return {});
    const size_t index = getPrepContextIndex(prepId);
    QSSG_ASSERT_X(index < renderableModelStore.size(), "Missing call to createRenderables()?", return {});

    float ret = model.globalOpacity;
    auto &renderables = renderableModelStore[index];
    auto it = std::find_if(renderables.cbegin(), renderables.cend(), [&model](const QSSGRenderableNodeEntry &e) { return e.node == &model; });
    if (it != renderables.cend() && (it->overridden & QSSGRenderableNodeEntry::Overridden::GlobalOpacity))
        ret = it->globalOpacity;

    return ret;
}

void QSSGLayerRenderData::setModelMaterials(QSSGRenderablesId renderablesId, const QSSGRenderModel &model, const QList<QSSGResourceId> &materials)
{
    const auto prepId = static_cast<QSSGPrepContextId>(renderablesId);
    QSSG_ASSERT_X(verifyPrepContext(prepId, *renderer), "Expired or invalid renderable id", return);
    const size_t index = getPrepContextIndex(prepId);
    QSSG_ASSERT(index < renderableModelStore.size(), return);

    // Sanity check
    if (materials.size() > 0 && !QSSG_GUARD(QSSGRenderGraphObject::isMaterial(QSSGRenderGraphObjectUtils::getResource(materials.at(0))->type)))
        return;

    auto &renderables = renderableModelStore[index];
    auto it = std::find_if(renderables.cbegin(), renderables.cend(), [&model](const QSSGRenderableNodeEntry &e) { return e.node == &model; });
    if (it != renderables.cend()) {
        it->materials.resize(materials.size());
        std::memcpy(it->materials.data(), materials.data(), it->materials.size() * sizeof(QSSGRenderGraphObject *));
        it->overridden |= QSSGRenderableNodeEntry::Overridden::Materials;
    }
}

void QSSGLayerRenderData::setModelMaterials(const QSSGRenderablesId renderablesId, const QList<QSSGResourceId> &materials)
{
    const auto prepId = static_cast<QSSGPrepContextId>(renderablesId);
    QSSG_ASSERT_X(verifyPrepContext(prepId, *renderer), "Expired or invalid renderablesId or renderables id", return);

    const size_t index = getPrepContextIndex(prepId);
    QSSG_ASSERT(index < renderableModelStore.size(), return);

    auto &renderables = renderableModelStore[index];
    for (auto &renderable : renderables) {
        auto &renderableMaterials = renderable.materials;
        renderableMaterials.resize(materials.size());
        std::memcpy(renderableMaterials.data(), materials.data(), renderableMaterials.size() * sizeof(QSSGRenderGraphObject *));
        renderable.overridden |= QSSGRenderableNodeEntry::Overridden::Materials;
    }
}

QSSGPrepResultId QSSGLayerRenderData::prepareModelsForRender(QSSGRenderContextInterface &contextInterface,
                                                                  QSSGPrepContextId prepId,
                                                                  QSSGRenderablesId renderablesId,
                                                                  float lodThreshold)
{
    QSSG_ASSERT_X(renderablesId != QSSGRenderablesId::Invalid && verifyPrepContext(prepId, *renderer),
                  "Expired or invalid prep or renderables id", return QSSGPrepResultId::Invalid);
    const size_t index = getPrepContextIndex(prepId);
    QSSG_ASSERT(index < renderableModelStore.size(), return {});

    const auto &extContext = extContexts.at(index);

    QSSG_ASSERT_X(extContext.camera != nullptr, "No camera set!", return {});

    const auto vp = contextInterface.renderer()->viewport();
    extContext.camera->calculateGlobalVariables(vp);

    auto &renderables = renderableModelStore[index];

    prepareModelMaterials(renderables, true /* Cull renderables without materials */);

    prepareModelMeshes(contextInterface, renderables, false /* globalPickingEnabled */);

    // ### multiview
    QSSGRenderCameraList camera({ extContext.camera });
    QSSGRenderCameraDataList cameraData({ getCameraRenderData(extContext.camera) });

    auto &modelContexts = modelContextStore[index];
    QSSG_ASSERT(modelContexts.isEmpty(), modelContexts.clear());

    auto &renderableObjects = renderableObjectStore[index];
    QSSG_ASSERT(renderableObjects.isEmpty(), renderableObjects.clear());

    auto &opaqueObjects = opaqueObjectStore[index];
    QSSG_ASSERT(opaqueObjects.isEmpty(), opaqueObjects.clear());

    auto &transparentObjects = transparentObjectStore[index];
    QSSG_ASSERT(transparentObjects.isEmpty(), transparentObjects.clear());

    auto &screenTextureObjects = screenTextureObjectStore[index];
    QSSG_ASSERT(screenTextureObjects.isEmpty(), screenTextureObjects.clear());

    bool wasDirty = prepareModelsForRender(contextInterface,
                                           renderables,
                                           layerPrepResult.flags,
                                           camera,
                                           cameraData,
                                           modelContexts,
                                           opaqueObjects,
                                           transparentObjects,
                                           screenTextureObjects,
                                           lodThreshold);

    (void)wasDirty;

    return static_cast<QSSGPrepResultId>(prepId);
}

static constexpr size_t pipelineStateIndex(QSSGRenderablesFilter filter)
{
    switch (filter) {
    case QSSGRenderablesFilter::All:
        return 0;
    case QSSGRenderablesFilter::Opaque:
        return 1;
    case QSSGRenderablesFilter::Transparent:
        return 2;
    }

    // GCC 8.x does not treat __builtin_unreachable() as constexpr
#  if !defined(Q_CC_GNU_ONLY) || (Q_CC_GNU >= 900)
    // NOLINTNEXTLINE(qt-use-unreachable-return): Triggers on Clang, breaking GCC 8
    Q_UNREACHABLE();
#  endif
    return 0;
}

void QSSGLayerRenderData::prepareRenderables(QSSGRenderContextInterface &ctx,
                                             QSSGPrepResultId prepId,
                                             QRhiRenderPassDescriptor *renderPassDescriptor,
                                             const QSSGRhiGraphicsPipelineState &ps,
                                             QSSGRenderablesFilters filter)
{
    QSSG_ASSERT_X(verifyPrepContext(static_cast<QSSGPrepContextId>(prepId), *renderer), "Expired or invalid result id", return);
    const size_t index = getPrepContextIndex(static_cast<QSSGPrepContextId>(prepId));
    QSSG_ASSERT(index < renderableObjectStore.size() && index < extContexts.size(), return);

    auto &extCtx = extContexts[index];
    QSSG_ASSERT(extCtx.camera, return);
    extCtx.filter |= filter;

    QSSGShaderFeatures featureSet = getShaderFeatures();

    QSSGPassKey passKey { reinterpret_cast<void *>(quintptr(extCtx.owner) ^ extCtx.slot) }; // TODO: Pass this along

    if ((filter & QSSGRenderablesFilter::Opaque) != 0) {
        auto psCpy = ps;
        if (filter == QSSGRenderablesFilter::All) { // If 'All' we set our defaults
            psCpy.depthFunc = QRhiGraphicsPipeline::LessOrEqual;
            psCpy.flags.setFlag(QSSGRhiGraphicsPipelineState::Flag::BlendEnabled, false);
        }
        const auto &sortedRenderables = getSortedOpaqueRenderableObjects(*extCtx.camera, index);
        OpaquePass::prep(ctx, *this, passKey, psCpy, featureSet, renderPassDescriptor, sortedRenderables);
        const size_t psIndex = pipelineStateIndex(QSSGRenderablesFilter::Opaque);
        extCtx.ps[psIndex] = psCpy;
    }

    if ((filter & QSSGRenderablesFilter::Transparent) != 0) {
        auto psCpy = ps;
        if (filter == QSSGRenderablesFilter::All) { // If 'All' we set our defaults
            // transparent objects (or, without LayerEnableDepthTest, all objects)
            psCpy.flags.setFlag(QSSGRhiGraphicsPipelineState::Flag::BlendEnabled, true);
            psCpy.flags.setFlag(QSSGRhiGraphicsPipelineState::Flag::DepthWriteEnabled, false);
        }
        const auto &sortedRenderables = getSortedTransparentRenderableObjects(*extCtx.camera, index);
        TransparentPass::prep(ctx, *this, passKey, psCpy, featureSet, renderPassDescriptor, sortedRenderables);
        const size_t psIndex = pipelineStateIndex(QSSGRenderablesFilter::Transparent);
        extCtx.ps[psIndex] = psCpy;
    }
}

void QSSGLayerRenderData::renderRenderables(QSSGRenderContextInterface &ctx, QSSGPrepResultId prepId)
{
    QSSG_ASSERT_X(verifyPrepContext(static_cast<QSSGPrepContextId>(prepId), *renderer), "Expired or invalid result id", return);
    const size_t index = getPrepContextIndex(static_cast<QSSGPrepContextId>(prepId));
    QSSG_ASSERT(index < renderableObjectStore.size() && index < extContexts.size(), return);

    const auto &extCtx = extContexts.at(index);
    const auto filter = extCtx.filter;

    if ((filter & QSSGRenderablesFilter::Opaque) != 0) {
        const size_t psIndex = pipelineStateIndex(QSSGRenderablesFilter::Opaque);
        const auto &ps = extCtx.ps[psIndex];
        const auto &sortedRenderables = getSortedOpaqueRenderableObjects(*extCtx.camera, index);
        OpaquePass::render(ctx, ps, sortedRenderables);
    }

    if ((filter & QSSGRenderablesFilter::Transparent) != 0) {
        const size_t psIndex = pipelineStateIndex(QSSGRenderablesFilter::Transparent);
        const auto &ps = extCtx.ps[psIndex];
        const auto &sortedRenderables = getSortedTransparentRenderableObjects(*extCtx.camera, index);
        TransparentPass::render(ctx, ps, sortedRenderables);
    }
}

const QSSGRenderableObjectList &QSSGLayerRenderData::getSortedRenderedDepthWriteObjects(const QSSGRenderCamera &camera, size_t index)
{
    updateSortedDepthObjectsListImp(camera, index);
    return sortedDepthWriteCache[index][&camera];
}

const QSSGRenderableObjectList &QSSGLayerRenderData::getSortedrenderedOpaqueDepthPrepassObjects(const QSSGRenderCamera &camera, size_t index)
{
    updateSortedDepthObjectsListImp(camera, index);
    return sortedOpaqueDepthPrepassCache[index][&camera];;
}

/**
 * Usage: T *ptr = RENDER_FRAME_NEW<T>(context, arg0, arg1, ...); is equivalent to: T *ptr = new T(arg0, arg1, ...);
 * so RENDER_FRAME_NEW() takes the RCI + T's arguments
 */
template <typename T, typename... Args>
[[nodiscard]] inline T *RENDER_FRAME_NEW(QSSGRenderContextInterface &ctx, Args&&... args)
{
    static_assert(std::is_trivially_destructible_v<T>, "Objects allocated using the per-frame allocator needs to be trivially destructible!");
    return new (QSSGLayerRenderData::perFrameAllocator(ctx)->allocate(sizeof(T)))T(std::forward<Args>(args)...);
}

template <typename T>
[[nodiscard]] inline QSSGDataRef<T> RENDER_FRAME_NEW_BUFFER(QSSGRenderContextInterface &ctx, size_t count)
{
    static_assert(std::is_trivially_destructible_v<T>, "Objects allocated using the per-frame allocator needs to be trivially destructible!");
    const size_t asize = sizeof(T) * count;
    return { reinterpret_cast<T *>(QSSGLayerRenderData::perFrameAllocator(ctx)->allocate(asize)), qsizetype(count) };
}

QSSGShaderDefaultMaterialKey QSSGLayerRenderData::generateLightingKey(
        QSSGRenderDefaultMaterial::MaterialLighting inLightingType, const QSSGShaderLightListView &lights, bool receivesShadows)
{
    QSSGShaderDefaultMaterialKey theGeneratedKey(qHash(features));
    const bool lighting = inLightingType != QSSGRenderDefaultMaterial::MaterialLighting::NoLighting;
    defaultMaterialShaderKeyProperties.m_hasLighting.setValue(theGeneratedKey, lighting);
    if (lighting) {
        defaultMaterialShaderKeyProperties.m_hasIbl.setValue(theGeneratedKey, layer.lightProbe != nullptr);

        quint32 numLights = quint32(lights.size());
        Q_ASSERT(numLights <= QSSGShaderDefaultMaterialKeyProperties::LightCount);
        defaultMaterialShaderKeyProperties.m_lightCount.setValue(theGeneratedKey, numLights);

        int shadowMapCount = 0;
        for (int lightIdx = 0, lightEnd = lights.size(); lightIdx < lightEnd; ++lightIdx) {
            QSSGRenderLight *theLight(lights[lightIdx].light);
            const bool isDirectional = theLight->type == QSSGRenderLight::Type::DirectionalLight;
            const bool isSpot = theLight->type == QSSGRenderLight::Type::SpotLight;
            const bool castsShadows = theLight->m_castShadow
                    && !theLight->m_fullyBaked
                    && receivesShadows
                    && shadowMapCount < QSSG_MAX_NUM_SHADOW_MAPS;
            if (castsShadows)
                ++shadowMapCount;

            defaultMaterialShaderKeyProperties.m_lightFlags[lightIdx].setValue(theGeneratedKey, !isDirectional);
            defaultMaterialShaderKeyProperties.m_lightSpotFlags[lightIdx].setValue(theGeneratedKey, isSpot);
            defaultMaterialShaderKeyProperties.m_lightShadowFlags[lightIdx].setValue(theGeneratedKey, castsShadows);
            defaultMaterialShaderKeyProperties.m_lightShadowMapSize[lightIdx].setValue(theGeneratedKey, theLight->m_shadowMapRes);
            defaultMaterialShaderKeyProperties.m_lightSoftShadowQuality[lightIdx].setValue(theGeneratedKey,
                                                                                           quint32(theLight->m_softShadowQuality));
        }
    }
    return theGeneratedKey;
}

void QSSGLayerRenderData::prepareImageForRender(QSSGRenderImage &inImage,
                                                           QSSGRenderableImage::Type inMapType,
                                                           QSSGRenderableImage *&ioFirstImage,
                                                           QSSGRenderableImage *&ioNextImage,
                                                           QSSGRenderableObjectFlags &ioFlags,
                                                           QSSGShaderDefaultMaterialKey &inShaderKey,
                                                           quint32 inImageIndex,
                                                           QSSGRenderDefaultMaterial *inMaterial)
{
    QSSGRenderContextInterface &contextInterface = *renderer->contextInterface();
    const auto &bufferManager = contextInterface.bufferManager();

    if (inImage.clearDirty())
        ioFlags |= QSSGRenderableObjectFlag::Dirty;

    // This is where the QRhiTexture gets created, if not already done. Note
    // that the bufferManager is per-QQuickWindow, and so per-render-thread.
    // Hence using the same Texture (backed by inImage as the backend node) in
    // multiple windows will work by each scene in each window getting its own
    // QRhiTexture. And that's why the QSSGRenderImageTexture cannot be a
    // member of the QSSGRenderImage. Conceptually this matches what we do for
    // models (QSSGRenderModel -> QSSGRenderMesh retrieved from the
    // bufferManager in each prepareModelForRender, etc.).

    const QSSGRenderImageTexture texture = bufferManager->loadRenderImage(&inImage);

    if (texture.m_texture) {
        if (texture.m_flags.hasTransparency()
            && (inMapType == QSSGRenderableImage::Type::Diffuse // note: Type::BaseColor is skipped here intentionally
                || inMapType == QSSGRenderableImage::Type::Opacity
                || inMapType == QSSGRenderableImage::Type::Translucency))
        {
            ioFlags |= QSSGRenderableObjectFlag::HasTransparency;
        }

        QSSGRenderableImage *theImage = RENDER_FRAME_NEW<QSSGRenderableImage>(contextInterface, inMapType, inImage, texture);
        QSSGShaderKeyImageMap &theKeyProp = defaultMaterialShaderKeyProperties.m_imageMaps[inImageIndex];

        theKeyProp.setEnabled(inShaderKey, true);
        switch (inImage.m_mappingMode) {
        case QSSGRenderImage::MappingModes::Normal:
            break;
        case QSSGRenderImage::MappingModes::Environment:
            theKeyProp.setEnvMap(inShaderKey, true);
            break;
        case QSSGRenderImage::MappingModes::LightProbe:
            theKeyProp.setLightProbe(inShaderKey, true);
            break;
        }

        bool hasA = false;
        bool hasG = false;
        bool hasB = false;


        //### TODO: More formats
        switch (texture.m_texture->format()) {
        case QRhiTexture::Format::RED_OR_ALPHA8:
            hasA = !renderer->contextInterface()->rhiContext()->rhi()->isFeatureSupported(QRhi::RedOrAlpha8IsRed);
            break;
        case QRhiTexture::Format::R8:
            // Leave BGA as false
            break;
        default:
            hasA = true;
            hasG = true;
            hasB = true;
            break;
        }

        if (inImage.isImageTransformIdentity())
            theKeyProp.setIdentityTransform(inShaderKey, true);

        if (inImage.m_indexUV == 1)
            theKeyProp.setUsesUV1(inShaderKey, true);

        if (texture.m_flags.isLinear())
            theKeyProp.setLinear(inShaderKey, true);

        if (ioFirstImage == nullptr)
            ioFirstImage = theImage;
        else
            ioNextImage->m_nextImage = theImage;

        ioNextImage = theImage;

        if (inMaterial && inImageIndex >= QSSGShaderDefaultMaterialKeyProperties::SingleChannelImagesFirst) {
            QSSGRenderDefaultMaterial::TextureChannelMapping value = QSSGRenderDefaultMaterial::R;

            const quint32 scIndex = inImageIndex - QSSGShaderDefaultMaterialKeyProperties::SingleChannelImagesFirst;
            QSSGShaderKeyTextureChannel &channelKey = defaultMaterialShaderKeyProperties.m_textureChannels[scIndex];
            switch (inImageIndex) {
            case QSSGShaderDefaultMaterialKeyProperties::OpacityMap:
                value = inMaterial->opacityChannel;
                break;
            case QSSGShaderDefaultMaterialKeyProperties::RoughnessMap:
                value = inMaterial->roughnessChannel;
                break;
            case QSSGShaderDefaultMaterialKeyProperties::MetalnessMap:
                value = inMaterial->metalnessChannel;
                break;
            case QSSGShaderDefaultMaterialKeyProperties::OcclusionMap:
                value = inMaterial->occlusionChannel;
                break;
            case QSSGShaderDefaultMaterialKeyProperties::TranslucencyMap:
                value = inMaterial->translucencyChannel;
                break;
            case QSSGShaderDefaultMaterialKeyProperties::HeightMap:
                value = inMaterial->heightChannel;
                break;
            case QSSGShaderDefaultMaterialKeyProperties::ClearcoatMap:
                value = inMaterial->clearcoatChannel;
                break;
            case QSSGShaderDefaultMaterialKeyProperties::ClearcoatRoughnessMap:
                value = inMaterial->clearcoatRoughnessChannel;
                break;
            case QSSGShaderDefaultMaterialKeyProperties::TransmissionMap:
                value = inMaterial->transmissionChannel;
                break;
            case QSSGShaderDefaultMaterialKeyProperties::ThicknessMap:
                value = inMaterial->thicknessChannel;
                break;
            case QSSGShaderDefaultMaterialKeyProperties::BaseColorMap:
                value = inMaterial->baseColorChannel;
                break;
            case QSSGShaderDefaultMaterialKeyProperties::SpecularAmountMap:
                value = inMaterial->specularAmountChannel;
                break;
            case QSSGShaderDefaultMaterialKeyProperties::EmissiveMap:
                value = inMaterial->emissiveChannel;
                break;
            default:
                break;
            }
            bool useDefault = false;
            switch (value) {
            case QSSGRenderDefaultMaterial::TextureChannelMapping::G:
                useDefault = !hasG;
                break;
            case QSSGRenderDefaultMaterial::TextureChannelMapping::B:
                useDefault = !hasB;
                break;
            case QSSGRenderDefaultMaterial::TextureChannelMapping::A:
                useDefault = !hasA;
                break;
            default:
                break;
            }
            if (useDefault)
                value = QSSGRenderDefaultMaterial::R; // Always Fallback to Red
            channelKey.setTextureChannel(QSSGShaderKeyTextureChannel::TexturChannelBits(value), inShaderKey);
        }
    }
}

void QSSGLayerRenderData::setVertexInputPresence(const QSSGRenderableObjectFlags &renderableFlags,
                                                 QSSGShaderDefaultMaterialKey &key)
{
    quint32 vertexAttribs = 0;
    if (renderableFlags.hasAttributePosition())
        vertexAttribs |= QSSGShaderKeyVertexAttribute::Position;
    if (renderableFlags.hasAttributeNormal())
        vertexAttribs |= QSSGShaderKeyVertexAttribute::Normal;
    if (renderableFlags.hasAttributeTexCoord0())
        vertexAttribs |= QSSGShaderKeyVertexAttribute::TexCoord0;
    if (renderableFlags.hasAttributeTexCoord1())
        vertexAttribs |= QSSGShaderKeyVertexAttribute::TexCoord1;
    if (renderableFlags.hasAttributeTexCoordLightmap())
        vertexAttribs |= QSSGShaderKeyVertexAttribute::TexCoordLightmap;
    if (renderableFlags.hasAttributeTangent())
        vertexAttribs |= QSSGShaderKeyVertexAttribute::Tangent;
    if (renderableFlags.hasAttributeBinormal())
        vertexAttribs |= QSSGShaderKeyVertexAttribute::Binormal;
    if (renderableFlags.hasAttributeColor())
        vertexAttribs |= QSSGShaderKeyVertexAttribute::Color;
    if (renderableFlags.hasAttributeJointAndWeight())
        vertexAttribs |= QSSGShaderKeyVertexAttribute::JointAndWeight;
    defaultMaterialShaderKeyProperties.m_vertexAttributes.setValue(key, vertexAttribs);
}

QSSGDefaultMaterialPreparationResult QSSGLayerRenderData::prepareDefaultMaterialForRender(
        QSSGRenderDefaultMaterial &inMaterial,
        QSSGRenderableObjectFlags &inExistingFlags,
        float inOpacity,
        const QSSGShaderLightListView &lights,
        QSSGLayerRenderPreparationResultFlags &ioFlags)
{
    QSSGRenderDefaultMaterial *theMaterial = &inMaterial;
    QSSGDefaultMaterialPreparationResult retval(generateLightingKey(theMaterial->lighting, lights, inExistingFlags.receivesShadows()));
    retval.renderableFlags = inExistingFlags;
    QSSGRenderableObjectFlags &renderableFlags(retval.renderableFlags);
    QSSGShaderDefaultMaterialKey &theGeneratedKey(retval.materialKey);
    retval.opacity = inOpacity;
    float &subsetOpacity(retval.opacity);

    if (theMaterial->isDirty())
        renderableFlags |= QSSGRenderableObjectFlag::Dirty;

    subsetOpacity *= theMaterial->opacity;

    QSSGRenderableImage *firstImage = nullptr;

    defaultMaterialShaderKeyProperties.m_specularAAEnabled.setValue(theGeneratedKey, layer.specularAAEnabled);

    // isDoubleSided
    defaultMaterialShaderKeyProperties.m_isDoubleSided.setValue(theGeneratedKey, theMaterial->cullMode == QSSGCullFaceMode::Disabled);

    // default materials never define their on position
    defaultMaterialShaderKeyProperties.m_overridesPosition.setValue(theGeneratedKey, false);

    // default materials dont make use of raw projection or inverse projection matrices
    defaultMaterialShaderKeyProperties.m_usesProjectionMatrix.setValue(theGeneratedKey, false);
    defaultMaterialShaderKeyProperties.m_usesInverseProjectionMatrix.setValue(theGeneratedKey, false);
    // nor they do rely on VAR_COLOR
    defaultMaterialShaderKeyProperties.m_usesVarColor.setValue(theGeneratedKey, false);

    // alpha Mode
    defaultMaterialShaderKeyProperties.m_alphaMode.setValue(theGeneratedKey, theMaterial->alphaMode);

    // vertex attribute presence flags
    setVertexInputPresence(renderableFlags, theGeneratedKey);

    // set the flag indicating the need for gl_PointSize
    defaultMaterialShaderKeyProperties.m_usesPointsTopology.setValue(theGeneratedKey, renderableFlags.isPointsTopology());

    // propagate the flag indicating the presence of a lightmap
    defaultMaterialShaderKeyProperties.m_lightmapEnabled.setValue(theGeneratedKey, renderableFlags.rendersWithLightmap());

    defaultMaterialShaderKeyProperties.m_specularGlossyEnabled.setValue(theGeneratedKey, theMaterial->type == QSSGRenderGraphObject::Type::SpecularGlossyMaterial);

    // debug modes
    defaultMaterialShaderKeyProperties.m_debugMode.setValue(theGeneratedKey, int(layer.debugMode));

    // fog
    defaultMaterialShaderKeyProperties.m_fogEnabled.setValue(theGeneratedKey, layer.fog.enabled);

    // multiview
    defaultMaterialShaderKeyProperties.m_viewCount.setValue(theGeneratedKey, layer.viewCount);
    defaultMaterialShaderKeyProperties.m_usesViewIndex.setValue(theGeneratedKey, layer.viewCount >= 2);

    if (!defaultMaterialShaderKeyProperties.m_hasIbl.getValue(theGeneratedKey) && theMaterial->iblProbe) {
        features.set(QSSGShaderFeatures::Feature::LightProbe, true);
        defaultMaterialShaderKeyProperties.m_hasIbl.setValue(theGeneratedKey, true);
        // features.set(ShaderFeatureDefines::enableIblFov(),
        // m_Renderer.GetLayerRenderData()->m_Layer.m_ProbeFov < 180.0f );
    }

    if (subsetOpacity >= QSSG_RENDER_MINIMUM_RENDER_OPACITY) {

        // Set the semi-transparency flag as specified in PrincipledMaterial's
        // blendMode and alphaMode:
        // - the default SourceOver blendMode does not imply alpha blending on
        //   its own,
        // - but other blendMode values do,
        // - an alphaMode of Blend guarantees blending to be enabled regardless
        //   of anything else.
        // Additionally:
        // - Opacity and texture map alpha are handled elsewhere (that's when a
        //   blendMode of SourceOver or an alphaMode of Default/Opaque can in the
        //   end still result in HasTransparency),
        // - the presence of an opacityMap guarantees alpha blending regardless
        //   of its content.

        if (theMaterial->blendMode != QSSGRenderDefaultMaterial::MaterialBlendMode::SourceOver
                || theMaterial->opacityMap
                || theMaterial->alphaMode == QSSGRenderDefaultMaterial::Blend)
        {
            renderableFlags |= QSSGRenderableObjectFlag::HasTransparency;
        }

        const bool specularEnabled = theMaterial->isSpecularEnabled();
        const bool metalnessEnabled = theMaterial->isMetalnessEnabled();
        defaultMaterialShaderKeyProperties.m_specularEnabled.setValue(theGeneratedKey, (specularEnabled || metalnessEnabled));
        if (specularEnabled || metalnessEnabled)
            defaultMaterialShaderKeyProperties.m_specularModel.setSpecularModel(theGeneratedKey, theMaterial->specularModel);

        defaultMaterialShaderKeyProperties.m_fresnelScaleBiasEnabled.setValue(theGeneratedKey, theMaterial->isFresnelScaleBiasEnabled());

        defaultMaterialShaderKeyProperties.m_clearcoatFresnelScaleBiasEnabled.setValue(theGeneratedKey, theMaterial->isClearcoatFresnelScaleBiasEnabled());

        defaultMaterialShaderKeyProperties.m_fresnelEnabled.setValue(theGeneratedKey, theMaterial->isFresnelEnabled());

        defaultMaterialShaderKeyProperties.m_fresnelEnabled.setValue(theGeneratedKey, theMaterial->isFresnelEnabled());

        defaultMaterialShaderKeyProperties.m_baseColorSingleChannelEnabled.setValue(theGeneratedKey,
                                                                            theMaterial->isBaseColorSingleChannelEnabled());
        defaultMaterialShaderKeyProperties.m_specularSingleChannelEnabled.setValue(theGeneratedKey,
                                                                            theMaterial->isSpecularAmountSingleChannelEnabled());
        defaultMaterialShaderKeyProperties.m_emissiveSingleChannelEnabled.setValue(theGeneratedKey,
                                                                                   theMaterial->isEmissiveSingleChannelEnabled());
        defaultMaterialShaderKeyProperties.m_invertOpacityMapValue.setValue(theGeneratedKey,
                                                                            theMaterial->isInvertOpacityMapValue());
        defaultMaterialShaderKeyProperties.m_vertexColorsEnabled.setValue(theGeneratedKey,
                                                                                      theMaterial->isVertexColorsEnabled());
        defaultMaterialShaderKeyProperties.m_vertexColorsMaskEnabled.setValue(theGeneratedKey,
                                                                         theMaterial->isVertexColorsMaskEnabled());
        defaultMaterialShaderKeyProperties.m_vertexColorRedMask.setValue(theGeneratedKey,
                                                                         theMaterial->vertexColorRedMask.toInt());
        defaultMaterialShaderKeyProperties.m_vertexColorGreenMask.setValue(theGeneratedKey,
                                                                         quint16(theMaterial->vertexColorGreenMask.toInt()));
        defaultMaterialShaderKeyProperties.m_vertexColorBlueMask.setValue(theGeneratedKey,
                                                                         quint16(theMaterial->vertexColorBlueMask.toInt()));
        defaultMaterialShaderKeyProperties.m_vertexColorAlphaMask.setValue(theGeneratedKey,
                                                                         quint16(theMaterial->vertexColorAlphaMask.toInt()));

        defaultMaterialShaderKeyProperties.m_clearcoatEnabled.setValue(theGeneratedKey,
                                                                                   theMaterial->isClearcoatEnabled());
        defaultMaterialShaderKeyProperties.m_transmissionEnabled.setValue(theGeneratedKey,
                                                                                      theMaterial->isTransmissionEnabled());

        // Run through the material's images and prepare them for render.
        // this may in fact set pickable on the renderable flags if one of the images
        // links to a sub presentation or any offscreen rendered object.
        QSSGRenderableImage *nextImage = nullptr;
#define CHECK_IMAGE_AND_PREPARE(img, imgtype, shadercomponent)                          \
    if ((img))                                                                          \
        prepareImageForRender(*(img), imgtype, firstImage, nextImage, renderableFlags,  \
                              theGeneratedKey, shadercomponent, &inMaterial)

        if (theMaterial->type == QSSGRenderGraphObject::Type::PrincipledMaterial ||
            theMaterial->type == QSSGRenderGraphObject::Type::SpecularGlossyMaterial) {
            CHECK_IMAGE_AND_PREPARE(theMaterial->colorMap,
                                    QSSGRenderableImage::Type::BaseColor,
                                    QSSGShaderDefaultMaterialKeyProperties::BaseColorMap);
            CHECK_IMAGE_AND_PREPARE(theMaterial->occlusionMap,
                                    QSSGRenderableImage::Type::Occlusion,
                                    QSSGShaderDefaultMaterialKeyProperties::OcclusionMap);
            CHECK_IMAGE_AND_PREPARE(theMaterial->heightMap,
                                    QSSGRenderableImage::Type::Height,
                                    QSSGShaderDefaultMaterialKeyProperties::HeightMap);
            CHECK_IMAGE_AND_PREPARE(theMaterial->clearcoatMap,
                                    QSSGRenderableImage::Type::Clearcoat,
                                    QSSGShaderDefaultMaterialKeyProperties::ClearcoatMap);
            CHECK_IMAGE_AND_PREPARE(theMaterial->clearcoatRoughnessMap,
                                    QSSGRenderableImage::Type::ClearcoatRoughness,
                                    QSSGShaderDefaultMaterialKeyProperties::ClearcoatRoughnessMap);
            CHECK_IMAGE_AND_PREPARE(theMaterial->clearcoatNormalMap,
                                    QSSGRenderableImage::Type::ClearcoatNormal,
                                    QSSGShaderDefaultMaterialKeyProperties::ClearcoatNormalMap);
            CHECK_IMAGE_AND_PREPARE(theMaterial->transmissionMap,
                                    QSSGRenderableImage::Type::Transmission,
                                    QSSGShaderDefaultMaterialKeyProperties::TransmissionMap);
            CHECK_IMAGE_AND_PREPARE(theMaterial->thicknessMap,
                                    QSSGRenderableImage::Type::Thickness,
                                    QSSGShaderDefaultMaterialKeyProperties::ThicknessMap);
            if (theMaterial->type == QSSGRenderGraphObject::Type::PrincipledMaterial) {
                CHECK_IMAGE_AND_PREPARE(theMaterial->metalnessMap,
                                        QSSGRenderableImage::Type::Metalness,
                                        QSSGShaderDefaultMaterialKeyProperties::MetalnessMap);
            }
        } else {
            CHECK_IMAGE_AND_PREPARE(theMaterial->colorMap,
                                    QSSGRenderableImage::Type::Diffuse,
                                    QSSGShaderDefaultMaterialKeyProperties::DiffuseMap);
        }
        CHECK_IMAGE_AND_PREPARE(theMaterial->emissiveMap, QSSGRenderableImage::Type::Emissive, QSSGShaderDefaultMaterialKeyProperties::EmissiveMap);
        CHECK_IMAGE_AND_PREPARE(theMaterial->specularReflection,
                                QSSGRenderableImage::Type::Specular,
                                QSSGShaderDefaultMaterialKeyProperties::SpecularMap);
        CHECK_IMAGE_AND_PREPARE(theMaterial->roughnessMap,
                                QSSGRenderableImage::Type::Roughness,
                                QSSGShaderDefaultMaterialKeyProperties::RoughnessMap);
        CHECK_IMAGE_AND_PREPARE(theMaterial->opacityMap, QSSGRenderableImage::Type::Opacity, QSSGShaderDefaultMaterialKeyProperties::OpacityMap);
        CHECK_IMAGE_AND_PREPARE(theMaterial->bumpMap, QSSGRenderableImage::Type::Bump, QSSGShaderDefaultMaterialKeyProperties::BumpMap);
        CHECK_IMAGE_AND_PREPARE(theMaterial->specularMap,
                                QSSGRenderableImage::Type::SpecularAmountMap,
                                QSSGShaderDefaultMaterialKeyProperties::SpecularAmountMap);
        CHECK_IMAGE_AND_PREPARE(theMaterial->normalMap, QSSGRenderableImage::Type::Normal, QSSGShaderDefaultMaterialKeyProperties::NormalMap);
        CHECK_IMAGE_AND_PREPARE(theMaterial->translucencyMap,
                                QSSGRenderableImage::Type::Translucency,
                                QSSGShaderDefaultMaterialKeyProperties::TranslucencyMap);
    }
#undef CHECK_IMAGE_AND_PREPARE

    if (subsetOpacity < QSSG_RENDER_MINIMUM_RENDER_OPACITY) {
        subsetOpacity = 0.0f;
        // You can still pick against completely transparent objects(or rather their bounding
        // box)
        // you just don't render them.
        renderableFlags |= QSSGRenderableObjectFlag::HasTransparency;
        renderableFlags |= QSSGRenderableObjectFlag::CompletelyTransparent;
    }

    if (subsetOpacity > 1.f - QSSG_RENDER_MINIMUM_RENDER_OPACITY)
        subsetOpacity = 1.f;
    else
        renderableFlags |= QSSGRenderableObjectFlag::HasTransparency;

    if (inMaterial.isTransmissionEnabled()) {
        ioFlags.setRequiresScreenTexture(true);
        ioFlags.setRequiresMipmapsForScreenTexture(true);
        renderableFlags |= QSSGRenderableObjectFlag::RequiresScreenTexture;
    }

    if (renderableFlags.hasTransparency()) {
        if (orderIndependentTransparencyEnabled)
            defaultMaterialShaderKeyProperties.m_orderIndependentTransparency.setValue(theGeneratedKey, int(layer.oitMethod));
        if (layer.oitMethodDirty)
            renderableFlags |= QSSGRenderableObjectFlag::Dirty;
    }

    retval.firstImage = firstImage;
    if (retval.renderableFlags.isDirty())
        retval.dirty = true;
    if (retval.dirty)
        renderer->addMaterialDirtyClear(&inMaterial);
    return retval;
}

QSSGDefaultMaterialPreparationResult QSSGLayerRenderData::prepareCustomMaterialForRender(
        QSSGRenderCustomMaterial &inMaterial, QSSGRenderableObjectFlags &inExistingFlags,
        float inOpacity, bool alreadyDirty, const QSSGShaderLightListView &lights,
        QSSGLayerRenderPreparationResultFlags &ioFlags)
{
    QSSGDefaultMaterialPreparationResult retval(
                generateLightingKey(QSSGRenderDefaultMaterial::MaterialLighting::FragmentLighting,
                                    lights, inExistingFlags.receivesShadows()));
    retval.renderableFlags = inExistingFlags;
    QSSGRenderableObjectFlags &renderableFlags(retval.renderableFlags);
    QSSGShaderDefaultMaterialKey &theGeneratedKey(retval.materialKey);
    retval.opacity = inOpacity;
    float &subsetOpacity(retval.opacity);

    if (subsetOpacity < QSSG_RENDER_MINIMUM_RENDER_OPACITY) {
        subsetOpacity = 0.0f;
        // You can still pick against completely transparent objects(or rather their bounding
        // box)
        // you just don't render them.
        renderableFlags |= QSSGRenderableObjectFlag::HasTransparency;
        renderableFlags |= QSSGRenderableObjectFlag::CompletelyTransparent;
    }

    if (subsetOpacity > 1.f - QSSG_RENDER_MINIMUM_RENDER_OPACITY)
        subsetOpacity = 1.f;
    else
        renderableFlags |= QSSGRenderableObjectFlag::HasTransparency;

    defaultMaterialShaderKeyProperties.m_specularAAEnabled.setValue(theGeneratedKey, layer.specularAAEnabled);

    // isDoubleSided
    defaultMaterialShaderKeyProperties.m_isDoubleSided.setValue(theGeneratedKey,
                                                                            inMaterial.m_cullMode == QSSGCullFaceMode::Disabled);

    // Does the material override the position output
    const bool overridesPosition = inMaterial.m_renderFlags.testFlag(QSSGRenderCustomMaterial::RenderFlag::OverridesPosition);
    defaultMaterialShaderKeyProperties.m_overridesPosition.setValue(theGeneratedKey, overridesPosition);

    // Optional usage of PROJECTION_MATRIX and/or INVERSE_PROJECTION_MATRIX
    const bool usesProjectionMatrix = inMaterial.m_renderFlags.testFlag(QSSGRenderCustomMaterial::RenderFlag::ProjectionMatrix);
    defaultMaterialShaderKeyProperties.m_usesProjectionMatrix.setValue(theGeneratedKey, usesProjectionMatrix);
    const bool usesInvProjectionMatrix = inMaterial.m_renderFlags.testFlag(QSSGRenderCustomMaterial::RenderFlag::InverseProjectionMatrix);
    defaultMaterialShaderKeyProperties.m_usesInverseProjectionMatrix.setValue(theGeneratedKey, usesInvProjectionMatrix);

    // vertex attribute presence flags
    setVertexInputPresence(renderableFlags, theGeneratedKey);

    // set the flag indicating the need for gl_PointSize
    defaultMaterialShaderKeyProperties.m_usesPointsTopology.setValue(theGeneratedKey, renderableFlags.isPointsTopology());

    // propagate the flag indicating the presence of a lightmap
    defaultMaterialShaderKeyProperties.m_lightmapEnabled.setValue(theGeneratedKey, renderableFlags.rendersWithLightmap());

    // debug modes
    defaultMaterialShaderKeyProperties.m_debugMode.setValue(theGeneratedKey, int(layer.debugMode));

    // fog
    defaultMaterialShaderKeyProperties.m_fogEnabled.setValue(theGeneratedKey, layer.fog.enabled);

    // multiview
    defaultMaterialShaderKeyProperties.m_viewCount.setValue(theGeneratedKey, layer.viewCount);
    defaultMaterialShaderKeyProperties.m_usesViewIndex.setValue(theGeneratedKey,
        inMaterial.m_renderFlags.testFlag(QSSGRenderCustomMaterial::RenderFlag::ViewIndex));

    // Knowing whether VAR_COLOR is used becomes relevant when there is no
    // custom vertex shader, but VAR_COLOR is present in the custom fragment
    // snippet, because that case needs special care.
    const bool usesVarColor = inMaterial.m_renderFlags.testFlag(QSSGRenderCustomMaterial::RenderFlag::VarColor);
    defaultMaterialShaderKeyProperties.m_usesVarColor.setValue(theGeneratedKey, usesVarColor);

    const bool usesClearcoat = inMaterial.m_renderFlags.testFlag(QSSGRenderCustomMaterial::RenderFlag::Clearcoat);
    defaultMaterialShaderKeyProperties.m_clearcoatEnabled.setValue(theGeneratedKey, usesClearcoat);

    const bool usesClearcoatFresnelScaleBias = inMaterial.m_renderFlags.testFlag(QSSGRenderCustomMaterial::RenderFlag::ClearcoatFresnelScaleBias);
    defaultMaterialShaderKeyProperties.m_clearcoatFresnelScaleBiasEnabled.setValue(theGeneratedKey, usesClearcoatFresnelScaleBias);

    const bool usesFresnelScaleBias = inMaterial.m_renderFlags.testFlag(QSSGRenderCustomMaterial::RenderFlag::FresnelScaleBias);
    defaultMaterialShaderKeyProperties.m_fresnelScaleBiasEnabled.setValue(theGeneratedKey, usesFresnelScaleBias);

    const bool usesTransmission = inMaterial.m_renderFlags.testFlag(QSSGRenderCustomMaterial::RenderFlag::Transmission);
    defaultMaterialShaderKeyProperties.m_transmissionEnabled.setValue(theGeneratedKey, usesTransmission);

    if (inMaterial.m_renderFlags.testFlag(QSSGRenderCustomMaterial::RenderFlag::Blending))
        renderableFlags |= QSSGRenderableObjectFlag::HasTransparency;

    if (inMaterial.m_renderFlags.testFlag(QSSGRenderCustomMaterial::RenderFlag::ScreenTexture)) {
        ioFlags.setRequiresScreenTexture(true);
        renderableFlags |= QSSGRenderableObjectFlag::RequiresScreenTexture;
    }

    if (inMaterial.m_renderFlags.testFlag(QSSGRenderCustomMaterial::RenderFlag::ScreenMipTexture)) {
        ioFlags.setRequiresScreenTexture(true);
        ioFlags.setRequiresMipmapsForScreenTexture(true);
        renderableFlags |= QSSGRenderableObjectFlag::RequiresScreenTexture;
    }

    if (inMaterial.m_renderFlags.testFlag(QSSGRenderCustomMaterial::RenderFlag::DepthTexture))
        ioFlags.setRequiresDepthTexture(true);

    if (inMaterial.m_renderFlags.testFlag(QSSGRenderCustomMaterial::RenderFlag::AoTexture)) {
        ioFlags.setRequiresDepthTexture(true);
        ioFlags.setRequiresSsaoPass(true);
    }
    if (orderIndependentTransparencyEnabled && renderableFlags.hasTransparency())
        defaultMaterialShaderKeyProperties.m_orderIndependentTransparency.setValue(theGeneratedKey, int(layer.oitMethod));
    retval.firstImage = nullptr;

    if (retval.dirty || alreadyDirty)
        renderer->addMaterialDirtyClear(&inMaterial);
    return retval;
}

enum class CullUnrenderables { Off, On };
template <CullUnrenderables cull = CullUnrenderables::On>
static void prepareModelMaterialsImpl(QSSGLayerRenderData::RenderableNodeEntries &renderableModels)
{
    const auto originalModelCount = renderableModels.size();
    auto end = originalModelCount;

    for (int idx = 0; idx < end; ++idx) {
        const auto &renderable = renderableModels.at(idx);
        const QSSGRenderModel &model = *static_cast<QSSGRenderModel *>(renderable.node);
        // Ensure we have at least 1 material
        if ((renderable.overridden & QSSGRenderableNodeEntry::Overridden::Materials) == 0)
            renderable.materials = model.materials;

        if constexpr (cull == CullUnrenderables::On) {
            const bool isDisabled = ((renderable.overridden & QSSGRenderableNodeEntry::Overridden::Disabled) != 0);
            if (isDisabled || renderable.materials.isEmpty()) {
                // Swap current (idx) and last item (--end).
                // Note, post-decrement idx to ensure we recheck the new current item on next iteration
                // and pre-decrement the end move the end of the list to not include the culled renderable.
                renderableModels.swapItemsAt(idx--, --end);
            }
        }
    }

    if constexpr (cull == CullUnrenderables::On) {
        // Any models without materials get dropped right here
        if (end != originalModelCount)
            renderableModels.resize(end);
    }
}

void QSSGLayerRenderData::prepareModelMaterials(RenderableNodeEntries &renderableModels, bool cullUnrenderables)
{
    if (cullUnrenderables)
        prepareModelMaterialsImpl<CullUnrenderables::On>(renderableModels);
    else
        prepareModelMaterialsImpl<CullUnrenderables::Off>(renderableModels);
}

void QSSGLayerRenderData::prepareModelMeshes(const QSSGRenderContextInterface &contextInterface,
                                             RenderableNodeEntries &renderableModels,
                                             bool globalPickingEnabled)
{
    const auto &bufferManager = contextInterface.bufferManager();

    const auto originalModelCount = renderableModels.size();
    auto end = originalModelCount;

    for (int idx = 0; idx < end; ++idx) {
        // It's up to the BufferManager to employ the appropriate caching mechanisms, so
        // loadMesh() is expected to be fast if already loaded. Note that preparing
        // the same QSSGRenderModel in different QQuickWindows (possible when a
        // scene is shared between View3Ds where the View3Ds belong to different
        // windows) leads to a different QSSGRenderMesh since the BufferManager is,
        // very correctly, per window, and so per scenegraph render thread.

        const auto &renderable = renderableModels.at(idx);
        const QSSGRenderModel &model = *static_cast<QSSGRenderModel *>(renderable.node);
        // Ensure we have a mesh
        if (auto *theMesh = bufferManager->loadMesh(&model)) {
            renderable.mesh = theMesh;
            // Completely transparent models cannot be pickable.  But models with completely
            // transparent materials still are.  This allows the artist to control pickability
            // in a somewhat fine-grained style.
            const bool canModelBePickable = (model.globalOpacity > QSSG_RENDER_MINIMUM_RENDER_OPACITY)
                    && (globalPickingEnabled
                        || model.getGlobalState(QSSGRenderModel::GlobalState::Pickable));
            if (canModelBePickable) {
                // Check if there is BVH data, if not generate it
                if (!theMesh->bvh) {
                    if (!model.meshPath.isNull())
                        theMesh->bvh = bufferManager->loadMeshBVH(model.meshPath);
                    else if (model.geometry)
                        theMesh->bvh = bufferManager->loadMeshBVH(model.geometry);

                    if (theMesh->bvh) {
                        const auto &roots = theMesh->bvh->roots();
                        for (qsizetype i = 0, end = qsizetype(roots.size()); i < end; ++i)
                            theMesh->subsets[i].bvhRoot = roots[i];
                    }
                }
            }
        } else {
            // Swap current (idx) and last item (--end).
            // Note, post-decrement idx to ensure we recheck the new current item on next iteration
            // and pre-decrement the end move the end of the list to not include the culled renderable.
            renderableModels.swapItemsAt(idx--, --end);
        }
    }

    // Any models without a mesh get dropped right here
    if (end != originalModelCount)
        renderableModels.resize(end);

    // Now is the time to kick off the vertex/index buffer updates for all the
    // new meshes (and their submeshes). This here is the last possible place
    // to kick this off because the rest of the rendering pipeline will only
    // see the individual sub-objects as "renderable objects".
    bufferManager->commitBufferResourceUpdates();
}

void QSSGLayerRenderData::setLightmapTexture(const QSSGModelContext &modelContext, QRhiTexture *lightmapTexture)
{
    lightmapTextures[&modelContext] = lightmapTexture;
}

QRhiTexture *QSSGLayerRenderData::getLightmapTexture(const QSSGModelContext &modelContext) const
{
    QRhiTexture *ret = nullptr;
    if (modelContext.model.hasLightmap()) {
        const auto it = lightmapTextures.constFind(&modelContext);
        ret = (it != lightmapTextures.cend()) ? *it : nullptr;
    }

    return ret;
}

void QSSGLayerRenderData::setBonemapTexture(const QSSGModelContext &modelContext, QRhiTexture *bonemapTexture)
{
    bonemapTextures[&modelContext] = bonemapTexture;
}

QRhiTexture *QSSGLayerRenderData::getBonemapTexture(const QSSGModelContext &modelContext) const
{
    QRhiTexture *ret = nullptr;
    if (modelContext.model.usesBoneTexture()) {
        const auto it = bonemapTextures.constFind(&modelContext);
        ret = (it != bonemapTextures.cend()) ? *it : nullptr;
    }

    return ret;
}

static bool hasCustomBlendMode(const QSSGRenderCustomMaterial &material)
{
    // Check SrcOver

    // srcAlpha is same for all
    if (material.m_srcAlphaBlend != QRhiGraphicsPipeline::One)
        return true;

    // SrcOver srcColor is SrcAlpha
    if (material.m_srcBlend != QRhiGraphicsPipeline::SrcAlpha)
        return true;

    if (material.m_dstBlend == QRhiGraphicsPipeline::OneMinusSrcAlpha
        && material.m_dstAlphaBlend == QRhiGraphicsPipeline::OneMinusSrcAlpha)
        return false;
    return true;
}

// inModel is const to emphasize the fact that its members cannot be written
// here: in case there is a scene shared between multiple View3Ds in different
// QQuickWindows, each window may run this in their own render thread, while
// inModel is the same.
bool QSSGLayerRenderData::prepareModelsForRender(QSSGRenderContextInterface &contextInterface,
                                                 const RenderableNodeEntries &renderableModels,
                                                 QSSGLayerRenderPreparationResultFlags &ioFlags,
                                                 const QSSGRenderCameraList &allCameras,
                                                 const QSSGRenderCameraDataList &allCameraData,
                                                 TModelContextPtrList &modelContexts,
                                                 QSSGRenderableObjectList &opaqueObjects,
                                                 QSSGRenderableObjectList &transparentObjects,
                                                 QSSGRenderableObjectList &screenTextureObjects,
                                                 float lodThreshold)
{
    const auto &rhiCtx = contextInterface.rhiContext();
    const auto &bufferManager = contextInterface.bufferManager();

    const auto &debugDrawSystem = contextInterface.debugDrawSystem();
    const bool maybeDebugDraw = debugDrawSystem && debugDrawSystem->isEnabled();

    bool wasDirty = false;

    for (const QSSGRenderableNodeEntry &renderable : renderableModels) {
        if ((renderable.overridden & QSSGRenderableNodeEntry::Overridden::Disabled) != 0)
            continue;

        const QSSGRenderModel &model = *static_cast<QSSGRenderModel *>(renderable.node);
        const auto &lights = renderable.lights;
        QSSGRenderMesh *theMesh = renderable.mesh;

        QSSG_ASSERT_X(theMesh != nullptr, "Only renderables with a mesh will be processed!", continue);

        const bool altGlobalTransform = ((renderable.overridden & QSSGRenderableNodeEntry::Overridden::GlobalTransform) != 0);
        const auto &globalTransform = altGlobalTransform ? renderable.globalTransform : model.globalTransform;
        QSSGModelContext &theModelContext = *RENDER_FRAME_NEW<QSSGModelContext>(contextInterface, model, globalTransform, allCameraData);
        modelContexts.push_back(&theModelContext);
        // We might over-allocate here, as the material list technically can contain an invalid (nullptr) material.
        // We'll fix that by adjusting the size at the end for now...
        const auto &meshSubsets = theMesh->subsets;
        const auto meshSubsetCount = meshSubsets.size();
        theModelContext.subsets = RENDER_FRAME_NEW_BUFFER<QSSGSubsetRenderable>(contextInterface, meshSubsetCount);

        // Prepare boneTexture for skinning
        if (model.skin) {
            auto boneTexture = bufferManager->loadSkinmap(model.skin);
            setBonemapTexture(theModelContext, boneTexture.m_texture);
        } else if (model.skeleton) {
            auto boneTexture = bufferManager->loadSkinmap(&(model.skeleton->boneTexData));
            setBonemapTexture(theModelContext, boneTexture.m_texture);
        } else {
            setBonemapTexture(theModelContext, nullptr);
        }

        // many renderableFlags are the same for all the subsets
        QSSGRenderableObjectFlags renderableFlagsForModel;

        if (meshSubsetCount > 0) {
            const QSSGRenderSubset &theSubset = meshSubsets.at(0);

            renderableFlagsForModel.setCastsShadows(model.castsShadows);
            renderableFlagsForModel.setReceivesShadows(model.receivesShadows);
            renderableFlagsForModel.setReceivesReflections(model.receivesReflections);
            renderableFlagsForModel.setCastsReflections(model.castsReflections);

            renderableFlagsForModel.setUsedInBakedLighting(model.usedInBakedLighting);
            if (model.hasLightmap()) {
                QSSGRenderImageTexture lmImageTexture = bufferManager->loadLightmap(model);
                if (lmImageTexture.m_texture) {
                    renderableFlagsForModel.setRendersWithLightmap(true);
                    setLightmapTexture(theModelContext, lmImageTexture.m_texture);
                }
            }

            // TODO: This should be a oneshot thing, move the flags over!
            // With the RHI we need to be able to tell the material shader
            // generator to not generate vertex input attributes that are not
            // provided by the mesh. (because unlike OpenGL, other graphics
            // APIs may treat unbound vertex inputs as a fatal error)
            bool hasJoint = false;
            bool hasWeight = false;
            bool hasMorphTarget = theSubset.rhi.targetsTexture != nullptr;
            for (const QSSGRhiInputAssemblerState::InputSemantic &sem : std::as_const(theSubset.rhi.ia.inputs)) {
                if (sem == QSSGRhiInputAssemblerState::PositionSemantic) {
                    renderableFlagsForModel.setHasAttributePosition(true);
                } else if (sem == QSSGRhiInputAssemblerState::NormalSemantic) {
                    renderableFlagsForModel.setHasAttributeNormal(true);
                } else if (sem == QSSGRhiInputAssemblerState::TexCoord0Semantic) {
                    renderableFlagsForModel.setHasAttributeTexCoord0(true);
                } else if (sem == QSSGRhiInputAssemblerState::TexCoord1Semantic) {
                    renderableFlagsForModel.setHasAttributeTexCoord1(true);
                } else if (sem == QSSGRhiInputAssemblerState::TexCoordLightmapSemantic) {
                    renderableFlagsForModel.setHasAttributeTexCoordLightmap(true);
                } else if (sem == QSSGRhiInputAssemblerState::TangentSemantic) {
                    renderableFlagsForModel.setHasAttributeTangent(true);
                } else if (sem == QSSGRhiInputAssemblerState::BinormalSemantic) {
                    renderableFlagsForModel.setHasAttributeBinormal(true);
                } else if (sem == QSSGRhiInputAssemblerState::ColorSemantic) {
                    renderableFlagsForModel.setHasAttributeColor(true);
                    // For skinning, we will set the HasAttribute only
                    // if the mesh has both joint and weight
                } else if (sem == QSSGRhiInputAssemblerState::JointSemantic) {
                    hasJoint = true;
                } else if (sem == QSSGRhiInputAssemblerState::WeightSemantic) {
                    hasWeight = true;
                }
            }
            renderableFlagsForModel.setHasAttributeJointAndWeight(hasJoint && hasWeight);
            renderableFlagsForModel.setHasAttributeMorphTarget(hasMorphTarget);
        }

        QSSGRenderableObjectList bakedLightingObjects;
        bool usesBlendParticles = particlesEnabled && theModelContext.model.particleBuffer != nullptr
                && model.particleBuffer->particleCount();
        const bool anyLightHasShadows = std::find_if(lights.begin(),
                                                     lights.end(),
                                                     [](const QSSGShaderLight &light) { return light.shadows; })
                != lights.end();

        // Subset(s)
        auto &renderableSubsets = theModelContext.subsets;
        const auto &materials = renderable.materials;
        const auto materialCount = materials.size();
        const bool altModelOpacity = ((renderable.overridden & QSSGRenderableNodeEntry::Overridden::GlobalOpacity) != 0);
        const float modelOpacity = altModelOpacity ? renderable.globalOpacity : model.globalOpacity;
        QSSGRenderGraphObject *lastMaterial = !materials.isEmpty() ? materials.last() : nullptr;
        int idx = 0, subsetIdx = 0;
        for (; idx < meshSubsetCount; ++idx) {
            // If the materials list < size of subsets, then use the last material for the rest
            QSSGRenderGraphObject *theMaterialObject = (idx >= materialCount) ? lastMaterial : materials[idx];
            QSSG_ASSERT_X(theMaterialObject != nullptr, "No material found for model!", continue);

            const QSSGRenderSubset &theSubset = meshSubsets.at(idx);
            QSSGRenderableObjectFlags renderableFlags = renderableFlagsForModel;
            float subsetOpacity = modelOpacity;

            renderableFlags.setPointsTopology(theSubset.rhi.ia.topology == QRhiGraphicsPipeline::Points);
            QSSGRenderableObject *theRenderableObject = &renderableSubsets[subsetIdx++];

            bool usesInstancing = theModelContext.model.instancing()
                    && rhiCtx->rhi()->isFeatureSupported(QRhi::Instancing);
            if (usesInstancing && theModelContext.model.instanceTable->hasTransparency())
                renderableFlags |= QSSGRenderableObjectFlag::HasTransparency;
            if (theModelContext.model.hasTransparency)
                renderableFlags |= QSSGRenderableObjectFlag::HasTransparency;

            // Level Of Detail
            quint32 subsetLevelOfDetail = 0;
            if (!theSubset.lods.isEmpty() && lodThreshold > 0.0f) {
                // Accounts for FOV
                float lodDistanceMultiplier = cameras[0]->getLevelOfDetailMultiplier();
                float distanceThreshold = 0.0f;
                const auto scale = QSSGUtils::mat44::getScale(model.globalTransform);
                float modelScale = qMax(scale.x(), qMax(scale.y(), scale.z()));
                QSSGBounds3 transformedBounds = theSubset.bounds;
                if (cameras[0]->type != QSSGRenderGraphObject::Type::OrthographicCamera) {
                    transformedBounds.transform(model.globalTransform);
                    if (maybeDebugDraw && debugDrawSystem->isEnabled(QSSGDebugDrawSystem::Mode::MeshLod))
                        debugDrawSystem->drawBounds(transformedBounds, QColor(Qt::red));
                    const QVector3D cameraNormal = cameras[0]->getScalingCorrectDirection();
                    const QVector3D cameraPosition = cameras[0]->getGlobalPos();
                    const QSSGPlane cameraPlane = QSSGPlane(cameraPosition, cameraNormal);
                    const QVector3D lodSupportMin = transformedBounds.getSupport(-cameraNormal);
                    const QVector3D lodSupportMax = transformedBounds.getSupport(cameraNormal);
                    if (maybeDebugDraw && debugDrawSystem->isEnabled(QSSGDebugDrawSystem::Mode::MeshLod))
                        debugDrawSystem->drawPoint(lodSupportMin, QColor("orange"));

                    const float distanceMin = cameraPlane.distance(lodSupportMin);
                    const float distanceMax = cameraPlane.distance(lodSupportMax);

                    if (distanceMin * distanceMax < 0.0)
                        distanceThreshold = 0.0;
                    else if (distanceMin >= 0.0)
                        distanceThreshold = distanceMin;
                    else if (distanceMax <= 0.0)
                        distanceThreshold = -distanceMax;

                } else {
                    // Orthographic Projection
                    distanceThreshold = 1.0;
                }

                int currentLod = -1;
                if (model.levelOfDetailBias > 0.0f) {
                    const float threshold = distanceThreshold * lodDistanceMultiplier;
                    const float modelBias = 1 / model.levelOfDetailBias;
                    for (qsizetype i = 0; i < theSubset.lods.count(); ++i) {
                        float subsetDistance = theSubset.lods[i].distance * modelScale * modelBias;
                        float screenSize = subsetDistance / threshold;
                        if (screenSize > lodThreshold)
                            break;
                        currentLod = i;
                    }
                }
                if (currentLod == -1)
                    subsetLevelOfDetail = 0;
                else
                    subsetLevelOfDetail = currentLod + 1;
                if (maybeDebugDraw && debugDrawSystem->isEnabled(QSSGDebugDrawSystem::Mode::MeshLod))
                    debugDrawSystem->drawBounds(transformedBounds, QSSGDebugDrawSystem::levelOfDetailColor(subsetLevelOfDetail));
            }

            QVector3D theModelCenter(theSubset.bounds.center());
            theModelCenter = QSSGUtils::mat44::transform(model.globalTransform, theModelCenter);
            if (maybeDebugDraw && debugDrawSystem->isEnabled(QSSGDebugDrawSystem::Mode::MeshLodNormal))
                debugDrawSystem->debugNormals(*bufferManager, theModelContext, theSubset, subsetLevelOfDetail, (theModelCenter - allCameras[0]->getGlobalPos()).length() * 0.01);

            static auto checkF32TypeIndex = [&rhiCtx](QRhiVertexInputAttribute::Format f) {
                if ((f ==  QRhiVertexInputAttribute::Format::Float4)
                        || (f == QRhiVertexInputAttribute::Format::Float3)
                        || (f == QRhiVertexInputAttribute::Format::Float2)
                        || (f == QRhiVertexInputAttribute::Format::Float)) {
                    return true;
                }
                if (!rhiCtx->rhi()->isFeatureSupported(QRhi::IntAttributes))
                    qWarning() << "WARN: Model has non-integer type indices for skinning but current RHI backend doesn't support it!";
                return false;
            };

            if (theMaterialObject->type == QSSGRenderGraphObject::Type::DefaultMaterial ||
                theMaterialObject->type == QSSGRenderGraphObject::Type::PrincipledMaterial ||
                theMaterialObject->type == QSSGRenderGraphObject::Type::SpecularGlossyMaterial) {
                QSSGRenderDefaultMaterial &theMaterial(static_cast<QSSGRenderDefaultMaterial &>(*theMaterialObject));
                QSSGDefaultMaterialPreparationResult theMaterialPrepResult(prepareDefaultMaterialForRender(theMaterial, renderableFlags, subsetOpacity, lights, ioFlags));
                QSSGShaderDefaultMaterialKey &theGeneratedKey(theMaterialPrepResult.materialKey);
                subsetOpacity = theMaterialPrepResult.opacity;
                QSSGRenderableImage *firstImage(theMaterialPrepResult.firstImage);
                wasDirty |= theMaterialPrepResult.dirty;
                renderableFlags = theMaterialPrepResult.renderableFlags;
                if (renderableFlags.hasTransparency())
                    ioFlags.setHasCustomBlendMode(theMaterial.blendMode != QSSGRenderDefaultMaterial::MaterialBlendMode::SourceOver);

                // Blend particles
                defaultMaterialShaderKeyProperties.m_blendParticles.setValue(theGeneratedKey, usesBlendParticles);

                // Skin
                const auto boneCount = model.skin ? model.skin->boneCount :
                                                    model.skeleton ? model.skeleton->boneCount : 0;
                defaultMaterialShaderKeyProperties.m_boneCount.setValue(theGeneratedKey, boneCount);
                if (auto idJoint = theSubset.rhi.ia.inputs.indexOf(QSSGRhiInputAssemblerState::JointSemantic); idJoint != -1) {
                    const auto attr = theSubset.rhi.ia.inputLayout.attributeAt(idJoint);
                    defaultMaterialShaderKeyProperties.m_usesFloatJointIndices.setValue(theGeneratedKey, checkF32TypeIndex(attr->format()));
                }

                // Instancing
                defaultMaterialShaderKeyProperties.m_usesInstancing.setValue(theGeneratedKey, usesInstancing);
                // Morphing
                defaultMaterialShaderKeyProperties.m_targetCount.setValue(theGeneratedKey,
                                        theSubset.rhi.ia.targetCount);
                defaultMaterialShaderKeyProperties.m_targetPositionOffset.setValue(theGeneratedKey,
                                        theSubset.rhi.ia.targetOffsets[QSSGRhiInputAssemblerState::PositionSemantic]);
                defaultMaterialShaderKeyProperties.m_targetNormalOffset.setValue(theGeneratedKey,
                                        theSubset.rhi.ia.targetOffsets[QSSGRhiInputAssemblerState::NormalSemantic]);
                defaultMaterialShaderKeyProperties.m_targetTangentOffset.setValue(theGeneratedKey,
                                        theSubset.rhi.ia.targetOffsets[QSSGRhiInputAssemblerState::TangentSemantic]);
                defaultMaterialShaderKeyProperties.m_targetBinormalOffset.setValue(theGeneratedKey,
                                        theSubset.rhi.ia.targetOffsets[QSSGRhiInputAssemblerState::BinormalSemantic]);
                defaultMaterialShaderKeyProperties.m_targetTexCoord0Offset.setValue(theGeneratedKey,
                                        theSubset.rhi.ia.targetOffsets[QSSGRhiInputAssemblerState::TexCoord0Semantic]);
                defaultMaterialShaderKeyProperties.m_targetTexCoord1Offset.setValue(theGeneratedKey,
                                        theSubset.rhi.ia.targetOffsets[QSSGRhiInputAssemblerState::TexCoord1Semantic]);
                defaultMaterialShaderKeyProperties.m_targetColorOffset.setValue(theGeneratedKey,
                                        theSubset.rhi.ia.targetOffsets[QSSGRhiInputAssemblerState::ColorSemantic]);

                new (theRenderableObject) QSSGSubsetRenderable(QSSGSubsetRenderable::Type::DefaultMaterialMeshSubset,
                                                               renderableFlags,
                                                               theModelCenter,
                                                               renderer,
                                                               theSubset,
                                                               theModelContext,
                                                               subsetOpacity,
                                                               subsetLevelOfDetail,
                                                               theMaterial,
                                                               firstImage,
                                                               theGeneratedKey,
                                                               lights,
                                                               anyLightHasShadows);
                wasDirty = wasDirty || renderableFlags.isDirty();
            } else if (theMaterialObject->type == QSSGRenderGraphObject::Type::CustomMaterial) {
                QSSGRenderCustomMaterial &theMaterial(static_cast<QSSGRenderCustomMaterial &>(*theMaterialObject));

                const auto &theMaterialSystem(contextInterface.customMaterialSystem());
                wasDirty |= theMaterialSystem->prepareForRender(theModelContext.model, theSubset, theMaterial);

                if (theMaterial.m_renderFlags.testFlag(QSSGRenderCustomMaterial::RenderFlag::Blending))
                    ioFlags.setHasCustomBlendMode(!hasCustomBlendMode(theMaterial));

                QSSGDefaultMaterialPreparationResult theMaterialPrepResult(
                        prepareCustomMaterialForRender(theMaterial, renderableFlags, subsetOpacity, wasDirty,
                                                       lights, ioFlags));
                QSSGShaderDefaultMaterialKey &theGeneratedKey(theMaterialPrepResult.materialKey);
                subsetOpacity = theMaterialPrepResult.opacity;
                QSSGRenderableImage *firstImage(theMaterialPrepResult.firstImage);
                renderableFlags = theMaterialPrepResult.renderableFlags;

                if (model.particleBuffer && model.particleBuffer->particleCount())
                    defaultMaterialShaderKeyProperties.m_blendParticles.setValue(theGeneratedKey, true);
                else
                    defaultMaterialShaderKeyProperties.m_blendParticles.setValue(theGeneratedKey, false);

                // Skin
                const auto boneCount = model.skin ? model.skin->boneCount :
                                                    model.skeleton ? model.skeleton->boneCount : 0;
                defaultMaterialShaderKeyProperties.m_boneCount.setValue(theGeneratedKey, boneCount);
                if (auto idJoint = theSubset.rhi.ia.inputs.indexOf(QSSGRhiInputAssemblerState::JointSemantic); idJoint != -1) {
                    const auto attr = theSubset.rhi.ia.inputLayout.attributeAt(idJoint);
                    defaultMaterialShaderKeyProperties.m_usesFloatJointIndices.setValue(theGeneratedKey, checkF32TypeIndex(attr->format()));
                }

                // Instancing
                bool usesInstancing = theModelContext.model.instancing()
                        && rhiCtx->rhi()->isFeatureSupported(QRhi::Instancing);
                defaultMaterialShaderKeyProperties.m_usesInstancing.setValue(theGeneratedKey, usesInstancing);
                // Morphing
                defaultMaterialShaderKeyProperties.m_targetCount.setValue(theGeneratedKey,
                                        theSubset.rhi.ia.targetCount);
                defaultMaterialShaderKeyProperties.m_targetPositionOffset.setValue(theGeneratedKey,
                                        theSubset.rhi.ia.targetOffsets[QSSGRhiInputAssemblerState::PositionSemantic]);
                defaultMaterialShaderKeyProperties.m_targetNormalOffset.setValue(theGeneratedKey,
                                        theSubset.rhi.ia.targetOffsets[QSSGRhiInputAssemblerState::NormalSemantic]);
                defaultMaterialShaderKeyProperties.m_targetTangentOffset.setValue(theGeneratedKey,
                                        theSubset.rhi.ia.targetOffsets[QSSGRhiInputAssemblerState::TangentSemantic]);
                defaultMaterialShaderKeyProperties.m_targetBinormalOffset.setValue(theGeneratedKey,
                                        theSubset.rhi.ia.targetOffsets[QSSGRhiInputAssemblerState::BinormalSemantic]);
                defaultMaterialShaderKeyProperties.m_targetTexCoord0Offset.setValue(theGeneratedKey,
                                        theSubset.rhi.ia.targetOffsets[QSSGRhiInputAssemblerState::TexCoord0Semantic]);
                defaultMaterialShaderKeyProperties.m_targetTexCoord1Offset.setValue(theGeneratedKey,
                                        theSubset.rhi.ia.targetOffsets[QSSGRhiInputAssemblerState::TexCoord1Semantic]);
                defaultMaterialShaderKeyProperties.m_targetColorOffset.setValue(theGeneratedKey,
                                        theSubset.rhi.ia.targetOffsets[QSSGRhiInputAssemblerState::ColorSemantic]);

                if (theMaterial.m_iblProbe)
                    theMaterial.m_iblProbe->clearDirty();

                new (theRenderableObject) QSSGSubsetRenderable(QSSGSubsetRenderable::Type::CustomMaterialMeshSubset,
                                                               renderableFlags,
                                                               theModelCenter,
                                                               renderer,
                                                               theSubset,
                                                               theModelContext,
                                                               subsetOpacity,
                                                               subsetLevelOfDetail,
                                                               theMaterial,
                                                               firstImage,
                                                               theGeneratedKey,
                                                               lights,
                                                               anyLightHasShadows);
            }
            if (theRenderableObject) // NOTE: Should just go in with the ctor args
                theRenderableObject->camdistSq = getCameraDistanceSq(*theRenderableObject, allCameraData[0]);
        }

        // If the indices don't match then something's off and we need to adjust the subset renderable list size.
        if (Q_UNLIKELY(idx != subsetIdx))
            renderableSubsets.mSize = subsetIdx + 1;

        for (auto &ro : renderableSubsets) {
            const auto depthMode = ro.depthWriteMode;
            hasDepthWriteObjects |= (depthMode == QSSGDepthDrawMode::Always || depthMode == QSSGDepthDrawMode::OpaqueOnly);
            enum ObjectType : quint8 { ScreenTexture, Transparent, Opaque };
            static constexpr DepthPrepassObject ppState[][2] = { {DepthPrepassObject::None, DepthPrepassObject::ScreenTexture},
                                                                 {DepthPrepassObject::None, DepthPrepassObject::Transparent},
                                                                 {DepthPrepassObject::None, DepthPrepassObject::Opaque} };

            if (ro.renderableFlags.requiresScreenTexture()) {
                depthPrepassObjectsState |= DepthPrepassObjectStateT(ppState[ObjectType::ScreenTexture][size_t(depthMode == QSSGDepthDrawMode::OpaquePrePass)]);
                screenTextureObjects.push_back({&ro, ro.camdistSq});
            } else if (ro.renderableFlags.hasTransparency()) {
                depthPrepassObjectsState |= DepthPrepassObjectStateT(ppState[ObjectType::Transparent][size_t(depthMode == QSSGDepthDrawMode::OpaquePrePass)]);
                transparentObjects.push_back({&ro, ro.camdistSq});
            } else {
                depthPrepassObjectsState |= DepthPrepassObjectStateT(ppState[ObjectType::Opaque][size_t(depthMode == QSSGDepthDrawMode::OpaquePrePass)]);
                opaqueObjects.push_back({&ro, ro.camdistSq});
            }

            if (ro.renderableFlags.usedInBakedLighting())
                bakedLightingObjects.push_back({&ro, ro.camdistSq});
        }

        if (!bakedLightingObjects.isEmpty())
            bakedLightingModels.push_back(QSSGBakedLightingModel(&model, bakedLightingObjects));
    }

    return wasDirty;
}

bool QSSGLayerRenderData::prepareParticlesForRender(const RenderableNodeEntries &renderableParticles, const QSSGRenderCameraData &cameraData, QSSGLayerRenderPreparationResultFlags &ioFlags)
{
    QSSG_ASSERT(particlesEnabled, return false);

    QSSGRenderContextInterface &contextInterface = *renderer->contextInterface();

    bool dirty = false;

    //
    auto &opaqueObjects = opaqueObjectStore[0];
    auto &transparentObjects = transparentObjectStore[0];
    auto &screenTextureObjects = screenTextureObjectStore[0];

    for (const auto &renderable : renderableParticles) {
        const QSSGRenderParticles &particles = *static_cast<QSSGRenderParticles *>(renderable.node);
        const auto &lights = renderable.lights;

        QSSGRenderableObjectFlags renderableFlags;
        renderableFlags.setCastsShadows(false);
        renderableFlags.setReceivesShadows(false);
        renderableFlags.setHasAttributePosition(true);
        renderableFlags.setHasAttributeNormal(true);
        renderableFlags.setHasAttributeTexCoord0(true);
        renderableFlags.setHasAttributeColor(true);
        renderableFlags.setHasTransparency(particles.m_hasTransparency);
        renderableFlags.setCastsReflections(particles.m_castsReflections);
        if (particles.m_hasTransparency && particles.m_blendMode != QSSGRenderParticles::BlendMode::SourceOver)
            ioFlags.setHasCustomBlendMode(true);

        float opacity = particles.globalOpacity;
        QVector3D center(particles.m_particleBuffer.bounds().center());
        center = QSSGUtils::mat44::transform(particles.globalTransform, center);

        QSSGRenderableImage *firstImage = nullptr;
        if (particles.m_sprite) {
            const auto &bufferManager = contextInterface.bufferManager();

            if (particles.m_sprite->clearDirty())
                dirty = true;

            const QSSGRenderImageTexture texture = bufferManager->loadRenderImage(particles.m_sprite);
            QSSGRenderableImage *theImage = RENDER_FRAME_NEW<QSSGRenderableImage>(contextInterface, QSSGRenderableImage::Type::Diffuse, *particles.m_sprite, texture);
            firstImage = theImage;
        }

        QSSGRenderableImage *colorTable = nullptr;
        if (particles.m_colorTable) {
            const auto &bufferManager = contextInterface.bufferManager();

            if (particles.m_colorTable->clearDirty())
                dirty = true;

            const QSSGRenderImageTexture texture = bufferManager->loadRenderImage(particles.m_colorTable);

            QSSGRenderableImage *theImage = RENDER_FRAME_NEW<QSSGRenderableImage>(contextInterface, QSSGRenderableImage::Type::Diffuse, *particles.m_colorTable, texture);
            colorTable = theImage;
        }

        if (opacity > 0.0f && particles.m_particleBuffer.particleCount()) {
            auto *theRenderableObject = RENDER_FRAME_NEW<QSSGParticlesRenderable>(contextInterface,
                                                                                  renderableFlags,
                                                                                  center,
                                                                                  renderer,
                                                                                  particles,
                                                                                  firstImage,
                                                                                  colorTable,
                                                                                  lights,
                                                                                  opacity);
            if (theRenderableObject) {
                if (theRenderableObject->renderableFlags.requiresScreenTexture())
                    screenTextureObjects.push_back({theRenderableObject, getCameraDistanceSq(*theRenderableObject, cameraData)});
                else if (theRenderableObject->renderableFlags.hasTransparency())
                    transparentObjects.push_back({theRenderableObject, getCameraDistanceSq(*theRenderableObject, cameraData)});
                else
                    opaqueObjects.push_back({theRenderableObject, getCameraDistanceSq(*theRenderableObject, cameraData)});
            }
        }
    }

    return dirty;
}

bool QSSGLayerRenderData::prepareItem2DsForRender(const QSSGRenderContextInterface &ctxIfc,
                                                  const RenderableItem2DEntries &renderableItem2Ds)
{
    const bool hasItems = (renderableItem2Ds.size() != 0);
    if (hasItems) {
        const auto &clipSpaceCorrMatrix = ctxIfc.rhiContext()->rhi()->clipSpaceCorrMatrix();
        const QSSGRenderCameraDataList &cameraDatas(getCachedCameraDatas());
        for (const auto &theItem2D : renderableItem2Ds) {
            theItem2D->mvps.clear();
            for (const QSSGRenderCameraData &camData : cameraDatas) {
                QMatrix4x4 mvp = camData.viewProjection * theItem2D->globalTransform;
                static const QMatrix4x4 flipMatrix(1.0f, 0.0f, 0.0f, 0.0f,
                                                0.0f, -1.0f, 0.0f, 0.0f,
                                                0.0f, 0.0f, 1.0f, 0.0f,
                                                0.0f, 0.0f, 0.0f, 1.0f);
                mvp = clipSpaceCorrMatrix * mvp * flipMatrix;
                theItem2D->mvps.append(mvp);
            }
        }
    }

    return hasItems;
}

void QSSGLayerRenderData::prepareResourceLoaders()
{
    QSSGRenderContextInterface &contextInterface = *renderer->contextInterface();
    const auto &bufferManager = contextInterface.bufferManager();

    for (const auto resourceLoader : std::as_const(layer.resourceLoaders))
        bufferManager->processResourceLoader(static_cast<QSSGRenderResourceLoader *>(resourceLoader));
}

void QSSGLayerRenderData::prepareReflectionProbesForRender()
{
    const auto probeCount = reflectionProbes.size();
    requestReflectionMapManager(); // ensure that we have a reflection map manager

    for (int i = 0; i < probeCount; i++) {
        QSSGRenderReflectionProbe* probe = reflectionProbes.at(i);

        int reflectionObjectCount = 0;
        QVector3D probeExtent = probe->boxSize / 2;
        QSSGBounds3 probeBound = QSSGBounds3::centerExtents(probe->getGlobalPos() + probe->boxOffset, probeExtent);

        const auto injectProbe = [&](const QSSGRenderableObjectHandle &handle) {
            if (handle.obj->renderableFlags.testFlag(QSSGRenderableObjectFlag::ReceivesReflections)
                && !(handle.obj->type == QSSGRenderableObject::Type::Particles)) {
                QSSGSubsetRenderable* renderableObj = static_cast<QSSGSubsetRenderable*>(handle.obj);
                QSSGBounds3 nodeBound = renderableObj->bounds;
                QVector4D vmin(nodeBound.minimum, 1.0);
                QVector4D vmax(nodeBound.maximum, 1.0);
                vmin = renderableObj->globalTransform * vmin;
                vmax = renderableObj->globalTransform * vmax;
                nodeBound.minimum = vmin.toVector3D();
                nodeBound.maximum = vmax.toVector3D();
                if (probeBound.intersects(nodeBound)) {
                    QVector3D nodeBoundCenter = nodeBound.center();
                    QVector3D probeBoundCenter = probeBound.center();
                    float distance = nodeBoundCenter.distanceToPoint(probeBoundCenter);
                    if (renderableObj->reflectionProbeIndex == -1 || distance < renderableObj->distanceFromReflectionProbe) {
                        renderableObj->reflectionProbeIndex = i;
                        renderableObj->distanceFromReflectionProbe = distance;
                        renderableObj->reflectionProbe.parallaxCorrection = probe->parallaxCorrection;
                        renderableObj->reflectionProbe.probeCubeMapCenter = probe->getGlobalPos();
                        renderableObj->reflectionProbe.probeBoxMax = probeBound.maximum;
                        renderableObj->reflectionProbe.probeBoxMin = probeBound.minimum;
                        renderableObj->reflectionProbe.enabled = true;
                        reflectionObjectCount++;
                    }
                }
            }
        };

        const auto &transparentObjects = std::as_const(transparentObjectStore[0]);
        const auto &opaqueObjects = std::as_const(opaqueObjectStore[0]);
        const auto &screenTextureObjects = std::as_const(screenTextureObjectStore[0]);

        for (const auto &handle : std::as_const(transparentObjects))
            injectProbe(handle);

        for (const auto &handle : std::as_const(opaqueObjects))
            injectProbe(handle);

        for (const auto &handle : std::as_const(screenTextureObjects))
            injectProbe(handle);

        if (probe->texture)
            reflectionMapManager->addTexturedReflectionMapEntry(i, *probe);
        else if (reflectionObjectCount > 0)
            reflectionMapManager->addReflectionMapEntry(i, *probe);
    }
}

static bool scopeLight(QSSGRenderNode *node, QSSGRenderNode *lightScope)
{
    // check if the node is parent of the lightScope
    while (node) {
        if (node == lightScope)
            return true;
        node = node->parent;
    }
    return false;
}

static const int REDUCED_MAX_LIGHT_COUNT_THRESHOLD_BYTES = 4096; // 256 vec4

static inline int effectiveMaxLightCount(const QSSGShaderFeatures &features)
{
    if (features.isSet(QSSGShaderFeatures::Feature::ReduceMaxNumLights))
        return QSSG_REDUCED_MAX_NUM_LIGHTS;

    return QSSG_MAX_NUM_LIGHTS;
}

void updateDirtySkeletons(const QVector<QSSGRenderableNodeEntry> &renderableNodes)
{
    // First model using skeleton clears the dirty flag so we need another mechanism
    // to tell to the other models the skeleton is dirty.
    QSet<QSSGRenderSkeleton *> dirtySkeletons;
    for (const auto &node : std::as_const(renderableNodes)) {
        if (node.node->type == QSSGRenderGraphObject::Type::Model) {
            auto modelNode = static_cast<QSSGRenderModel *>(node.node);
            auto skeletonNode = modelNode->skeleton;
            bool hcj = false;
            if (skeletonNode) {
                const bool dirtySkeleton = dirtySkeletons.contains(skeletonNode);
                const bool hasDirtyNonJoints = (skeletonNode->containsNonJointNodes
                                                && (hasDirtyNonJointNodes(skeletonNode, hcj) || dirtySkeleton));
                const bool dirtyTransform = skeletonNode->isDirty(QSSGRenderNode::DirtyFlag::TransformDirty);
                if (skeletonNode->skinningDirty || hasDirtyNonJoints || dirtyTransform) {
                    skeletonNode->boneTransformsDirty = false;
                    if (hasDirtyNonJoints && !dirtySkeleton)
                        dirtySkeletons.insert(skeletonNode);
                    skeletonNode->skinningDirty = false;
                    const qsizetype dataSize = BONEDATASIZE4ID(skeletonNode->maxIndex);
                    if (skeletonNode->boneData.size() < dataSize)
                        skeletonNode->boneData.resize(dataSize);
                    skeletonNode->calculateGlobalVariables();
                    skeletonNode->containsNonJointNodes = false;
                    for (auto &child : skeletonNode->children)
                        collectBoneTransforms(&child, skeletonNode, modelNode->inverseBindPoses);
                }
                skeletonNode->boneCount = skeletonNode->boneData.size() / 2 / 4 / 16;
                const int boneTexWidth = qCeil(qSqrt(skeletonNode->boneCount * 4 * 2));
                skeletonNode->boneTexData.setSize(QSize(boneTexWidth, boneTexWidth));
                skeletonNode->boneData.resize(boneTexWidth * boneTexWidth * 16);
                skeletonNode->boneTexData.setTextureData(skeletonNode->boneData);
            }
            const int numMorphTarget = modelNode->morphTargets.size();
            for (int i = 0; i < numMorphTarget; ++i) {
                auto morphTarget = static_cast<const QSSGRenderMorphTarget *>(modelNode->morphTargets.at(i));
                modelNode->morphWeights[i] = morphTarget->weight;
                modelNode->morphAttributes[i] = morphTarget->attributes;
                if (i > MAX_MORPH_TARGET_INDEX_SUPPORTS_NORMALS)
                    modelNode->morphAttributes[i] &= 0x1; // MorphTarget.Position
                else if (i > MAX_MORPH_TARGET_INDEX_SUPPORTS_TANGENTS)
                    modelNode->morphAttributes[i] &= 0x3; // MorphTarget.Position | MorphTarget.Normal
            }
        }
    }

    dirtySkeletons.clear();
}

void QSSGLayerRenderData::prepareForRender()
{
    QSSG_ASSERT_X(layerPrepResult.isNull(), "Prep-result was not reset for render!", layerPrepResult = {});

    QRect theViewport(renderer->viewport());

    // NOTE: The renderer won't change in practice (after being set the first time), but just update
    // it anyways.
    frameData.m_ctx = renderer->contextInterface();
    frameData.clear();

    // Create base pipeline state
    ps = {}; // Reset
    ps.viewport = { float(theViewport.x()), float(theViewport.y()), float(theViewport.width()), float(theViewport.height()), 0.0f, 1.0f };
    if (layer.scissorRect.isValid()) {
        ps.flags |= QSSGRhiGraphicsPipelineState::Flag::UsesScissor;
        ps.scissor = { layer.scissorRect.x(),
                       theViewport.height() - (layer.scissorRect.y() + layer.scissorRect.height()),
                       layer.scissorRect.width(),
                       layer.scissorRect.height() };
    }

    ps.depthFunc = QRhiGraphicsPipeline::LessOrEqual;
    ps.flags.setFlag(QSSGRhiGraphicsPipelineState::Flag::BlendEnabled, false);

    // Enable Wireframe mode
    ps.polygonMode = layer.wireframeMode ? QRhiGraphicsPipeline::Line : QRhiGraphicsPipeline::Fill;

    bool wasDirty = false;
    bool wasDataDirty = false;
    wasDirty = layer.isDirty();

    layerPrepResult = { theViewport, layer };

    // SSAO
    const bool SSAOEnabled = layer.ssaoEnabled();
    layerPrepResult.flags.setRequiresSsaoPass(SSAOEnabled);
    features.set(QSSGShaderFeatures::Feature::Ssao, SSAOEnabled);

    // Effects
    bool requiresDepthTexture = SSAOEnabled;
    for (QSSGRenderEffect *theEffect = layer.firstEffect; theEffect; theEffect = theEffect->m_nextEffect) {
        if (theEffect->isDirty()) {
            wasDirty = true;
            theEffect->clearDirty();
        }
        if (theEffect->requiresDepthTexture)
            requiresDepthTexture = true;
    }

    const auto &rhiCtx = renderer->contextInterface()->rhiContext();
    orderIndependentTransparencyEnabled = (layer.oitMethod != QSSGRenderLayer::OITMethod::None);
    if (layer.oitMethod == QSSGRenderLayer::OITMethod::WeightedBlended) {
        orderIndependentTransparencyEnabled = rhiCtx->rhi()->isFeatureSupported(QRhi::PerRenderTargetBlending);
        if (rhiCtx->mainPassSampleCount() > 1)
            orderIndependentTransparencyEnabled |= rhiCtx->rhi()->isFeatureSupported(QRhi::TexelFetch);
    }
    if (layer.oitMethodDirty) {
        oitRenderContext.reset();
        for (auto &renderResult : renderResults)
            renderResult.reset();
    }

    layerPrepResult.flags.setRequiresDepthTexture(requiresDepthTexture);

    // Tonemapping. Except when there are effects, then it is up to the
    // last pass of the last effect to perform tonemapping.
    if (!layer.firstEffect)
        QSSGLayerRenderData::setTonemapFeatures(features, layer.tonemapMode);

    // We may not be able to have an array of 15 light struct elements in
    // the shaders. Switch on the reduced-max-number-of-lights feature
    // if necessary. In practice this is relevant with OpenGL ES 3.0 or
    // 2.0, because there are still implementations in use that only
    // support the spec mandated minimum of 224 vec4s (so 3584 bytes).
    if (rhiCtx->rhi()->resourceLimit(QRhi::MaxUniformBufferRange) < REDUCED_MAX_LIGHT_COUNT_THRESHOLD_BYTES) {
        features.set(QSSGShaderFeatures::Feature::ReduceMaxNumLights, true);
        static bool notified = false;
        if (!notified) {
            notified = true;
            qCDebug(lcQuick3DRender, "Qt Quick 3D maximum number of lights has been reduced from %d to %d due to the graphics driver's limitations",
                    QSSG_MAX_NUM_LIGHTS, QSSG_REDUCED_MAX_NUM_LIGHTS);
        }
    }

    // IBL Lightprobe Image
    QSSGRenderImageTexture lightProbeTexture;
    if (layer.lightProbe) {
        const auto &lightProbeSettings = layer.lightProbeSettings;
        if (layer.lightProbe->m_format == QSSGRenderTextureFormat::Unknown) {
            // Choose on a format that makes sense for a light probe
            // At this point it's just a suggestion
            if (renderer->contextInterface()->rhiContext()->rhi()->isTextureFormatSupported(QRhiTexture::RGBA16F))
                layer.lightProbe->m_format = QSSGRenderTextureFormat::RGBA16F;
            else
                layer.lightProbe->m_format = QSSGRenderTextureFormat::RGBE8;
        }

        if (layer.lightProbe->clearDirty())
            wasDataDirty = true;

        // NOTE: This call can lead to rendering (of envmap) and a texture upload
        lightProbeTexture = renderer->contextInterface()->bufferManager()->loadRenderImage(layer.lightProbe, QSSGBufferManager::MipModeBsdf);
        if (lightProbeTexture.m_texture) {

            features.set(QSSGShaderFeatures::Feature::LightProbe, true);
            features.set(QSSGShaderFeatures::Feature::IblOrientation, !lightProbeSettings.probeOrientation.isIdentity());

            // By this point we will know what the actual texture format of the light probe is
            // Check if using RGBE format light probe texture (the Rhi format will be RGBA8)
            if (lightProbeTexture.m_flags.isRgbe8())
                features.set(QSSGShaderFeatures::Feature::RGBELightProbe, true);
        } else {
            layer.lightProbe = nullptr;
        }

        const bool forceIblExposureValues = (features.isSet(QSSGShaderFeatures::Feature::LightProbe) && layer.tonemapMode == QSSGRenderLayer::TonemapMode::Custom);
        features.set(QSSGShaderFeatures::Feature::ForceIblExposure, forceIblExposureValues);
    }

    // Gather Spatial Nodes from Render Tree
    // Do not just clear() renderableNodes and friends. Rather, reuse
    // the space (even if clear does not actually deallocate, it still
    // costs time to run dtors and such). In scenes with a static node
    // count in the range of thousands this may matter.
    int renderableModelsCount = 0;
    int renderableParticlesCount = 0;
    int renderableItem2DsCount = 0;
    int cameraNodeCount = 0;
    int lightNodeCount = 0;
    int reflectionProbeCount = 0;
    quint32 dfsIndex = 0;
    for (auto &theChild : layer.children)
        wasDataDirty |= maybeQueueNodeForRender(theChild,
                                                renderableModels,
                                                renderableModelsCount,
                                                renderableParticles,
                                                renderableParticlesCount,
                                                renderableItem2Ds,
                                                renderableItem2DsCount,
                                                cameras,
                                                cameraNodeCount,
                                                lights,
                                                lightNodeCount,
                                                reflectionProbes,
                                                reflectionProbeCount,
                                                dfsIndex);

    if (renderableModels.size() != renderableModelsCount)
        renderableModels.resize(renderableModelsCount);
    if (renderableParticles.size() != renderableParticlesCount)
        renderableParticles.resize(renderableParticlesCount);
    if (renderableItem2Ds.size() != renderableItem2DsCount)
        renderableItem2Ds.resize(renderableItem2DsCount);

    if (cameras.size() != cameraNodeCount)
        cameras.resize(cameraNodeCount);
    if (lights.size() != lightNodeCount)
        lights.resize(lightNodeCount);
    if (reflectionProbes.size() != reflectionProbeCount)
        reflectionProbes.resize(reflectionProbeCount);

    // Cameras
    // 1. If there's an explicit camera set and it's active (visible) we'll use that.
    // 2. ... if the explicitly set camera is not visible, no further attempts will be done.
    // 3. If no explicit camera is set, we'll search and pick the first active camera.
    renderedCameras.clear();
    if (!layer.explicitCameras.isEmpty()) {
        for (QSSGRenderCamera *cam : std::as_const(layer.explicitCameras)) {
            // 1.
            wasDataDirty = wasDataDirty || cam->isDirty();
            QSSGCameraGlobalCalculationResult theResult = layerPrepResult.setupCameraForRender(*cam, renderer->dpr());
            wasDataDirty = wasDataDirty || theResult.m_wasDirty;
            if (!theResult.m_computeFrustumSucceeded)
                qCCritical(INTERNAL_ERROR, "Failed to calculate camera frustum");

            // 2.
            if (cam->getGlobalState(QSSGRenderCamera::GlobalState::Active))
                renderedCameras.append(cam);
        }
    } else if (QSSG_GUARD_X(layer.viewCount == 1, "Multiview rendering requires explicit cameras to be set!.")) {
        // NOTE: This path can never be hit with multiview, hence the guard.
        // (Multiview will always have explicit cameras set.)

        // 3.
        for (auto iter = cameras.cbegin(); renderedCameras.isEmpty() && iter != cameras.cend(); iter++) {
            QSSGRenderCamera *theCamera = *iter;
            wasDataDirty = wasDataDirty || theCamera->isDirty();
            QSSGCameraGlobalCalculationResult theResult = layerPrepResult.setupCameraForRender(*theCamera, renderer->dpr());
            wasDataDirty = wasDataDirty || theResult.m_wasDirty;
            if (!theResult.m_computeFrustumSucceeded)
                qCCritical(INTERNAL_ERROR, "Failed to calculate camera frustum");
            if (theCamera->getGlobalState(QSSGRenderCamera::GlobalState::Active))
                renderedCameras.append(theCamera);
        }
    }

    for (QSSGRenderCamera *cam : renderedCameras)
        cam->dpr = renderer->dpr();

    float meshLodThreshold = 1.0f;
    if (!renderedCameras.isEmpty())
        meshLodThreshold = renderedCameras[0]->levelOfDetailPixelThreshold / theViewport.width();

    layer.renderedCamerasMutex.lock();
    layer.renderedCameras = renderedCameras;
    layer.renderedCamerasMutex.unlock();

    // ResourceLoaders
    prepareResourceLoaders();

    // Skeletons
    updateDirtySkeletons(renderableModels);

    // Lights
    int shadowMapCount = 0;
    bool hasScopedLights = false;
    // Determine which lights will actually Render
    // Determine how many lights will need shadow maps
    // NOTE: This culling is specific to our Forward renderer
    const int maxLightCount = effectiveMaxLightCount(features);
    const bool showLightCountWarning = !tooManyLightsWarningShown && (lights.size() > maxLightCount);
    if (showLightCountWarning) {
        qWarning("Too many lights in scene, maximum is %d", maxLightCount);
        tooManyLightsWarningShown = true;
    }

    QSSGShaderLightList renderableLights; // All lights (upto 'maxLightCount')

    // List should contain only enabled lights (active && birghtness > 0).
    {
        auto it = lights.crbegin();
        const auto end = it + qMin(maxLightCount, lights.size());

        for (; it != end; ++it) {
            QSSGRenderLight *renderLight = (*it);
            hasScopedLights |= (renderLight->m_scope != nullptr);
            const bool mightCastShadows = renderLight->m_castShadow && !renderLight->m_fullyBaked;
            const bool shadows = mightCastShadows && (shadowMapCount < QSSG_MAX_NUM_SHADOW_MAPS);
            shadowMapCount += int(shadows);
            const auto &direction = renderLight->getScalingCorrectDirection();
            renderableLights.push_back(QSSGShaderLight{ renderLight, shadows, direction });
        }

        if ((shadowMapCount >= QSSG_MAX_NUM_SHADOW_MAPS) && !tooManyShadowLightsWarningShown) {
            qWarning("Too many shadow casting lights in scene, maximum is %d", QSSG_MAX_NUM_SHADOW_MAPS);
            tooManyShadowLightsWarningShown = true;
        }
    }

    if (shadowMapCount > 0) { // Setup Shadow Maps Entries for Lights casting shadows
        requestShadowMapManager(); // Ensure we have a shadow map manager
        layerPrepResult.flags.setRequiresShadowMapPass(true);
        // Any light with castShadow=true triggers shadow mapping
        // in the generated shaders. The fact that some (or even
        // all) objects may opt out from receiving shadows plays no
        // role here whatsoever.
        features.set(QSSGShaderFeatures::Feature::Ssm, true);
        shadowMapManager->addShadowMaps(renderableLights);
    } else if (shadowMapManager) {
        // No shadows but a shadow manager so clear old resources
        shadowMapManager->releaseCachedResources();
    }

    // Give each renderable a copy of the lights available
    // Also setup scoping for scoped lights

    QSSG_ASSERT(globalLights.isEmpty(), globalLights.clear());
    if (hasScopedLights) { // Filter out scoped lights from the global lights list
        for (const auto &shaderLight : std::as_const(renderableLights)) {
            if (!shaderLight.light->m_scope)
                globalLights.push_back(shaderLight);
        }

        const auto prepareLightsWithScopedLights = [&renderableLights, this](QVector<QSSGRenderableNodeEntry> &renderableNodes) {
            for (qint32 idx = 0, end = renderableNodes.size(); idx < end; ++idx) {
                QSSGRenderableNodeEntry &theNodeEntry(renderableNodes[idx]);
                QSSGShaderLightList filteredLights;
                for (const auto &light : std::as_const(renderableLights)) {
                    if (light.light->m_scope && !scopeLight(theNodeEntry.node, light.light->m_scope))
                        continue;
                    filteredLights.push_back(light);
                }

                if (filteredLights.isEmpty()) { // Node without scoped lights, just reference the global light list.
                    theNodeEntry.lights = QSSGDataView(globalLights);
                } else {
                    // This node has scoped lights, i.e., it's lights differ from the global list
                    // we therefore create a bespoke light list for it. Technically this might be the same for
                    // more then this one node, but the overhead for tracking that is not worth it.
                    auto customLightList = RENDER_FRAME_NEW_BUFFER<QSSGShaderLight>(*renderer->contextInterface(), filteredLights.size());
                    std::copy(filteredLights.cbegin(), filteredLights.cend(), customLightList.begin());
                    theNodeEntry.lights = customLightList;
                }
            }
        };

        prepareLightsWithScopedLights(renderableModels);
        prepareLightsWithScopedLights(renderableParticles);
    } else { // Just a simple copy
        globalLights = renderableLights;
        // No scoped lights, all nodes can just reference the global light list.
        const auto prepareLights = [this](QVector<QSSGRenderableNodeEntry> &renderableNodes) {
            for (qint32 idx = 0, end = renderableNodes.size(); idx < end; ++idx) {
                QSSGRenderableNodeEntry &theNodeEntry(renderableNodes[idx]);
                theNodeEntry.lights = QSSGDataView(globalLights);
            }
        };

        prepareLights(renderableModels);
        prepareLights(renderableParticles);
    }

    bool hasUserExtensions = false;

    {
        // Give user provided passes a chance to modify the renderable data before starting
        // Note: All non-active extensions should be filtered out by now
        Q_STATIC_ASSERT(USERPASSES == size_t(QSSGRenderLayer::RenderExtensionStage::Count));
        for (size_t i = 0; i != size_t(QSSGRenderLayer::RenderExtensionStage::Count); ++i) {
            const auto &renderExtensions = layer.renderExtensions[i];
            auto &userPass = userPasses[i];
            for (auto rit = renderExtensions.crbegin(), rend = renderExtensions.crend(); rit != rend; ++rit) {
                hasUserExtensions = true;
                if ((*rit)->prepareData(frameData)) {
                    wasDirty |= true;
                    userPass.extensions.push_back(*rit);
                }
            }
        }
    }

    // Ensure materials (If there are user extensions we don't cull renderables without materials!)
    prepareModelMaterials(renderableModels, !hasUserExtensions);
    // Ensure meshes for models
    prepareModelMeshes(*renderer->contextInterface(), renderableModels, QSSGRendererPrivate::isGlobalPickingEnabled(*renderer));

    auto &opaqueObjects = opaqueObjectStore[0];
    auto &transparentObjects = transparentObjectStore[0];
    auto &screenTextureObjects = screenTextureObjectStore[0];

    if (!renderedCameras.isEmpty()) { // NOTE: We shouldn't really get this far without a camera...
        wasDirty |= prepareModelsForRender(*renderer->contextInterface(), renderableModels, layerPrepResult.flags, renderedCameras, getCachedCameraDatas(), modelContexts, opaqueObjects, transparentObjects, screenTextureObjects, meshLodThreshold);
        if (particlesEnabled) {
            const auto &cameraDatas = getCachedCameraDatas();
            wasDirty |= prepareParticlesForRender(renderableParticles, cameraDatas[0], layerPrepResult.flags);
        }
        wasDirty |= prepareItem2DsForRender(*renderer->contextInterface(), renderableItem2Ds);
    }
    if (orderIndependentTransparencyEnabled) {
        // OIT blending mode must be SourceOver and have transparent objects
        if (transparentObjects.size() > 0 && !layerPrepResult.flags.hasCustomBlendMode()) {
            if (layer.oitMethod == QSSGRenderLayer::OITMethod::WeightedBlended) {
                if (rhiCtx->mainPassSampleCount() > 1)
                    layerPrepResult.flags.setRequiresDepthTextureMS(true);
                else
                    layerPrepResult.flags.setRequiresDepthTexture(true);
            }
        } else {
            orderIndependentTransparencyEnabled = false;
        }
    }
    layer.oitMethodDirty = false;

    prepareReflectionProbesForRender();

    wasDirty = wasDirty || wasDataDirty;
    layerPrepResult.flags.setWasDirty(wasDirty);
    layerPrepResult.flags.setLayerDataDirty(wasDataDirty);

    //
    const bool animating = wasDirty;
    if (animating)
        layer.progAAPassIndex = 0;

    const bool progressiveAA = layer.isProgressiveAAEnabled() && !animating;
    layer.progressiveAAIsActive = progressiveAA;
    const bool temporalAA = layer.isTemporalAAEnabled() && !progressiveAA;

    layer.temporalAAIsActive = temporalAA;

    QVector2D vertexOffsetsAA;

    if (progressiveAA && layer.progAAPassIndex > 0 && layer.progAAPassIndex < quint32(layer.antialiasingQuality)) {
        int idx = layer.progAAPassIndex - 1;
        vertexOffsetsAA = s_ProgressiveAAVertexOffsets[idx] / QVector2D{ float(theViewport.width()/2.0), float(theViewport.height()/2.0) };
    }

    if (temporalAA) {
        const int t = 1 - 2 * (layer.tempAAPassIndex % 2);
        const float f = t * layer.temporalAAStrength;
        vertexOffsetsAA = { f / float(theViewport.width()/2.0), f / float(theViewport.height()/2.0) };
    }

    if (!renderedCameras.isEmpty()) {
        if (temporalAA || progressiveAA /*&& !vertexOffsetsAA.isNull()*/) {
            QMatrix4x4 offsetProjection = renderedCameras[0]->projection;
            QMatrix4x4 invProjection = renderedCameras[0]->projection.inverted();
            if (renderedCameras[0]->type == QSSGRenderCamera::Type::OrthographicCamera) {
                offsetProjection(0, 3) -= vertexOffsetsAA.x();
                offsetProjection(1, 3) -= vertexOffsetsAA.y();
            } else if (renderedCameras[0]->type == QSSGRenderCamera::Type::PerspectiveCamera) {
                offsetProjection(0, 2) += vertexOffsetsAA.x();
                offsetProjection(1, 2) += vertexOffsetsAA.y();
            }
            for (auto &modelContext : std::as_const(modelContexts)) {
                for (int mvpIdx = 0; mvpIdx < renderedCameras.count(); ++mvpIdx)
                    modelContext->modelViewProjections[mvpIdx] = offsetProjection * invProjection * modelContext->modelViewProjections[mvpIdx];
            }
        }
    }

    const bool hasItem2Ds = (renderableItem2DsCount > 0);
    const bool layerEnableDepthTest = layer.layerFlags.testFlag(QSSGRenderLayer::LayerFlag::EnableDepthTest);
    const bool layerEnabledDepthPrePass = layer.layerFlags.testFlag(QSSGRenderLayer::LayerFlag::EnableDepthPrePass);
    const bool depthTestEnableDefault = layerEnableDepthTest && (!opaqueObjects.isEmpty() || depthPrepassObjectsState || hasDepthWriteObjects);
    const bool zPrePassForced = (depthPrepassObjectsState != 0);
    zPrePassActive = zPrePassForced || (layerEnabledDepthPrePass && layerEnableDepthTest && (hasDepthWriteObjects || hasItem2Ds));
    const bool depthWriteEnableDefault = depthTestEnableDefault && (!layerEnabledDepthPrePass || !zPrePassActive);

    ps.flags.setFlag(QSSGRhiGraphicsPipelineState::Flag::DepthTestEnabled, depthTestEnableDefault);
    ps.flags.setFlag(QSSGRhiGraphicsPipelineState::Flag::DepthWriteEnabled, depthWriteEnableDefault);

    // Prepare passes
    QSSG_ASSERT(activePasses.isEmpty(), activePasses.clear());
    // If needed, generate a depth texture with the opaque objects. This
    // and the SSAO texture must come first since other passes may want to
    // expose these textures to their shaders.
    if (layerPrepResult.flags.requiresDepthTexture())
        activePasses.push_back(&depthMapPass);
    if (layerPrepResult.flags.requiresDepthTextureMS())
        activePasses.push_back(&depthMapPassMS);

    // Screen space ambient occlusion. Relies on the depth texture and generates an AO map.
    if (layerPrepResult.flags.requiresSsaoPass())
        activePasses.push_back(&ssaoMapPass);

    // Shadows. Generates a 2D or cube shadow map. (opaque + pre-pass transparent objects)
    if (layerPrepResult.flags.requiresShadowMapPass())
        activePasses.push_back(&shadowMapPass);

    if (zPrePassActive)
        activePasses.push_back(&zPrePassPass);

    // Screen texture with opaque objects.
    if (layerPrepResult.flags.requiresScreenTexture())
        activePasses.push_back(&screenMapPass);

    // Reflection pass
    activePasses.push_back(&reflectionMapPass);

    auto &underlayPass = userPasses[size_t(QSSGRenderLayer::RenderExtensionStage::Underlay)];
    if (underlayPass.hasData())
        activePasses.push_back(&underlayPass);

    const bool hasOpaqueObjects = (opaqueObjects.size() > 0);

    if (hasOpaqueObjects)
        activePasses.push_back(&opaquePass);

    // NOTE: When the a screen texture is used, the skybox pass will be called twice. First from
    // the screen texture pass and later as part of the normal run through the list.
    if (renderer->contextInterface()->rhiContext()->rhi()->isFeatureSupported(QRhi::TexelFetch)) {
        if (layer.background == QSSGRenderLayer::Background::SkyBoxCubeMap && layer.skyBoxCubeMap)
            activePasses.push_back(&skyboxCubeMapPass);
        else if (layer.background == QSSGRenderLayer::Background::SkyBox && layer.lightProbe)
            activePasses.push_back(&skyboxPass);
    }

    if (hasItem2Ds)
        activePasses.push_back(&item2DPass);

    if (layerPrepResult.flags.requiresScreenTexture())
        activePasses.push_back(&reflectionPass);

    // Note: Transparent pass includeds opaque objects when layerEnableDepthTest is false.
    if (transparentObjects.size() > 0 || (!layerEnableDepthTest && hasOpaqueObjects)) {
        if (orderIndependentTransparencyEnabled) {
            activePasses.push_back(&oitRenderPass);
            activePasses.push_back(&oitCompositePass);
            oitRenderPass.setMethod(layer.oitMethod);
            oitCompositePass.setMethod(layer.oitMethod);
        } else {
            activePasses.push_back(&transparentPass);
        }
    }

    auto &overlayPass = userPasses[size_t(QSSGRenderLayer::RenderExtensionStage::Overlay)];
    if (overlayPass.hasData())
        activePasses.push_back(&overlayPass);

    if (layer.gridEnabled)
        activePasses.push_back(&infiniteGridPass);

    if (const auto &dbgDrawSystem = renderer->contextInterface()->debugDrawSystem(); dbgDrawSystem && dbgDrawSystem->isEnabled())
        activePasses.push_back(&debugDrawPass);
}

template<typename T>
static void clearTable(std::vector<T> &entry)
{
    for (auto &e : entry)
        e.clear();
}

void QSSGLayerRenderData::resetForFrame()
{
    for (const auto &pass : activePasses)
        pass->resetForFrame();
    activePasses.clear();
    bakedLightingModels.clear();
    layerPrepResult = {};
    renderedCameras.clear();
    renderedCameraData.reset();
    renderedItem2Ds.clear();
    renderedBakedLightingModels.clear();
    renderableItem2Ds.clear();
    lightmapTextures.clear();
    bonemapTextures.clear();
    globalLights.clear();
    modelContexts.clear();
    features = QSSGShaderFeatures();
    hasDepthWriteObjects = false;
    depthPrepassObjectsState = { DepthPrepassObjectStateT(DepthPrepassObject::None) };
    zPrePassActive = false;

    clearTable(renderableModelStore);
    clearTable(modelContextStore);
    clearTable(renderableObjectStore);
    clearTable(opaqueObjectStore);
    clearTable(transparentObjectStore);
    clearTable(screenTextureObjectStore);
    clearTable(sortedOpaqueObjectCache);
    clearTable(sortedTransparentObjectCache);
    clearTable(sortedScreenTextureObjectCache);
    clearTable(sortedOpaqueDepthPrepassCache);
    clearTable(sortedDepthWriteCache);
}

QSSGLayerRenderPreparationResult::QSSGLayerRenderPreparationResult(const QRectF &inViewport, QSSGRenderLayer &inLayer)
    : layer(&inLayer)
{
    viewport = inViewport;
}

bool QSSGLayerRenderPreparationResult::isLayerVisible() const
{
    return viewport.height() >= 2.0f && viewport.width() >= 2.0f;
}

QSize QSSGLayerRenderPreparationResult::textureDimensions() const
{
    const auto size = viewport.size().toSize();
    return QSize(QSSGRendererUtil::nextMultipleOf4(size.width()), QSSGRendererUtil::nextMultipleOf4(size.height()));
}

QSSGCameraGlobalCalculationResult QSSGLayerRenderPreparationResult::setupCameraForRender(QSSGRenderCamera &inCamera, float dpr)
{
    // When using ssaa we need to zoom with the ssaa multiplier since otherwise the
    // orthographic camera will be zoomed out due to the bigger viewport. We therefore
    // scale the magnification before calulating the camera variables and then revert.
    // Since the same camera can be used in several View3Ds with or without ssaa we
    // cannot store the magnification permanently.
    const float horizontalMagnification = inCamera.horizontalMagnification;
    const float verticalMagnification = inCamera.verticalMagnification;
    inCamera.dpr = dpr;
    const float ssaaMultiplier = layer->isSsaaEnabled() ? layer->ssaaMultiplier : 1.0f;
    inCamera.horizontalMagnification *= ssaaMultiplier;
    inCamera.verticalMagnification *= ssaaMultiplier;
    const auto result = inCamera.calculateGlobalVariables(viewport);
    inCamera.horizontalMagnification = horizontalMagnification;
    inCamera.verticalMagnification = verticalMagnification;
    return result;
}

QSSGLayerRenderData::QSSGLayerRenderData(QSSGRenderLayer &inLayer, QSSGRenderer &inRenderer)
    : layer(inLayer)
    , renderer(&inRenderer)
    , orderIndependentTransparencyEnabled(false)
    , particlesEnabled(checkParticleSupport(inRenderer.contextInterface()->rhi()))
{
    depthMapPassMS.setMultisamplingEnabled(true);
    Q_ASSERT(extContexts.size() == 1);
}

QSSGLayerRenderData::~QSSGLayerRenderData()
{
    delete m_lightmapper;
    for (auto &pass : activePasses)
        pass->resetForFrame();

    for (auto &renderResult : renderResults)
        renderResult.reset();
    oitRenderContext.reset();
}

static void sortInstances(QByteArray &sortedData, QList<QSSGRhiSortData> &sortData, const void *instances,
                          int stride, int count, const QVector3D &cameraDirection)
{
    sortData.resize(count);
    Q_ASSERT(stride == sizeof(QSSGRenderInstanceTableEntry));
    // create sort data
    {
        const QSSGRenderInstanceTableEntry *instance = reinterpret_cast<const QSSGRenderInstanceTableEntry *>(instances);
        for (int i = 0; i < count; i++) {
            const QVector3D pos = QVector3D(instance->row0.w(), instance->row1.w(), instance->row2.w());
            sortData[i] = {QVector3D::dotProduct(pos, cameraDirection), i};
            instance++;
        }
    }

    // sort
    std::sort(sortData.begin(), sortData.end(), [](const QSSGRhiSortData &a, const QSSGRhiSortData &b){
        return a.d > b.d;
    });

    // copy instances
    {
        const QSSGRenderInstanceTableEntry *instance = reinterpret_cast<const QSSGRenderInstanceTableEntry *>(instances);
        QSSGRenderInstanceTableEntry *dest = reinterpret_cast<QSSGRenderInstanceTableEntry *>(sortedData.data());
        for (auto &s : sortData)
            *dest++ = instance[s.indexOrOffset];
    }
}

static void cullLodInstances(QByteArray &lodData, const void *instances, int count,
                             const QVector3D &cameraPosition, float minThreshold, float maxThreshold)
{
    const QSSGRenderInstanceTableEntry *instance = reinterpret_cast<const QSSGRenderInstanceTableEntry *>(instances);
    QSSGRenderInstanceTableEntry *dest = reinterpret_cast<QSSGRenderInstanceTableEntry *>(lodData.data());
    for (int i = 0; i < count; ++i) {
        const float x = cameraPosition.x() - instance->row0.w();
        const float y = cameraPosition.y() - instance->row1.w();
        const float z = cameraPosition.z() - instance->row2.w();
        const float distanceSq = x * x + y * y + z * z;
        if (distanceSq >= minThreshold * minThreshold && (maxThreshold < 0 || distanceSq < maxThreshold * maxThreshold))
            *dest = *instance;
        else
            *dest= {};
        dest++;
        instance++;
    }
}

bool QSSGLayerRenderData::prepareInstancing(QSSGRhiContext *rhiCtx,
                                            QSSGSubsetRenderable *renderable,
                                            const QVector3D &cameraDirection,
                                            const QVector3D &cameraPosition,
                                            float minThreshold,
                                            float maxThreshold)
{
    QSSGRhiContextPrivate *rhiCtxD = QSSGRhiContextPrivate::get(rhiCtx);
    auto &modelContext = renderable->modelContext;
    auto &instanceBuffer = renderable->instanceBuffer; // intentional ref2ptr
    if (!modelContext.model.instancing() || instanceBuffer)
        return instanceBuffer;
    auto *table = modelContext.model.instanceTable;
    bool usesLod = minThreshold >= 0 || maxThreshold >= 0;
    QSSGRhiInstanceBufferData &instanceData(usesLod ? rhiCtxD->instanceBufferData(&modelContext.model) : rhiCtxD->instanceBufferData(table));
    quint32 instanceBufferSize = table->dataSize();
    // Create or resize the instance buffer ### if (instanceData.owned)
    bool sortingChanged = table->isDepthSortingEnabled() != instanceData.sorting;
    bool cameraDirectionChanged = !qFuzzyCompare(instanceData.sortedCameraDirection, cameraDirection);
    bool cameraPositionChanged = !qFuzzyCompare(instanceData.cameraPosition, cameraPosition);
    bool updateInstanceBuffer = table->serial() != instanceData.serial || sortingChanged || (cameraDirectionChanged && table->isDepthSortingEnabled());
    bool updateForLod = cameraPositionChanged && usesLod;
    if (sortingChanged && !table->isDepthSortingEnabled()) {
        instanceData.sortedData.clear();
        instanceData.sortData.clear();
        instanceData.sortedCameraDirection = {};
    }
    instanceData.sorting = table->isDepthSortingEnabled();
    if (instanceData.buffer && instanceData.buffer->size() < instanceBufferSize) {
        updateInstanceBuffer = true;
        //                    qDebug() << "Resizing instance buffer";
        instanceData.buffer->setSize(instanceBufferSize);
        instanceData.buffer->create();
    }
    if (!instanceData.buffer) {
        //                    qDebug() << "Creating instance buffer";
        updateInstanceBuffer = true;
        instanceData.buffer = rhiCtx->rhi()->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::VertexBuffer, instanceBufferSize);
        instanceData.buffer->create();
    }
    if (updateInstanceBuffer || updateForLod) {
        const void *data = nullptr;
        if (table->isDepthSortingEnabled()) {
            if (updateInstanceBuffer) {
                QMatrix4x4 invGlobalTransform = modelContext.model.globalTransform.inverted();
                instanceData.sortedData.resize(table->dataSize());
                sortInstances(instanceData.sortedData,
                              instanceData.sortData,
                              table->constData(),
                              table->stride(),
                              table->count(),
                              invGlobalTransform.map(cameraDirection).normalized());
            }
            data = instanceData.sortedData.constData();
            instanceData.sortedCameraDirection = cameraDirection;
        } else {
            data = table->constData();
        }
        if (data) {
            if (updateForLod) {
                if (table->isDepthSortingEnabled()) {
                    instanceData.lodData.resize(table->dataSize());
                    cullLodInstances(instanceData.lodData, instanceData.sortedData.constData(), instanceData.sortedData.size(), cameraPosition, minThreshold, maxThreshold);
                    data = instanceData.lodData.constData();
                } else {
                    instanceData.lodData.resize(table->dataSize());
                    cullLodInstances(instanceData.lodData, table->constData(), table->count(), cameraPosition, minThreshold, maxThreshold);
                    data = instanceData.lodData.constData();
                }
            }
            QRhiResourceUpdateBatch *rub = rhiCtx->rhi()->nextResourceUpdateBatch();
            rub->updateDynamicBuffer(instanceData.buffer, 0, instanceBufferSize, data);
            rhiCtx->commandBuffer()->resourceUpdate(rub);
            //qDebug() << "****** UPDATING INST BUFFER. Size" << instanceBufferSize;
        } else {
            qWarning() << "NO DATA IN INSTANCE TABLE";
        }
        instanceData.serial = table->serial();
        instanceData.cameraPosition = cameraPosition;
    }
    instanceBuffer = instanceData.buffer;
    return instanceBuffer;
}

void QSSGLayerRenderData::maybeBakeLightmap()
{
    if (!interactiveLightmapBakingRequested) {
        static bool bakeRequested = false;
        static bool bakeFlagChecked = false;
        if (!bakeFlagChecked) {
            bakeFlagChecked = true;
            const bool cmdLineReq = QCoreApplication::arguments().contains(QStringLiteral("--bake-lightmaps"));
            const bool envReq = qEnvironmentVariableIntValue("QT_QUICK3D_BAKE_LIGHTMAPS");
            bakeRequested = cmdLineReq || envReq;
        }
        if (!bakeRequested)
            return;
    }

    const auto &sortedBakedLightingModels = getSortedBakedLightingModels(); // front to back

    QSSGRhiContext *rhiCtx = renderer->contextInterface()->rhiContext().get();

    if (!m_lightmapper)
        m_lightmapper = new QSSGLightmapper(rhiCtx, renderer);

    // sortedBakedLightingModels contains all models with
    // usedInBakedLighting: true. These, together with lights that
    // have a bakeMode set to either Indirect or All, form the
    // lightmapped scene. A lightmap is stored persistently only
    // for models that have their lightmapKey set.

    m_lightmapper->reset();
    m_lightmapper->setOptions(layer.lmOptions);
    m_lightmapper->setOutputCallback(lightmapBakingOutputCallback);

    for (int i = 0, ie = sortedBakedLightingModels.size(); i != ie; ++i)
        m_lightmapper->add(sortedBakedLightingModels[i]);

    QRhiCommandBuffer *cb = rhiCtx->commandBuffer();
    cb->debugMarkBegin("Quick3D lightmap baking");
    m_lightmapper->bake();
    cb->debugMarkEnd();

    if (!interactiveLightmapBakingRequested) {
        qDebug("Lightmap baking done, exiting application");
        QMetaObject::invokeMethod(qApp, "quit");
    }

    interactiveLightmapBakingRequested = false;
}

QSSGFrameData &QSSGLayerRenderData::getFrameData()
{
    return frameData;
}

QSSGRenderGraphObject *QSSGLayerRenderData::getCamera(QSSGCameraId id) const
{
    QSSGRenderGraphObject *ret = nullptr;
    if (auto res = reinterpret_cast<QSSGRenderGraphObject *>(id))
        ret = res;

    return ret;
}

QSSGRenderCameraData QSSGLayerRenderData::getCameraRenderData(const QSSGRenderCamera *camera_)
{
    if ((!camera_ || camera_ == renderedCameras[0]) && renderedCameraData.has_value())
        return renderedCameraData.value()[0];
    if (camera_)
        return getCameraDataImpl(camera_);
    return {};
}

QSSGRenderCameraData QSSGLayerRenderData::getCameraRenderData(const QSSGRenderCamera *camera_) const
{
    if ((!camera_ || camera_ == renderedCameras[0]) && renderedCameraData.has_value())
        return renderedCameraData.value()[0];
    if (camera_)
        return getCameraDataImpl(camera_);
    return {};
}

QSSGRenderContextInterface *QSSGLayerRenderData::contextInterface() const
{
    return renderer ? renderer->contextInterface() : nullptr;
}

QSSGLayerRenderData::GlobalRenderProperties QSSGLayerRenderData::globalRenderProperties(const QSSGRenderContextInterface &ctx)
{
    GlobalRenderProperties props {};
    if (const auto &rhiCtx = ctx.rhiContext(); rhiCtx->isValid()) {
        QRhi *rhi = rhiCtx->rhi();
        props.isYUpInFramebuffer = rhi->isYUpInFramebuffer();
        props.isYUpInNDC = rhi->isYUpInNDC();
        props.isClipDepthZeroToOne = rhi->isClipDepthZeroToOne();
    }

    return props;
}

const QSSGRenderShadowMapPtr &QSSGLayerRenderData::requestShadowMapManager()
{
    if (!shadowMapManager && QSSG_GUARD(renderer && renderer->contextInterface()))
        shadowMapManager.reset(new QSSGRenderShadowMap(*renderer->contextInterface()));
    return shadowMapManager;
}

const QSSGRenderReflectionMapPtr &QSSGLayerRenderData::requestReflectionMapManager()
{
    if (!reflectionMapManager && QSSG_GUARD(renderer && renderer->contextInterface()))
        reflectionMapManager.reset(new QSSGRenderReflectionMap(*renderer->contextInterface()));
    return reflectionMapManager;
}

QT_END_NAMESPACE
