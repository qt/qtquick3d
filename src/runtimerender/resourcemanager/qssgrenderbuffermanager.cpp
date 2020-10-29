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

#include "qssgrenderbuffermanager_p.h"

#include <QtQuick3DRuntimeRender/private/qssgrenderloadedtexture_p.h>

#include <QtQuick3DRuntimeRender/private/qssgruntimerenderlogging_p.h>
#include <QtQuick3DAssetImport/private/qssgmeshbvhbuilder_p.h>
#include <QtQuick3DUtils/private/qssgbounds3_p.h>

#include <QtQuick/QSGTexture>

#include <QtCore/QDir>
#include <QtGui/private/qimage_p.h>
#include <QtQuick/private/qsgtexture_p.h>
#include <QtQuick/private/qsgcompressedtexture_p.h>

#include <QtQuick3DUtils/private/qssgrenderbasetypes_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendergeometry_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendermodel_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderimage_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendertexturedata_p.h>

QT_BEGIN_NAMESPACE

namespace {
struct PrimitiveEntry
{
    // Name of the primitive as it will be in the UIP file
    const char *primitive;
    // Name of the primitive file on the filesystem
    const char *file;
};
}

static const int nPrimitives = 5;
static const PrimitiveEntry primitives[nPrimitives] = {
    {"#Rectangle", "/Rectangle.mesh"},
    {"#Sphere","/Sphere.mesh"},
    {"#Cube","/Cube.mesh"},
    {"#Cone","/Cone.mesh"},
    {"#Cylinder","/Cylinder.mesh"},
};

static const char *primitivesDirectory = "res//primitives";

static constexpr QSize sizeForMipLevel(int mipLevel, const QSize &baseLevelSize)
{
    return QSize(qMax(1, baseLevelSize.width() >> mipLevel), qMax(1, baseLevelSize.height() >> mipLevel));
}

QSSGBufferManager::QSSGBufferManager(const QSSGRef<QSSGRhiContext> &inRenderContext,
                                     const QSSGRef<QSSGShaderCache> &inShaderContext,
                                     const QSSGRef<QSSGInputStreamFactory> &inInputStreamFactory)
{
    context = inRenderContext;
    shaderCache = inShaderContext;
    inputStreamFactory = inInputStreamFactory;
}

QSSGBufferManager::~QSSGBufferManager()
{
    clear();
}

QSSGRenderImageTextureData QSSGBufferManager::loadRenderImage(const QSSGRenderImage *image,
                                                              bool inForceScanForTransparency,
                                                              MipMode inMipMode)
{
    QSSGRenderImageTextureData newImage;
    if (image->m_qsgTexture) {
        newImage = loadRenderImage(image->m_qsgTexture);
    } else if (image->m_rawTextureData) {
        // Textures using QSSGRenderTextureData
        // QSSGRenderImage can override the mipmode for its texture data
        if (inMipMode == MipModeNone && image->m_generateMipmaps)
            inMipMode = MipModeGenerated;
        return image->m_rawTextureData->createOrUpdate(this, inMipMode);
    } else if (!image->m_imagePath.isEmpty()) {
        newImage = loadRenderImage(image->m_imagePath, image->m_format, inForceScanForTransparency, inMipMode);
        // Check if the source path has changed since the last load
        auto imagePathItr = cachedImagePathMap.constFind(image);
        if (imagePathItr != cachedImagePathMap.cend())
            if (!(imagePathItr.value() == image->m_imagePath))
                removeImageReference(imagePathItr.value(), imagePathItr.key());

        addImageReference(image->m_imagePath, image);
    }
    return newImage;
}

QSSGRenderImageTextureData QSSGBufferManager::loadTextureData(QSSGRenderTextureData *data, MipMode inMipMode)
{
    if (Q_UNLIKELY(!data))
        return QSSGRenderImageTextureData();

    auto theImage = customTextureMap.find(data);
    if (theImage == customTextureMap.end()) {
        theImage = customTextureMap.insert(data, QSSGRenderImageTextureData());
    } else {
        // release first
        releaseTextureData(data);
        // reinsert the placeholder since releaseTextureData removed from map
        theImage = customTextureMap.insert(data, QSSGRenderImageTextureData());
    }
    theImage.value() = loadRenderImage(theImage.value(), data, inMipMode);
    return theImage.value();
}

QRhiTexture::Format QSSGBufferManager::toRhiFormat(const QSSGRenderTextureFormat format)
{
    switch (format.format) {

    case QSSGRenderTextureFormat::RGBA8:
        return QRhiTexture::RGBA8;
    case QSSGRenderTextureFormat::R8:
        return QRhiTexture::R8;
    case QSSGRenderTextureFormat::Luminance16: //???
    case QSSGRenderTextureFormat::R16:
        return QRhiTexture::R16;
    case QSSGRenderTextureFormat::LuminanceAlpha8:
    case QSSGRenderTextureFormat::Luminance8:
    case QSSGRenderTextureFormat::Alpha8:
        return QRhiTexture::RED_OR_ALPHA8;
    case QSSGRenderTextureFormat::RGBA16F:
        return QRhiTexture::RGBA16F;
    case QSSGRenderTextureFormat::RGBA32F:
        return QRhiTexture::RGBA32F;
    case QSSGRenderTextureFormat::RGBE8:
        return QRhiTexture::RGBA8;
    case QSSGRenderTextureFormat::RGB_DXT1:
        return QRhiTexture::BC1;
    case QSSGRenderTextureFormat::RGBA_DXT3:
        return QRhiTexture::BC2;
    case QSSGRenderTextureFormat::RGBA_DXT5:
        return QRhiTexture::BC3;
    case QSSGRenderTextureFormat::RGBA8_ETC2_EAC:
        return QRhiTexture::ETC2_RGBA8;
    case QSSGRenderTextureFormat::RGBA_ASTC_4x4:
        return QRhiTexture::ASTC_4x4;
    case QSSGRenderTextureFormat::RGBA_ASTC_5x4:
        return QRhiTexture::ASTC_5x4;
    case QSSGRenderTextureFormat::RGBA_ASTC_5x5:
        return QRhiTexture::ASTC_5x5;
    case QSSGRenderTextureFormat::RGBA_ASTC_6x5:
        return QRhiTexture::ASTC_6x5;
    case QSSGRenderTextureFormat::RGBA_ASTC_6x6:
        return QRhiTexture::ASTC_6x6;
    case QSSGRenderTextureFormat::RGBA_ASTC_8x5:
        return QRhiTexture::ASTC_8x5;
    case QSSGRenderTextureFormat::RGBA_ASTC_8x6:
        return QRhiTexture::ASTC_8x6;
    case QSSGRenderTextureFormat::RGBA_ASTC_8x8:
        return QRhiTexture::ASTC_8x8;
    case QSSGRenderTextureFormat::RGBA_ASTC_10x5:
        return QRhiTexture::ASTC_10x5;
    case QSSGRenderTextureFormat::RGBA_ASTC_10x6:
        return QRhiTexture::ASTC_10x6;
    case QSSGRenderTextureFormat::RGBA_ASTC_10x8:
        return QRhiTexture::ASTC_10x8;
    case QSSGRenderTextureFormat::RGBA_ASTC_10x10:
        return QRhiTexture::ASTC_10x10;
    case QSSGRenderTextureFormat::RGBA_ASTC_12x10:
        return QRhiTexture::ASTC_12x10;
    case QSSGRenderTextureFormat::RGBA_ASTC_12x12:
        return QRhiTexture::ASTC_12x12;


    default:
        qWarning() << "Unsupported texture format" << format.format;
        return QRhiTexture::UnknownFormat;
    }

}

