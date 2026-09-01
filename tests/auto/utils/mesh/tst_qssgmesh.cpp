// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QtTest>

#include <QtQuick3DUtils/private/qssgmesh_p.h>

#include <QtCore/qbuffer.h>
#include <QtCore/qdir.h>
#include <QtCore/qendian.h>
#include <QtCore/qfile.h>

using namespace Qt::StringLiterals;
using namespace QSSGMesh;

// Assembles a .mesh container byte by byte, from the format description in
// src/quick3d/doc/src/qtquick3d-mesh-format.qdoc rather than by mirroring the
// reader, so that a wrong reading of the format is not masked.
class MeshBuilder
{
public:
    struct Entry
    {
        QByteArray name;
        quint32 componentType = 10; // Float32
        quint32 componentCount = 3;
        quint32 offset = 0;
    };

    struct Lod
    {
        quint32 count = 0;
        quint32 offset = 0;
        float distance = 0.0f;
    };

    struct Subset
    {
        QString name;
        quint32 count = 0;
        quint32 offset = 0;
        float minX = -1.0f, minY = -1.0f, minZ = -1.0f;
        float maxX = 1.0f, maxY = 1.0f, maxZ = 1.0f;
        quint32 lightmapWidth = 0;
        quint32 lightmapHeight = 0;
        QList<Lod> lods;
    };

    // The alignment step adds a full four bytes when the position is already
    // aligned, and the position is always aligned when a block starts, so the
    // padding is only ever a function of the block size. See the format page.
    static QByteArray padded(const QByteArray &block)
    {
        return block + QByteArray(4 - (block.size() % 4), '\0');
    }

    MeshBuilder &setVersion(quint16 version)
    {
        m_version = version;
        return *this;
    }
    MeshBuilder &setStride(quint32 stride)
    {
        m_stride = stride;
        return *this;
    }
    MeshBuilder &setEntries(const QList<Entry> &entries)
    {
        m_entries = entries;
        return *this;
    }
    MeshBuilder &setVertexData(const QByteArray &data)
    {
        m_vertexData = data;
        return *this;
    }
    MeshBuilder &setIndexData(const QByteArray &data, quint32 componentType)
    {
        m_indexData = data;
        m_indexComponentType = componentType;
        return *this;
    }
    MeshBuilder &setSubsets(const QList<Subset> &subsets)
    {
        m_subsets = subsets;
        return *this;
    }
    MeshBuilder &setJointCount(quint32 count)
    {
        m_jointCount = count;
        return *this;
    }
    MeshBuilder &setTargetEntries(const QList<Entry> &entries)
    {
        m_targetEntries = entries;
        return *this;
    }
    MeshBuilder &setTargetData(const QByteArray &data, quint32 numTargets)
    {
        m_targetData = data;
        m_numTargets = numTargets;
        return *this;
    }
    MeshBuilder &setDrawMode(quint32 drawMode)
    {
        m_drawMode = drawMode;
        return *this;
    }
    MeshBuilder &setWinding(quint32 winding)
    {
        m_winding = winding;
        return *this;
    }
    // Replaces everything after the 12 byte MeshDataHeader.
    MeshBuilder &setRawPayload(const QByteArray &payload)
    {
        m_payload = payload;
        m_hasPayload = true;
        return *this;
    }
    // Overrides one of the fourteen quint32 fields of the Mesh struct by index.
    MeshBuilder &patchMeshField(int index, quint32 value)
    {
        m_patches.insert(index, value);
        return *this;
    }

    // Everything from the MeshDataHeader onwards.
    QByteArray mesh() const
    {
        const QByteArray payload = m_hasPayload ? m_payload : body();
        QByteArray out;
        append32(&out, MeshFileId);
        append16(&out, m_version);
        append16(&out, 0); // headerFlags
        append32(&out, quint32(12 + payload.size()));
        return out + payload;
    }

    // A whole container holding this one mesh.
    QByteArray build(quint32 id = 1) const { return container({ mesh() }, { id }); }

    static QByteArray container(const QList<QByteArray> &meshes, const QList<quint32> &ids)
    {
        QByteArray out;
        QList<quint64> offsets;
        for (const QByteArray &m : meshes) {
            offsets.append(quint64(out.size()));
            out += m;
        }
        const quint32 entriesOffset = quint32(out.size());
        for (qsizetype i = 0; i < meshes.size(); ++i) {
            append64(&out, offsets.at(i));
            append32(&out, ids.at(i));
            append32(&out, 0); // padding
        }
        append32(&out, ContainerFileId);
        append32(&out, 1); // container version
        append32(&out, entriesOffset);
        append32(&out, quint32(meshes.size()));
        return out;
    }

