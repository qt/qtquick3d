// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QtTest>
#include <QDebug>
#include <QtQuick3DAssetImport/private/qssgassetimportmanager_p.h>
#include <QtQuick3DAssetUtils/private/qssgscenedesc_p.h>
#include <QtQuick3DUtils/private/qssgmesh_p.h>
#include <QDir>
#include <QByteArray>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QUrl>
#include <QImageReader>
#include <QRegularExpression>

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
    void idsDoNotLeakBetweenAssets();
    void importUrl_data();
    void importUrl();
    void nativeGltfRouting();
    void gltfNodeVisibilityAndWebp();
    void gltfMaterialVariant_data();
    void gltfMaterialVariant();
    void generatedVariantBinding();
    void materialExtensionMapping();
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

// balsam converts every positional argument in one process, and the QML id
// allocator is a process-wide static keyed by node pointer. Each scene frees
// its nodes before the next is built, so the next scene's nodes land on the
// same addresses and used to inherit the previous asset's ids - the second
// component's root could come out named after a material from the first.
void tst_assetimport::idsDoNotLeakBetweenAssets()
{
    const QString file = QFINDTESTDATA(QStringLiteral("resources/cube_scene.gltf"));
    QVERIFY(!file.isEmpty());

    const auto convertAndRead = [&file](const QDir &outdir) {
        QSSGAssetImportManager manager;
        QString error;
        if (manager.importFile(file, outdir, &error) != QSSGAssetImportManager::ImportState::Success)
            return QString();
        const QStringList generated = outdir.entryList({ QStringLiteral("*.qml") }, QDir::Files);
        if (generated.size() != 1)
            return QString();
        QFile qml(outdir.filePath(generated.first()));
        if (!qml.open(QIODevice::ReadOnly))
            return QString();
        return QString::fromUtf8(qml.readAll());
    };

    QTemporaryDir firstDir;
    QVERIFY(firstDir.isValid());
    QTemporaryDir secondDir;
    QVERIFY(secondDir.isValid());

    const QString first = convertAndRead(QDir(firstDir.path()));
    if (first.isEmpty())
        QSKIP("Asset could not be converted");
    const QString second = convertAndRead(QDir(secondDir.path()));
    QVERIFY(!second.isEmpty());

    // The same input converted twice has to produce the same component
    QCOMPARE(second, first);
    QVERIFY2(first.contains(QStringLiteral("id: root")), qPrintable(first));
}

// The runtime overload picks the importer from the URL rather than from a
// filename, so it has its own path from URL to extension to importer.
void tst_assetimport::importUrl_data()
{
    QTest::addColumn<QUrl>("url");
    QTest::addColumn<QSSGAssetImportManager::ImportState>("result");

    QTest::newRow("local file") << QUrl::fromLocalFile(QFINDTESTDATA("resources/cube_scene.gltf"))
                                << QSSGAssetImportManager::ImportState::Success;
    QTest::newRow("qrc") << QUrl("qrc:/resources/cube_scene.glb") << QSSGAssetImportManager::ImportState::Success;
    QTest::newRow("unsupported extension") << QUrl::fromLocalFile(QFINDTESTDATA("resources/cube_scene.mtl"))
                                           << QSSGAssetImportManager::ImportState::Unsupported;
    // A remote URL cannot be loaded from here, so IoError is the expected
    // outcome. What matters is that it is not Unsupported: the extension has to
    // be read from the path, because the suffix of the whole serialized URL
    // would be "abc" and no importer would ever be found.
    QTest::newRow("query string") << QUrl("http://localhost/cube_scene.gltf?token=abc")
                                  << QSSGAssetImportManager::ImportState::IoError;
}

void tst_assetimport::importUrl()
{
    QFETCH(QUrl, url);
    QFETCH(QSSGAssetImportManager::ImportState, result);

    QSSGAssetImportManager importManager;
    QSSGSceneDesc::Scene scene;
    QString error;

    const auto importState = importManager.importFile(url, scene, &error);
    if (importState != result)
        qDebug() << "Error message:" << error;
    QCOMPARE(importState, result);

    scene.cleanup();
}