// Vertex data for rendering environment cube map
static const float cube[] = {
    -1.0f,-1.0f,-1.0f,  // -X side
    -1.0f,-1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f,-1.0f,
    -1.0f,-1.0f,-1.0f,

    -1.0f,-1.0f,-1.0f,  // -Z side
    1.0f, 1.0f,-1.0f,
    1.0f,-1.0f,-1.0f,
    -1.0f,-1.0f,-1.0f,
    -1.0f, 1.0f,-1.0f,
    1.0f, 1.0f,-1.0f,

    -1.0f,-1.0f,-1.0f,  // -Y side
    1.0f,-1.0f,-1.0f,
    1.0f,-1.0f, 1.0f,
    -1.0f,-1.0f,-1.0f,
    1.0f,-1.0f, 1.0f,
    -1.0f,-1.0f, 1.0f,

    -1.0f, 1.0f,-1.0f,  // +Y side
    -1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f,-1.0f,
    1.0f, 1.0f, 1.0f,
    1.0f, 1.0f,-1.0f,

    1.0f, 1.0f,-1.0f,  // +X side
    1.0f, 1.0f, 1.0f,
    1.0f,-1.0f, 1.0f,
    1.0f,-1.0f, 1.0f,
    1.0f,-1.0f,-1.0f,
    1.0f, 1.0f,-1.0f,

    -1.0f, 1.0f, 1.0f,  // +Z side
    -1.0f,-1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    -1.0f,-1.0f, 1.0f,
    1.0f,-1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,

    0.0f, 1.0f,  // -X side
    1.0f, 1.0f,
    1.0f, 0.0f,
    1.0f, 0.0f,
    0.0f, 0.0f,
    0.0f, 1.0f,

    1.0f, 1.0f,  // -Z side
    0.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 1.0f,
    1.0f, 0.0f,
    0.0f, 0.0f,

    1.0f, 0.0f,  // -Y side
    1.0f, 1.0f,
    0.0f, 1.0f,
    1.0f, 0.0f,
    0.0f, 1.0f,
    0.0f, 0.0f,

    1.0f, 0.0f,  // +Y side
    0.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 1.0f,

    1.0f, 0.0f,  // +X side
    0.0f, 0.0f,
    0.0f, 1.0f,
    0.0f, 1.0f,
    1.0f, 1.0f,
    1.0f, 0.0f,

    0.0f, 0.0f,  // +Z side
    0.0f, 1.0f,
    1.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 1.0f,
    1.0f, 0.0f,
};

