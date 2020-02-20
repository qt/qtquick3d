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

#include <QtQuick3DRuntimeRender/private/qssgrenderprefiltertexture_p.h>
#include <QtQuick3DRuntimeRender/private/qssgruntimerenderlogging_p.h>

#include <QtQuick/QSGTexture>

#include <QtCore/QDir>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>

#include <QtGui/private/qimage_p.h>
#include <QtQuick/private/qsgtexture_p.h>
#include <QtQuick/private/qsgcompressedtexture_p.h>

QT_BEGIN_NAMESPACE

namespace {

struct PrimitiveEntry
{
    // Name of the primitive as it will be in the UIP file
    const char *primitive;
    // Name of the primitive file on the filesystem
    const char *file;
};

const int nPrimitives = 5;
const PrimitiveEntry primitives[nPrimitives] = {
        {"#Rectangle", "/Rectangle.mesh"},
        {"#Sphere","/Sphere.mesh"},
        {"#Cube","/Cube.mesh"},
        {"#Cone","/Cone.mesh"},
        {"#Cylinder","/Cylinder.mesh"},
};

const char *primitivesDirectory = "res//primitives";

}


QSSGBufferManager::QSSGBufferManager(const QSSGRef<QSSGRenderContext> &ctx,
                                         const QSSGRef<QSSGInputStreamFactory> &inInputStreamFactory,
                                         QSSGPerfTimer *inTimer)
{
    context = ctx;
    inputStreamFactory = inInputStreamFactory;
    perfTimer = inTimer;
    gpuSupportsDXT = ctx->supportsDXTImages();
}

QSSGBufferManager::~QSSGBufferManager()
{ clear(); }

void QSSGBufferManager::setImageHasTransparency(const QString &inImagePath, bool inHasTransparency)
{
    ImageMap::iterator theImage = imageMap.insert(inImagePath, QSSGRenderImageTextureData());
    theImage.value().m_textureFlags.setHasTransparency(inHasTransparency);
}

bool QSSGBufferManager::getImageHasTransparency(const QString &inSourcePath) const
{
    ImageMap::const_iterator theIter = imageMap.find(inSourcePath);
    if (theIter != imageMap.end())
        return theIter.value().m_textureFlags.hasTransparency();
    return false;
}

void QSSGBufferManager::setImageTransparencyToFalseIfNotSet(const QString &inSourcePath)
{
    ImageMap::iterator theImage = imageMap.find(inSourcePath);

    // If we did actually insert something
    if (theImage != imageMap.end())
        theImage.value().m_textureFlags.setHasTransparency(false);
}

void QSSGBufferManager::setInvertImageUVCoords(const QString &inImagePath, bool inShouldInvertCoords)
{
    ImageMap::iterator theImage = imageMap.find(inImagePath);
    if (theImage != imageMap.end())
        theImage.value().m_textureFlags.setInvertUVCoords(inShouldInvertCoords);
}

bool QSSGBufferManager::isImageLoaded(const QString &inSourcePath)
{
    QMutexLocker locker(&loadedImageSetMutex);
    return loadedImageSet.find(inSourcePath) != loadedImageSet.end();
}

bool QSSGBufferManager::aliasImagePath(const QString &inSourcePath,
                                       const QString &inAliasPath,
                                       bool inIgnoreIfLoaded)
{
    if (inSourcePath.isEmpty() || inAliasPath.isEmpty())
        return false;
    // If the image is loaded then we ignore this call in some cases.
    if (inIgnoreIfLoaded && isImageLoaded(inSourcePath))
        return false;
    aliasImageMap.insert(inSourcePath, inAliasPath);
    return true;
}

void QSSGBufferManager::unaliasImagePath(const QString &inSourcePath)
{
    aliasImageMap.remove(inSourcePath);
}

QString QSSGBufferManager::getImagePath(const QString &inSourcePath) const
{
    const auto foundIt = aliasImageMap.constFind(inSourcePath);
    return (foundIt != aliasImageMap.cend()) ? foundIt.value() : inSourcePath;
}

namespace {
QSize sizeForMipLevel(int mipLevel, const QSize &baseLevelSize)
{
    const int w = qMax(1, baseLevelSize.width() >> mipLevel);
    const int h = qMax(1, baseLevelSize.height() >> mipLevel);
    return QSize(w, h);
}
}

static QRhiTexture::Format toRhiFormat(const QSSGRenderTextureFormat format)
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
        return QRhiTexture::RED_OR_ALPHA8; //??
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


// ### This code is copied from  qssgrenderprefiltertexture.cpp
// TODO: don't do that
// TODO: Create mipmaps on the GPU

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

static QShader getMipmapShader()
{
    static bool first = true;
    static QShader theShader;
    if (first) {
        QFile f(QLatin1String(":/res/rhishaders/miprgbe8.comp.qsb"));
        if (f.open(QIODevice::ReadOnly))
            theShader = QShader::fromSerialized(f.readAll());
        else
            qWarning("Could not load miprgbe8 compute shader");
        first = false;
    }
    return theShader;
}