    static constexpr quint32 MeshFileId = 3365961549U;
    static constexpr quint32 ContainerFileId = 555777497U;

private:
    static void append16(QByteArray *out, quint16 v)
    {
        char buf[2];
        qToLittleEndian(v, buf);
        out->append(buf, 2);
    }
    static void append32(QByteArray *out, quint32 v)
    {
        char buf[4];
        qToLittleEndian(v, buf);
        out->append(buf, 4);
    }
    static void append64(QByteArray *out, quint64 v)
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

    QByteArray body() const
    {
        const bool v7 = m_version >= 7;
        QByteArray out;

        // The Mesh struct: fourteen quint32 fields, three of them repurposed in
        // version 7.
        QList<quint32> fields = {
            v7 ? quint32(m_targetEntries.size()) : 0u, // entriesOffset / targetBufferEntriesCount
            quint32(m_entries.size()),
            m_stride,
            v7 ? quint32(m_targetData.size()) : 0u, // dataOffset / targetBufferDataSize
            quint32(m_vertexData.size()),
            m_indexComponentType,
            0u, // indexBuffer.dataOffset
            quint32(m_indexData.size()),
            v7 ? m_numTargets : 0u, // subsetsOffset / targetCount
            quint32(m_subsets.size()),
            0u, // jointsOffset
            m_jointCount,
            m_drawMode,
            m_winding,
        };
        for (auto it = m_patches.cbegin(); it != m_patches.cend(); ++it)
            fields[it.key()] = it.value();
        for (quint32 f : fields)
            append32(&out, f);

        out += padded(entryBlock(m_entries));
        for (const Entry &e : m_entries)
            out += nameBlock(e.name);

        out += padded(m_vertexData);
        out += padded(m_indexData);

        QByteArray subsetBlock;
        for (const Subset &s : m_subsets) {
            append32(&subsetBlock, s.count);
            append32(&subsetBlock, s.offset);
            appendFloat(&subsetBlock, s.minX);
            appendFloat(&subsetBlock, s.minY);
            appendFloat(&subsetBlock, s.minZ);
            appendFloat(&subsetBlock, s.maxX);
            appendFloat(&subsetBlock, s.maxY);
            appendFloat(&subsetBlock, s.maxZ);
            append32(&subsetBlock, 0); // nameOffset
            append32(&subsetBlock, quint32(s.name.size() + 1)); // with the terminator
            if (m_version >= 5) {
                append32(&subsetBlock, s.lightmapWidth);
                append32(&subsetBlock, s.lightmapHeight);
                if (m_version >= 6)
                    append32(&subsetBlock, quint32(s.lods.size()));
            }
        }
        out += padded(subsetBlock);

        for (const Subset &s : m_subsets) {
            QByteArray name;
            for (QChar c : s.name)
                append16(&name, c.unicode());
            append16(&name, 0); // terminator
            out += padded(name);
        }

        if (m_version >= 6) {
            QByteArray lodBlock;
            for (const Subset &s : m_subsets) {
                for (const Lod &l : s.lods) {
                    append32(&lodBlock, l.count);
                    append32(&lodBlock, l.offset);
                    appendFloat(&lodBlock, l.distance);
                }
            }
            out += padded(lodBlock);
        }

        // Joint records are 136 bytes and carry no padding of their own.
        out += QByteArray(qsizetype(m_jointCount) * 136, '\0');

        if (v7) {
            out += padded(entryBlock(m_targetEntries));
            for (const Entry &e : m_targetEntries)
                out += nameBlock(e.name);
            out += m_targetData; // the last block, never padded
        }
        return out;
    }

    static QByteArray entryBlock(const QList<Entry> &entries)
    {
        QByteArray out;
        for (const Entry &e : entries) {
            append32(&out, 0); // nameOffset
            append32(&out, e.componentType);
            append32(&out, e.componentCount);
            append32(&out, e.offset);
        }
        return out;
    }

    static QByteArray nameBlock(const QByteArray &name)
    {
        QByteArray out;
        append32(&out, quint32(name.size() + 1));
        return out + padded(name + '\0');
    }

    quint16 m_version = 7;
    quint32 m_stride = 12;
    QList<Entry> m_entries = { { "attr_pos"_ba, 10, 3, 0 } };
    QByteArray m_vertexData = QByteArray(36, '\x01');
    QByteArray m_indexData;
    quint32 m_indexComponentType = 5; // UnsignedInt32
    QList<Subset> m_subsets = { { QStringLiteral("sub"), 3, 0, -1, -1, -1, 1, 1, 1, 0, 0, {} } };
    quint32 m_jointCount = 0;
    QList<Entry> m_targetEntries;
    QByteArray m_targetData;
    quint32 m_numTargets = 0;
    quint32 m_drawMode = 7; // Triangles
    quint32 m_winding = 2; // CounterClockwise
    QByteArray m_payload;
    bool m_hasPayload = false;
    QMap<int, quint32> m_patches;
};

