// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include <QtQuick3DTextureHelpers/private/qssgexrreader_p.h>
#include <QtQuick3DTextureHelpers/private/qssgexrcodecs_p.h>

#include <QtCore/qbitarray.h>
#include <QtCore/qendian.h>
#include <QtCore/qfloat16.h>
#include <QtCore/qnumeric.h>
#include <QtCore/qvarlengtharray.h>

#include <algorithm>
#include <cstring>
#include <limits>
#include <memory>
#include <vector>

QT_BEGIN_NAMESPACE

namespace {

constexpr quint32 ExrMagic = 0x01312f76;
constexpr int ExrVersion = 2;

constexpr quint32 FlagTiled = 0x0200;
constexpr quint32 FlagLongNames = 0x0400;
constexpr quint32 FlagNonImage = 0x0800; // deep data
constexpr quint32 FlagMultiPart = 0x1000;
constexpr quint32 FlagAll = FlagTiled | FlagLongNames | FlagNonImage | FlagMultiPart;

constexpr int ShortNameLimit = 31;
constexpr int LongNameLimit = 255;

constexpr float MaxFiniteHalf = 65504.0f;
constexpr float MaxFiniteFloat = std::numeric_limits<float>::max();

qint64 linesPerChunk(QSSGExrReader::Compression c)
{
    switch (c) {
    case QSSGExrReader::Compression::None:
    case QSSGExrReader::Compression::Rle:
    case QSSGExrReader::Compression::Zips:
        return 1;
    case QSSGExrReader::Compression::Zip:
        return 16;
    case QSSGExrReader::Compression::Piz:
        return 32;
    default:
        return 0;
    }
}

const char *compressionName(QSSGExrReader::Compression c)
{
    switch (c) {
    case QSSGExrReader::Compression::None:
        return "NONE";
    case QSSGExrReader::Compression::Rle:
        return "RLE";
    case QSSGExrReader::Compression::Zips:
        return "ZIPS";
    case QSSGExrReader::Compression::Zip:
        return "ZIP";
    case QSSGExrReader::Compression::Piz:
        return "PIZ";
    case QSSGExrReader::Compression::Pxr24:
        return "PXR24";
    case QSSGExrReader::Compression::B44:
        return "B44";
    case QSSGExrReader::Compression::B44A:
        return "B44A";
    case QSSGExrReader::Compression::DwaA:
        return "DWAA";
    case QSSGExrReader::Compression::DwaB:
        return "DWAB";
    }
    return "unknown";
}

qint64 bytesPerSample(QSSGExrReader::PixelType t)
{
    return t == QSSGExrReader::PixelType::Half ? 2 : 4;
}

class ByteCursor
{
public:
    ByteCursor(QByteArrayView data, QString *error) : m_data(data), m_error(error) { }

    bool seek(qint64 pos)
    {
        if (pos < 0 || pos > m_data.size())
            return fail(QStringLiteral("seek past the end of the file"));
        m_pos = pos;
        return true;
    }

    qint64 pos() const { return m_pos; }
    qint64 remaining() const { return m_data.size() - m_pos; }
    qint64 size() const { return m_data.size(); }

    bool require(qint64 n)
    {
        if (n < 0 || n > remaining())
            return fail(QStringLiteral("unexpected end of file"));
        return true;
    }

    bool skip(qint64 n)
    {
        if (!require(n))
            return false;
        m_pos += n;
        return true;
    }

    bool readU8(quint8 *out)
    {
        if (!require(1))
            return false;
        *out = quint8(m_data.at(m_pos));
        m_pos += 1;
        return true;
    }

    bool readI32(qint32 *out)
    {
        if (!require(4))
            return false;
        *out = qFromLittleEndian<qint32>(m_data.data() + m_pos);
        m_pos += 4;
        return true;
    }

    bool readU32(quint32 *out)
    {
        if (!require(4))
            return false;
        *out = qFromLittleEndian<quint32>(m_data.data() + m_pos);
        m_pos += 4;
        return true;
    }

    bool readU64(quint64 *out)
    {
        if (!require(8))
            return false;
        *out = qFromLittleEndian<quint64>(m_data.data() + m_pos);
        m_pos += 8;
        return true;
    }

