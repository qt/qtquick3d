/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "assimpimporter.h"

#include <assimputils.h>

#include <QtCore/qurl.h>
#include <QtCore/QBuffer>

#include <QtGui/QImage>
#include <QtGui/QImageReader>
#include <QtGui/QImageWriter>
#include <QtGui/QQuaternion>

#include <QtQuick3DAssetImport/private/qssgassetimporterfactory_p.h>
#include <QtQuick3DAssetImport/private/qssgassetimporter_p.h>
#include <QtQuick3DAssetImport/private/qssglightmapuvgenerator_p.h>

#include <QtQuick3DAssetUtils/private/qssgscenedesc_p.h>

#include <QtQuick3DUtils/private/qssgmesh_p.h>

#include <QtQuick3DRuntimeRender/private/qssgrenderbuffermanager_p.h>

// ASSIMP INC
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/Logger.hpp>
#include <assimp/DefaultLogger.hpp>
#include <assimp/postprocess.h>
#include <assimp/pbrmaterial.h>
#include <assimp/importerdesc.h>

// ASSIMP INC

QT_BEGIN_NAMESPACE

////////////////////////  ASSIMP IMP

#define AI_GLTF_FILTER_NEAREST                  0x2600
#define AI_GLTF_FILTER_LINEAR                   0x2601
#define AI_GLTF_FILTER_NEAREST_MIPMAP_NEAREST   0x2700
#define AI_GLTF_FILTER_LINEAR_MIPMAP_NEAREST    0x2701
#define AI_GLTF_FILTER_NEAREST_MIPMAP_LINEAR    0x2702
#define AI_GLTF_FILTER_LINEAR_MIPMAP_LINEAR     0x2703

Q_REQUIRED_RESULT static inline QColor aiColorToQColor(const aiColor3D &color)
{
    return QColor::fromRgbF(color.r, color.g, color.b, 1.0f);
}

Q_REQUIRED_RESULT static inline QColor aiColorToQColor(const aiColor4D &color)
{
    return QColor::fromRgbF(color.r, color.g, color.b, color.a);
}

struct NodeInfo
{
    using Type = QSSGSceneDesc::Node::Type;
    size_t index;
    Type type;
};

Q_DECLARE_TYPEINFO(NodeInfo, Q_PRIMITIVE_TYPE);

using NodeMap = QHash<const aiNode *, NodeInfo>;

struct TextureInfo
{
    aiTextureMapping mapping = aiTextureMapping::aiTextureMapping_UV;
    aiTextureMapMode modes[3] {};
    unsigned int minFilter { AI_GLTF_FILTER_NEAREST_MIPMAP_LINEAR };
    unsigned int magFilter { AI_GLTF_FILTER_NEAREST_MIPMAP_LINEAR };
    uint uvIndex { aiTextureMapping::aiTextureMapping_OTHER };
    aiUVTransform *transform = nullptr;
};

struct SceneInfo
{
    enum class GltfVersion : quint8
    {
        Unknown,
        v1,
        v2
    };

    enum Options
    {
        None,
        generateMipMaps = 0x1
    };

    const aiScene &scene;
    QDir workingDir;
    GltfVersion ver;
    Options opt;
};

static void setNodeProperties(QSSGSceneDesc::Node &target,
                              const aiNode &source,
                              aiMatrix4x4 *transformCorrection,
                              bool skipScaling)
{
    const aiMatrix4x4 &transformMatrix = source.mTransformation;

    // Decompose Transform Matrix to get properties
    aiVector3D scaling;
    aiQuaternion rotation;
    aiVector3D translation;
    transformMatrix.Decompose(scaling, rotation, translation);

    // Apply correction if necessary
    // transformCorrection is just for cameras and lights
    // and its factor just contains rotation.
    // In this case, this rotation will replace previous rotation.
    if (transformCorrection) {
        aiVector3D dummyTrans;
        transformCorrection->DecomposeNoScaling(rotation, dummyTrans);
    }

    // translate
    QSSGSceneDesc::setProperty(target, "position", &QQuick3DNode::setPosition, QVector3D { translation.x, translation.y, translation.z });

    // rotation
    const QQuaternion rot(rotation.w, rotation.x, rotation.y, rotation.z);
    QSSGSceneDesc::setProperty(target, "rotation", &QQuick3DNode::setRotation, rot);

    // scale
    if (!skipScaling) {
        // Apply the global scale for a root node
        // TODO:
        // if (!sourceNode.mParent)
        //     scaling *= m_globalScaleValue;

        QSSGSceneDesc::setProperty(target, "scale", &QQuick3DNode::setScale, QVector3D { scaling.x, scaling.y, scaling.z });
    }
    // pivot

    // opacity

    // boneid

    // visible
}

