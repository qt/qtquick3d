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

#include <QtQuick3DRuntimeRender/private/qssgrenderloadedtexture_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendererutil_p.h>
#include <QtQuick3DRuntimeRender/private/qssgruntimerenderlogging_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendertexturedata_p.h>
#include <QtGui/QImageReader>
#include <QtMath>

#include <QtQuick3DUtils/private/qssgutils_p.h>

#include <private/qtexturefilereader_p.h>

QT_BEGIN_NAMESPACE


QSharedPointer<QIODevice> QSSGInputUtil::getStreamForFile(const QString &inPath, bool inQuiet, QString *outPath)
{
    QFile *file = nullptr;
    QString tryPath = inPath.startsWith(QLatin1String("qrc:/")) ? inPath.mid(3) : inPath;
    QFileInfo fi(tryPath);
    bool found = fi.exists();
    if (!found && fi.isNativePath()) {
        tryPath.prepend(QLatin1String(":/"));
        fi.setFile(tryPath);
        found = fi.exists();
    }
    if (found) {
        QString filePath = fi.canonicalFilePath();
        file = new QFile(filePath);
        if (file->open(QIODevice::ReadOnly)) {
            if (outPath)
                *outPath = filePath;
        } else {
            delete file;
            file = nullptr;
        }
    }
    if (!file && !inQuiet)
        qCWarning(WARNING, "Failed to find file: %s", qPrintable(inPath));
    return QSharedPointer<QIODevice>(file);
}

QSharedPointer<QIODevice> QSSGInputUtil::getStreamForTextureFile(const QString &inPath, bool inQuiet,
                                                                 QString *outPath, FileType *outFileType)
{
    static const QList<QByteArray> hdrFormats = QList<QByteArray>({ "hdr" });
    static const QList<QByteArray> textureFormats = QTextureFileReader::supportedFileFormats();
    static const QList<QByteArray> imageFormats = QImageReader::supportedImageFormats();
    static const QList<QByteArray> allFormats = textureFormats + hdrFormats + imageFormats;

    QString filePath;
    QByteArray ext;
    QSharedPointer<QIODevice> stream = getStreamForFile(inPath, true, &filePath);
    if (stream) {
        ext = QFileInfo(filePath).suffix().toLatin1().toLower();
    } else {
        for (const QByteArray &format : allFormats) {
            QString tryName = inPath + QLatin1Char('.') + QLatin1String(format);
            stream = getStreamForFile(tryName, true, &filePath);
            if (stream) {
                ext = format;
                break;
            }
        }
    }
    if (stream) {
        if (outPath)
            *outPath = filePath;
        if (outFileType) {
            FileType type = UnknownFile;
            if (hdrFormats.contains(ext))
                type = HdrFile;
            else if (textureFormats.contains(ext))
                type = TextureFile;
            else if (imageFormats.contains(ext))
                type = ImageFile;
            *outFileType = type;
        }
    } else if (!inQuiet) {
        qCWarning(WARNING, "Failed to find texture file for: %s", qPrintable(inPath));
    }
    return stream;
}

static inline QSSGRenderTextureFormat fromGLtoTextureFormat(quint32 internalFormat)
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

QSSGLoadedTexture *QSSGLoadedTexture::loadQImage(const QString &inPath, qint32 flipVertical)
{
    QImage image(inPath);
    if (image.isNull())
        return nullptr;
    const QPixelFormat pixFormat = image.pixelFormat();
    QImage::Format targetFormat = QImage::Format_RGBA8888_Premultiplied;
    if (image.colorCount()) // a palleted image
        targetFormat = QImage::Format_RGBA8888;
    else if (pixFormat.channelCount() == 1)
        targetFormat = QImage::Format_Grayscale8;
    else if (pixFormat.alphaUsage() == QPixelFormat::IgnoresAlpha)
        targetFormat = QImage::Format_RGBX8888;
    else if (pixFormat.premultiplied() == QPixelFormat::NotPremultiplied)
        targetFormat = QImage::Format_RGBA8888;

    image.convertTo(targetFormat); // convert to a format mappable to QRhiTexture::Format
    if (flipVertical)
        image.mirror(); // Flip vertically to the conventional Y-up orientation

    QSSGLoadedTexture *retval = new QSSGLoadedTexture;
    retval->width = image.width();
    retval->height = image.height();
    retval->components = image.pixelFormat().channelCount();
    retval->image = image;
    retval->data = (void *)retval->image.bits();
    retval->dataSizeInBytes = image.sizeInBytes();
    retval->setFormatFromComponents();

    return retval;
}