    bool readFloat(float *out)
    {
        if (!require(4))
            return false;
        *out = qFromLittleEndian<float>(m_data.data() + m_pos);
        m_pos += 4;
        return true;
    }

    bool readName(QByteArray *out, int maxLen)
    {
        out->clear();
        for (int i = 0; i <= maxLen; ++i) {
            quint8 c = 0;
            if (!readU8(&c))
                return false;
            if (c == 0)
                return true;
            out->append(char(c));
        }
        return fail(QStringLiteral("attribute name is longer than %1 bytes").arg(maxLen));
    }

    ByteCursor sub(qint64 n) const { return ByteCursor(view(n), m_error); }

    QByteArrayView view(qint64 n) const
    {
        Q_ASSERT(n >= 0 && n <= remaining());
        return m_data.sliced(m_pos, qBound(qint64(0), n, remaining()));
    }

    bool fail(const QString &message)
    {
        if (m_error->isEmpty())
            *m_error = message;
        return false;
    }

private:
    QByteArrayView m_data;
    QString *m_error;
    qint64 m_pos = 0;
};

qint64 floorLog2(qint64 x)
{
    qint64 y = 0;
    while (x > 1) {
        y += 1;
        x >>= 1;
    }
    return y;
}

qint64 ceilLog2(qint64 x)
{
    qint64 y = 0;
    qint64 r = 0;
    while (x > 1) {
        if (x & 1)
            r = 1;
        y += 1;
        x >>= 1;
    }
    return y + r;
}

qint64 levelSize(qint64 top, qint64 level, bool roundUp)
{
    const qint64 b = qint64(1) << level;
    qint64 s = top / b;
    if (roundUp && s * b < top)
        s += 1;
    return qMax(s, qint64(1));
}

} // namespace

class QSSGExrReader::Private
{
public:
    QByteArray data;
    QString error;
    Limits limits;
    bool valid = false;

    quint32 versionFlags = 0;
    bool tiled = false;
    bool longNames = false;

    QRect dataWindow;
    QRect displayWindow;
    Compression compression = Compression::None;
    LineOrder lineOrder = LineOrder::IncreasingY;
    QList<Channel> channels;

    qint32 tileWidth = 0;
    qint32 tileHeight = 0;
    LevelMode levelMode = LevelMode::OneLevel;
    bool roundUp = false;

    qint64 width = 0;
    qint64 height = 0;
    qint64 bytesPerPixel = 0;
    QVarLengthArray<qint64, 8> channelByteOffset;
    QList<QSSGExr::PizChannel> pizChannels;

    QList<quint64> offsets;
    qint64 tilesX = 0;
    qint64 tilesY = 0;

    std::unique_ptr<QSSGExr::PizScratch> pizScratch;

    std::vector<float> band;

    bool setError(const QString &message)
    {
        if (error.isEmpty())
            error = message;
        return false;
    }

    bool parseHeaderAttributes(ByteCursor &cursor);
    bool validate();
    bool readOffsetTable(ByteCursor &cursor);

    bool decodeChunk(QByteArrayView payload, qint64 expected, qint64 tw, qint64 th,
                     QByteArray *out, QByteArray *scratch, QByteArrayView *decoded);
    void scatter(QByteArrayView chunk, qint64 r, qint64 tw, qint64 rowStride, const ChannelMap &map, float *row);

    bool decodeScanlines(const ChannelMap &map, Flags flags, RowSink sink);
    bool decodeTiles(const ChannelMap &map, Flags flags, RowSink sink);

    qint64 destinationRow(qint64 imageRow, Flags flags) const
    {
        return flags.testFlag(FlipVertically) ? height - 1 - imageRow : imageRow;
    }
};

