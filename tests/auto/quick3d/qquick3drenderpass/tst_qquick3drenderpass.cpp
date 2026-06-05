// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlComponent>
#include <QtQml/qqmlextensionplugin.h>
#include <QtQuick3D/private/qquick3dcontentlayer_p.h>
#include <QtQuick3D/private/qquick3drenderpass_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderuserpass_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercache_p.h>

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
    void testReplaceAugmentMacros_data();
    void testReplaceAugmentMacros();
    void testReplaceAugmentMacrosAccumulatesFlags();
    void testSplitAugmentShader_data();
    void testSplitAugmentShader();
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

using FeatureFlags = QSSGUserShaderAugmentation::FeatureFlags;
using FeatureFlagsType = QSSGUserShaderAugmentation::FeatureFlagsType;

// Combine a set of FeatureFlags into the raw underlying mask used by the parser.
static FeatureFlagsType flags(std::initializer_list<FeatureFlags> list)
{
    FeatureFlagsType mask = 0;
    for (FeatureFlags f : list)
        mask |= static_cast<FeatureFlagsType>(f);
    return mask;
}

void tst_QQuick3DRenderPass::testReplaceAugmentMacros_data()
{
    QTest::addColumn<QByteArray>("input");
    QTest::addColumn<QByteArray>("expectedCode");
    QTest::addColumn<FeatureFlagsType>("expectedFlags");

    // Every macro maps to its built-in variable name and sets its feature flag.
    QTest::newRow("base_color") << QByteArray("BASE_COLOR") << QByteArray("qt_diffuseColor") << flags({FeatureFlags::BaseColor});
    QTest::newRow("roughness") << QByteArray("ROUGHNESS") << QByteArray("qt_roughnessAmount") << flags({FeatureFlags::Roughness});
    QTest::newRow("metalness") << QByteArray("METALNESS") << QByteArray("qt_metalnessAmount") << flags({FeatureFlags::Metalness});
    QTest::newRow("diffuse_light") << QByteArray("DIFFUSE_LIGHT") << QByteArray("global_diffuse_light.rgb") << flags({FeatureFlags::DiffuseLight});
    QTest::newRow("specular_light") << QByteArray("SPECULAR_LIGHT") << QByteArray("global_specular_light") << flags({FeatureFlags::SpecularLight});
    QTest::newRow("emissive_light") << QByteArray("EMISSIVE_LIGHT") << QByteArray("global_emission") << flags({FeatureFlags::EmissiveLight});
    QTest::newRow("world_normal") << QByteArray("WORLD_NORMAL") << QByteArray("qt_world_normal") << flags({FeatureFlags::WorldNormal});
    QTest::newRow("world_tangent") << QByteArray("WORLD_TANGENT") << QByteArray("qt_tangent") << flags({FeatureFlags::WorldTangent});
    QTest::newRow("world_binormal") << QByteArray("WORLD_BINORMAL") << QByteArray("qt_binormal") << flags({FeatureFlags::WorldBinormal});
    QTest::newRow("f0") << QByteArray("F0") << QByteArray("qt_f0") << flags({FeatureFlags::F0});
    QTest::newRow("f90") << QByteArray("F90") << QByteArray("qt_f90") << flags({FeatureFlags::F90});

    // Macros surrounded by non-identifier characters are still replaced.
    QTest::newRow("punct_before") << QByteArray("(BASE_COLOR)") << QByteArray("(qt_diffuseColor)") << flags({FeatureFlags::BaseColor});
    QTest::newRow("punct_after") << QByteArray("BASE_COLOR;") << QByteArray("qt_diffuseColor;") << flags({FeatureFlags::BaseColor});
    QTest::newRow("member_access") << QByteArray("WORLD_NORMAL.x") << QByteArray("qt_world_normal.x") << flags({FeatureFlags::WorldNormal});

    // Identifier-boundary checks: a macro that is only a substring of a larger identifier
    // must be left untouched (this is the leading-boundary regression case).
    QTest::newRow("leading_underscore") << QByteArray("MY_BASE_COLOR") << QByteArray("MY_BASE_COLOR") << flags({});
    QTest::newRow("leading_letter") << QByteArray("aBASE_COLOR") << QByteArray("aBASE_COLOR") << flags({});
    QTest::newRow("trailing_underscore") << QByteArray("ROUGHNESS_VALUE") << QByteArray("ROUGHNESS_VALUE") << flags({});
    QTest::newRow("embedded_both_sides") << QByteArray("MY_ROUGHNESS_VALUE") << QByteArray("MY_ROUGHNESS_VALUE") << flags({});
    QTest::newRow("f0_trailing_letter") << QByteArray("F0X") << QByteArray("F0X") << flags({});

    // Disambiguation between macros sharing a prefix.
    QTest::newRow("world_prefix_not_normal") << QByteArray("WORLD_NORMALIZED") << QByteArray("WORLD_NORMALIZED") << flags({});
    QTest::newRow("f90_not_f0") << QByteArray("F90") << QByteArray("qt_f90") << flags({FeatureFlags::F90});

    // Macros inside comments must be ignored.
    QTest::newRow("line_comment") << QByteArray("// BASE_COLOR\nROUGHNESS")
                                  << QByteArray("// BASE_COLOR\nqt_roughnessAmount") << flags({FeatureFlags::Roughness});
    QTest::newRow("block_comment") << QByteArray("/* BASE_COLOR */ METALNESS")
                                   << QByteArray("/* BASE_COLOR */ qt_metalnessAmount") << flags({FeatureFlags::Metalness});

    // Multiple macros: all replaced, all flags set, with differing macro/replacement lengths.
    QTest::newRow("two_macros") << QByteArray("BASE_COLOR + ROUGHNESS")
                                << QByteArray("qt_diffuseColor + qt_roughnessAmount")
                                << flags({FeatureFlags::BaseColor, FeatureFlags::Roughness});
    QTest::newRow("short_then_long") << QByteArray("F0 BASE_COLOR")
                                     << QByteArray("qt_f0 qt_diffuseColor")
                                     << flags({FeatureFlags::F0, FeatureFlags::BaseColor});

    // No macros at all leaves the code and flags untouched.
    QTest::newRow("no_macros") << QByteArray("vec3 c = vec3(1.0);") << QByteArray("vec3 c = vec3(1.0);") << flags({});
}

