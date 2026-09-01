// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QtTest>

#include <QtQuick3DTextureHelpers/private/qssgexrcodecs_p.h>
#include <QtQuick3DTextureHelpers/private/qssgexrreader_p.h>

#include <QtCore/qendian.h>
#include <QtCore/qfile.h>
#include <QtCore/qfloat16.h>

#include <cmath>
#include <limits>

using namespace Qt::StringLiterals;

// Builds a valid EXR byte for byte, for the malformed cases to mutate.
class ExrBuilder
{
public:
    enum PixelType { Uint = 0, Half = 1, Float = 2 };

    struct Channel
    {
        QByteArray name;
        PixelType type = Half;
        qint32 xSampling = 1;
        qint32 ySampling = 1;
    };

    ExrBuilder() { m_channels = { { "B"_ba, Half, 1, 1 }, { "G"_ba, Half, 1, 1 }, { "R"_ba, Half, 1, 1 } }; }

    ExrBuilder &setSize(int w, int h)
    {
        m_width = w;
        m_height = h;
        return *this;
    }
    ExrBuilder &setOrigin(int x, int y)
    {
        m_originX = x;
        m_originY = y;
        return *this;
    }
    ExrBuilder &setCompression(quint8 c)
    {
        m_compression = c;
        return *this;
    }
    ExrBuilder &setChannels(const QList<Channel> &channels)
    {
        m_channels = channels;
        return *this;
    }
    ExrBuilder &setVersionField(quint32 v)
    {
        m_versionField = v;
        m_versionFieldSet = true;
        return *this;
    }
    ExrBuilder &setTiles(qint32 w, qint32 h, quint8 mode = 0)
    {
        m_tiled = true;
        m_tileWidth = w;
        m_tileHeight = h;
        m_tileMode = mode;
        return *this;
    }
    ExrBuilder &setLineOrder(quint8 o)
    {
        m_lineOrder = o;
        return *this;
    }
    ExrBuilder &setReverseChunkStorage(bool reverse)
    {
        m_reverseChunkStorage = reverse;
        return *this;
    }
    ExrBuilder &dropAttribute(const QByteArray &name)
    {
        m_dropped.append(name);
        return *this;
    }
    ExrBuilder &addRawAttribute(const QByteArray &name, const QByteArray &type, const QByteArray &value)
    {
        m_extra.append({ name, type, value });
        return *this;
    }
    // Overrides the declared size of an attribute without changing its payload.
    ExrBuilder &overrideAttributeSize(const QByteArray &name, qint32 size)
    {
        m_sizeOverride[name] = size;
        return *this;
    }

    QByteArray build() const;

    qint64 offsetOf(const QByteArray &what) const { return m_offsets.value(what, -1); }

    static float sample(int x, int y, int c) { return (x * 7 + y * 13 + c * 29) / 256.0f; }

    // Independent of the reader's inverse, so a misunderstanding cannot cancel out.
    static QByteArray applyZipTransform(const QByteArray &raw);
    static QByteArray rleEncode(const QByteArray &in);

private:
    struct Attribute
    {
        QByteArray name;
        QByteArray type;
        QByteArray value;
    };

    QList<Channel> m_channels;
    QList<Attribute> m_extra;
    QList<QByteArray> m_dropped;
    QHash<QByteArray, qint32> m_sizeOverride;
    int m_width = 8;
    int m_height = 8;
    int m_originX = 0;
    int m_originY = 0;
    quint8 m_compression = 0; // NONE
    quint8 m_lineOrder = 0;
    bool m_reverseChunkStorage = false;
    quint32 m_versionField = 2;
    bool m_versionFieldSet = false;
    bool m_tiled = false;
    qint32 m_tileWidth = 0;
    qint32 m_tileHeight = 0;
    quint8 m_tileMode = 0;
    mutable QHash<QByteArray, qint64> m_offsets;
};

static void appendI32(QByteArray *out, qint32 v)
{
    char buf[4];
    qToLittleEndian(v, buf);
    out->append(buf, 4);
}

static void appendU32(QByteArray *out, quint32 v)
{
    char buf[4];
    qToLittleEndian(v, buf);
    out->append(buf, 4);
}

static void appendU64(QByteArray *out, quint64 v)
{
    char buf[8];
    qToLittleEndian(v, buf);
    out->append(buf, 8);
}

static void appendFloat(QByteArray *out, float v)
{
    char buf[4];
    qToLittleEndian(v, buf);
    out->append(buf, 4);
}

QByteArray ExrBuilder::applyZipTransform(const QByteArray &raw)
{
    const qsizetype n = raw.size();
    QByteArray tmp(n, '\0');
    // Split into a low and a high half, the low half taking the odd byte.
    qsizetype i1 = 0;
    qsizetype i2 = (n + 1) / 2;
    qsizetype s = 0;
    while (s < n) {
        tmp[i1++] = raw[s++];
        if (s < n)
            tmp[i2++] = raw[s++];
    }
    // Then a running delta over the whole buffer.
    int prev = quint8(tmp[0]);
    for (qsizetype i = 1; i < n; ++i) {
        const int cur = quint8(tmp[i]);
        tmp[i] = char(quint8(cur - prev + (128 + 256)));
        prev = cur;
    }
    return tmp;
}

QByteArray ExrBuilder::rleEncode(const QByteArray &in)
{
    QByteArray out;
    qsizetype i = 0;
    while (i < in.size()) {
        qsizetype run = 1;
        while (i + run < in.size() && in[i + run] == in[i] && run < 128)
            ++run;
        if (run >= 3) {
            out.append(char(qint8(run - 1)));
            out.append(in[i]);
            i += run;
        } else {
            qsizetype lit = 0;
            while (i + lit < in.size() && lit < 128) {
                qsizetype r = 1;
                while (i + lit + r < in.size() && in[i + lit + r] == in[i + lit] && r < 3)
                    ++r;
                if (r >= 3)
                    break;
                ++lit;
            }
            if (lit == 0)
                lit = 1;
            out.append(char(qint8(-lit)));
            out.append(in.sliced(i, lit));
            i += lit;
        }
    }
    return out;
}