// The gltf and glb extensions route to the native glTF importer by default,
// and back to the Assimp importer when QT_QUICK3D_DISABLE_NATIVE_GLTF is
// set. Both paths must import successfully.
void tst_assetimport::nativeGltfRouting()
{
    const QString file = QFINDTESTDATA(QStringLiteral("resources/cube_scene.gltf"));
    QVERIFY(!file.isEmpty());

    {
        QSSGAssetImportManager manager;
        // The native importer plugin must be present and claim gltf/glb
        bool nativePresent = false;
        const auto importers = manager.getImporterPluginInfos();
        for (const auto &importer : importers) {
            if (importer.name == QStringLiteral("gltf")) {
                nativePresent = true;
                QCOMPARE(importer.inputExtensions,
                         QStringList({ QStringLiteral("gltf"), QStringLiteral("glb") }));
            }
        }
        if (!nativePresent)
            QSKIP("Native glTF importer plugin not available");

        QString error;
        const auto state = manager.importFile(file, QDir(QStringLiteral("./")), &error);
        QVERIFY2(state == QSSGAssetImportManager::ImportState::Success, qPrintable(error));
    }

    {
        // The opt-out makes the native importer dormant, and the Assimp
        // importer handles glTF like before
        qputenv("QT_QUICK3D_DISABLE_NATIVE_GLTF", "1");
        const auto cleanup = qScopeGuard([] { qunsetenv("QT_QUICK3D_DISABLE_NATIVE_GLTF"); });

        QSSGAssetImportManager manager;
        const auto importers = manager.getImporterPluginInfos();
        for (const auto &importer : importers) {
            if (importer.name == QStringLiteral("gltf"))
                QVERIFY(importer.inputExtensions.isEmpty());
        }

        QString error;
        const auto state = manager.importFile(file, QDir(QStringLiteral("./")), &error);
        QVERIFY2(state == QSSGAssetImportManager::ImportState::Success, qPrintable(error));
    }
}

static QSSGSceneDesc::Node *findNode(QSSGSceneDesc::Node *node, const QByteArray &name)
{
    if (node->name == name)
        return node;
    for (QSSGSceneDesc::Node *child : std::as_const(node->children)) {
        if (auto *found = findNode(child, name))
            return found;
    }
    return nullptr;
}

static const QSSGSceneDesc::Property *findProperty(const QSSGSceneDesc::Node *node, const QByteArray &name)
{
    for (const QSSGSceneDesc::Property *property : node->properties) {
        if (property->name == name)
            return property;
    }
    return nullptr;
}

// KHR_node_visibility hides a node, and its children through it, and
// EXT_texture_webp takes the WebP image whenever it can be decoded, keeping
// the regular image as the fallback otherwise.
void tst_assetimport::gltfNodeVisibilityAndWebp()
{
    const QString file = QFINDTESTDATA(QStringLiteral("resources/visibility_webp.gltf"));
    QVERIFY(!file.isEmpty());

    QSSGAssetImportManager manager;
    bool nativePresent = false;
    for (const auto &importer : manager.getImporterPluginInfos())
        nativePresent |= importer.name == QStringLiteral("gltf");
    if (!nativePresent)
        QSKIP("Native glTF importer plugin not available");

    QSSGSceneDesc::Scene scene;
    const auto cleanup = qScopeGuard([&scene] { scene.cleanup(); });
    QString error;
    QCOMPARE(manager.importFile(QUrl::fromLocalFile(file), scene, &error),
             QSSGAssetImportManager::ImportState::Success);

    QSSGSceneDesc::Node *hidden = findNode(scene.root, "hidden");
    QVERIFY(hidden);
    const QSSGSceneDesc::Property *visible = findProperty(hidden, "visible");
    QVERIFY(visible);
    QCOMPARE(visible->value.toBool(), false);

    // Visibility cascades through Node.visible, so it is only set where the
    // extension says so
    for (const QByteArray &name : { QByteArrayLiteral("shown"), QByteArrayLiteral("hiddenChild") }) {
        QSSGSceneDesc::Node *node = findNode(scene.root, name);
        QVERIFY2(node, name.constData());
        QVERIFY2(!findProperty(node, "visible"), name.constData());
    }

    // Textures are resources rather than part of the node tree
    const auto it = std::find_if(scene.resources.cbegin(), scene.resources.cend(), [](const QSSGSceneDesc::Node *node) {
        return node->runtimeType == QSSGSceneDesc::Node::RuntimeType::Image2D;
    });
    QVERIFY(it != scene.resources.cend());
    const QSSGSceneDesc::Node *texture = *it;
    const QSSGSceneDesc::Property *source = findProperty(texture, "source");
    QVERIFY(source);
    const bool webpSupported = QImageReader::supportedImageFormats().contains("webp");
    QCOMPARE(source->value.toUrl().fileName(),
             webpSupported ? QStringLiteral("preferred.webp") : QStringLiteral("fallback.png"));
}