bool QSSGExrReader::Private::parseHeaderAttributes(ByteCursor &cursor)
{
    const int nameLimit = longNames ? LongNameLimit : ShortNameLimit;

    bool haveChannels = false;
    bool haveCompression = false;
    bool haveDataWindow = false;
    bool haveDisplayWindow = false;
    bool haveTiles = false;

    int attributeCount = 0;

    for (;;) {
        QByteArray name;
        if (!cursor.readName(&name, nameLimit))
            return false;
        if (name.isEmpty())
            break;

        if (attributeCount++ >= limits.maxAttributes)
            return setError(QStringLiteral("header has too many attributes"));

        QByteArray type;
        if (!cursor.readName(&type, nameLimit))
            return false;
        if (type.isEmpty())
            return setError(QStringLiteral("attribute '%1' has no type").arg(QString::fromLatin1(name)));

        qint32 rawSize = 0;
        if (!cursor.readI32(&rawSize))
            return false;
        const qint64 size = rawSize;
        if (size < 0 || size > limits.maxAttributeBytes)
            return setError(QStringLiteral("attribute '%1' has an implausible size").arg(QString::fromLatin1(name)));
        if (!cursor.require(size))
            return false;

        ByteCursor value = cursor.sub(size);

        if (name == "channels" && type == "chlist") {
            channels.clear();
            for (;;) {
                QByteArray channelName;
                if (!value.readName(&channelName, nameLimit))
                    return false;
                if (channelName.isEmpty())
                    break;
                if (channels.size() >= limits.maxChannels)
                    return setError(QStringLiteral("image has too many channels"));

                qint32 pixelType = 0;
                quint8 pLinear = 0;
                qint32 xSampling = 0;
                qint32 ySampling = 0;
                if (!value.readI32(&pixelType) || !value.readU8(&pLinear) || !value.skip(3)
                    || !value.readI32(&xSampling) || !value.readI32(&ySampling)) {
                    return false;
                }
                if (pixelType < 0 || pixelType > 2)
                    return setError(QStringLiteral("channel '%1' has an unknown pixel type").arg(QString::fromLatin1(channelName)));
                if (xSampling != 1 || ySampling != 1) {
                    return setError(QStringLiteral("channel '%1' is subsampled, which is not "
                                                   "supported")
                                            .arg(QString::fromLatin1(channelName)));
                }

                Channel channel;
                channel.name = channelName;
                channel.type = PixelType(pixelType);

                // Also rules out duplicate names.
                if (!channels.isEmpty() && !(channels.last().name < channel.name))
                    return setError(QStringLiteral("channel list is not sorted"));

                channels.append(channel);
            }
            haveChannels = true;
        } else if (name == "compression" && type == "compression") {
            quint8 c = 0;
            if (!value.readU8(&c))
                return false;
            if (c > quint8(Compression::DwaB))
                return setError(QStringLiteral("unknown compression id %1").arg(c));
            compression = Compression(c);
            haveCompression = true;
        } else if ((name == "dataWindow" || name == "displayWindow") && type == "box2i") {
            qint32 xMin = 0, yMin = 0, xMax = 0, yMax = 0;
            if (!value.readI32(&xMin) || !value.readI32(&yMin) || !value.readI32(&xMax) || !value.readI32(&yMax)) {
                return false;
            }
            const qint64 w = qint64(xMax) - qint64(xMin) + 1;
            const qint64 h = qint64(yMax) - qint64(yMin) + 1;
            if (w < 1 || h < 1 || w > limits.maxWidth || h > limits.maxHeight) {
                return setError(QStringLiteral("%1 is empty or implausibly large").arg(QString::fromLatin1(name)));
            }
            const QRect rect(QPoint(xMin, yMin), QSize(int(w), int(h)));
            if (name == "dataWindow") {
                dataWindow = rect;
                haveDataWindow = true;
            } else {
                displayWindow = rect;
                haveDisplayWindow = true;
            }
        } else if (name == "lineOrder" && type == "lineOrder") {
            quint8 o = 0;
            if (!value.readU8(&o))
                return false;
            if (o > quint8(LineOrder::RandomY))
                return setError(QStringLiteral("unsupported line order %1").arg(o));
            lineOrder = LineOrder(o);
        } else if (name == "tiles" && type == "tiledesc") {
            quint32 xSize = 0, ySize = 0;
            quint8 mode = 0;
            if (!value.readU32(&xSize) || !value.readU32(&ySize) || !value.readU8(&mode))
                return false;
            if (xSize < 1 || ySize < 1 || xSize > quint32(limits.maxWidth) || ySize > quint32(limits.maxHeight)) {
                return setError(QStringLiteral("implausible tile size"));
            }
            const quint8 level = mode & 0x0f;
            const quint8 rounding = (mode >> 4) & 0x0f;
            if (level > quint8(LevelMode::RipMap))
                return setError(QStringLiteral("unknown tile level mode %1").arg(level));
            if (rounding > 1)
                return setError(QStringLiteral("unknown tile rounding mode %1").arg(rounding));
            tileWidth = qint32(xSize);
            tileHeight = qint32(ySize);
            levelMode = LevelMode(level);
            roundUp = rounding == 1;
            haveTiles = true;
        } else if (name == "type" && type == "string") {
            const QByteArray kind = value.view(size).toByteArray();
            if (kind == "deepscanline" || kind == "deeptile") {
                return setError(QStringLiteral("deep images are not supported"));
            }
        }

        if (!cursor.skip(size))
            return false;
    }

    if (!haveChannels)
        return setError(QStringLiteral("header has no channel list"));
    if (!haveCompression)
        return setError(QStringLiteral("header has no compression attribute"));
    if (!haveDataWindow)
        return setError(QStringLiteral("header has no data window"));
    if (tiled && !haveTiles)
        return setError(QStringLiteral("tiled image has no tile description"));

    // Required by the specification, but real writers omit them.
    if (!haveDisplayWindow)
        displayWindow = dataWindow;

    return true;
}

