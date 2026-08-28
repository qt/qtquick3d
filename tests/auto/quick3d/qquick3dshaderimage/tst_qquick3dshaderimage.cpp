// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QSignalSpy>
#include <QtCore/qmetaobject.h>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlComponent>
#include <QtQml/qqmlextensionplugin.h>
#include <QtQuick3D/private/qquick3dshaderimage_p.h>
#include <QtQuick3D/private/qquick3dobject_p.h>
#include <QtQuick3D/private/qquick3dshaderutils_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderimage_p.h>

#ifdef QT_STATIC
// Static builds need the QML plugins explicitly imported, since the test loads
// "import QtQuick" and "import QtQuick3D" via QQmlComponent::setData() at
// runtime and the import scanner cannot discover them from the C++ sources.
Q_IMPORT_QML_PLUGIN(QtQuick2Plugin)
Q_IMPORT_QML_PLUGIN(QQuick3DPlugin)
#endif

class tst_QQuick3DShaderImage : public QObject
{
    Q_OBJECT

private slots:
    void testCreation();
    void testSizeProperties();
    void testSizePropertiesReject();
    void testArraySizeProperty();
    void testShapeProperty();
    void testShapePropertyInvalid();
    void testToShaderImageType_data();
    void testToShaderImageType();
    void testFormatProperty();
    void testFormatPropertyInvalid();
    void testToShaderFormat_data();
    void testToShaderFormat();
    void testToShaderFormatCoverage();
};

void tst_QQuick3DShaderImage::testCreation()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import QtQuick3D; ShaderImage { }", QUrl());
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);

    auto *image = qobject_cast<QQuick3DShaderImage *>(object.data());
    QVERIFY(image != nullptr);
    QCOMPARE(image->shape(), QQuick3DShaderImage::Image2D);
    QCOMPARE(image->width(), 1);
    QCOMPARE(image->height(), 1);
    QCOMPARE(image->depth(), 1);
    QCOMPARE(image->arraySize(), 1);
    QCOMPARE(image->format(), QQuick3DShaderImage::RGBA8);
}

void tst_QQuick3DShaderImage::testSizeProperties()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import QtQuick3D; ShaderImage { }", QUrl());
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));
    QScopedPointer<QObject> object(component.create());
    auto *image = qobject_cast<QQuick3DShaderImage *>(object.data());
    QVERIFY(image != nullptr);

    QSignalSpy widthSpy(image, &QQuick3DShaderImage::widthChanged);
    QSignalSpy heightSpy(image, &QQuick3DShaderImage::heightChanged);
    QSignalSpy depthSpy(image, &QQuick3DShaderImage::depthChanged);

    image->setWidth(256);
    image->setHeight(128);
    image->setDepth(4);

    QCOMPARE(image->width(), 256);
    QCOMPARE(image->height(), 128);
    QCOMPARE(image->depth(), 4);
    QCOMPARE(widthSpy.size(), 1);
    QCOMPARE(heightSpy.size(), 1);
    QCOMPARE(depthSpy.size(), 1);

    // Setting the same values again is a no-op.
    image->setWidth(256);
    image->setHeight(128);
    image->setDepth(4);
    QCOMPARE(widthSpy.size(), 1);
    QCOMPARE(heightSpy.size(), 1);
    QCOMPARE(depthSpy.size(), 1);
}

void tst_QQuick3DShaderImage::testFormatProperty()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(R"(
import QtQuick3D
ShaderImage {
    format: ShaderImage.RGBA16F
})",
                       QUrl());
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));
    QScopedPointer<QObject> object(component.create());
    auto *image = qobject_cast<QQuick3DShaderImage *>(object.data());
    QVERIFY(image != nullptr);

    QCOMPARE(image->format(), QQuick3DShaderImage::RGBA16F);
    QCOMPARE(QSSGShaderUtils::toShaderFormat(image->format()), QByteArray("rgba16f"));

    QSignalSpy formatSpy(image, &QQuick3DShaderImage::formatChanged);

    image->setFormat(QQuick3DShaderImage::R32UI);
    QCOMPARE(image->format(), QQuick3DShaderImage::R32UI);
    QCOMPARE(formatSpy.size(), 1);

    // Setting the same value again is a no-op.
    image->setFormat(QQuick3DShaderImage::R32UI);
    QCOMPARE(formatSpy.size(), 1);
}