void tst_QQuick3DRenderPass::testReplaceAugmentMacros()
{
    QFETCH(QByteArray, input);
    QFETCH(QByteArray, expectedCode);
    QFETCH(FeatureFlagsType, expectedFlags);

    QSSGUserShaderAugmentation augmentation;
    QSSGRenderUserPass::replaceAugmentMacros(input, augmentation);

    QCOMPARE(input, expectedCode);
    QCOMPARE(augmentation.featureFlags, expectedFlags);
}

void tst_QQuick3DRenderPass::testReplaceAugmentMacrosAccumulatesFlags()
{
    // finalizeShaders() runs the preamble and body through the same augmentation,
    // so flags set by an earlier call must be preserved on the next.
    QSSGUserShaderAugmentation augmentation;

    QByteArray preamble("BASE_COLOR");
    QSSGRenderUserPass::replaceAugmentMacros(preamble, augmentation);
    QCOMPARE(preamble, QByteArray("qt_diffuseColor"));
    QCOMPARE(augmentation.featureFlags, flags({FeatureFlags::BaseColor}));

    QByteArray body("ROUGHNESS");
    QSSGRenderUserPass::replaceAugmentMacros(body, augmentation);
    QCOMPARE(body, QByteArray("qt_roughnessAmount"));
    QCOMPARE(augmentation.featureFlags, flags({FeatureFlags::BaseColor, FeatureFlags::Roughness}));
}