QByteArray ExrBuilder::build() const
{
    m_offsets.clear();
    QByteArray out;

    m_offsets["magic"_ba] = out.size();
    appendU32(&out, 0x01312f76);
    m_offsets["version"_ba] = out.size();
    appendU32(&out, m_versionFieldSet ? m_versionField : (m_tiled ? 2u | 0x200u : 2u));

    QList<Attribute> attributes;

    QByteArray chlist;
    for (const Channel &c : m_channels) {
        chlist.append(c.name);
        chlist.append('\0');
        appendI32(&chlist, c.type);
        chlist.append(char(0)); // pLinear
        chlist.append(3, '\0'); // reserved
        appendI32(&chlist, c.xSampling);
        appendI32(&chlist, c.ySampling);
    }
    chlist.append('\0');
    attributes.append({ "channels"_ba, "chlist"_ba, chlist });

    attributes.append({ "compression"_ba, "compression"_ba, QByteArray(1, char(m_compression)) });

    QByteArray dataWindow;
    appendI32(&dataWindow, m_originX);
    appendI32(&dataWindow, m_originY);
    appendI32(&dataWindow, m_originX + m_width - 1);
    appendI32(&dataWindow, m_originY + m_height - 1);
    attributes.append({ "dataWindow"_ba, "box2i"_ba, dataWindow });

    attributes.append({ "displayWindow"_ba, "box2i"_ba, dataWindow });

    attributes.append({ "lineOrder"_ba, "lineOrder"_ba, QByteArray(1, char(m_lineOrder)) });

    QByteArray par;
    appendFloat(&par, 1.0f);
    attributes.append({ "pixelAspectRatio"_ba, "float"_ba, par });

    QByteArray swc;
    appendFloat(&swc, 0.0f);
    appendFloat(&swc, 0.0f);
    attributes.append({ "screenWindowCenter"_ba, "v2f"_ba, swc });

    QByteArray sww;
    appendFloat(&sww, 1.0f);
    attributes.append({ "screenWindowWidth"_ba, "float"_ba, sww });

    if (m_tiled) {
        QByteArray tiles;
        appendU32(&tiles, quint32(m_tileWidth));
        appendU32(&tiles, quint32(m_tileHeight));
        tiles.append(char(m_tileMode));
        attributes.append({ "tiles"_ba, "tiledesc"_ba, tiles });
    }

    for (Attribute &a : attributes) {
        if (m_dropped.contains(a.name))
            a.name.clear(); // marked for removal below
    }
    attributes.append(m_extra);

    for (const Attribute &a : attributes) {
        if (a.name.isEmpty())
            continue;
        m_offsets["attr:"_ba + a.name] = out.size();
        out.append(a.name);
        out.append('\0');
        out.append(a.type);
        out.append('\0');
        m_offsets["attr:"_ba + a.name + ":size"_ba] = out.size();
        appendI32(&out, m_sizeOverride.value(a.name, qint32(a.value.size())));
        m_offsets["attr:"_ba + a.name + ":value"_ba] = out.size();
        out.append(a.value);
    }
    out.append('\0'); // end of header
    m_offsets["headerEnd"_ba] = out.size();

    qint64 bytesPerPixel = 0;
    for (const Channel &c : m_channels)
        bytesPerPixel += c.type == Half ? 2 : 4;

    struct Chunk
    {
        qint64 x0 = 0;
        qint64 y0 = 0;
        qint64 w = 0;
        qint64 h = 0;
        qint64 tx = 0;
        qint64 ty = 0;
    };
    QList<Chunk> chunks;

    if (m_tiled) {
        const qint64 tilesX = (m_width + m_tileWidth - 1) / m_tileWidth;
        const qint64 tilesY = (m_height + m_tileHeight - 1) / m_tileHeight;
        for (qint64 ty = 0; ty < tilesY; ++ty) {
            for (qint64 tx = 0; tx < tilesX; ++tx) {
                Chunk c;
                c.x0 = tx * m_tileWidth;
                c.y0 = ty * m_tileHeight;
                c.w = qMin(qint64(m_tileWidth), m_width - c.x0);
                c.h = qMin(qint64(m_tileHeight), m_height - c.y0);
                c.tx = tx;
                c.ty = ty;
                chunks.append(c);
            }
        }
    } else {
        qint64 lpc = 1;
        if (m_compression == 3)
            lpc = 16;
        else if (m_compression == 4)
            lpc = 32;
        for (qint64 row = 0; row < m_height; row += lpc) {
            Chunk c;
            c.x0 = 0;
            c.y0 = row;
            c.w = m_width;
            c.h = qMin(lpc, m_height - row);
            chunks.append(c);
        }
    }

    m_offsets["offsetTable"_ba] = out.size();
    const qint64 tableStart = out.size();
    for (qsizetype i = 0; i < chunks.size(); ++i)
        appendU64(&out, 0);

    QList<qsizetype> storageOrder;
    for (qsizetype i = 0; i < chunks.size(); ++i)
        storageOrder.append(m_reverseChunkStorage ? chunks.size() - 1 - i : i);

    for (qsizetype k = 0; k < storageOrder.size(); ++k) {
        const qsizetype i = storageOrder[k];
        const Chunk &chunk = chunks[i];
        const qint64 offset = out.size();
        qToLittleEndian(quint64(offset), out.data() + tableStart + i * 8);

        m_offsets["chunk:"_ba + QByteArray::number(i)] = out.size();
        if (m_tiled) {
            appendI32(&out, qint32(chunk.tx));
            appendI32(&out, qint32(chunk.ty));
            appendI32(&out, 0); // levelX
            appendI32(&out, 0); // levelY
        } else {
            appendI32(&out, qint32(m_originY + chunk.y0));
        }

        QByteArray raw;
        for (qint64 r = 0; r < chunk.h; ++r) {
            for (qsizetype c = 0; c < m_channels.size(); ++c) {
                for (qint64 x = 0; x < chunk.w; ++x) {
                    const float v = sample(int(chunk.x0 + x), int(chunk.y0 + r), int(c));
                    if (m_channels[c].type == Half) {
                        char buf[2];
                        qToLittleEndian(qfloat16(v), buf);
                        raw.append(buf, 2);
                    } else {
                        appendFloat(&raw, v);
                    }
                }
            }
        }
        Q_ASSERT(raw.size() == chunk.h * chunk.w * bytesPerPixel);

        QByteArray payload = raw;
        if (m_compression == 1) {
            payload = rleEncode(applyZipTransform(raw));
        } else if (m_compression == 2 || m_compression == 3) {
            payload = qCompress(applyZipTransform(raw), 9).sliced(4);
        }
        // Stored verbatim if it did not get smaller, as the writers do.
        if (payload.size() >= raw.size())
            payload = raw;

        m_offsets["chunk:"_ba + QByteArray::number(i) + ":dataSize"_ba] = out.size();
        appendI32(&out, qint32(payload.size()));
        m_offsets["chunk:"_ba + QByteArray::number(i) + ":payload"_ba] = out.size();
        out.append(payload);
    }

    return out;
}

class tst_QSSGExrReader : public QObject
{
    Q_OBJECT

private slots:
    // Header and attribute parsing
    void magicAndVersion();
    void attributeOrderIndependence();
    void unknownAttributesSkipped();
    void dataWindowGeometry();
    void lineOrderValues_data();
    void lineOrderValues();

    // Valid decode
    void decodeBuilt_data();
    void decodeBuilt();
    void decodeFixture_data();
    void decodeFixture();
    void channelSelection();
    void halfConversionIsExhaustive();
    void nonFiniteValuesAreSanitised();
    void flipVertically();
    void chunkStorageOrderIsIrrelevant_data();
    void chunkStorageOrderIsIrrelevant();
    void errorIsClearedBetweenDecodes();
    void headerIsResetBetweenParses();

    // Rejection
    void badMagic_data();
    void badMagic();
    void truncatedAtEveryOffset_data();
    void truncatedAtEveryOffset();
    void unsupportedFlags_data();
    void unsupportedFlags();
    void unsupportedCompression_data();
    void unsupportedCompression();
    void unsupportedCompressionFixture_data();
    void unsupportedCompressionFixture();
    void missingRequiredAttribute_data();
    void missingRequiredAttribute();
    void malformedAttributes_data();
    void malformedAttributes();
    void badGeometry_data();
    void badGeometry();
    void badChannels_data();
    void badChannels();
    void badOffsetTable_data();
    void badOffsetTable();
    void badChunks_data();
    void badChunks();
    void limitsAreEnforced();
    void hugeTileOnSmallImage();

    // Codec level
    void zipTransformRoundTrip_data();
    void zipTransformRoundTrip();
    void rleDecodeHostile_data();
    void rleDecodeHostile();
    void inflateExactRejects_data();
    void inflateExactRejects();
    void hufUncompressHostile_data();
    void hufUncompressHostile();
    void hufRejectsUndecodableCodeLength();
    void wav2DecodeDegenerateSizes();

    // Fuzz findings
    void fuzzRegressions_data();
    void fuzzRegressions();

private:
    static QString testFile(const QString &name) { return QFINDTESTDATA(QStringLiteral("data/") + name); }
    static QByteArray fixture(const QString &name)
    {
        QFile f(testFile(name));
        if (!f.open(QIODevice::ReadOnly))
            return {};
        return f.readAll();
    }
    // Decodes and checks against the generator's pixel formula.
    static void verifyDecode(const QByteArray &data, int w, int h, int channelCount, bool singleChannel);
};

void tst_QSSGExrReader::verifyDecode(const QByteArray &data, int w, int h, int channelCount, bool singleChannel)
{
    QSSGExrReader reader;
    QVERIFY2(reader.parseHeader(data), qPrintable(reader.errorMessage()));
    QCOMPARE(reader.size(), QSize(w, h));

    QList<float> pixels(qsizetype(w) * h * 4, 0.0f);
    QVERIFY2(reader.decodeToRgba32F(qssgExrRgbaChannelMap(reader), QSSGExrReader::NoFlags, pixels.data(), pixels.size()),
             qPrintable(reader.errorMessage()));

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const float *p = pixels.constData() + (qsizetype(y) * w + x) * 4;
            if (singleChannel) {
                const float v = ExrBuilder::sample(x, y, 0);
                for (int k = 0; k < 4; ++k)
                    QCOMPARE(p[k], v);
            } else {
                // Both write channels in name order, so translate back.
                for (int k = 0; k < 3; ++k) {
                    static const char *names[3] = { "R", "G", "B" };
                    const qint32 idx = reader.indexOfChannel(QByteArrayView(names[k]));
                    QVERIFY(idx >= 0);
                    QCOMPARE(p[k], ExrBuilder::sample(x, y, idx));
                }
                const qint32 alpha = reader.indexOfChannel("A");
                QCOMPARE(p[3], alpha >= 0 ? ExrBuilder::sample(x, y, alpha) : 1.0f);
            }
        }
    }
    Q_UNUSED(channelCount);
}

void tst_QSSGExrReader::magicAndVersion()
{
    const QByteArray good = ExrBuilder().build();
    QVERIFY(QSSGExrReader::isExr(good));

    QSSGExrReader reader;
    QVERIFY2(reader.parseHeader(good), qPrintable(reader.errorMessage()));
    QVERIFY(reader.isValid());
    QVERIFY(reader.errorMessage().isEmpty());

    for (quint32 version : { 0u, 1u, 3u, 255u }) {
        const QByteArray bad = ExrBuilder().setVersionField(version).build();
        QSSGExrReader r;
        QVERIFY(!r.parseHeader(bad));
        QVERIFY(r.errorMessage().contains("version"_L1));
    }

    // A bit outside the four the format defines.
    const QByteArray unknownFlag = ExrBuilder().setVersionField(2u | 0x2000u).build();
    QSSGExrReader r;
    QVERIFY(!r.parseHeader(unknownFlag));
    QVERIFY(r.errorMessage().contains("flags"_L1));
}

void tst_QSSGExrReader::attributeOrderIndependence()
{
    ExrBuilder builder;
    builder.setSize(8, 4);
    const QByteArray reference = builder.build();

    QByteArray dataWindow;
    appendI32(&dataWindow, 0);
    appendI32(&dataWindow, 0);
    appendI32(&dataWindow, 7);
    appendI32(&dataWindow, 3);

    ExrBuilder reordered;
    reordered.setSize(8, 4).dropAttribute("dataWindow"_ba).addRawAttribute("dataWindow"_ba, "box2i"_ba, dataWindow);
    const QByteArray moved = reordered.build();

    QSSGExrReader a, b;
    QVERIFY2(a.parseHeader(reference), qPrintable(a.errorMessage()));
    QVERIFY2(b.parseHeader(moved), qPrintable(b.errorMessage()));
    QCOMPARE(a.size(), b.size());
    QCOMPARE(a.dataWindow(), b.dataWindow());
}