void tst_QQuick3DShaderImage::testFormatPropertyInvalid()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import QtQuick3D; ShaderImage { }", QUrl());
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));
    QScopedPointer<QObject> object(component.create());
    auto *image = qobject_cast<QQuick3DShaderImage *>(object.data());
    QVERIFY(image != nullptr);

    const QQuick3DShaderImage::ImageFormat defaultFormat = image->format();
    QSignalSpy formatSpy(image, &QQuick3DShaderImage::formatChanged);

    // A value outside the ImageFormat enum has no corresponding
    // QRhiTexture::Format and must be rejected. QML coerces any int to an
    // enum-typed property without range-checking it, so this is reachable
    // from ordinary QML, not just from a C++ cast.
    QTest::ignoreMessage(QtWarningMsg, "ShaderImage format not supported.");
    image->setFormat(static_cast<QQuick3DShaderImage::ImageFormat>(9999));

    QCOMPARE(image->format(), defaultFormat);
    QCOMPARE(formatSpy.size(), 0);
}

void tst_QQuick3DShaderImage::testToShaderFormat_data()
{
    QTest::addColumn<int>("format");
    QTest::addColumn<QByteArray>("glslFormat");

    // Every ImageFormat value must map to a valid GLSL image format
    // qualifier: it is spliced into the shader's layout() qualifier and
    // compared for equality against the format string in the shader's
    // SHADER_IMAGE() declaration (see processMetadata()), so a typo here
    // makes the whole ShaderImage unusable.
    QTest::newRow("RGBA8") << int(QQuick3DShaderImage::RGBA8) << QByteArray("rgba8");
    QTest::newRow("R8") << int(QQuick3DShaderImage::R8) << QByteArray("r8");
    QTest::newRow("RG8") << int(QQuick3DShaderImage::RG8) << QByteArray("rg8");
    QTest::newRow("R16") << int(QQuick3DShaderImage::R16) << QByteArray("r16");
    QTest::newRow("RG16") << int(QQuick3DShaderImage::RG16) << QByteArray("rg16");
    QTest::newRow("RGBA16F") << int(QQuick3DShaderImage::RGBA16F) << QByteArray("rgba16f");
    QTest::newRow("RGBA32F") << int(QQuick3DShaderImage::RGBA32F) << QByteArray("rgba32f");
    QTest::newRow("R16F") << int(QQuick3DShaderImage::R16F) << QByteArray("r16f");
    QTest::newRow("R32F") << int(QQuick3DShaderImage::R32F) << QByteArray("r32f");
    // Note the underscore: the GLSL qualifier is rgb10_a2, unlike every other
    // format name, which is a plain concatenation.
    QTest::newRow("RGB10A2") << int(QQuick3DShaderImage::RGB10A2) << QByteArray("rgb10_a2");
    // The SI/UI enum names carry the bit width of the Qt-side format, while
    // the GLSL qualifiers use the i/ui suffix.
    QTest::newRow("R8SI") << int(QQuick3DShaderImage::R8SI) << QByteArray("r8i");
    QTest::newRow("R32SI") << int(QQuick3DShaderImage::R32SI) << QByteArray("r32i");
    QTest::newRow("RG32SI") << int(QQuick3DShaderImage::RG32SI) << QByteArray("rg32i");
    QTest::newRow("RGBA32SI") << int(QQuick3DShaderImage::RGBA32SI) << QByteArray("rgba32i");
    QTest::newRow("R8UI") << int(QQuick3DShaderImage::R8UI) << QByteArray("r8ui");
    QTest::newRow("R32UI") << int(QQuick3DShaderImage::R32UI) << QByteArray("r32ui");
    QTest::newRow("RG32UI") << int(QQuick3DShaderImage::RG32UI) << QByteArray("rg32ui");
    QTest::newRow("RGBA32UI") << int(QQuick3DShaderImage::RGBA32UI) << QByteArray("rgba32ui");
}

void tst_QQuick3DShaderImage::testToShaderFormat()
{
    QFETCH(int, format);
    QFETCH(QByteArray, glslFormat);

    QCOMPARE(QSSGShaderUtils::toShaderFormat(static_cast<QQuick3DShaderImage::ImageFormat>(format)), glslFormat);
}

