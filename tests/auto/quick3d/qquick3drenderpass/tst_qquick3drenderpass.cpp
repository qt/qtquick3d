// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QtCore/qfile.h>
#include <QtCore/qregularexpression.h>
#include <QtCore/qscopeguard.h>
#include <QtCore/qtemporarydir.h>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlComponent>
#include <QtQml/qqmlextensionplugin.h>
#include <QtQuick3D/private/qquick3dcontentlayer_p.h>
#include <QtQuick3D/private/qquick3drenderpass_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderuserpass_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercache_p.h>
#include <QtQuick3D/private/qquick3dshaderimage_p.h>
#include <QtQuick3D/private/qquick3dobject_p.h>
#include <QtQuick3D/private/qquick3dscenemanager_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderimage_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderbuffer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgshadermaterialadapter_p.h>

#ifdef QT_STATIC
// Static builds need the QML plugins explicitly imported, since the test loads
// "import QtQuick" and "import QtQuick3D" via QQmlComponent::setData() at
// runtime and the import scanner cannot discover them from the C++ sources.
Q_IMPORT_QML_PLUGIN(QtQuick2Plugin)
Q_IMPORT_QML_PLUGIN(QQuick3DPlugin)
#endif

namespace {

// Writes an augment shader next to the test's temporary directory and returns
// the URL to hand to RenderPass::augmentShader.
QUrl writeShader(const QTemporaryDir &dir, const QString &name, const QByteArray &source)
{
    const QString path = dir.filePath(name);
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly))
        return {};
    f.write(source);
    f.close();
    return QUrl::fromLocalFile(path);
}

const QSSGBaseTypeProperty *findUniform(const QSSGUserShaderAugmentation &augmentation, const QByteArray &name)
{
    for (const auto &prop : augmentation.propertyUniforms) {
        if (prop.name == name)
            return &prop;
    }
    return nullptr;
}

// An augment shader declaring one storage image, plus the RenderPass subtype
// exposing it. The declaration's format and the property's ShaderImage.format
// have to agree, otherwise processMetadata() rejects the pair.
QByteArray imageAugmentShader()
{
    return QByteArrayLiteral(R"(
SHADER_IMAGE("rgba8", myImage, image2D, "readonly");

void MAIN_FRAGMENT_AUGMENT()
{
    BASE_COLOR = imageLoad(myImage, ivec2(0));
}
)");
}

QByteArray imageAugmentQml()
{
    return QByteArrayLiteral(R"(
import QtQuick3D
RenderPass {
    materialMode: RenderPass.AugmentMaterial
    property real strength: 1.0
    property ShaderImage myImage: ShaderImage {
        width: 8; height: 8; format: ShaderImage.RGBA8
    }
})");
}

// A storage buffer and an image property, with the metadata the frontend
// resolves for them, as handed to processMetadata().
struct MetadataInput
{
    QList<QSSGBaseTypeProperty> props;
    QByteArrayList metadata;
};

MetadataInput metadataInput()
{
    MetadataInput input;
    input.props.append(QSSGBaseTypeProperty("values", "QQuick3DShaderStorageBuffer*", QVariant::fromValue(QSSGShaderBuffer()),
                                            QSSGRenderShaderValue::StorageBuffer));
    input.props.append(QSSGBaseTypeProperty("img", "QQuick3DShaderImage*",
                                            QVariant::fromValue(QSSGShaderImage(nullptr, {}, "rgba8", {})),
                                            QSSGRenderShaderValue::Image));
    input.metadata << "buffer" << "" << "values" << ""
                   << "image" << "rgba8" << "img" << "image2D";
    return input;
}

} // namespace

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
    void testAugmentShaderSplit();
    void testAugmentShaderMalformed_data();
    void testAugmentShaderMalformed();
    void testAugmentShaderImageDeclaration();
    void testAugmentResourcesSurviveValueUpdate();
    void testResourcePropertyResolvesOnLaterUpdate();
    void testNullImagePropertyAssignedLater();
    void testProcessMetadataIgnoresComments();
    void testProcessMetadataBlockCommentedDeclaration();
    void testProcessMetadataBufferBody_data();
    void testProcessMetadataBufferBody();
    void testProcessMetadataImageType_data();
    void testProcessMetadataImageType();
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

