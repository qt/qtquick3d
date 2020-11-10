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
#include <QtQuick3DRuntimeRender/private/qssgrenderlayer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendereffect_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderlight_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercamera_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderskeleton_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderjoint_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercontextcore_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderresourcemanager_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderbuffermanager_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrhicustommaterialsystem_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercache_p.h>
#include <QtQuick3DRuntimeRender/private/qssgperframeallocator_p.h>
#include <QtQuick3DUtils/private/qssgutils_p.h>
#include <QtQuick3DRuntimeRender/private/qssgruntimerenderlogging_p.h>

#ifdef Q_CC_MSVC
#pragma warning(disable : 4355)
#endif

QT_BEGIN_NAMESPACE

static void collectBoneTransforms(QSSGRenderNode *node, QSSGRenderModel *modelNode, const QVector<QMatrix4x4> &poses)
{
    QSSGRenderJoint *jointNode = static_cast<QSSGRenderJoint *>(node);
    jointNode->calculateGlobalVariables();
    QMatrix4x4 M = jointNode->globalTransform;
    // if user doesn't give the inverseBindPose, identity matrixes are used.
    if (poses.size() > jointNode->index)
        M *= poses[jointNode->index];
    modelNode->boneTransforms[jointNode->index] = M;
    QMatrix3x3 N = mat44::getUpper3x3(M);
    modelNode->boneNormalTransforms[jointNode->index] = mat33::getInverse(N).transposed();

    for (auto &child : node->children)
        collectBoneTransforms(&child, modelNode, poses);
}

template<typename T, typename V>
inline void collectNode(const V &node, QVector<T> &dst, int &dstPos)
{
    if (dstPos < dst.size())
        dst[dstPos] = node;
    else
        dst.append(node);
    ++dstPos;
}

static void maybeQueueNodeForRender(QSSGRenderNode &inNode,
                                    QVector<QSSGRenderableNodeEntry> &outRenderables,
                                    int &ioRenderableCount,
                                    QVector<QSSGRenderCamera *> &outCameras,
                                    int &ioCameraCount,
                                    QVector<QSSGRenderLight *> &outLights,
                                    int &ioLightCount,
                                    quint32 &ioDFSIndex)
{
    ++ioDFSIndex;
    inNode.dfsIndex = ioDFSIndex;
    if (inNode.isRenderableType()) {
        collectNode(QSSGRenderableNodeEntry(inNode), outRenderables, ioRenderableCount);
        if (inNode.type == QSSGRenderGraphObject::Type::Model) {
            auto modelNode = static_cast<QSSGRenderModel *>(&inNode);
            auto skeletonNode = modelNode->skeleton;
            if (skeletonNode && modelNode->skinningDirty) {
                skeletonNode->boneTransformsDirty = false;
                modelNode->skinningDirty = false;
                // For now, boneTransforms is a QVector<QMatrix4x4>
                // but it will be efficient to use QVector<float>
                // to pass it to the shader uniform buffer
                if (modelNode->boneTransforms.size() < skeletonNode->maxIndex + 1) {
                    modelNode->boneTransforms.resize(skeletonNode->maxIndex + 1);
                    modelNode->boneNormalTransforms.resize(skeletonNode->maxIndex + 1);
                }
                for (auto &child : skeletonNode->children)
                    collectBoneTransforms(&child, modelNode, modelNode->inverseBindPoses);
            }
        }
    } else if (inNode.type == QSSGRenderGraphObject::Type::Camera) {
        collectNode(static_cast<QSSGRenderCamera *>(&inNode), outCameras, ioCameraCount);
    } else if (inNode.type == QSSGRenderGraphObject::Type::Light) {
        collectNode(static_cast<QSSGRenderLight *>(&inNode), outLights, ioLightCount);
    }

    for (auto &theChild : inNode.children)
        maybeQueueNodeForRender(theChild, outRenderables, ioRenderableCount, outCameras, ioCameraCount, outLights, ioLightCount, ioDFSIndex);
}

static inline bool hasValidLightProbe(QSSGRenderImage *inLightProbeImage)
{
    return inLightProbeImage && inLightProbeImage->m_textureData.hasTexture();
}

QSSGDefaultMaterialPreparationResult::QSSGDefaultMaterialPreparationResult(QSSGShaderDefaultMaterialKey inKey)
    : firstImage(nullptr), opacity(1.0f), materialKey(inKey), dirty(false)
{
}

#define MAX_AA_LEVELS 8

QSSGLayerRenderPreparationData::QSSGLayerRenderPreparationData(QSSGRenderLayer &inLayer,
                                                                   const QSSGRef<QSSGRenderer> &inRenderer)
    : layer(inLayer)
    , renderer(inRenderer)
    , camera(nullptr)
    , featuresDirty(true)
    , featureSetHash(0)
    , tooManyLightsError(false)
{
}

QSSGLayerRenderPreparationData::~QSSGLayerRenderPreparationData()
{
    delete shadowMapManager;
}

void QSSGLayerRenderPreparationData::setShaderFeature(QSSGShaderDefines::Define inFeature, bool inValue)
{
    auto iter = features.cbegin();
    const auto end = features.cend();

    while (iter != end && iter->feature != inFeature)
        ++iter;

    if (iter != end) {
        if (iter->enabled != inValue) {
            iter->enabled = inValue;
            featuresDirty = true;
            featureSetHash = 0;
        }
    } else {
        features.append({ inFeature, inValue });
        featuresDirty = true;
        featureSetHash = 0;
    }
}

const ShaderFeatureSetList &QSSGLayerRenderPreparationData::getShaderFeatureSet()
{
    if (featuresDirty) {
        std::sort(features.begin(), features.end());
        featuresDirty = false;
    }
    return features;
}

size_t QSSGLayerRenderPreparationData::getShaderFeatureSetHash()
{
    if (!featureSetHash)
        featureSetHash = hashShaderFeatureSet(getShaderFeatureSet());
    return featureSetHash;
}

QVector3D QSSGLayerRenderPreparationData::getCameraDirection()
{
    if (!cameraDirection.hasValue()) {
        if (camera)
            cameraDirection = camera->getScalingCorrectDirection();
        else
            cameraDirection = QVector3D(0, 0, -1);
    }
    return *cameraDirection;
}

// Per-frame cache of renderable objects post-sort.
const QVector<QSSGRenderableObjectHandle> &QSSGLayerRenderPreparationData::getOpaqueRenderableObjects(bool performSort)
{
    if (!renderedOpaqueObjects.empty() || camera == nullptr)
        return renderedOpaqueObjects;
    if (layer.flags.testFlag(QSSGRenderLayer::Flag::LayerEnableDepthTest) && !opaqueObjects.empty()) {
        QVector3D theCameraDirection(getCameraDirection());
        QVector3D theCameraPosition = camera->getGlobalPos();
        renderedOpaqueObjects = opaqueObjects;
        // Setup the object's sorting information
        for (int idx = 0, end = renderedOpaqueObjects.size(); idx < end; ++idx) {
            QSSGRenderableObjectHandle &theInfo = renderedOpaqueObjects[idx];
            const QVector3D difference = theInfo.obj->worldCenterPoint - theCameraPosition;
            theInfo.cameraDistanceSq = QVector3D::dotProduct(difference, theCameraDirection);
        }

        static const auto isRenderObjectPtrLessThan = [](const QSSGRenderableObjectHandle &lhs, const QSSGRenderableObjectHandle &rhs) {
            return lhs.cameraDistanceSq < rhs.cameraDistanceSq;
        };
        // Render nearest to furthest objects
        if (performSort)
            std::sort(renderedOpaqueObjects.begin(), renderedOpaqueObjects.end(), isRenderObjectPtrLessThan);
    }
    return renderedOpaqueObjects;
}