void tst_QSSGExrReader::unknownAttributesSkipped()
{
    ExrBuilder builder;
    builder.setSize(8, 4)
            .addRawAttribute("Software"_ba, "string"_ba, "something"_ba)
            .addRawAttribute("frobnicate"_ba, "unknownType"_ba, QByteArray(4096, 'x'))
            .addRawAttribute("Orientation"_ba, "int"_ba, QByteArray(4, '\1'));
    const QByteArray data = builder.build();

    QSSGExrReader reader;
    QVERIFY2(reader.parseHeader(data), qPrintable(reader.errorMessage()));
    QCOMPARE(reader.size(), QSize(8, 4));
}

void tst_QSSGExrReader::dataWindowGeometry()
{
    {
        ExrBuilder builder;
        builder.setSize(16, 8).setOrigin(7, 3);
        const QByteArray data = builder.build();
        QSSGExrReader reader;
        QVERIFY2(reader.parseHeader(data), qPrintable(reader.errorMessage()));
        QCOMPARE(reader.size(), QSize(16, 8));
        QCOMPARE(reader.dataWindow(), QRect(7, 3, 16, 8));
    }
    {
        // A single pixel data window is legal.
        ExrBuilder builder;
        builder.setSize(1, 1);
        const QByteArray data = builder.build();
        QSSGExrReader reader;
        QVERIFY2(reader.parseHeader(data), qPrintable(reader.errorMessage()));
        QCOMPARE(reader.size(), QSize(1, 1));
    }
    {
        // A negative origin is legal too.
        ExrBuilder builder;
        builder.setSize(4, 4).setOrigin(-4, -4);
        const QByteArray data = builder.build();
        QSSGExrReader reader;
        QVERIFY2(reader.parseHeader(data), qPrintable(reader.errorMessage()));
        QCOMPARE(reader.dataWindow(), QRect(-4, -4, 4, 4));
    }
}

void tst_QSSGExrReader::lineOrderValues_data()
{
    QTest::addColumn<quint8>("value");
    QTest::addColumn<bool>("accepted");

    QTest::newRow("increasingY") << quint8(0) << true;
    QTest::newRow("decreasingY") << quint8(1) << true;
    QTest::newRow("randomY") << quint8(2) << true;
    QTest::newRow("three") << quint8(3) << false;
    QTest::newRow("max") << quint8(255) << false;
}

void tst_QSSGExrReader::lineOrderValues()
{
    QFETCH(quint8, value);
    QFETCH(bool, accepted);

    const QByteArray data = ExrBuilder().setSize(8, 4).setLineOrder(value).build();
    QSSGExrReader reader;
    QCOMPARE(reader.parseHeader(data), accepted);
    if (accepted)
        QCOMPARE(quint8(reader.lineOrder()), value);
    else
        QVERIFY(reader.errorMessage().contains("line order"_L1));
}

void tst_QSSGExrReader::decodeBuilt_data()
{
    QTest::addColumn<quint8>("compression");
    QTest::addColumn<ExrBuilder::PixelType>("pixelType");
    QTest::addColumn<int>("width");
    QTest::addColumn<int>("height");
    QTest::addColumn<int>("tileWidth");
    QTest::addColumn<int>("tileHeight");

    struct
    {
        const char *name;
        quint8 id;
    } codecs[] = { { "none", 0 }, { "rle", 1 }, { "zips", 2 }, { "zip", 3 } };

    for (const auto &codec : codecs) {
        for (const auto pt : { ExrBuilder::Half, ExrBuilder::Float }) {
            const char *ptName = pt == ExrBuilder::Half ? "half" : "float";
            // 40 rows makes zip take three blocks, the last one short.
            QTest::addRow("%s-%s-scanline", codec.name, ptName) << codec.id << pt << 16 << 40 << 0 << 0;
            QTest::addRow("%s-%s-1x1", codec.name, ptName) << codec.id << pt << 1 << 1 << 0 << 0;
            QTest::addRow("%s-%s-1x17", codec.name, ptName) << codec.id << pt << 1 << 17 << 0 << 0;
            QTest::addRow("%s-%s-17x1", codec.name, ptName) << codec.id << pt << 17 << 1 << 0 << 0;
            // Partial right column and bottom row of tiles.
            QTest::addRow("%s-%s-tiled", codec.name, ptName) << codec.id << pt << 17 << 9 << 8 << 8;
        }
    }
}

void tst_QSSGExrReader::decodeBuilt()
{
    QFETCH(quint8, compression);
    QFETCH(ExrBuilder::PixelType, pixelType);
    QFETCH(int, width);
    QFETCH(int, height);
    QFETCH(int, tileWidth);
    QFETCH(int, tileHeight);

    ExrBuilder builder;
    builder.setSize(width, height)
            .setCompression(compression)
            .setChannels({ { "B"_ba, pixelType, 1, 1 }, { "G"_ba, pixelType, 1, 1 }, { "R"_ba, pixelType, 1, 1 } });
    if (tileWidth > 0)
        builder.setTiles(tileWidth, tileHeight);

    verifyDecode(builder.build(), width, height, 3, false);
}

void tst_QSSGExrReader::decodeFixture_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<int>("width");
    QTest::addColumn<int>("height");
    QTest::addColumn<bool>("singleChannel");

    QTest::newRow("piz-half-scanline") << u"piz_half_scanline_rgb_16x40.exr"_s << 16 << 40 << false;
    QTest::newRow("piz-half-extra-channels") << u"piz_half_scanline_extra_16x24.exr"_s << 16 << 24 << false;
    QTest::newRow("piz-half-odd-geometry") << u"piz_half_scanline_rgb_33x33.exr"_s << 33 << 33 << false;
    QTest::newRow("piz-float-1x1") << u"piz_float_scanline_r_1x1.exr"_s << 1 << 1 << true;
    QTest::newRow("piz-float-tiled") << u"piz_float_tiled_rgba_17x9_t8.exr"_s << 17 << 9 << false;
    QTest::newRow("piz-half-offset-window") << u"piz_half_scanline_offset_16x8.exr"_s << 16 << 8 << false;
    QTest::newRow("zip-half-tiled") << u"zip_half_tiled_rgba_17x9_t8.exr"_s << 17 << 9 << false;
    QTest::newRow("zip-half-tile-1x1") << u"zip_half_tiled_rgb_9x5_t1.exr"_s << 9 << 5 << false;
    QTest::newRow("zip-half-tile-larger-than-image") << u"zip_half_tiled_rgb_9x5_t64.exr"_s << 9 << 5 << false;
    QTest::newRow("zip-half-mipmap") << u"zip_half_mipmap_rgb_16x16_t8.exr"_s << 16 << 16 << false;
    QTest::newRow("zip-half-ripmap") << u"zip_half_ripmap_rgb_16x8_t8.exr"_s << 16 << 8 << false;
    QTest::newRow("zip-half-decreasing-line-order") << u"zip_half_scanline_decreasing_16x40.exr"_s << 16 << 40 << false;
    QTest::newRow("rle-half-reference") << u"rle_half_scanline_rgb_16x24.exr"_s << 16 << 24 << false;
    QTest::newRow("zips-half-reference") << u"zips_half_scanline_rgb_16x24.exr"_s << 16 << 24 << false;
    QTest::newRow("zip-float-rgba") << u"zip_float_scanline_rgba_16x8.exr"_s << 16 << 8 << false;
    QTest::newRow("none-half-single") << u"none_half_scanline_r_16x8.exr"_s << 16 << 8 << true;
}

void tst_QSSGExrReader::decodeFixture()
{
    QFETCH(QString, file);
    QFETCH(int, width);
    QFETCH(int, height);
    QFETCH(bool, singleChannel);

    const QByteArray data = fixture(file);
    QVERIFY2(!data.isEmpty(), qPrintable(u"missing fixture "_s + file));
    verifyDecode(data, width, height, 0, singleChannel);
}

void tst_QSSGExrReader::channelSelection()
{
    // Alpha too, the baseline images depend on it.
    {
        ExrBuilder builder;
        builder.setSize(4, 4).setChannels({ { "Y"_ba, ExrBuilder::Half, 1, 1 } });
        const QByteArray data = builder.build();
        QSSGExrReader reader;
        QVERIFY2(reader.parseHeader(data), qPrintable(reader.errorMessage()));
        const auto map = qssgExrRgbaChannelMap(reader);
        QCOMPARE(map.source[0], 0);
        QCOMPARE(map.source[1], 0);
        QCOMPARE(map.source[2], 0);
        QCOMPARE(map.source[3], 0);
    }
    // Extra channels must not shift the ones we want.
    {
        ExrBuilder builder;
        builder.setSize(4, 4).setChannels({ { "A"_ba, ExrBuilder::Half, 1, 1 },
                                            { "B"_ba, ExrBuilder::Half, 1, 1 },
                                            { "G"_ba, ExrBuilder::Half, 1, 1 },
                                            { "R"_ba, ExrBuilder::Half, 1, 1 },
                                            { "Z"_ba, ExrBuilder::Half, 1, 1 } });
        const QByteArray data = builder.build();
        QSSGExrReader reader;
        QVERIFY2(reader.parseHeader(data), qPrintable(reader.errorMessage()));
        const auto map = qssgExrRgbaChannelMap(reader);
        QCOMPARE(map.source[0], 3); // R
        QCOMPARE(map.source[1], 2); // G
        QCOMPARE(map.source[2], 1); // B
        QCOMPARE(map.source[3], 0); // A
    }
    // Two channels named something else used to index the plane array with -1.
    {
        ExrBuilder builder;
        builder.setSize(4, 4).setChannels({ { "A"_ba, ExrBuilder::Half, 1, 1 }, { "Y"_ba, ExrBuilder::Half, 1, 1 } });
        const QByteArray data = builder.build();
        QSSGExrReader reader;
        QVERIFY2(reader.parseHeader(data), qPrintable(reader.errorMessage()));
        const auto map = qssgExrRgbaChannelMap(reader);
        QCOMPARE(map.source[0], -1);
        QCOMPARE(map.source[1], -1);
        QCOMPARE(map.source[2], -1);
        QList<float> pixels(4 * 4 * 4, -1.0f);
        QVERIFY2(reader.decodeToRgba32F(map, QSSGExrReader::NoFlags, pixels.data(), pixels.size()),
                 qPrintable(reader.errorMessage()));
        QCOMPARE(pixels[0], 0.0f);
        QCOMPARE(pixels[1], 0.0f);
        QCOMPARE(pixels[2], 0.0f);
        QCOMPARE(pixels[3], ExrBuilder::sample(0, 0, 0)); // A is channel 0
    }
    {
        ExrBuilder builder;
        builder.setSize(4, 4).setChannels(
                { { "B"_ba, ExrBuilder::Uint, 1, 1 }, { "G"_ba, ExrBuilder::Uint, 1, 1 }, { "R"_ba, ExrBuilder::Uint, 1, 1 } });
        const QByteArray data = builder.build();
        QSSGExrReader reader;
        QVERIFY2(reader.parseHeader(data), qPrintable(reader.errorMessage()));
        QList<float> pixels(4 * 4 * 4, 0.0f);
        QVERIFY(!reader.decodeToRgba32F(qssgExrRgbaChannelMap(reader), QSSGExrReader::NoFlags, pixels.data(), pixels.size()));
        QVERIFY(reader.errorMessage().contains("integer"_L1));
    }
}