// The augment shader is split in two: the contents of MAIN_FRAGMENT_AUGMENT()
// become the snippet injected into the generated main(), everything else
// becomes the preamble emitted at file scope before it.
void tst_QQuick3DRenderPass::testAugmentShaderSplit()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QByteArray source = QByteArrayLiteral(R"(
float helper(float x) { return x * 2.0; }

void MAIN_FRAGMENT_AUGMENT()
{
    BASE_COLOR.rgb = vec3(helper(0.5));
    if (BASE_COLOR.a > 0.0) { BASE_COLOR.a = 1.0; }
}

float trailing() { return 1.0; }
)");
    const QUrl shaderUrl = writeShader(dir, QStringLiteral("split.frag"), source);
    QVERIFY(!shaderUrl.isEmpty());

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import QtQuick3D; RenderPass { materialMode: RenderPass.AugmentMaterial }", QUrl());
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));
    QScopedPointer<QObject> object(component.create());
    auto *pass = qobject_cast<QQuick3DRenderPass *>(object.data());
    QVERIFY(pass != nullptr);
    QCOMPARE(pass->materialMode(), QQuick3DRenderPass::AugmentMaterial);

    pass->setAugmentShader(shaderUrl);
    QCOMPARE(pass->augmentShader(), shaderUrl);

    QScopedPointer<QSSGRenderGraphObject> node(QQuick3DObjectPrivate::updateSpatialNode(pass, nullptr));
    QVERIFY(node != nullptr);
    auto *userPass = static_cast<QSSGRenderUserPass *>(node.data());
    const QSSGUserShaderAugmentation &augmentation = userPass->shaderAugmentation;
    QVERIFY(augmentation.hasUserAugmentation());

    // The body carries the function's statements, nested braces included, and
    // neither the function signature nor anything outside it.
    QVERIFY(augmentation.body.contains("BASE_COLOR.rgb = vec3(helper(0.5));"));
    QVERIFY(augmentation.body.contains("BASE_COLOR.a = 1.0;"));
    QVERIFY(!augmentation.body.contains("MAIN_FRAGMENT_AUGMENT"));
    QVERIFY(!augmentation.body.contains("float helper"));
    QVERIFY(!augmentation.body.contains("float trailing"));

    // The preamble keeps the code from both before and after the function.
    QVERIFY(augmentation.preamble.contains("float helper(float x)"));
    QVERIFY(augmentation.preamble.contains("float trailing()"));
    QVERIFY(!augmentation.preamble.contains("MAIN_FRAGMENT_AUGMENT"));

    // The macro substitution (BASE_COLOR -> qt_diffuseColor) and the feature
    // detection derived from it happen in finalizeShaders() on the render
    // thread, which needs a render context, so they are not covered here.
}

void tst_QQuick3DRenderPass::testAugmentShaderMalformed_data()
{
    QTest::addColumn<QByteArray>("source");
    QTest::addColumn<QString>("warning");

    QTest::newRow("no_function")
            << QByteArrayLiteral("float helper() { return 1.0; }\n")
            << QStringLiteral("Could not find MAIN_FRAGMENT_AUGMENT() function");
    QTest::newRow("no_opening_brace")
            << QByteArrayLiteral("void MAIN_FRAGMENT_AUGMENT()\n")
            << QStringLiteral("Could not find the opening brace");
    QTest::newRow("unbalanced_braces")
            << QByteArrayLiteral("void MAIN_FRAGMENT_AUGMENT()\n{\n    if (true) {\n")
            << QStringLiteral("Could not find the closing brace");
}