void tst_QQuick3DShaderImage::testToShaderFormatCoverage()
{
    // Guards against an ImageFormat value being added without a matching
    // toShaderFormat() case: the function has a default label, so the
    // compiler will not catch it, and the new value would silently produce
    // an empty qualifier. The keyCount() check is deliberately exact, so
    // that adding a value also forces a new row in testToShaderFormat_data()
    // to pin down its expected qualifier.
    const QMetaEnum me = QMetaEnum::fromType<QQuick3DShaderImage::ImageFormat>();
    QCOMPARE(me.keyCount(), 18);
    for (int i = 0; i < me.keyCount(); ++i) {
        const auto format = static_cast<QQuick3DShaderImage::ImageFormat>(me.value(i));
        QVERIFY2(!QSSGShaderUtils::toShaderFormat(format).isEmpty(), me.key(i));
    }
}

// The size properties are QML-writable and end up as the arguments of
// QRhi::newTexture()/newTextureArray(), so out-of-range values have to be
// rejected in the setter instead of reaching the RHI.
void tst_QQuick3DShaderImage::testSizePropertiesReject()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import QtQuick3D; ShaderImage { width: 64; height: 32; depth: 2; arraySize: 3 }", QUrl());
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));
    QScopedPointer<QObject> object(component.create());
    auto *image = qobject_cast<QQuick3DShaderImage *>(object.data());
    QVERIFY(image != nullptr);

    QSignalSpy widthSpy(image, &QQuick3DShaderImage::widthChanged);
    QSignalSpy heightSpy(image, &QQuick3DShaderImage::heightChanged);
    QSignalSpy depthSpy(image, &QQuick3DShaderImage::depthChanged);
    QSignalSpy arraySizeSpy(image, &QQuick3DShaderImage::arraySizeChanged);

    // The shape decides which sizes are used, so none of them has a special
    // meaning for zero.
    image->setWidth(0);
    image->setWidth(-1);
    QCOMPARE(image->width(), 64);
    QCOMPARE(widthSpy.size(), 0);

    image->setHeight(0);
    image->setHeight(-1);
    QCOMPARE(image->height(), 32);
    QCOMPARE(heightSpy.size(), 0);

    image->setDepth(0);
    image->setDepth(-1);
    QCOMPARE(image->depth(), 2);
    QCOMPARE(depthSpy.size(), 0);

    // An array must have at least one layer; zero is not "not an array".
    image->setArraySize(0);
    image->setArraySize(-1);
    QCOMPARE(image->arraySize(), 3);
    QCOMPARE(arraySizeSpy.size(), 0);
}

void tst_QQuick3DShaderImage::testArraySizeProperty()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import QtQuick3D; ShaderImage { arraySize: 6 }", QUrl());
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));
    QScopedPointer<QObject> object(component.create());
    auto *image = qobject_cast<QQuick3DShaderImage *>(object.data());
    QVERIFY(image != nullptr);
    QCOMPARE(image->arraySize(), 6);

    QSignalSpy arraySizeSpy(image, &QQuick3DShaderImage::arraySizeChanged);

    image->setArraySize(2);
    QCOMPARE(image->arraySize(), 2);
    QCOMPARE(arraySizeSpy.size(), 1);
    // The array size is part of the image's shape, so it must invalidate the
    // image for anything watching it, not just notify its own property.

    // Setting the same value again is a no-op.
    image->setArraySize(2);
    QCOMPARE(arraySizeSpy.size(), 1);
}

void tst_QQuick3DShaderImage::testShapeProperty()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import QtQuick3D; ShaderImage { shape: ShaderImage.ImageCube; width: 16; height: 16 }",
                      QUrl());
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));
    QScopedPointer<QObject> object(component.create());
    auto *image = qobject_cast<QQuick3DShaderImage *>(object.data());
    QVERIFY(image != nullptr);
    QCOMPARE(image->shape(), QQuick3DShaderImage::ImageCube);

    // The backend node is always an Image2D, so that the shape can change
    // without replacing the node; the texture follows the shape instead.
    QScopedPointer<QSSGRenderGraphObject> node(QQuick3DObjectPrivate::updateSpatialNode(image, nullptr));
    QVERIFY(node != nullptr);
    QCOMPARE(node->type, QSSGRenderGraphObject::Type::Image2D);
    auto *renderImage = static_cast<QSSGRenderImage *>(node.data());
    QVERIFY(renderImage->m_shaderImage);
    QCOMPARE(renderImage->m_shaderImageShape, QSSGRenderImage::ShaderImageShape::ImageCube);
    QCOMPARE(renderImage->m_shaderImageSize.x, 16);
    QCOMPARE(renderImage->m_shaderImageSize.y, 16);

    QSignalSpy shapeSpy(image, &QQuick3DShaderImage::shapeChanged);
    renderImage->m_flags.setFlag(QSSGRenderImage::Flag::ShaderImageDirty, false);
    image->setShape(QQuick3DShaderImage::Image3D);
    image->setDepth(8);
    QCOMPARE(shapeSpy.size(), 1);
    QQuick3DObjectPrivate::updateSpatialNode(image, node.data());
    QCOMPARE(renderImage->m_shaderImageShape, QSSGRenderImage::ShaderImageShape::Image3D);
    QCOMPARE(renderImage->m_shaderImageSize.z, 8);
    // A new shape needs a new texture.
    QVERIFY(renderImage->m_flags.testFlag(QSSGRenderImage::Flag::ShaderImageDirty));

    // Setting the same value again is a no-op.
    image->setShape(QQuick3DShaderImage::Image3D);
    QCOMPARE(shapeSpy.size(), 1);
}

