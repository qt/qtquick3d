// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#ifndef QSSGEXRREADER_P_H
#define QSSGEXRREADER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qbytearray.h>
#include <QtCore/qbytearrayview.h>
#include <QtCore/qlist.h>
#include <QtCore/qrect.h>
#include <QtCore/qsize.h>
#include <QtCore/qstring.h>
#include <QtCore/qxpfunctional.h>

#include <memory>

QT_BEGIN_NAMESPACE

// Reads single part scanline and tiled OpenEXR images with NONE, RLE, ZIPS,
// ZIP or PIZ compression. Tiles have to be grouped by tile row. An infinity
// becomes the largest finite value of its source format and a NaN becomes 0.
class QSSGExrReader
{
public:
    enum class Compression : quint8 {
        None = 0,
        Rle = 1,
        Zips = 2,
        Zip = 3,
        Piz = 4,
        Pxr24 = 5,
        B44 = 6,
        B44A = 7,
        DwaA = 8,
        DwaB = 9
    };

    enum class PixelType : quint8 { UInt = 0, Half = 1, Float = 2 };

    enum class LevelMode : quint8 { OneLevel = 0, MipMap = 1, RipMap = 2 };

    enum class LineOrder : quint8 { IncreasingY = 0, DecreasingY = 1, RandomY = 2 };

    struct Channel
    {
        QByteArray name;
        PixelType type = PixelType::Half;
    };

    // A negative source fills that component with constantValue instead.
    struct ChannelMap
    {
        qint32 source[4] = { -1, -1, -1, -1 };
        float constantValue[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    };

    // Per quantity bounds, not a budget for the total memory a decode uses.
    struct Limits
    {
        qint32 maxWidth = 65536;
        qint32 maxHeight = 65536;
        qint64 maxPixels = 1 << 28;
        qint32 maxChannels = 64;
        qint32 maxAttributes = 1024;
        qint64 maxAttributeBytes = 1 << 20;
        qint64 maxChunkBytes = 1 << 28;
        qint64 maxDecodedBytes = 1 << 30;
    };

    enum Flag : quint8 { NoFlags = 0x0, FlipVertically = 0x1 };
    Q_DECLARE_FLAGS(Flags, Flag)

    QSSGExrReader();
    ~QSSGExrReader();
    Q_DISABLE_COPY_MOVE(QSSGExrReader)

    // Checks the magic number only.
    static bool isExr(QByteArrayView data);

    void setLimits(const Limits &limits);
    const Limits &limits() const;

    // Keeps a shallow copy of data for decode().
    bool parseHeader(const QByteArray &data);

    bool isValid() const;
    QSize size() const; // of the data window
    QRect dataWindow() const;
    QRect displayWindow() const;
    Compression compression() const;
    LineOrder lineOrder() const;
    const QList<Channel> &channels() const;
    qint32 indexOfChannel(QByteArrayView name) const; // -1 when absent

    bool isTiled() const;
    QSize tileSize() const; // invalid when not tiled
    LevelMode levelMode() const;

    // 'rgba' holds 4 * size().width() floats, valid for the call only. Rows
    // arrive in an unspecified order, exactly once each, dstRow already
    // accounting for FlipVertically. The sink may be called from several
    // threads at once.
    using RowSink = qxp::function_ref<void(qint32 dstRow, const float *rgba) const>;

    // Resolution level 0 only. Success means every row was delivered.
    bool decode(const ChannelMap &map, Flags flags, RowSink sink);

    // width * height * 4 tightly packed floats.
    bool decodeToRgba32F(const ChannelMap &map, Flags flags, float *dst, qsizetype dstFloatCount);

    QString errorMessage() const;

private:
    class Private;
    std::unique_ptr<Private> d;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QSSGExrReader::Flags)

// R, G, B and A by name, or a single channel as greyscale.
QSSGExrReader::ChannelMap qssgExrRgbaChannelMap(const QSSGExrReader &reader);

QT_END_NAMESPACE

#endif // QSSGEXRREADER_P_H