void tst_assetimport::gltfMaterialVariant_data()
{
    QTest::addColumn<QString>("variant");
    QTest::addColumn<QByteArrayList>("materials");
    QTest::addColumn<bool>("tangents");
    // Without the option every variant stays switchable, so all materials
    // are kept and tangents follow the one variant that needs them
    QTest::newRow("all variants") << QString()
                                  << QByteArrayList { "defaultMaterial", "redMaterial", "blueMaterial" } << true;
    QTest::newRow("Red") << QStringLiteral("Red") << QByteArrayList { "redMaterial" } << false;
    QTest::newRow("Blue") << QStringLiteral("Blue") << QByteArrayList { "blueMaterial" } << true;
    QTest::newRow("unknown") << QStringLiteral("Green") << QByteArrayList { "defaultMaterial" } << false;
}

// The baked KHR_materials_variants variant decides both the materials and,
// as only blueMaterial has a normal map, whether tangents are generated
void tst_assetimport::gltfMaterialVariant()
{
    QFETCH(QString, variant);
    QFETCH(QByteArrayList, materials);
    QFETCH(bool, tangents);

    const QString file = QFINDTESTDATA(QStringLiteral("resources/material_variants.gltf"));
    QVERIFY(!file.isEmpty());

    QSSGAssetImportManager manager;
    bool nativePresent = false;
    for (const auto &importer : manager.getImporterPluginInfos())
        nativePresent |= importer.name == QStringLiteral("gltf");
    if (!nativePresent)
        QSKIP("Native glTF importer plugin not available");

    QJsonObject options;
    if (!variant.isEmpty())
        options.insert(QStringLiteral("materialVariant"), QJsonObject { { QStringLiteral("value"), variant } });

    if (QTest::currentDataTag() == QByteArrayLiteral("unknown"))
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("no material variant \"Green\"")));

    QSSGSceneDesc::Scene scene;
    const auto cleanup = qScopeGuard([&scene] { scene.cleanup(); });
    QString error;
    QCOMPARE(manager.importFile(QUrl::fromLocalFile(file), scene, options, &error),
             QSSGAssetImportManager::ImportState::Success);

    QByteArrayList imported;
    for (const QSSGSceneDesc::Node *node : scene.resources) {
        if (node->nodeType == QSSGSceneDesc::Node::Type::Material)
            imported.append(node->name);
    }
    QCOMPARE(imported, materials);

    QCOMPARE(scene.meshStorage.size(), 1);
    bool hasTangents = false;
    for (const auto &entry : scene.meshStorage.first().vertexBuffer().entries)
        hasTangents |= entry.name == QSSGMesh::MeshInternal::getTexTanAttrName();
    QCOMPARE(hasTangents, tangents);
}

// The generated component binds each varying model's material list to the
// root's materialVariant property. balsam converts several assets in one
// process and the id allocator is process-wide, so the second root is not
// called "root" - the binding has to use whatever id the root actually got.
void tst_assetimport::generatedVariantBinding()
{
    const QString file = QFINDTESTDATA(QStringLiteral("resources/material_variants_binding.gltf"));
    QVERIFY(!file.isEmpty());

    const auto convertAndRead = [&file](const QDir &outdir) {
        QSSGAssetImportManager manager;
        QString error;
        const auto state = manager.importFile(file, outdir, &error);
        if (state != QSSGAssetImportManager::ImportState::Success)
            return QString();
        const QStringList generated = outdir.entryList({ QStringLiteral("*.qml") }, QDir::Files);
        if (generated.size() != 1)
            return QString();
        QFile qml(outdir.filePath(generated.first()));
        if (!qml.open(QIODevice::ReadOnly))
            return QString();
        return QString::fromUtf8(qml.readAll());
    };

    QTemporaryDir first;
    QVERIFY(first.isValid());
    QTemporaryDir second;
    QVERIFY(second.isValid());

    // Both conversions happen in this one process, as they do in balsam
    const QString firstQml = convertAndRead(QDir(first.path()));
    if (firstQml.isEmpty())
        QSKIP("glTF asset could not be converted");
    const QString secondQml = convertAndRead(QDir(second.path()));
    QVERIFY(!secondQml.isEmpty());

    const QRegularExpression rootIdRe(QStringLiteral("\\bid: (root[0-9]*)\\b"));
    for (const QString &qml : { firstQml, secondQml }) {
        // The variant list is exposed as a typed list, matching RuntimeLoader
        QVERIFY2(qml.contains(QStringLiteral("readonly property list<string> materialVariants: [\"Red\", \"Blue\"]")),
                 qPrintable(qml));
        QVERIFY(qml.contains(QStringLiteral("property string materialVariant:")));

        // Whatever id the root got, the binding must reference that id
        const auto match = rootIdRe.match(qml);
        QVERIFY2(match.hasMatch(), qPrintable(qml));
        const QString rootId = match.captured(1);
        QVERIFY(!rootId.isEmpty());
        QVERIFY2(qml.contains(rootId + QStringLiteral(".materialVariant === \"Red\"")), qPrintable(qml));

        // Only the model with variant mappings gets a ternary chain; the plain
        // one keeps a static material list
        QCOMPARE(qml.count(QStringLiteral(".materialVariant === ")), 2);
    }
}

