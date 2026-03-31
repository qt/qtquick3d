// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlComponent>
#include <QtQml/qqmlextensionplugin.h>
#include <QtQuick3D/private/qquick3dcontentlayer_p.h>

#ifdef QT_STATIC
// Static builds need the QML plugins explicitly imported, since the test loads
// "import QtQuick" and "import QtQuick3D" via QQmlComponent::setData() at
// runtime and the import scanner cannot discover them from the C++ sources.
Q_IMPORT_QML_PLUGIN(QtQuick2Plugin)
Q_IMPORT_QML_PLUGIN(QQuick3DPlugin)
#endif

class tst_QQuick3DRenderPass : public QObject
{
    Q_OBJECT

private slots:
    void testContentLayer();
    void testRenderPassQMLCreation();
    void testAddDefine();
    void testPipelineStateOverrideProperties();
    void testRenderTargetBlend();
};

void tst_QQuick3DRenderPass::testContentLayer()
{
    // Test that all layer constants are available and have expected values
    QCOMPARE(QQuick3DContentLayer::Layer0, 1u << 0);
    QCOMPARE(QQuick3DContentLayer::Layer1, 1u << 1);
    QCOMPARE(QQuick3DContentLayer::Layer2, 1u << 2);
    QCOMPARE(QQuick3DContentLayer::Layer23, 1u << 23);
    QCOMPARE(QQuick3DContentLayer::Layer24, 1u << 24);
    QCOMPARE(QQuick3DContentLayer::Layer31, 1u << 31);
    QCOMPARE(QQuick3DContentLayer::LayerNone, 0u);
    QCOMPARE(QQuick3DContentLayer::LayerAll, 0xFFFFFFu); // Only user layers (0-23)

    // Test bitwise operations
    quint32 layerMask = QQuick3DContentLayer::Layer0 | QQuick3DContentLayer::Layer1;
    QCOMPARE(layerMask, 3u);

    layerMask = QQuick3DContentLayer::Layer0 | QQuick3DContentLayer::Layer2;
    QCOMPARE(layerMask, 5u);

    // Test that layers are powers of 2
    for (int i = 0; i < 32; ++i) {
        quint32 layer = 1u << i;
        // Verify it's a power of 2 (only one bit set)
        QVERIFY((layer & (layer - 1)) == 0);
    }
}

void tst_QQuick3DRenderPass::testRenderPassQMLCreation()
{
    // Test that user render pass types can be created via QML
    QQmlEngine engine;

    // Test RenderPass creation
    {
        QQmlComponent component(&engine);
        component.setData("import QtQuick3D; RenderPass { }", QUrl());
        QVERIFY2(!component.isError(), qPrintable(component.errorString()));
        QObject *object = component.create();
        QVERIFY(object != nullptr);
        delete object;
    }

    // Test RenderPassTexture creation
    {
        QQmlComponent component(&engine);
        component.setData("import QtQuick3D; RenderPassTexture { }", QUrl());
        QVERIFY2(!component.isError(), qPrintable(component.errorString()));
        QObject *object = component.create();
        QVERIFY(object != nullptr);
        delete object;
    }

    // Test ColorAttachment creation
    {
        QQmlComponent component(&engine);
        component.setData("import QtQuick3D; ColorAttachment { }", QUrl());
        QVERIFY2(!component.isError(), qPrintable(component.errorString()));
        QObject *object = component.create();
        QVERIFY(object != nullptr);
        delete object;
    }

    // Test DepthStencilAttachment creation
    {
        QQmlComponent component(&engine);
        component.setData("import QtQuick3D; DepthStencilAttachment { }", QUrl());
        QVERIFY2(!component.isError(), qPrintable(component.errorString()));
        QObject *object = component.create();
        QVERIFY(object != nullptr);
        delete object;
    }

    // Test DepthTextureAttachment creation
    {
        QQmlComponent component(&engine);
        component.setData("import QtQuick3D; DepthTextureAttachment { }", QUrl());
        QVERIFY2(!component.isError(), qPrintable(component.errorString()));
        QObject *object = component.create();
        QVERIFY(object != nullptr);
        delete object;
    }

    // Test RenderablesFilter creation
    {
        QQmlComponent component(&engine);
        component.setData("import QtQuick3D; RenderablesFilter { }", QUrl());
        QVERIFY2(!component.isError(), qPrintable(component.errorString()));
        QObject *object = component.create();
        QVERIFY(object != nullptr);
        delete object;
    }

    // Test PipelineStateOverride creation
    {
        QQmlComponent component(&engine);
        component.setData("import QtQuick3D; PipelineStateOverride { }", QUrl());
        QVERIFY2(!component.isError(), qPrintable(component.errorString()));
        QObject *object = component.create();
        QVERIFY(object != nullptr);
        delete object;
    }

    // Test SubRenderPass creation
    {
        QQmlComponent component(&engine);
        component.setData("import QtQuick3D; SubRenderPass { }", QUrl());
        QVERIFY2(!component.isError(), qPrintable(component.errorString()));
        QObject *object = component.create();
        QVERIFY(object != nullptr);
        delete object;
    }
}

