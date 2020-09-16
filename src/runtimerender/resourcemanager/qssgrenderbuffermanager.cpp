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

static QSSGRenderTextureFormat fromGLtoTextureFormat(quint32 internalFormat)
{
    switch (internalFormat) {
    case 0x8229:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::R8);
    case 0x822A:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::R16);
    case 0x822D:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::R16F);
    case 0x8235:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::R32I);
    case 0x8236:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::R32UI);
    case 0x822E:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::R32F);
    case 0x822B:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::RG8);
    case 0x8058:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::RGBA8);
    case 0x8051:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::RGB8);
    case 0x8C41:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::SRGB8);
    case 0x8C43:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::SRGB8A8);
    case 0x8D62:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::RGB565);
    case 0x803C:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::Alpha8);
    case 0x8040:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::Luminance8);
    case 0x8042:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::Luminance16);
    case 0x8045:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::LuminanceAlpha8);
    case 0x881A:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::RGBA16F);
    case 0x822F:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::RG16F);
    case 0x8230:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::RG32F);
    case 0x8815:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::RGB32F);
    case 0x8814:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::RGBA32F);
    case 0x8C3A:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::R11G11B10);
    case 0x8C3D:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::RGB9E5);
    case 0x8059:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::RGB10_A2);
    case 0x881B:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::RGB16F);
    case 0x8D70:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::RGBA32UI);
    case 0x8D71:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::RGB32UI);
    case 0x8D76:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::RGBA16UI);
    case 0x8D77:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::RGB16UI);
    case 0x8D7C:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::RGBA8UI);
    case 0x8D7D:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::RGB8UI);
    case 0x8D82:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::RGBA32I);
    case 0x8D83:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::RGB32I);
    case 0x8D88:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::RGBA16I);
    case 0x8D89:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::RGB16I);
    case 0x8D8E:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::RGBA8I);
    case 0x8D8F:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::RGB8I);
    case 0x83F1:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::RGBA_DXT1);
    case 0x83F0:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::RGB_DXT1);
    case 0x83F2:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::RGBA_DXT3);
    case 0x83F3:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::RGBA_DXT5);
    case 0x9270:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::R11_EAC_UNorm);
    case 0x9271:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::R11_EAC_SNorm);
    case 0x9272:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::RG11_EAC_UNorm);
    case 0x9273:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::RG11_EAC_SNorm);
    case 0x9274:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::RGB8_ETC2);
    case 0x9275:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::SRGB8_ETC2);
    case 0x9276:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::RGB8_PunchThrough_Alpha1_ETC2);
    case 0x9277:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::SRGB8_PunchThrough_Alpha1_ETC2);
    case 0x9278:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::RGBA8_ETC2_EAC);
    case 0x9279:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::SRGB8_Alpha8_ETC2_EAC);
    case 0x93B0:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::RGBA_ASTC_4x4);
    case 0x93B1:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::RGBA_ASTC_5x4);
    case 0x93B2:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::RGBA_ASTC_5x5);
    case 0x93B3:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::RGBA_ASTC_6x5);
    case 0x93B4:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::RGBA_ASTC_6x6);
    case 0x93B5:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::RGBA_ASTC_8x5);
    case 0x93B6:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::RGBA_ASTC_8x6);
    case 0x93B7:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::RGBA_ASTC_8x8);
    case 0x93B8:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::RGBA_ASTC_10x5);
    case 0x93B9:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::RGBA_ASTC_10x6);
    case 0x93BA:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::RGBA_ASTC_10x8);
    case 0x93BB:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::RGBA_ASTC_10x10);
    case 0x93BC:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::RGBA_ASTC_12x10);
    case 0x93BD:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::RGBA_ASTC_12x12);
    case 0x93D0:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::SRGB8_Alpha8_ASTC_4x4);
    case 0x93D1:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::SRGB8_Alpha8_ASTC_5x4);
    case 0x93D2:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::SRGB8_Alpha8_ASTC_5x5);
    case 0x93D3:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::SRGB8_Alpha8_ASTC_6x5);
    case 0x93D4:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::SRGB8_Alpha8_ASTC_6x6);
    case 0x93D5:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::SRGB8_Alpha8_ASTC_8x5);
    case 0x93D6:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::SRGB8_Alpha8_ASTC_8x6);
    case 0x93D7:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::SRGB8_Alpha8_ASTC_8x8);
    case 0x93D8:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::SRGB8_Alpha8_ASTC_10x5);
    case 0x93D9:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::SRGB8_Alpha8_ASTC_10x6);
    case 0x93DA:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::SRGB8_Alpha8_ASTC_10x8);
    case 0x93DB:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::SRGB8_Alpha8_ASTC_10x10);
    case 0x93DC:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::SRGB8_Alpha8_ASTC_12x10);
    case 0x93DD:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::SRGB8_Alpha8_ASTC_12x12);
    case 0x81A5:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::Depth16);
    case 0x81A6:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::Depth24);
    case 0x81A7:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::Depth32);
    case 0x88F0:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::Depth24Stencil8);
    default:
        return QSSGRenderTextureFormat(QSSGRenderTextureFormat::Unknown);
    }
}