bool QSSGExrReader::Private::validate()
{
    if (channels.isEmpty())
        return setError(QStringLiteral("image has no channels"));

    if (linesPerChunk(compression) == 0) {
        return setError(QStringLiteral("%1 compression is not supported").arg(QString::fromLatin1(compressionName(compression))));
    }

    width = dataWindow.width();
    height = dataWindow.height();

    qint64 pixels = 0;
    if (qMulOverflow(width, height, &pixels) || pixels > limits.maxPixels)
        return setError(QStringLiteral("image is implausibly large"));

    channelByteOffset.resize(channels.size() + 1);
    channelByteOffset[0] = 0;
    for (qsizetype c = 0; c < channels.size(); ++c)
        channelByteOffset[c + 1] = channelByteOffset[c] + bytesPerSample(channels[c].type);
    bytesPerPixel = channelByteOffset[channels.size()];

    pizChannels.clear();
    pizChannels.reserve(channels.size());
    for (qsizetype c = 0; c < channels.size(); ++c) {
        QSSGExr::PizChannel pc;
        pc.wordsPerSample = bytesPerSample(channels[c].type) / 2;
        pc.byteOffsetInPixel = channelByteOffset[c];
        pizChannels.append(pc);
    }

    qint64 decoded = 0;
    if (qMulOverflow(pixels, qint64(4 * sizeof(float)), &decoded) || decoded > limits.maxDecodedBytes) {
        return setError(QStringLiteral("image needs more memory than the limit allows"));
    }

    return true;
}