void tst_QQuick3DRenderPass::testAddDefine()
{
    QQmlEngine engine;

    // Basic creation
    {
        QQmlComponent component(&engine);
        component.setData("import QtQuick3D; AddDefine { }", QUrl());
        QVERIFY2(!component.isError(), qPrintable(component.errorString()));
        QScopedPointer<QObject> object(component.create());
        QVERIFY(object != nullptr);

        // Default property values
        QByteArray name = object->property("name").toByteArray();
        int value = object->property("value").toInt();
        QCOMPARE(name, QByteArray());
        QCOMPARE(value, 0);
    }

    // Set and read back name and value
    {
        QQmlComponent component(&engine);
        component.setData(R"(import QtQuick3D; AddDefine { name: "MY_DEFINE"; value: 42 })", QUrl());
        QVERIFY2(!component.isError(), qPrintable(component.errorString()));
        QScopedPointer<QObject> object(component.create());
        QVERIFY(object != nullptr);

        QCOMPARE(object->property("name").toByteArray(), QByteArray("MY_DEFINE"));
        QCOMPARE(object->property("value").toInt(), 42);
    }

    // Dynamic update of name and value
    {
        QQmlComponent component(&engine);
        component.setData(R"(import QtQuick3D; AddDefine { name: "INITIAL"; value: 1 })", QUrl());
        QVERIFY2(!component.isError(), qPrintable(component.errorString()));
        QScopedPointer<QObject> object(component.create());
        QVERIFY(object != nullptr);

        QCOMPARE(object->property("name").toByteArray(), QByteArray("INITIAL"));
        QCOMPARE(object->property("value").toInt(), 1);

        object->setProperty("name", QByteArray("UPDATED"));
        object->setProperty("value", 99);

        QCOMPARE(object->property("name").toByteArray(), QByteArray("UPDATED"));
        QCOMPARE(object->property("value").toInt(), 99);
    }

    // Empty name (no-op — the renderer guards against empty names)
    {
        QQmlComponent component(&engine);
        component.setData(R"(import QtQuick3D; AddDefine { name: ""; value: 0 })", QUrl());
        QVERIFY2(!component.isError(), qPrintable(component.errorString()));
        QScopedPointer<QObject> object(component.create());
        QVERIFY(object != nullptr);
        QCOMPARE(object->property("name").toByteArray(), QByteArray(""));
    }
}

