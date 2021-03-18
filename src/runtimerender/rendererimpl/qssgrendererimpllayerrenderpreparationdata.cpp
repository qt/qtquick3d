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
#include <QtQuick3DRuntimeRender/private/qssgrendererimpl_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderlayer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendereffect_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderlight_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercamera_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercontextcore_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderresourcemanager_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendereffectsystem_p.h>
#include <QtQuick3DRender/private/qssgrenderframebuffer_p.h>
#include <QtQuick3DRender/private/qssgrenderrenderbuffer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderresourcebufferobjects_p.h>
#include <QtQuick3DUtils/private/qssgperftimer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderbuffermanager_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercustommaterialsystem_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercache_p.h>
#include <QtQuick3DRuntimeRender/private/qssgperframeallocator_p.h>
#include <QtQuick3DUtils/private/qssgutils_p.h>
#include <QtQuick3DRuntimeRender/private/qssgruntimerenderlogging_p.h>

#ifdef Q_CC_MSVC
#pragma warning(disable : 4355)
#endif

QT_BEGIN_NAMESPACE

static void maybeQueueNodeForRender(QSSGRenderNode &inNode,
                                    QVector<QSSGRenderableNodeEntry> &outRenderables,
                                    QVector<QSSGRenderCamera *> &outCameras,
                                    QVector<QSSGRenderLight *> &outLights,
                                    quint32 &ioDFSIndex)
{
    ++ioDFSIndex;
    inNode.dfsIndex = ioDFSIndex;
    if (inNode.isRenderableType())
        outRenderables.push_back(inNode);
    else if (inNode.type == QSSGRenderGraphObject::Type::Camera)
        outCameras.push_back(static_cast<QSSGRenderCamera *>(&inNode));
    else if (inNode.type == QSSGRenderGraphObject::Type::Light)
        outLights.push_back(static_cast<QSSGRenderLight *>(&inNode));

    for (QSSGRenderNode *theChild = inNode.firstChild; theChild != nullptr; theChild = theChild->nextSibling)
        maybeQueueNodeForRender(*theChild, outRenderables, outCameras, outLights, ioDFSIndex);
}

static inline bool hasValidLightProbe(QSSGRenderImage *inLightProbeImage)
{
    return inLightProbeImage && inLightProbeImage->m_textureData.m_texture;
}

QSSGDefaultMaterialPreparationResult::QSSGDefaultMaterialPreparationResult(QSSGShaderDefaultMaterialKey inKey)
    : firstImage(nullptr), opacity(1.0f), materialKey(inKey), dirty(false)
{
}

#define MAX_AA_LEVELS 8

QSSGLayerRenderPreparationData::QSSGLayerRenderPreparationData(QSSGRenderLayer &inLayer,
                                                                   const QSSGRef<QSSGRendererImpl> &inRenderer)
    : layer(inLayer)
    , renderer(inRenderer)
    , camera(nullptr)
    , featuresDirty(true)
    , featureSetHash(0)
    , tooManyLightsError(false)
{
}

QSSGLayerRenderPreparationData::~QSSGLayerRenderPreparationData() = default;

void QSSGLayerRenderPreparationData::setShaderFeature(const char *theStr, bool inValue)
{
    auto iter = features.cbegin();
    const auto end = features.cend();

    while (iter != end && iter->name != theStr)
        ++iter;

    if (iter != end) {
        if (iter->enabled != inValue) {
            iter->enabled = inValue;
            featuresDirty = true;
            featureSetHash = 0;
        }
    } else {
        features.push_back(QSSGShaderPreprocessorFeature{theStr, inValue});
        featuresDirty = true;
        featureSetHash = 0;
    }
}

ShaderFeatureSetList QSSGLayerRenderPreparationData::getShaderFeatureSet()
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