bool QSSGBufferManager::loadRenderImageEnvironmentMap(const QSSGLoadedTexture *inImage, QSSGRenderImageTextureData *outImageData)
{
    // The objective of this method is to take the equirectangular texture
    // provided by inImage and create a cubeMap that contains both pre-filtered
    // specular environment maps, as well as a irradiance map for diffuse
    // operations.
    // To achieve this though we first convert convert the Equirectangular texture
    // to a cubeMap with genereated mip map levels (no filtering) to make the
    // process of creating the prefiltered and irradiance maps eaiser. This
    // intermediate texture as well as the original equirectangular texture are
    // destroyed after this frame completes, and all further associations with
    // the source lightProbe texture are instead associated with the final
    // generated environment map.
    // The intermediate environment cubemap is used to generate the final
    // cubemap. This cubemap will generate 6 mip levels for each face
    // (the remaining faces are unused).  This is what the contents of each
    // face mip level looks like:
    // 0: Pre-filtered with roughness 0 (basically unfiltered)
    // 1: Pre-filtered with roughness 0.25
    // 2: Pre-filtered with roughness 0.5
    // 3: Pre-filtered with roughness 0.75
    // 4: Pre-filtered with rougnness 1.0
    // 5: Irradiance map (ideally at least 16x16)
    // It would be better if we could use a separate cubemap for irradiance, but
    // right now there is a 1:1 association between texture sources on the front-
    // end and backend.

    auto *rhi = context->rhi();
    // Right now minimum face size needs to be 512x512 to be able to have 6 reasonably sized mips
    int suggestedSize = inImage->height * 0.5f;
    suggestedSize = qMax(512, suggestedSize);
    const QSize environmentMapSize(suggestedSize, suggestedSize);
    const bool isRGBE = inImage->format.format == QSSGRenderTextureFormat::Format::RGBE8;
    const QRhiTexture::Format sourceTextureFormat = toRhiFormat(inImage->format.format);
    QRhiTexture::Format cubeTextureFormat = inImage->format.isCompressedTextureFormat()
            ? QRhiTexture::RGBA16F // let's just assume that if compressed textures are available, then it's at least a GLES 3.0 level API
            : sourceTextureFormat;

    // Phase 1: Convert the Equirectangular texture to a Cubemap
    QRhiTexture *envCubeMap = rhi->newTexture(cubeTextureFormat, environmentMapSize, 1,
                                              QRhiTexture::RenderTarget | QRhiTexture::CubeMap | QRhiTexture::MipMapped | QRhiTexture::UsedWithGenerateMips);
    if (!envCubeMap->create()) {
        qWarning("Failed to create Environment Cube Map");
        return false;
    }
    envCubeMap->deleteLater();

    // Create a renderbuffer the size of a the cubeMap face
    QRhiRenderBuffer *envMapRenderBuffer = rhi->newRenderBuffer(QRhiRenderBuffer::Color, environmentMapSize);
    if (!envMapRenderBuffer->create()) {
        qWarning("Failed to create Environment Map Render Buffer");
        return false;
    }
    envMapRenderBuffer->deleteLater();

    // Setup the 6 render targets for each cube face
    QVarLengthArray<QRhiTextureRenderTarget *, 6> renderTargets;
    QRhiRenderPassDescriptor *renderPassDesc = nullptr;
    for (int face = 0; face < 6; ++face) {
        QRhiColorAttachment att(envCubeMap);
        att.setLayer(face);
        QRhiTextureRenderTargetDescription rtDesc;
        rtDesc.setColorAttachments({att});
        auto renderTarget = rhi->newTextureRenderTarget(rtDesc);
        renderTarget->setDescription(rtDesc);
        if (!renderPassDesc)
            renderPassDesc = renderTarget->newCompatibleRenderPassDescriptor();
        renderTarget->setRenderPassDescriptor(renderPassDesc);
        if (!renderTarget->create()) {
            qWarning("Failed to build env map render target");
            return false;
        }
        renderTarget->deleteLater();
        renderTargets << renderTarget;
    }
    renderPassDesc->deleteLater();

    // Setup the sampler for reading the equirectangular loaded texture
    QSize size(inImage->width, inImage->height);
    auto *sourceTexture = rhi->newTexture(sourceTextureFormat, size, 1);
    if (!sourceTexture->create()) {
        qWarning("failed to create source env map texture");
        return false;
    }
    sourceTexture->deleteLater();

    // Upload the equirectangular texture
    QRhiTextureUploadDescription desc;
    if (inImage->compressedData.isValid()) {
        desc = {{ 0, 0, { inImage->compressedData.data().constData() + inImage->compressedData.dataOffset(0),
                          int(inImage->compressedData.dataLength(0)) }
               }};
    } else {
        desc = {{ 0, 0, { inImage->data, int(inImage->dataSizeInBytes) }
               }};
    }
    auto *rub = rhi->nextResourceUpdateBatch();
    rub->uploadTexture(sourceTexture, desc);

    const QSSGRhiSamplerDescription samplerDesc {
        QRhiSampler::Linear,
                QRhiSampler::Linear,
                QRhiSampler::None,
                QRhiSampler::ClampToEdge,
                QRhiSampler::ClampToEdge
    };
    QRhiSampler *sampler = context->sampler(samplerDesc);

    // Load shader and setup render pipeline
    QSSGRef<QSSGRhiShaderPipeline> envMapShaderStages = shaderCache->loadBuiltinForRhi("environmentmap");

    // Vertex Buffer - Just a single cube that will be viewed from inside
    QRhiBuffer *vertexBuffer = rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(cube));
    vertexBuffer->create();
    vertexBuffer->deleteLater();
    rub->uploadStaticBuffer(vertexBuffer, cube);

    // Uniform Buffer - 2x mat4
    int ubufElementSize = rhi->ubufAligned(128);
    QRhiBuffer *uBuf = rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, ubufElementSize * 6);
    uBuf->create();
    uBuf->deleteLater();

    // Shader Resource Bindings
    QRhiShaderResourceBindings *envMapSrb = rhi->newShaderResourceBindings();
    envMapSrb->setBindings({
                         QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(0, QRhiShaderResourceBinding::VertexStage, uBuf, 128),
                         QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, sourceTexture, sampler)
                     });
    envMapSrb->create();
    envMapSrb->deleteLater();

    // Pipeline
    QRhiGraphicsPipeline *envMapPipeline = rhi->newGraphicsPipeline();
    envMapPipeline->setCullMode(QRhiGraphicsPipeline::Front);
    envMapPipeline->setFrontFace(QRhiGraphicsPipeline::CCW);
    envMapPipeline->setShaderStages({
                            *envMapShaderStages->vertexStage(),
                            *envMapShaderStages->fragmentStage()
                        });

    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({
                                { 3 * sizeof(float) }
                            });
    inputLayout.setAttributes({
                                  { 0, 0, QRhiVertexInputAttribute::Float3, 0 }
                              });

    envMapPipeline->setVertexInputLayout(inputLayout);
    envMapPipeline->setShaderResourceBindings(envMapSrb);
    envMapPipeline->setRenderPassDescriptor(renderPassDesc);
    if (!envMapPipeline->create()) {
        qWarning("failed to create source env map pipeline state");
        return false;
    }
    envMapPipeline->deleteLater();

    // Do the actual render passes
    auto *cb = context->commandBuffer();
    cb->debugMarkBegin("Environment Cubemap Generation");
    const QRhiCommandBuffer::VertexInput vbufBinding(vertexBuffer, 0);

    // Set the Uniform Data
    QMatrix4x4 mvp = rhi->clipSpaceCorrMatrix();
    mvp.perspective(90.0f, 1.0f, 0.1f, 10.0f);

    auto lookAt = [](const QVector3D &eye, const QVector3D &center, const QVector3D &up) {
        QMatrix4x4 viewMatrix;
        viewMatrix.lookAt(eye, center, up);
        return viewMatrix;
    };
    QVarLengthArray<QMatrix4x4, 6> views;
    views.append(lookAt(QVector3D(0.0f, 0.0f, 0.0f), QVector3D(1.0, 0.0, 0.0), QVector3D(0.0f, -1.0f, 0.0f)));
    views.append(lookAt(QVector3D(0.0f, 0.0f, 0.0f), QVector3D(-1.0, 0.0, 0.0), QVector3D(0.0f, -1.0f, 0.0f)));
    if (rhi->isYUpInFramebuffer()) {
        views.append(lookAt(QVector3D(0.0f, 0.0f, 0.0f), QVector3D(0.0, 1.0, 0.0), QVector3D(0.0f, 0.0f, 1.0f)));
        views.append(lookAt(QVector3D(0.0f, 0.0f, 0.0f), QVector3D(0.0, -1.0, 0.0), QVector3D(0.0f, 0.0f, -1.0f)));
    } else {
        views.append(lookAt(QVector3D(0.0f, 0.0f, 0.0f), QVector3D(0.0, -1.0, 0.0), QVector3D(0.0f, 0.0f, -1.0f)));
        views.append(lookAt(QVector3D(0.0f, 0.0f, 0.0f), QVector3D(0.0, 1.0, 0.0), QVector3D(0.0f, 0.0f, 1.0f)));
    }
    views.append(lookAt(QVector3D(0.0f, 0.0f, 0.0f), QVector3D(0.0, 0.0, 1.0), QVector3D(0.0f, -1.0f, 0.0f)));
    views.append(lookAt(QVector3D(0.0f, 0.0f, 0.0f), QVector3D(0.0, 0.0, -1.0), QVector3D(0.0f, -1.0f, 0.0f)));
    for (int face = 0; face < 6; ++face) {
        rub->updateDynamicBuffer(uBuf, face * ubufElementSize, 64, mvp.constData());
        rub->updateDynamicBuffer(uBuf, face * ubufElementSize + 64, 64, views[face].constData());
    }
    cb->resourceUpdate(rub);

    for (int face = 0; face < 6; ++face) {
        cb->beginPass(renderTargets[face], QColor(0, 0, 0, 1), { 1.0f, 0 }, nullptr, QSSGRhiContext::commonPassFlags());

        // Execute render pass
        cb->setGraphicsPipeline(envMapPipeline);
        cb->setVertexInput(0, 1, &vbufBinding);
        cb->setViewport(QRhiViewport(0, 0, environmentMapSize.width(), environmentMapSize.height()));
        QPair<int, quint32> dynamicOffset = { 0, quint32(ubufElementSize * face) };
        cb->setShaderResources(envMapSrb, 1, &dynamicOffset);

        cb->draw(36);
        cb->endPass();
    }
    cb->debugMarkEnd();

    if (!isRGBE) {
        // Generate mipmaps for envMap
        rub = rhi->nextResourceUpdateBatch();
        rub->generateMips(envCubeMap);
        cb->resourceUpdate(rub);
    }

    // Phase 2: Generate the pre-filtered environment cubemap
    cb->debugMarkBegin("Pre-filtered Environment Cubemap Generation");
    QRhiTexture *preFilteredEnvCubeMap = rhi->newTexture(cubeTextureFormat, environmentMapSize, 1, QRhiTexture::RenderTarget | QRhiTexture::CubeMap| QRhiTexture::MipMapped);
    if (!preFilteredEnvCubeMap->create())
        qWarning("Failed to create Pre-filtered Environment Cube Map");
    int mipmapCount = rhi->mipLevelsForSize(environmentMapSize);
    mipmapCount = qMin(mipmapCount, 6);  // don't create more than 6 mip levels
    QMap<int, QSize> mipLevelSizes;
    QMap<int, QVarLengthArray<QRhiTextureRenderTarget *, 6>> renderTargetsMap;
    QRhiRenderPassDescriptor *renderPassDescriptorPhase2 = nullptr;

    // Create a renderbuffer for each mip level
    for (int mipLevel = 0; mipLevel < mipmapCount; ++mipLevel) {
        const QSize levelSize = QSize(environmentMapSize.width() * std::pow(0.5, mipLevel),
                                      environmentMapSize.height() * std::pow(0.5, mipLevel));
        mipLevelSizes.insert(mipLevel, levelSize);
        // Setup Render targets (6 * mipmapCount)
        QVarLengthArray<QRhiTextureRenderTarget *, 6> renderTargets;
        for (int face = 0; face < 6; ++face) {
            QRhiColorAttachment att(preFilteredEnvCubeMap);
            att.setLayer(face);
            att.setLevel(mipLevel);
            QRhiTextureRenderTargetDescription rtDesc;
            rtDesc.setColorAttachments({att});
            auto renderTarget = rhi->newTextureRenderTarget(rtDesc);
            renderTarget->setDescription(rtDesc);
            if (!renderPassDescriptorPhase2)
                renderPassDescriptorPhase2 = renderTarget->newCompatibleRenderPassDescriptor();
            renderTarget->setRenderPassDescriptor(renderPassDescriptorPhase2);
            if (!renderTarget->create())
                qWarning("Failed to build prefilter env map render target");
            renderTarget->deleteLater();
            renderTargets << renderTarget;
        }
        renderTargetsMap.insert(mipLevel, renderTargets);
        renderPassDescriptorPhase2->deleteLater();
    }

    // Load the prefilter shader stages
    QSSGRef<QSSGRhiShaderPipeline> prefilterShaderStages;
    if (isRGBE)
        prefilterShaderStages = shaderCache->loadBuiltinForRhi("environmentmapprefilter_rgbe");
    else
        prefilterShaderStages = shaderCache->loadBuiltinForRhi("environmentmapprefilter");

    // Create a new Sampler
    const QSSGRhiSamplerDescription samplerMipMapDesc {
        QRhiSampler::Linear,
                QRhiSampler::Linear,
                QRhiSampler::Linear,
                QRhiSampler::ClampToEdge,
                QRhiSampler::ClampToEdge
    };

    QRhiSampler *envMapCubeSampler = nullptr;
    // Only use mipmap interpoliation if not using RGBE
    if (!isRGBE)
        envMapCubeSampler = context->sampler(samplerMipMapDesc);
    else
        envMapCubeSampler = sampler;

    // Reuse Vertex Buffer from phase 1
    // Reuse UniformBuffer from phase 1 (for vertex shader)

    // UniformBuffer (roughness + resolution)
    int ubufRoughnessElementSize = rhi->ubufAligned(8);
    QRhiBuffer *uBufRoughness = rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, ubufRoughnessElementSize * mipmapCount);
    uBufRoughness->create();
    uBufRoughness->deleteLater();

    // Shader Resource Bindings
    QRhiShaderResourceBindings *preFilterSrb = rhi->newShaderResourceBindings();
    preFilterSrb->setBindings({
                          QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(0, QRhiShaderResourceBinding::VertexStage, uBuf, 128),
                          QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(2, QRhiShaderResourceBinding::FragmentStage, uBufRoughness, 8),
                          QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, envCubeMap, envMapCubeSampler)
                      });
    preFilterSrb->create();
    preFilterSrb->deleteLater();

    // Pipeline
    QRhiGraphicsPipeline *prefilterPipeline = rhi->newGraphicsPipeline();
    prefilterPipeline->setCullMode(QRhiGraphicsPipeline::Front);
    prefilterPipeline->setFrontFace(QRhiGraphicsPipeline::CCW);
    prefilterPipeline->setDepthOp(QRhiGraphicsPipeline::LessOrEqual);
    prefilterPipeline->setShaderStages({
                            *prefilterShaderStages->vertexStage(),
                            *prefilterShaderStages->fragmentStage()
                        });
    // same as phase 1
    prefilterPipeline->setVertexInputLayout(inputLayout);
    prefilterPipeline->setShaderResourceBindings(preFilterSrb);
    prefilterPipeline->setRenderPassDescriptor(renderPassDescriptorPhase2);
    if (!prefilterPipeline->create()) {
        qWarning("failed to create pre-filter env map pipeline state");
        return false;
    }
    prefilterPipeline->deleteLater();


    // Load the prefilter shader stages
    QSSGRef<QSSGRhiShaderPipeline> irradianceShaderStages;
    if (isRGBE)
        irradianceShaderStages = shaderCache->loadBuiltinForRhi("environmentmapirradiance_rgbe");
    else
        irradianceShaderStages = shaderCache->loadBuiltinForRhi("environmentmapirradiance");

    // Setup Irradiance pipline as well

    QRhiGraphicsPipeline *irradiancePipeline = rhi->newGraphicsPipeline();
    irradiancePipeline->setCullMode(QRhiGraphicsPipeline::Front);
    irradiancePipeline->setFrontFace(QRhiGraphicsPipeline::CCW);
    irradiancePipeline->setDepthOp(QRhiGraphicsPipeline::LessOrEqual);
    irradiancePipeline->setShaderStages({
                             *irradianceShaderStages->vertexStage(),
                             *irradianceShaderStages->fragmentStage()
                         });
    QRhiShaderResourceBindings *irradianceSrb = rhi->newShaderResourceBindings();
    irradianceSrb->setBindings({
                          QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(0, QRhiShaderResourceBinding::VertexStage, uBuf, 128),
                          QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, envCubeMap, sampler)
                      });
    irradianceSrb->create();
    irradianceSrb->deleteLater();
    irradiancePipeline->setShaderResourceBindings(irradianceSrb);
    irradiancePipeline->setVertexInputLayout(inputLayout);
    irradiancePipeline->setRenderPassDescriptor(renderPassDescriptorPhase2);
    if (!irradiancePipeline->create()) {
        qWarning("failed to create irradiance env map pipeline state");
        return false;
    }
    irradiancePipeline->deleteLater();

    // Uniform Data
    // set the roughness uniform buffer data
    rub = rhi->nextResourceUpdateBatch();
    for (int mipLevel = 0; mipLevel < mipmapCount - 1; ++mipLevel) {
        Q_ASSERT(mipmapCount - 2);
        const float roughness = float(mipLevel) / float(mipmapCount - 2);
        const float resolution = environmentMapSize.width();
        rub->updateDynamicBuffer(uBufRoughness, mipLevel * ubufRoughnessElementSize, 4, &roughness);
        rub->updateDynamicBuffer(uBufRoughness, mipLevel * ubufRoughnessElementSize + 4, 4, &resolution);
    }
    cb->resourceUpdate(rub);

    // Render
    for (int mipLevel = 0; mipLevel < mipmapCount; ++mipLevel) {
        for (int face = 0; face < 6; ++face) {
            cb->beginPass(renderTargetsMap[mipLevel][face], QColor(0, 0, 0, 1), { 1.0f, 0 }, nullptr, QSSGRhiContext::commonPassFlags());
            if (mipLevel < mipmapCount - 1) {
                // Specular pre-filtered Environment Map levels
                cb->setGraphicsPipeline(prefilterPipeline);
                cb->setVertexInput(0, 1, &vbufBinding);
                cb->setViewport(QRhiViewport(0, 0, mipLevelSizes[mipLevel].width(), mipLevelSizes[mipLevel].height()));
                QVector<QPair<int, quint32>> dynamicOffsets = {
                    { 0, quint32(ubufElementSize * face) },
                    { 2, quint32(ubufRoughnessElementSize * mipLevel) }
                };
                cb->setShaderResources(preFilterSrb, 2, dynamicOffsets.constData());
            } else {
                // Diffuse Irradiance
                cb->setGraphicsPipeline(irradiancePipeline);
                cb->setVertexInput(0, 1, &vbufBinding);
                cb->setViewport(QRhiViewport(0, 0, mipLevelSizes[mipLevel].width(), mipLevelSizes[mipLevel].height()));
                QVector<QPair<int, quint32>> dynamicOffsets = {
                    { 0, quint32(ubufElementSize * face) },
                };
                cb->setShaderResources(irradianceSrb, 1, dynamicOffsets.constData());
            }
            cb->draw(36);
            cb->endPass();
        }
    }
    cb->debugMarkEnd();

    outImageData->m_rhiTexture = preFilteredEnvCubeMap;
    outImageData->m_mipmapCount = mipmapCount;
    return true;
}