class tst_QSSGMesh : public QObject
{
    Q_OBJECT

private slots:
    void builtMeshDecodes_data();
    void builtMeshDecodes();
    void subsetLayoutPerVersion_data();
    void subsetLayoutPerVersion();
    void entryNamesOfEveryLength_data();
    void entryNamesOfEveryLength();
    void indexBufferComponentTypes_data();
    void indexBufferComponentTypes();
    void multiMeshContainer();
    void saveAndLoadRoundTrip_data();
    void saveAndLoadRoundTrip();
    void rejectsBadFileIds_data();
    void rejectsBadFileIds();
    void truncatedAtEveryOffset_data();
    void truncatedAtEveryOffset();
    void countsAndSizesAreBounded_data();
    void countsAndSizesAreBounded();
    void subsetsShareTheLevelOfDetailBudget();
    void malformedLegacyMorphTargets_data();
    void malformedLegacyMorphTargets();
    void drawRangesAreValidated_data();
    void drawRangesAreValidated();
    void unknownComponentTypes_data();
    void unknownComponentTypes();
    void jointBlockIsSkipped();
    void jointCountIsBounded();
    void fuzzRegressions_data();
    void fuzzRegressions();

private:
    static Mesh load(const QByteArray &blob, quint32 id = 0)
    {
        QByteArray copy = blob;
        QBuffer buffer(&copy);
        buffer.open(QIODevice::ReadOnly);
        return Mesh::loadMesh(&buffer, id);
    }
    // Finds a little endian quint32 with a distinctive value, so a field can be
    // located without counting bytes by hand.
    static qsizetype offsetOf(const QByteArray &blob, quint32 marker)
    {
        char needle[4];
        qToLittleEndian(marker, needle);
        return blob.indexOf(QByteArray(needle, 4));
    }
    // Overwrites a little endian quint32 at a byte offset, counted from the end
    // of the blob when negative.
    static QByteArray patched(const QByteArray &blob, qsizetype at, quint32 value)
    {
        QByteArray out = blob;
        const qsizetype pos = at < 0 ? out.size() + at : at;
        qToLittleEndian(value, out.data() + pos);
        return out;
    }
    static QMap<quint32, Mesh> loadAll(const QByteArray &blob)
    {
        QByteArray copy = blob;
        QBuffer buffer(&copy);
        buffer.open(QIODevice::ReadOnly);
        return Mesh::loadAll(&buffer);
    }
};

void tst_QSSGMesh::builtMeshDecodes_data()
{
    QTest::addColumn<int>("version");
    for (int v = 3; v <= 7; ++v)
        QTest::addRow("v%d", v) << v;
}

void tst_QSSGMesh::builtMeshDecodes()
{
    QFETCH(int, version);

    MeshBuilder builder;
    builder.setVersion(quint16(version))
            .setStride(20)
            .setEntries({ { "attr_pos"_ba, 10, 3, 0 }, { "attr_uv0"_ba, 10, 2, 12 } })
            .setVertexData(QByteArray(20 * 6, '\x2a'))
            .setIndexData(QByteArray(2 * 6, '\x03'), 3 /* UnsignedInt16 */)
            .setSubsets({ { QStringLiteral("first"), 3, 0, -2, -2, -2, 2, 2, 2, 0, 0, {} },
                          { QStringLiteral("second"), 3, 3, -1, -1, -1, 1, 1, 1, 0, 0, {} } })
            .setDrawMode(7)
            .setWinding(2);

    const Mesh mesh = load(builder.build());
    QVERIFY(mesh.isValid());
    QCOMPARE(mesh.drawMode(), Mesh::DrawMode::Triangles);
    QCOMPARE(mesh.winding(), Mesh::Winding::CounterClockwise);

    const Mesh::VertexBuffer vb = mesh.vertexBuffer();
    QCOMPARE(vb.stride, 20u);
    QCOMPARE(vb.data, QByteArray(20 * 6, '\x2a'));
    QCOMPARE(vb.entries.size(), 2);
    QCOMPARE(vb.entries.at(0).name, "attr_pos"_ba);
    QCOMPARE(vb.entries.at(0).componentCount, 3u);
    QCOMPARE(vb.entries.at(0).offset, 0u);
    QCOMPARE(vb.entries.at(1).name, "attr_uv0"_ba);
    QCOMPARE(vb.entries.at(1).componentCount, 2u);
    QCOMPARE(vb.entries.at(1).offset, 12u);

    const Mesh::IndexBuffer ib = mesh.indexBuffer();
    QCOMPARE(ib.componentType, Mesh::ComponentType::UnsignedInt16);
    QCOMPARE(ib.data, QByteArray(2 * 6, '\x03'));

    const QVector<Mesh::Subset> subsets = mesh.subsets();
    QCOMPARE(subsets.size(), 2);
    QCOMPARE(subsets.at(0).name, QStringLiteral("first"));
    QCOMPARE(subsets.at(0).count, 3u);
    QCOMPARE(subsets.at(0).offset, 0u);
    QCOMPARE(subsets.at(0).bounds.min, QVector3D(-2, -2, -2));
    QCOMPARE(subsets.at(0).bounds.max, QVector3D(2, 2, 2));
    QCOMPARE(subsets.at(1).name, QStringLiteral("second"));
    QCOMPARE(subsets.at(1).offset, 3u);
}

