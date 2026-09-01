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