QSSGRenderImageTextureData QSSGBufferManager::loadRenderImage(const QSSGRenderPath &inImagePath,
                                                              const QSSGLoadedTexture *inLoadedImage,
                                                              bool inForceScanForTransparency,
                                                              MipMode inMipMode)
{
    ImageMap::iterator theImage = imageMap.find({ inImagePath, inMipMode });
    const bool notFound = theImage == imageMap.end();
    if (notFound)
        theImage = imageMap.insert({ inImagePath, inMipMode }, QSSGRenderImageTextureData());
    const bool checkTransp = (notFound || inForceScanForTransparency);

    if (!loadRenderImage(theImage.value(), inLoadedImage, checkTransp, inMipMode))
        theImage.value() = QSSGRenderImageTextureData();
    return theImage.value();
}

bool QSSGBufferManager::loadRenderImage(QSSGRenderImageTextureData &textureData,
                                        const QSSGLoadedTexture *inTexture,
                                        bool inForceScanForTransparency,
                                        MipMode inMipMode)
{
    QVarLengthArray<QRhiTextureUploadEntry, 16> textureUploads;
    int textureSampleCount = 1;
    QRhiTexture::Flags textureFlags;
    int mipmapCount = 1;
    const bool checkTransp = inForceScanForTransparency;
    bool hasTransp = false;

    auto *rhi = context->rhi();
    QRhiTexture::Format rhiFormat = QRhiTexture::UnknownFormat;
    QSize size;
    if (inTexture->format.format == QSSGRenderTextureFormat::Format::RGBE8)
        textureData.m_textureFlags.setRgbe8(true);
    if (inMipMode == MipModeBsdf && (inTexture->data || inTexture->compressedData.isValid())) {
        if (loadRenderImageEnvironmentMap(inTexture, &textureData)) {
            context->registerTexture(textureData.m_rhiTexture);
            return true;
        }
    } else if (inTexture->compressedData.isValid()) {
        const QTextureFileData &tex = inTexture->compressedData;
        size = tex.size();
        mipmapCount = tex.numLevels();
        for (int i = 0; i < tex.numLevels(); i++) {
            QRhiTextureSubresourceUploadDescription subDesc;
            subDesc.setSourceSize(sizeForMipLevel(i, size));
            subDesc.setData(tex.data().mid(tex.dataOffset(i), tex.dataLength(i)));
            textureUploads << QRhiTextureUploadEntry{ 0, i, subDesc };
        }
        rhiFormat = toRhiFormat(inTexture->format.format);
        if (checkTransp) {
            auto glFormat = tex.glInternalFormat() ? tex.glInternalFormat() : tex.glFormat();
            hasTransp = !QSGCompressedTexture::formatIsOpaque(glFormat);
        }
    } else {
        QRhiTextureSubresourceUploadDescription subDesc;
        if (!inTexture->image.isNull()) {
            rhiFormat = toRhiFormat(inTexture->format.format);
            size = inTexture->image.size();
            subDesc.setImage(inTexture->image);
            if (checkTransp)
                hasTransp = QImageData::get(inTexture->image)->checkForAlphaPixels();
        } else if (inTexture->data) {
            rhiFormat = toRhiFormat(inTexture->format.format);
            size = QSize(inTexture->width, inTexture->height);
            QByteArray buf(static_cast<const char *>(inTexture->data), qMax(0, int(inTexture->dataSizeInBytes)));
            subDesc.setData(buf);
            if (checkTransp)
                hasTransp = inTexture->scanForTransparency();

        }
        subDesc.setSourceSize(size);
        if (!subDesc.data().isEmpty() || !subDesc.image().isNull())
            textureUploads << QRhiTextureUploadEntry{0, 0, subDesc};
    }

    bool generateMipmaps = false;
    if (inMipMode == MipModeGenerated && mipmapCount == 1) {
        textureFlags |= QRhiTexture::Flag::UsedWithGenerateMips;
        generateMipmaps = true;
        mipmapCount = rhi->mipLevelsForSize(size);
    }

    if (mipmapCount > 1)
        textureFlags |= QRhiTexture::Flag::MipMapped;

    if (textureUploads.isEmpty() || size.isEmpty() || rhiFormat == QRhiTexture::UnknownFormat) {
        qWarning() << "Could not load texture";
        return false;
    } else if (!rhi->isTextureFormatSupported(rhiFormat)) {
        qWarning() << "Unsupported texture format";
        return false;
    }

    auto *tex = rhi->newTexture(rhiFormat, size, textureSampleCount, textureFlags);
    tex->create();

    if (checkTransp)
        textureData.m_textureFlags.setHasTransparency(hasTransp);
    textureData.m_rhiTexture = tex;

    QRhiTextureUploadDescription uploadDescription;
    uploadDescription.setEntries(textureUploads.cbegin(), textureUploads.cend());
    auto *rub = rhi->nextResourceUpdateBatch(); // TODO: optimize
    rub->uploadTexture(tex, uploadDescription);
    if (generateMipmaps)
        rub->generateMips(tex);
    context->commandBuffer()->resourceUpdate(rub);

    textureData.m_mipmapCount = mipmapCount;

    context->registerTexture(textureData.m_rhiTexture); // owned by the QSSGRhiContext from here on
    return true;
}