void QSSGLayerRenderPreparationData::createShadowMapManager()
{
    shadowMapManager = QSSGRenderShadowMap::create(renderer->contextInterface());
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

QSSGShaderDefaultMaterialKey QSSGLayerRenderPreparationData::generateLightingKey(QSSGRenderDefaultMaterial::MaterialLighting inLightingType, bool receivesShadows)
{
    const uint features = uint(getShaderFeatureSetHash());
    QSSGShaderDefaultMaterialKey theGeneratedKey(features);
    const bool lighting = inLightingType != QSSGRenderDefaultMaterial::MaterialLighting::NoLighting;
    renderer->defaultMaterialShaderKeyProperties().m_hasLighting.setValue(theGeneratedKey, lighting);
    if (lighting) {
        const bool lightProbe = layer.lightProbe && layer.lightProbe->m_textureData.m_texture;
        renderer->defaultMaterialShaderKeyProperties().m_hasIbl.setValue(theGeneratedKey, lightProbe);

        quint32 numLights = quint32(globalLights.size());
        if (Q_UNLIKELY(numLights > QSSGShaderDefaultMaterialKeyProperties::LightCount && !tooManyLightsError)) {
            tooManyLightsError = true;
            numLights = QSSGShaderDefaultMaterialKeyProperties::LightCount;
            qCCritical(INVALID_OPERATION, "Too many lights on layer, max is %d", QSSGShaderDefaultMaterialKeyProperties::LightCount);
            Q_ASSERT(false);
        }
        renderer->defaultMaterialShaderKeyProperties().m_lightCount.setValue(theGeneratedKey, numLights);

        for (qint32 lightIdx = 0, lightEnd = globalLights.size(); lightIdx < lightEnd; ++lightIdx) {
            QSSGRenderLight *theLight(globalLights[lightIdx]);
            const bool isDirectional = theLight->m_lightType == QSSGRenderLight::Type::Directional;
            const bool isArea = theLight->m_lightType == QSSGRenderLight::Type::Area;
            const bool isSpot = theLight->m_lightType == QSSGRenderLight::Type::Spot;
            const bool castShadowsArea = (theLight->m_lightType != QSSGRenderLight::Type::Area) && (theLight->m_castShadow) && receivesShadows;

            renderer->defaultMaterialShaderKeyProperties().m_lightFlags[lightIdx].setValue(theGeneratedKey, !isDirectional);
            renderer->defaultMaterialShaderKeyProperties().m_lightAreaFlags[lightIdx].setValue(theGeneratedKey, isArea);
            renderer->defaultMaterialShaderKeyProperties().m_lightSpotFlags[lightIdx].setValue(theGeneratedKey, isSpot);
            renderer->defaultMaterialShaderKeyProperties().m_lightShadowFlags[lightIdx].setValue(theGeneratedKey, castShadowsArea);
        }
    }
    return theGeneratedKey;
}

void QSSGLayerRenderPreparationData::prepareImageForRender(QSSGRenderImage &inImage,
                                                           QSSGImageMapTypes inMapType,
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

    if (inImage.m_textureData.m_texture) {
        if (inImage.m_textureData.m_textureFlags.hasTransparency()
            && (inMapType == QSSGImageMapTypes::Diffuse || inMapType == QSSGImageMapTypes::Opacity
                || inMapType == QSSGImageMapTypes::Translucency)) {
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
        switch (inImage.m_textureData.m_texture->textureDetails().format.format) {
        case QSSGRenderTextureFormat::RG8:
        case QSSGRenderTextureFormat::RG16F:
        case QSSGRenderTextureFormat::RG32F:
            hasG = true;
            break;
        case QSSGRenderTextureFormat::RGB8:
        case QSSGRenderTextureFormat::RGB16F:
        case QSSGRenderTextureFormat::RGB32F:
            hasG = true;
            hasB = true;
            break;
        case QSSGRenderTextureFormat::Alpha8:
            hasA = true;
            break;
        case QSSGRenderTextureFormat::LuminanceAlpha8:
            hasA = true;
            hasG = true;
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

        QSSGShaderKeyTextureSwizzle &theSwizzleKeyProp = renderer->defaultMaterialShaderKeyProperties().m_textureSwizzle[inImageIndex];
        theSwizzleKeyProp.setSwizzleMode(inShaderKey, inImage.m_textureData.m_texture->textureSwizzleMode(), true);

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
    if (renderableFlags.hasAttributeTangent())
        vertexAttribs |= QSSGShaderKeyVertexAttribute::Tangent;
    if (renderableFlags.hasAttributeBinormal())
        vertexAttribs |= QSSGShaderKeyVertexAttribute::Binormal;
    if (renderableFlags.hasAttributeColor())
        vertexAttribs |= QSSGShaderKeyVertexAttribute::Color;
    renderer->defaultMaterialShaderKeyProperties().m_vertexAttributes.setValue(key, vertexAttribs);
}

QSSGDefaultMaterialPreparationResult QSSGLayerRenderPreparationData::prepareDefaultMaterialForRender(
        QSSGRenderDefaultMaterial &inMaterial,
        QSSGRenderableObjectFlags &inExistingFlags,
        float inOpacity)
{
    QSSGRenderDefaultMaterial *theMaterial = &inMaterial;
    QSSGDefaultMaterialPreparationResult retval(generateLightingKey(theMaterial->lighting, inExistingFlags.receivesShadows()));
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

    // set wireframe mode
    renderer->defaultMaterialShaderKeyProperties().m_wireframeMode.setValue(theGeneratedKey,
                                                                            renderer->contextInterface()->wireframeMode());
    // isDoubleSided
    renderer->defaultMaterialShaderKeyProperties().m_isDoubleSided.setValue(theGeneratedKey, theMaterial->cullMode == QSSGCullFaceMode::Disabled);

    // alpha Mode
    renderer->defaultMaterialShaderKeyProperties().m_alphaMode.setValue(theGeneratedKey, theMaterial->alphaMode);

    // vertex attribute presence flags
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
    renderer->defaultMaterialShaderKeyProperties().m_vertexAttributes.setValue(theGeneratedKey, vertexAttribs);

    if (theMaterial->iblProbe && checkLightProbeDirty(*theMaterial->iblProbe)) {
        renderer->prepareImageForIbl(*theMaterial->iblProbe);
    }

    if (!renderer->defaultMaterialShaderKeyProperties().m_hasIbl.getValue(theGeneratedKey)) {
        bool lightProbeValid = hasValidLightProbe(theMaterial->iblProbe);
        setShaderFeature(QSSGShaderDefines::asString(QSSGShaderDefines::LightProbe), lightProbeValid);
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
                                    QSSGImageMapTypes::BaseColor,
                                    QSSGShaderDefaultMaterialKeyProperties::BaseColorMap);
            CHECK_IMAGE_AND_PREPARE(theMaterial->metalnessMap,
                                    QSSGImageMapTypes::Metalness,
                                    QSSGShaderDefaultMaterialKeyProperties::MetalnessMap);
            CHECK_IMAGE_AND_PREPARE(theMaterial->occlusionMap,
                                    QSSGImageMapTypes::Occlusion,
                                    QSSGShaderDefaultMaterialKeyProperties::OcclusionMap);
        } else {
            CHECK_IMAGE_AND_PREPARE(theMaterial->colorMap,
                                    QSSGImageMapTypes::Diffuse,
                                    QSSGShaderDefaultMaterialKeyProperties::DiffuseMap);
        }
        CHECK_IMAGE_AND_PREPARE(theMaterial->emissiveMap, QSSGImageMapTypes::Emissive, QSSGShaderDefaultMaterialKeyProperties::EmissiveMap);
        CHECK_IMAGE_AND_PREPARE(theMaterial->specularReflection,
                                QSSGImageMapTypes::Specular,
                                QSSGShaderDefaultMaterialKeyProperties::SpecularMap);
        CHECK_IMAGE_AND_PREPARE(theMaterial->roughnessMap,
                                QSSGImageMapTypes::Roughness,
                                QSSGShaderDefaultMaterialKeyProperties::RoughnessMap);
        CHECK_IMAGE_AND_PREPARE(theMaterial->opacityMap, QSSGImageMapTypes::Opacity, QSSGShaderDefaultMaterialKeyProperties::OpacityMap);
        CHECK_IMAGE_AND_PREPARE(theMaterial->bumpMap, QSSGImageMapTypes::Bump, QSSGShaderDefaultMaterialKeyProperties::BumpMap);
        CHECK_IMAGE_AND_PREPARE(theMaterial->specularMap,
                                QSSGImageMapTypes::SpecularAmountMap,
                                QSSGShaderDefaultMaterialKeyProperties::SpecularAmountMap);
        CHECK_IMAGE_AND_PREPARE(theMaterial->normalMap, QSSGImageMapTypes::Normal, QSSGShaderDefaultMaterialKeyProperties::NormalMap);
        CHECK_IMAGE_AND_PREPARE(theMaterial->displacementMap,
                                QSSGImageMapTypes::Displacement,
                                QSSGShaderDefaultMaterialKeyProperties::DisplacementMap);
        CHECK_IMAGE_AND_PREPARE(theMaterial->translucencyMap,
                                QSSGImageMapTypes::Translucency,
                                QSSGShaderDefaultMaterialKeyProperties::TranslucencyMap);
        CHECK_IMAGE_AND_PREPARE(theMaterial->lightmaps.m_lightmapIndirect,
                                QSSGImageMapTypes::LightmapIndirect,
                                QSSGShaderDefaultMaterialKeyProperties::LightmapIndirect);
        CHECK_IMAGE_AND_PREPARE(theMaterial->lightmaps.m_lightmapRadiosity,
                                QSSGImageMapTypes::LightmapRadiosity,
                                QSSGShaderDefaultMaterialKeyProperties::LightmapRadiosity);
        CHECK_IMAGE_AND_PREPARE(theMaterial->lightmaps.m_lightmapShadow,
                                QSSGImageMapTypes::LightmapShadow,
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

QSSGDefaultMaterialPreparationResult QSSGLayerRenderPreparationData::prepareCustomMaterialForRender(QSSGRenderCustomMaterial &inMaterial,
                                                                                                    QSSGRenderableObjectFlags &inExistingFlags,
                                                                                                    float inOpacity, bool alreadyDirty)
{
    QSSGDefaultMaterialPreparationResult retval(generateLightingKey(QSSGRenderDefaultMaterial::MaterialLighting::FragmentLighting, inExistingFlags.receivesShadows())); // always fragment lighting
    retval.renderableFlags = inExistingFlags;
    QSSGRenderableObjectFlags &renderableFlags(retval.renderableFlags);
    QSSGShaderDefaultMaterialKey &theGeneratedKey(retval.materialKey);
    retval.opacity = inOpacity;
    float &subsetOpacity(retval.opacity);

    // set wireframe mode
    renderer->defaultMaterialShaderKeyProperties().m_wireframeMode.setValue(theGeneratedKey,
                                                                            renderer->contextInterface()->wireframeMode());

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

    QSSGRenderableImage *firstImage = nullptr;
    QSSGRenderableImage *nextImage = nullptr;

#define CHECK_IMAGE_AND_PREPARE(img, imgtype, shadercomponent)                          \
    if ((img))                                                                          \
        prepareImageForRender(*(img), imgtype, firstImage, nextImage, renderableFlags,  \
                              theGeneratedKey, shadercomponent, nullptr)

    CHECK_IMAGE_AND_PREPARE(inMaterial.m_displacementMap,
                            QSSGImageMapTypes::Displacement,
                            QSSGShaderDefaultMaterialKeyProperties::DisplacementMap);
    CHECK_IMAGE_AND_PREPARE(inMaterial.m_lightmaps.m_lightmapIndirect,
                            QSSGImageMapTypes::LightmapIndirect,
                            QSSGShaderDefaultMaterialKeyProperties::LightmapIndirect);
    CHECK_IMAGE_AND_PREPARE(inMaterial.m_lightmaps.m_lightmapRadiosity,
                            QSSGImageMapTypes::LightmapRadiosity,
                            QSSGShaderDefaultMaterialKeyProperties::LightmapRadiosity);
    CHECK_IMAGE_AND_PREPARE(inMaterial.m_lightmaps.m_lightmapShadow,
                            QSSGImageMapTypes::LightmapShadow,
                            QSSGShaderDefaultMaterialKeyProperties::LightmapShadow);
#undef CHECK_IMAGE_AND_PREPARE

    retval.firstImage = firstImage;
    if (retval.dirty || alreadyDirty)
        renderer->addMaterialDirtyClear(&inMaterial);

    // register the custom material shaders with the dynamic object system if they
    // are not registered already
    const QSSGRef<QSSGDynamicObjectSystem> &theDynamicSystem(renderer->contextInterface()->dynamicObjectSystem());
    for (auto shaderPath : inMaterial.shaders.keys())
        theDynamicSystem->setShaderData(shaderPath,
                                        inMaterial.shaders[shaderPath],
                                        inMaterial.shaderInfo.type,
                                        inMaterial.shaderInfo.version,
                                        false,
                                        false);

    return retval;
}

bool QSSGLayerRenderPreparationData::prepareModelForRender(QSSGRenderModel &inModel,
                                                             const QMatrix4x4 &inViewProjection,
                                                             const QSSGOption<QSSGClippingFrustum> &inClipFrustum,
                                                             QSSGNodeLightEntryList &inScopedLights)
{
    const QSSGRef<QSSGRenderContextInterface> &contextInterface(renderer->contextInterface());
    const QSSGRef<QSSGBufferManager> &bufferManager = contextInterface->bufferManager();

    QSSGRenderMesh *theMesh = nullptr;
    // create custom mesh if set
    if (inModel.meshPath.isNull() && inModel.geometry)
        theMesh = inModel.geometry->createOrUpdate(bufferManager);
    else
        theMesh = bufferManager->loadMesh(inModel.meshPath);

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

    const QSSGScopedLightsListScope lightsScope(globalLights, lightDirections, sourceLightDirections, inScopedLights);
    setShaderFeature(QSSGShaderDefines::asString(QSSGShaderDefines::CgLighting), !globalLights.empty());
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
            QSSGRenderSubset &theSubset(theOuterSubset);
            QSSGRenderableObjectFlags renderableFlags;
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

            renderableFlags.setPickable(canModelBePickable);

            // Casting and Receiving Shadows
            renderableFlags.setCastsShadows(inModel.castsShadows);
            renderableFlags.setReceivesShadows(inModel.receivesShadows);

            for (const QByteArray &attr : qAsConst(theMesh->inputLayoutInputNames)) {
                using namespace QSSGMeshUtilities;
                if (attr == Mesh::getPositionAttrName())
                    renderableFlags.setHasAttributePosition(true);
                else if (attr == Mesh::getNormalAttrName())
                    renderableFlags.setHasAttributeNormal(true);
                else if (attr == Mesh::getUVAttrName())
                    renderableFlags.setHasAttributeTexCoord0(true);
                else if (attr == Mesh::getUV2AttrName())
                    renderableFlags.setHasAttributeTexCoord1(true);
                else if (attr == Mesh::getTexTanAttrName())
                    renderableFlags.setHasAttributeTangent(true);
                else if (attr == Mesh::getTexBinormalAttrName())
                    renderableFlags.setHasAttributeBinormal(true);
                else if (attr == Mesh::getColorAttrName())
                    renderableFlags.setHasAttributeColor(true);
            }

            QSSGRenderableObject *theRenderableObject = nullptr;
            QSSGRenderGraphObject *theMaterialObject = theSourceMaterialObject;

            if (theMaterialObject == nullptr)
                continue;

            // set tessellation
            if (inModel.tessellationMode != TessellationModeValues::NoTessellation) {
                theSubset.primitiveType = QSSGRenderDrawMode::Patches;
                // set tessellation factor
                theSubset.edgeTessFactor = inModel.edgeTessellation;
                theSubset.innerTessFactor = inModel.innerTessellation;
                // update the vertex ver patch count in the input assembler
                // currently we only support triangle patches so count is always 3
                theSubset.inputAssembler->setPatchVertexCount(3);
                theSubset.inputAssemblerDepth->setPatchVertexCount(3);
                // check wireframe mode
                theSubset.wireframeMode = contextInterface->wireframeMode();

                subsetDirty = subsetDirty | (theSubset.wireframeMode != inModel.wireframeMode);
                inModel.wireframeMode = contextInterface->wireframeMode();
            } else {
                theSubset.primitiveType = theSubset.inputAssembler->drawMode();
                theSubset.inputAssembler->setPatchVertexCount(1);
                theSubset.inputAssemblerDepth->setPatchVertexCount(1);
                // currently we allow wirframe mode only if tessellation is on
                theSubset.wireframeMode = false;

                subsetDirty = subsetDirty | (theSubset.wireframeMode != inModel.wireframeMode);
                inModel.wireframeMode = false;
            }

            if (theMaterialObject == nullptr)
                continue;

            if (theMaterialObject->type == QSSGRenderGraphObject::Type::DefaultMaterial || theMaterialObject->type == QSSGRenderGraphObject::Type::PrincipledMaterial) {
                QSSGRenderDefaultMaterial &theMaterial(static_cast<QSSGRenderDefaultMaterial &>(*theMaterialObject));
                // vertexColor should be supported in both DefaultMaterial and PrincipleMaterial
                // if the mesh has it.
                theMaterial.vertexColorsEnabled = renderableFlags.hasAttributeColor();
                QSSGDefaultMaterialPreparationResult theMaterialPrepResult(
                        prepareDefaultMaterialForRender(theMaterial, renderableFlags, subsetOpacity));
                QSSGShaderDefaultMaterialKey theGeneratedKey = theMaterialPrepResult.materialKey;
                subsetOpacity = theMaterialPrepResult.opacity;
                QSSGRenderableImage *firstImage(theMaterialPrepResult.firstImage);
                subsetDirty |= theMaterialPrepResult.dirty;
                renderableFlags = theMaterialPrepResult.renderableFlags;

                renderer->defaultMaterialShaderKeyProperties().m_tessellationMode.setTessellationMode(theGeneratedKey,
                                                                                                      inModel.tessellationMode,
                                                                                                      true);

                QSSGDataView<QMatrix4x4> boneGlobals;
                if (theSubset.joints.size()) {
                    Q_ASSERT(false);
                }

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
                                                                             boneGlobals);
                subsetDirty = subsetDirty || renderableFlags.isDirty();
            } else if (theMaterialObject->type == QSSGRenderGraphObject::Type::CustomMaterial) {
                QSSGRenderCustomMaterial &theMaterial(static_cast<QSSGRenderCustomMaterial &>(*theMaterialObject));

                const QSSGRef<QSSGMaterialSystem> &theMaterialSystem(contextInterface->customMaterialSystem());
                subsetDirty |= theMaterialSystem->prepareForRender(theModelContext.model, theSubset, theMaterial);

                QSSGDefaultMaterialPreparationResult theMaterialPrepResult(
                        prepareCustomMaterialForRender(theMaterial, renderableFlags, subsetOpacity, subsetDirty));
                QSSGShaderDefaultMaterialKey theGeneratedKey = theMaterialPrepResult.materialKey;
                subsetOpacity = theMaterialPrepResult.opacity;
                QSSGRenderableImage *firstImage(theMaterialPrepResult.firstImage);
                renderableFlags = theMaterialPrepResult.renderableFlags;

                // prepare for render tells us if the object is transparent
                if (theMaterial.m_hasTransparency)
                    renderableFlags |= QSSGRenderableObjectFlag::HasTransparency;
                // prepare for render tells us if the object is transparent
                if (theMaterial.m_hasRefraction)
                    renderableFlags |= QSSGRenderableObjectFlag::HasRefraction;

                renderer->defaultMaterialShaderKeyProperties().m_tessellationMode.setTessellationMode(theGeneratedKey,
                                                                                                      inModel.tessellationMode,
                                                                                                      true);

                if (theMaterial.m_iblProbe && checkLightProbeDirty(*theMaterial.m_iblProbe)) {
                    renderer->prepareImageForIbl(*theMaterial.m_iblProbe);
                }

                theRenderableObject = RENDER_FRAME_NEW<QSSGCustomMaterialRenderable>(renderer->contextInterface(),
                                                                                     renderableFlags,
                                                                                     theModelCenter,
                                                                                     renderer,
                                                                                     theSubset,
                                                                                     theMaterial,
                                                                                     theModelContext,
                                                                                     subsetOpacity,
                                                                                     firstImage,
                                                                                     theGeneratedKey);
            }
            if (theRenderableObject) {
                theRenderableObject->scopedLights = inScopedLights;
                // set tessellation
                theRenderableObject->tessellationMode = inModel.tessellationMode;

                if (theRenderableObject->renderableFlags.hasTransparency() || theRenderableObject->renderableFlags.hasRefraction()) {
                    transparentObjects.push_back(QSSGRenderableObjectHandle::create(theRenderableObject));
                } else {
                    opaqueObjects.push_back(QSSGRenderableObjectHandle::create(theRenderableObject));
                }
            }
        }
    }
    return subsetDirty;
}

bool QSSGLayerRenderPreparationData::prepareRenderablesForRender(const QMatrix4x4 &inViewProjection,
                                                                   const QSSGOption<QSSGClippingFrustum> &inClipFrustum,
                                                                   QSSGLayerRenderPreparationResultFlags &ioFlags)
{
    Q_UNUSED(ioFlags)
    QSSGStackPerfTimer perfTimer(renderer->contextInterface()->performanceTimer(), Q_FUNC_INFO);
    viewProjection = inViewProjection;
    bool wasDataDirty = false;
    for (qint32 idx = 0, end = renderableNodes.size(); idx < end; ++idx) {
        QSSGRenderableNodeEntry &theNodeEntry(renderableNodes[idx]);
        QSSGRenderNode *theNode = theNodeEntry.node;
        wasDataDirty = wasDataDirty || theNode->flags.testFlag(QSSGRenderNode::Flag::Dirty);
        switch (theNode->type) {
        case QSSGRenderGraphObject::Type::Model: {
            QSSGRenderModel *theModel = static_cast<QSSGRenderModel *>(theNode);
            theModel->calculateGlobalVariables();
            if (theModel->flags.testFlag(QSSGRenderModel::Flag::GloballyActive)) {
                bool wasModelDirty = prepareModelForRender(*theModel, inViewProjection, inClipFrustum, theNodeEntry.lights);
                wasDataDirty = wasDataDirty || wasModelDirty;
            }
        } break;
        case QSSGRenderGraphObject::Type::Item2D: {
            QSSGRenderItem2D *theItem2D = static_cast<QSSGRenderItem2D *>(theNode);
            theItem2D->calculateGlobalVariables();
            if (theItem2D->flags.testFlag(QSSGRenderModel::Flag::GloballyActive)) {
                theItem2D->MVP = inViewProjection * theItem2D->globalTransform;
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

struct QSSGLightNodeMarker
{
    QSSGRenderLight *light = nullptr;
    quint32 lightIndex = 0;
    quint32 firstValidIndex = 0;
    quint32 justPastLastValidIndex = 0;
    bool addOrRemove = false;
    QSSGLightNodeMarker() = default;
    QSSGLightNodeMarker(QSSGRenderLight &inLight, quint32 inLightIndex, QSSGRenderNode &inNode, bool aorm)
        : light(&inLight), lightIndex(inLightIndex), addOrRemove(aorm)
    {
        if (inNode.type == QSSGRenderGraphObject::Type::Layer) {
            firstValidIndex = 0;
            justPastLastValidIndex = std::numeric_limits<quint32>::max();
        } else {
            firstValidIndex = inNode.dfsIndex;
            QSSGRenderNode *lastChild = nullptr;
            QSSGRenderNode *firstChild = inNode.firstChild;
            // find deepest last child
            while (firstChild) {
                for (QSSGRenderNode *childNode = firstChild; childNode; childNode = childNode->nextSibling)
                    lastChild = childNode;

                if (lastChild)
                    firstChild = lastChild->firstChild;
                else
                    firstChild = nullptr;
            }
            if (lastChild)
                // last valid index would be the last child index + 1
                justPastLastValidIndex = lastChild->dfsIndex + 1;
            else // no children.
                justPastLastValidIndex = firstValidIndex + 1;
        }
    }
};

void QSSGLayerRenderPreparationData::prepareForRender(const QSize &inViewportDimensions)
{
    QSSGStackPerfTimer perfTimer(renderer->contextInterface()->performanceTimer(), Q_FUNC_INFO);
    if (layerPrepResult.hasValue())
        return;

    features.clear();
    featureSetHash = 0;
    QVector2D thePresentationDimensions((float)inViewportDimensions.width(), (float)inViewportDimensions.height());
    QRect theViewport(renderer->contextInterface()->viewport());
    QRect theScissor(renderer->contextInterface()->scissorRect());
    if (theScissor.isNull() || (theScissor == theViewport)) {
        theScissor = theViewport;
        renderer->contextInterface()->renderContext()->setScissorTestEnabled(false);
    } else {
        renderer->contextInterface()->renderContext()->setScissorTestEnabled(true);
    }

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
    bool SSDOEnabled = (layer.shadowStrength > 0.0f && layer.shadowDist > 0.0f);
    setShaderFeature(QSSGShaderDefines::asString(QSSGShaderDefines::Ssao), SSAOEnabled);
    setShaderFeature(QSSGShaderDefines::asString(QSSGShaderDefines::Ssdo), SSDOEnabled);
    bool requiresDepthPrepass = (SSAOEnabled || SSDOEnabled);
    setShaderFeature(QSSGShaderDefines::asString(QSSGShaderDefines::Ssm), false); // by default no shadow map generation

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
                    requiresDepthPrepass = true;
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
        thePrepResult.flags.setRequiresDepthTexture(requiresDepthPrepass);
        if (renderer->context()->renderContextType() != QSSGRenderContextType::GLES2)
            thePrepResult.flags.setRequiresSsaoPass(SSAOEnabled);

        if (thePrepResult.isLayerVisible()) {
            if (layer.lightProbe && checkLightProbeDirty(*layer.lightProbe)) {
                renderer->prepareImageForIbl(*layer.lightProbe);
                wasDataDirty = true;
            }

            bool lightProbeValid = hasValidLightProbe(layer.lightProbe);

            setShaderFeature(QSSGShaderDefines::asString(QSSGShaderDefines::LightProbe), lightProbeValid);
            setShaderFeature(QSSGShaderDefines::asString(QSSGShaderDefines::IblFov), layer.probeFov < 180.0f);

            if (lightProbeValid && layer.lightProbe2 && checkLightProbeDirty(*layer.lightProbe2)) {
                renderer->prepareImageForIbl(*layer.lightProbe2);
                wasDataDirty = true;
            }

            setShaderFeature(QSSGShaderDefines::asString(QSSGShaderDefines::LightProbe2), lightProbeValid && hasValidLightProbe(layer.lightProbe2));

            // Push nodes in reverse depth first order
//            if (renderableNodes.empty()) {
//                camerasAndLights.clear();
//                quint32 dfsIndex = 0;
//                for (QSSGRenderNode *theChild = layer.firstChild; theChild; theChild = theChild->nextSibling)
//                    MaybeQueueNodeForRender(*theChild, renderableNodes, camerasAndLights, dfsIndex);
//                std::reverse(camerasAndLights.begin(), camerasAndLights.end());
//                std::reverse(renderableNodes.begin(), renderableNodes.end());
//                lightToNodeMap.clear();
//            }
            // ### TODO: Really this should only be done if renderableNodes is empty or dirty
            // but we don't have a way to say it's dirty yet (new renderables added to the tree)
            cameras.clear();
            lights.clear();
            renderableNodes.clear();
            renderableItem2Ds.clear();
            quint32 dfsIndex = 0;
            for (QSSGRenderNode *theChild = layer.firstChild; theChild; theChild = theChild->nextSibling)
                maybeQueueNodeForRender(*theChild, renderableNodes, cameras, lights, dfsIndex);
            lightToNodeMap.clear();

            globalLights.clear();
            for (const auto &oo : qAsConst(opaqueObjects))
                delete oo.obj;
            opaqueObjects.clear();
            for (const auto &to : qAsConst(transparentObjects))
                delete to.obj;
            transparentObjects.clear();
            QVector<QSSGLightNodeMarker> theLightNodeMarkers;
            sourceLightDirections.clear();

            // Cameras
            // First, check the activeCamera is GloballyActive
            // and then if not, seek a GloballyActive one from the first
            camera = layer.activeCamera;
            if (camera != nullptr) {
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

            // Lights
            for (auto rIt = lights.crbegin(); rIt != lights.crend(); rIt++) {
                QSSGRenderLight *theLight = *rIt;
                wasDataDirty = wasDataDirty || theLight->flags.testFlag(QSSGRenderNode::Flag::Dirty);
                bool lightResult = theLight->calculateGlobalVariables();
                wasDataDirty = lightResult || wasDataDirty;
                // Note we setup the light index such that it is completely invariant of if
                // the
                // light is active or scoped.
                quint32 lightIndex = (quint32)sourceLightDirections.size();
                sourceLightDirections.push_back(QVector3D(0.0, 0.0, 0.0));
                // Note we still need a light check when building the renderable light list.
                // We also cannot cache shader-light bindings based on layers any more
                // because
                // the number of lights for a given renderable does not depend on the layer
                // as it used to but
                // additional perhaps on the light's scoping rules.
                if (theLight->flags.testFlag(QSSGRenderLight::Flag::GloballyActive)) {
                    if (theLight->m_scope == nullptr) {
                        globalLights.push_back(theLight);
                        if (renderer->context()->renderContextType() != QSSGRenderContextType::GLES2
                                && theLight->m_castShadow) {
                            if (!shadowMapManager)
                                createShadowMapManager();

                            // PKC -- use of "res" as an exponent of two is an annoying
                            // artifact of the XML interface
                            // I'll change this with an enum interface later on, but that's
                            // less important right now.
                            quint32 mapSize = 1 << theLight->m_shadowMapRes;
                            ShadowMapModes mapMode = (theLight->m_lightType != QSSGRenderLight::Type::Directional)
                                    ? ShadowMapModes::CUBE
                                    : ShadowMapModes::VSM;
                            shadowMapManager->addShadowMapEntry(globalLights.size() - 1,
                                                                mapSize,
                                                                mapSize,
                                                                QSSGRenderTextureFormat::R16F,
                                                                1,
                                                                mapMode,
                                                                ShadowFilterValues::NONE);
                            thePrepResult.flags.setRequiresShadowMapPass(true);
                            setShaderFeature(QSSGShaderDefines::asString(QSSGShaderDefines::Ssm), true);
                        }
                    }
                    TLightToNodeMap::iterator iter = lightToNodeMap.insert(theLight, (QSSGRenderNode *)nullptr);
                    QSSGRenderNode *oldLightScope = iter.value();
                    QSSGRenderNode *newLightScope = theLight->m_scope;

                    if (oldLightScope != newLightScope) {
                        iter.value() = newLightScope;
                        if (oldLightScope)
                            theLightNodeMarkers.push_back(QSSGLightNodeMarker(*theLight, lightIndex, *oldLightScope, false));
                        if (newLightScope)
                            theLightNodeMarkers.push_back(QSSGLightNodeMarker(*theLight, lightIndex, *newLightScope, true));
                    }
                    if (newLightScope) {
                        sourceLightDirections.back() = theLight->getScalingCorrectDirection();
                    }
                }
            }
            if (!theLightNodeMarkers.empty()) {
                for (auto rIt = renderableNodes.rbegin();
                        rIt != renderableNodes.rend(); rIt++) {
                    QSSGRenderableNodeEntry &theNodeEntry(*rIt);
                    quint32 nodeDFSIndex = theNodeEntry.node->dfsIndex;
                    for (quint32 markerIdx = 0, markerEnd = theLightNodeMarkers.size(); markerIdx < markerEnd; ++markerIdx) {
                        QSSGLightNodeMarker &theMarker = theLightNodeMarkers[markerIdx];
                        if (nodeDFSIndex >= theMarker.firstValidIndex && nodeDFSIndex < theMarker.justPastLastValidIndex) {
                            if (theMarker.addOrRemove) {
                                QSSGNodeLightEntry *theNewEntry = new QSSGNodeLightEntry(theMarker.light, theMarker.lightIndex);
                                theNodeEntry.lights.push_back(*theNewEntry);
                            } else {
                                for (QSSGNodeLightEntryList::iterator lightIter = theNodeEntry.lights.begin(),
                                                                        lightEnd = theNodeEntry.lights.end();
                                     lightIter != lightEnd;
                                     ++lightIter) {
                                    if (lightIter->light == theMarker.light) {
                                        QSSGNodeLightEntry &theEntry = *lightIter;
                                        theNodeEntry.lights.remove(theEntry);
                                        delete &theEntry;
                                        break;
                                    }
                                }
                            }
                        }
                    }
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

            // Setup the light directions here.

            for (qint32 lightIdx = 0, lightEnd = globalLights.size(); lightIdx < lightEnd; ++lightIdx) {
                lightDirections.push_back(globalLights.at(lightIdx)->getScalingCorrectDirection());
            }

            modelContexts.clear();

            bool renderablesDirty = prepareRenderablesForRender(viewProjection,
                                                                clippingFrustum,
                                                                thePrepResult.flags);
            wasDataDirty = wasDataDirty || renderablesDirty;
        }
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
    lightDirections.clear();
    renderedOpaqueObjects.clear();
    renderedTransparentObjects.clear();
    renderedItem2Ds.clear();
}

QT_END_NAMESPACE