bool QSSGExrReader::Private::readOffsetTable(ByteCursor &cursor)
{
    qint64 chunkCount = 0;

    if (tiled) {
        const bool mip = levelMode != LevelMode::OneLevel;
        const qint64 numXLevels = levelMode == LevelMode::OneLevel
                ? 1
                : (roundUp ? ceilLog2(levelMode == LevelMode::RipMap ? width : qMax(width, height))
                           : floorLog2(levelMode == LevelMode::RipMap ? width : qMax(width, height)))
                        + 1;
        const qint64 numYLevels = levelMode == LevelMode::RipMap ? (roundUp ? ceilLog2(height) : floorLog2(height)) + 1 : numXLevels;

        tilesX = (levelSize(width, 0, roundUp) + tileWidth - 1) / tileWidth;
        tilesY = (levelSize(height, 0, roundUp) + tileHeight - 1) / tileHeight;

        // Every level, or a chunk offset could point into the table.
        if (!mip) {
            chunkCount = tilesX * tilesY;
        } else if (levelMode == LevelMode::MipMap) {
            for (qint64 l = 0; l < numXLevels; ++l) {
                const qint64 tx = (levelSize(width, l, roundUp) + tileWidth - 1) / tileWidth;
                const qint64 ty = (levelSize(height, l, roundUp) + tileHeight - 1) / tileHeight;
                if (qAddOverflow(chunkCount, tx * ty, &chunkCount))
                    return setError(QStringLiteral("tile count is implausibly large"));
            }
        } else {
            for (qint64 ly = 0; ly < numYLevels; ++ly) {
                const qint64 ty = (levelSize(height, ly, roundUp) + tileHeight - 1) / tileHeight;
                for (qint64 lx = 0; lx < numXLevels; ++lx) {
                    const qint64 tx = (levelSize(width, lx, roundUp) + tileWidth - 1) / tileWidth;
                    if (qAddOverflow(chunkCount, tx * ty, &chunkCount))
                        return setError(QStringLiteral("tile count is implausibly large"));
                }
            }
        }
    } else {
        const qint64 lpc = linesPerChunk(compression);
        chunkCount = (height + lpc - 1) / lpc;
    }

    if (chunkCount < 1)
        return setError(QStringLiteral("image has no chunks"));

    qint64 tableBytes = 0;
    if (qMulOverflow(chunkCount, qint64(8), &tableBytes) || tableBytes > cursor.remaining())
        return setError(QStringLiteral("chunk offset table extends past the end of the file"));

    // Level (0,0) comes first.
    const qint64 wanted = tiled ? tilesX * tilesY : chunkCount;

    const qint64 minChunkHeader = tiled ? 20 : 8;
    const quint64 maxOffset = quint64(qMax(cursor.size() - minChunkHeader, qint64(0)));

    offsets.resize(wanted);
    for (qint64 i = 0; i < wanted; ++i) {
        quint64 offset = 0;
        if (!cursor.readU64(&offset))
            return false;
        if (offset == 0 || offset > maxOffset)
            return setError(QStringLiteral("chunk offset %1 is out of range").arg(i));
        offsets[i] = offset;
    }

    return true;
}

bool QSSGExrReader::Private::decodeChunk(QByteArrayView payload, qint64 expected, qint64 tw,
                                         qint64 th, QByteArray *out, QByteArray *scratch,
                                         QByteArrayView *decoded)
{
    if (payload.size() == expected) {
        // A block that did not shrink is stored raw.
        *decoded = payload;
        return true;
    }

    out->resize(expected);
    *decoded = QByteArrayView(out->constData(), expected);
    QSpan<char> outSpan(out->data(), expected);

    switch (compression) {
    case Compression::None:
        return setError(QStringLiteral("uncompressed chunk has the wrong size"));
    case Compression::Rle:
        scratch->resize(expected);
        if (!QSSGExr::rleDecode(payload, QSpan<char>(scratch->data(), expected), &error))
            return false;
        return QSSGExr::undoPredictorAndDeinterleave(outSpan, QSpan<char>(scratch->data(), expected), &error);
    case Compression::Zips:
    case Compression::Zip:
        scratch->resize(expected);
        if (!QSSGExr::inflateExact(payload, QSpan<char>(scratch->data(), expected), &error))
            return false;
        return QSSGExr::undoPredictorAndDeinterleave(outSpan, QSpan<char>(scratch->data(), expected), &error);
    case Compression::Piz:
        if (!pizScratch)
            pizScratch = std::make_unique<QSSGExr::PizScratch>();
        return QSSGExr::pizDecode(payload, outSpan, pizChannels, tw, th, bytesPerPixel, pizScratch.get(), &error);
    default:
        break;
    }

    return setError(QStringLiteral("%1 compression is not supported").arg(QString::fromLatin1(compressionName(compression))));
}