QSSGRenderImageTextureData QSSGBufferManager::loadRenderImage(const QSSGRenderPath &inImagePath,
                                                              const QSSGRenderTextureFormat &inFormat,
                                                              bool inForceScanForTransparency,
                                                              MipMode inMipMode)
{
    if (Q_UNLIKELY(inImagePath.isNull()))
        return QSSGRenderImageTextureData();

    const auto foundIt = imageMap.constFind({ inImagePath, inMipMode });
    if (foundIt != imageMap.cend())
        return foundIt.value();

    if (Q_LIKELY(!inImagePath.isNull())) {
        QScopedPointer<QSSGLoadedTexture> theLoadedImage;
        {
            const auto &path = inImagePath.path();
            theLoadedImage.reset(QSSGLoadedTexture::load(path, inFormat, *inputStreamFactory, true));

            // ### is this nonsense still needed?

            // Hackish solution to custom materials not finding their textures if they are used
            // in sub-presentations. Note: Runtime 1 is going to be removed in Qt 3D Studio 2.x,
            // so this should be ok.
            if (!theLoadedImage) {
                if (QDir(path).isRelative()) {
                    QString searchPath = path;
                    if (searchPath.startsWith(QLatin1String("./")))
                        searchPath.prepend(QLatin1String("."));
                    int loops = 0;
                    while (!theLoadedImage && ++loops <= 3) {
                        theLoadedImage.reset(QSSGLoadedTexture::load(searchPath,
                                                                     inFormat,
                                                                     *inputStreamFactory,
                                                                     true));
                        searchPath.prepend(QLatin1String("../"));
                    }
                } else {
                    // Some textures, for example environment maps for custom materials,
                    // have absolute path at this point. It points to the wrong place with
                    // the new project structure, so we need to split it up and construct
                    // the new absolute path here.
                    QStringList splitPath = path.split(QLatin1String("../"));
                    if (splitPath.size() > 1) {
                        QString searchPath = splitPath.at(0) + splitPath.at(1);
                        int loops = 0;
                        while (!theLoadedImage && ++loops <= 3) {
                            theLoadedImage.reset(QSSGLoadedTexture::load(searchPath,
                                                                         inFormat,
                                                                         *inputStreamFactory,
                                                                         true));
                            searchPath = splitPath.at(0);
                            for (int i = 0; i < loops; i++)
                                searchPath.append(QLatin1String("../"));
                            searchPath.append(splitPath.at(1));
                        }
                    }
                }
            }
        }

        if (Q_LIKELY(theLoadedImage))
            return loadRenderImage(inImagePath, theLoadedImage.data(), inForceScanForTransparency, inMipMode);

        // We want to make sure that bad path fails once and doesn't fail over and over
        // again
        // which could slow down the system quite a bit.
        imageMap.insert({ inImagePath, inMipMode }, QSSGRenderImageTextureData());
        qCWarning(WARNING, "Failed to load image: %s", qPrintable(inImagePath.path()));
    }

    return QSSGRenderImageTextureData();
}