// A shader that cannot be split has to be diagnosed and leave the augmentation
// empty, rather than passing half-parsed text on to the shader generator.
void tst_QQuick3DRenderPass::testAugmentShaderMalformed()
{
    QFETCH(QByteArray, source);
    QFETCH(QString, warning);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QUrl shaderUrl = writeShader(dir, QStringLiteral("malformed.frag"), source);
    QVERIFY(!shaderUrl.isEmpty());

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import QtQuick3D; RenderPass { materialMode: RenderPass.AugmentMaterial }", QUrl());
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));
    QScopedPointer<QObject> object(component.create());
    auto *pass = qobject_cast<QQuick3DRenderPass *>(object.data());
    QVERIFY(pass != nullptr);
    pass->setAugmentShader(shaderUrl);

    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QRegularExpression::escape(warning)));
    QScopedPointer<QSSGRenderGraphObject> node(QQuick3DObjectPrivate::updateSpatialNode(pass, nullptr));
    QVERIFY(node != nullptr);
    auto *userPass = static_cast<QSSGRenderUserPass *>(node.data());
    QVERIFY(!userPass->shaderAugmentation.hasUserAugmentation());
    QVERIFY(userPass->shaderAugmentation.body.isEmpty());
}

// A SHADER_IMAGE declaration in the augment shader is paired with the matching
// ShaderImage property: the declaration is replaced by a marker for the
// shader generator, and the GLSL type, format and memory qualifiers it names
// are resolved onto the property.
void tst_QQuick3DRenderPass::testAugmentShaderImageDeclaration()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QUrl shaderUrl = writeShader(dir, QStringLiteral("image.frag"), imageAugmentShader());
    QVERIFY(!shaderUrl.isEmpty());

    // Extracting a resource property registers a watcher, which references the
    // watched object into the owner's scene manager, so the manager has to
    // exist and outlive the objects. It deliberately has no window attached,
    // which means it must never be driven through its own sync entry points
    // here: a backend node reaching QQuick3DSceneManager::cleanupNodes() is
    // handed to the (null) window attachment unconditionally. The syncs below
    // are therefore driven directly, and the nodes stay owned by the test.
    QQuick3DSceneManager manager;

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(imageAugmentQml(), QUrl());
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));
    QScopedPointer<QObject> object(component.create());
    auto *pass = qobject_cast<QQuick3DRenderPass *>(object.data());
    QVERIFY(pass != nullptr);
    QQuick3DObjectPrivate::refSceneManager(pass, manager);
    pass->setAugmentShader(shaderUrl);

    QScopedPointer<QSSGRenderGraphObject> node(QQuick3DObjectPrivate::updateSpatialNode(pass, nullptr));
    QVERIFY(node != nullptr);
    auto *userPass = static_cast<QSSGRenderUserPass *>(node.data());
    const QSSGUserShaderAugmentation &augmentation = userPass->shaderAugmentation;

    // The declaration is rewritten in place as a self-delimiting marker, which
    // the shader generator later expands into the real layout(...) statement.
    QVERIFY(!augmentation.preamble.contains("SHADER_IMAGE("));
    QVERIFY(augmentation.preamble.contains("@myImage@"));
    QVERIFY(augmentation.body.contains("imageLoad(myImage, ivec2(0))"));

    const QSSGBaseTypeProperty *prop = findUniform(augmentation, QByteArrayLiteral("myImage"));
    QVERIFY(prop != nullptr);
    QCOMPARE(prop->shaderDataType, QSSGRenderShaderValue::Image);

    const QSSGShaderImage image = prop->value.value<QSSGShaderImage>();
    QCOMPARE(image.imageType, QByteArray("image2D"));
    QCOMPARE(image.format, QByteArray("rgba8"));
    QCOMPARE(image.qualifiers, QSSGMemoryQualifier::Readonly);

    // The scalar property is exposed as an ordinary uniform alongside it.
    const QSSGBaseTypeProperty *scalar = findUniform(augmentation, QByteArrayLiteral("strength"));
    QVERIFY(scalar != nullptr);
    QCOMPARE(scalar->shaderDataType, QSSGRenderShaderValue::Float);
}