static constexpr QSize sizeForMipLevel(int mipLevel, const QSize &baseLevelSize)
{
    return QSize(qMax(1, baseLevelSize.width() >> mipLevel), qMax(1, baseLevelSize.height() >> mipLevel));
}

QSSGBufferManager::QSSGBufferManager(const QSSGRef<QSSGRhiContext> &ctx,
                                     const QSSGRef<QSSGInputStreamFactory> &inInputStreamFactory)
{
    context = ctx;
    inputStreamFactory = inInputStreamFactory;
}

QSSGBufferManager::~QSSGBufferManager()
{ clear(); }

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
    case QSSGRenderTextureFormat::RGBA_DXT1:
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

// CPU fallback for generating BSDF MipLevels
static inline int wrapMod(int a, int base)
{
    return (a >= 0) ? a % base : (a % base) + base;
}

static inline void getWrappedCoords(int &sX, int &sY, int width, int height)
{
    if (sY < 0) {
        sX -= width >> 1;
        sY = -sY;
    }
    if (sY >= height) {
        sX += width >> 1;
        sY = height - sY;
    }
    sX = wrapMod(sX, width);
}

static QSSGTextureData createBsdfMipLevel(QSSGTextureData &preallocData,
                                          QSSGTextureData &inPrevMipLevel,
                                          int width,
                                          int height)
{
    QSSGTextureData retval;
    int newWidth = width >> 1;
    int newHeight = height >> 1;
    newWidth = newWidth >= 1 ? newWidth : 1;
    newHeight = newHeight >= 1 ? newHeight : 1;

    if (preallocData.data) {
        retval = preallocData;
        retval.dataSizeInBytes = newWidth * newHeight * inPrevMipLevel.format.getSizeofFormat();
    } else {
        retval.dataSizeInBytes = newWidth * newHeight * inPrevMipLevel.format.getSizeofFormat();
        retval.format = inPrevMipLevel.format; // inLoadedImage.format;
        retval.data = ::malloc(retval.dataSizeInBytes);
    }

    for (int y = 0; y < newHeight; ++y) {
        for (int x = 0; x < newWidth; ++x) {
            float accumVal[4];
            accumVal[0] = 0;
            accumVal[1] = 0;
            accumVal[2] = 0;
            accumVal[3] = 0;
            for (int sy = -2; sy <= 2; ++sy) {
                for (int sx = -2; sx <= 2; ++sx) {
                    int sampleX = sx + (x << 1);
                    int sampleY = sy + (y << 1);
                    getWrappedCoords(sampleX, sampleY, width, height);

                    // Cauchy filter (this is simply because it's the easiest to evaluate, and
                    // requires no complex
                    // functions).
                    float filterPdf = 1.f / (1.f + float(sx * sx + sy * sy) * 2.f);
                    // With FP HDR formats, we're not worried about intensity loss so much as
                    // unnecessary energy gain,
                    // whereas with LDR formats, the fear with a continuous normalization factor is
                    // that we'd lose
                    // intensity and saturation as well.
                    filterPdf /= (retval.format.getSizeofFormat() >= 8) ? 4.71238898f : 4.5403446f;
                    // filterPdf /= 4.5403446f;        // Discrete normalization factor
                    // filterPdf /= 4.71238898f;        // Continuous normalization factor
                    float curPix[4];
                    qint32 byteOffset = (sampleY * width + sampleX) * retval.format.getSizeofFormat();
                    if (byteOffset < 0) {
                        sampleY = height + sampleY;
                        byteOffset = (sampleY * width + sampleX) * retval.format.getSizeofFormat();
                    }

                    retval.format.decodeToFloat(inPrevMipLevel.data, byteOffset, curPix);

                    accumVal[0] += filterPdf * curPix[0];
                    accumVal[1] += filterPdf * curPix[1];
                    accumVal[2] += filterPdf * curPix[2];
                    accumVal[3] += filterPdf * curPix[3];
                }
            }

            quint32 newIdx = (y * newWidth + x) * retval.format.getSizeofFormat();

            retval.format.encodeToPixel(accumVal, retval.data, newIdx);
        }
    }

    return retval;
}
// End of copied code