QSSGRenderImageTextureData QSSGBufferManager::loadRenderImage(QSSGRenderImageTextureData &imageData,
                                                              QSSGRenderTextureData *textureData,
                                                              MipMode inMipMode)
{
    QScopedPointer<QSSGLoadedTexture> theLoadedImage;
    if (textureData->textureData().isNull())
        return QSSGRenderImageTextureData();
    theLoadedImage.reset(QSSGLoadedTexture::loadTextureData(textureData));
    theLoadedImage->ownsData = false;

    if (!loadRenderImage(imageData, theLoadedImage.data(), false, inMipMode))
        return QSSGRenderImageTextureData();

    return imageData;
}

QSSGRenderImageTextureData QSSGBufferManager::loadRenderImage(QSGTexture *qsgTexture)
{
    if (Q_UNLIKELY(!qsgTexture))
        return QSSGRenderImageTextureData();

    // A QSGTexture from a textureprovider that is not a QSGDynamicTexture
    // needs to be pushed to get its content updated (or even to create a
    // QRhiTexture in the first place).
    QRhi *rhi = context->rhi();
    QRhiResourceUpdateBatch *rub = rhi->nextResourceUpdateBatch();
    if (qsgTexture->isAtlasTexture()) {
        // This returns a non-atlased QSGTexture (or does nothing if the
        // extraction has already been done), the ownership of which stays with
        // the atlas. As we do not store (and do not own) qsgTexture below,
        // apart from using it as a cache key and querying its QRhiTexture
        // (which we again do not own), we can just pretend we got the
        // non-atlased QSGTexture in the first place.
        qsgTexture = qsgTexture->removedFromAtlas(rub);
    }
    qsgTexture->commitTextureOperations(rhi, rub);
    context->commandBuffer()->resourceUpdate(rub);

    auto theImage = qsgImageMap.find(qsgTexture);
    if (theImage == qsgImageMap.end()) {
        theImage = qsgImageMap.insert(qsgTexture, QSSGRenderImageTextureData());
        theImage.value().m_rhiTexture = qsgTexture->rhiTexture();
    } else {
        theImage.value().m_rhiTexture = qsgTexture->rhiTexture();
    }

    return theImage.value();
}

QSSGMeshUtilities::MultiLoadResult QSSGBufferManager::loadPrimitive(const QString &inRelativePath) const
{
    QByteArray theName = inRelativePath.toUtf8();
    for (size_t idx = 0; idx < nPrimitives; ++idx) {
        if (primitives[idx].primitive == theName) {
            QString pathBuilder = QString::fromLatin1(primitivesDirectory);
            pathBuilder += QLatin1String(primitives[idx].file);
            quint32 id = 1;
            QSharedPointer<QIODevice> theInStream(inputStreamFactory->getStreamForFile(pathBuilder));
            if (theInStream)
                return QSSGMeshUtilities::Mesh::loadMulti(*theInStream, id);

            qCCritical(INTERNAL_ERROR, "Unable to find mesh primitive %s", qPrintable(pathBuilder));
            return QSSGMeshUtilities::MultiLoadResult();
        }
    }
    return QSSGMeshUtilities::MultiLoadResult();
}

QSSGRenderMesh *QSSGBufferManager::getMesh(const QSSGRenderPath &inSourcePath) const
{
    if (inSourcePath.isNull())
        return nullptr;

    const auto foundIt = meshMap.constFind(inSourcePath);
    return (foundIt != meshMap.constEnd()) ? *foundIt : nullptr;
}

QSSGRenderMesh *QSSGBufferManager::getMesh(QSSGRenderGeometry *geometry) const
{
    if (!geometry)
        return nullptr;
    const auto foundIt = customMeshMap.constFind(geometry);
    return (foundIt != customMeshMap.constEnd()) ? *foundIt : nullptr;
}

QSSGRenderMesh *QSSGBufferManager::loadMesh(const QSSGRenderModel *model)
{
    QSSGRenderMesh *theMesh = nullptr;
    if (model->meshPath.isNull() && model->geometry)
        theMesh = model->geometry->createOrUpdate(this);
    else {
        theMesh = loadMesh(model->meshPath);
        auto meshPathItr = cachedModelPathMap.constFind(model);
        if (meshPathItr != cachedModelPathMap.cend())
            if (!(meshPathItr.value() == model->meshPath))
                removeMeshReference(meshPathItr.value(), meshPathItr.key());
        addMeshReference(model->meshPath, model);
    }
    return theMesh;
}