static void setTextureProperties(QSSGSceneDesc::Texture &target, const TextureInfo &texInfo, const SceneInfo &sceneInfo)
{
    const bool forceMipMapGeneration = (sceneInfo.opt & SceneInfo::Options::generateMipMaps);

    if (texInfo.uvIndex > 0) {
        // Quick3D supports 2 tex coords.
        // According to gltf's khronos default implementation,
        // the index will be selected to the nearest one.
        QSSGSceneDesc::setProperty(target, "indexUV", &QQuick3DTexture::setIndexUV, 1);
    }

    // mapping
    if (texInfo.mapping == aiTextureMapping_UV) {
        // So we should be able to always hit this case by passing the right flags
        // at import.
        QSSGSceneDesc::setProperty(target, "mappingMode", &QQuick3DTexture::setMappingMode, QQuick3DTexture::MappingMode::UV);
        // It would be possible to use another channel than UV0 to map texture data
        // but for now we force everything to use UV0
        //int uvSource;
        //material->Get(AI_MATKEY_UVWSRC(textureType, index), uvSource);
    } // else (not supported)

    static const auto asQtTilingMode = [](aiTextureMapMode mode) {
        switch (mode) {
        case aiTextureMapMode_Wrap:
            return QQuick3DTexture::TilingMode::Repeat;
        case aiTextureMapMode_Clamp:
            return QQuick3DTexture::TilingMode::ClampToEdge;
        case aiTextureMapMode_Mirror:
            return QQuick3DTexture::TilingMode::MirroredRepeat;
        default:
            break;
        }

        return QQuick3DTexture::TilingMode::Repeat;
    };

    // mapping mode U
    QSSGSceneDesc::setProperty(target, "tilingModeHorizontal", &QQuick3DTexture::setHorizontalTiling, asQtTilingMode(texInfo.modes[0]));

    // mapping mode V
    QSSGSceneDesc::setProperty(target, "tilingModeVertical", &QQuick3DTexture::setHorizontalTiling, asQtTilingMode(texInfo.modes[1]));

    if (texInfo.transform) {
        // UV origins -
        //      glTF: 0, 1 (top left of texture)
        //      Assimp, Collada?, FBX?: 0.5, 0.5
        //      Quick3D: 0, 0 (bottom left of texture)
        // Assimp already tries to fix it but it's not correct.
        // So, we restore original values and then use pivot
        const auto &transform = *texInfo.transform;
        float rotation = -transform.mRotation;
        float rotationUV = qRadiansToDegrees(rotation);
        float posU = transform.mTranslation.x;
        float posV = transform.mTranslation.y;
        {
            const float rcos = std::cos(rotation);
            const float rsin = std::sin(rotation);
            posU -= 0.5f * transform.mScaling.x * (-rcos + rsin + 1.0f);
            posV -= (0.5f * transform.mScaling.y * (rcos + rsin - 1.0f) + 1.0f - transform.mScaling.y);
            QSSGSceneDesc::setProperty(target, "pivotV", &QQuick3DTexture::setPivotV, 1.0f);
        }

        QSSGSceneDesc::setProperty(target, "positionU", &QQuick3DTexture::setPositionU, posU);
        QSSGSceneDesc::setProperty(target, "positionV", &QQuick3DTexture::setPositionV, posV);
        QSSGSceneDesc::setProperty(target, "rotationUV", &QQuick3DTexture::setRotationUV, rotationUV);
        QSSGSceneDesc::setProperty(target, "scaleU", &QQuick3DTexture::setScaleU, transform.mScaling.x);
        QSSGSceneDesc::setProperty(target, "scaleV", &QQuick3DTexture::setScaleV, transform.mScaling.y);
    }
    // We don't make use of the data here, but there are additional flags
    // available for example the usage of the alpha channel
    // texture flags
    //int textureFlags;
    //material->Get(AI_MATKEY_TEXFLAGS(textureType, index), textureFlags);

    // Always generate and use mipmaps for imported assets
    bool generateMipMaps = forceMipMapGeneration;
    auto mipFilter = forceMipMapGeneration ? QQuick3DTexture::Filter::Linear : QQuick3DTexture::Filter::None;

    if (sceneInfo.ver == SceneInfo::GltfVersion::v2) {
        // magFilter
        auto filter = (texInfo.magFilter == AI_GLTF_FILTER_NEAREST) ? QQuick3DTexture::Filter::Nearest : QQuick3DTexture::Filter::Linear;
        QSSGSceneDesc::setProperty(target, "magFilter", &QQuick3DTexture::setMagFilter, filter);

        // minFilter
        if (texInfo.magFilter == AI_GLTF_FILTER_NEAREST) {
            filter = QQuick3DTexture::Filter::Nearest;
        } else if (texInfo.magFilter == AI_GLTF_FILTER_NEAREST_MIPMAP_NEAREST) {
            filter = QQuick3DTexture::Filter::Nearest;
            mipFilter = QQuick3DTexture::Filter::Nearest;
        } else if (texInfo.magFilter == AI_GLTF_FILTER_LINEAR_MIPMAP_NEAREST) {
            mipFilter = QQuick3DTexture::Filter::Nearest;
        } else if (texInfo.magFilter == AI_GLTF_FILTER_NEAREST_MIPMAP_LINEAR) {
            filter = QQuick3DTexture::Filter::Nearest;
            mipFilter = QQuick3DTexture::Filter::Linear;
        } else if (texInfo.magFilter == AI_GLTF_FILTER_LINEAR_MIPMAP_LINEAR) {
            mipFilter = QQuick3DTexture::Filter::Linear;
        }
        QSSGSceneDesc::setProperty(target, "minFilter", &QQuick3DTexture::setMinFilter, filter);

        // mipFilter
        generateMipMaps = (mipFilter != QQuick3DTexture::Filter::None);
    }

    if (generateMipMaps) {
        QSSGSceneDesc::setProperty(target, "generateMipmaps", &QQuick3DTexture::setGenerateMipmaps, true);
        QSSGSceneDesc::setProperty(target, "mipFilter", &QQuick3DTexture::setMipFilter, mipFilter);
    }
}