void tst_QSSGMesh::subsetLayoutPerVersion_data()
{
    QTest::addColumn<int>("version");
    QTest::addColumn<bool>("hasLightmapHint");
    QTest::addColumn<bool>("hasLods");

    QTest::newRow("v3") << 3 << false << false;
    QTest::newRow("v4") << 4 << false << false;
    QTest::newRow("v5") << 5 << true << false;
    QTest::newRow("v6") << 6 << true << true;
    QTest::newRow("v7") << 7 << true << true;
}

void tst_QSSGMesh::subsetLayoutPerVersion()
{
    QFETCH(int, version);
    QFETCH(bool, hasLightmapHint);
    QFETCH(bool, hasLods);

    // The subset record grew twice, so a reader that gets the size wrong for a
    // version loses its place in the file and everything after it.
    MeshBuilder::Subset subset;
    subset.name = QStringLiteral("sub");
    subset.count = 6;
    subset.lightmapWidth = 64;
    subset.lightmapHeight = 32;
    subset.lods = { { 3, 0, 10.0f }, { 6, 3, 20.0f } };

    const Mesh mesh = load(MeshBuilder()
                                   .setVersion(quint16(version))
                                   .setIndexData(QByteArray(4 * 9, '\0'), 5)
                                   .setSubsets({ subset })
                                   .build());
    QVERIFY(mesh.isValid());
    QCOMPARE(mesh.subsets().size(), 1);

    // subsets() returns by value, so this has to be a copy.
    const Mesh::Subset got = mesh.subsets().first();
    QCOMPARE(got.name, QStringLiteral("sub"));
    QCOMPARE(got.count, 6u);
    QCOMPARE(got.lightmapSizeHint, hasLightmapHint ? QSize(64, 32) : QSize(0, 0));
    QCOMPARE(got.lods.size(), hasLods ? 2 : 0);
    if (hasLods) {
        QCOMPARE(got.lods.at(0).count, 3u);
        QCOMPARE(got.lods.at(0).offset, 0u);
        QCOMPARE(got.lods.at(0).distance, 10.0f);
        QCOMPARE(got.lods.at(1).count, 6u);
        QCOMPARE(got.lods.at(1).distance, 20.0f);
    }
}

void tst_QSSGMesh::entryNamesOfEveryLength_data()
{
    QTest::addColumn<int>("length");
    for (int n = 1; n <= 12; ++n)
        QTest::addRow("len%d", n) << n;
}

void tst_QSSGMesh::entryNamesOfEveryLength()
{
    QFETCH(int, length);

    // Names are the only blocks whose size is not a multiple of four, so they
    // are what exercises the padding rule.
    const QByteArray name = QByteArray(length, 'a');
    const Mesh mesh = load(MeshBuilder().setEntries({ { name, 10, 3, 0 } }).build());
    QVERIFY(mesh.isValid());
    QCOMPARE(mesh.vertexBuffer().entries.size(), 1);
    QCOMPARE(mesh.vertexBuffer().entries.first().name, name);
    QCOMPARE(mesh.subsets().size(), 1);
    QCOMPARE(mesh.subsets().first().name, QStringLiteral("sub"));
}

void tst_QSSGMesh::indexBufferComponentTypes_data()
{
    QTest::addColumn<int>("componentType");
    QTest::addColumn<int>("expected");

    QTest::newRow("u8") << 1 << int(Mesh::ComponentType::UnsignedInt8);
    QTest::newRow("u16") << 3 << int(Mesh::ComponentType::UnsignedInt16);
    QTest::newRow("u32") << 5 << int(Mesh::ComponentType::UnsignedInt32);
    QTest::newRow("f32") << 10 << int(Mesh::ComponentType::Float32);
}

void tst_QSSGMesh::indexBufferComponentTypes()
{
    QFETCH(int, componentType);
    QFETCH(int, expected);

    const Mesh mesh =
            load(MeshBuilder().setIndexData(QByteArray(16, '\0'), quint32(componentType)).build());
    QVERIFY(mesh.isValid());
    QCOMPARE(int(mesh.indexBuffer().componentType), expected);
}

