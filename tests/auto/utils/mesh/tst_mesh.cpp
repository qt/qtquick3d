// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest>

#include <QtQuick3DUtils/private/qssgmesh_p.h>

#include <QtGui/QVector3D>

class tst_mesh : public QObject
{
    Q_OBJECT

private slots:
    void weldVertices();
    void weldVerticesComparesEveryStream();
    void levelsOfDetailWithoutNormals();
    void levelsOfDetailClearsSplitVertices();
    void levelsOfDetailFromUnweldedMesh();
};

// Two triangles given as six separate vertices, where three of them repeat a
// position and normal that another vertex already has, so welding has to find
// exactly four unique vertices.
static void makeQuadSoup(QVector<QVector3D> &positions, QVector<QVector3D> &normals, QVector<quint32> &indexes)
{
    const QVector3D up(0.0f, 0.0f, 1.0f);
    positions = { { 0, 0, 0 }, { 1, 0, 0 }, { 1, 1, 0 }, { 0, 0, 0 }, { 1, 1, 0 }, { 0, 1, 0 } };
    normals = { up, up, up, up, up, up };
    indexes = { 0, 1, 2, 3, 4, 5 };
}

static QVector<unsigned int> weld(const QVector<QVector3D> &positions,
                                  const QVector<QVector3D> &normals,
                                  const QVector<quint32> &indexes,
                                  size_t *uniqueCount)
{
    const QSSGMesh::MeshVertexStream streams[] = {
        { positions.constData(), sizeof(QVector3D), sizeof(QVector3D) },
        { normals.constData(), sizeof(QVector3D), sizeof(QVector3D) }
    };

    QVector<unsigned int> remap(positions.size());
    *uniqueCount = QSSGMesh::generateVertexRemap(remap.data(),
                                                 indexes.constData(),
                                                 indexes.size(),
                                                 positions.size(),
                                                 streams,
                                                 std::size(streams));
    return remap;
}

void tst_mesh::weldVertices()
{
    QVector<QVector3D> positions;
    QVector<QVector3D> normals;
    QVector<quint32> indexes;
    makeQuadSoup(positions, normals, indexes);

    size_t uniqueCount = 0;
    const QVector<unsigned int> remap = weld(positions, normals, indexes, &uniqueCount);
    QCOMPARE(uniqueCount, size_t(4));

    QVector<QVector3D> weldedPositions(uniqueCount);
    QSSGMesh::remapVertexBuffer(weldedPositions.data(),
                                positions.constData(),
                                positions.size(),
                                sizeof(QVector3D),
                                remap.constData());

    QVector<unsigned int> weldedIndexes(indexes.size());
    QSSGMesh::remapIndexBuffer(weldedIndexes.data(),
                               indexes.constData(),
                               indexes.size(),
                               remap.constData());

    // The whole point of the remap: drawing the welded buffers has to produce
    // the same geometry as drawing the original ones.
    QCOMPARE(weldedIndexes.size(), indexes.size());
    for (qsizetype i = 0; i < indexes.size(); ++i) {
        QVERIFY(weldedIndexes[i] < uniqueCount);
        QCOMPARE(weldedPositions[weldedIndexes[i]], positions[indexes[i]]);
    }
}

void tst_mesh::weldVerticesComparesEveryStream()
{
    QVector<QVector3D> positions;
    QVector<QVector3D> normals;
    QVector<quint32> indexes;
    makeQuadSoup(positions, normals, indexes);

    // A hard edge is a shared position carrying two different normals, so
    // these two vertices must survive as separate ones even though welding on
    // position alone would merge them.
    normals[3] = QVector3D(0.0f, 1.0f, 0.0f);

    size_t uniqueCount = 0;
    weld(positions, normals, indexes, &uniqueCount);
    QCOMPARE(uniqueCount, size_t(5));
}