static void setMaterialProperties(QSSGSceneDesc::Material &target, const aiMaterial &source, const SceneInfo &sceneInfo)
{
    const auto createTextureNode = [&sceneInfo, &target](const aiMaterial &material, aiTextureType textureType, unsigned int index) {
        const auto &srcScene = sceneInfo.scene;
        QSSGSceneDesc::Texture *tex = nullptr;
        aiString texturePath;
        TextureInfo texInfo;

        auto &scene = target.scene;

        if (material.GetTexture(textureType, index, &texturePath, &texInfo.mapping, &texInfo.uvIndex, nullptr, nullptr, texInfo.modes) == aiReturn_SUCCESS) {
            if (texturePath.length > 0) {
                aiUVTransform transform;
                if (material.Get(AI_MATKEY_UVTRANSFORM(textureType, index), transform) == aiReturn_SUCCESS)
                    texInfo.transform = &transform;

                material.Get(AI_MATKEY_GLTF_MAPPINGFILTER_MIN(textureType, index), texInfo.minFilter);
                material.Get(AI_MATKEY_GLTF_MAPPINGFILTER_MAG(textureType, index), texInfo.magFilter);

                // TODO: The same texture data can be referenced multiple times, so create cache the
                // data so we don't re-create it for each texture using the same data.
                tex = scene->create<QSSGSceneDesc::Texture>();
                QSSGSceneDesc::addNode(target, *tex);

                // Two types, externally referenced or embedded
                const bool isEmbedded = (*texturePath.C_Str() == '*');
                setTextureProperties(*tex, texInfo, sceneInfo);
                if (isEmbedded) {
                    if (auto sourceTexture = srcScene.GetEmbeddedTexture(texturePath.C_Str())) {
                        Q_ASSERT(sourceTexture->pcData);
                        // Two cases of embedded textures, uncompress and compressed.
                        const bool isCompressed = (sourceTexture->mHeight == 0);
                        // Create a textureData node
                        if (isCompressed) {
                            QByteArray data(reinterpret_cast<char *>(sourceTexture->pcData), sourceTexture->mWidth);
                            QBuffer readBuffer(&data);
                            QByteArray format = sourceTexture->achFormatHint;
                            QImageReader imageReader(&readBuffer, format);
                            const auto image = imageReader.read();
                            if (image.isNull()) {
                                qWarning() << imageReader.errorString();
                            } else {
                                const auto size = image.size();
                                const auto bytes = image.sizeInBytes();
                                auto texData = scene->create<QSSGSceneDesc::TextureData>();
                                QSSGSceneDesc::addNode(*tex, *texData);
                                QSSGSceneDesc::setProperty(*texData, "textureData", &QQuick3DTextureData::setTextureData, QByteArray(reinterpret_cast<const char *>(image.constBits()), bytes));
                                QSSGSceneDesc::setProperty(*texData, "size", &QQuick3DTextureData::setSize, size);
                                QSSGSceneDesc::setProperty(*texData, "format", &QQuick3DTextureData::setFormat, QQuick3DTextureData::Format::RGBA8); // TODO: Format
                                QSSGSceneDesc::setProperty(*tex, "textureData", &QQuick3DTexture::setTextureData, texData);
                            }
                        } else {
                            // RAW
                            auto texData = scene->create<QSSGSceneDesc::TextureData>();
                            QSSGSceneDesc::addNode(*tex, *texData);
                            const auto bytes = sourceTexture->mHeight * sourceTexture->mWidth;
                            QSSGSceneDesc::setProperty(*texData, "textureData", &QQuick3DTextureData::setTextureData, QByteArray(reinterpret_cast<const char *>(sourceTexture->pcData), bytes));
                            QSSGSceneDesc::setProperty(*texData, "size", &QQuick3DTextureData::setSize, QSize{int(sourceTexture->mWidth), int(sourceTexture->mHeight)});
                            QSSGSceneDesc::setProperty(*texData, "format", &QQuick3DTextureData::setFormat, QQuick3DTextureData::Format::RGBA8);
                            QSSGSceneDesc::setProperty(*tex, "textureData", &QQuick3DTexture::setTextureData, texData);
                        }
                    }
                } else {
                    const auto path = sceneInfo.workingDir.canonicalPath() + QDir::separator() + QString::fromUtf8(texturePath.C_Str());
                    if (QFileInfo::exists(path)) {
                        // NOTE: We do not know how this will be used yet, so the source path might need to adjusted
                        // in the qml writer.
                        QSSGSceneDesc::setProperty(*tex, "source", &QQuick3DTexture::setSource, QUrl(path));
                    } else {
                        qWarning("Unable to locate texture at %s\n", qPrintable(path));
                    }
                }
            }
        }

        return tex;
    };

    const bool gltfMode = true;
    if (gltfMode) { // GLTF Mode
        aiReturn result;
        {
            aiColor4D baseColorFactor;
            result = source.Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_BASE_COLOR_FACTOR, baseColorFactor);
            if (result == aiReturn_SUCCESS)
                QSSGSceneDesc::setProperty(target, "baseColor", &QQuick3DPrincipledMaterial::setBaseColor, aiColorToQColor(baseColorFactor));
        }

        if (auto baseColorTexture = createTextureNode(source, AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_BASE_COLOR_TEXTURE)) {
            QSSGSceneDesc::setProperty(target, "baseColorMap", &QQuick3DPrincipledMaterial::setBaseColorMap, baseColorTexture);
            QSSGSceneDesc::setProperty(target, "opacityChannel", &QQuick3DPrincipledMaterial::setOpacityChannel, QQuick3DPrincipledMaterial::TextureChannelMapping::A);
        }

        if (auto metalicRoughnessTexture = createTextureNode(source, AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE)) {
            QSSGSceneDesc::setProperty(target, "metalnessMap", &QQuick3DPrincipledMaterial::setRoughnessMap, metalicRoughnessTexture);
            QSSGSceneDesc::setProperty(target, "metalnessChannel", &QQuick3DPrincipledMaterial::setMetalnessChannel, QQuick3DPrincipledMaterial::TextureChannelMapping::B);
            QSSGSceneDesc::setProperty(target, "roughnessMap", &QQuick3DPrincipledMaterial::setRoughnessMap, metalicRoughnessTexture);
            QSSGSceneDesc::setProperty(target, "roughnessChannel", &QQuick3DPrincipledMaterial::setRoughnessChannel, QQuick3DPrincipledMaterial::TextureChannelMapping::G);
        }

        {
            ai_real metallicFactor;
            result = source.Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLIC_FACTOR, metallicFactor);
            if (result == aiReturn_SUCCESS)
                QSSGSceneDesc::setProperty(target, "metalness", &QQuick3DPrincipledMaterial::setMetalness, float(metallicFactor));
        }

        {
            ai_real roughnessFactor;
            result = source.Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_ROUGHNESS_FACTOR, roughnessFactor);
            if (result == aiReturn_SUCCESS)
                QSSGSceneDesc::setProperty(target, "roughness", &QQuick3DPrincipledMaterial::setRoughness, float(roughnessFactor));
        }

        if (auto normalTexture = createTextureNode(source, aiTextureType_NORMALS, 0)) {
            QSSGSceneDesc::setProperty(target, "normalMap", &QQuick3DPrincipledMaterial::setNormalMap, normalTexture);
            {
                ai_real normalScale;
                result = source.Get(AI_MATKEY_GLTF_TEXTURE_SCALE(aiTextureType_NORMALS, 0), normalScale);
                if (result == aiReturn_SUCCESS)
                    QSSGSceneDesc::setProperty(target, "normalStrength", &QQuick3DPrincipledMaterial::setNormalStrength, float(normalScale));
            }
        }

        // Occlusion Textures are not implimented (yet)
        if (auto occlusionTexture = createTextureNode(source, aiTextureType_LIGHTMAP, 0)) {
            QSSGSceneDesc::setProperty(target, "occlusionTextureImage", &QQuick3DPrincipledMaterial::setOcclusionMap, occlusionTexture);
            QSSGSceneDesc::setProperty(target, "occlusionChannel", &QQuick3DPrincipledMaterial::setOcclusionChannel, QQuick3DPrincipledMaterial::TextureChannelMapping::R);
            {
                ai_real occlusionAmount;
                result = source.Get(AI_MATKEY_GLTF_TEXTURE_STRENGTH(aiTextureType_LIGHTMAP, 0), occlusionAmount);
                if (result == aiReturn_SUCCESS)
                    QSSGSceneDesc::setProperty(target, "occlusionAmount", &QQuick3DPrincipledMaterial::setOcclusionAmount, float(occlusionAmount));
            }
        }

        if (auto emissiveTexture = createTextureNode(source, aiTextureType_EMISSIVE, 0))
            QSSGSceneDesc::setProperty(target, "emissiveMap", &QQuick3DPrincipledMaterial::setEmissiveMap, emissiveTexture);

        {
            aiColor3D emissiveColorFactor;
            result = source.Get(AI_MATKEY_COLOR_EMISSIVE, emissiveColorFactor);
            if (result == aiReturn_SUCCESS)
                QSSGSceneDesc::setProperty(target, "emissiveColor", &QQuick3DPrincipledMaterial::setEmissiveColor, aiColorToQColor(emissiveColorFactor));
        }

        {
            bool isDoubleSided;
            result = source.Get(AI_MATKEY_TWOSIDED, isDoubleSided);
            if (result == aiReturn_SUCCESS && isDoubleSided)
                QSSGSceneDesc::setProperty(target, "cullMode", &QQuick3DPrincipledMaterial::setCullMode, QQuick3DPrincipledMaterial::CullMode::NoCulling);
        }

        {
            aiString alphaMode;
            result = source.Get(AI_MATKEY_GLTF_ALPHAMODE, alphaMode);
            if (result == aiReturn_SUCCESS) {
                auto mode = QQuick3DPrincipledMaterial::AlphaMode::Default;
                if (QByteArrayView(alphaMode.C_Str()) == "OPAQUE")
                    mode = QQuick3DPrincipledMaterial::AlphaMode::Opaque;
                else if (QByteArrayView(alphaMode.C_Str()) == "MASK")
                    mode = QQuick3DPrincipledMaterial::AlphaMode::Mask;
                else if (QByteArrayView(alphaMode.C_Str()) == "BLEND")
                    mode = QQuick3DPrincipledMaterial::AlphaMode::Blend;

                if (mode != QQuick3DPrincipledMaterial::AlphaMode::Default)
                    QSSGSceneDesc::setProperty(target, "alphaMode", &QQuick3DPrincipledMaterial::setAlphaMode, mode);
            }
        }

        {
            ai_real alphaCutoff;
            result = source.Get(AI_MATKEY_GLTF_ALPHACUTOFF, alphaCutoff);
            if (result == aiReturn_SUCCESS)
                QSSGSceneDesc::setProperty(target, "alphaCutoff", &QQuick3DPrincipledMaterial::setAlphaCutoff, float(alphaCutoff));
        }

        {
            bool isUnlit = false;
            result = source.Get(AI_MATKEY_GLTF_UNLIT, isUnlit);
            if (result == aiReturn_SUCCESS && isUnlit)
                QSSGSceneDesc::setProperty(target, "lighting", &QQuick3DPrincipledMaterial::setLighting, QQuick3DPrincipledMaterial::Lighting::NoLighting);
        }
    }
}