void tst_QSSGMesh::multiMeshContainer()
{
    const QByteArray first = MeshBuilder()
                                     .setStride(12)
                                     .setVertexData(QByteArray(36, '\x11'))
                                     .setSubsets({ { QStringLiteral("one"), 3, 0, -1, -1, -1, 1, 1,
                                                     1, 0, 0, {} } })
                                     .mesh();
    const QByteArray second = MeshBuilder()
                                      .setStride(16)
                                      .setVertexData(QByteArray(48, '\x22'))
                                      .setSubsets({ { QStringLiteral("two"), 3, 0, -2, -2, -2, 2, 2,
                                                      2, 0, 0, {} } })
                                      .mesh();
    const QByteArray blob = MeshBuilder::container({ first, second }, { 1, 7 });

    const QMap<quint32, Mesh> all = loadAll(blob);
    QCOMPARE(all.size(), 2);
    QVERIFY(all.contains(1));
    QVERIFY(all.contains(7));
    QCOMPARE(all.value(1).vertexBuffer().stride, 12u);
    QCOMPARE(all.value(1).subsets().first().name, QStringLiteral("one"));
    QCOMPARE(all.value(7).vertexBuffer().stride, 16u);
    QCOMPARE(all.value(7).subsets().first().name, QStringLiteral("two"));

    // By id, and id 0 meaning "the first one".
    QCOMPARE(load(blob, 7).vertexBuffer().stride, 16u);
    QCOMPARE(load(blob, 0).vertexBuffer().stride, 12u);
    QVERIFY(!load(blob, 99).isValid());
}

void tst_QSSGMesh::saveAndLoadRoundTrip_data()
{
    QTest::addColumn<int>("version");
    for (int v = 3; v <= 7; ++v)
        QTest::addRow("v%d", v) << v;
}

void tst_QSSGMesh::saveAndLoadRoundTrip()
{
    QFETCH(int, version);

    // Whatever version came in, save() writes the current one, and the result
    // has to describe the same geometry.
    MeshBuilder::Subset subset;
    subset.name = QStringLiteral("round");
    subset.count = 9;
    subset.lightmapWidth = 16;
    subset.lightmapHeight = 8;
    subset.lods = { { 6, 0, 5.0f } };

    const Mesh loaded = load(MeshBuilder()
                                     .setVersion(quint16(version))
                                     .setStride(12)
                                     .setEntries({ { "attr_pos"_ba, 10, 3, 0 } })
                                     .setVertexData(QByteArray(12 * 9, '\x7f'))
                                     .setIndexData(QByteArray(4 * 9, '\x05'), 5)
                                     .setSubsets({ subset })
                                     .build());
    QVERIFY(loaded.isValid());

    QByteArray saved;
    QBuffer out(&saved);
    QVERIFY(out.open(QIODevice::ReadWrite));
    QCOMPARE(loaded.save(&out), 1u);

    const Mesh again = load(saved);
    QVERIFY(again.isValid());
    QCOMPARE(again.vertexBuffer().stride, loaded.vertexBuffer().stride);
    QCOMPARE(again.vertexBuffer().data, loaded.vertexBuffer().data);
    QCOMPARE(again.indexBuffer().data, loaded.indexBuffer().data);
    QCOMPARE(int(again.indexBuffer().componentType), int(loaded.indexBuffer().componentType));
    QCOMPARE(again.subsets().size(), loaded.subsets().size());
    QCOMPARE(again.subsets().first().name, loaded.subsets().first().name);
    QCOMPARE(again.subsets().first().count, loaded.subsets().first().count);
    QCOMPARE(again.subsets().first().offset, loaded.subsets().first().offset);
    QCOMPARE(again.subsets().first().lightmapSizeHint, loaded.subsets().first().lightmapSizeHint);
    QCOMPARE(again.subsets().first().lods.size(), loaded.subsets().first().lods.size());
    QCOMPARE(again.vertexBuffer().entries.size(), loaded.vertexBuffer().entries.size());
    QCOMPARE(again.vertexBuffer().entries.first().name,
             loaded.vertexBuffer().entries.first().name);
}

void tst_QSSGMesh::rejectsBadFileIds_data()
{
    QTest::addColumn<QByteArray>("data");

    const QByteArray good = MeshBuilder().build();

    QTest::newRow("empty") << QByteArray();
    QTest::newRow("shorter-than-the-footer") << good.left(8);

    QByteArray badContainerId = good;
    badContainerId[badContainerId.size() - 16] = char(0);
    QTest::newRow("bad-container-id") << badContainerId;

    QByteArray badContainerVersion = good;
    badContainerVersion[badContainerVersion.size() - 12] = char(9);
    QTest::newRow("bad-container-version") << badContainerVersion;

    QTest::newRow("bad-mesh-id") << MeshBuilder().patchMeshField(0, 0).build().replace(
            0, 4, QByteArray(4, '\0'));

    QTest::newRow("version-too-old")
            << MeshBuilder().setVersion(2).build();
    QTest::newRow("version-too-new")
            << MeshBuilder().setVersion(8).build();
}

void tst_QSSGMesh::rejectsBadFileIds()
{
    QFETCH(QByteArray, data);

    // A rejected file has to come back as an invalid Mesh, never as a crash and
    // never as something half parsed.
    const Mesh mesh = load(data);
    QVERIFY(!mesh.isValid());
    QVERIFY(mesh.subsets().isEmpty());
}