void tst_QSSGExrReader::halfConversionIsExhaustive()
{
    // Every half bit pattern, with infinities clamped and NaNs zeroed.
    const int w = 256;
    const int h = 256;
    QByteArray raw;
    raw.reserve(qsizetype(w) * h * 2);
    for (int i = 0; i < 65536; ++i) {
        char buf[2];
        qToLittleEndian(quint16(i), buf);
        raw.append(buf, 2);
    }

    ExrBuilder builder;
    builder.setSize(w, h).setChannels({ { "R"_ba, ExrBuilder::Half, 1, 1 } }).setCompression(0);
    QByteArray data = builder.build();
    // NONE stores rows verbatim, one row of 256 halves per chunk.
    for (int row = 0; row < h; ++row) {
        const qint64 payload = builder.offsetOf("chunk:"_ba + QByteArray::number(row) + ":payload"_ba);
        QVERIFY(payload > 0);
        memcpy(data.data() + payload, raw.constData() + qsizetype(row) * w * 2, size_t(w) * 2);
    }

    QSSGExrReader reader;
    QVERIFY2(reader.parseHeader(data), qPrintable(reader.errorMessage()));
    QList<float> pixels(qsizetype(w) * h * 4, 0.0f);
    QVERIFY2(reader.decodeToRgba32F(qssgExrRgbaChannelMap(reader), QSSGExrReader::NoFlags, pixels.data(), pixels.size()),
             qPrintable(reader.errorMessage()));

    for (int i = 0; i < 65536; ++i) {
        qfloat16 h16;
        memcpy(&h16, raw.constData() + qsizetype(i) * 2, 2);
        float expected = float(h16);
        if (qIsNaN(expected))
            expected = 0.0f;
        else if (qIsInf(expected))
            expected = expected > 0.0f ? 65504.0f : -65504.0f;
        QCOMPARE(pixels[qsizetype(i) * 4], expected);
    }
}

void tst_QSSGExrReader::nonFiniteValuesAreSanitised()
{
    const QByteArray data = fixture(u"zip_float_scanline_specials_8x8.exr"_s);
    QVERIFY(!data.isEmpty());

    QSSGExrReader reader;
    QVERIFY2(reader.parseHeader(data), qPrintable(reader.errorMessage()));
    QList<float> pixels(8 * 8 * 4, 0.0f);
    QVERIFY2(reader.decodeToRgba32F(qssgExrRgbaChannelMap(reader), QSSGExrReader::NoFlags, pixels.data(), pixels.size()),
             qPrintable(reader.errorMessage()));

    // The last two samples are the infinities.
    const float floatMax = std::numeric_limits<float>::max();
    const float specials[8] = { 0.0f, -0.0f, 1.17549435e-38f, 65504.0f, 1e30f, -1e30f, floatMax, -floatMax };
    for (int i = 0; i < 64; ++i) {
        for (int k = 0; k < 4; ++k)
            QCOMPARE(pixels[qsizetype(i) * 4 + k], specials[i % 8]);
    }
    for (const float v : pixels)
        QVERIFY(qIsFinite(v));

    // The FLOAT NaN branch: the fixture above has the infinities but no NaN.
    {
        // +inf, -inf, quiet NaN, signed quiet NaN, signalling NaN, -FLT_MAX,
        // the smallest subnormal, 1.0.
        const quint32 patterns[8] = { 0x7f800000u, 0xff800000u, 0x7fc00000u, 0xffc00000u,
                                      0x7f800001u, 0xff7fffffu, 0x00000001u, 0x3f800000u };

        ExrBuilder builder;
        builder.setSize(8, 1).setChannels({ { "R"_ba, ExrBuilder::Float, 1, 1 } }).setCompression(0);
        QByteArray built = builder.build();
        const qint64 payload = builder.offsetOf("chunk:0:payload"_ba);
        QVERIFY(payload > 0);
        for (int i = 0; i < 8; ++i)
            qToLittleEndian(patterns[i], built.data() + payload + i * 4);

        QSSGExrReader floatReader;
        QVERIFY2(floatReader.parseHeader(built), qPrintable(floatReader.errorMessage()));
        QList<float> floatPixels(8 * 4, 0.0f);
        QVERIFY2(floatReader.decodeToRgba32F(qssgExrRgbaChannelMap(floatReader), QSSGExrReader::NoFlags,
                                             floatPixels.data(), floatPixels.size()),
                 qPrintable(floatReader.errorMessage()));

        for (int i = 0; i < 8; ++i) {
            float expected;
            memcpy(&expected, &patterns[i], sizeof(expected));
            if (qIsNaN(expected))
                expected = 0.0f;
            else if (qIsInf(expected))
                expected = expected > 0.0f ? floatMax : -floatMax;
            // A lone channel, so every component samples it.
            for (int k = 0; k < 4; ++k)
                QCOMPARE(floatPixels[qsizetype(i) * 4 + k], expected);
        }
    }
}

void tst_QSSGExrReader::flipVertically()
{
    ExrBuilder builder;
    builder.setSize(4, 4).setCompression(3);
    const QByteArray data = builder.build();

    QSSGExrReader a, b;
    QVERIFY2(a.parseHeader(data), qPrintable(a.errorMessage()));
    QVERIFY2(b.parseHeader(data), qPrintable(b.errorMessage()));

    QList<float> normal(4 * 4 * 4, 0.0f);
    QList<float> flipped(4 * 4 * 4, 0.0f);
    QVERIFY(a.decodeToRgba32F(qssgExrRgbaChannelMap(a), QSSGExrReader::NoFlags, normal.data(), normal.size()));
    QVERIFY(b.decodeToRgba32F(qssgExrRgbaChannelMap(b), QSSGExrReader::FlipVertically, flipped.data(), flipped.size()));

    for (int y = 0; y < 4; ++y) {
        for (int x = 0; x < 4 * 4; ++x)
            QCOMPARE(flipped[qsizetype(3 - y) * 16 + x], normal[qsizetype(y) * 16 + x]);
    }
}

void tst_QSSGExrReader::chunkStorageOrderIsIrrelevant_data()
{
    QTest::addColumn<bool>("tiled");
    QTest::addColumn<quint8>("compression");

    QTest::newRow("scanline-none") << false << quint8(0);
    QTest::newRow("scanline-zip") << false << quint8(3);
    QTest::newRow("tiled-none") << true << quint8(0);
    QTest::newRow("tiled-zip") << true << quint8(3);
}

void tst_QSSGExrReader::chunkStorageOrderIsIrrelevant()
{
    QFETCH(bool, tiled);
    QFETCH(quint8, compression);

    constexpr int w = 17;
    constexpr int h = 37;

    const auto make = [&](bool reversed, quint8 order) {
        ExrBuilder builder;
        builder.setSize(w, h).setCompression(compression).setLineOrder(order).setReverseChunkStorage(reversed);
        if (tiled)
            builder.setTiles(8, 8);
        return builder.build();
    };

    const QByteArray forward = make(false, 0);
    const QByteArray backward = make(true, 2);

    QCOMPARE(backward.size(), forward.size());
    QVERIFY(make(false, 2) != backward);

    QSSGExrReader a, b;
    QVERIFY2(a.parseHeader(forward), qPrintable(a.errorMessage()));
    QVERIFY2(b.parseHeader(backward), qPrintable(b.errorMessage()));
    QCOMPARE(b.lineOrder(), QSSGExrReader::LineOrder::RandomY);

    QList<float> first(qsizetype(w) * h * 4, 0.0f);
    QList<float> second(qsizetype(w) * h * 4, 0.0f);
    QVERIFY2(a.decodeToRgba32F(qssgExrRgbaChannelMap(a), QSSGExrReader::NoFlags, first.data(), first.size()),
             qPrintable(a.errorMessage()));
    QVERIFY2(b.decodeToRgba32F(qssgExrRgbaChannelMap(b), QSSGExrReader::NoFlags, second.data(), second.size()),
             qPrintable(b.errorMessage()));

    QCOMPARE(second, first);
    verifyDecode(backward, w, h, 3, false);
}