bool QSSGBufferManager::loadRenderImageComputeMipmap(const QSSGLoadedTexture *inLoadedImage, QSSGRenderImageTextureData *outImageData)
{
    static const int MAX_MIP_LEVELS = 20;

    auto *rhi = context->rhiContext()->rhi();
    if (!rhi->isFeatureSupported(QRhi::Compute))
        return false;

    if (inLoadedImage->format.format != QSSGRenderTextureFormat::RGBE8) {
        qWarning() << "Unsupported HDR format";
        return false;
    }

    QSize size(inLoadedImage->width, inLoadedImage->height);
    int mipmaps = rhi->mipLevelsForSize(size);

    if (mipmaps > MAX_MIP_LEVELS) {
        qWarning("Texture too big for GPU compute");
        return false;
    }

    auto computeShader = getMipmapShader();
    if (!computeShader.isValid())
        return false;

    auto rhiCtx = context->rhiContext();

    auto *tex = rhi->newTexture(QRhiTexture::RGBA8, size, 1, QRhiTexture::UsedWithLoadStore | QRhiTexture::MipMapped);
    tex->build();

    QRhiTextureUploadDescription desc{{0, 0, {inLoadedImage->data, int(inLoadedImage->dataSizeInBytes)}}};
    auto *rub = rhi->nextResourceUpdateBatch(); // TODO: collect all image loading for one frame into one update batch?
    rub->uploadTexture(tex, desc);

    int ubufElementSize = rhi->ubufAligned(12);
    const QSSGRhiUniformBufferSetKey ubufKey = { inLoadedImage, nullptr, nullptr, QSSGRhiUniformBufferSetKey::ComputeMipmap };
    QSSGRhiUniformBufferSet &uniformBuffers(rhiCtx->uniformBufferSet(ubufKey));
    QRhiBuffer *&ubuf = uniformBuffers.ubuf;
    int ubufSize = ubufElementSize * mipmaps;
    if (!ubuf) {
        ubuf = rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, ubufSize);
        ubuf->build();
    } else if (ubuf->size() < ubufSize) {
        ubuf->setSize(ubufSize);
        ubuf->build();
    }

    QRhiShaderResourceBindings *computeBindings[MAX_MIP_LEVELS]; // TODO: QVarLengthArray to avoid having a maximum supported size?
    quint32 numWorkGroups[MAX_MIP_LEVELS][3];
    int mipW = size.width() >> 1;
    int mipH = size.height() >> 1;
    for (int level = 1; level < mipmaps; ++level) {
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
    for (int level = 1; level < mipmaps; ++level) {
        const int i = level - 1;
        const int mipW = numWorkGroups[i][0];
        const int mipH = numWorkGroups[i][1];
        QPair<int, quint32> dynamicOffset = { 0, quint32(ubufElementSize * i) };
        cb->setShaderResources(computeBindings[i], 1, &dynamicOffset);
        cb->dispatch(mipW, mipH, 1);
    }

    cb->endComputePass();

    outImageData->m_rhiTexture = tex;
    outImageData->m_mipmaps = mipmaps;

    return true;
}

