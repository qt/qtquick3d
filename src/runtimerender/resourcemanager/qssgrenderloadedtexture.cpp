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
#include <QtQuick3DRuntimeRender/private/qssgrenderinputstreamfactory_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendererutil_p.h>
#include <QtQuick3DRuntimeRender/private/qssgruntimerenderlogging_p.h>
#include <QtGui/QImage>
#include <QtGui/QOpenGLTexture>
#include <QtMath>

#include <QtQuick3DUtils/private/qssgutils_p.h>

#include <private/qtexturefilereader_p.h>

QT_BEGIN_NAMESPACE

QSSGRef<QSSGLoadedTexture> QSSGLoadedTexture::loadQImage(const QString &inPath,
                                                               const QSSGRenderTextureFormat &inFormat,
                                                               qint32 flipVertical,
                                                               QSSGRenderContextType renderContextType)
{
    Q_UNUSED(flipVertical)
    Q_UNUSED(renderContextType)
    static constexpr bool systemIsLittleEndian = QSysInfo::ByteOrder == QSysInfo::LittleEndian;
    QSSGRef<QSSGLoadedTexture> retval(nullptr);
    QImage image(inPath);
    if (inFormat == QSSGRenderTextureFormat::Unknown) {
        switch (image.format()) {
        case QImage::Format_Indexed8: // Convert palleted images
            image.convertTo(QImage::Format_RGBA8888_Premultiplied);
            break;
        case QImage::Format_RGBA64:
            image.convertTo(QImage::Format_RGBA8888);
            break;
        case QImage::Format_RGBA64_Premultiplied:
            image.convertTo(QImage::Format_RGBA8888_Premultiplied);
            break;
        case QImage::Format_RGBX64:
            image.convertTo(QImage::Format_RGBX8888);
            break;
        default:
            break;
        }
    }
    bool swizzleNeeded = image.pixelFormat().colorModel() == QPixelFormat::RGB
            && image.pixelFormat().typeInterpretation() == QPixelFormat::UnsignedInteger
            && systemIsLittleEndian;
    //??? What does inFormat mean? Is it even in use? Why always swizzle? Why do the musicians come out gradually?
    if (swizzleNeeded || inFormat != QSSGRenderTextureFormat::Unknown)
        image = std::move(image).rgbSwapped();
    image = std::move(image).mirrored();
    retval = new QSSGLoadedTexture;
    retval->width = image.width();
    retval->height = image.height();
    retval->components = image.pixelFormat().channelCount();
    retval->image = image;
    retval->data = (void *)retval->image.bits();
    retval->dataSizeInBytes = image.sizeInBytes();
    if (inFormat != QSSGRenderTextureFormat::Unknown)
        retval->format = inFormat;
    else
        retval->setFormatFromComponents();
    return retval;
}

QSSGRef<QSSGLoadedTexture> QSSGLoadedTexture::loadCompressedImage(const QString &inPath, const QSSGRenderTextureFormat &inFormat, bool inFlipY, const QSSGRenderContextType &renderContextType)
{
    Q_UNUSED(inFlipY)
    Q_UNUSED(renderContextType)

    QSSGRef<QSSGLoadedTexture> retval(nullptr);

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
    if (inFormat != QSSGRenderTextureFormat::Unknown)
        retval->format = inFormat;

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
        memcpy(target, scanline, size_t(4 * width));
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

QSSGRef<QSSGLoadedTexture> QSSGLoadedTexture::loadHdrImage(const QSharedPointer<QIODevice> &source, QSSGRenderContextType renderContextType)
{
    Q_UNUSED(renderContextType)
    QSSGRef<QSSGLoadedTexture> imageData(nullptr);

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
    if (!sscanf(line.constData(), "-Y %d +X %d", &height, &width)) {
        qWarning("Unsupported HDR resolution string '%s'", line.constData());
        return imageData;
    }
    if (width <= 0 || height <= 0) {
        qWarning("Invalid HDR resolution");
        return imageData;
    }


    // Format
    QSSGRenderTextureFormat imageFormat(QSSGRenderTextureFormat::RGBE8);

    const int bytesPerPixel = imageFormat.getSizeofFormat();
    const int bitCount = bytesPerPixel * 8;
    const int pitch = calculatePitch(calculateLine(width, bitCount));
    const quint32 dataSize = quint32(height * pitch);
    imageData = new QSSGLoadedTexture;
    imageData->dataSizeInBytes = dataSize;
    imageData->data = ::malloc(dataSize);
    imageData->width = width;
    imageData->height = height;
    imageData->m_bitCount = bitCount;
    imageData->m_ExtendedFormat = QSSGExtendedTextureFormats::CustomRGB;
    imageData->format = imageFormat;
    imageData->components = imageFormat.getNumberOfComponent();

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
        decodeScanlineToTexture(scanline, width, imageData->data, byteOffset, imageFormat);
    }

    delete[] scanline;

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
    if (data && image.sizeInBytes() <= 0) {
        ::free(data);
    }
    if (m_palette)
        ::free(m_palette);
    if (m_transparencyTable)
        ::free(m_transparencyTable);
}

bool QSSGLoadedTexture::scanForTransparency()
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
        // PKC TODO : For now, since IBL will be the main consumer, we'll just pretend there's no
        // alpha.
        // Need to do a proper scan down the line, but doing it for floats is a little different
        // from
        // integer scans.
        return false;
    default:
        break;
    }
    Q_ASSERT(false);
    return false;
}

QSSGRef<QSSGLoadedTexture> QSSGLoadedTexture::load(const QString &inPath,
                                                         const QSSGRenderTextureFormat &inFormat,
                                                         QSSGInputStreamFactory &inFactory,
                                                         bool inFlipY,
                                                         const QSSGRenderContextType &renderContextType)
{
    if (inPath.isEmpty())
        return nullptr;

    QSSGRef<QSSGLoadedTexture> theLoadedImage = nullptr;
    QSharedPointer<QIODevice> theStream(inFactory.getStreamForFile(inPath));
    QString fileName;
    inFactory.getPathForFile(inPath, fileName);
    if (theStream && inPath.size() > 3) {
        if (inPath.endsWith(QStringLiteral("png"), Qt::CaseInsensitive) || inPath.endsWith(QStringLiteral("jpg"), Qt::CaseInsensitive)
            || inPath.endsWith(QStringLiteral("peg"), Qt::CaseInsensitive)
            || inPath.endsWith(QStringLiteral("gif"), Qt::CaseInsensitive)
            || inPath.endsWith(QStringLiteral("bmp"), Qt::CaseInsensitive)) {
            theLoadedImage = loadQImage(fileName, inFormat, inFlipY, renderContextType);
        } else if (inPath.endsWith(QStringLiteral("dds"), Qt::CaseInsensitive)
                   || inPath.endsWith(QStringLiteral("ktx"), Qt::CaseInsensitive)
                   || inPath.endsWith(QStringLiteral("pkm"), Qt::CaseInsensitive)
                   || inPath.endsWith(QStringLiteral("astc"), Qt::CaseInsensitive)) {
            theLoadedImage = loadCompressedImage(fileName, inFormat, inFlipY, renderContextType);
        } else if (inPath.endsWith(QStringLiteral("hdr"), Qt::CaseInsensitive)) {
            theLoadedImage = loadHdrImage(theStream, renderContextType);
        } else {
            qCWarning(INTERNAL_ERROR, "Unrecognized image extension: %s", qPrintable(inPath));
        }
    }
    return theLoadedImage;
}

QT_END_NAMESPACE