static void setCameraProperties(QSSGSceneDesc::Camera &target, const aiCamera &source, const aiNode &sourceNode)
{
    using namespace QSSGSceneDesc;

    // assimp does not have a camera type but it works for gltf2 format.
    target.runtimeType = (source.mHorizontalFOV == 0.0f) ? Node::RuntimeType::OrthographicCamera
                                                         : Node::RuntimeType::PerspectiveCamera;

    // We assume these default forward and up vectors, so if this isn't
    // the case we have to do additional transform
    aiMatrix4x4 correctionMatrix;
    bool needsCorrection = false;
    if (source.mLookAt != aiVector3D(0, 0, -1)) {
        aiMatrix4x4 lookAtCorrection;
        aiMatrix4x4::FromToMatrix(aiVector3D(0, 0, -1), source.mLookAt, lookAtCorrection);
        correctionMatrix *= lookAtCorrection;
        needsCorrection = true;
    }
    if (source.mUp != aiVector3D(0, 1, 0)) {
        aiMatrix4x4 upCorrection;
        aiMatrix4x4::FromToMatrix(aiVector3D(0, 1, 0), source.mUp, upCorrection);
        correctionMatrix *= upCorrection;
        needsCorrection = true;
    }

    setNodeProperties(target, sourceNode, needsCorrection ? &correctionMatrix : nullptr, true);

    // clipNear and clipFar
    if (target.runtimeType == Node::RuntimeType::PerspectiveCamera) {
        setProperty(target, "clipNear", &QQuick3DPerspectiveCamera::setClipNear, source.mClipPlaneNear);
        setProperty(target, "clipFar", &QQuick3DPerspectiveCamera::setClipFar, source.mClipPlaneFar);
    } else { //OrthographicCamera
        setProperty(target, "clipNear", &QQuick3DOrthographicCamera::setClipNear, source.mClipPlaneNear);
        setProperty(target, "clipFar", &QQuick3DOrthographicCamera::setClipFar, source.mClipPlaneFar);
    }

    if (target.runtimeType == Node::RuntimeType::PerspectiveCamera) {
        // fieldOfView
        // mHorizontalFOV is defined as a half horizontal fov
        // in the assimp header but it seems not half now.
        const float fov = qRadiansToDegrees(source.mHorizontalFOV);
        setProperty(target, "fieldOfView", &QQuick3DPerspectiveCamera::setFieldOfView, fov);

        // isFieldOfViewHorizontal
        setProperty(target, "fieldOfViewOrientation", &QQuick3DPerspectiveCamera::setFieldOfViewOrientation,
                    QQuick3DPerspectiveCamera::FieldOfViewOrientation::Horizontal);
    } else { //OrthographicCamera
        const float width = source.mOrthographicWidth * 2.0f;
        const float height = width / source.mAspect;
        setProperty(target, "horizontalMagnification", &QQuick3DOrthographicCamera::setHorizontalMagnification, width);
        setProperty(target, "verticalMagnification", &QQuick3DOrthographicCamera::setVerticalMagnification, height);
    }
    // projectionMode

    // scaleMode

    // scaleAnchor

    // frustomScaleX

    // frustomScaleY
}