using TextureUploads = QVarLengthArray<QRhiTextureUploadEntry, 16>;

static int createBsdfMipUpload(TextureUploads *uploads, const QSSGLoadedTexture *img)
{
    int currentWidth = img->width;
    int currentHeight = img->height;
    int maxDim = qMax(currentWidth, currentHeight);
    int maxMipLevel = int(logf(maxDim) / logf(2.0f));
    QSSGTextureData currentData{img->data, img->dataSizeInBytes, img->format};
    QSSGTextureData nextData;
    QSSGTextureData prevData; //we keep the old buffer around so we don't have to allocate new ones for each level

    for (int i = 0; i <= maxMipLevel; ++i) {
        *uploads << QRhiTextureUploadEntry{0, i, {currentData.data, int(currentData.dataSizeInBytes)}};
        nextData = createBsdfMipLevel(prevData, currentData, currentWidth, currentHeight);
        prevData = (i > 0) ? currentData : QSSGTextureData{}; // don't overwrite the input image
        currentData = nextData;
        currentWidth = qMax(currentWidth / 2, 1);
        currentHeight = qMax(currentHeight / 2, 1);
    }
    ::free(nextData.data);
    ::free(prevData.data);
    return maxMipLevel;
}

static QShader getMipmapShader(const QSSGRenderTextureFormat inFormat)
{
    static QMap<QSSGRenderTextureFormat::Format, QShader> shaderMap;
    const auto foundIt = shaderMap.constFind(inFormat.format);
    if (foundIt != shaderMap.cend())
        return foundIt.value();

    // Load the shader
    QShader theShader;
    QFile f;
    if (inFormat == QSSGRenderTextureFormat::RGBE8) {
        f.setFileName(QLatin1String(":/res/rhishaders/miprgbe8.comp.qsb"));
    } else if (inFormat == QSSGRenderTextureFormat::RGBA32F) {
        f.setFileName(QLatin1String(":/res/rhishaders/miprgba32f.comp.qsb"));
    } else if (inFormat == QSSGRenderTextureFormat::RGBA16F) {
        f.setFileName(QLatin1String(":/res/rhishaders/miprgba16f.comp.qsb"));
    } else {
        qWarning("Unsupported Texture format for compute shader");
        return theShader;
    }

    if (f.open(QIODevice::ReadOnly))
        theShader = QShader::fromSerialized(f.readAll());
    else
        qWarning("Could not load compute shader");

    shaderMap.insert(inFormat.format, theShader);

    return theShader;
}