void tst_QSSGExrReader::errorIsClearedBetweenDecodes()
{
    const QByteArray data = ExrBuilder().setSize(8, 4).build();
    QSSGExrReader reader;
    QVERIFY2(reader.parseHeader(data), qPrintable(reader.errorMessage()));
    QVERIFY(reader.errorMessage().isEmpty());

    QList<float> pixels(8 * 4 * 4, 0.0f);

    QSSGExrReader::ChannelMap absent;
    absent.source[0] = qint32(reader.channels().size());
    QVERIFY(!reader.decodeToRgba32F(absent, QSSGExrReader::NoFlags, pixels.data(), pixels.size()));
    const QString firstError = reader.errorMessage();
    QVERIFY(firstError.contains("absent"_L1));

    QVERIFY(!reader.decodeToRgba32F(qssgExrRgbaChannelMap(reader), QSSGExrReader::NoFlags, pixels.data(), 4));
    QVERIFY(reader.errorMessage().contains("too small"_L1));

    QVERIFY2(reader.decodeToRgba32F(qssgExrRgbaChannelMap(reader), QSSGExrReader::NoFlags, pixels.data(), pixels.size()),
             qPrintable(reader.errorMessage()));
    QVERIFY(reader.errorMessage().isEmpty());
}

void tst_QSSGExrReader::headerIsResetBetweenParses()
{
    QSSGExrReader reader;
    QSSGExrReader::Limits limits;
    limits.maxChannels = 8;
    reader.setLimits(limits);

    const QByteArray tiled = ExrBuilder().setSize(16, 16).setTiles(8, 8).setLineOrder(2).build();
    QVERIFY2(reader.parseHeader(tiled), qPrintable(reader.errorMessage()));
    QVERIFY(reader.isTiled());
    QCOMPARE(reader.lineOrder(), QSSGExrReader::LineOrder::RandomY);

    // lineOrder is optional, so a stale value would otherwise survive.
    const QByteArray scanline = ExrBuilder().setSize(8, 4).dropAttribute("lineOrder"_ba).build();
    QVERIFY2(reader.parseHeader(scanline), qPrintable(reader.errorMessage()));
    QVERIFY(!reader.isTiled());
    QVERIFY(!reader.tileSize().isValid());
    QCOMPARE(reader.lineOrder(), QSSGExrReader::LineOrder::IncreasingY);
    QCOMPARE(reader.size(), QSize(8, 4));
    QCOMPARE(reader.limits().maxChannels, 8);

    QList<float> pixels(8 * 4 * 4, 0.0f);
    QVERIFY2(reader.decodeToRgba32F(qssgExrRgbaChannelMap(reader), QSSGExrReader::NoFlags, pixels.data(), pixels.size()),
             qPrintable(reader.errorMessage()));
}

void tst_QSSGExrReader::badMagic_data()
{
    QTest::addColumn<QByteArray>("data");

    QTest::newRow("empty") << QByteArray();
    QTest::newRow("one-byte") << QByteArray(1, '\x76');
    QTest::newRow("two-bytes") << QByteArray::fromHex("762f");
    QTest::newRow("three-bytes") << QByteArray::fromHex("762f31");
    QTest::newRow("byte-swapped-magic") << QByteArray::fromHex("01312f7600000002");
    QTest::newRow("zeroes") << QByteArray(64, '\0');
    QTest::newRow("png") << QByteArray::fromHex("89504e470d0a1a0a") + QByteArray(56, '\0');
    // Must be rejected, because loadHdrImage() sniffs .hdr through this path.
    QTest::newRow("radiance-hdr") << QByteArray("#?RADIANCE\nFORMAT=32-bit_rle_rgbe\n\n-Y 8 +X 8\n");
}

void tst_QSSGExrReader::badMagic()
{
    QFETCH(QByteArray, data);
    QSSGExrReader reader;
    QVERIFY(!reader.parseHeader(data));
    QVERIFY(!reader.errorMessage().isEmpty());
    if (data.size() >= 4)
        QVERIFY(!QSSGExrReader::isExr(data));
}

void tst_QSSGExrReader::truncatedAtEveryOffset_data()
{
    QTest::addColumn<QByteArray>("good");

    QTest::newRow("none") << ExrBuilder().setSize(8, 6).setCompression(0).build();
    QTest::newRow("rle") << ExrBuilder().setSize(8, 6).setCompression(1).build();
    QTest::newRow("zips") << ExrBuilder().setSize(8, 6).setCompression(2).build();
    QTest::newRow("zip") << ExrBuilder().setSize(8, 20).setCompression(3).build();
    QTest::newRow("tiled") << ExrBuilder().setSize(9, 5).setCompression(3).setTiles(4, 4).build();
    QTest::newRow("piz-fixture") << fixture(u"piz_half_scanline_rgb_16x40.exr"_s);
}

void tst_QSSGExrReader::truncatedAtEveryOffset()
{
    QFETCH(QByteArray, good);
    QVERIFY(!good.isEmpty());

    // Every prefix of a valid file has to fail with a message.
    for (qsizetype n = 0; n < good.size(); ++n) {
        QSSGExrReader reader;
        const QByteArray truncated = good.left(n);
        bool ok = reader.parseHeader(truncated);
        if (ok) {
            QList<float> pixels(qsizetype(reader.size().width()) * reader.size().height() * 4, 0.0f);
            ok = reader.decodeToRgba32F(qssgExrRgbaChannelMap(reader), QSSGExrReader::NoFlags, pixels.data(), pixels.size());
        }
        QVERIFY2(!ok, qPrintable(QStringLiteral("prefix of %1 bytes was accepted").arg(n)));
        QVERIFY2(!reader.errorMessage().isEmpty(), qPrintable(QStringLiteral("prefix of %1 bytes failed silently").arg(n)));
    }
}

void tst_QSSGExrReader::unsupportedFlags_data()
{
    QTest::addColumn<quint32>("versionField");
    QTest::addColumn<QString>("expected");

    QTest::newRow("multipart") << quint32(2 | 0x1000) << u"multi part"_s;
    QTest::newRow("deep") << quint32(2 | 0x800) << u"deep"_s;
    QTest::newRow("multipart-and-deep") << quint32(2 | 0x1800) << u"multi part"_s;
    QTest::newRow("unknown-bit") << quint32(2 | 0x4000) << u"flags"_s;
}

void tst_QSSGExrReader::unsupportedFlags()
{
    QFETCH(quint32, versionField);
    QFETCH(QString, expected);

    const QByteArray data = ExrBuilder().setVersionField(versionField).build();
    QSSGExrReader reader;
    QVERIFY(!reader.parseHeader(data));
    QVERIFY2(reader.errorMessage().contains(expected), qPrintable(reader.errorMessage()));
}

void tst_QSSGExrReader::unsupportedCompression_data()
{
    QTest::addColumn<quint8>("id");
    QTest::addColumn<QString>("expected");

    QTest::newRow("pxr24") << quint8(5) << u"PXR24"_s;
    QTest::newRow("b44") << quint8(6) << u"B44"_s;
    QTest::newRow("b44a") << quint8(7) << u"B44A"_s;
    QTest::newRow("dwaa") << quint8(8) << u"DWAA"_s;
    QTest::newRow("dwab") << quint8(9) << u"DWAB"_s;
    QTest::newRow("unknown-10") << quint8(10) << u"unknown compression"_s;
    QTest::newRow("unknown-127") << quint8(127) << u"unknown compression"_s;
    QTest::newRow("unknown-128") << quint8(128) << u"unknown compression"_s;
    QTest::newRow("unknown-255") << quint8(255) << u"unknown compression"_s;
}

void tst_QSSGExrReader::unsupportedCompression()
{
    QFETCH(quint8, id);
    QFETCH(QString, expected);

    const QByteArray data = ExrBuilder().setCompression(id).build();
    QSSGExrReader reader;
    QVERIFY(!reader.parseHeader(data));
    QVERIFY2(reader.errorMessage().contains(expected), qPrintable(reader.errorMessage()));
}

void tst_QSSGExrReader::unsupportedCompressionFixture_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("expected");

    QTest::newRow("pxr24") << u"unsupported_pxr24.exr"_s << u"PXR24"_s;
    QTest::newRow("b44") << u"unsupported_b44.exr"_s << u"B44"_s;
    QTest::newRow("b44a") << u"unsupported_b44a.exr"_s << u"B44A"_s;
    QTest::newRow("dwaa") << u"unsupported_dwaa.exr"_s << u"DWAA"_s;
    QTest::newRow("dwab") << u"unsupported_dwab.exr"_s << u"DWAB"_s;
}

void tst_QSSGExrReader::unsupportedCompressionFixture()
{
    QFETCH(QString, file);
    QFETCH(QString, expected);

    const QByteArray data = fixture(file);
    QVERIFY(!data.isEmpty());
    QSSGExrReader reader;
    QVERIFY(!reader.parseHeader(data));
    QVERIFY2(reader.errorMessage().contains(expected), qPrintable(reader.errorMessage()));
}