// 'row' points at the first component of the column this chunk starts at.
void QSSGExrReader::Private::scatter(QByteArrayView chunk, qint64 r, qint64 tw, qint64 rowStride, const ChannelMap &map, float *row)
{
    const char *rowBase = chunk.data() + r * rowStride;

    for (int k = 0; k < 4; ++k) {
        const qint32 source = map.source[k];
        if (source < 0) {
            const float v = map.constantValue[k];
            for (qint64 x = 0; x < tw; ++x)
                row[x * 4 + k] = v;
            continue;
        }

        // Checks the exponent bits, since qIsFinite() is out of line and too slow here.
        const char *src = rowBase + channelByteOffset[source] * tw;
        if (channels[source].type == PixelType::Half) {
            for (qint64 x = 0; x < tw; ++x) {
                const quint16 bits = qFromLittleEndian<quint16>(src + x * 2);
                if ((bits & 0x7c00) == 0x7c00) {
                    const float clamped = (bits & 0x8000) ? -MaxFiniteHalf : MaxFiniteHalf;
                    row[x * 4 + k] = (bits & 0x03ff) ? 0.0f : clamped;
                    continue;
                }
                // GCC rejects memcpy() into a class type without the cast.
                qfloat16 h;
                memcpy(static_cast<void *>(&h), &bits, sizeof(h));
                row[x * 4 + k] = float(h);
            }
        } else {
            for (qint64 x = 0; x < tw; ++x) {
                const quint32 bits = qFromLittleEndian<quint32>(src + x * 4);
                if ((bits & 0x7f800000u) == 0x7f800000u) {
                    const float clamped = (bits & 0x80000000u) ? -MaxFiniteFloat : MaxFiniteFloat;
                    row[x * 4 + k] = (bits & 0x007fffffu) ? 0.0f : clamped;
                    continue;
                }
                float v;
                memcpy(&v, &bits, sizeof(v));
                row[x * 4 + k] = v;
            }
        }
    }
}

bool QSSGExrReader::Private::decodeScanlines(const ChannelMap &map, Flags flags, RowSink sink)
{
    const qint64 lpc = linesPerChunk(compression);
    QBitArray seen(offsets.size());
    QByteArray chunk;
    QByteArray scratch;
    std::vector<float> row(size_t(width * 4), 0.0f);

    for (qsizetype i = 0; i < offsets.size(); ++i) {
        ByteCursor cursor(data, &error);
        if (!cursor.seek(qint64(offsets[i])))
            return false;

        qint32 y = 0;
        qint32 rawSize = 0;
        if (!cursor.readI32(&y) || !cursor.readI32(&rawSize))
            return false;

        const qint64 blockRow = qint64(y) - dataWindow.top();
        if (blockRow < 0 || blockRow >= height)
            return setError(QStringLiteral("chunk %1 is outside the data window").arg(i));
        if (blockRow % lpc != 0)
            return setError(QStringLiteral("chunk %1 does not start on a block boundary").arg(i));

        const qint64 block = blockRow / lpc;
        if (block >= offsets.size())
            return setError(QStringLiteral("chunk %1 is outside the data window").arg(i));
        if (seen.testBit(block))
            return setError(QStringLiteral("row block %1 appears more than once").arg(block));
        seen.setBit(block);

        const qint64 numLines = qMin(lpc, height - blockRow);
        qint64 expected = 0;
        if (qMulOverflow(numLines * width, bytesPerPixel, &expected) || expected > limits.maxChunkBytes) {
            return setError(QStringLiteral("chunk %1 is implausibly large").arg(i));
        }

        const qint64 size = rawSize;
        if (size < 1 || size > expected || size > cursor.remaining())
            return setError(QStringLiteral("chunk %1 has an implausible size").arg(i));

        QByteArrayView decoded;
        if (!decodeChunk(cursor.view(size), expected, width, numLines, &chunk, &scratch, &decoded))
            return false;

        for (qint64 r = 0; r < numLines; ++r) {
            scatter(decoded, r, width, width * bytesPerPixel, map, row.data());
            sink(qint32(destinationRow(blockRow + r, flags)), row.data());
        }
    }

    if (seen.count(false) != 0)
        return setError(QStringLiteral("image is missing %1 row blocks").arg(seen.count(false)));

    return true;
}