bool QSSGBufferManager::loadRenderImageComputeMipmap(const QSSGLoadedTexture *inLoadedImage, QSSGRenderImageTextureData *outImageData)
{
    static const int MAX_MIP_LEVELS = 20;

    auto *rhi = context->rhi();
    if (!rhi->isFeatureSupported(QRhi::Compute))
        return false;

    if (!(inLoadedImage->format.format == QSSGRenderTextureFormat::RGBE8 ||
        inLoadedImage->format.format == QSSGRenderTextureFormat::RGBA32F ||
        inLoadedImage->format.format == QSSGRenderTextureFormat::RGBA16F)) {
        qWarning() << "Unsupported HDR format";
        return false;
    }

    QSize size(inLoadedImage->width, inLoadedImage->height);
    int mipmapCount = rhi->mipLevelsForSize(size);

    if (mipmapCount > MAX_MIP_LEVELS) {
        qWarning("Texture too big for GPU compute");
        return false;
    }

    auto computeShader = getMipmapShader(inLoadedImage->format.format);
    if (!computeShader.isValid())
        return false;

    auto rhiCtx = context;

    auto *tex = rhi->newTexture(toRhiFormat(inLoadedImage->format.format), size, 1, QRhiTexture::UsedWithLoadStore | QRhiTexture::MipMapped);
    tex->create();

    QRhiTextureUploadDescription desc{{0, 0, {inLoadedImage->data, int(inLoadedImage->dataSizeInBytes)}}};
    auto *rub = rhi->nextResourceUpdateBatch(); // TODO: collect all image loading for one frame into one update batch?
    rub->uploadTexture(tex, desc);

    int ubufElementSize = rhi->ubufAligned(12);
    const QSSGRhiUniformBufferSetKey ubufKey = { inLoadedImage, nullptr, nullptr, 0, QSSGRhiUniformBufferSetKey::ComputeMipmap };
    QSSGRhiUniformBufferSet &uniformBuffers(rhiCtx->uniformBufferSet(ubufKey));
    QRhiBuffer *&ubuf = uniformBuffers.ubuf;
    int ubufSize = ubufElementSize * mipmapCount;
    if (!ubuf) {
        ubuf = rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, ubufSize);
        ubuf->create();
    } else if (ubuf->size() < ubufSize) {
        ubuf->setSize(ubufSize);
        ubuf->create();
    }

    QRhiShaderResourceBindings *computeBindings[MAX_MIP_LEVELS]; // TODO: QVarLengthArray to avoid having a maximum supported size?
    quint32 numWorkGroups[MAX_MIP_LEVELS][3];
    int mipW = size.width() >> 1;
    int mipH = size.height() >> 1;
    for (int level = 1; level < mipmapCount; ++level) {
        const int i = level - 1;
        numWorkGroups[i][0] = quint32(mipW);
        numWorkGroups[i][1] = quint32(mipH);
        numWorkGroups[i][2] = 0;
        rub->updateDynamicBuffer(ubuf, ubufElementSize * i, 12, numWorkGroups[i]);
        mipW = mipW > 2 ? mipW >> 1 : 1;
        mipH = mipH > 2 ? mipH >> 1 : 1;

        auto *srb = rhiCtx->srb({
                                    QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(0, QRhiShaderResourceBinding::ComputeStage, ubuf, 12),
                                    QRhiShaderResourceBinding::imageLoad(1, QRhiShaderResourceBinding::ComputeStage, tex, level - 1),
                                    QRhiShaderResourceBinding::imageStore(2, QRhiShaderResourceBinding::ComputeStage, tex, level)
                                });

        computeBindings[i] = srb;
    }

    QRhiComputePipeline *computePipeline = rhiCtx->computePipeline({ computeShader,  computeBindings[0] });

    auto *cb = rhiCtx->commandBuffer();
    cb->beginComputePass(rub);

    cb->setComputePipeline(computePipeline);
    for (int level = 1; level < mipmapCount; ++level) {
        const int i = level - 1;
        const int mipW = numWorkGroups[i][0];
        const int mipH = numWorkGroups[i][1];
        QPair<int, quint32> dynamicOffset = { 0, quint32(ubufElementSize * i) };
        cb->setShaderResources(computeBindings[i], 1, &dynamicOffset);
        cb->dispatch(mipW, mipH, 1);
    }

    cb->endComputePass();

    outImageData->m_rhiTexture = tex;
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
    int mipmapCount = 0;
    const bool checkTransp = inForceScanForTransparency;
    bool hasTransp = false;

    auto *rhi = context->rhi();
    QRhiTexture::Format rhiFormat = QRhiTexture::UnknownFormat;
    QSize size;
    if (inMipMode == MipModeBsdf && inTexture->data) {
        if (loadRenderImageComputeMipmap(inTexture, &textureData)) {
            context->registerTexture(textureData.m_rhiTexture); // owned by the QSSGRhiContext from here on
            return true;
        }

        size = QSize(inTexture->width, inTexture->height);
        mipmapCount = createBsdfMipUpload(&textureUploads, inTexture) + 1;
        textureFlags |= QRhiTexture::Flag::MipMapped;
        rhiFormat = toRhiFormat(inTexture->format.format);
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
        auto glFormat = tex.glInternalFormat() ? tex.glInternalFormat() : tex.glFormat();
        rhiFormat = toRhiFormat(fromGLtoTextureFormat(glFormat));
        if (checkTransp)
            hasTransp = !QSGCompressedTexture::formatIsOpaque(glFormat);
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
    if (inMipMode == MipModeGenerated && mipmapCount == 0){
        textureFlags |= QRhiTexture::Flag::UsedWithGenerateMips;
        generateMipmaps = true;
        mipmapCount = rhi->mipLevelsForSize(size);
    }

    if (mipmapCount)
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
    qsgTexture->commitTextureOperations(rhi, rub);
    if (qsgTexture->isAtlasTexture()) {
        // This returns a non-atlased QSGTexture (or does nothing if the
        // extraction has already been done), the ownership of which stays with
        // the atlas. As we do not store (and do not own) qsgTexture below,
        // apart from using it as a cache key and querying its QRhiTexture
        // (which we again do not own), we can just pretend we got the
        // non-atlased QSGTexture in the first place.
        qsgTexture = qsgTexture->removedFromAtlas(rub);
    }
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

QVector<QVector3D> QSSGBufferManager::createPackedPositionDataArray(
        const QSSGMeshUtilities::MultiLoadResult &inResult) const
{
    // we assume a position consists of 3 floats
    const auto mesh = inResult.m_mesh;
    qint32 vertexCount = mesh->m_vertexBuffer.m_data.size() / mesh->m_vertexBuffer.m_stride;
    QVector<QVector3D> positions(vertexCount);
    quint8 *baseOffset = reinterpret_cast<quint8 *>(mesh);

    // copy position data
    float *srcData = reinterpret_cast<float *>(mesh->m_vertexBuffer.m_data.begin(baseOffset));
    quint32 srcStride = mesh->m_vertexBuffer.m_stride / sizeof(float);
    QVector3D *p = positions.data();

    for (qint32 i = 0; i < vertexCount; ++i) {
        p[i] = QVector3D(srcData[0], srcData[1], srcData[2]);
        srcData += srcStride;
    }

    return positions;
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

    // create a tight packed position data VBO
    // this should improve our depth pre pass rendering
    QVector<QVector3D> posData = createPackedPositionDataArray(result);

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
        QSSGRef<QSSGRhiBuffer> posVertexBuffer;
        QSSGRef<QSSGRhiBuffer> indexBuffer;
        QSSGRhiInputAssemblerState ia;
        QSSGRhiInputAssemblerState iaDepth;
    } rhi;


    newMesh->bufferResourceUpdates = context->rhi()->nextResourceUpdateBatch();
    rhi.vertexBuffer = new QSSGRhiBuffer(*context.data(),
                                         QRhiBuffer::Static,
                                         QRhiBuffer::VertexBuffer,
                                         result.m_mesh->m_vertexBuffer.m_stride,
                                         vertexBufferData.size());
    newMesh->bufferResourceUpdates->uploadStaticBuffer(rhi.vertexBuffer->buffer(), vertexBufferData);

    if (posData.size()) {
        QSSGByteView posDataView = toByteView(posData);
        rhi.posVertexBuffer = new QSSGRhiBuffer(*context.data(),
                                                QRhiBuffer::Static,
                                                QRhiBuffer::VertexBuffer,
                                                3 * sizeof(float),
                                                posDataView.size());
        newMesh->bufferResourceUpdates->uploadStaticBuffer(rhi.posVertexBuffer->buffer(), posDataView);
    }

    if (result.m_mesh->m_indexBuffer.m_data.size()) {
        QSSGByteView indexBufferData(result.m_mesh->m_indexBuffer.m_data.begin(baseAddress),
                                     result.m_mesh->m_indexBuffer.m_data.size());
        rhi.indexBuffer = new QSSGRhiBuffer(*context.data(),
                                            QRhiBuffer::Static,
                                            QRhiBuffer::IndexBuffer,
                                            0,
                                            indexBufferData.size(),
                                            rhiIndexFormat);
        newMesh->bufferResourceUpdates->uploadStaticBuffer(rhi.indexBuffer->buffer(), indexBufferData);
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
        QRhiVertexInputAttribute inputAttr(binding, location, format, offset);

        inputAttrs.append(inputAttr);
        rhi.ia.inputLayoutInputNames.append(QByteArray(vbe.m_name));
    }
    rhi.ia.inputLayout.setAttributes(inputAttrs.cbegin(), inputAttrs.cend());
    rhi.ia.inputLayout.setBindings({ result.m_mesh->m_vertexBuffer.m_stride });
    rhi.ia.topology = QSSGRhiInputAssemblerState::toTopology(result.m_mesh->m_drawMode);
    rhi.ia.vertexBuffer = rhi.vertexBuffer;
    rhi.ia.indexBuffer = rhi.indexBuffer;

    rhi.iaDepth.inputLayout.setAttributes({ { 0, 0, QRhiVertexInputAttribute::Float3, 0 } });
    rhi.iaDepth.inputLayout.setBindings({ rhi.posVertexBuffer ? quint32(3 * sizeof(float))
                                          : result.m_mesh->m_vertexBuffer.m_stride });
    rhi.iaDepth.inputLayoutInputNames.append(QByteArrayLiteral("attr_pos"));
    rhi.iaDepth.topology = QSSGRhiInputAssemblerState::toTopology(result.m_mesh->m_drawMode);
    rhi.iaDepth.vertexBuffer = rhi.posVertexBuffer ? rhi.posVertexBuffer : rhi.vertexBuffer;
    rhi.iaDepth.indexBuffer = rhi.indexBuffer;

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
        if (rhi.posVertexBuffer) {
            subset.rhi.posVertexBuffer = rhi.posVertexBuffer;
            subset.rhi.iaDepth = rhi.iaDepth;
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
    context->cleanupUniformBufferSets(model);

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

QT_END_NAMESPACE
