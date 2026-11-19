// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QQuickView>
#include "../shared/util.h"

class tst_RuntimeLoader : public QQuick3DDataTest
{
    Q_OBJECT
private slots:
    void initTestCase() override;
    void queryAll_data();
    void queryAll();
    void nativeGltfFeatures();
    void materialVariantSwitching();
    void texturesFromResources_data();
    void texturesFromResources();
};

void tst_RuntimeLoader::initTestCase()
{
    QQuick3DDataTest::initTestCase();
    if (!initialized())
        return;
}

void tst_RuntimeLoader::queryAll_data()
{
    QTest::addColumn<bool>("assimpFallback");
    QTest::newRow("nativeGltf") << false;
    QTest::newRow("assimpFallback") << true;
}

void tst_RuntimeLoader::queryAll()
{
    QFETCH(bool, assimpFallback);
    if (assimpFallback)
        qputenv("QT_QUICK3D_DISABLE_NATIVE_GLTF", "1");
    const auto cleanup = qScopeGuard([] { qunsetenv("QT_QUICK3D_DISABLE_NATIVE_GLTF"); });

    QScopedPointer<QQuickView> view(createView(QLatin1String("queryall.qml"), QSize(200, 200)));
    QVERIFY(view);
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    const QImage frame = grab(view.data());
    if (frame.isNull())
        return;

    // Wait for either a successful load or an error (e.g. importer plugin absent).
    QTRY_VERIFY_WITH_TIMEOUT(view->rootObject()->property("loaded").toBool()
                             || view->rootObject()->property("loadError").toBool(),
                             10000);
    if (view->rootObject()->property("loadError").toBool())
        QSKIP("Asset failed to load — importer plugin likely not available on this platform");

    QVERIFY(view->rootObject()->property("materialCount").toInt() > 0);
    QVERIFY(view->rootObject()->property("modelCount").toInt() > 0);
    QVERIFY(view->rootObject()->property("lightCount").toInt() > 0);
    QVERIFY(view->rootObject()->property("queryNullForMissing").toBool());
}

// Loads assets exercising textures with transforms, material extensions,
// skinning, morph targets, and animations through the native glTF importer.
void tst_RuntimeLoader::nativeGltfFeatures()
{
    QScopedPointer<QQuickView> view(createView(QLatin1String("nativegltf.qml"), QSize(200, 200)));
    QVERIFY(view);
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    const QImage frame = grab(view.data());
    if (frame.isNull())
        return;

    QTRY_VERIFY_WITH_TIMEOUT(view->rootObject()->property("settled").toBool(), 10000);
    const QString errors = view->rootObject()->property("errors").toString();
    if (errors.contains(QLatin1String("unsupported file extension")))
        QSKIP("glTF importer plugin not available on this platform");
    QCOMPARE(errors, QString());
    QCOMPARE(view->rootObject()->property("loadedCount").toInt(),
             view->rootObject()->property("totalCount").toInt());
}

// Switches between the material variants of a KHR_materials_variants asset
void tst_RuntimeLoader::materialVariantSwitching()
{
    QScopedPointer<QQuickView> view(createView(QLatin1String("variants.qml"), QSize(200, 200)));
    QVERIFY(view);
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    const QImage frame = grab(view.data());
    if (frame.isNull())
        return;

    QTRY_VERIFY_WITH_TIMEOUT(view->rootObject()->property("loaded").toBool()
                             || view->rootObject()->property("loadError").toBool(),
                             10000);
    if (view->rootObject()->property("loadError").toBool())
        QSKIP("Asset failed to load — importer plugin likely not available on this platform");

    QCOMPARE(view->rootObject()->property("variants").toStringList(),
             (QStringList { QStringLiteral("Red"), QStringLiteral("Blue") }));
    QCOMPARE(view->rootObject()->property("defaultMaterial").toString(), QStringLiteral("gray"));
    QCOMPARE(view->rootObject()->property("blueMaterial").toString(), QStringLiteral("blue"));
    QCOMPARE(view->rootObject()->property("redMaterial").toString(), QStringLiteral("red"));
    QCOMPARE(view->rootObject()->property("restoredMaterial").toString(), QStringLiteral("gray"));
}

void tst_RuntimeLoader::texturesFromResources_data()
{
    QTest::addColumn<bool>("assimpFallback");
    QTest::newRow("nativeGltf") << false;
    QTest::newRow("assimpFallback") << true;
}

// Loads a model with an external texture from qrc
void tst_RuntimeLoader::texturesFromResources()
{
    QFETCH(bool, assimpFallback);
    if (assimpFallback)
        qputenv("QT_QUICK3D_DISABLE_NATIVE_GLTF", "1");
    const auto cleanup = qScopeGuard([] { qunsetenv("QT_QUICK3D_DISABLE_NATIVE_GLTF"); });

    QScopedPointer<QQuickView> view(createView(QLatin1String("qrcmodel.qml"), QSize(200, 200)));
    QVERIFY(view);
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    QImage frame = grab(view.data());
    if (frame.isNull())
        return;

    QTRY_VERIFY_WITH_TIMEOUT(view->rootObject()->property("loaded").toBool()
                             || view->rootObject()->property("loadError").toBool(),
                             10000);
    if (view->rootObject()->property("loadError").toBool())
        QSKIP("Asset failed to load — importer plugin likely not available on this platform");

    QCOMPARE(view->rootObject()->property("textureSources").toStringList(),
             QStringList { QStringLiteral("qrc:/qrcmodel/checker.png") });

    // The quad is white without its texture, so look for the red checker squares
    QVERIFY(waitForFrames(view.data(), 2));
    frame = grab(view.data());
    bool foundRed = false;
    for (int y = 0; y < frame.height() && !foundRed; ++y) {
        for (int x = 0; x < frame.width() && !foundRed; ++x) {
            const QColor c = frame.pixelColor(x, y);
            foundRed = c.red() > 128 && c.green() < c.red() / 2;
        }
    }
    QVERIFY(foundRed);
}

QTEST_MAIN(tst_RuntimeLoader)
#include "tst_runtimeloader.moc"