// If layer depth test is false, this may also contain opaque objects.
const QVector<QSSGRenderableObjectHandle> &QSSGLayerRenderPreparationData::getTransparentRenderableObjects()
{
    if (!renderedTransparentObjects.empty() || camera == nullptr)
        return renderedTransparentObjects;

    renderedTransparentObjects = transparentObjects;

    if (!layer.flags.testFlag(QSSGRenderLayer::Flag::LayerEnableDepthTest))
        renderedTransparentObjects.append(opaqueObjects);

    if (!renderedTransparentObjects.empty()) {
        QVector3D theCameraDirection(getCameraDirection());
        QVector3D theCameraPosition = camera->getGlobalPos();

        // Setup the object's sorting information
        for (quint32 idx = 0, end = renderedTransparentObjects.size(); idx < end; ++idx) {
            QSSGRenderableObjectHandle &theInfo = renderedTransparentObjects[idx];
            const QVector3D difference = theInfo.obj->worldCenterPoint - theCameraPosition;
            theInfo.cameraDistanceSq = QVector3D::dotProduct(difference, theCameraDirection);
        }

        static const auto iSRenderObjectPtrGreatThan = [](const QSSGRenderableObjectHandle &lhs, const QSSGRenderableObjectHandle &rhs) {
            return lhs.cameraDistanceSq > rhs.cameraDistanceSq;
        };
        // render furthest to nearest.
        std::sort(renderedTransparentObjects.begin(), renderedTransparentObjects.end(), iSRenderObjectPtrGreatThan);
    }

    return renderedTransparentObjects;
}