static void setLightProperties(QSSGSceneDesc::Light &target, const aiLight &source, const aiNode &sourceNode)
{
    // We assume that the direction vector for a light is (0, 0, -1)
    // so if the direction vector is non-null, but not (0, 0, -1) we
    // need to correct the translation
    aiMatrix4x4 correctionMatrix;
    bool needsCorrection = false;
    if (source.mDirection != aiVector3D(0, 0, 0)) {
        if (source.mDirection != aiVector3D(0, 0, -1)) {
            aiMatrix4x4::FromToMatrix(aiVector3D(0, 0, -1), source.mDirection, correctionMatrix);
            needsCorrection = true;
        }
    }

    // lightType
    static const auto asQtLightType = [](aiLightSourceType type) {
        switch (type) {
        case aiLightSource_AMBIENT:
            Q_FALLTHROUGH();
        case aiLightSource_DIRECTIONAL:
            return QSSGSceneDesc::Light::RuntimeType::DirectionalLight;
        case aiLightSource_POINT:
            return QSSGSceneDesc::Light::RuntimeType::PointLight;
        case aiLightSource_SPOT:
            return QSSGSceneDesc::Light::RuntimeType::SpotLight;
        default:
            return QSSGSceneDesc::Light::RuntimeType::PointLight;
        }
    };

    target.runtimeType = asQtLightType(source.mType);

    setNodeProperties(target, sourceNode, needsCorrection ? &correctionMatrix : nullptr, true);

    // brightness
    // Assimp has no property related to brightness or intensity.
    // They are multiplied to diffuse, ambient and specular colors.
    // For extracting the property value, we will check the maximum value of them.
    // (In most cases, Assimp uses the same specular values with diffuse values,
    // so we will compare just components of the diffuse and the ambient)
    float brightness = qMax(qMax(1.0f, source.mColorDiffuse.r),
                            qMax(source.mColorDiffuse.g, source.mColorDiffuse.b));

    // ambientColor
    if (source.mType == aiLightSource_AMBIENT) {
        brightness = qMax(qMax(brightness, source.mColorAmbient.r),
                          qMax(source.mColorAmbient.g, source.mColorAmbient.b));

        // We only want ambient light color if it is explicit
        const QColor ambientColor = QColor::fromRgbF(source.mColorAmbient.r / brightness,
                                                     source.mColorAmbient.g / brightness,
                                                     source.mColorAmbient.b / brightness);
        QSSGSceneDesc::setProperty(target, "ambientColor", &QQuick3DAbstractLight::setAmbientColor, ambientColor);
    }

    // diffuseColor
    const QColor diffuseColor = QColor::fromRgbF(source.mColorDiffuse.r / brightness,
                                                 source.mColorDiffuse.g / brightness,
                                                 source.mColorDiffuse.b / brightness);
    QSSGSceneDesc::setProperty(target, "color", &QQuick3DAbstractLight::setColor, diffuseColor);

    // describe brightness here
    QSSGSceneDesc::setProperty(target, "brightness", &QQuick3DAbstractLight::setBrightness, brightness);

    const bool isSpot = (source.mType == aiLightSource_SPOT);
    if (source.mType == aiLightSource_POINT || isSpot) {
        // constantFade
        // Some assets have this constant attenuation value as 0.0f and it makes light attenuation makes infinite at distance 0.
        // In that case, we will use the default constant attenuation, 1.0f.
        const bool hasAttConstant = !qFuzzyIsNull(source.mAttenuationConstant);

        if (isSpot) {
            if (hasAttConstant)
                QSSGSceneDesc::setProperty(target, "constantFade", &QQuick3DSpotLight::setConstantFade, source.mAttenuationConstant);
            QSSGSceneDesc::setProperty(target, "linearFade", &QQuick3DSpotLight::setLinearFade, source.mAttenuationLinear * 100.0f);
            QSSGSceneDesc::setProperty(target, "quadraticFade", &QQuick3DSpotLight::setQuadraticFade, source.mAttenuationQuadratic * 10000.0f);
            QSSGSceneDesc::setProperty(target, "coneAngle", &QQuick3DSpotLight::setConeAngle, qRadiansToDegrees(source.mAngleOuterCone) * 2.0f);
            QSSGSceneDesc::setProperty(target, "innerConeAngle", &QQuick3DSpotLight::setInnerConeAngle, qRadiansToDegrees(source.mAngleInnerCone) * 2.0f);
        } else {
            if (hasAttConstant)
                QSSGSceneDesc::setProperty(target, "constantFade", &QQuick3DPointLight::setConstantFade, source.mAttenuationConstant);
            QSSGSceneDesc::setProperty(target, "linearFade", &QQuick3DPointLight::setLinearFade, source.mAttenuationLinear * 100.0f);
            QSSGSceneDesc::setProperty(target, "quadraticFade", &QQuick3DPointLight::setQuadraticFade, source.mAttenuationQuadratic * 10000.0f);
        }
    }
    // castShadow

    // shadowBias

    // shadowFactor

    // shadowMapResolution

    // shadowMapFar

    // shadowMapFieldOfView

    // shadowFilter
}

