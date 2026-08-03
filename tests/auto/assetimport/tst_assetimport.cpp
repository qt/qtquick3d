// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QtTest>
#include <QDebug>
#include <QtQuick3DAssetImport/private/qssgassetimportmanager_p.h>
#include <QtQuick3DUtils/private/qssgmesh_p.h>
#include <QDir>
#include <QByteArray>
#include <QJsonObject>
#include <QTemporaryDir>

// add necessary includes here

class tst_assetimport : public QObject
{
    Q_OBJECT

public:
    tst_assetimport();
    ~tst_assetimport();

private slots:
    void initTestCase();
    void cleanupTestCase();
    void importFile_data();
    void importFile();
    void meshLevelsOfDetailForUnweldedMesh_data();
    void meshLevelsOfDetailForUnweldedMesh();

};

tst_assetimport::tst_assetimport()
{

}

tst_assetimport::~tst_assetimport()
{

}

void tst_assetimport::initTestCase()
{

}

void tst_assetimport::cleanupTestCase()
{

}

void tst_assetimport::importFile_data()
{
    QTest::addColumn<QString>("extension");
    QTest::addColumn<bool>("result");

    QTest::newRow("fbx") << QString("fbx") << true;
    QTest::newRow("dae") << QString("dae") << true;
    QTest::newRow("obj") << QString("obj") << true;
    QTest::newRow("gltf") << QString("gltf") << true;
    QTest::newRow("glb") << QString("glb") << true;
    QTest::newRow("stl") << QString("stl") << true;
    QTest::newRow("ply") << QString("ply") << true;
}

void tst_assetimport::importFile()
{
    QFETCH(QString, extension);
    QFETCH(bool, result);

    QSSGAssetImportManager importManager;
    QString file = "resources/cube_scene." + extension;
    QString error;

    // Should return "true" if there were no errors opening the source or creating the exported object.
    auto importState = importManager.importFile(QFINDTESTDATA(file), QDir("./"), &error);
    const bool realResult = (importState == QSSGAssetImportManager::ImportState::Success);
    if(!error.isEmpty()) {
        if (importState == QSSGAssetImportManager::ImportState::Unsupported) {
            QEXPECT_FAIL("", "Unsupported format!", Continue);
        } else {
            qDebug() << "Error message:" << error;
            QFAIL(error.toStdString().c_str());
        }
    }

    QCOMPARE(realResult, result);
}

// Simplification can only collapse an edge when the faces around it share its
// vertices, so a mesh stored with one vertex per triangle corner has no
// collapsible edge and used to come out of --generateMeshLevelsOfDetail with no
// levels at all. Both assets here are such a mesh - a displaced 8x8 grid whose
// 81 grid points are stored as 384 corner vertices, each carrying its own
// texture coordinate so that nothing upstream welds them first - and they differ
// only in their normals, which is what decides whether welding by position alone
// is safe.
void tst_assetimport::meshLevelsOfDetailForUnweldedMesh_data()
{
    QTest::addColumn<QString>("asset");
    QTest::addColumn<bool>("recalculateLodNormals");
    QTest::addColumn<bool>("expectLevels");

    const QString smooth = QStringLiteral("lod_unwelded_smooth.gltf");
    const QString faceted = QStringLiteral("lod_unwelded_faceted.gltf");

    // Welding by position recovers the real topology, so levels are produced
    // where there were none before
    QTest::newRow("smooth, normals recalculated") << smooth << true << true;
    // Every vertex at a given position carries the same normal here, so welding
    // them together cannot change the shading, whether or not the level's
    // normals are recalculated afterwards
    QTest::newRow("smooth, normals preserved") << smooth << false << true;
    // Faceted normals are safe to weld across as long as the level recalculates
    // them, which is what the default asks for
    QTest::newRow("faceted, normals recalculated") << faceted << true << true;
    // ...but with the stored normals preserved, welding a faceted position would
    // leave every face around it reading one arbitrary face's normal. Those
    // vertices have to stay pinned instead, even though it means no levels.
    QTest::newRow("faceted, normals preserved") << faceted << false << false;
}

void tst_assetimport::meshLevelsOfDetailForUnweldedMesh()
{
    QFETCH(QString, asset);
    QFETCH(bool, recalculateLodNormals);
    QFETCH(bool, expectLevels);

    const QString file = QFINDTESTDATA(QStringLiteral("resources/") + asset);
    QVERIFY(!file.isEmpty());

    const auto option = [](QJsonValue value) {
        QJsonObject option;
        option[QStringLiteral("value")] = value;
        return option;
    };
    QJsonObject options;
    options[QStringLiteral("generateMeshLevelsOfDetail")] = option(true);
    options[QStringLiteral("recalculateLodNormals")] = option(recalculateLodNormals);
    // Spelled out rather than left to the plugin, since an importer that finds
    // no angle in the options is free to fall back to 0.0, which would disable
    // the recalculation the row above just asked for
    options[QStringLiteral("recalculateLodNormalsMergeAngle")] = option(60.0);
    options[QStringLiteral("recalculateLodNormalsSplitAngle")] = option(25.0);

    QTemporaryDir outDir;
    QVERIFY(outDir.isValid());
    const QDir out(outDir.path());

    QSSGAssetImportManager manager;
    QString error;
    if (manager.importFile(file, out, options, &error) != QSSGAssetImportManager::ImportState::Success)
        QSKIP(qPrintable(QStringLiteral("glTF asset could not be converted: ") + error));

    // Each asset holds a single mesh, so the one file written is the one to
    // look at, whatever the generator decided to call it
    const QDir meshDir(out.filePath(QStringLiteral("meshes")));
    const QStringList written = meshDir.entryList({ QStringLiteral("*.mesh") }, QDir::Files);
    QCOMPARE(written.size(), 1);
    QFile meshFile(meshDir.filePath(written.first()));
    QVERIFY(meshFile.open(QIODevice::ReadOnly));

    const QSSGMesh::Mesh loaded = QSSGMesh::Mesh::loadMesh(&meshFile);
    QVERIFY(loaded.isValid());
    const QVector<QSSGMesh::Mesh::Subset> subsets = loaded.subsets();
    QCOMPARE(subsets.size(), 1);

    const QVector<QSSGMesh::Mesh::Lod> lods = subsets.first().lods;
    if (!expectLevels) {
        QCOMPARE(lods.size(), 0);
        return;
    }

    QVERIFY2(!lods.isEmpty(), "welding by position should have made the mesh simplifiable");

    // Subset lods run from the highest level of detail to the lowest, and every
    // one of them has to be a whole number of triangles and coarser than the
    // full resolution mesh the subset itself covers
    const quint32 indexCount = subsets.first().count;
    quint32 previous = indexCount;
    for (const QSSGMesh::Mesh::Lod &lod : lods) {
        QCOMPARE(lod.count % 3, 0u);
        QVERIFY2(lod.count > 0 && lod.count < previous,
                 qPrintable(QStringLiteral("level %1 following %2, full resolution %3")
                                    .arg(lod.count).arg(previous).arg(indexCount)));
        previous = lod.count;
    }
}

QTEST_APPLESS_MAIN(tst_assetimport)

#include "tst_assetimport.moc"