// The image and buffer declarations are only resolved while the augment shader
// is being processed. A later update triggered by an ordinary property value
// change must not rebuild the resource properties from scratch, or the type
// and qualifiers resolved from the shader source are lost and the generated
// declaration becomes uncompilable.
void tst_QQuick3DRenderPass::testAugmentResourcesSurviveValueUpdate()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QUrl shaderUrl = writeShader(dir, QStringLiteral("survive.frag"), imageAugmentShader());
    QVERIFY(!shaderUrl.isEmpty());

    QQuick3DSceneManager manager;

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(imageAugmentQml(), QUrl());
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));
    QScopedPointer<QObject> object(component.create());
    auto *pass = qobject_cast<QQuick3DRenderPass *>(object.data());
    QVERIFY(pass != nullptr);
    QQuick3DObjectPrivate::refSceneManager(pass, manager);
    pass->setAugmentShader(shaderUrl);

    QScopedPointer<QSSGRenderGraphObject> node(QQuick3DObjectPrivate::updateSpatialNode(pass, nullptr));
    QVERIFY(node != nullptr);
    auto *userPass = static_cast<QSSGRenderUserPass *>(node.data());

    const QSSGBaseTypeProperty *prop = findUniform(userPass->shaderAugmentation, QByteArrayLiteral("myImage"));
    QVERIFY(prop != nullptr);
    QCOMPARE(prop->value.value<QSSGShaderImage>().imageType, QByteArray("image2D"));

    // Change an unrelated scalar property. Its notify signal marks the pass
    // dirty for a value-only update, with the augment shader unchanged.
    pass->setProperty("strength", 2.0);

    QSSGRenderGraphObject *sameNode = QQuick3DObjectPrivate::updateSpatialNode(pass, node.data());
    QCOMPARE(sameNode, node.data());

    const QSSGBaseTypeProperty *scalar = findUniform(userPass->shaderAugmentation, QByteArrayLiteral("strength"));
    QVERIFY(scalar != nullptr);
    QCOMPARE(scalar->value.toFloat(), 2.0f);

    prop = findUniform(userPass->shaderAugmentation, QByteArrayLiteral("myImage"));
    QVERIFY(prop != nullptr);
    const QSSGShaderImage image = prop->value.value<QSSGShaderImage>();
    QCOMPARE(image.imageType, QByteArray("image2D"));
    QCOMPARE(image.format, QByteArray("rgba8"));
    QCOMPARE(image.qualifiers, QSSGMemoryQualifier::Readonly);
}

// A resource property whose backend node does not exist yet cannot be resolved
// on the first sync. The pass has to keep the pending property update alive and
// re-resolve it once the node appears; if that request is dropped, the image
// stays unbound for the rest of the object's life and the shader reads nothing.
void tst_QQuick3DRenderPass::testResourcePropertyResolvesOnLaterUpdate()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QUrl shaderUrl = writeShader(dir, QStringLiteral("retry.frag"), imageAugmentShader());
    QVERIFY(!shaderUrl.isEmpty());

    QQuick3DSceneManager manager;

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(imageAugmentQml(), QUrl());
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));
    QScopedPointer<QObject> object(component.create());
    auto *pass = qobject_cast<QQuick3DRenderPass *>(object.data());
    QVERIFY(pass != nullptr);
    QQuick3DObjectPrivate::refSceneManager(pass, manager);
    pass->setAugmentShader(shaderUrl);

    QQuick3DShaderImage *shaderImage = pass->property("myImage").value<QQuick3DShaderImage *>();
    QVERIFY(shaderImage != nullptr);
    auto *imagePriv = QQuick3DObjectPrivate::get(shaderImage);
    QVERIFY(imagePriv->spatialNode == nullptr);

    // First sync: the image has no backend node yet, so the property cannot be
    // resolved and the pass has to ask to be updated again.
    QScopedPointer<QSSGRenderGraphObject> node(QQuick3DObjectPrivate::updateSpatialNode(pass, nullptr));
    QVERIFY(node != nullptr);
    auto *userPass = static_cast<QSSGRenderUserPass *>(node.data());

    const QSSGBaseTypeProperty *prop = findUniform(userPass->shaderAugmentation, QByteArrayLiteral("myImage"));
    QVERIFY(prop != nullptr);
    QVERIFY(prop->value.value<QSSGShaderImage>().image == nullptr);
    QVERIFY2(QQuick3DObjectPrivate::get(pass)->secondaryUpdateRequested,
             "the pass did not request another update while the image was unresolved");

    // The node appears, as it does once the image itself is synced.
    imagePriv->spatialNode = QQuick3DObjectPrivate::updateSpatialNode(shaderImage, nullptr);
    QVERIFY(imagePriv->spatialNode != nullptr);

    // Second sync: the pending property update has to still be live, so that
    // the property picks the node up even though the augment shader is not
    // reprocessed this time.
    QSSGRenderGraphObject *sameNode = QQuick3DObjectPrivate::updateSpatialNode(pass, node.data());
    QCOMPARE(sameNode, node.data());

    prop = findUniform(userPass->shaderAugmentation, QByteArrayLiteral("myImage"));
    QVERIFY(prop != nullptr);
    const QSSGShaderImage resolved = prop->value.value<QSSGShaderImage>();
    QVERIFY2(resolved.image != nullptr, "the image property never resolved to a backend node");
    QCOMPARE(resolved.image, static_cast<QSSGRenderImage *>(imagePriv->spatialNode));
    QCOMPARE(resolved.image->type, QSSGRenderGraphObject::Type::Image2D);
    // Re-resolving must refresh the node without dropping what the shader
    // source contributed.
    QCOMPARE(resolved.imageType, QByteArray("image2D"));
    QCOMPARE(resolved.qualifiers, QSSGMemoryQualifier::Readonly);

    // This scene manager has no window attachment, so it cannot take over
    // ownership of backend nodes: QQuick3DSceneManager::cleanupNodes() hands
    // any node that has graphics resources to the attachment unconditionally.
    // Release the node and detach it before the frontend object goes away.
    delete imagePriv->spatialNode;
    imagePriv->spatialNode = nullptr;
}