static void setModelProperties(QSSGSceneDesc::Model &target, const aiNode &source, const SceneInfo &sceneInfo)
{
    auto &targetScene = target.scene;
    const auto &srcScene = sceneInfo.scene;
    // TODO: Correction and scale
    setNodeProperties(target, source, nullptr, true);

    // source
    // Combine all the meshes referenced by this model into a single MultiMesh file
    // For the morphing, the target mesh must have the same AnimMeshes.
    // It means if only one mesh has a morphing animation, the other sub-meshes will
    // get null target attributes. However this case might not be common.
    // These submeshes will animate with the same morphing weight!
    AssimpUtils::MeshList meshes;
    QVector<const aiMaterial *> materials;

    using It = decltype (source.mNumMeshes);
    for (It i = 0, end = source.mNumMeshes; i != end; ++i) {
        const aiMesh &mesh = *srcScene.mMeshes[source.mMeshes[i]];
        meshes.push_back(&mesh);
        aiMaterial *material = srcScene.mMaterials[mesh.mMaterialIndex];
        materials.push_back(material);
    }

    QString errorString;
    {
        const auto meshSourceName = QSSGQmlUtilities::getMeshSourceName(QString::fromUtf8(source.mName.C_Str()));
        {
            // TODO: There's a bug here when the lightmap generation is enabled...
            auto meshData = AssimpUtils::generateMeshData(srcScene, meshes, {}, false, false, errorString);
            targetScene->meshStorage.push_back(qMakePair(meshSourceName, std::move(meshData)));
        }
        const auto idx = targetScene->meshStorage.size() - 1;
        const auto meshNode = targetScene->create<QSSGSceneDesc::Mesh>(idx);
        QSSGSceneDesc::addNode(target, *meshNode);
        QSSGSceneDesc::setProperty(target, "source", &QQuick3DModel::setSource, QUrl(meshSourceName));
    }

    // materials
    {
        QVarLengthArray<QSSGSceneDesc::Material *> matList;
        for (const auto &sourceMat : qAsConst(materials)) {
            auto targetMat = targetScene->create<QSSGSceneDesc::Material>(QSSGSceneDesc::Material::RuntimeType::PrincipledMaterial);
            QSSGSceneDesc::addNode(target, *targetMat);
            setMaterialProperties(*targetMat, *sourceMat, sceneInfo);
            matList.push_back(targetMat);
        }
        // Note that we use a QVector/QList here instead of a QQmlListProperty, as that would be really inconvenient.
        // Since we don't create any runtime objects at this point, the list also contains the node type that corresponds with the
        // type expected to be in the list (this is ensured at compile-time).
        QSSGSceneDesc::setProperty(target, "materials", &QQuick3DModel::materials, matList);
    }
}