QSSGRenderMesh *QSSGBufferManager::createRenderMesh(const QSSGMeshUtilities::MultiLoadResult &result)
{
    QSSGRenderMesh *newMesh = new QSSGRenderMesh(result.m_mesh->m_drawMode,
                                                 result.m_mesh->m_winding,
                                                 result.m_id);
    quint8 *baseAddress = reinterpret_cast<quint8 *>(result.m_mesh);
    QSSGByteView vertexBufferData(result.m_mesh->m_vertexBuffer.m_data.begin(baseAddress),
                                  result.m_mesh->m_vertexBuffer.m_data.size());

    QSSGRenderComponentType indexBufComponentType = QSSGRenderComponentType::UnsignedInteger16;
    QRhiCommandBuffer::IndexFormat rhiIndexFormat = QRhiCommandBuffer::IndexUInt16;
    if (result.m_mesh->m_indexBuffer.m_data.size()) {
        indexBufComponentType = result.m_mesh->m_indexBuffer.m_componentType;
        const quint32 sizeofType = getSizeOfType(indexBufComponentType);
        if (sizeofType == 2 || sizeofType == 4) {
            // Ensure type is unsigned; else things will fail in rendering pipeline.
            if (indexBufComponentType == QSSGRenderComponentType::Integer16)
                indexBufComponentType = QSSGRenderComponentType::UnsignedInteger16;
            if (indexBufComponentType == QSSGRenderComponentType::Integer32)
                indexBufComponentType = QSSGRenderComponentType::UnsignedInteger32;
            rhiIndexFormat = indexBufComponentType == QSSGRenderComponentType::UnsignedInteger32
                    ? QRhiCommandBuffer::IndexUInt32 : QRhiCommandBuffer::IndexUInt16;
        } else {
            Q_ASSERT(false);
        }
    }

    struct {
        QSSGRef<QSSGRhiBuffer> vertexBuffer;
        QSSGRef<QSSGRhiBuffer> indexBuffer;
        QSSGRhiInputAssemblerState ia;
    } rhi;

    QRhiResourceUpdateBatch *rub = meshBufferUpdateBatch();
    rhi.vertexBuffer = new QSSGRhiBuffer(*context.data(),
                                         QRhiBuffer::Static,
                                         QRhiBuffer::VertexBuffer,
                                         result.m_mesh->m_vertexBuffer.m_stride,
                                         vertexBufferData.size());
    rub->uploadStaticBuffer(rhi.vertexBuffer->buffer(), vertexBufferData);

    if (result.m_mesh->m_indexBuffer.m_data.size()) {
        QSSGByteView indexBufferData(result.m_mesh->m_indexBuffer.m_data.begin(baseAddress),
                                     result.m_mesh->m_indexBuffer.m_data.size());
        rhi.indexBuffer = new QSSGRhiBuffer(*context.data(),
                                            QRhiBuffer::Static,
                                            QRhiBuffer::IndexBuffer,
                                            0,
                                            indexBufferData.size(),
                                            rhiIndexFormat);
        rub->uploadStaticBuffer(rhi.indexBuffer->buffer(), indexBufferData);
    }

    const QSSGMeshUtilities::OffsetDataRef<QSSGMeshUtilities::MeshVertexBufferEntry> &entries
            = result.m_mesh->m_vertexBuffer.m_entries;
    entryBuffer.resize(entries.size());
    for (quint32 entryIdx = 0, entryEnd = entries.size(); entryIdx < entryEnd; ++entryIdx)
        entryBuffer[entryIdx] = entries.index(baseAddress, entryIdx).toVertexBufferEntry(baseAddress);


    QVarLengthArray<QRhiVertexInputAttribute, 4> inputAttrs;
    for (quint32 entryIdx = 0, entryEnd = entries.size(); entryIdx < entryEnd; ++entryIdx) {
        const QSSGRenderVertexBufferEntry &vbe(entryBuffer[entryIdx]);
        const int binding = 0;
        const int location = 0; // for now, will be resolved later, hence the separate inputLayoutInputNames list
        const QRhiVertexInputAttribute::Format format = QSSGRhiInputAssemblerState::toVertexInputFormat(
                    vbe.m_componentType, vbe.m_numComponents);
        const int offset = int(vbe.m_firstItemOffset);

        bool ok = true;
        if (!strcmp(vbe.m_name, QSSGMeshUtilities::Mesh::getPositionAttrName())) {
            rhi.ia.inputs << QSSGRhiInputAssemblerState::PositionSemantic;
        } else if (!strcmp(vbe.m_name, QSSGMeshUtilities::Mesh::getNormalAttrName())) {
            rhi.ia.inputs << QSSGRhiInputAssemblerState::NormalSemantic;
        } else if (!strcmp(vbe.m_name, QSSGMeshUtilities::Mesh::getUVAttrName())) {
            rhi.ia.inputs << QSSGRhiInputAssemblerState::TexCoord0Semantic;
        } else if (!strcmp(vbe.m_name, QSSGMeshUtilities::Mesh::getUV2AttrName())) {
            rhi.ia.inputs << QSSGRhiInputAssemblerState::TexCoord1Semantic;
        } else if (!strcmp(vbe.m_name, QSSGMeshUtilities::Mesh::getTexTanAttrName())) {
            rhi.ia.inputs << QSSGRhiInputAssemblerState::TangentSemantic;
        } else if (!strcmp(vbe.m_name, QSSGMeshUtilities::Mesh::getTexBinormalAttrName())) {
            rhi.ia.inputs << QSSGRhiInputAssemblerState::BinormalSemantic;
        } else if (!strcmp(vbe.m_name, QSSGMeshUtilities::Mesh::getColorAttrName())) {
            rhi.ia.inputs << QSSGRhiInputAssemblerState::ColorSemantic;
        } else if (!strcmp(vbe.m_name, QSSGMeshUtilities::Mesh::getJointAttrName())) {
            rhi.ia.inputs << QSSGRhiInputAssemblerState::JointSemantic;
        } else if (!strcmp(vbe.m_name, QSSGMeshUtilities::Mesh::getWeightAttrName())) {
            rhi.ia.inputs << QSSGRhiInputAssemblerState::WeightSemantic;
        } else {
            qWarning("Unknown vertex input %s in mesh", vbe.m_name);
            ok = false;
        }
        if (ok) {
            QRhiVertexInputAttribute inputAttr(binding, location, format, offset);
            inputAttrs.append(inputAttr);
        }
    }
    rhi.ia.inputLayout.setAttributes(inputAttrs.cbegin(), inputAttrs.cend());
    rhi.ia.inputLayout.setBindings({ result.m_mesh->m_vertexBuffer.m_stride });
    rhi.ia.topology = QSSGRhiInputAssemblerState::toTopology(result.m_mesh->m_drawMode);
    rhi.ia.vertexBuffer = rhi.vertexBuffer;
    rhi.ia.indexBuffer = rhi.indexBuffer;

    for (quint32 subsetIdx = 0, subsetEnd = result.m_mesh->m_subsets.size(); subsetIdx < subsetEnd; ++subsetIdx) {
        QSSGRenderSubset subset;
        const QSSGMeshUtilities::MeshSubset &source(result.m_mesh->m_subsets.index(baseAddress, subsetIdx));
        subset.bounds = source.m_bounds;
        subset.bvhRoot = nullptr;
        subset.count = source.m_count;
        subset.offset = source.m_offset;
        subset.name = QString::fromUtf16(reinterpret_cast<const char16_t *>(source.m_name.begin(baseAddress)));

        if (rhi.vertexBuffer) {
            subset.rhi.vertexBuffer = rhi.vertexBuffer;
            subset.rhi.ia = rhi.ia;
        }
        if (rhi.indexBuffer)
            subset.rhi.indexBuffer = rhi.indexBuffer;

        newMesh->subsets.push_back(subset);
    }
    return newMesh;
}

void QSSGBufferManager::releaseGeometry(QSSGRenderGeometry *geometry)
{
    const auto meshItr = customMeshMap.constFind(geometry);
    if (meshItr != customMeshMap.cend()) {
        delete meshItr.value();
        customMeshMap.erase(meshItr);
    }
}

void QSSGBufferManager::releaseTextureData(QSSGRenderTextureData *textureData)
{
    const auto textureDataItr = customTextureMap.constFind(textureData);
    if (textureDataItr != customTextureMap.cend()) {
        auto rhiTexture = textureDataItr.value().m_rhiTexture;
        if (rhiTexture)
            context->releaseTexture(rhiTexture);
        customTextureMap.erase(textureDataItr);
    }
}

void QSSGBufferManager::releaseMesh(const QSSGRenderPath &inSourcePath)
{

    const auto meshItr = meshMap.constFind(inSourcePath);
    if (meshItr != meshMap.cend()) {
        delete meshItr.value();
        meshMap.erase(meshItr);
    }
}

void QSSGBufferManager::releaseImage(const ImageCacheKey &key)
{
    const auto imageItr = imageMap.constFind(key);
    if (imageItr != imageMap.cend()) {
        auto rhiTexture = imageItr.value().m_rhiTexture;
        if (rhiTexture)
            context->releaseTexture(rhiTexture);
        imageMap.erase(imageItr);
    }
}