// A ShaderImage property that is null at the first sync has no image to
// watch. Assigning one later still has to be noticed, and since the property
// gains an image, the augment shader has to be processed again so that the
// declaration is generated.
void tst_QQuick3DRenderPass::testNullImagePropertyAssignedLater()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QUrl shaderUrl = writeShader(dir, QStringLiteral("nullimage.frag"), imageAugmentShader());
    QVERIFY(!shaderUrl.isEmpty());

    QQuick3DSceneManager manager;
    QQmlEngine engine;

    QQmlComponent imageComponent(&engine);
    imageComponent.setData("import QtQuick3D; ShaderImage { width: 8; height: 8; format: ShaderImage.RGBA8 }", QUrl());
    QVERIFY2(!imageComponent.isError(), qPrintable(imageComponent.errorString()));
    // Declared before the pass, so that the pass releases its reference first.
    QScopedPointer<QQuick3DShaderImage> image(qobject_cast<QQuick3DShaderImage *>(imageComponent.create()));
    QVERIFY(image != nullptr);
    auto *imagePriv = QQuick3DObjectPrivate::get(image.data());

    QQmlComponent component(&engine);
    component.setData(R"(
import QtQuick3D
RenderPass {
    materialMode: RenderPass.AugmentMaterial
    property ShaderImage myImage: null
})",
                      QUrl());
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));
    QScopedPointer<QObject> object(component.create());
    auto *pass = qobject_cast<QQuick3DRenderPass *>(object.data());
    QVERIFY(pass != nullptr);
    QQuick3DObjectPrivate::refSceneManager(pass, manager);
    pass->setAugmentShader(shaderUrl);

    // Without an image, the declaration in the shader has nothing to pair with.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("Unable to find matching image or property for")));
    QTest::ignoreMessage(QtWarningMsg, "RenderPass: Shader Image and/or Buffer processing failed.");
    QScopedPointer<QSSGRenderGraphObject> node(QQuick3DObjectPrivate::updateSpatialNode(pass, nullptr));
    QVERIFY(node != nullptr);
    auto *userPass = static_cast<QSSGRenderUserPass *>(node.data());
    QVERIFY(!userPass->shaderAugmentation.preamble.contains("@myImage@"));

    // See testResourcePropertyResolvesOnLaterUpdate(). Declared after the pass,
    // so that the node is released before the pass releases the image.
    const auto releaseNode = qScopeGuard([imagePriv] {
        delete imagePriv->spatialNode;
        imagePriv->spatialNode = nullptr;
    });
    imagePriv->spatialNode = QQuick3DObjectPrivate::updateSpatialNode(image.data(), nullptr);
    QVERIFY(imagePriv->spatialNode != nullptr);
    pass->setProperty("myImage", QVariant::fromValue(image.data()));

    QSSGRenderGraphObject *sameNode = QQuick3DObjectPrivate::updateSpatialNode(pass, node.data());
    QCOMPARE(sameNode, node.data());
    QVERIFY2(userPass->shaderAugmentation.preamble.contains("@myImage@"),
             "the augment shader was not processed again after the image was assigned");

    const QSSGBaseTypeProperty *prop = findUniform(userPass->shaderAugmentation, QByteArrayLiteral("myImage"));
    QVERIFY(prop != nullptr);
    const QSSGShaderImage resolved = prop->value.value<QSSGShaderImage>();
    QCOMPARE(resolved.image, static_cast<QSSGRenderImage *>(imagePriv->spatialNode));
    QCOMPARE(resolved.imageType, QByteArray("image2D"));
    QCOMPARE(resolved.format, QByteArray("rgba8"));

    // The image is referenced into the pass's scene.
    QCOMPARE(imagePriv->sceneManager.data(), &manager);
}