void tst_QSSGExrReader::missingRequiredAttribute_data()
{
    QTest::addColumn<QByteArray>("attribute");
    QTest::addColumn<bool>("required");

    QTest::newRow("channels") << "channels"_ba << true;
    QTest::newRow("compression") << "compression"_ba << true;
    QTest::newRow("dataWindow") << "dataWindow"_ba << true;
    // Required by the specification, but real writers omit them.
    QTest::newRow("displayWindow") << "displayWindow"_ba << false;
    QTest::newRow("lineOrder") << "lineOrder"_ba << false;
    QTest::newRow("pixelAspectRatio") << "pixelAspectRatio"_ba << false;
    QTest::newRow("screenWindowCenter") << "screenWindowCenter"_ba << false;
    QTest::newRow("screenWindowWidth") << "screenWindowWidth"_ba << false;
}

void tst_QSSGExrReader::missingRequiredAttribute()
{
    QFETCH(QByteArray, attribute);
    QFETCH(bool, required);

    const QByteArray data = ExrBuilder().setSize(8, 4).dropAttribute(attribute).build();
    QSSGExrReader reader;
    const bool ok = reader.parseHeader(data);
    if (required) {
        QVERIFY(!ok);
        QVERIFY2(reader.errorMessage().contains(QString::fromLatin1(attribute)) || reader.errorMessage().contains("no channel list"_L1)
                         || reader.errorMessage().contains("no compression"_L1) || reader.errorMessage().contains("no data window"_L1),
                 qPrintable(reader.errorMessage()));
    } else {
        QVERIFY2(ok, qPrintable(reader.errorMessage()));
    }
}

void tst_QSSGExrReader::malformedAttributes_data()
{
    QTest::addColumn<QByteArray>("data");

    {
        // A name with no terminator, running to the end of the file.
        QByteArray d = ExrBuilder().build();
        const qint64 at = ExrBuilder().offsetOf("magic"_ba);
        Q_UNUSED(at);
        d = d.left(8) + QByteArray(400, 'x');
        QTest::newRow("unterminated-name") << d;
    }
    {
        ExrBuilder b;
        QByteArray d = b.build();
        const qint64 sizeAt = b.offsetOf("attr:dataWindow:size"_ba);
        qToLittleEndian(qint32(-1), d.data() + sizeAt);
        QTest::newRow("negative-attribute-size") << d;
    }
    {
        ExrBuilder b;
        QByteArray d = b.build();
        const qint64 sizeAt = b.offsetOf("attr:dataWindow:size"_ba);
        qToLittleEndian(qint32(1), d.data() + sizeAt);
        QTest::newRow("box2i-declared-as-one-byte") << d;
    }
    {
        ExrBuilder b;
        QByteArray d = b.build();
        const qint64 sizeAt = b.offsetOf("attr:dataWindow:size"_ba);
        qToLittleEndian(qint32(d.size()), d.data() + sizeAt);
        QTest::newRow("attribute-size-overruns-file") << d;
    }
    {
        ExrBuilder b;
        QByteArray d = b.build();
        const qint64 sizeAt = b.offsetOf("attr:dataWindow:size"_ba);
        qToLittleEndian(std::numeric_limits<qint32>::max(), d.data() + sizeAt);
        QTest::newRow("attribute-size-int-max") << d;
    }
    {
        // Long names without the flag that permits them.
        ExrBuilder b;
        b.addRawAttribute(QByteArray(200, 'n'), "string"_ba, "x"_ba);
        QTest::newRow("long-name-without-flag") << b.build();
    }
    {
        // An attribute count bomb.
        ExrBuilder b;
        for (int i = 0; i < 4000; ++i)
            b.addRawAttribute(QByteArray("a") + QByteArray::number(i), "int"_ba, QByteArray());
        QTest::newRow("too-many-attributes") << b.build();
    }
}

void tst_QSSGExrReader::malformedAttributes()
{
    QFETCH(QByteArray, data);
    QSSGExrReader reader;
    QVERIFY(!reader.parseHeader(data));
    QVERIFY(!reader.errorMessage().isEmpty());
}

void tst_QSSGExrReader::badGeometry_data()
{
    QTest::addColumn<qint32>("xMin");
    QTest::addColumn<qint32>("yMin");
    QTest::addColumn<qint32>("xMax");
    QTest::addColumn<qint32>("yMax");
    QTest::addColumn<bool>("valid");

    QTest::newRow("single-pixel") << 0 << 0 << 0 << 0 << true;
    QTest::newRow("inverted-x") << 8 << 0 << 0 << 8 << false;
    QTest::newRow("inverted-y") << 0 << 8 << 8 << 0 << false;
    QTest::newRow("full-int-range") << std::numeric_limits<qint32>::min() << std::numeric_limits<qint32>::min()
                                    << std::numeric_limits<qint32>::max() << std::numeric_limits<qint32>::max() << false;
    QTest::newRow("width-plus-one-overflows") << 0 << 0 << std::numeric_limits<qint32>::max() << 0 << false;
    QTest::newRow("past-max-width") << 0 << 0 << 200000 << 0 << false;
    QTest::newRow("past-max-pixels") << 0 << 0 << 60000 << 60000 << false;
}

void tst_QSSGExrReader::badGeometry()
{
    QFETCH(qint32, xMin);
    QFETCH(qint32, yMin);
    QFETCH(qint32, xMax);
    QFETCH(qint32, yMax);
    QFETCH(bool, valid);

    QByteArray dataWindow;
    appendI32(&dataWindow, xMin);
    appendI32(&dataWindow, yMin);
    appendI32(&dataWindow, xMax);
    appendI32(&dataWindow, yMax);

    ExrBuilder b;
    b.setSize(1, 1).dropAttribute("dataWindow"_ba).addRawAttribute("dataWindow"_ba, "box2i"_ba, dataWindow);
    const QByteArray data = b.build();
    QSSGExrReader reader;
    const bool ok = reader.parseHeader(data);
    if (valid) {
        QVERIFY2(ok, qPrintable(reader.errorMessage()));
    } else {
        QVERIFY(!ok);
        QVERIFY(!reader.errorMessage().isEmpty());
    }
}

void tst_QSSGExrReader::badChannels_data()
{
    QTest::addColumn<QByteArray>("chlist");
    QTest::addColumn<QString>("expected");

    auto channel = [](const QByteArray &name, qint32 type, qint32 xs = 1, qint32 ys = 1) {
        QByteArray c;
        c.append(name);
        c.append('\0');
        appendI32(&c, type);
        c.append(char(0));
        c.append(3, '\0');
        appendI32(&c, xs);
        appendI32(&c, ys);
        return c;
    };

    QTest::newRow("no-channels") << QByteArray(1, '\0') << u"no channels"_s;
    QTest::newRow("unknown-pixel-type") << channel("R"_ba, 3) + QByteArray(1, '\0') << u"pixel type"_s;
    QTest::newRow("negative-pixel-type") << channel("R"_ba, -1) + QByteArray(1, '\0') << u"pixel type"_s;
    QTest::newRow("x-subsampled") << channel("R"_ba, 1, 2, 1) + QByteArray(1, '\0') << u"subsampled"_s;
    QTest::newRow("y-subsampled") << channel("R"_ba, 1, 1, 2) + QByteArray(1, '\0') << u"subsampled"_s;
    QTest::newRow("zero-sampling") << channel("R"_ba, 1, 0, 0) + QByteArray(1, '\0') << u"subsampled"_s;
    QTest::newRow("duplicate-names") << channel("R"_ba, 1) + channel("R"_ba, 1) + QByteArray(1, '\0') << u"sorted"_s;
    QTest::newRow("unsorted-names") << channel("R"_ba, 1) + channel("G"_ba, 1) + QByteArray(1, '\0') << u"sorted"_s;
    QTest::newRow("unterminated-list") << channel("R"_ba, 1) << u""_s;

    {
        QByteArray many;
        for (int i = 0; i < 200; ++i)
            many.append(channel(QByteArray::number(100000 + i), 1));
        many.append('\0');
        QTest::newRow("too-many-channels") << many << u"too many channels"_s;
    }
}

void tst_QSSGExrReader::badChannels()
{
    QFETCH(QByteArray, chlist);
    QFETCH(QString, expected);

    ExrBuilder b;
    b.setSize(4, 4).dropAttribute("channels"_ba).addRawAttribute("channels"_ba, "chlist"_ba, chlist);
    const QByteArray data = b.build();
    QSSGExrReader reader;
    QVERIFY(!reader.parseHeader(data));
    QVERIFY(!reader.errorMessage().isEmpty());
    if (!expected.isEmpty())
        QVERIFY2(reader.errorMessage().contains(expected), qPrintable(reader.errorMessage()));
}

void tst_QSSGExrReader::badOffsetTable_data()
{
    QTest::addColumn<int>("entry");
    QTest::addColumn<quint64>("value");

    QTest::newRow("zero") << 0 << quint64(0);
    QTest::newRow("one") << 0 << quint64(1);
    QTest::newRow("into-the-header") << 0 << quint64(16);
    QTest::newRow("past-eof") << 0 << quint64(1 << 30);
    QTest::newRow("uint64-max") << 0 << std::numeric_limits<quint64>::max();
    QTest::newRow("second-entry-zero") << 1 << quint64(0);
    QTest::newRow("second-entry-past-eof") << 1 << quint64(1 << 30);
}

void tst_QSSGExrReader::badOffsetTable()
{
    QFETCH(int, entry);
    QFETCH(quint64, value);

    ExrBuilder b;
    b.setSize(8, 40).setCompression(3);
    QByteArray data = b.build();
    const qint64 table = b.offsetOf("offsetTable"_ba);
    QVERIFY(table > 0);
    qToLittleEndian(value, data.data() + table + entry * 8);

    QSSGExrReader reader;
    bool ok = reader.parseHeader(data);
    if (ok) {
        QList<float> pixels(8 * 40 * 4, 0.0f);
        ok = reader.decodeToRgba32F(qssgExrRgbaChannelMap(reader), QSSGExrReader::NoFlags, pixels.data(), pixels.size());
    }
    QVERIFY(!ok);
    QVERIFY(!reader.errorMessage().isEmpty());
}