void tst_QQuick3DShaderImage::testShapePropertyInvalid()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import QtQuick3D; ShaderImage { shape: ShaderImage.Image2DArray }", QUrl());
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));
    QScopedPointer<QObject> object(component.create());
    auto *image = qobject_cast<QQuick3DShaderImage *>(object.data());
    QVERIFY(image != nullptr);

    QSignalSpy shapeSpy(image, &QQuick3DShaderImage::shapeChanged);
    QTest::ignoreMessage(QtWarningMsg, "ShaderImage shape not supported.");
    image->setShape(static_cast<QQuick3DShaderImage::ImageShape>(42));
    QCOMPARE(image->shape(), QQuick3DShaderImage::Image2DArray);
    QCOMPARE(shapeSpy.size(), 0);
}

void tst_QQuick3DShaderImage::testToShaderImageType_data()
{
    QTest::addColumn<int>("shape");
    QTest::addColumn<int>("format");
    QTest::addColumn<QByteArray>("glslType");

    // Compared for equality against the image type in the shader's
    // SHADER_IMAGE() declaration (see processMetadata()).
    QTest::newRow("Image1D RGBA8") << int(QQuick3DShaderImage::Image1D) << int(QQuick3DShaderImage::RGBA8)
                                   << QByteArray("image1D");
    QTest::newRow("Image2D RGBA16F") << int(QQuick3DShaderImage::Image2D) << int(QQuick3DShaderImage::RGBA16F)
                                     << QByteArray("image2D");
    QTest::newRow("Image3D R32F") << int(QQuick3DShaderImage::Image3D) << int(QQuick3DShaderImage::R32F)
                                  << QByteArray("image3D");
    QTest::newRow("Image2DArray RGB10A2") << int(QQuick3DShaderImage::Image2DArray) << int(QQuick3DShaderImage::RGB10A2)
                                          << QByteArray("image2DArray");
    QTest::newRow("ImageCube RGBA8") << int(QQuick3DShaderImage::ImageCube) << int(QQuick3DShaderImage::RGBA8)
                                     << QByteArray("imageCube");
    QTest::newRow("Image2D R8SI") << int(QQuick3DShaderImage::Image2D) << int(QQuick3DShaderImage::R8SI)
                                  << QByteArray("iimage2D");
    QTest::newRow("Image3D RGBA32SI") << int(QQuick3DShaderImage::Image3D) << int(QQuick3DShaderImage::RGBA32SI)
                                      << QByteArray("iimage3D");
    QTest::newRow("Image2D R32UI") << int(QQuick3DShaderImage::Image2D) << int(QQuick3DShaderImage::R32UI)
                                   << QByteArray("uimage2D");
    QTest::newRow("ImageCube RG32UI") << int(QQuick3DShaderImage::ImageCube) << int(QQuick3DShaderImage::RG32UI)
                                      << QByteArray("uimageCube");
}

void tst_QQuick3DShaderImage::testToShaderImageType()
{
    QFETCH(int, shape);
    QFETCH(int, format);
    QFETCH(QByteArray, glslType);

    QCOMPARE(QSSGShaderUtils::toShaderImageType(static_cast<QQuick3DShaderImage::ImageShape>(shape),
                                                static_cast<QQuick3DShaderImage::ImageFormat>(format)),
             glslType);
}

QTEST_MAIN(tst_QQuick3DShaderImage)
#include "tst_qquick3dshaderimage.moc"