// Declarations inside comments are not declarations. A commented-out
// declaration followed by the real one must not be reported as a duplicate,
// and a declaration inside a block comment must not be processed.
void tst_QQuick3DRenderPass::testProcessMetadataIgnoresComments()
{
    MetadataInput input = metadataInput();
    QByteArrayList stageMetadata;
    QByteArray shader(R"(// STORAGE_BUFFER(values, "readonly") { float old[]; };
// SHADER_IMAGE("rgba8", img, image2D, "writeonly");
/*
SHADER_IMAGE("rgba8", img, image3D, "writeonly");
*/
STORAGE_BUFFER(values, "readonly") { vec4 data[]; };
SHADER_IMAGE("rgba8", img, image2D, "readonly");
void main() { }
)");

    QVERIFY(processMetadata(input.props, input.metadata, stageMetadata, shader));

    // The comments are left as they are, and only the real declarations are
    // replaced by markers.
    QVERIFY(shader.contains(R"(// STORAGE_BUFFER(values, "readonly") { float old[]; };)"));
    QVERIFY(shader.contains(R"(// SHADER_IMAGE("rgba8", img, image2D, "writeonly");)"));
    QVERIFY(shader.contains(R"(SHADER_IMAGE("rgba8", img, image3D, "writeonly");)"));
    QCOMPARE(shader.count("@values@"), 1);
    QCOMPARE(shader.count("@img@"), 1);

    const QSSGShaderBuffer buffer = input.props[0].value.value<QSSGShaderBuffer>();
    QCOMPARE(buffer.dataType, QByteArray("vec4 data[];"));
    QCOMPARE(buffer.qualifiers, QSSGMemoryQualifier::Readonly);
    const QSSGShaderImage image = input.props[1].value.value<QSSGShaderImage>();
    QCOMPARE(image.imageType, QByteArray("image2D"));
    QCOMPARE(image.qualifiers, QSSGMemoryQualifier::Readonly);

    const QByteArrayList expected { "buffer", "vec4 data[];", "values", "readonly",
                                    "image2D", "rgba8", "img", "readonly" };
    QCOMPARE(stageMetadata, expected);
}

void tst_QQuick3DRenderPass::testProcessMetadataBlockCommentedDeclaration()
{
    MetadataInput input = metadataInput();
    QByteArrayList stageMetadata;
    const QByteArray commented(R"(/* SHADER_IMAGE("rgba8", img, image2D, "readonly"); */)");
    QByteArray shader = commented + R"(
STORAGE_BUFFER(values, "") { vec4 data[]; };
void main() { }
)";

    QVERIFY(processMetadata(input.props, input.metadata, stageMetadata, shader));

    QVERIFY(shader.contains(commented));
    QVERIFY(!shader.contains("@img@"));
    QCOMPARE(input.props[1].value.value<QSSGShaderImage>().imageType, QByteArray());
    // Only the buffer is declared.
    QCOMPARE(stageMetadata.size(), 4);
    QCOMPARE(stageMetadata.first(), QByteArray("buffer"));
}