const QVector<QSSGRenderableNodeEntry> &QSSGLayerRenderPreparationData::getRenderableItem2Ds()
{
    if (!renderedItem2Ds.isEmpty() || camera == nullptr)
        return renderedItem2Ds;

    renderedItem2Ds = renderableItem2Ds;

    if (!renderedItem2Ds.isEmpty()) {
        const QVector3D cameraDirection(getCameraDirection());
        const QVector3D cameraPosition = camera->getGlobalPos();

        const auto isItemNodeDistanceGreatThan = [cameraDirection, cameraPosition]
                (const QSSGRenderableNodeEntry &lhs, const QSSGRenderableNodeEntry &rhs) {
            if (!lhs.node->parent || !rhs.node->parent)
                return false;
            const QVector3D lhsDifference = lhs.node->parent->position - cameraPosition;
            const float lhsCameraDistanceSq = QVector3D::dotProduct(lhsDifference, cameraDirection);
            const QVector3D rhsDifference = rhs.node->parent->position - cameraPosition;
            const float rhsCameraDistanceSq = QVector3D::dotProduct(rhsDifference, cameraDirection);
            return lhsCameraDistanceSq > rhsCameraDistanceSq;
        };

        const auto isItemZOrderLessThan = []
                (const QSSGRenderableNodeEntry &lhs, const QSSGRenderableNodeEntry &rhs) {
            if (lhs.node->parent && rhs.node->parent && lhs.node->parent == rhs.node->parent) {
                // Same parent nodes, so sort with item z-ordering
                QSSGRenderItem2D *lhsItem = static_cast<QSSGRenderItem2D *>(lhs.node);
                QSSGRenderItem2D *rhsItem = static_cast<QSSGRenderItem2D *>(rhs.node);
                return lhsItem->zOrder < rhsItem->zOrder;
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

/**
 * Usage: T *ptr = RENDER_FRAME_NEW<T>(context, arg0, arg1, ...); is equivalent to: T *ptr = new T(arg0, arg1, ...);
 * so RENDER_FRAME_NEW() takes the RCI + T's arguments
 */
template <typename T, typename... Args>
inline T *RENDER_FRAME_NEW(const QSSGRef<QSSGRenderContextInterface> &ctx, const Args&... args)
{
    return new (ctx->perFrameAllocator().allocate(sizeof(T)))T(const_cast<Args &>(args)...);
}

QSSGShaderDefaultMaterialKey QSSGLayerRenderPreparationData::generateLightingKey(
        QSSGRenderDefaultMaterial::MaterialLighting inLightingType, const QSSGShaderLightList &lights, bool receivesShadows)
{
    const size_t features = getShaderFeatureSetHash();
    QSSGShaderDefaultMaterialKey theGeneratedKey(features);
    const bool lighting = inLightingType != QSSGRenderDefaultMaterial::MaterialLighting::NoLighting;
    renderer->defaultMaterialShaderKeyProperties().m_hasLighting.setValue(theGeneratedKey, lighting);
    if (lighting) {
        const bool lightProbe = layer.lightProbe && layer.lightProbe->m_textureData.hasTexture();
        renderer->defaultMaterialShaderKeyProperties().m_hasIbl.setValue(theGeneratedKey, lightProbe);

        quint32 numLights = quint32(lights.size());
        if (Q_UNLIKELY(numLights > QSSGShaderDefaultMaterialKeyProperties::LightCount && !tooManyLightsError)) {
            tooManyLightsError = true;
            numLights = QSSGShaderDefaultMaterialKeyProperties::LightCount;
            qCCritical(INVALID_OPERATION, "Too many lights on layer, max is %d", QSSGShaderDefaultMaterialKeyProperties::LightCount);
            Q_ASSERT(false);
        }
        renderer->defaultMaterialShaderKeyProperties().m_lightCount.setValue(theGeneratedKey, numLights);

        for (int lightIdx = 0, lightEnd = lights.size(); lightIdx < lightEnd; ++lightIdx) {
            QSSGRenderLight *theLight(lights[lightIdx].light);
            const bool isDirectional = theLight->m_lightType == QSSGRenderLight::Type::Directional;
            const bool isSpot = theLight->m_lightType == QSSGRenderLight::Type::Spot;
            const bool castShadowsArea = theLight->m_castShadow && receivesShadows;

            renderer->defaultMaterialShaderKeyProperties().m_lightFlags[lightIdx].setValue(theGeneratedKey, !isDirectional);
            renderer->defaultMaterialShaderKeyProperties().m_lightSpotFlags[lightIdx].setValue(theGeneratedKey, isSpot);
            renderer->defaultMaterialShaderKeyProperties().m_lightShadowFlags[lightIdx].setValue(theGeneratedKey, castShadowsArea);
        }
    }
    return theGeneratedKey;
}

void QSSGLayerRenderPreparationData::prepareImageForRender(QSSGRenderImage &inImage,
                                                           QSSGRenderableImage::Type inMapType,
                                                           QSSGRenderableImage *&ioFirstImage,
                                                           QSSGRenderableImage *&ioNextImage,
                                                           QSSGRenderableObjectFlags &ioFlags,
                                                           QSSGShaderDefaultMaterialKey &inShaderKey,
                                                           quint32 inImageIndex,
                                                           QSSGRenderDefaultMaterial *inMaterial)
{
    const QSSGRef<QSSGRenderContextInterface> &contextInterface(renderer->contextInterface());
    const QSSGRef<QSSGBufferManager> &bufferManager = contextInterface->bufferManager();

    if (inImage.clearDirty(bufferManager))
        ioFlags |= QSSGRenderableObjectFlag::Dirty;
    if (inImage.m_textureData.m_rhiTexture) {
        if (inImage.m_textureData.m_textureFlags.hasTransparency()
            && (inMapType == QSSGRenderableImage::Type::Diffuse || inMapType == QSSGRenderableImage::Type::Opacity
                || inMapType == QSSGRenderableImage::Type::Translucency)) {
            ioFlags |= QSSGRenderableObjectFlag::HasTransparency;
        }
        // Textures used in general have linear characteristics.
        // PKC -- The filters are properly set already.  Setting them here only overrides what
        // would
        // otherwise be a correct setting.
        // inImage.m_TextureData.m_Texture->SetMinFilter( QSSGRenderTextureMinifyingOp::Linear );
        // inImage.m_TextureData.m_Texture->SetMagFilter( QSSGRenderTextureMagnifyingOp::Linear );

        QSSGRenderableImage *theImage = RENDER_FRAME_NEW<QSSGRenderableImage>(renderer->contextInterface(), inMapType, inImage);
        QSSGShaderKeyImageMap &theKeyProp = renderer->defaultMaterialShaderKeyProperties().m_imageMaps[inImageIndex];

        theKeyProp.setEnabled(inShaderKey, true);
        switch (inImage.m_mappingMode) {
        default:
            Q_ASSERT(false);
            // fallthrough intentional
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
        switch (inImage.m_textureData.m_rhiTexture->format()) {
        case QRhiTexture::Format::RED_OR_ALPHA8:
            hasA = !renderer->contextInterface()->rhiContext()->rhi()->isFeatureSupported(QRhi::RedOrAlpha8IsRed);
            break;
        default:
            hasA = true;
            hasG = true;
            hasB = true;
            break;
        }

        if (inImage.m_textureData.m_textureFlags.isInvertUVCoords())
            theKeyProp.setInvertUVMap(inShaderKey, true);

        if (inImage.isImageTransformIdentity())
            theKeyProp.setIdentityTransform(inShaderKey, true);

        if (ioFirstImage == nullptr)
            ioFirstImage = theImage;
        else
            ioNextImage->m_nextImage = theImage;

        // assume offscreen renderer produces non-premultiplied image
        if (inImage.m_textureData.m_textureFlags.isPreMultiplied())
            theKeyProp.setPremultiplied(inShaderKey, true);

        ioNextImage = theImage;

        if (inMaterial && inImageIndex >= QSSGShaderDefaultMaterialKeyProperties::SingleChannelImagesFirst) {
            QSSGRenderDefaultMaterial::TextureChannelMapping value = QSSGRenderDefaultMaterial::R;
            QSSGRenderDefaultMaterial::TextureChannelMapping defaultValues[5] = {QSSGRenderDefaultMaterial::R, QSSGRenderDefaultMaterial::G, QSSGRenderDefaultMaterial::B, QSSGRenderDefaultMaterial::R, QSSGRenderDefaultMaterial::A};
            if (inMaterial->type == QSSGRenderGraphObject::Type::DefaultMaterial)
                defaultValues[1] = defaultValues[2] = QSSGRenderDefaultMaterial::R;

            const quint32 scIndex = inImageIndex - QSSGShaderDefaultMaterialKeyProperties::SingleChannelImagesFirst;
            QSSGShaderKeyTextureChannel &channelKey = renderer->defaultMaterialShaderKeyProperties().m_textureChannels[scIndex];
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
                value = defaultValues[scIndex];
            channelKey.setTextureChannel(QSSGShaderKeyTextureChannel::TexturChannelBits(value), inShaderKey);
        }
    }
}

void QSSGLayerRenderPreparationData::setVertexInputPresence(const QSSGRenderableObjectFlags &renderableFlags,
                                                            QSSGShaderDefaultMaterialKey &key,
                                                            QSSGRenderer *renderer)
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
    if (renderableFlags.hasAttributeTangent())
        vertexAttribs |= QSSGShaderKeyVertexAttribute::Tangent;
    if (renderableFlags.hasAttributeBinormal())
        vertexAttribs |= QSSGShaderKeyVertexAttribute::Binormal;
    if (renderableFlags.hasAttributeColor())
        vertexAttribs |= QSSGShaderKeyVertexAttribute::Color;
    if (renderableFlags.hasAttributeJointAndWeight())
        vertexAttribs |= QSSGShaderKeyVertexAttribute::JointAndWeight;
    renderer->defaultMaterialShaderKeyProperties().m_vertexAttributes.setValue(key, vertexAttribs);
}

QSSGDefaultMaterialPreparationResult QSSGLayerRenderPreparationData::prepareDefaultMaterialForRender(
        QSSGRenderDefaultMaterial &inMaterial,
        QSSGRenderableObjectFlags &inExistingFlags,
        float inOpacity,
        const QSSGShaderLightList &lights)
{
    QSSGRenderDefaultMaterial *theMaterial = &inMaterial;
    QSSGDefaultMaterialPreparationResult retval(generateLightingKey(theMaterial->lighting, lights, inExistingFlags.receivesShadows()));
    retval.renderableFlags = inExistingFlags;
    QSSGRenderableObjectFlags &renderableFlags(retval.renderableFlags);
    QSSGShaderDefaultMaterialKey &theGeneratedKey(retval.materialKey);
    retval.opacity = inOpacity;
    float &subsetOpacity(retval.opacity);

    if (theMaterial->dirty.isDirty()) {
        renderableFlags |= QSSGRenderableObjectFlag::Dirty;
    }
    subsetOpacity *= theMaterial->opacity;

    QSSGRenderableImage *firstImage = nullptr;

    // isDoubleSided
    renderer->defaultMaterialShaderKeyProperties().m_isDoubleSided.setValue(theGeneratedKey, theMaterial->cullMode == QSSGCullFaceMode::Disabled);

    // default materials never define their on position
    renderer->defaultMaterialShaderKeyProperties().m_overridesPosition.setValue(theGeneratedKey, false);

    // default materials dont make use of raw projection or inverse projection matrices
    renderer->defaultMaterialShaderKeyProperties().m_usesProjectionMatrix.setValue(theGeneratedKey, false);
    renderer->defaultMaterialShaderKeyProperties().m_usesInverseProjectionMatrix.setValue(theGeneratedKey, false);
    // nor they do rely on VAR_COLOR
    renderer->defaultMaterialShaderKeyProperties().m_usesVarColor.setValue(theGeneratedKey, false);

    // alpha Mode
    renderer->defaultMaterialShaderKeyProperties().m_alphaMode.setValue(theGeneratedKey, theMaterial->alphaMode);

    // vertex attribute presence flags
    setVertexInputPresence(renderableFlags, theGeneratedKey, renderer.data());

    // set the flag indicating the need for gl_PointSize
    renderer->defaultMaterialShaderKeyProperties().m_usesPointsTopology.setValue(theGeneratedKey, renderableFlags.isPointsTopology());

//    if (theMaterial->iblProbe && checkLightProbeDirty(*theMaterial->iblProbe)) {
//        renderer->prepareImageForIbl(*theMaterial->iblProbe);
//    }

    if (!renderer->defaultMaterialShaderKeyProperties().m_hasIbl.getValue(theGeneratedKey)) {
        bool lightProbeValid = hasValidLightProbe(theMaterial->iblProbe);
        setShaderFeature(QSSGShaderDefines::LightProbe, lightProbeValid);
        renderer->defaultMaterialShaderKeyProperties().m_hasIbl.setValue(theGeneratedKey, lightProbeValid);
        // setShaderFeature(ShaderFeatureDefines::enableIblFov(),
        // m_Renderer.GetLayerRenderData()->m_Layer.m_ProbeFov < 180.0f );
    }

    if (subsetOpacity >= QSSG_RENDER_MINIMUM_RENDER_OPACITY) {

        if (theMaterial->blendMode != QSSGRenderDefaultMaterial::MaterialBlendMode::SourceOver ||
            theMaterial->opacityMap ||
            theMaterial->alphaMode == QSSGRenderDefaultMaterial::Blend ||
            theMaterial->alphaMode == QSSGRenderDefaultMaterial::Mask) {
            renderableFlags |= QSSGRenderableObjectFlag::HasTransparency;
        }

        const bool specularEnabled = theMaterial->isSpecularEnabled();
        const bool metalnessEnabled = theMaterial->isMetalnessEnabled();
        renderer->defaultMaterialShaderKeyProperties().m_specularEnabled.setValue(theGeneratedKey, (specularEnabled || metalnessEnabled));
        if (specularEnabled || metalnessEnabled)
            renderer->defaultMaterialShaderKeyProperties().m_specularModel.setSpecularModel(theGeneratedKey, theMaterial->specularModel);

        renderer->defaultMaterialShaderKeyProperties().m_fresnelEnabled.setValue(theGeneratedKey, theMaterial->isFresnelEnabled());

        renderer->defaultMaterialShaderKeyProperties().m_vertexColorsEnabled.setValue(theGeneratedKey,
                                                                                      theMaterial->isVertexColorsEnabled());

        // Run through the material's images and prepare them for render.
        // this may in fact set pickable on the renderable flags if one of the images
        // links to a sub presentation or any offscreen rendered object.
        QSSGRenderableImage *nextImage = nullptr;
#define CHECK_IMAGE_AND_PREPARE(img, imgtype, shadercomponent)                          \
    if ((img))                                                                          \
        prepareImageForRender(*(img), imgtype, firstImage, nextImage, renderableFlags,  \
                              theGeneratedKey, shadercomponent, &inMaterial)

        if (theMaterial->type == QSSGRenderGraphObject::Type::PrincipledMaterial) {
            CHECK_IMAGE_AND_PREPARE(theMaterial->colorMap,
                                    QSSGRenderableImage::Type::BaseColor,
                                    QSSGShaderDefaultMaterialKeyProperties::BaseColorMap);
            CHECK_IMAGE_AND_PREPARE(theMaterial->metalnessMap,
                                    QSSGRenderableImage::Type::Metalness,
                                    QSSGShaderDefaultMaterialKeyProperties::MetalnessMap);
            CHECK_IMAGE_AND_PREPARE(theMaterial->occlusionMap,
                                    QSSGRenderableImage::Type::Occlusion,
                                    QSSGShaderDefaultMaterialKeyProperties::OcclusionMap);
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
        CHECK_IMAGE_AND_PREPARE(theMaterial->lightmaps.m_lightmapIndirect,
                                QSSGRenderableImage::Type::LightmapIndirect,
                                QSSGShaderDefaultMaterialKeyProperties::LightmapIndirect);
        CHECK_IMAGE_AND_PREPARE(theMaterial->lightmaps.m_lightmapRadiosity,
                                QSSGRenderableImage::Type::LightmapRadiosity,
                                QSSGShaderDefaultMaterialKeyProperties::LightmapRadiosity);
        CHECK_IMAGE_AND_PREPARE(theMaterial->lightmaps.m_lightmapShadow,
                                QSSGRenderableImage::Type::LightmapShadow,
                                QSSGShaderDefaultMaterialKeyProperties::LightmapShadow);
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

    retval.firstImage = firstImage;
    if (retval.renderableFlags.isDirty())
        retval.dirty = true;
    if (retval.dirty)
        renderer->addMaterialDirtyClear(&inMaterial);
    return retval;
}

QSSGDefaultMaterialPreparationResult QSSGLayerRenderPreparationData::prepareCustomMaterialForRender(
        QSSGRenderCustomMaterial &inMaterial, QSSGRenderableObjectFlags &inExistingFlags,
        float inOpacity, bool alreadyDirty, const QSSGShaderLightList &lights,
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

    // isDoubleSided
    renderer->defaultMaterialShaderKeyProperties().m_isDoubleSided.setValue(theGeneratedKey,
                                                                            inMaterial.m_cullMode == QSSGCullFaceMode::Disabled);

    // Does the material override the position output
    const bool overridesPosition = inMaterial.m_renderFlags.testFlag(QSSGRenderCustomMaterial::RenderFlag::OverridesPosition);
    renderer->defaultMaterialShaderKeyProperties().m_overridesPosition.setValue(theGeneratedKey, overridesPosition);

    // Optional usage of PROJECTION_MATRIX and/or INVERSE_PROJECTION_MATRIX
    const bool usesProjectionMatrix = inMaterial.m_renderFlags.testFlag(QSSGRenderCustomMaterial::RenderFlag::ProjectionMatrix);
    renderer->defaultMaterialShaderKeyProperties().m_usesProjectionMatrix.setValue(theGeneratedKey, usesProjectionMatrix);
    const bool usesInvProjectionMatrix = inMaterial.m_renderFlags.testFlag(QSSGRenderCustomMaterial::RenderFlag::InverseProjectionMatrix);
    renderer->defaultMaterialShaderKeyProperties().m_usesInverseProjectionMatrix.setValue(theGeneratedKey, usesInvProjectionMatrix);

    // vertex attribute presence flags
    setVertexInputPresence(renderableFlags, theGeneratedKey, renderer.data());

    // set the flag indicating the need for gl_PointSize
    renderer->defaultMaterialShaderKeyProperties().m_usesPointsTopology.setValue(theGeneratedKey, renderableFlags.isPointsTopology());

    // Knowing whether VAR_COLOR is used becomes relevant when there is no
    // custom vertex shader, but VAR_COLOR is present in the custom fragment
    // snippet, because that case needs special care.
    const bool usesVarColor = inMaterial.m_renderFlags.testFlag(QSSGRenderCustomMaterial::RenderFlag::VarColor);
    renderer->defaultMaterialShaderKeyProperties().m_usesVarColor.setValue(theGeneratedKey, usesVarColor);

    if (inMaterial.m_renderFlags.testFlag(QSSGRenderCustomMaterial::RenderFlag::Blending))
        renderableFlags |= QSSGRenderableObjectFlag::HasTransparency;

    if (inMaterial.m_renderFlags.testFlag(QSSGRenderCustomMaterial::RenderFlag::ScreenTexture)) {
        ioFlags.setRequiresScreenTexture(true);
        renderableFlags |= QSSGRenderableObjectFlag::HasTransparency;
    }

    if (inMaterial.m_renderFlags.testFlag(QSSGRenderCustomMaterial::RenderFlag::ScreenMipTexture)) {
        ioFlags.setRequiresScreenTexture(true);
        ioFlags.setRequiresMipmapsForScreenTexture(true);
        renderableFlags |= QSSGRenderableObjectFlag::HasTransparency;
    }

    if (inMaterial.m_renderFlags.testFlag(QSSGRenderCustomMaterial::RenderFlag::DepthTexture))
        ioFlags.setRequiresDepthTexture(true);

    if (inMaterial.m_renderFlags.testFlag(QSSGRenderCustomMaterial::RenderFlag::AoTexture)) {
        ioFlags.setRequiresDepthTexture(true);
        ioFlags.setRequiresSsaoPass(true);
    }

    QSSGRenderableImage *firstImage = nullptr;
    QSSGRenderableImage *nextImage = nullptr;

#define CHECK_IMAGE_AND_PREPARE(img, imgtype, shadercomponent)                          \
    if ((img))                                                                          \
        prepareImageForRender(*(img), imgtype, firstImage, nextImage, renderableFlags,  \
                              theGeneratedKey, shadercomponent, nullptr)

    CHECK_IMAGE_AND_PREPARE(inMaterial.m_lightmaps.m_lightmapIndirect,
                            QSSGRenderableImage::Type::LightmapIndirect,
                            QSSGShaderDefaultMaterialKeyProperties::LightmapIndirect);
    CHECK_IMAGE_AND_PREPARE(inMaterial.m_lightmaps.m_lightmapRadiosity,
                            QSSGRenderableImage::Type::LightmapRadiosity,
                            QSSGShaderDefaultMaterialKeyProperties::LightmapRadiosity);
    CHECK_IMAGE_AND_PREPARE(inMaterial.m_lightmaps.m_lightmapShadow,
                            QSSGRenderableImage::Type::LightmapShadow,
                            QSSGShaderDefaultMaterialKeyProperties::LightmapShadow);
#undef CHECK_IMAGE_AND_PREPARE

    retval.firstImage = firstImage;
    if (retval.dirty || alreadyDirty)
        renderer->addMaterialDirtyClear(&inMaterial);
    return retval;
}

// inModel is const to emphasize the fact that its members cannot be written
// here: in case there is a scene shared between multiple View3Ds in different
// QQuickWindows, each window may run this in their own render thread, while
// inModel is the same.
bool QSSGLayerRenderPreparationData::prepareModelForRender(const QSSGRenderModel &inModel,
                                                           const QMatrix4x4 &inViewProjection,
                                                           const QSSGOption<QSSGClippingFrustum> &inClipFrustum,
                                                           QSSGShaderLightList &lights,
                                                           QSSGLayerRenderPreparationResultFlags &ioFlags)
{
    const QSSGRef<QSSGRenderContextInterface> &contextInterface(renderer->contextInterface());
    const QSSGRef<QSSGBufferManager> &bufferManager = contextInterface->bufferManager();

    // Up to the BufferManager to employ the appropriate caching mechanisms, so
    // loadMesh() is expected to be fast if already loaded. Note that preparing
    // the same QSSGRenderModel in different QQuickWindows (possible when a
    // scene is shared between View3Ds where the View3Ds belong to different
    // windows) leads to a different QSSGRenderMesh since the BufferManager is,
    // very correctly, per window, and so per scenegraph render thread.

    QSSGRenderMesh *theMesh = bufferManager->loadMesh(&inModel);

    if (theMesh == nullptr)
        return false;

    QSSGModelContext &theModelContext = *RENDER_FRAME_NEW<QSSGModelContext>(renderer->contextInterface(), inModel, inViewProjection);
    modelContexts.push_back(&theModelContext);

    bool subsetDirty = false;

    // Completely transparent models cannot be pickable.  But models with completely
    // transparent materials still are.  This allows the artist to control pickability
    // in a somewhat fine-grained style.
    const bool canModelBePickable = (inModel.globalOpacity > QSSG_RENDER_MINIMUM_RENDER_OPACITY)
                                    && (theModelContext.model.flags.testFlag(QSSGRenderModel::Flag::GloballyPickable));
    if (canModelBePickable) {
        // Check if there is BVH data, if not generate it
        if (!theMesh->bvh && !inModel.meshPath.isNull()) {
            theMesh->bvh = bufferManager->loadMeshBVH(inModel.meshPath);
            if (theMesh->bvh) {
                for (int i = 0; i < theMesh->bvh->roots.count(); ++i)
                    theMesh->subsets[i].bvhRoot = theMesh->bvh->roots.at(i);
            }
        }
    }

    // many renderableFlags are the same for all the subsets
    QSSGRenderableObjectFlags renderableFlagsForModel;
    if (theMesh->subsets.size() > 0) {
        QSSGRenderSubset &theSubset = theMesh->subsets[0];

        renderableFlagsForModel.setPickable(canModelBePickable);
        renderableFlagsForModel.setCastsShadows(inModel.castsShadows);
        renderableFlagsForModel.setReceivesShadows(inModel.receivesShadows);

        // With the RHI we need to be able to tell the material shader
        // generator to not generate vertex input attributes that are not
        // provided by the mesh. (because unlike OpenGL, other graphics
        // APIs may treat unbound vertex inputs as a fatal error)
        bool hasJoint = false;
        bool hasWeight = false;
        for (const QSSGRhiInputAssemblerState::InputSemantic &sem : qAsConst(theSubset.rhi.ia.inputs)) {
            if (sem == QSSGRhiInputAssemblerState::PositionSemantic)
                renderableFlagsForModel.setHasAttributePosition(true);
            else if (sem == QSSGRhiInputAssemblerState::NormalSemantic)
                renderableFlagsForModel.setHasAttributeNormal(true);
            else if (sem == QSSGRhiInputAssemblerState::TexCoord0Semantic)
                renderableFlagsForModel.setHasAttributeTexCoord0(true);
            else if (sem == QSSGRhiInputAssemblerState::TexCoord1Semantic)
                renderableFlagsForModel.setHasAttributeTexCoord1(true);
            else if (sem == QSSGRhiInputAssemblerState::TangentSemantic)
                renderableFlagsForModel.setHasAttributeTangent(true);
            else if (sem == QSSGRhiInputAssemblerState::BinormalSemantic)
                renderableFlagsForModel.setHasAttributeBinormal(true);
            else if (sem == QSSGRhiInputAssemblerState::ColorSemantic)
                renderableFlagsForModel.setHasAttributeColor(true);
            // For skinning, we will set the HasAttribute only
            // if the mesh has both joint and weight
            else if (sem == QSSGRhiInputAssemblerState::JointSemantic)
                hasJoint = true;
            else if (sem == QSSGRhiInputAssemblerState::WeightSemantic)
                hasWeight = true;
        }
        renderableFlagsForModel.setHasAttributeJointAndWeight(hasJoint && hasWeight);
    }

    QSSGDataView<QMatrix4x4> boneGlobals;
    QSSGDataView<QMatrix3x3> boneNormals;
    const auto &rhiCtx = renderer->contextInterface()->rhiContext();
    // Skeletal Animation passes it's boneId as unsigned integers
    if (inModel.skeleton) {
        boneGlobals = toDataView(inModel.boneTransforms);
        boneNormals = toDataView(inModel.boneNormalTransforms);
    }

    for (int idx = 0; idx < theMesh->subsets.size(); ++idx) {
        // If the materials list < size of subsets, then use the last material for the rest
        QSSGRenderGraphObject *theSourceMaterialObject = nullptr;
        if (inModel.materials.isEmpty())
            break;
        if (idx + 1 > inModel.materials.count())
            theSourceMaterialObject = inModel.materials.last();
        else
            theSourceMaterialObject = inModel.materials.at(idx);

        QSSGRenderSubset &theOuterSubset(theMesh->subsets[idx]);
        {
            QSSGRenderSubset &theSubset = theOuterSubset;
            QSSGRenderableObjectFlags renderableFlags = renderableFlagsForModel;
            float subsetOpacity = inModel.globalOpacity;
            QVector3D theModelCenter(theSubset.bounds.center());
            theModelCenter = mat44::transform(inModel.globalTransform, theModelCenter);

            if (subsetOpacity >= QSSG_RENDER_MINIMUM_RENDER_OPACITY && inClipFrustum.hasValue()) {
                // Check bounding box against the clipping planes
                QSSGBounds3 theGlobalBounds = theSubset.bounds;
                theGlobalBounds.transform(theModelContext.model.globalTransform);
                if (!inClipFrustum->intersectsWith(theGlobalBounds))
                    subsetOpacity = 0.0f;
            }

            renderableFlags.setPointsTopology(theSubset.rhi.ia.topology == QRhiGraphicsPipeline::Points);
            QSSGRenderableObject *theRenderableObject = nullptr;
            QSSGRenderGraphObject *theMaterialObject = theSourceMaterialObject;

            if (theMaterialObject == nullptr)
                continue;

            if (theMaterialObject->type == QSSGRenderGraphObject::Type::DefaultMaterial || theMaterialObject->type == QSSGRenderGraphObject::Type::PrincipledMaterial) {
                QSSGRenderDefaultMaterial &theMaterial(static_cast<QSSGRenderDefaultMaterial &>(*theMaterialObject));
                // vertexColor should be supported in both DefaultMaterial and PrincipleMaterial
                // if the mesh has it.
                theMaterial.vertexColorsEnabled = renderableFlags.hasAttributeColor();
                QSSGDefaultMaterialPreparationResult theMaterialPrepResult(
                        prepareDefaultMaterialForRender(theMaterial, renderableFlags, subsetOpacity, lights));
                QSSGShaderDefaultMaterialKey &theGeneratedKey(theMaterialPrepResult.materialKey);
                subsetOpacity = theMaterialPrepResult.opacity;
                QSSGRenderableImage *firstImage(theMaterialPrepResult.firstImage);
                subsetDirty |= theMaterialPrepResult.dirty;
                renderableFlags = theMaterialPrepResult.renderableFlags;

                // Skin
                renderer->defaultMaterialShaderKeyProperties().m_boneCount.setValue(theGeneratedKey, boneGlobals.mSize);
                renderer->defaultMaterialShaderKeyProperties().m_usesFloatJointIndices.setValue(
                        theGeneratedKey, !rhiCtx->rhi()->isFeatureSupported(QRhi::IntAttributes));

                theRenderableObject = RENDER_FRAME_NEW<QSSGSubsetRenderable>(renderer->contextInterface(),
                                                                             renderableFlags,
                                                                             theModelCenter,
                                                                             renderer,
                                                                             theSubset,
                                                                             theMaterial,
                                                                             theModelContext,
                                                                             subsetOpacity,
                                                                             firstImage,
                                                                             theGeneratedKey,
                                                                             boneGlobals,
                                                                             boneNormals,
                                                                             lights);
                subsetDirty = subsetDirty || renderableFlags.isDirty();
            } else if (theMaterialObject->type == QSSGRenderGraphObject::Type::CustomMaterial) {
                QSSGRenderCustomMaterial &theMaterial(static_cast<QSSGRenderCustomMaterial &>(*theMaterialObject));

                const QSSGRef<QSSGCustomMaterialSystem> &theMaterialSystem(contextInterface->customMaterialSystem());
                subsetDirty |= theMaterialSystem->prepareForRender(theModelContext.model, theSubset, theMaterial);

                QSSGDefaultMaterialPreparationResult theMaterialPrepResult(
                        prepareCustomMaterialForRender(theMaterial, renderableFlags, subsetOpacity, subsetDirty,
                                                       lights, ioFlags));
                QSSGShaderDefaultMaterialKey &theGeneratedKey(theMaterialPrepResult.materialKey);
                subsetOpacity = theMaterialPrepResult.opacity;
                QSSGRenderableImage *firstImage(theMaterialPrepResult.firstImage);
                renderableFlags = theMaterialPrepResult.renderableFlags;

                // Skin
                renderer->defaultMaterialShaderKeyProperties().m_boneCount.setValue(theGeneratedKey, boneGlobals.mSize);
                renderer->defaultMaterialShaderKeyProperties().m_usesFloatJointIndices.setValue(
                        theGeneratedKey, !rhiCtx->rhi()->isFeatureSupported(QRhi::IntAttributes));

                if (theMaterial.m_iblProbe)
                    checkLightProbeDirty(*theMaterial.m_iblProbe);

                theRenderableObject = RENDER_FRAME_NEW<QSSGCustomMaterialRenderable>(renderer->contextInterface(),
                                                                                     renderableFlags,
                                                                                     theModelCenter,
                                                                                     renderer,
                                                                                     theSubset,
                                                                                     theMaterial,
                                                                                     theModelContext,
                                                                                     subsetOpacity,
                                                                                     firstImage,
                                                                                     theGeneratedKey,
                                                                                     boneGlobals,
                                                                                     boneNormals,
                                                                                     lights);
            }
            if (theRenderableObject) {

                if (theRenderableObject->renderableFlags.hasTransparency())
                    transparentObjects.push_back(QSSGRenderableObjectHandle::create(theRenderableObject));
                else
                    opaqueObjects.push_back(QSSGRenderableObjectHandle::create(theRenderableObject));
            }
        }
    }

    // Now is the time to kick off the vertex/index buffer updates for all the
    // new meshes (and their submeshes). This here is the last possible place
    // to kick this off because the rest of the rendering pipeline will only
    // see the individual sub-objects as "renderable objects".
    bufferManager->commitBufferResourceUpdates();

    return subsetDirty;
}

bool QSSGLayerRenderPreparationData::prepareRenderablesForRender(const QMatrix4x4 &inViewProjection,
                                                                   const QSSGOption<QSSGClippingFrustum> &inClipFrustum,
                                                                   QSSGLayerRenderPreparationResultFlags &ioFlags)
{
    viewProjection = inViewProjection;
    bool wasDataDirty = false;
    QSSGRhiContext *rhiCtx = renderer->contextInterface()->rhiContext().data();
    for (qint32 idx = 0, end = renderableNodes.size(); idx < end; ++idx) {
        QSSGRenderableNodeEntry &theNodeEntry(renderableNodes[idx]);
        QSSGRenderNode *theNode = theNodeEntry.node;
        wasDataDirty = wasDataDirty || theNode->flags.testFlag(QSSGRenderNode::Flag::Dirty);
        switch (theNode->type) {
        case QSSGRenderGraphObject::Type::Model: {
            QSSGRenderModel *theModel = static_cast<QSSGRenderModel *>(theNode);
            theModel->calculateGlobalVariables();
            if (theModel->flags.testFlag(QSSGRenderModel::Flag::GloballyActive)) {
                bool wasModelDirty = prepareModelForRender(*theModel, inViewProjection, inClipFrustum, theNodeEntry.lights, ioFlags);
                wasDataDirty = wasDataDirty || wasModelDirty;
            }
        } break;
        case QSSGRenderGraphObject::Type::Item2D: {
            QSSGRenderItem2D *theItem2D = static_cast<QSSGRenderItem2D *>(theNode);
            theItem2D->calculateGlobalVariables();
            if (theItem2D->flags.testFlag(QSSGRenderModel::Flag::GloballyActive)) {
                theItem2D->MVP = inViewProjection * theItem2D->globalTransform;
                static const QMatrix4x4 flipMatrix(1.0f, 0.0f, 0.0f, 0.0f,
                                                   0.0f, -1.0f, 0.0f, 0.0f,
                                                   0.0f, 0.0f, 1.0f, 0.0f,
                                                   0.0f, 0.0f, 0.0f, 1.0f);
                if (rhiCtx->isValid())
                    theItem2D->MVP = rhiCtx->rhi()->clipSpaceCorrMatrix() * theItem2D->MVP * flipMatrix;
                // Pushing front to keep item order inside QML file
                renderableItem2Ds.push_front(theNodeEntry);
            }
        } break;
        default:
            Q_ASSERT(false);
            break;
        }
    }
    return wasDataDirty;
}

bool QSSGLayerRenderPreparationData::checkLightProbeDirty(QSSGRenderImage &inLightProbe)
{
    const QSSGRef<QSSGRenderContextInterface> &theContext(renderer->contextInterface());
    const QSSGRef<QSSGBufferManager> &bufferManager = theContext->bufferManager();
    return inLightProbe.clearDirty(bufferManager, true);
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

void QSSGLayerRenderPreparationData::prepareForRender(const QSize &inViewportDimensions)
{
    if (layerPrepResult.hasValue())
        return;

    features.clear();
    featureSetHash = 0;
    QVector2D thePresentationDimensions((float)inViewportDimensions.width(), (float)inViewportDimensions.height());
    QRect theViewport(renderer->contextInterface()->viewport());
    QRect theScissor(renderer->contextInterface()->scissorRect());
    if (theScissor.isNull() || (theScissor == theViewport))
        theScissor = theViewport;

    bool wasDirty = false;
    bool wasDataDirty = false;
    wasDirty = layer.flags.testFlag(QSSGRenderLayer::Flag::Dirty);
    // The first pass is just to render the data.
    quint32 maxNumAAPasses = layer.antialiasingMode == QSSGRenderLayer::AAMode::NoAA ? (quint32)0 : (quint32)(layer.antialiasingQuality) + 1;
    maxNumAAPasses = qMin((quint32)(MAX_AA_LEVELS + 1), maxNumAAPasses);
    QSSGRenderEffect *theLastEffect = nullptr;
    // Uncomment the line below to disable all progressive AA.
    // maxNumAAPasses = 0;

    QSSGLayerRenderPreparationResult thePrepResult;

    bool SSAOEnabled = (layer.aoStrength > 0.0f && layer.aoDistance > 0.0f);
    bool requiresDepthTexture = SSAOEnabled;
    setShaderFeature(QSSGShaderDefines::Ssm, false); // by default no shadow map generation

    if (layer.flags.testFlag(QSSGRenderLayer::Flag::Active)) {
        // Get the layer's width and height.
        for (QSSGRenderEffect *theEffect = layer.firstEffect; theEffect; theEffect = theEffect->m_nextEffect) {
            if (theEffect->flags.testFlag(QSSGRenderEffect::Flag::Dirty)) {
                wasDirty = true;
                theEffect->flags.setFlag(QSSGRenderEffect::Flag::Dirty, false);
            }
            if (theEffect->flags.testFlag(QSSGRenderEffect::Flag::Active)) {
                theLastEffect = theEffect;
                if (theEffect->requiresDepthTexture)
                    requiresDepthTexture = true;
            }
        }
        if (layer.flags.testFlag(QSSGRenderLayer::Flag::Dirty)) {
            wasDirty = true;
            layer.calculateGlobalVariables();
        }

        thePrepResult = QSSGLayerRenderPreparationResult(
                QSSGLayerRenderHelper(theViewport,
                                        theScissor,
                                        layer));

        thePrepResult.lastEffect = theLastEffect;
        thePrepResult.maxAAPassIndex = maxNumAAPasses;

        // these may still get modified due to custom materials below
        thePrepResult.flags.setRequiresDepthTexture(requiresDepthTexture);
        thePrepResult.flags.setRequiresSsaoPass(SSAOEnabled);

        if (thePrepResult.isLayerVisible()) {
            if (layer.lightProbe) {
                if (layer.lightProbe->m_format == QSSGRenderTextureFormat::Unknown) {
                    // Choose on a format that makes sense for a light probe
                    // At this point it's just a suggestion
                    if (renderer->contextInterface()->rhiContext()->rhi()->isTextureFormatSupported(QRhiTexture::RGBA16F))
                        layer.lightProbe->m_format = QSSGRenderTextureFormat::RGBA16F;
                    else
                        layer.lightProbe->m_format = QSSGRenderTextureFormat::RGBE8;
                }

                if (checkLightProbeDirty(*layer.lightProbe))
                    wasDataDirty = true;
            }

            bool lightProbeValid = hasValidLightProbe(layer.lightProbe);

            setShaderFeature(QSSGShaderDefines::LightProbe, lightProbeValid);
            setShaderFeature(QSSGShaderDefines::IblOrientation, !layer.probeOrientation.isIdentity());
            // By this point we will know what the actual texture format of the light probe is
            // Check if using RGBE format light probe texture (the Rhi format will be RGBA8)
            if (lightProbeValid &&
                layer.lightProbe->m_textureData.m_rhiTexture &&
                layer.lightProbe->m_textureData.m_textureFlags.isRgbe8())
            {
                setShaderFeature(QSSGShaderDefines::RGBELightProbe, true);
            }

            // ### TODO: Really this should only be done if renderableNodes is empty or dirty
            // but we don't have a way to say it's dirty yet (new renderables added to the tree)

            // Do not just clear() renderableNodes and friends. Rather, reuse
            // the space (even if clear does not actually deallocate, it still
            // costs time to run dtors and such). In scenes with a static node
            // count in the range of thousands this may matter.
            int renderableNodeCount = 0;
            int cameraNodeCount = 0;
            int lightNodeCount = 0;
            quint32 dfsIndex = 0;
            for (auto &theChild : layer.children)
                maybeQueueNodeForRender(theChild, renderableNodes, renderableNodeCount, cameras, cameraNodeCount, lights, lightNodeCount, dfsIndex);

            if (renderableNodes.size() != renderableNodeCount)
                renderableNodes.resize(renderableNodeCount);
            if (cameras.size() != cameraNodeCount)
                cameras.resize(cameraNodeCount);
            if (lights.size() != lightNodeCount)
                lights.resize(lightNodeCount);

            renderableItem2Ds.clear();

            globalLights.clear();
            for (const auto &oo : qAsConst(opaqueObjects))
                delete oo.obj;
            opaqueObjects.clear();
            for (const auto &to : qAsConst(transparentObjects))
                delete to.obj;
            transparentObjects.clear();

            // Cameras
            // First, check the activeCamera is GloballyActive
            // and then if not, seek a GloballyActive one from the first
            camera = layer.activeCamera;
            if (camera != nullptr) {
                camera->dpr = renderer->contextInterface()->dpr();
                wasDataDirty = wasDataDirty
                    || camera->flags.testFlag(QSSGRenderNode::Flag::Dirty);
                QSSGCameraGlobalCalculationResult theResult = thePrepResult.setupCameraForRender(*camera);
                wasDataDirty = wasDataDirty || theResult.m_wasDirty;
                if (!theResult.m_computeFrustumSucceeded)
                    qCCritical(INTERNAL_ERROR, "Failed to calculate camera frustum");
                if (!camera->flags.testFlag(QSSGRenderCamera::Flag::GloballyActive))
                    camera = nullptr;

            }
            for (auto iter = cameras.cbegin();
                    (camera == nullptr) && (iter != cameras.cend()); iter++) {
                QSSGRenderCamera *theCamera = *iter;
                theCamera->dpr = renderer->contextInterface()->dpr();
                wasDataDirty = wasDataDirty
                    || theCamera->flags.testFlag(QSSGRenderNode::Flag::Dirty);
                QSSGCameraGlobalCalculationResult theResult = thePrepResult.setupCameraForRender(*theCamera);
                wasDataDirty = wasDataDirty || theResult.m_wasDirty;
                if (!theResult.m_computeFrustumSucceeded)
                    qCCritical(INTERNAL_ERROR, "Failed to calculate camera frustum");
                if (theCamera->flags.testFlag(QSSGRenderCamera::Flag::GloballyActive))
                    camera = theCamera;
            }
            layer.renderedCamera = camera;

            QSSGShaderLightList renderableLights;
            // Lights
            for (auto rIt = lights.crbegin(); rIt != lights.crend(); rIt++) {
                QSSGRenderLight *theLight = *rIt;
                wasDataDirty = wasDataDirty || theLight->flags.testFlag(QSSGRenderNode::Flag::Dirty);
                bool lightResult = theLight->calculateGlobalVariables();
                wasDataDirty = lightResult || wasDataDirty;

                QSSGShaderLight shaderLight;
                shaderLight.light = theLight;
                shaderLight.enabled = theLight->flags.testFlag(QSSGRenderLight::Flag::GloballyActive);
                shaderLight.enabled &= theLight->m_brightness > 0.0f;
                shaderLight.shadows = theLight->m_castShadow;

                if (shaderLight.enabled)
                    renderableLights.push_back(shaderLight);

            }

            const auto lightCount = renderableLights.size();
            for (int lightIdx = 0; lightIdx < lightCount; lightIdx++) {
                auto &shaderLight = renderableLights[lightIdx];
                shaderLight.direction = shaderLight.light->getScalingCorrectDirection();
                if (shaderLight.shadows) {
                    if (!shadowMapManager)
                        shadowMapManager = new QSSGRenderShadowMap(*renderer->contextInterface());

                    quint32 mapSize = 1 << shaderLight.light->m_shadowMapRes;
                    ShadowMapModes mapMode = (shaderLight.light->m_lightType != QSSGRenderLight::Type::Directional)
                            ? ShadowMapModes::CUBE
                            : ShadowMapModes::VSM;
                    shadowMapManager->addShadowMapEntry(lightIdx,
                                                        mapSize,
                                                        mapSize,
                                                        mapMode);
                    thePrepResult.flags.setRequiresShadowMapPass(true);
                    // Any light with castShadow=true triggers shadow mapping
                    // in the generated shaders. The fact that some (or even
                    // all) objects may opt out from receiving shadows plays no
                    // role here whatsoever.
                    setShaderFeature(QSSGShaderDefines::Ssm, true);
                }
            }
            globalLights = renderableLights;
            for (qint32 idx = 0, end = renderableNodes.size(); idx < end; ++idx) {
                QSSGRenderableNodeEntry &theNodeEntry(renderableNodes[idx]);
                theNodeEntry.lights = renderableLights;
                for (auto &light : theNodeEntry.lights) {
                    if (light.light->m_scope)
                        light.enabled = scopeLight(theNodeEntry.node, light.light->m_scope);
                }
            }

            if (camera) {
                camera->calculateViewProjectionMatrix(viewProjection);
                if (camera->enableFrustumClipping) {
                    QSSGClipPlane nearPlane;
                    QMatrix3x3 theUpper33(camera->globalTransform.normalMatrix());

                    QVector3D dir(mat33::transform(theUpper33, QVector3D(0, 0, -1)));
                    dir.normalize();
                    nearPlane.normal = dir;
                    QVector3D theGlobalPos = camera->getGlobalPos() + camera->clipNear * dir;
                    nearPlane.d = -(QVector3D::dotProduct(dir, theGlobalPos));
                    // the near plane's bbox edges are calculated in the clipping frustum's
                    // constructor.
                    clippingFrustum = QSSGClippingFrustum(viewProjection, nearPlane);
                } else if (clippingFrustum.hasValue()) {
                    clippingFrustum.setEmpty();
                }
            } else
                viewProjection = QMatrix4x4();

            modelContexts.clear();

            bool renderablesDirty = prepareRenderablesForRender(viewProjection,
                                                                clippingFrustum,
                                                                thePrepResult.flags);
            wasDataDirty = wasDataDirty || renderablesDirty;
        }

        setShaderFeature(QSSGShaderDefines::Ssao, thePrepResult.flags.requiresSsaoPass());

        // Tonemapping
        setShaderFeature(QSSGShaderDefines::LinearTonemapping,
                         layer.tonemapMode == QSSGRenderLayer::TonemapMode::Linear);
        setShaderFeature(QSSGShaderDefines::AcesTonemapping,
                         layer.tonemapMode == QSSGRenderLayer::TonemapMode::Aces);
        setShaderFeature(QSSGShaderDefines::HejlDawsonTonemapping,
                         layer.tonemapMode == QSSGRenderLayer::TonemapMode::HejlDawson);
        setShaderFeature(QSSGShaderDefines::FilmicTonemapping,
                         layer.tonemapMode == QSSGRenderLayer::TonemapMode::Filmic);
    }
    wasDirty = wasDirty || wasDataDirty;
    thePrepResult.flags.setWasDirty(wasDirty);
    thePrepResult.flags.setLayerDataDirty(wasDataDirty);

    layerPrepResult = thePrepResult;

    // Per-frame cache of renderable objects post-sort.
    getOpaqueRenderableObjects();
    // If layer depth test is false, this may also contain opaque objects.
    getTransparentRenderableObjects();

    getCameraDirection();
}

void QSSGLayerRenderPreparationData::resetForFrame()
{
    transparentObjects.clear();
    opaqueObjects.clear();
    layerPrepResult.setEmpty();
    // The check for if the camera is or is not null is used
    // to figure out if this layer was rendered at all.
    camera = nullptr;
    cameraDirection.setEmpty();
    renderedOpaqueObjects.clear();
    renderedTransparentObjects.clear();
    renderedItem2Ds.clear();
}

QT_END_NAMESPACE