void tst_QSSGMesh::truncatedAtEveryOffset_data()
{
    QTest::addColumn<QByteArray>("data");

    for (int v = 3; v <= 7; ++v) {
        MeshBuilder builder;
        builder.setVersion(quint16(v))
                .setEntries({ { "attr_pos"_ba, 10, 3, 0 } })
                .setIndexData(QByteArray(4 * 3, '\x01'), 5);
        if (v >= 6) {
            MeshBuilder::Subset subset;
            subset.name = QStringLiteral("s");
            subset.count = 3;
            subset.lods = { { 3, 0, 1.0f } };
            builder.setSubsets({ subset });
        }
        QTest::addRow("v%d", v) << builder.build();
    }
}

void tst_QSSGMesh::truncatedAtEveryOffset()
{
    QFETCH(QByteArray, data);

    // Cutting a known good file at every length at once. Nothing may crash,
    // hang or allocate wildly, and the result is either the whole mesh or none
    // of it.
    for (qsizetype n = 0; n < data.size(); ++n) {
        const Mesh mesh = load(data.left(n));
        if (mesh.isValid())
            QCOMPARE(n, data.size());
    }
}

void tst_QSSGMesh::countsAndSizesAreBounded_data()
{
    QTest::addColumn<QByteArray>("data");

    const quint32 huge = 0xfffffff0u;

    // Every count in the header sizes an allocation or bounds a loop, and none
    // of them can be larger than the file that holds the data.
    QTest::newRow("vertex-entry-count") << MeshBuilder().patchMeshField(1, huge).build();
    QTest::newRow("vertex-data-size") << MeshBuilder().patchMeshField(4, huge).build();
    QTest::newRow("index-data-size") << MeshBuilder().patchMeshField(7, huge).build();
    QTest::newRow("subset-count") << MeshBuilder().patchMeshField(9, huge).build();
    QTest::newRow("target-entry-count") << MeshBuilder().patchMeshField(0, huge).build();
    QTest::newRow("target-data-size")
            << MeshBuilder()
                       .setTargetEntries({ { "attr_pos"_ba, 10, 3, 0 } })
                       .setTargetData(QByteArray(16, '\0'), 1)
                       .patchMeshField(3, huge)
                       .build();

    // The mesh count is in the container footer, which is the last four bytes.
    QTest::newRow("mesh-count") << patched(MeshBuilder().build(), -4, huge);

    // An attribute name length, which is the first quint32 after the entry list
    // and its padding: 56 bytes of Mesh struct, 16 of entry, 4 of padding.
    QTest::newRow("attribute-name-length")
            << patched(MeshBuilder().build(), 12 + 56 + 16 + 4, huge);

    // A subset name length. The subset gets a distinctive count so its record
    // can be found, and nameLength is nine quint32 fields further on.
    MeshBuilder::Subset marked;
    marked.name = QStringLiteral("sub");
    marked.count = 0x51525354u;
    const QByteArray oneSubset = MeshBuilder().setSubsets({ marked }).build();
    const qsizetype subsetAt = offsetOf(oneSubset, marked.count);
    QVERIFY(subsetAt >= 0);
    QTest::newRow("subset-name-length") << patched(oneSubset, subsetAt + 9 * 4, huge);
}

void tst_QSSGMesh::countsAndSizesAreBounded()
{
    QFETCH(QByteArray, data);

    const Mesh mesh = load(data);
    QVERIFY(!mesh.isValid());
    QVERIFY(mesh.vertexBuffer().data.isEmpty());
    QVERIFY(mesh.targetBuffer().data.isEmpty());
}

void tst_QSSGMesh::subsetsShareTheLevelOfDetailBudget()
{
    // One subset carries real levels, so there are bytes for the others to claim.
    constexpr int levels = 100;
    MeshBuilder::Subset withLods;
    withLods.name = QStringLiteral("a");
    withLods.count = 0x41414141u;
    for (int i = 0; i < levels; ++i)
        withLods.lods.append({ 0, 0, float(i) });

    MeshBuilder::Subset second;
    second.name = QStringLiteral("b");
    second.count = 0x42424242u;
    second.lods.append({ 0, 0, 0.0f });

    MeshBuilder::Subset third;
    third.name = QStringLiteral("c");
    third.count = 0x43434343u;
    third.lods.append({ 0, 0, 0.0f });

    QByteArray blob = MeshBuilder().setSubsets({ withLods, second, third }).build();

    // lodCount is the thirteenth quint32 of a subset record.
    for (quint32 marker : { 0x42424242u, 0x43434343u }) {
        const qsizetype at = offsetOf(blob, marker);
        QVERIFY(at >= 0);
        blob = patched(blob, at + 12 * 4, quint32(levels));
    }

    const Mesh mesh = load(blob);
    QVERIFY(!mesh.isValid());
}