QSSGLoadedTexture *QSSGLoadedTexture::loadCompressedImage(const QString &inPath)
{
    QSSGLoadedTexture *retval = nullptr;

    // Open File
    QFile imageFile(inPath);
    if (!imageFile.open(QIODevice::ReadOnly)) {
        qWarning() << "Could not open image file: " << inPath;
        return retval;
    }
    auto reader = new QTextureFileReader(&imageFile, inPath);

    if (!reader->canRead()) {
        qWarning() << "Unable to read image file: " << inPath;
        delete reader;
        return retval;
    }
    retval = new QSSGLoadedTexture;
    retval->compressedData = reader->read();

    // Fill out what makes sense, leave the rest at the default 0 and null.
    retval->width = retval->compressedData.size().width();
    retval->height = retval->compressedData.size().height();
    auto glFormat = retval->compressedData.glInternalFormat()
            ? retval->compressedData.glInternalFormat()
            : retval->compressedData.glFormat();
    retval->format = fromGLtoTextureFormat(glFormat);

    delete reader;
    imageFile.close();

    return retval;

}

namespace {
typedef unsigned char RGBE[4];
#define R 0
#define G 1
#define B 2
#define E 3

#define MINELEN 8 // minimum scanline length for encoding
#define MAXELEN 0x7fff // maximum scanline length for encoding



inline int calculateLine(int width, int bitdepth) { return ((width * bitdepth) + 7) / 8; }

inline int calculatePitch(int line) { return (line + 3) & ~3; }

float convertComponent(int exponent, int val)
{
    float v = val / (256.0f);
    float d = powf(2.0f, (float)exponent - 128.0f);
    return v * d;
}

void decrunchScanline(const char *&p, const char *pEnd, RGBE *scanline, int w)
{
    scanline[0][R] = *p++;
    scanline[0][G] = *p++;
    scanline[0][B] = *p++;
    scanline[0][E] = *p++;

    if (scanline[0][R] == 2 && scanline[0][G] == 2 && scanline[0][B] < 128) {
        // new rle, the first pixel was a dummy
        for (int channel = 0; channel < 4; ++channel) {
            for (int x = 0; x < w && p < pEnd; ) {
                unsigned char c = *p++;
                if (c > 128) { // run
                    if (p < pEnd) {
                        int repCount = c & 127;
                        c = *p++;
                        while (repCount--)
                            scanline[x++][channel] = c;
                    }
                } else { // not a run
                    while (c-- && p < pEnd)
                        scanline[x++][channel] = *p++;
                }
            }
        }
    } else {
        // old rle
        scanline[0][R] = 2;
        int bitshift = 0;
        int x = 1;
        while (x < w && pEnd - p >= 4) {
            scanline[x][R] = *p++;
            scanline[x][G] = *p++;
            scanline[x][B] = *p++;
            scanline[x][E] = *p++;

            if (scanline[x][R] == 1 && scanline[x][G] == 1 && scanline[x][B] == 1) { // run
                int repCount = scanline[x][3] << bitshift;
                while (repCount--) {
                    memcpy(scanline[x], scanline[x - 1], 4);
                    ++x;
                }
                bitshift += 8;
            } else { // not a run
                ++x;
                bitshift = 0;
            }
        }
    }
}

void decodeScanlineToTexture(RGBE *scanline, int width, void *outBuf, quint32 offset, QSSGRenderTextureFormat inFormat)
{
    quint8 *target = reinterpret_cast<quint8 *>(outBuf);
    target += offset;

    if (inFormat == QSSGRenderTextureFormat::RGBE8) {
        memcpy(target, scanline, size_t(width) * 4);
    } else {
        float rgbaF32[4];
        for (int i = 0; i < width; ++i) {
            rgbaF32[R] = convertComponent(scanline[i][E], scanline[i][R]);
            rgbaF32[G] = convertComponent(scanline[i][E], scanline[i][G]);
            rgbaF32[B] = convertComponent(scanline[i][E], scanline[i][B]);
            rgbaF32[3] = 1.0f;

            inFormat.encodeToPixel(rgbaF32, target, i * inFormat.getSizeofFormat());
        }
    }
}

}