QSSGRenderImageTextureData QSSGBufferManager::loadRenderImage(const QString &inImagePath, const QSSGRef<QSSGLoadedTexture> &inLoadedImage, bool inForceScanForTransparency, bool inBsdfMipmaps)
{
    //        SStackPerfTimer __perfTimer(perfTimer, "Image Upload");
    {
        QMutexLocker mapLocker(&loadedImageSetMutex);
        loadedImageSet.insert(inImagePath);
    }
    ImageMap::iterator theImage = imageMap.find(inImagePath);
    bool wasInserted = theImage == imageMap.end();
    if (wasInserted)
        theImage = imageMap.insert(inImagePath, QSSGRenderImageTextureData());

    if (context->rhiContext()->isValid()) {
        QVarLengthArray<QRhiTextureUploadEntry, 16> textureUploads;
        int textureSampleCount = 1;
        QRhiTexture::Flags textureFlags;
        int mipmaps = 0;
        const bool checkTransp = (wasInserted == true || inForceScanForTransparency);
        bool hasTransp = false;

        auto *rhi = context->rhiContext()->rhi();
        QRhiTexture::Format rhiFormat = QRhiTexture::UnknownFormat;
        QSize size;
        if (inBsdfMipmaps) {
            if (inLoadedImage->data) {
                if (loadRenderImageComputeMipmap(inLoadedImage.data(), &theImage.value()))
                    return theImage.value();

                size = QSize(inLoadedImage->width, inLoadedImage->height);
                mipmaps = createBsdfMipUpload(&textureUploads, inLoadedImage.data()); // ->data and .data() are of course utterly and completely different...
                textureFlags |= QRhiTexture::Flag::MipMapped;
                rhiFormat = toRhiFormat(inLoadedImage->format.format);
            } else {
                qWarning() << "Compressed bsdf not supported";
                //size = QSize(inLoadedImage->width, inLoadedImage->height);
                //textureDesc << QRhiTextureUploadEntry{0, 0, {inLoadedImage->data, int(inLoadedImage->dataSizeInBytes)}};
                rhiFormat = QRhiTexture::UnknownFormat;
            }
        } else {
            QRhiTextureSubresourceUploadDescription subDesc;
            if (!inLoadedImage->image.isNull()) {
                rhiFormat = toRhiFormat(inLoadedImage->format.format);
                size = inLoadedImage->image.size();
                subDesc.setImage(inLoadedImage->image);
                if (checkTransp)
                    hasTransp = inLoadedImage->image.data_ptr()->checkForAlphaPixels();
            } else if (inLoadedImage->compressedData.isValid()) {
                const QTextureFileData &tex = inLoadedImage->compressedData;
                auto glFormat = tex.glInternalFormat() ? tex.glInternalFormat() : tex.glFormat();
                rhiFormat = toRhiFormat(GLConversion::fromGLtoTextureFormat(glFormat));
                size = tex.size();
                subDesc.setData(tex.data().mid(tex.dataOffset(), tex.dataLength()));
                if (checkTransp)
                    hasTransp = !QSGCompressedTexture::formatIsOpaque(glFormat);
            } else if (inLoadedImage->data) {
                rhiFormat = toRhiFormat(inLoadedImage->format.format);
                size = QSize(inLoadedImage->width, inLoadedImage->height);
                QByteArray buf(static_cast<const char *>(inLoadedImage->data), qMax(0, int(inLoadedImage->dataSizeInBytes)));
                subDesc.setData(buf);
                if (checkTransp)
                    hasTransp = inLoadedImage->scanForTransparency();

            }
            subDesc.setSourceSize(size);
            if (!subDesc.data().isEmpty() || !subDesc.image().isNull())
                textureUploads << QRhiTextureUploadEntry{0, 0, subDesc};
        }

        qDebug() << "Load RHI texture:" << inImagePath << size << inLoadedImage->format.format << rhiFormat << hasTransp;
        if (textureUploads.isEmpty() || size.isEmpty() || rhiFormat == QRhiTexture::UnknownFormat) {
            qWarning() << "Could not load texture from" << inImagePath;
            return QSSGRenderImageTextureData();
        }

        auto *tex = rhi->newTexture(rhiFormat, size, textureSampleCount, textureFlags);
        tex->build();
        qDebug() << inImagePath << size << "format" << inLoadedImage->format.format << "RHI format"  << rhiFormat << " RHI tex" << tex << "levels" << textureUploads.size();

        if (checkTransp)
            theImage.value().m_textureFlags.setHasTransparency(hasTransp);
        theImage.value().m_rhiTexture = tex;

        QRhiTextureUploadDescription uploadDescription;
        uploadDescription.setEntries(textureUploads.cbegin(), textureUploads.cend());
        auto *rub = rhi->nextResourceUpdateBatch(); // TODO: optimize
        rub->uploadTexture(tex, uploadDescription);
        context->rhiContext()->commandBuffer()->resourceUpdate(rub);

        //### TODO: we own this texture, so remember to release it!!!
        theImage.value().m_rhiTexture = tex;
        theImage.value().m_mipmaps = mipmaps;
        return theImage.value();
    }

    // inLoadedImage.EnsureMultiplerOfFour( context->GetFoundation(), inImagePath.c_str() );

    QSSGRef<QSSGRenderTexture2D> theTexture = new QSSGRenderTexture2D(context);
    if (inLoadedImage->data) {
        QSSGRenderTextureFormat destFormat = inLoadedImage->format;
        if (inBsdfMipmaps) {
            if (inLoadedImage->format != QSSGRenderTextureFormat::RGBE8) {
                if (context->renderContextType() == QSSGRenderContextType::GLES2)
                    destFormat = QSSGRenderTextureFormat::RGBA8;
                else
                    destFormat = QSSGRenderTextureFormat::RGBA16F;
            }
        } else {
            theTexture->setTextureData(QSSGByteView((quint8 *)inLoadedImage->data, inLoadedImage->dataSizeInBytes),
                                       0,
                                       inLoadedImage->width,
                                       inLoadedImage->height,
                                       inLoadedImage->format,
                                       destFormat);
        }

        if (inBsdfMipmaps && inLoadedImage->format.isUncompressedTextureFormat()) {
            theTexture->setMinFilter(QSSGRenderTextureMinifyingOp::LinearMipmapLinear);
            QSSGRef<QSSGRenderPrefilterTexture> theBSDFMipMap = theImage.value().m_bsdfMipMap;
            if (theBSDFMipMap == nullptr) {
                theBSDFMipMap = QSSGRenderPrefilterTexture::create(context, inLoadedImage->width, inLoadedImage->height, theTexture, destFormat);
                theImage.value().m_bsdfMipMap = theBSDFMipMap;
            }

            if (theBSDFMipMap) {
                theBSDFMipMap->build(inLoadedImage->data, inLoadedImage->dataSizeInBytes, inLoadedImage->format);
            }
        }
    } else if (inLoadedImage->compressedData.isValid()) {
        // Compressed Texture Image handling using QTextureFileData
        for (int i = 0; i < inLoadedImage->compressedData.numLevels(); i++) {
            QSize imageSize = sizeForMipLevel(i, inLoadedImage->compressedData.size());
            auto format = GLConversion::fromGLtoTextureFormat(inLoadedImage->compressedData.glInternalFormat());
            theTexture->setTextureData(QSSGByteView(reinterpret_cast<quint8 *>(inLoadedImage->compressedData.data().data() + inLoadedImage->compressedData.dataOffset(i)), inLoadedImage->compressedData.dataLength(i)),
                                       i, imageSize.width(), imageSize.height(), format);
        }
    }



    /*else if (inLoadedImage->dds) {
            theImage.first->second.m_Texture = theTexture;
            bool supportsDXT = GPUSupportsDXT;
            bool isDXT = QSSGRenderTextureFormat::isCompressedTextureFormat(inLoadedImage.format);
            bool requiresDecompression = (supportsDXT == false && isDXT) || false;
            // test code for DXT decompression
            // if ( isDXT ) requiresDecompression = true;
            if (requiresDecompression) {
                qCWarning(WARNING, PERF_INFO,
                          "Image %s is DXT format which is unsupported by "
                          "the graphics subsystem, decompressing in CPU",
                          inImagePath.c_str());
            }
            STextureData theDecompressedImage;
            for (int idx = 0; idx < inLoadedImage.dds->numMipmaps; ++idx) {
                if (inLoadedImage.dds->mipwidth[idx] && inLoadedImage.dds->mipheight[idx]) {
                    if (requiresDecompression == false) {
                        theTexture->SetTextureData(
                                    toU8DataRef((char *)inLoadedImage.dds->data[idx],
                                                (quint32)inLoadedImage.dds->size[idx]),
                                    (quint8)idx, (quint32)inLoadedImage.dds->mipwidth[idx],
                                    (quint32)inLoadedImage.dds->mipheight[idx], inLoadedImage.format);
                    } else {
                        theDecompressedImage =
                                inLoadedImage.DecompressDXTImage(idx, &theDecompressedImage);

                        if (theDecompressedImage.data) {
                            theTexture->SetTextureData(
                                        toU8DataRef((char *)theDecompressedImage.data,
                                                    (quint32)theDecompressedImage.dataSizeInBytes),
                                        (quint8)idx, (quint32)inLoadedImage.dds->mipwidth[idx],
                                        (quint32)inLoadedImage.dds->mipheight[idx],
                                        theDecompressedImage.format);
                        }
                    }
                }
            }
            if (theDecompressedImage.data)
                inLoadedImage.ReleaseDecompressedTexture(theDecompressedImage);
        }*/
    if (wasInserted == true || inForceScanForTransparency)
        theImage.value().m_textureFlags.setHasTransparency(inLoadedImage->scanForTransparency());
    theImage.value().m_texture = theTexture;
    return theImage.value();
}