static QSSGSceneDesc::Node *createSceneNode(const NodeInfo &nodeInfo,
                                            const aiNode &srcNode,
                                            QSSGSceneDesc::Node &parent,
                                            const SceneInfo &sceneInfo)
{
    auto &targetScene = parent.scene;
    QSSGSceneDesc::Node *node = nullptr;
    const auto &srcScene = sceneInfo.scene;
    switch (nodeInfo.type) {
    case QSSGSceneDesc::Node::Type::Camera:
    {
        const auto &srcType = *srcScene.mCameras[nodeInfo.index];
        // We set the initial rt-type to 'Custom', but we'll change it when updateing the properties.
        auto targetType = targetScene->create<QSSGSceneDesc::Camera>(QSSGSceneDesc::Node::RuntimeType::CustomCamera);
        QSSGSceneDesc::addNode(parent, *targetType);
        setCameraProperties(*targetType, srcType, srcNode);
        node = targetType;
    }
        break;
    case QSSGSceneDesc::Node::Type::Light:
    {
        const auto &srcType = *srcScene.mLights[nodeInfo.index];
        // Initial type is DirectonalLight, but will be change (if needed) when setting the properties.
        auto targetType = targetScene->create<QSSGSceneDesc::Light>(QSSGSceneDesc::Node::RuntimeType::DirectionalLight);
        QSSGSceneDesc::addNode(parent, *targetType);
        setLightProperties(*targetType, srcType, srcNode);
        node = targetType;
    }
        break;
    case QSSGSceneDesc::Node::Type::Model:
    {
        auto target = targetScene->create<QSSGSceneDesc::Model>();
        QSSGSceneDesc::addNode(parent, *target);
        setModelProperties(*target, srcNode, sceneInfo);
        node = target;
    }
        break;
    case QSSGSceneDesc::Node::Type::Transform:
    { // TODO: Transform nodes (see: containsNodesOfConsequence())
        node = targetScene->create<QSSGSceneDesc::Node>(QSSGSceneDesc::Node::Type::Transform, QSSGSceneDesc::Node::RuntimeType::Node);
        QSSGSceneDesc::addNode(parent, *node);
        // TODO: arguments for correction
        setNodeProperties(*node, srcNode, nullptr, false);
    }
        break;
    default:
        break;
    }

    return node;
}

static void processNode(const SceneInfo &sceneInfo, const aiNode &source, QSSGSceneDesc::Node &parent, const NodeMap &nodeMap)
{
    QSSGSceneDesc::Node *node = nullptr;
    if (source.mNumMeshes != 0) {
        node = createSceneNode(NodeInfo { 0, QSSGSceneDesc::Node::Type::Model }, source, parent, sceneInfo);
    } else {
        auto it = nodeMap.constFind(&source);
        const auto end = nodeMap.constEnd();
        if (it != end)
            node = createSceneNode(*it, *it.key(), parent, sceneInfo);
    }

    if (!node) {
        qWarning("No node created for %s\n", source.mName.C_Str());
        node = &parent;
    }

    Q_ASSERT(node);
    Q_ASSERT(node->scene);

    // Process child nodes
    using It = decltype (source.mNumChildren);
    for (It i = 0, end = source.mNumChildren; i != end; ++i)
        processNode(sceneInfo, **(source.mChildren + i), *node, nodeMap);
}

static QString importImp(const QUrl &url, const QVariantMap &options, QSSGSceneDesc::Scene &targetScene)
{
    Q_UNUSED(options);

    auto filePath = url.toLocalFile();

    auto sourceFile = QFileInfo(filePath);
    if (!sourceFile.exists())
        return QLatin1String("File not found");

    const auto extension = sourceFile.suffix().toLower();

    if (extension != QLatin1String("gltf") && extension != QLatin1String("glb"))
        return QLatin1String("Extension \'%1\' is not supported!").arg(extension);

    std::unique_ptr<Assimp::Importer> importer(new Assimp::Importer());
    // Remove primatives that are not Triangles
    importer->SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_POINT | aiPrimitiveType_LINE);

    // TODO: Right now we don't do any extra processing...
    auto m_postProcessSteps = aiPostProcessSteps(0);

    auto sourceScene = importer->ReadFile(filePath.toStdString(), m_postProcessSteps);
    if (!sourceScene) {
        // Scene failed to load, use logger to get the reason
        return QString::fromLocal8Bit(importer->GetErrorString());
    }

    SceneInfo::GltfVersion gltfVersion = SceneInfo::GltfVersion::Unknown;

    // gltf 1.x version's material will use DefaultMaterial
    int impIndex = importer->GetPropertyInteger("importerIndex");
    if (const aiImporterDesc *impInfo = importer->GetImporterInfo(impIndex)) {
        // The name must be either "glTF Importer" or "glTF2 Importer"
        if (impInfo->mName) {
            // We're only interested in the 5 first letters
            if (qstrnlen(impInfo->mName, 5) > 0) {
                if (qstrncmp(impInfo->mName, "glTF", 4) == 0) {
                    if (impInfo->mName[4] == '2')
                        gltfVersion = SceneInfo::GltfVersion::v2;
                    else
                        gltfVersion = SceneInfo::GltfVersion::v1;
                }
            }
        }
    }

    if (gltfVersion == SceneInfo::GltfVersion::Unknown)
        return QLatin1String("Unknown format version!");

    // Before we can start processing the scene we start my mapping out the nodes
    // we can tell the type of.
    NodeMap nodeMap;
    {
        using It = decltype (sourceScene->mNumMeshes);
        const auto &srcRootNode = *sourceScene->mRootNode;

        if (sourceScene->HasLights()) {
            for (It i = 0, end = sourceScene->mNumLights; i != end; ++i) {
                const auto &type = *sourceScene->mLights[i];
                if (auto node = srcRootNode.FindNode(type.mName))
                    nodeMap[node] = { i, NodeInfo::Type::Light };
            }
        }

        if (sourceScene->HasCameras()) {
            for (It i = 0, end = sourceScene->mNumCameras; i != end; ++i) {
                const auto &srcCam = *sourceScene->mCameras[i];
                if (auto node = srcRootNode.FindNode(srcCam.mName))
                    nodeMap[node] = { i, NodeInfo::Type::Camera };
            }
        }

        // TODO: Bones and animations
    }

    const auto opt = SceneInfo::Options::None;
    SceneInfo sceneInfo { *sourceScene, sourceFile.dir(), gltfVersion, opt };

    if (!targetScene.root) {
        auto root = targetScene.create<QSSGSceneDesc::Node>(QSSGSceneDesc::Node::Type::Transform, QSSGSceneDesc::Node::RuntimeType::Node);
        QSSGSceneDesc::addNode(targetScene, *root);
    }


    // Now lets go through the scene
    if (sourceScene->mRootNode)
        processNode(sceneInfo, *sourceScene->mRootNode, *targetScene.root, nodeMap);

    return QString();
}