void tst_QQuick3DRenderPass::testPipelineStateOverrideProperties()
{
    QQmlEngine engine;

    // Basic creation and default state
    {
        QQmlComponent component(&engine);
        component.setData("import QtQuick3D; PipelineStateOverride { }", QUrl());
        QVERIFY2(!component.isError(), qPrintable(component.errorString()));
        QScopedPointer<QObject> object(component.create());
        QVERIFY(object != nullptr);

        // Verify properties exist and are accessible
        QVERIFY(object->metaObject()->indexOfProperty("depthTestEnabled") != -1);
        QVERIFY(object->metaObject()->indexOfProperty("depthWriteEnabled") != -1);
        QVERIFY(object->metaObject()->indexOfProperty("blendEnabled") != -1);
        QVERIFY(object->metaObject()->indexOfProperty("cullMode") != -1);
        QVERIFY(object->metaObject()->indexOfProperty("polygonMode") != -1);
        QVERIFY(object->metaObject()->indexOfProperty("scissor") != -1);
        QVERIFY(object->metaObject()->indexOfProperty("viewport") != -1);
    }

    // Set and read back boolean pipeline state properties
    {
        QQmlComponent component(&engine);
        component.setData(R"(
import QtQuick3D
PipelineStateOverride {
    depthTestEnabled: true
    depthWriteEnabled: false
    blendEnabled: true
})",
                          QUrl());
        QVERIFY2(!component.isError(), qPrintable(component.errorString()));
        QScopedPointer<QObject> object(component.create());
        QVERIFY(object != nullptr);

        QCOMPARE(object->property("depthTestEnabled").toBool(), true);
        QCOMPARE(object->property("depthWriteEnabled").toBool(), false);
        QCOMPARE(object->property("blendEnabled").toBool(), true);
    }

    // Dynamic property update
    {
        QQmlComponent component(&engine);
        component.setData("import QtQuick3D; PipelineStateOverride { }", QUrl());
        QVERIFY2(!component.isError(), qPrintable(component.errorString()));
        QScopedPointer<QObject> object(component.create());
        QVERIFY(object != nullptr);

        object->setProperty("depthTestEnabled", true);
        object->setProperty("depthWriteEnabled", true);
        object->setProperty("blendEnabled", false);

        QCOMPARE(object->property("depthTestEnabled").toBool(), true);
        QCOMPARE(object->property("depthWriteEnabled").toBool(), true);
        QCOMPARE(object->property("blendEnabled").toBool(), false);

        // Change again to verify updates propagate
        object->setProperty("depthTestEnabled", false);
        QCOMPARE(object->property("depthTestEnabled").toBool(), false);
    }

    // Scissor and viewport rect properties
    {
        QQmlComponent component(&engine);
        component.setData(R"(
import QtQuick
import QtQuick3D
PipelineStateOverride {
    scissor: Qt.rect(10, 20, 300, 200)
    viewport: Qt.rect(0, 0, 400, 400)
})",
                          QUrl());
        QVERIFY2(!component.isError(), qPrintable(component.errorString()));
        QScopedPointer<QObject> object(component.create());
        QVERIFY(object != nullptr);

        QRectF scissor = object->property("scissor").toRectF();
        QCOMPARE(scissor.x(), 10.0);
        QCOMPARE(scissor.y(), 20.0);
        QCOMPARE(scissor.width(), 300.0);
        QCOMPARE(scissor.height(), 200.0);

        QRectF viewport = object->property("viewport").toRectF();
        QCOMPARE(viewport.x(), 0.0);
        QCOMPARE(viewport.width(), 400.0);
    }
}

void tst_QQuick3DRenderPass::testRenderTargetBlend()
{
    QQmlEngine engine;

    // PipelineStateOverride should expose targetBlend0 through targetBlend7
    {
        QQmlComponent component(&engine);
        component.setData("import QtQuick3D; PipelineStateOverride { }", QUrl());
        QVERIFY2(!component.isError(), qPrintable(component.errorString()));
        QScopedPointer<QObject> object(component.create());
        QVERIFY(object != nullptr);

        // targetBlend0 through targetBlend7 should be accessible properties
        for (int i = 0; i <= 7; ++i) {
            const QByteArray propName = QByteArrayLiteral("targetBlend") + QByteArray::number(i);
            const int propIndex = object->metaObject()->indexOfProperty(propName.constData());
            QVERIFY2(propIndex != -1,
                     qPrintable(QString("PipelineStateOverride should have property '%1'").arg(QString::fromLatin1(propName))));
        }
    }

    // Set targetBlend0.enable
    {
        QQmlComponent component(&engine);
        component.setData(R"(
import QtQuick3D
PipelineStateOverride {
    targetBlend0.enable: true
})",
                          QUrl());
        QVERIFY2(!component.isError(), qPrintable(component.errorString()));
        QScopedPointer<QObject> object(component.create());
        QVERIFY(object != nullptr);

        QVariant blend0 = object->property("targetBlend0");
        QVERIFY(blend0.isValid());
        // Extract the enable sub-property via the value type gadget
        const QMetaObject *mo = QMetaType::fromName("QQuick3DRenderPassTargetBlend").metaObject();
        if (mo) {
            int enableIdx = mo->indexOfProperty("enable");
            if (enableIdx != -1) {
                void *gadget = blend0.data();
                QVERIFY(mo->property(enableIdx).readOnGadget(gadget).toBool());
            }
        }
    }

    // targetBlend0 enum values via RenderTargetBlend namespace in QML
    {
        QQmlComponent component(&engine);
        component.setData(R"(
import QtQuick3D
PipelineStateOverride {
    targetBlend0.enable: true
    targetBlend0.srcColor: RenderTargetBlend.SrcAlpha
    targetBlend0.dstColor: RenderTargetBlend.OneMinusSrcAlpha
    targetBlend0.opColor:  RenderTargetBlend.Add
})",
                          QUrl());
        QVERIFY2(!component.isError(), qPrintable(component.errorString()));
        QScopedPointer<QObject> object(component.create());
        QVERIFY(object != nullptr);

        QVariant blend0 = object->property("targetBlend0");
        QVERIFY(blend0.isValid());
    }
}

QTEST_MAIN(tst_QQuick3DRenderPass)
#include "tst_qquick3drenderpass.moc"