void tst_assetimport::materialExtensionMapping()
{
    const QString file = QFINDTESTDATA(QStringLiteral("resources/brdf_extensions.gltf"));
    QVERIFY(!file.isEmpty());

    QSSGAssetImportManager manager;
    bool nativePresent = false;
    const auto importers = manager.getImporterPluginInfos();
    for (const auto &importer : importers) {
        if (importer.name == QStringLiteral("gltf"))
            nativePresent = true;
    }
    if (!nativePresent)
        QSKIP("Native glTF importer plugin not available");

    QTemporaryDir outDir;
    QVERIFY(outDir.isValid());

    QString error;
    const auto state = manager.importFile(file, QDir(outDir.path()), &error);
    QVERIFY2(state == QSSGAssetImportManager::ImportState::Success, qPrintable(error));

    const QStringList generated = QDir(outDir.path()).entryList(QStringList { QStringLiteral("*.qml") });
    QCOMPARE(generated.size(), 1);
    QFile qml(outDir.filePath(generated.first()));
    QVERIFY(qml.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString contents = QString::fromUtf8(qml.readAll());

    // The values are exactly representable as floats, so they are matched whole

    // Sheen. The color factor is linear in the asset and has to come out as an
    // sRGB color: linear 0.25, 0.5, 1.0 is #89bbff.
    QVERIFY2(contents.contains(QStringLiteral("sheenColor: \"#ff89bbff\"\n")), qPrintable(contents));
    QVERIFY(contents.contains(QStringLiteral("sheenRoughness: 0.25\n")));
    QVERIFY(contents.contains(QStringLiteral("sheenColorMap:")));
    QVERIFY(contents.contains(QStringLiteral("sheenRoughnessMap:")));

    // Anisotropy. The asset states the rotation in radians, the property is in
    // degrees, so pi/4 has to arrive as 45.
    QVERIFY(contents.contains(QStringLiteral("anisotropyStrength: 0.625\n")));
    QVERIFY2(contents.contains(QStringLiteral("anisotropyRotation: 45\n")), qPrintable(contents));
    QVERIFY(contents.contains(QStringLiteral("anisotropyMap:")));

    // Iridescence, including the property renamed from the extension's
    // iridescenceIor, and the thickness range in nanometers
    QVERIFY(contents.contains(QStringLiteral("iridescenceFactor: 0.875\n")));
    QVERIFY(contents.contains(QStringLiteral("iridescenceIndexOfRefraction: 1.75\n")));
    QVERIFY(contents.contains(QStringLiteral("iridescenceThicknessMinimum: 220\n")));
    QVERIFY(contents.contains(QStringLiteral("iridescenceThicknessMaximum: 810\n")));
    QVERIFY(contents.contains(QStringLiteral("iridescenceMap:")));
    QVERIFY(contents.contains(QStringLiteral("iridescenceThicknessMap:")));

    // Dispersion, which needs the transmission and volume the asset also sets
    QVERIFY(contents.contains(QStringLiteral("dispersion: 0.5\n")));
    QVERIFY(contents.contains(QStringLiteral("transmissionFactor: 0.75\n")));
    QVERIFY(contents.contains(QStringLiteral("thicknessFactor: 2.5\n")));
}

QTEST_APPLESS_MAIN(tst_assetimport)

#include "tst_assetimport.moc"