// A height field, so that simplification has something to lose and normals
// actually vary across the surface.
static void makeHeightField(int quadsPerSide,
                            QVector<QVector3D> &positions,
                            QVector<QVector3D> &normals,
                            QVector<quint32> &indexes)
{
    for (int y = 0; y <= quadsPerSide; ++y) {
        for (int x = 0; x <= quadsPerSide; ++x) {
            const float fx = float(x);
            const float fy = float(y);
            positions.append(QVector3D(fx, fy, std::sin(fx * 0.5f) * std::cos(fy * 0.5f)));
            // The analytic normal of z = sin(x/2)cos(y/2)
            const QVector3D normal(-0.5f * std::cos(fx * 0.5f) * std::cos(fy * 0.5f),
                                   0.5f * std::sin(fx * 0.5f) * std::sin(fy * 0.5f),
                                   1.0f);
            normals.append(normal.normalized());
        }
    }

    const int stride = quadsPerSide + 1;
    for (int y = 0; y < quadsPerSide; ++y) {
        for (int x = 0; x < quadsPerSide; ++x) {
            const quint32 i0 = quint32(y * stride + x);
            const quint32 i1 = i0 + 1;
            const quint32 i2 = i0 + quint32(stride);
            const quint32 i3 = i2 + 1;
            indexes << i0 << i1 << i2 << i1 << i3 << i2;
        }
    }
}

// Normals are optional in glTF, so the importer calls this with an empty
// normals vector. Correcting normals then has nothing to read, and must be
// skipped rather than indexing past the end of the vector.
void tst_mesh::levelsOfDetailWithoutNormals()
{
    QVector<QVector3D> positions;
    QVector<QVector3D> normals;
    QVector<quint32> indexes;
    makeHeightField(12, positions, normals, indexes);

    QVector<QSSGMesh::MeshVertexSplit> splitVertices;
    const auto lods = QSSGMesh::generateMeshLevelsOfDetail(positions, {}, indexes, splitVertices);

    // No normals means no normal correction, so no vertex can have been split
    QVERIFY(splitVertices.isEmpty());
    for (const auto &lod : lods) {
        for (const quint32 index : lod.indexes)
            QVERIFY(index < quint32(positions.size()));
    }
}

// The split vertices are reported positionally, starting at positions.size(),
// so anything left over from an earlier call would make the caller append the
// wrong vertices.
void tst_mesh::levelsOfDetailClearsSplitVertices()
{
    QVector<QVector3D> positions;
    QVector<QVector3D> normals;
    QVector<quint32> indexes;
    makeHeightField(12, positions, normals, indexes);

    constexpr quint32 sentinel = 123456;
    QVector<QSSGMesh::MeshVertexSplit> splitVertices;
    splitVertices.append({ sentinel, QVector3D(1.0f, 0.0f, 0.0f) });

    const auto lods = QSSGMesh::generateMeshLevelsOfDetail(positions, normals, indexes, splitVertices);

    for (const auto &split : splitVertices)
        QVERIFY(split.sourceIndex != sentinel);

    const quint32 vertexCount = quint32(positions.size() + splitVertices.size());
    for (const auto &lod : lods) {
        for (const quint32 index : lod.indexes)
            QVERIFY(index < vertexCount);
    }
}

// An edge can only collapse when the faces around it share its vertices, so a
// vertex buffer that repeats a position pins every edge touching it. Assets are
// commonly exported with one vertex per triangle corner, which pins the whole
// mesh, and simplification then returns the input unchanged - which used to
// leave the asset with no levels at all.
void tst_mesh::levelsOfDetailFromUnweldedMesh()
{
    QVector<QVector3D> positions;
    QVector<QVector3D> normals;
    QVector<quint32> indexes;
    makeHeightField(24, positions, normals, indexes);

    // Give every triangle corner a vertex of its own
    QVector<QVector3D> soupPositions;
    QVector<QVector3D> soupNormals;
    QVector<quint32> soupIndexes;
    for (const quint32 index : indexes) {
        soupIndexes.append(quint32(soupPositions.size()));
        soupPositions.append(positions.at(index));
        soupNormals.append(normals.at(index));
    }
    QCOMPARE(soupPositions.size(), soupIndexes.size());

    QVector<QSSGMesh::MeshVertexSplit> splitVertices;
    const auto lods = QSSGMesh::generateMeshLevelsOfDetail(soupPositions, soupNormals, soupIndexes,
                                                           splitVertices);

    QVERIFY(!lods.isEmpty());

    const quint32 vertexCount = quint32(soupPositions.size() + splitVertices.size());
    qsizetype previousSize = 0;
    for (const auto &lod : lods) {
        // Levels are produced coarsest first, and each one has to be a real
        // reduction of the original
        QCOMPARE(lod.indexes.size() % 3, 0);
        QVERIFY(lod.indexes.size() > previousSize);
        QVERIFY(lod.indexes.size() < soupIndexes.size());
        previousSize = lod.indexes.size();

        for (const quint32 index : lod.indexes)
            QVERIFY(index < vertexCount);
    }
}

QTEST_APPLESS_MAIN(tst_mesh)

#include "tst_mesh.moc"