bool QSSGExrReader::Private::decodeTiles(const ChannelMap &map, Flags flags, RowSink sink)
{
    QBitArray seen(offsets.size());
    QByteArray chunk;
    QByteArray scratch;

    const qint64 bandHeight = qMin(qint64(tileHeight), height);
    band.assign(size_t(width * bandHeight * 4), 0.0f);
    qint64 bandY = -1;
    qint64 bandTiles = 0;

    for (qsizetype i = 0; i < offsets.size(); ++i) {
        ByteCursor cursor(data, &error);
        if (!cursor.seek(qint64(offsets[i])))
            return false;

        qint32 tileX = 0, tileY = 0, levelX = 0, levelY = 0, rawSize = 0;
        if (!cursor.readI32(&tileX) || !cursor.readI32(&tileY) || !cursor.readI32(&levelX) || !cursor.readI32(&levelY)
            || !cursor.readI32(&rawSize)) {
            return false;
        }

        if (levelX != 0 || levelY != 0)
            return setError(QStringLiteral("tile %1 is not in resolution level 0").arg(i));
        if (tileX < 0 || tileX >= tilesX || tileY < 0 || tileY >= tilesY)
            return setError(QStringLiteral("tile %1 is out of range").arg(i));

        const qint64 index = tileY * tilesX + tileX;
        if (seen.testBit(index))
            return setError(QStringLiteral("tile %1,%2 appears more than once").arg(tileX).arg(tileY));
        seen.setBit(index);

        const qint64 x0 = qint64(tileX) * tileWidth;
        const qint64 y0 = qint64(tileY) * tileHeight;
        const qint64 tw = qMin(qint64(tileWidth), width - x0);
        const qint64 th = qMin(qint64(tileHeight), height - y0);
        if (tw < 1 || th < 1)
            return setError(QStringLiteral("tile %1 is empty").arg(i));

        qint64 expected = 0;
        if (qMulOverflow(th * tw, bytesPerPixel, &expected) || expected > limits.maxChunkBytes) {
            return setError(QStringLiteral("tile %1 is implausibly large").arg(i));
        }

        const qint64 size = rawSize;
        if (size < 1 || size > expected || size > cursor.remaining())
            return setError(QStringLiteral("tile %1 has an implausible size").arg(i));

        QByteArrayView decoded;
        if (!decodeChunk(cursor.view(size), expected, tw, th, &chunk, &scratch, &decoded))
            return false;

        if (bandY != y0) {
            if (bandY >= 0 && bandTiles != tilesX) {
                return setError(QStringLiteral("tile row %1 is incomplete: tiles are not grouped "
                                               "by tile row")
                                        .arg(bandY));
            }
            std::fill(band.begin(), band.end(), 0.0f);
            bandY = y0;
            bandTiles = 0;
        }
        for (qint64 r = 0; r < th; ++r)
            scatter(decoded, r, tw, tw * bytesPerPixel, map, band.data() + (r * width + x0) * 4);
        if (++bandTiles == tilesX) {
            for (qint64 r = 0; r < th; ++r)
                sink(qint32(destinationRow(y0 + r, flags)), band.data() + r * width * 4);
        }
    }

    if (seen.count(false) != 0)
        return setError(QStringLiteral("image is missing %1 tiles").arg(seen.count(false)));

    return true;
}

QSSGExrReader::QSSGExrReader() : d(std::make_unique<Private>()) { }

QSSGExrReader::~QSSGExrReader() = default;

bool QSSGExrReader::isExr(QByteArrayView data)
{
    if (data.size() < 4)
        return false;
    return qFromLittleEndian<quint32>(data.data()) == ExrMagic;
}

void QSSGExrReader::setLimits(const Limits &limits)
{
    d->limits = limits;
}

const QSSGExrReader::Limits &QSSGExrReader::limits() const
{
    return d->limits;
}

bool QSSGExrReader::parseHeader(const QByteArray &data)
{
    const Limits limits = d->limits;
    d = std::make_unique<Private>();
    d->limits = limits;
    d->data = data;

    ByteCursor cursor(data, &d->error);

    quint32 magic = 0;
    if (!cursor.readU32(&magic))
        return false;
    if (magic != ExrMagic)
        return d->setError(QStringLiteral("not an OpenEXR file"));

    quint32 versionField = 0;
    if (!cursor.readU32(&versionField))
        return false;

    const quint32 version = versionField & 0xff;
    d->versionFlags = versionField & ~0xffu;
    if (version != ExrVersion)
        return d->setError(QStringLiteral("unsupported OpenEXR version %1").arg(version));
    if (d->versionFlags & ~FlagAll)
        return d->setError(QStringLiteral("unknown flags in the version field"));
    if (d->versionFlags & FlagMultiPart)
        return d->setError(QStringLiteral("multi part files are not supported"));
    if (d->versionFlags & FlagNonImage)
        return d->setError(QStringLiteral("deep images are not supported"));

    d->tiled = d->versionFlags & FlagTiled;
    d->longNames = d->versionFlags & FlagLongNames;

    if (!d->parseHeaderAttributes(cursor))
        return false;
    if (!d->validate())
        return false;
    if (!d->readOffsetTable(cursor))
        return false;

    d->valid = true;
    return true;
}