////////////////////////

QString AssimpImporter::import(const QUrl &url, const QVariantMap &, QSSGSceneDesc::Scene &scene)
{
    // We'll simply use assimp to load the scene and then translate the Aassimp scene
    // into our own format.
    return importImp(url, {}, scene);
}

static void setProperties(QQuick3DObject &obj, const QSSGSceneDesc::Node &node)
{
    using namespace QSSGSceneDesc;
    const auto &properties = node.properties;
    auto it = properties.begin();
    const auto end = properties.end();
    for (; it != end; ++it) {
        const auto &v = *it;
        if (v.value.mt.id() == qMetaTypeId<Node *>()) {
            if (const auto *node = reinterpret_cast<Node *>(v.value.dptr)) {
                Q_ASSERT(node->obj);
                v.call->set(obj, node->obj);
            }
        } else {
            v.call->set(obj, v.value.dptr);
        }
    }
}

template<typename GraphObjectType, typename NodeType>
GraphObjectType *createRuntimeObject(NodeType &node, QQuick3DObject &parent)
{
    GraphObjectType *obj = qobject_cast<GraphObjectType *>(node.obj);
    if (!obj) {
        node.obj = qobject_cast<QQuick3DObject *>(obj = new GraphObjectType);
        obj->setParent(&parent);
        obj->setParentItem(&parent);
    }

    return obj;
}

static void createGraphObject(QSSGSceneDesc::Node &node, QQuick3DObject &parent, bool traverse = true)
{
    using namespace QSSGSceneDesc;

    QQuick3DObject *obj = nullptr;
    switch (node.nodeType) {
    case QSSGSceneDesc::Node::Type::Skeleton:
        obj = createRuntimeObject<QQuick3DSkeleton>(static_cast<Skeleton &>(node), parent);
        break;
    case QSSGSceneDesc::Node::Type::Joint:
        obj = createRuntimeObject<QQuick3DJoint>(static_cast<Joint &>(node), parent);
        break;
    case QSSGSceneDesc::Node::Type::Light:
    {
        auto &light = static_cast<Light &>(node);
        if (light.runtimeType == Node::RuntimeType::DirectionalLight)
            obj = createRuntimeObject<QQuick3DDirectionalLight>(light, parent);
        else if (light.runtimeType == Node::RuntimeType::PointLight)
            obj = createRuntimeObject<QQuick3DPointLight>(light, parent);
        else if (light.runtimeType == Node::RuntimeType::SpotLight)
            obj = createRuntimeObject<QQuick3DSpotLight>(light, parent);
        else
            Q_UNREACHABLE();
    }
        break;
    case Node::Type::Transform:
        obj = createRuntimeObject<QQuick3DNode>(node, parent);
        break;
    case Node::Type::Camera:
    {
        auto &camera = static_cast<Camera &>(node);
        if (camera.runtimeType == Node::RuntimeType::OrthographicCamera)
            obj = createRuntimeObject<QQuick3DOrthographicCamera>(camera, parent);
        else if (camera.runtimeType == Node::RuntimeType::PerspectiveCamera)
            obj = createRuntimeObject<QQuick3DPerspectiveCamera>(camera, parent);
        else if (camera.runtimeType == Node::RuntimeType::CustomCamera)
            obj = createRuntimeObject<QQuick3DCustomCamera>(camera, parent);
        else
            Q_UNREACHABLE();
    }
        break;
    case Node::Type::Model:
        obj = createRuntimeObject<QQuick3DModel>(static_cast<Model &>(node), parent);
        break;
    case Node::Type::Texture:
        if (node.runtimeType == Node::RuntimeType::TextureData)
            obj = createRuntimeObject<QQuick3DTextureData>(static_cast<TextureData &>(node), parent);
        else
            obj = createRuntimeObject<QQuick3DTexture>(static_cast<Texture &>(node), parent);
        break;
    case Node::Type::Material:
    {
        if (node.runtimeType == Node::RuntimeType::PrincipledMaterial)
            obj = createRuntimeObject<QQuick3DPrincipledMaterial>(static_cast<Material &>(node), parent);
        else if (node.runtimeType == Node::RuntimeType::DefaultMaterial)
            obj = createRuntimeObject<QQuick3DDefaultMaterial>(static_cast<Material &>(node), parent);
        else
            Q_UNREACHABLE();
    }
        break;
    case Node::Type::Mesh:
    {
        // There's no runtime object for this type, but we need to register the mesh with the
        // buffer manager.
        if (Q_LIKELY(node.scene)) {
            const auto &scene = *node.scene;
            auto &meshNode = static_cast<const Mesh &>(node);
            const auto &meshData = scene.meshStorage.at(meshNode.idx);
            const auto &name = meshData.first;
            const auto &mesh = meshData.second;
            QSSGBufferManager::registerMeshData(QSSGRenderPath(name), mesh);
        }
    }
        break;
    }

    if (obj && traverse) {
        setProperties(*obj, node);

        for (auto &chld : node.children)
            createGraphObject(chld, *obj);
    }
}

QQuick3DNode *AssimpImporter::import(QQuick3DNode &parent, const QSSGSceneDesc::Scene &scene)
{
    Q_ASSERT(scene.root);
    Q_ASSERT(QQuick3DObjectPrivate::get(&parent)->sceneManager);

    auto root = scene.root;
    for (const auto &resource : scene.resources)
        createGraphObject(*resource, parent, false);

    createGraphObject(*root, parent);

    return qobject_cast<QQuick3DNode *>(scene.root->obj);
}

QT_END_NAMESPACE