// The buffer's block ends at the brace that closes it in the code, not at a
// brace inside a comment, and the body is taken from the source as written.
// Like any other interface block, the declaration may end with a ';', which
// has to go together with the declaration, or it is left behind as a stray
// semicolon at global scope.
void tst_QQuick3DRenderPass::testProcessMetadataBufferBody_data()
{
    QTest::addColumn<QByteArray>("source");
    QTest::addColumn<QByteArray>("dataType");
    QTest::addColumn<QByteArray>("result");

    QTest::newRow("semicolon")
        << QByteArray(R"(STORAGE_BUFFER(values, "") { vec4 data[]; };
float x;)")
        << QByteArray("vec4 data[];")
        << QByteArray(R"(@values@
float x;)");
    QTest::newRow("no semicolon")
        << QByteArray(R"(STORAGE_BUFFER(values, "") { vec4 data[]; }
float x;)")
        << QByteArray("vec4 data[];")
        << QByteArray(R"(@values@
float x;)");
    QTest::newRow("semicolon on the next line")
        << QByteArray(R"(STORAGE_BUFFER(values, "") { vec4 data[]; }
    ;
float x;)")
        << QByteArray("vec4 data[];")
        << QByteArray(R"(@values@
float x;)");
    QTest::newRow("brace in block comment")
        << QByteArray(R"(STORAGE_BUFFER(values, "") { vec4 data[]; /* } */ };
float x;)")
        << QByteArray("vec4 data[]; /* } */")
        << QByteArray(R"(@values@
float x;)");
    QTest::newRow("brace in line comment")
        << QByteArray(R"(STORAGE_BUFFER(values, "") {
    vec4 data[]; // }
};
float x;)")
        << QByteArray("vec4 data[]; // }")
        << QByteArray(R"(@values@
float x;)");
}

void tst_QQuick3DRenderPass::testProcessMetadataBufferBody()
{
    QFETCH(QByteArray, source);
    QFETCH(QByteArray, dataType);
    QFETCH(QByteArray, result);

    MetadataInput input = metadataInput();
    QByteArrayList stageMetadata;
    QByteArray shader = source;

    QVERIFY(processMetadata(input.props, input.metadata, stageMetadata, shader));
    QCOMPARE(shader, result);
    QCOMPARE(input.props[0].value.value<QSSGShaderBuffer>().dataType, dataType);
}

// The image type in a SHADER_IMAGE declaration has to be the one the
// image's shape and format call for, like the format has to be its format.
void tst_QQuick3DRenderPass::testProcessMetadataImageType_data()
{
    QTest::addColumn<QByteArray>("declaration");
    QTest::addColumn<bool>("accepted");

    QTest::newRow("matching") << QByteArray(R"(SHADER_IMAGE("rgba8", img, image2D, "readonly");)") << true;
    QTest::newRow("other shape") << QByteArray(R"(SHADER_IMAGE("rgba8", img, image3D, "readonly");)") << false;
    QTest::newRow("integer type") << QByteArray(R"(SHADER_IMAGE("rgba8", img, uimage2D, "readonly");)") << false;
    QTest::newRow("other format") << QByteArray(R"(SHADER_IMAGE("r32f", img, image2D, "readonly");)") << false;
}

void tst_QQuick3DRenderPass::testProcessMetadataImageType()
{
    QFETCH(QByteArray, declaration);
    QFETCH(bool, accepted);

    MetadataInput input = metadataInput();
    QByteArrayList stageMetadata;
    QByteArray shader = R"(STORAGE_BUFFER(values, "readonly") { vec4 data[]; };
)" + declaration + R"(
void main() { }
)";

    if (!accepted)
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("doesn't match")));
    QCOMPARE(processMetadata(input.props, input.metadata, stageMetadata, shader), accepted);
}

QTEST_MAIN(tst_QQuick3DRenderPass)
#include "tst_qquick3drenderpass.moc"