void tst_QQuick3DRenderPass::testSplitAugmentShader_data()
{
    using Result = QQuick3DRenderPass::AugmentSplitResult;

    QTest::addColumn<QByteArray>("input");
    QTest::addColumn<Result>("expectedResult");
    QTest::addColumn<QByteArray>("expectedPreamble");
    QTest::addColumn<QByteArray>("expectedBody");

    // Basic: the whole shader is the augment function, so there is no preamble.
    QTest::newRow("basic")
        << QByteArray("void MAIN_FRAGMENT_AUGMENT() { FRAGCOLOR = vec4(1.0); }")
        << Result::Ok << QByteArray() << QByteArray(" FRAGCOLOR = vec4(1.0); ");

    // Whitespace between the tokens of the signature must not matter (the old exact-string
    // search required single spaces).
    QTest::newRow("awkward_spacing")
        << QByteArray("void   MAIN_FRAGMENT_AUGMENT  (  )  {body;}")
        << Result::Ok << QByteArray() << QByteArray("body;");

    // Helper code before the function becomes preamble.
    QTest::newRow("preamble_before")
        << QByteArray("float helper() { return 2.0; }\nvoid MAIN_FRAGMENT_AUGMENT() { BASE_COLOR = vec4(0.0); }")
        << Result::Ok << QByteArray("float helper() { return 2.0; }\n") << QByteArray(" BASE_COLOR = vec4(0.0); ");

    // Code after the function is also preamble.
    QTest::newRow("code_after")
        << QByteArray("void MAIN_FRAGMENT_AUGMENT() { a = 1.0; }\nfloat post() { return 0.0; }")
        << Result::Ok << QByteArray("\nfloat post() { return 0.0; }") << QByteArray(" a = 1.0; ");

    // Nested braces are balanced correctly.
    QTest::newRow("nested_braces")
        << QByteArray("void MAIN_FRAGMENT_AUGMENT() { if (x) { y; } }")
        << Result::Ok << QByteArray() << QByteArray(" if (x) { y; } ");

    // A closing brace inside a comment must not be mistaken for the end of the body.
    QTest::newRow("brace_in_comment")
        << QByteArray("void MAIN_FRAGMENT_AUGMENT() { a; /* } */ b; }")
        << Result::Ok << QByteArray() << QByteArray(" a; /* } */ b; ");

    // The keyword appearing in a comment is ignored; the real function is still found.
    QTest::newRow("keyword_in_comment")
        << QByteArray("// void MAIN_FRAGMENT_AUGMENT() {}\nvoid MAIN_FRAGMENT_AUGMENT() { real; }")
        << Result::Ok << QByteArray("// void MAIN_FRAGMENT_AUGMENT() {}\n") << QByteArray(" real; ");

    // A call to the function (not preceded by "void") must not be treated as the definition.
    QTest::newRow("call_site_not_definition")
        << QByteArray("void doStuff() { MAIN_FRAGMENT_AUGMENT(); }")
        << Result::FunctionNotFound << QByteArray() << QByteArray();

    QTest::newRow("function_missing")
        << QByteArray("void other() { FRAGCOLOR = vec4(1.0); }")
        << Result::FunctionNotFound << QByteArray() << QByteArray();

    // A forward declaration has no body.
    QTest::newRow("declaration_only")
        << QByteArray("void MAIN_FRAGMENT_AUGMENT();")
        << Result::OpeningBraceNotFound << QByteArray() << QByteArray();

    QTest::newRow("unbalanced_braces")
        << QByteArray("void MAIN_FRAGMENT_AUGMENT() { a;")
        << Result::ClosingBraceNotFound << QByteArray() << QByteArray();
}

void tst_QQuick3DRenderPass::testSplitAugmentShader()
{
    using Result = QQuick3DRenderPass::AugmentSplitResult;

    QFETCH(QByteArray, input);
    QFETCH(Result, expectedResult);
    QFETCH(QByteArray, expectedPreamble);
    QFETCH(QByteArray, expectedBody);

    QByteArray preamble;
    QByteArray body;
    const auto result = QQuick3DRenderPass::splitAugmentShader(input, preamble, body);

    QCOMPARE(result, expectedResult);
    QCOMPARE(preamble, expectedPreamble);
    QCOMPARE(body, expectedBody);
}

QTEST_MAIN(tst_QQuick3DRenderPass)
#include "tst_qquick3drenderpass.moc"