QSSGRenderImageTextureData QSSGBufferManager::loadRenderImage(const QString &inImagePath, const QSSGRenderTextureFormat &inFormat, bool inForceScanForTransparency, bool inBsdfMipmaps)
{
    const QString realImagePath = getImagePath(inImagePath);

    if (Q_UNLIKELY(realImagePath.isNull()))
        return QSSGRenderImageTextureData();

    const auto foundIt = imageMap.constFind(realImagePath);
    if (foundIt != imageMap.cend())
        return foundIt.value();

    if (Q_LIKELY(!realImagePath.isNull())) {
        QSSGRef<QSSGLoadedTexture> theLoadedImage;
        {
            //                SStackPerfTimer __perfTimer(perfTimer, "Image Decompression");
            theLoadedImage = QSSGLoadedTexture::load(realImagePath, inFormat, *inputStreamFactory, true, context->renderContextType());
            // Hackish solution to custom materials not finding their textures if they are used
            // in sub-presentations. Note: Runtime 1 is going to be removed in Qt 3D Studio 2.x,
            // so this should be ok.
            if (!theLoadedImage) {
                if (QDir(realImagePath).isRelative()) {
                    QString searchPath = realImagePath;
                    if (searchPath.startsWith(QLatin1String("./")))
                        searchPath.prepend(QLatin1String("."));
                    int loops = 0;
                    while (!theLoadedImage && ++loops <= 3) {
                        theLoadedImage = QSSGLoadedTexture::load(searchPath,
                                                                   inFormat,
                                                                   *inputStreamFactory,
                                                                   true,
                                                                   context->renderContextType());
                        searchPath.prepend(QLatin1String("../"));
                    }
                } else {
                    // Some textures, for example environment maps for custom materials,
                    // have absolute path at this point. It points to the wrong place with
                    // the new project structure, so we need to split it up and construct
                    // the new absolute path here.
                    const QString &wholePath = realImagePath;
                    QStringList splitPath = wholePath.split(QLatin1String("../"));
                    if (splitPath.size() > 1) {
                        QString searchPath = splitPath.at(0) + splitPath.at(1);
                        int loops = 0;
                        while (!theLoadedImage && ++loops <= 3) {
                            theLoadedImage = QSSGLoadedTexture::load(searchPath,
                                                                       inFormat,
                                                                       *inputStreamFactory,
                                                                       true,
                                                                       context->renderContextType());
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
            return loadRenderImage(realImagePath, theLoadedImage, inForceScanForTransparency, inBsdfMipmaps);

        // We want to make sure that bad path fails once and doesn't fail over and over
        // again
        // which could slow down the system quite a bit.
        imageMap.insert(realImagePath, QSSGRenderImageTextureData());
        qCWarning(WARNING, "Failed to load image: %s", qPrintable(realImagePath));
    }

    return QSSGRenderImageTextureData();
}

QSSGRenderImageTextureData QSSGBufferManager::loadRenderImage(QSGTexture *qsgTexture)
{
    if (Q_UNLIKELY(!qsgTexture))
        return QSSGRenderImageTextureData();
    const bool isRhi = context->rhiContext()->isValid();

    auto theImage = qsgImageMap.find(qsgTexture);
    if (theImage == qsgImageMap.end()) {
        theImage = qsgImageMap.insert(qsgTexture, QSSGRenderImageTextureData());
        if (isRhi) {
            qDebug("===== RHI texture from scenegraph =====");
            QSGTexturePrivate *texPriv = QSGTexturePrivate::get(qsgTexture);
            theImage.value().m_rhiTexture = texPriv->rhiTexture();
        } else {
            QSSGRef<QSSGRenderTexture2D> theTexture = new QSSGRenderTexture2D(context, qsgTexture);
            theImage.value().m_texture = theTexture;
            QObject::connect(qsgTexture, &QObject::destroyed, [this, qsgTexture]() {
                qsgImageMap.remove(qsgTexture);
            });
        }
    } else if (isRhi) {
//        qDebug("TODO: RHI update texture");
//        qDebug() << "          qsg rhi" << QSGTexturePrivate::get(qsgTexture)->rhiTexture();
//        qDebug() << "          ssg rhi" << theImage->m_rhiTexture;
        theImage.value().m_rhiTexture = QSGTexturePrivate::get(qsgTexture)->rhiTexture();
    } else {
        //TODO: make QSSGRenderTexture2D support updating handles instead of this hack
        auto textureId = reinterpret_cast<QSSGRenderBackend::QSSGRenderBackendTextureObject>(quintptr(qsgTexture->textureId()));
        if (theImage.value().m_texture->handle() != textureId) {
            QSSGRef<QSSGRenderTexture2D> theTexture = new QSSGRenderTexture2D(context, qsgTexture);
            theImage.value().m_texture = theTexture;
        }
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

QSSGRenderMesh *QSSGBufferManager::getMesh(const QSSGRenderMeshPath &inSourcePath) const
{
    if (inSourcePath.isNull())
        return nullptr;

    const auto foundIt = meshMap.constFind(inSourcePath);
    return (foundIt != meshMap.constEnd()) ? *foundIt : nullptr;
}

QSSGRenderMesh *QSSGBufferManager::createRenderMesh(
        const QSSGMeshUtilities::MultiLoadResult &result, const QSSGRenderMeshPath &inSourcePath)
{
    QSSGRenderMesh *newMesh = new QSSGRenderMesh(result.m_mesh->m_drawMode,
                                                 result.m_mesh->m_winding,
                                                 result.m_id);
    quint8 *baseAddress = reinterpret_cast<quint8 *>(result.m_mesh);
    meshMap.insert(QSSGRenderMeshPath::create(inSourcePath.path), newMesh);
    QSSGByteView vertexBufferData(result.m_mesh->m_vertexBuffer.m_data.begin(baseAddress),
                                  result.m_mesh->m_vertexBuffer.m_data.size());

    // create a tight packed position data VBO
    // this should improve our depth pre pass rendering
    QVector<QVector3D> posData = createPackedPositionDataArray(result);

    QSSGRenderComponentType indexBufComponentType = QSSGRenderComponentType::Unknown;
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
        QSSGRhiInputAssemblerState iaPoints;
    } rhi;
    struct {
        QSSGRef<QSSGRenderVertexBuffer> vertexBuffer;
        QSSGRef<QSSGRenderVertexBuffer> posVertexBuffer;
        QSSGRef<QSSGRenderIndexBuffer> indexBuffer;
        QSSGRef<QSSGRenderInputAssembler> inputAssembler;
        QSSGRef<QSSGRenderInputAssembler> inputAssemblerDepth;
        QSSGRef<QSSGRenderInputAssembler> inputAssemblerPoints;
    } gl;

    const bool usingRhi = context->rhiContext()->isValid();
    if (usingRhi) {
        newMesh->bufferResourceUpdates = context->rhiContext()->rhi()->nextResourceUpdateBatch();
        rhi.vertexBuffer = new QSSGRhiBuffer(*context->rhiContext().data(),
                                             QRhiBuffer::Static,
                                             QRhiBuffer::VertexBuffer,
                                             result.m_mesh->m_vertexBuffer.m_stride,
                                             vertexBufferData.size());
        newMesh->bufferResourceUpdates->uploadStaticBuffer(rhi.vertexBuffer->buffer(), vertexBufferData);

        if (posData.size()) {
            QSSGByteView posDataView = toByteView(posData);
            rhi.posVertexBuffer = new QSSGRhiBuffer(*context->rhiContext().data(),
                                                    QRhiBuffer::Static,
                                                    QRhiBuffer::VertexBuffer,
                                                    3 * sizeof(float),
                                                    posDataView.size());
            newMesh->bufferResourceUpdates->uploadStaticBuffer(rhi.posVertexBuffer->buffer(), posDataView);
        }

        if (result.m_mesh->m_indexBuffer.m_data.size()) {
            QSSGByteView indexBufferData(result.m_mesh->m_indexBuffer.m_data.begin(baseAddress),
                                         result.m_mesh->m_indexBuffer.m_data.size());
            rhi.indexBuffer = new QSSGRhiBuffer(*context->rhiContext().data(),
                                                QRhiBuffer::Static,
                                                QRhiBuffer::IndexBuffer,
                                                0,
                                                indexBufferData.size(),
                                                rhiIndexFormat);
            newMesh->bufferResourceUpdates->uploadStaticBuffer(rhi.indexBuffer->buffer(), indexBufferData);
        }
    } else {
        gl.vertexBuffer = new QSSGRenderVertexBuffer(context, QSSGRenderBufferUsageType::Static,
                                                     result.m_mesh->m_vertexBuffer.m_stride,
                                                     vertexBufferData);

        if (posData.size()) {
            gl.posVertexBuffer = new QSSGRenderVertexBuffer(context, QSSGRenderBufferUsageType::Static,
                                                            3 * sizeof(float),
                                                            toByteView(posData));
        }

        if (result.m_mesh->m_indexBuffer.m_data.size()) {
            QSSGByteView indexBufferData(result.m_mesh->m_indexBuffer.m_data.begin(baseAddress),
                                         result.m_mesh->m_indexBuffer.m_data.size());
            gl.indexBuffer = new QSSGRenderIndexBuffer(context, QSSGRenderBufferUsageType::Static,
                                                       indexBufComponentType,
                                                       indexBufferData);
        }
    }

    const QSSGMeshUtilities::OffsetDataRef<QSSGMeshUtilities::MeshVertexBufferEntry> &entries
            = result.m_mesh->m_vertexBuffer.m_entries;
    entryBuffer.resize(entries.size());
    for (quint32 entryIdx = 0, entryEnd = entries.size(); entryIdx < entryEnd; ++entryIdx)
        entryBuffer[entryIdx] = entries.index(baseAddress, entryIdx).toVertexBufferEntry(baseAddress);

    if (usingRhi) {
        QVarLengthArray<QRhiVertexInputAttribute, 4> inputAttrs;
        for (quint32 entryIdx = 0, entryEnd = entries.size(); entryIdx < entryEnd; ++entryIdx) {
            const QSSGRenderVertexBufferEntry &vbe(entryBuffer[entryIdx]);
            const int binding = 0;
            const int location = 0; // for now, will be resolved later, hence the separate inputLayoutInputNames list
            const QRhiVertexInputAttribute::Format format = QSSGRhiInputAssemblerState::toVertexInputFormat(
                        vbe.m_componentType, vbe.m_numComponents);
            const int offset = int(vbe.m_firstItemOffset);
            QRhiVertexInputAttribute inputAttr(binding, location, format, offset);

            //qDebug() << "inputAttr" << entryIdx << "binding" << binding << "location" << location << "format" << format << "offset" << offset << vbe.m_name;

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

        rhi.iaPoints.inputLayout.setAttributes({ { 0, 0, QRhiVertexInputAttribute::Float3, 0 } });
        rhi.iaPoints.inputLayout.setBindings({ rhi.posVertexBuffer ? quint32(3 * sizeof(float))
                                               : result.m_mesh->m_vertexBuffer.m_stride });
        rhi.iaPoints.inputLayoutInputNames.append(QByteArrayLiteral("attr_pos"));
        rhi.iaPoints.topology = QRhiGraphicsPipeline::Points;
        rhi.iaPoints.vertexBuffer = rhi.posVertexBuffer ? rhi.posVertexBuffer : rhi.vertexBuffer;
        rhi.iaPoints.indexBuffer = nullptr;

    } else {

        // create our attribute layout
        auto attribLayout = context->createAttributeLayout(toDataView(entryBuffer.constData(), entryBuffer.count()));
        // create our attribute layout for depth pass
        QSSGRenderVertexBufferEntry vertBufferEntries[] = {
            QSSGRenderVertexBufferEntry("attr_pos", QSSGRenderComponentType::Float32, 3),
        };
        auto attribLayoutDepth = context->createAttributeLayout(toDataView(vertBufferEntries, 1));

        // create input assembler object
        quint32 strides = result.m_mesh->m_vertexBuffer.m_stride;
        quint32 offsets = 0;
        gl.inputAssembler = context->createInputAssembler(attribLayout,
                                                          toDataView(&gl.vertexBuffer, 1),
                                                          gl.indexBuffer,
                                                          toDataView(&strides, 1),
                                                          toDataView(&offsets, 1),
                                                          result.m_mesh->m_drawMode);

        // create depth input assembler object
        quint32 posStrides = (gl.posVertexBuffer) ? 3 * sizeof(float) : strides;
        gl.inputAssemblerDepth = context->createInputAssembler(
                    attribLayoutDepth,
                    toDataView((gl.posVertexBuffer) ? &gl.posVertexBuffer : &gl.vertexBuffer, 1),
                    gl.indexBuffer, toDataView(&posStrides, 1), toDataView(&offsets, 1),
                    result.m_mesh->m_drawMode);

        gl.inputAssemblerPoints = context->createInputAssembler(
                    attribLayoutDepth,
                    toDataView((gl.posVertexBuffer) ? &gl.posVertexBuffer : &gl.vertexBuffer, 1),
                    nullptr, toDataView(&posStrides, 1), toDataView(&offsets, 1),
                    QSSGRenderDrawMode::Points);

        if (!gl.inputAssembler || !gl.inputAssemblerDepth || !gl.inputAssemblerPoints) {
            Q_ASSERT(false);
            return nullptr;
        }
    }

    newMesh->joints.resize(result.m_mesh->m_joints.size());
    for (quint32 jointIdx = 0, jointEnd = result.m_mesh->m_joints.size(); jointIdx < jointEnd; ++jointIdx) {
        const QSSGMeshUtilities::Joint &importJoint(result.m_mesh->m_joints.index(baseAddress, jointIdx));
        QSSGRenderJoint &newJoint(newMesh->joints[jointIdx]);
        newJoint.jointID = importJoint.m_jointID;
        newJoint.parentID = importJoint.m_parentID;
        ::memcpy(newJoint.invBindPose, importJoint.m_invBindPose, 16 * sizeof(float));
        ::memcpy(newJoint.localToGlobalBoneSpace, importJoint.m_localToGlobalBoneSpace, 16 * sizeof(float));
    }

    for (quint32 subsetIdx = 0, subsetEnd = result.m_mesh->m_subsets.size(); subsetIdx < subsetEnd; ++subsetIdx) {
        QSSGRenderSubset subset;
        const QSSGMeshUtilities::MeshSubset &source(result.m_mesh->m_subsets.index(baseAddress, subsetIdx));
        subset.bounds = source.m_bounds;
        subset.count = source.m_count;
        subset.offset = source.m_offset;
        subset.joints = newMesh->joints;
        subset.name = QString::fromUtf16(reinterpret_cast<const char16_t *>(source.m_name.begin(baseAddress)));

        if (rhi.vertexBuffer) {
            subset.rhi.vertexBuffer = rhi.vertexBuffer;
            subset.rhi.ia = rhi.ia;
        }
        if (rhi.posVertexBuffer) {
            subset.rhi.posVertexBuffer = rhi.posVertexBuffer;
            subset.rhi.iaDepth = rhi.iaDepth;
            subset.rhi.iaPoints = rhi.iaPoints;
        }
        if (rhi.indexBuffer)
            subset.rhi.indexBuffer = rhi.indexBuffer;

        if (gl.vertexBuffer)
            subset.gl.vertexBuffer = gl.vertexBuffer;
        if (gl.posVertexBuffer)
            subset.gl.posVertexBuffer = gl.posVertexBuffer;
        if (gl.indexBuffer)
            subset.gl.indexBuffer = gl.indexBuffer;
        if (gl.inputAssembler)
            subset.gl.inputAssembler = gl.inputAssembler;
        if (gl.inputAssemblerDepth)
            subset.gl.inputAssemblerDepth = gl.inputAssemblerDepth;
        if (gl.inputAssemblerPoints)
            subset.gl.inputAssemblerPoints = gl.inputAssemblerPoints;

        subset.gl.primitiveType = result.m_mesh->m_drawMode;
        newMesh->subsets.push_back(subset);
    }
    // If we want to, we can an in a quite stupid way break up modes into sub-subsets.
    // These are assumed to use the same material as the outer subset but have fewer tris
    // and should have a more exact bounding box.  This sort of thing helps with using the frustum
    // culling system but it is really done incorrectly.  It should be done via some sort of
    // oct-tree mechanism and it so that the sub-subsets spatially sorted and it should only be done
    // upon save-to-binary with the results saved out to disk.  As you can see, doing it properly
    // requires some real engineering effort so it is somewhat unlikely it will ever happen.
    // Or it could be done on import if someone really wants to change the mesh buffer format.
    // Either way it isn't going to happen here and it isn't going to happen this way but this
    // is a working example of using the technique.
#ifdef QSSG_RENDER_GENERATE_SUB_SUBSETS
    QSSGOption<QSSGRenderVertexBufferEntry> thePosAttrOpt = theVertexBuffer->getEntryByName("attr_pos");
    bool hasPosAttr = thePosAttrOpt.hasValue() && thePosAttrOpt->m_componentType == QSSGRenderComponentTypes::Float32
            && thePosAttrOpt->m_numComponents == 3;

    for (size_t subsetIdx = 0, subsetEnd = theNewMesh->subsets.size(); subsetIdx < subsetEnd; ++subsetIdx) {
        QSSGRenderSubset &theOuterSubset = theNewMesh->subsets[subsetIdx];
        if (theOuterSubset.count && theIndexBuffer
                && theIndexBuffer->getComponentType() == QSSGRenderComponentTypes::UnsignedInteger16
                && theNewMesh->drawMode == QSSGRenderDrawMode::Triangles && hasPosAttr) {
            // Num tris in a sub subset.
            quint32 theSubsetSize = 3334 * 3; // divisible by three.
            size_t theNumSubSubsets = ((theOuterSubset.count - 1) / theSubsetSize) + 1;
            quint32 thePosAttrOffset = thePosAttrOpt->m_firstItemOffset;
            const quint8 *theVertData = theResult.m_mesh->m_vertexBuffer.m_data.begin();
            const quint8 *theIdxData = theResult.m_mesh->m_indexBuffer.m_data.begin();
            quint32 theVertStride = theResult.m_mesh->m_vertexBuffer.m_stride;
            quint32 theOffset = theOuterSubset.offset;
            quint32 theCount = theOuterSubset.count;
            for (size_t subSubsetIdx = 0, subSubsetEnd = theNumSubSubsets; subSubsetIdx < subSubsetEnd; ++subSubsetIdx) {
                QSSGRenderSubsetBase theBase;
                theBase.offset = theOffset;
                theBase.count = NVMin(theSubsetSize, theCount);
                theBase.bounds.setEmpty();
                theCount -= theBase.count;
                theOffset += theBase.count;
                // Create new bounds.
                // Offset is in item size, not bytes.
                const quint16 *theSubsetIdxData = reinterpret_cast<const quint16 *>(theIdxData + theBase.m_Offset * 2);
                for (size_t theIdxIdx = 0, theIdxEnd = theBase.m_Count; theIdxIdx < theIdxEnd; ++theIdxIdx) {
                    quint32 theVertOffset = theSubsetIdxData[theIdxIdx] * theVertStride;
                    theVertOffset += thePosAttrOffset;
                    QVector3D thePos = *(reinterpret_cast<const QVector3D *>(theVertData + theVertOffset));
                    theBase.bounds.include(thePos);
                }
                theOuterSubset.subSubsets.push_back(theBase);
            }
        } else {
            QSSGRenderSubsetBase theBase;
            theBase.bounds = theOuterSubset.bounds;
            theBase.count = theOuterSubset.count;
            theBase.offset = theOuterSubset.offset;
            theOuterSubset.subSubsets.push_back(theBase);
        }
    }
#endif
    return newMesh;
}

QSSGRenderMesh *QSSGBufferManager::loadMesh(const QSSGRenderMeshPath &inMeshPath)
{
    if (inMeshPath.isNull())
        return nullptr;

    // check if it is already loaded
    MeshMap::iterator meshItr = meshMap.find(inMeshPath);
    if (meshItr != meshMap.end())
        return meshItr.value();

    // loading new mesh
    QSSGMeshUtilities::MultiLoadResult result;

    // check to see if this is a primitive mesh
    if (inMeshPath.path.startsWith('#'))
        result = loadPrimitive(inMeshPath.path);

    // Attempt a load from the filesystem if this mesh isn't a primitive.
    if (result.m_mesh == nullptr) {
        QString pathBuilder = inMeshPath.path;
        int poundIndex = pathBuilder.lastIndexOf('#');
        int id = 0;
        if (poundIndex != -1) {
            id = pathBuilder.midRef(poundIndex + 1).toInt();
            pathBuilder = pathBuilder.left(poundIndex);
        }
        QSharedPointer<QIODevice> ioStream(inputStreamFactory->getStreamForFile(pathBuilder));
        if (ioStream)
            result = QSSGMeshUtilities::Mesh::loadMulti(*ioStream, id);
    }

    if (result.m_mesh == nullptr) {
        qCWarning(WARNING, "Failed to load mesh: %s", qPrintable(inMeshPath.path));
        return nullptr;
    }

    auto ret = createRenderMesh(result, inMeshPath);
    ::free(result.m_mesh);
    return ret;
}

QSSGRenderMesh *QSSGBufferManager::loadCustomMesh(const QSSGRenderMeshPath &inSourcePath,
                                                  QSSGMeshUtilities::Mesh *mesh, bool update)
{
    if (!inSourcePath.isNull() && mesh) {
        MeshMap::iterator meshItr = meshMap.find(inSourcePath);
        // Only create the mesh if it doesn't yet exist or update is true
        if (meshItr == meshMap.end() || update) {
            if (meshItr != meshMap.end()) {
                releaseMesh(*meshItr.value());
                meshMap.erase(meshItr);
            }
            QSSGMeshUtilities::MultiLoadResult result;
            result.m_mesh = mesh;
            auto ret = createRenderMesh(result, inSourcePath);
            return ret;
        }
    }
    return nullptr;
}

QSSGRenderMesh *QSSGBufferManager::createMesh(const QString &inSourcePath, quint8 *inVertData, quint32 inNumVerts, quint32 inVertStride, quint32 *inIndexData, quint32 inIndexCount, QSSGBounds3 inBounds)
{
    QString sourcePath = inSourcePath;

    // QPair<QString, SRenderMesh*> thePair(sourcePath, (SRenderMesh*)nullptr);
    // Make sure there isn't already a buffer entry for this mesh.
    const auto meshPath = QSSGRenderMeshPath::create(sourcePath);
    auto it = meshMap.find(meshPath);
    const auto end = meshMap.end();

    QPair<MeshMap::iterator, bool> theMesh;
    if (it != end)
        theMesh = { it, true };
    else
        theMesh = { meshMap.insert(meshPath, nullptr), false };

    if (theMesh.second == true) {
        QSSGRenderMesh *theNewMesh = new QSSGRenderMesh(QSSGRenderDrawMode::Triangles, QSSGRenderWinding::CounterClockwise, 0);

        // If we failed to create the RenderMesh, return a failure.
        if (!theNewMesh) {
            Q_ASSERT(false);
            return nullptr;
        }

        // Get rid of any old mesh that was sitting here and fill it with a new one.
        // NOTE : This is assuming that the source of our mesh data doesn't do its own memory
        // management and always returns new buffer pointers every time.
        // Don't know for sure if that's what we'll get from our intended sources, but that's
        // easily
        // adjustable by looking for matching pointers in the Subsets.
        if (theNewMesh && theMesh.first.value() != nullptr) {
            delete theMesh.first.value();
        }

        theMesh.first.value() = theNewMesh;
        quint32 vertDataSize = inNumVerts * inVertStride;
        Q_ASSERT(vertDataSize <= INT32_MAX); // TODO:
        QSSGByteView theVBufData(inVertData, qint32(vertDataSize));
        // QSSGConstDataRef<quint8> theVBufData( theResult.Mesh->VertexBuffer.Data.begin(
        // baseAddress )
        //		, theResult.Mesh->VertexBuffer.Data.size() );

        QSSGRef<QSSGRenderVertexBuffer> theVertexBuffer = new QSSGRenderVertexBuffer(context, QSSGRenderBufferUsageType::Static,
                                                                                            inVertStride,
                                                                                            theVBufData);
        QSSGRef<QSSGRenderIndexBuffer> theIndexBuffer = nullptr;
        if (inIndexData != nullptr && inIndexCount > 3) {
            const quint32 inSize = inIndexCount * sizeof(quint32);
            Q_ASSERT(inSize <= INT32_MAX);
            Q_ASSERT(*inIndexData <= INT8_MAX);
            QSSGByteView theIBufData(reinterpret_cast<quint8 *>(inIndexData), qint32(inSize));
            theIndexBuffer = new QSSGRenderIndexBuffer(context, QSSGRenderBufferUsageType::Static,
                                                          QSSGRenderComponentType::UnsignedInteger32,
                                                          theIBufData);
        }

        // WARNING
        // Making an assumption here about the contents of the stream
        // PKC TODO : We may have to consider some other format.
        QSSGRenderVertexBufferEntry theEntries[] = {
            QSSGRenderVertexBufferEntry("attr_pos", QSSGRenderComponentType::Float32, 3),
            QSSGRenderVertexBufferEntry("attr_uv", QSSGRenderComponentType::Float32, 2, 12),
            QSSGRenderVertexBufferEntry("attr_norm", QSSGRenderComponentType::Float32, 3, 18),
        };

        // create our attribute layout
        QSSGRef<QSSGRenderAttribLayout> theAttribLayout = context->createAttributeLayout(toDataView(theEntries, 3));
        /*
            // create our attribute layout for depth pass
            QSSGRenderVertexBufferEntry theEntriesDepth[] = {
                    QSSGRenderVertexBufferEntry( "attr_pos",
            QSSGRenderComponentTypes::float, 3 ),
            };
            QSSGRenderAttribLayout* theAttribLayoutDepth = context->CreateAttributeLayout(
            toConstDataRef( theEntriesDepth, 1 ) );
            */
        // create input assembler object
        quint32 strides = inVertStride;
        quint32 offsets = 0;
        QSSGRef<QSSGRenderInputAssembler> theInputAssembler = context->createInputAssembler(theAttribLayout,
                                                                                                  toDataView(&theVertexBuffer, 1),
                                                                                                  theIndexBuffer,
                                                                                                  toDataView(&strides, 1),
                                                                                                  toDataView(&offsets, 1),
                                                                                                  QSSGRenderDrawMode::Triangles);

        if (!theInputAssembler) {
            Q_ASSERT(false);
            return nullptr;
        }

        // Pull out just the mesh object name from the total path
        const QString &fullName = inSourcePath;
        QString subName(inSourcePath);

        int indexOfSub = fullName.lastIndexOf('#');
        if (indexOfSub != -1) {
            subName = fullName.right(indexOfSub + 1);
        }

        theNewMesh->joints.clear();
        QSSGRenderSubset theSubset;
        theSubset.bounds = inBounds;
        theSubset.count = inIndexCount;
        theSubset.offset = 0;
        theSubset.joints = theNewMesh->joints;
        theSubset.name = subName;
        theSubset.gl.vertexBuffer = theVertexBuffer;
        theSubset.gl.posVertexBuffer = nullptr;
        theSubset.gl.indexBuffer = theIndexBuffer;
        theSubset.gl.inputAssembler = theInputAssembler;
        theSubset.gl.inputAssemblerDepth = theInputAssembler;
        theSubset.gl.inputAssemblerPoints = theInputAssembler;
        theSubset.gl.primitiveType = QSSGRenderDrawMode::Triangles;
        theNewMesh->subsets.push_back(theSubset);
    }

    return theMesh.first.value();
}

void QSSGBufferManager::releaseMesh(QSSGRenderMesh &inMesh)
{
    delete &inMesh;
}

void QSSGBufferManager::releaseTexture(QSSGRenderImageTextureData &inEntry)
{
    // TODO:
    Q_UNUSED(inEntry);
    // if (inEntry.Texture)
    //     inEntry.Texture->release();
}

void QSSGBufferManager::clear()
{
    for (auto iter = meshMap.begin(), end = meshMap.end(); iter != end; ++iter) {
        QSSGRenderMesh *theMesh = iter.value();
        if (theMesh)
            QSSGBufferManager::releaseMesh(*theMesh);
    }
    meshMap.clear();
    for (auto iter = imageMap.begin(), end = imageMap.end(); iter != end; ++iter) {
        QSSGRenderImageTextureData &theEntry = iter.value();
        QSSGBufferManager::releaseTexture(theEntry);
    }
    imageMap.clear();
    aliasImageMap.clear();
    {
        QMutexLocker locker(&loadedImageSetMutex);
        loadedImageSet.clear();
    }
}

void QSSGBufferManager::invalidateBuffer(const QString &inSourcePath)
{
    {
        // TODO:
        const auto meshPath = QSSGRenderMeshPath::create(inSourcePath);
        const auto iter = meshMap.constFind(meshPath);
        if (iter != meshMap.cend()) {
            if (iter.value())
                releaseMesh(*iter.value());
            meshMap.erase(iter);
            return;
        }
    }
    {
        ImageMap::iterator iter = imageMap.find(inSourcePath);
        if (iter != imageMap.end()) {
            QSSGRenderImageTextureData &theEntry = iter.value();
            releaseTexture(theEntry);
            imageMap.remove(inSourcePath);
            {
                QMutexLocker locker(&loadedImageSetMutex);
                loadedImageSet.remove(inSourcePath);
            }
        }
    }
}

QT_END_NAMESPACE