void QSSGBufferManager::releaseImage(const QSSGRenderPath &sourcePath)
{
    for (auto it = imageMap.begin(); it != imageMap.end(); ) {
        if (it.key().path == sourcePath) {
            auto rhiTexture = it.value().m_rhiTexture;
            if (rhiTexture)
                context->releaseTexture(rhiTexture);
            it = imageMap.erase(it);
        } else {
            ++it;
        }
    }
}

void QSSGBufferManager::addMeshReference(const QSSGRenderPath &sourcePath, const QSSGRenderModel *model)
{
    auto meshItr = modelRefMap.find(sourcePath);
    if (meshItr == modelRefMap.cend()) {
        modelRefMap.insert(sourcePath, {model});
    } else {
        meshItr.value().insert(model);
    }
    cachedModelPathMap.insert(model, sourcePath);
}

void QSSGBufferManager::addImageReference(const QSSGRenderPath &sourcePath, const QSSGRenderImage *image)
{
    auto imageItr = imageRefMap.find(sourcePath);
    if (imageItr == imageRefMap.cend())
        imageRefMap.insert(sourcePath, {image});
    else
        imageItr.value().insert(image);
    cachedImagePathMap.insert(image, sourcePath);
}

void QSSGBufferManager::removeMeshReference(const QSSGRenderPath &sourcePath, const QSSGRenderModel *model)
{
    auto meshItr = modelRefMap.find(sourcePath);
    if (meshItr != modelRefMap.cend()) {
        meshItr.value().remove(model);
    }
    // Remove UniformBufferSets associated with the model
    context->cleanupDrawCallData(model);

    cachedModelPathMap.remove(model);
}

void QSSGBufferManager::removeImageReference(const QSSGRenderPath &sourcePath, const QSSGRenderImage *image)
{
    auto imageItr = imageRefMap.find(sourcePath);
    if (imageItr != imageRefMap.cend())
        imageItr.value().remove(image);
    cachedImagePathMap.remove(image);
}

void QSSGBufferManager::cleanupUnreferencedBuffers()
{
    // Release all images who are not referenced
    for (const auto &imagePath : imageRefMap.keys()) {
        if (imageRefMap[imagePath].count() > 0)
            continue;
        releaseImage(imagePath);
        imageRefMap.remove(imagePath);
    }

    // Release all meshes who are not referenced
    for (const auto &meshPath : modelRefMap.keys()) {
        if (modelRefMap[meshPath].count() > 0)
            continue;
        releaseMesh(meshPath);
        modelRefMap.remove(meshPath);
    }
}

QSSGRenderMesh *QSSGBufferManager::loadMesh(const QSSGRenderPath &inMeshPath)
{
    if (inMeshPath.isNull())
        return nullptr;

    // check if it is already loaded
    const auto meshItr = meshMap.constFind(inMeshPath);
    if (meshItr != meshMap.cend())
        return meshItr.value();

    // loading new mesh
    QSSGMeshUtilities::MultiLoadResult result = loadMeshData(inMeshPath);

    if (result.m_mesh == nullptr) {
        qCWarning(WARNING, "Failed to load mesh: %s", qPrintable(inMeshPath.path()));
        return nullptr;
    }

    auto ret = createRenderMesh(result);
    meshMap.insert(inMeshPath, ret);

    ::free(result.m_mesh);
    return ret;
}

QSSGRenderMesh *QSSGBufferManager::loadCustomMesh(QSSGRenderGeometry *geometry,
                                                  QSSGMeshUtilities::Mesh *mesh,
                                                  bool update)
{
    if (geometry && mesh) {
        CustomMeshMap::iterator meshItr = customMeshMap.find(geometry);
        // Only create the mesh if it doesn't yet exist or update is true
        if (meshItr == customMeshMap.end() || update) {
            if (meshItr != customMeshMap.end()) {
                delete meshItr.value();
                customMeshMap.erase(meshItr);
            }
            QSSGMeshUtilities::MultiLoadResult result;
            result.m_mesh = mesh;
            auto ret = createRenderMesh(result);
            customMeshMap.insert(geometry, ret);
            return ret;
        }
    }
    return nullptr;
}

QSSGMeshBVH *QSSGBufferManager::loadMeshBVH(const QSSGRenderPath &inSourcePath)
{
    // loading new mesh
    QSSGMeshUtilities::MultiLoadResult result = loadMeshData(inSourcePath);

    if (result.m_mesh == nullptr) {
        qCWarning(WARNING, "Failed to load mesh: %s", qPrintable(inSourcePath.path()));
        return nullptr;
    }

    // Build BVH for Mesh
    QSSGMeshBVHBuilder meshBVHBuilder(result.m_mesh);
    auto bvh = meshBVHBuilder.buildTree();

    ::free(result.m_mesh);
    return bvh;
}

QSSGMeshUtilities::MultiLoadResult QSSGBufferManager::loadMeshData(const QSSGRenderPath &inMeshPath) const
{
    // loading new mesh
    QSSGMeshUtilities::MultiLoadResult result;

    // check to see if this is a primitive mesh
    if (inMeshPath.path().startsWith(QChar::fromLatin1('#')))
        result = loadPrimitive(inMeshPath.path());

    // Attempt a load from the filesystem if this mesh isn't a primitive.
    if (result.m_mesh == nullptr) {
        QString pathBuilder = inMeshPath.path();
        int poundIndex = pathBuilder.lastIndexOf(QChar::fromLatin1('#'));
        int id = 0;
        if (poundIndex != -1) {
            id = QStringView(pathBuilder).mid(poundIndex + 1).toInt();
            pathBuilder = pathBuilder.left(poundIndex);
        }
        if (!pathBuilder.isEmpty()) {
            QSharedPointer<QIODevice> ioStream(inputStreamFactory->getStreamForFile(pathBuilder));
            if (ioStream)
                result = QSSGMeshUtilities::Mesh::loadMulti(*ioStream, id);
        }
    }
    return result;
}

void QSSGBufferManager::clear()
{
    if (meshBufferUpdates) {
        meshBufferUpdates->release();
        meshBufferUpdates = nullptr;
    }

    for (auto iter = meshMap.begin(), end = meshMap.end(); iter != end; ++iter) {
        QSSGRenderMesh *theMesh = iter.value();
        if (theMesh)
            delete theMesh;
    }
    meshMap.clear();
    for (auto iter = customMeshMap.begin(), end = customMeshMap.end(); iter != end; ++iter) {
        QSSGRenderMesh *theMesh = iter.value();
        if (theMesh)
            delete theMesh;
    }
    customMeshMap.clear();
    for (auto iter = imageMap.begin(), end = imageMap.end(); iter != end; ++iter) {
        releaseImage(iter.key());
    }
    imageMap.clear();

    modelRefMap.clear();
    cachedModelPathMap.clear();
    imageRefMap.clear();
    cachedImagePathMap.clear();
}

QRhiResourceUpdateBatch *QSSGBufferManager::meshBufferUpdateBatch()
{
    if (!meshBufferUpdates)
        meshBufferUpdates = context->rhi()->nextResourceUpdateBatch();
    return meshBufferUpdates;
}

void QSSGBufferManager::commitBufferResourceUpdates()
{
    if (meshBufferUpdates) {
        context->commandBuffer()->resourceUpdate(meshBufferUpdates);
        meshBufferUpdates = nullptr;
    }
}

QT_END_NAMESPACE