bool QSSGExrReader::isValid() const
{
    return d->valid;
}

QSize QSSGExrReader::size() const
{
    return d->dataWindow.size();
}

QRect QSSGExrReader::dataWindow() const
{
    return d->dataWindow;
}

QRect QSSGExrReader::displayWindow() const
{
    return d->displayWindow;
}

QSSGExrReader::Compression QSSGExrReader::compression() const
{
    return d->compression;
}

QSSGExrReader::LineOrder QSSGExrReader::lineOrder() const
{
    return d->lineOrder;
}

const QList<QSSGExrReader::Channel> &QSSGExrReader::channels() const
{
    return d->channels;
}

qint32 QSSGExrReader::indexOfChannel(QByteArrayView name) const
{
    for (qsizetype i = 0; i < d->channels.size(); ++i) {
        if (d->channels[i].name == name)
            return qint32(i);
    }
    return -1;
}

bool QSSGExrReader::isTiled() const
{
    return d->tiled;
}

QSize QSSGExrReader::tileSize() const
{
    return d->tiled ? QSize(d->tileWidth, d->tileHeight) : QSize();
}

QSSGExrReader::LevelMode QSSGExrReader::levelMode() const
{
    return d->levelMode;
}

bool QSSGExrReader::decode(const ChannelMap &map, Flags flags, RowSink sink)
{
    if (!d->valid)
        return d->setError(QStringLiteral("no valid header"));

    d->error.clear();

    for (int k = 0; k < 4; ++k) {
        if (map.source[k] >= d->channels.size())
            return d->setError(QStringLiteral("channel map refers to a channel that is absent"));
        if (map.source[k] >= 0 && d->channels[map.source[k]].type == PixelType::UInt) {
            return d->setError(QStringLiteral("channel '%1' is an integer channel, which is not "
                                              "supported")
                                       .arg(QString::fromLatin1(d->channels[map.source[k]].name)));
        }
    }

    const bool ok = d->tiled ? d->decodeTiles(map, flags, sink) : d->decodeScanlines(map, flags, sink);

    d->band.clear();
    d->band.shrink_to_fit();
    d->pizScratch.reset();

    return ok;
}

bool QSSGExrReader::decodeToRgba32F(const ChannelMap &map, Flags flags, float *dst, qsizetype dstFloatCount)
{
    if (!d->valid)
        return d->setError(QStringLiteral("no valid header"));

    d->error.clear();

    const qint64 needed = d->width * d->height * 4;
    if (!dst || dstFloatCount < needed)
        return d->setError(QStringLiteral("destination buffer is too small"));

    // Never hand uninitialised memory to a texture.
    memset(dst, 0, size_t(needed) * sizeof(float));

    const qint64 rowFloats = d->width * 4;
    return decode(map, flags, [&](qint32 dstRow, const float *rgba) {
        memcpy(dst + dstRow * rowFloats, rgba, size_t(rowFloats) * sizeof(float));
    });
}

QString QSSGExrReader::errorMessage() const
{
    return d->error;
}

QSSGExrReader::ChannelMap qssgExrRgbaChannelMap(const QSSGExrReader &reader)
{
    QSSGExrReader::ChannelMap map;
    map.constantValue[0] = 0.0f;
    map.constantValue[1] = 0.0f;
    map.constantValue[2] = 0.0f;
    map.constantValue[3] = 1.0f;

    if (reader.channels().size() == 1) {
        // Alpha too, the baseline images depend on it.
        map.source[0] = map.source[1] = map.source[2] = map.source[3] = 0;
        return map;
    }

    map.source[0] = reader.indexOfChannel("R");
    map.source[1] = reader.indexOfChannel("G");
    map.source[2] = reader.indexOfChannel("B");
    map.source[3] = reader.indexOfChannel("A"); // absent means opaque

    return map;
}

QT_END_NAMESPACE