QSSGLoadedTexture *QSSGLoadedTexture::loadHdrImage(const QSharedPointer<QIODevice> &source, const QSSGRenderTextureFormat &inFormat)
{
    QSSGLoadedTexture *imageData = nullptr;

    char sig[256];
    source->read(sig, 11);
    if (strncmp(sig, "#?RADIANCE\n", 11))
        return imageData;

    QByteArray buf = source->readAll();
    const char *p = buf.constData();
    const char *pEnd = p + buf.size();

    // Process lines until the empty one.
    QByteArray line;
    while (p < pEnd) {
        char c = *p++;
        if (c == '\n') {
            if (line.isEmpty())
                break;
            if (line.startsWith(QByteArrayLiteral("FORMAT="))) {
                const QByteArray format = line.mid(7).trimmed();
                if (format != QByteArrayLiteral("32-bit_rle_rgbe")) {
                    qWarning("HDR format '%s' is not supported", format.constData());
                    return imageData;
                }
            }
            line.clear();
        } else {
            line.append(c);
        }
    }
    if (p == pEnd) {
        qWarning("Malformed HDR image data at property strings");
        return imageData;
    }

    // Get the resolution string.
    while (p < pEnd) {
        char c = *p++;
        if (c == '\n')
            break;
        line.append(c);
    }
    if (p == pEnd) {
        qWarning("Malformed HDR image data at resolution string");
        return imageData;
    }

    int width = 0;
    int height = 0;
    // We only care about the standard orientation.
#ifdef Q_CC_MSVC
    if (!sscanf_s(line.constData(), "-Y %d +X %d", &height, &width)) {
#else
    if (!sscanf(line.constData(), "-Y %d +X %d", &height, &width)) {
#endif
        qWarning("Unsupported HDR resolution string '%s'", line.constData());
        return imageData;
    }
    if (width <= 0 || height <= 0) {
        qWarning("Invalid HDR resolution");
        return imageData;
    }

    // We need to do a sanity check on the inFormat
    QSSGRenderTextureFormat format = inFormat;
    if (format.format == QSSGRenderTextureFormat::Unknown) {
        // Loading HDR images for use outside of lightProbes will end up here
        // The renderer doesn't understand RGBE8 textures outside of lightProbes
        // So this needs to be a "real" format
        // TODO: This is a fallback, but there is no way of telling here what formats are supported
        format = QSSGRenderTextureFormat::RGBA16F;
    }

    const int bytesPerPixel = format.getSizeofFormat();
    const int bitCount = bytesPerPixel * 8;
    const int pitch = calculatePitch(calculateLine(width, bitCount));
    const quint32 dataSize = quint32(height * pitch);
    imageData = new QSSGLoadedTexture;
    imageData->dataSizeInBytes = dataSize;
    imageData->data = ::malloc(dataSize);
    imageData->width = width;
    imageData->height = height;
    imageData->format = format;
    imageData->components = format.getNumberOfComponent();

    // Allocate a scanline worth of RGBE data
    RGBE *scanline = new RGBE[width];

    // Note we are writing to the data buffer from bottom to top
    // to correct for -Y orientation
    for (int y = 0; y < height; ++y) {
        quint32 byteOffset = quint32((height - 1 - y) * width * bytesPerPixel);
        if (pEnd - p < 4) {
            qWarning("Unexpected end of HDR data");
            delete[] scanline;
            return imageData;
        }
        decrunchScanline(p, pEnd, scanline, width);
        decodeScanlineToTexture(scanline, width, imageData->data, byteOffset, format);
    }

    delete[] scanline;

    return imageData;
}

QSSGLoadedTexture *QSSGLoadedTexture::loadTextureData(QSSGRenderTextureData *textureData)
{
    const int bytesPerPixel = textureData->format().getSizeofFormat();
    const int bitCount = bytesPerPixel * 8;
    const int pitch = calculatePitch(calculateLine(textureData->size().width(), bitCount));
    const quint32 dataSize = quint32(textureData->size().height() * pitch);
    QSSGLoadedTexture *imageData = new QSSGLoadedTexture;
    imageData->dataSizeInBytes = dataSize;
    // We won't modifiy the data, but that is a nasty cast...
    imageData->data = const_cast<void*>(reinterpret_cast<const void*>(textureData->textureData().data()));
    imageData->width = textureData->size().width();
    imageData->height = textureData->size().height();
    imageData->format = textureData->format();
    imageData->components = textureData->format().getNumberOfComponent();

    return imageData;
}

namespace {

bool scanImageForAlpha(const void *inData, quint32 inWidth, quint32 inHeight, quint32 inPixelSizeInBytes, quint8 inAlphaSizeInBits)
{
    const quint8 *rowPtr = reinterpret_cast<const quint8 *>(inData);
    bool hasAlpha = false;
    if (inAlphaSizeInBits == 0)
        return hasAlpha;
    if (inPixelSizeInBytes != 2 && inPixelSizeInBytes != 4) {
        Q_ASSERT(false);
        return false;
    }
    if (inAlphaSizeInBits > 8) {
        Q_ASSERT(false);
        return false;
    }

    quint32 alphaRightShift = inPixelSizeInBytes * 8 - inAlphaSizeInBits;
    quint32 maxAlphaValue = (1 << inAlphaSizeInBits) - 1;

    for (quint32 rowIdx = 0; rowIdx < inHeight && !hasAlpha; ++rowIdx) {
        for (quint32 idx = 0; idx < inWidth && !hasAlpha; ++idx, rowPtr += inPixelSizeInBytes) {
            quint32 pixelValue = 0;
            if (inPixelSizeInBytes == 2)
                pixelValue = *(reinterpret_cast<const quint16 *>(rowPtr));
            else
                pixelValue = *(reinterpret_cast<const quint32 *>(rowPtr));
            pixelValue = pixelValue >> alphaRightShift;
            if (pixelValue < maxAlphaValue)
                hasAlpha = true;
        }
    }
    return hasAlpha;
}
}

QSSGLoadedTexture::~QSSGLoadedTexture()
{
    if (data && image.sizeInBytes() <= 0 && ownsData)
        ::free(data);
}

bool QSSGLoadedTexture::scanForTransparency() const
{
    switch (format.format) {
    case QSSGRenderTextureFormat::SRGB8A8:
    case QSSGRenderTextureFormat::RGBA8:
        if (!data) // dds
            return true;

        return scanImageForAlpha(data, width, height, 4, 8);
    // Scan the image.
    case QSSGRenderTextureFormat::SRGB8:
    case QSSGRenderTextureFormat::RGB8:
    case QSSGRenderTextureFormat::RGBE8:
        return false;
    case QSSGRenderTextureFormat::RGB565:
        return false;
    case QSSGRenderTextureFormat::RGBA5551:
        if (!data) { // dds
            return true;
        } else {
            return scanImageForAlpha(data, width, height, 2, 1);
        }
    case QSSGRenderTextureFormat::Alpha8:
        return true;
    case QSSGRenderTextureFormat::R8:
    case QSSGRenderTextureFormat::Luminance8:
    case QSSGRenderTextureFormat::RG8:
        return false;
    case QSSGRenderTextureFormat::LuminanceAlpha8:
        if (!data) // dds
            return true;

        return scanImageForAlpha(data, width, height, 2, 8);
    case QSSGRenderTextureFormat::RGB_DXT1:
        return false;
    case QSSGRenderTextureFormat::RGBA_DXT3:
    case QSSGRenderTextureFormat::RGBA_DXT1:
    case QSSGRenderTextureFormat::RGBA_DXT5:
        return false;
    case QSSGRenderTextureFormat::RGB9E5:
        return false;
    case QSSGRenderTextureFormat::RG32F:
    case QSSGRenderTextureFormat::RGB32F:
    case QSSGRenderTextureFormat::RGBA16F:
    case QSSGRenderTextureFormat::RGBA32F:
        // TODO : For now, since IBL will be the main consumer, we'll just
        // pretend there's no alpha. Need to do a proper scan down the line,
        // but doing it for floats is a little different from integer scans.
        return false;
    default:
        break;
    }
    Q_ASSERT(false);
    return false;
}

QSSGLoadedTexture *QSSGLoadedTexture::load(const QString &inPath,
                                           const QSSGRenderTextureFormat &inFormat,
                                           bool inFlipY)
{
    if (inPath.isEmpty())
        return nullptr;

    QSSGLoadedTexture *theLoadedImage = nullptr;
    QString fileName;
    QSSGInputUtil::FileType fileType = QSSGInputUtil::UnknownFile;
    QSharedPointer<QIODevice> theStream =
            QSSGInputUtil::getStreamForTextureFile(inPath, true, &fileName, &fileType);

    if (theStream) {
        switch (fileType) {
        case QSSGInputUtil::HdrFile:
            // inFormat is a suggestion that's only relevant for HDR images
            // (tells if we want want RGBA16F or RGBE-on-RGBA8)
            theLoadedImage = loadHdrImage(theStream, inFormat);
            break;
        case QSSGInputUtil::TextureFile:
            theLoadedImage = loadCompressedImage(fileName); // no choice but to ignore inFlipY here
            break;
        default:
            theLoadedImage = loadQImage(fileName, inFlipY);
            break;
        }
    }
    return theLoadedImage;
}

QT_END_NAMESPACE