void tst_QSSGExrReader::badChunks_data()
{
    QTest::addColumn<QByteArray>("data");

    auto zipFile = [] {
        ExrBuilder b;
        b.setSize(8, 40).setCompression(3);
        return b;
    };

    {
        ExrBuilder b = zipFile();
        QByteArray d = b.build();
        qToLittleEndian(qint32(-1), d.data() + b.offsetOf("chunk:0:dataSize"_ba));
        QTest::newRow("negative-data-size") << d;
    }
    {
        ExrBuilder b = zipFile();
        QByteArray d = b.build();
        qToLittleEndian(qint32(0), d.data() + b.offsetOf("chunk:0:dataSize"_ba));
        QTest::newRow("zero-data-size") << d;
    }
    {
        ExrBuilder b = zipFile();
        QByteArray d = b.build();
        qToLittleEndian(std::numeric_limits<qint32>::max(), d.data() + b.offsetOf("chunk:0:dataSize"_ba));
        QTest::newRow("int-max-data-size") << d;
    }
    {
        ExrBuilder b = zipFile();
        QByteArray d = b.build();
        // Larger than the uncompressed block, which a writer would store raw.
        qToLittleEndian(qint32(8 * 16 * 6 + 1), d.data() + b.offsetOf("chunk:0:dataSize"_ba));
        QTest::newRow("data-size-larger-than-block") << d;
    }
    {
        ExrBuilder b = zipFile();
        QByteArray d = b.build();
        qToLittleEndian(qint32(1000), d.data() + b.offsetOf("chunk:0"_ba));
        QTest::newRow("row-outside-data-window") << d;
    }
    {
        ExrBuilder b = zipFile();
        QByteArray d = b.build();
        qToLittleEndian(qint32(-1000), d.data() + b.offsetOf("chunk:0"_ba));
        QTest::newRow("row-below-data-window") << d;
    }
    {
        ExrBuilder b = zipFile();
        QByteArray d = b.build();
        qToLittleEndian(std::numeric_limits<qint32>::min(), d.data() + b.offsetOf("chunk:0"_ba));
        QTest::newRow("row-int-min") << d;
    }
    {
        ExrBuilder b = zipFile();
        QByteArray d = b.build();
        // Not on a 16 row block boundary.
        qToLittleEndian(qint32(1), d.data() + b.offsetOf("chunk:0"_ba));
        QTest::newRow("row-not-block-aligned") << d;
    }
    {
        ExrBuilder b = zipFile();
        QByteArray d = b.build();
        // Two chunks claiming the same rows.
        qToLittleEndian(qint32(0), d.data() + b.offsetOf("chunk:1"_ba));
        QTest::newRow("duplicate-row-block") << d;
    }
    {
        ExrBuilder b;
        b.setSize(9, 5).setCompression(3).setTiles(4, 4);
        QByteArray d = b.build();
        qToLittleEndian(qint32(99), d.data() + b.offsetOf("chunk:0"_ba));
        QTest::newRow("tile-index-out-of-range") << d;
    }
    {
        ExrBuilder b;
        b.setSize(9, 5).setCompression(3).setTiles(4, 4);
        QByteArray d = b.build();
        qToLittleEndian(qint32(-1), d.data() + b.offsetOf("chunk:0"_ba));
        QTest::newRow("tile-index-negative") << d;
    }
    {
        ExrBuilder b;
        b.setSize(9, 5).setCompression(3).setTiles(4, 4);
        QByteArray d = b.build();
        // levelX is the third int of a tile chunk header.
        qToLittleEndian(qint32(1), d.data() + b.offsetOf("chunk:0"_ba) + 8);
        QTest::newRow("tile-level-not-zero") << d;
    }
    {
        ExrBuilder b = zipFile();
        QByteArray d = b.build();
        const qint64 at = b.offsetOf("chunk:0:payload"_ba);
        // Destroy the zlib stream while keeping the declared size.
        memset(d.data() + at, 0, 8);
        QTest::newRow("corrupt-zlib-stream") << d;
    }
}

void tst_QSSGExrReader::badChunks()
{
    QFETCH(QByteArray, data);

    QSSGExrReader reader;
    bool ok = reader.parseHeader(data);
    if (ok) {
        QList<float> pixels(qsizetype(reader.size().width()) * reader.size().height() * 4, 0.0f);
        ok = reader.decodeToRgba32F(qssgExrRgbaChannelMap(reader), QSSGExrReader::NoFlags, pixels.data(), pixels.size());
    }
    QVERIFY(!ok);
    QVERIFY(!reader.errorMessage().isEmpty());
}

void tst_QSSGExrReader::limitsAreEnforced()
{
    const QByteArray data = ExrBuilder().setSize(64, 64).setCompression(3).build();

    // Each cap has to fire on its own, with its own message.
    struct
    {
        const char *name;
        QSSGExrReader::Limits limits;
    } cases[] = {
        { "maxWidth",
          [] {
              QSSGExrReader::Limits l;
              l.maxWidth = 8;
              return l;
          }() },
        { "maxHeight",
          [] {
              QSSGExrReader::Limits l;
              l.maxHeight = 8;
              return l;
          }() },
        { "maxPixels",
          [] {
              QSSGExrReader::Limits l;
              l.maxPixels = 16;
              return l;
          }() },
        { "maxChannels",
          [] {
              QSSGExrReader::Limits l;
              l.maxChannels = 1;
              return l;
          }() },
        { "maxDecodedBytes",
          [] {
              QSSGExrReader::Limits l;
              l.maxDecodedBytes = 64;
              return l;
          }() },
        { "maxChunkBytes",
          [] {
              QSSGExrReader::Limits l;
              l.maxChunkBytes = 8;
              return l;
          }() },
    };

    for (const auto &c : cases) {
        QSSGExrReader reader;
        reader.setLimits(c.limits);
        bool ok = reader.parseHeader(data);
        if (ok) {
            QList<float> pixels(64 * 64 * 4, 0.0f);
            ok = reader.decodeToRgba32F(qssgExrRgbaChannelMap(reader), QSSGExrReader::NoFlags, pixels.data(), pixels.size());
        }
        QVERIFY2(!ok, c.name);
        QVERIFY2(!reader.errorMessage().isEmpty(), c.name);
    }
}

// A tile far taller than the image must not size the band.
void tst_QSSGExrReader::hugeTileOnSmallImage()
{
    ExrBuilder builder;
    builder.setSize(64, 4).setCompression(3).setTiles(64, 4);
    QByteArray data = builder.build();

    // Claim 65536 pixel tall tiles.
    const qint64 tiles = builder.offsetOf("attr:tiles:value"_ba);
    QVERIFY(tiles > 0);
    qToLittleEndian(quint32(65536), data.data() + tiles);
    qToLittleEndian(quint32(65536), data.data() + tiles + 4);

    QSSGExrReader reader;
    QVERIFY2(reader.parseHeader(data), qPrintable(reader.errorMessage()));
    QList<float> pixels(64 * 4 * 4, 0.0f);
    if (!reader.decodeToRgba32F(qssgExrRgbaChannelMap(reader), QSSGExrReader::NoFlags, pixels.data(), pixels.size())) {
        QVERIFY(!reader.errorMessage().isEmpty());
    }
}

void tst_QSSGExrReader::zipTransformRoundTrip_data()
{
    QTest::addColumn<QByteArray>("raw");

    QTest::newRow("one-byte") << QByteArray(1, 'a');
    QTest::newRow("two-bytes") << QByteArray("ab");
    QTest::newRow("three-bytes") << QByteArray("abc");
    // Odd lengths cannot come from a valid file.
    QTest::newRow("odd-length") << QByteArray("abcdefghijklmno");
    QTest::newRow("even-length") << QByteArray("abcdefghijklmnop");
    QTest::newRow("all-equal") << QByteArray(64, 'z');

    QByteArray counting(256, '\0');
    for (int i = 0; i < 256; ++i)
        counting[i] = char(i);
    QTest::newRow("all-byte-values") << counting;
}

void tst_QSSGExrReader::zipTransformRoundTrip()
{
    QFETCH(QByteArray, raw);

    const QByteArray transformed = ExrBuilder::applyZipTransform(raw);
    QCOMPARE(transformed.size(), raw.size());

    QByteArray out(raw.size(), '\0');
    QByteArray scratch = transformed;
    QString error;
    QVERIFY(QSSGExr::undoPredictorAndDeinterleave(QSpan<char>(out.data(), out.size()),
                                                  QSpan<char>(scratch.data(), scratch.size()), &error));
    QCOMPARE(out, raw);

    QVERIFY(!QSSGExr::undoPredictorAndDeinterleave(QSpan<char>(out.data(), out.size()),
                                                   QSpan<char>(scratch.data(), scratch.size() - 1), &error));
}