void tst_QSSGMesh::malformedLegacyMorphTargets_data()
{
    QTest::addColumn<QByteArray>("data");

    // Before version 7 morph targets were extra vertex buffer entries, and the
    // reader rebuilds a target buffer out of them. Every step of that layout
    // comes from the file.
    QTest::newRow("no-target-zero-to-copy-from")
            << MeshBuilder()
                       .setVersion(6)
                       .setStride(16)
                       .setEntries({ { "attr_tpos1"_ba, 10, 3, 0 },
                                     { "attr_unsupported"_ba, 10, 3, 12 } })
                       .setVertexData(QByteArray(32, '\0'))
                       .build();

    QList<MeshBuilder::Entry> nineTargets;
    for (int i = 0; i < 9; ++i)
        nineTargets.append({ QByteArrayLiteral("attr_tpos") + QByteArray::number(i), 10, 3, 0 });
    QTest::newRow("more-targets-than-the-format-allows")
            << MeshBuilder()
                       .setVersion(6)
                       .setStride(12)
                       .setEntries(nineTargets)
                       .setVertexData(QByteArray(12, '\0'))
                       .build();

    QList<MeshBuilder::Entry> fiveComponents;
    for (int i = 0; i < 5; ++i)
        fiveComponents.append({ "attr_tpos0"_ba, 10, 3, 0 });
    QTest::newRow("more-components-per-target-than-exist")
            << MeshBuilder()
                       .setVersion(6)
                       .setStride(12)
                       .setEntries(fiveComponents)
                       .setVertexData(QByteArray(12, '\0'))
                       .build();

    QTest::newRow("stride-zero")
            << MeshBuilder()
                       .setVersion(6)
                       .setStride(0)
                       .setEntries({ { "attr_tpos0"_ba, 10, 3, 0 } })
                       .setVertexData(QByteArray(16, '\0'))
                       .build();

    // One entry claiming to belong to target 7, so eight targets are inferred
    // from one entry and the components per target round down to zero.
    QTest::newRow("fewer-entries-than-targets")
            << MeshBuilder()
                       .setVersion(6)
                       .setStride(16)
                       .setEntries({ { "attr_tpos7"_ba, 10, 3, 0 } })
                       .setVertexData(QByteArray(16, '\0'))
                       .build();

    // No vertex data at all, so there is no vertex to build a target from and
    // the vertex count the layout is derived from is zero.
    QTest::newRow("no-vertices")
            << MeshBuilder()
                       .setVersion(6)
                       .setStride(16)
                       .setEntries({ { "attr_tpos0"_ba, 10, 3, 0 } })
                       .setVertexData(QByteArray())
                       .build();

    // A name too short to slice from the seventh byte. Once one target
    // attribute has been seen every later name takes that path, whatever it is.
    QTest::newRow("short-attribute-name-after-a-target")
            << MeshBuilder()
                       .setVersion(6)
                       .setStride(16)
                       .setEntries({ { "attr_tpos0"_ba, 10, 3, 0 }, { "ab"_ba, 10, 3, 12 } })
                       .setVertexData(QByteArray(32, '\0'))
                       .build();

    // An attribute offset past the end of the vertex data it is read from.
    QTest::newRow("entry-offset-past-the-vertex-data")
            << MeshBuilder()
                       .setVersion(6)
                       .setStride(4)
                       .setEntries({ { "attr_tpos0"_ba, 10, 3, 0x7ffffff0 } })
                       .setVertexData(QByteArray(64, '\0'))
                       .build();
}

void tst_QSSGMesh::malformedLegacyMorphTargets()
{
    QFETCH(QByteArray, data);

    const Mesh mesh = load(data);
    QVERIFY(!mesh.isValid());
    QVERIFY(mesh.targetBuffer().data.isEmpty());
}

void tst_QSSGMesh::drawRangesAreValidated_data()
{
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<bool>("valid");

    // Nine indices of four bytes, so nine are available to draw.
    auto indexed = [](quint32 count, quint32 offset) {
        MeshBuilder::Subset subset;
        subset.name = QStringLiteral("s");
        subset.count = count;
        subset.offset = offset;
        return MeshBuilder()
                .setIndexData(QByteArray(4 * 9, '\0'), 5)
                .setSubsets({ subset })
                .build();
    };
    QTest::newRow("whole-index-buffer") << indexed(9, 0) << true;
    QTest::newRow("tail-of-the-index-buffer") << indexed(3, 6) << true;
    QTest::newRow("one-index-too-many") << indexed(10, 0) << false;
    QTest::newRow("one-past-the-end") << indexed(3, 7) << false;
    QTest::newRow("offset-past-the-end") << indexed(1, 4000) << false;
    QTest::newRow("count-wraps") << indexed(0xffffffffu, 8) << false;

    // With no index buffer the ranges are vertices instead: 36 bytes at a
    // stride of 12 is three of them.
    auto nonIndexed = [](quint32 count, quint32 offset) {
        MeshBuilder::Subset subset;
        subset.name = QStringLiteral("s");
        subset.count = count;
        subset.offset = offset;
        return MeshBuilder()
                .setStride(12)
                .setVertexData(QByteArray(36, '\0'))
                .setSubsets({ subset })
                .build();
    };
    QTest::newRow("whole-vertex-buffer") << nonIndexed(3, 0) << true;
    QTest::newRow("one-vertex-too-many") << nonIndexed(4, 0) << false;

    // A level of detail range is drawn the same way and gets the same check.
    MeshBuilder::Subset withLods;
    withLods.name = QStringLiteral("s");
    withLods.count = 9;
    withLods.lods = { { 6, 0, 1.0f }, { 4, 6, 2.0f } };
    QTest::newRow("level-of-detail-past-the-end")
            << MeshBuilder()
                       .setVersion(6)
                       .setIndexData(QByteArray(4 * 9, '\0'), 5)
                       .setSubsets({ withLods })
                       .build()
            << false;
}