void tst_QSSGExrReader::rleDecodeHostile_data()
{
    QTest::addColumn<QByteArray>("in");
    QTest::addColumn<int>("outSize");
    QTest::addColumn<bool>("valid");

    QTest::newRow("literal-run") << QByteArray::fromHex("fd616263") << 3 << true; // -3, 'a','b','c'
    QTest::newRow("repeat-run") << QByteArray::fromHex("0261") << 3 << true; // 2 -> 3 copies of 'a'
    QTest::newRow("repeat-of-one") << QByteArray::fromHex("0061") << 1 << true;
    QTest::newRow("max-repeat") << QByteArray::fromHex("7f61") << 128 << true;
    QTest::newRow("max-literal") << QByteArray::fromHex("80") + QByteArray(128, 'a') << 128 << true;
    QTest::newRow("empty-in-nonempty-out") << QByteArray() << 4 << false;
    QTest::newRow("literal-overruns-input") << QByteArray::fromHex("f061") << 16 << false;
    QTest::newRow("literal-overruns-output") << QByteArray::fromHex("fd616263") << 2 << false;
    QTest::newRow("repeat-overruns-output") << QByteArray::fromHex("7f61") << 4 << false;
    QTest::newRow("missing-repeat-value") << QByteArray::fromHex("02") << 3 << false;
    QTest::newRow("short-output") << QByteArray::fromHex("0061") << 8 << false;
    QTest::newRow("all-zero-tags") << QByteArray(32, '\0') << 16 << true; // each pair yields one byte
}

void tst_QSSGExrReader::rleDecodeHostile()
{
    QFETCH(QByteArray, in);
    QFETCH(int, outSize);
    QFETCH(bool, valid);

    // Guard bytes either side, so an overrun shows up even without a sanitiser.
    const int guard = 16;
    QByteArray buffer(outSize + 2 * guard, '\xa5');
    QString error;
    const bool ok = QSSGExr::rleDecode(in, QSpan<char>(buffer.data() + guard, outSize), &error);

    QCOMPARE(ok, valid);
    if (!ok)
        QVERIFY(!error.isEmpty());
    for (int i = 0; i < guard; ++i) {
        QCOMPARE(quint8(buffer[i]), 0xa5);
        QCOMPARE(quint8(buffer[outSize + guard + i]), 0xa5);
    }
}

void tst_QSSGExrReader::inflateExactRejects_data()
{
    QTest::addColumn<QByteArray>("in");
    QTest::addColumn<int>("outSize");
    QTest::addColumn<bool>("valid");

    const QByteArray raw(64, 'q');
    const QByteArray stream = qCompress(raw, 9).sliced(4);

    QTest::newRow("exact") << stream << 64 << true;
    QTest::newRow("expect-one-less") << stream << 63 << false;
    QTest::newRow("expect-one-more") << stream << 65 << false;
    QTest::newRow("truncated") << stream.left(stream.size() - 1) << 64 << false;
    QTest::newRow("trailing-garbage") << stream + QByteArray(4, 'x') << 64 << false;
    QTest::newRow("all-zero") << QByteArray(32, '\0') << 64 << false;
    QTest::newRow("all-ff") << QByteArray(32, '\xff') << 64 << false;
    QTest::newRow("empty") << QByteArray() << 64 << false;
    QTest::newRow("corrupt-checksum") << [stream] {
        QByteArray s = stream;
        s[s.size() - 1] = char(s[s.size() - 1] ^ 0xff);
        return s;
    }() << 64 << false;
}

void tst_QSSGExrReader::inflateExactRejects()
{
    QFETCH(QByteArray, in);
    QFETCH(int, outSize);
    QFETCH(bool, valid);

    const int guard = 16;
    QByteArray buffer(outSize + 2 * guard, '\xa5');
    QString error;
    const bool ok = QSSGExr::inflateExact(in, QSpan<char>(buffer.data() + guard, outSize), &error);

    QCOMPARE(ok, valid);
    if (!ok)
        QVERIFY(!error.isEmpty());
    for (int i = 0; i < guard; ++i) {
        QCOMPARE(quint8(buffer[i]), 0xa5);
        QCOMPARE(quint8(buffer[outSize + guard + i]), 0xa5);
    }
}

void tst_QSSGExrReader::hufUncompressHostile_data()
{
    QTest::addColumn<QByteArray>("in");

    // im, iM, tableLength, nBits and a reserved word.
    auto preamble = [](quint32 im, quint32 iM, quint32 tableLength, quint32 nBits) {
        QByteArray d;
        appendU32(&d, im);
        appendU32(&d, iM);
        appendU32(&d, tableLength);
        appendU32(&d, nBits);
        appendU32(&d, 0);
        return d;
    };

    QTest::newRow("empty") << QByteArray();
    for (int n = 1; n < 20; ++n)
        QTest::addRow("short-preamble-%d", n) << QByteArray(n, '\0');

    QTest::newRow("im-greater-than-iM") << preamble(100, 10, 0, 0);
    QTest::newRow("im-past-encsize") << preamble(65537, 65537, 0, 0);
    QTest::newRow("iM-past-encsize") << preamble(0, 70000, 0, 0);
    QTest::newRow("im-uint-max") << preamble(0xffffffff, 0xffffffff, 0, 0);
    QTest::newRow("nBits-past-input") << preamble(0, 10, 0, 1u << 20) + QByteArray(8, '\0');
    QTest::newRow("empty-table") << preamble(0, 100, 0, 0);
    QTest::newRow("all-ff-table") << preamble(0, 100, 0, 64) + QByteArray(64, '\xff');
    QTest::newRow("zero-run-past-iM")
            // A long zero run that walks past iM.
            << preamble(0, 4, 0, 64) + QByteArray(64, '\xfc');
}

void tst_QSSGExrReader::hufUncompressHostile()
{
    QFETCH(QByteArray, in);

    const qsizetype words = 64;
    const int guard = 16;
    QList<quint16> buffer(words + 2 * guard, 0xa5a5);
    QSSGExr::PizScratch scratch;
    QString error;

    // Must return, one way or the other, without writing outside the span.
    const bool ok = QSSGExr::hufUncompress(in, QSpan<quint16>(buffer.data() + guard, words), &scratch, &error);
    if (!ok)
        QVERIFY(!error.isEmpty());
    for (int i = 0; i < guard; ++i) {
        QCOMPARE(buffer[i], quint16(0xa5a5));
        QCOMPARE(buffer[words + guard + i], quint16(0xa5a5));
    }
}

void tst_QSSGExrReader::hufRejectsUndecodableCodeLength()
{
    QByteArray in;
    appendU32(&in, 0); // im
    appendU32(&in, 1); // iM
    appendU32(&in, 0); // packed table length, written but unused
    appendU32(&in, 8); // nBits
    appendU32(&in, 0);
    in += QByteArray::fromHex("eba0"); // two six bit lengths, both 58
    in += QByteArray(1, '\0');

    QList<quint16> out(64, 0);
    QSSGExr::PizScratch scratch;
    QString error;
    QVERIFY(!QSSGExr::hufUncompress(in, QSpan<quint16>(out.data(), out.size()), &scratch, &error));
    QVERIFY(error.contains("code length"_L1));
}

void tst_QSSGExrReader::wav2DecodeDegenerateSizes()
{
    // Sizes where the level loop degenerates.
    const struct
    {
        qint64 nx;
        qint64 ny;
    } sizes[] = { { 1, 1 }, { 1, 2 }, { 2, 1 }, { 1, 40 }, { 40, 1 }, { 3, 3 }, { 5, 7 }, { 7, 5 }, { 2, 2 }, { 33, 33 } };

    for (const auto &s : sizes) {
        for (const quint16 maxValue : { quint16(0), quint16(1000), quint16(0xffff) }) {
            const qsizetype n = qsizetype(s.nx * s.ny);
            const int guard = 8;
            QList<quint16> buffer(n + 2 * guard, 0xa5a5);
            for (qsizetype i = 0; i < n; ++i)
                buffer[guard + i] = quint16((i * 2654435761u) & 0xffff);

            QSSGExr::wav2Decode(QSpan<quint16>(buffer.data() + guard, n), 0, s.nx, 1, s.ny, s.nx, maxValue);

            for (int i = 0; i < guard; ++i) {
                QCOMPARE(buffer[i], quint16(0xa5a5));
                QCOMPARE(buffer[n + guard + i], quint16(0xa5a5));
            }
        }
    }
}

void tst_QSSGExrReader::fuzzRegressions_data()
{
    QTest::addColumn<QByteArray>("data");

    const QString dir = QFINDTESTDATA(QStringLiteral("data/fuzz"));
    if (dir.isEmpty())
        return;
    const QFileInfoList entries = QDir(dir).entryInfoList({ u"*.exr"_s }, QDir::Files, QDir::Name);
    for (const QFileInfo &info : entries) {
        QFile f(info.absoluteFilePath());
        if (!f.open(QIODevice::ReadOnly))
            continue;
        QTest::newRow(qPrintable(info.fileName())) << f.readAll();
    }
}

void tst_QSSGExrReader::fuzzRegressions()
{
    if (QTest::currentDataTag() == nullptr)
        QSKIP("no fuzz findings recorded yet");
    QFETCH(QByteArray, data);

    // Each has to fail with a message or decode, never crash.
    QSSGExrReader reader;
    if (!reader.parseHeader(data)) {
        QVERIFY(!reader.errorMessage().isEmpty());
        return;
    }
    const QSize size = reader.size();
    QVERIFY(size.width() > 0 && size.height() > 0);
    QList<float> pixels(qsizetype(size.width()) * size.height() * 4, 0.0f);
    if (!reader.decodeToRgba32F(qssgExrRgbaChannelMap(reader), QSSGExrReader::NoFlags, pixels.data(), pixels.size())) {
        QVERIFY(!reader.errorMessage().isEmpty());
    }
}

QTEST_APPLESS_MAIN(tst_QSSGExrReader)

#include "tst_qssgexrreader.moc"