void tst_QSSGMesh::drawRangesAreValidated()
{
    QFETCH(QByteArray, data);
    QFETCH(bool, valid);

    QCOMPARE(load(data).isValid(), valid);
}

void tst_QSSGMesh::unknownComponentTypes_data()
{
    QTest::addColumn<QByteArray>("data");

    // getSizeOfType() is Q_UNREACHABLE for anything outside the enumeration, so
    // a component type from the file has to be checked before it gets there.
    for (quint32 bad : { 0u, 12u, 0xffffffffu }) {
        QTest::addRow("index-buffer-%u", bad)
                << MeshBuilder().setIndexData(QByteArray(16, '\0'), bad).build();
        QTest::addRow("attribute-%u", bad)
                << MeshBuilder().setEntries({ { "attr_pos"_ba, bad, 3, 0 } }).build();
        QTest::addRow("morph-target-attribute-%u", bad)
                << MeshBuilder()
                           .setTargetEntries({ { "attr_pos"_ba, bad, 3, 0 } })
                           .setTargetData(QByteArray(16, '\0'), 1)
                           .build();
    }
}

void tst_QSSGMesh::unknownComponentTypes()
{
    QFETCH(QByteArray, data);

    QVERIFY(!load(data).isValid());
}

void tst_QSSGMesh::jointBlockIsSkipped()
{
    // The joint records sit between the level of detail data and the morph
    // target entries, so a reader that does not step over them reads the target
    // entries out of joint data. Nothing Qt writes has joints, but the format
    // has them and Qt 3D Studio wrote them.
    MeshBuilder builder;
    builder.setStride(12)
            .setEntries({ { "attr_pos"_ba, 10, 3, 0 } })
            .setVertexData(QByteArray(36, '\x11'))
            .setJointCount(2)
            .setTargetEntries({ { "attr_pos"_ba, 10, 3, 0 } })
            .setTargetData(QByteArray(48, '\x22'), 1);

    const Mesh mesh = load(builder.build());
    QVERIFY(mesh.isValid());
    QCOMPARE(mesh.targetBuffer().numTargets, 1u);
    QCOMPARE(mesh.targetBuffer().entries.size(), 1);
    QCOMPARE(mesh.targetBuffer().entries.first().name, "attr_pos"_ba);
    QCOMPARE(mesh.targetBuffer().data, QByteArray(48, '\x22'));
    QCOMPARE(mesh.vertexBuffer().data, QByteArray(36, '\x11'));
}

void tst_QSSGMesh::jointCountIsBounded()
{
    const quint32 huge = 0xfffffff0u;
    QVERIFY(!load(MeshBuilder().patchMeshField(11, huge).build()).isValid());
}

void tst_QSSGMesh::fuzzRegressions_data()
{
    QTest::addColumn<QByteArray>("data");

    const QString dir = QFINDTESTDATA(QStringLiteral("data/fuzz"));
    if (dir.isEmpty())
        return;
    const QFileInfoList entries = QDir(dir).entryInfoList({ u"*.mesh"_s }, QDir::Files, QDir::Name);
    for (const QFileInfo &info : entries) {
        QFile f(info.absoluteFilePath());
        if (!f.open(QIODevice::ReadOnly))
            continue;
        QTest::newRow(qPrintable(info.fileName())) << f.readAll();
    }
}

void tst_QSSGMesh::fuzzRegressions()
{
    if (QTest::currentDataTag() == nullptr)
        QSKIP("no fuzz findings recorded yet");
    QFETCH(QByteArray, data);

    // Minimised fuzzer findings. Each one has to either come back invalid or
    // describe itself consistently, never crash.
    const Mesh mesh = load(data);
    if (!mesh.isValid())
        return;
    const Mesh::VertexBuffer vb = mesh.vertexBuffer();
    for (const Mesh::Subset &subset : mesh.subsets())
        QVERIFY(subset.count <= quint32(vb.data.size()) + quint32(mesh.indexBuffer().data.size()));
}

QTEST_APPLESS_MAIN(tst_QSSGMesh)

#include "tst_qssgmesh.moc"
