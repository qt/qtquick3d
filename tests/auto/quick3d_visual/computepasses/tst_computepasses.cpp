// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QtQuick/QQuickView>
#include "../shared/util.h"
#include <rhi/qrhi.h>
#include <QtQuick3D/private/qquick3dshaderimage_p.h>

// NoLighting materials and Effect output are stored in a linear render
// target and displayed after sRGB encoding/tonemap-pipeline handling; pure
// primaries can shift by a noticeable amount without indicating a bug (see
// the identical rationale in tst_userpasses.cpp). Use a generous fuzz.
static const int FUZZ = 60;

// Scans the whole image for a pixel close to the given colour, rather than
// asserting an exact position -- used where a primitive's UV origin/winding
// isn't verified, so a generated region's on-screen position isn't known.
static bool imageContainsColor(const QImage &image, const QColor &color, int fuzz)
{
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QColor pixel = image.pixelColor(x, y);
            if (qAbs(pixel.red() - color.red()) <= fuzz && qAbs(pixel.green() - color.green()) <= fuzz
                && qAbs(pixel.blue() - color.blue()) <= fuzz) {
                return true;
            }
        }
    }
    return false;
}

// Ordered 1:1 with the results[idx++] sequence in builtin_uniforms.comp,
// which paints each check into its own cell of a GRID_COLS x GRID_ROWS grid
// (green = pass, red = fail) instead of collapsing everything into a single
// pass/fail colour, so a failure here can name the exact uniform at fault.
static const char *const kBuiltinUniformCheckNames[] = {
    "VIEW_MATRIX * INVERSE_VIEW_MATRIX == identity",
    "PROJECTION_MATRIX * INVERSE_PROJECTION_MATRIX == identity",
    "VIEWPROJECTION_MATRIX * INVERSE_VIEWPROJECTION_MATRIX == identity",
    "CAMERA_POSITION",
    "CAMERA_DIRECTION",
    "CAMERA_PROPERTIES.x (near clip)",
    "CAMERA_PROPERTIES.y (far clip)",
    "FRAMEBUFFER_Y_UP",
    "NDC_Y_UP",
    "NEAR_CLIP_VALUE",
    "FRAME",
    "DIRECTIONAL_LIGHT_COUNT",
    "POINT_LIGHT_COUNT",
    "SPOT_LIGHT_COUNT",
    "DIRECTIONAL_LIGHT_DIFFUSE(0)",
    "POINT_LIGHT_DIFFUSE(0)",
    "POINT_LIGHT_POSITION(0)",
    "POINT_LIGHT_ATTENUATION(0)",
    "SPOT_LIGHT_DIFFUSE(0)",
    "SPOT_LIGHT_POSITION(0)",
    "SPOT_LIGHT_CONES(0)",
    "DEPTH_TEXTURE (sphere centre)",
    "DEPTH_TEXTURE (background corner)",
    "SCREEN_TEXTURE (sphere centre)",
    "AO_TEXTURE range",
    "NORMAL_ROUGHNESS_TEXTURE sanity",
};
static const int kBuiltinUniformCheckCount =
    int(sizeof(kBuiltinUniformCheckNames) / sizeof(kBuiltinUniformCheckNames[0]));
static const int kBuiltinUniformGridCols = 6;
static const int kBuiltinUniformGridRows = 5;

// Ordered 1:1 with the results[idx++] sequence in shadow_uniforms.comp.
static const char *const kShadowUniformCheckNames[] = {
    "DIRECTIONAL_LIGHT_COUNT",
    "POINT_LIGHT_COUNT",
    "SPOT_LIGHT_COUNT",
    "QSSG_SHADOW_SOFTNESS",
    "DIRECTIONAL_LIGHT_SHADOW range",
    "POINT_LIGHT_SHADOW range",
    "SPOT_LIGHT_SHADOW range",
};
static const int kShadowUniformCheckCount =
    int(sizeof(kShadowUniformCheckNames) / sizeof(kShadowUniformCheckNames[0]));
static const int kShadowUniformGridCols = 4;
static const int kShadowUniformGridRows = 2;

class tst_ComputePasses : public QQuick3DDataTest
{
    Q_OBJECT

private slots:
    void initTestCase() override;
    void testScreenTextureInvert();
    void testLutBuffer();
    void testBufferShrink();
    void testChainedPasses();
    void testAtomicAccess();
    void testBuiltinUniforms();
    void testShadowUniforms();
    void testShadowOnly();
    void testImageShapes_data();
    void testImageShapes();
    void testProceduralTexture();
    void testRenderPassDisplay();
    void testEffectWrite();
    void testCustomMaterialWrite();
    void testBufferProvider();
    void testBufferProviderUnregistered();
};

void tst_ComputePasses::initTestCase()
{
    QQuick3DDataTest::initTestCase();
    if (!initialized())
        QSKIP("Could not initialize visual test");
    if (!isComputeSupported())
        QSKIP("Compute shaders are not supported with this graphics API");
}

void tst_ComputePasses::testScreenTextureInvert()
{
    // A ComputePass reads SCREEN_TEXTURE (red sphere on black background),
    // inverts it, and writes the result into a ShaderImage that an Effect
    // displays full-screen.
    QScopedPointer<QQuickView> view(createView(QLatin1String("screentexture_invert.qml"), QSize(400, 400)));
    QVERIFY(view);
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    const QImage result = grab(view.data());
    QVERIFY(!result.isNull());

    // Background corner (originally black) should be inverted to white.
    QVERIFY(comparePixelNormPos(result, 0.02, 0.02, Qt::white, FUZZ));
    // Sphere centre (originally red) should be inverted to cyan.
    QVERIFY(comparePixelNormPos(result, 0.5, 0.5, Qt::cyan, FUZZ));
}

void tst_ComputePasses::testLutBuffer()
{
    // LutBufferData uploads a fixed 4-colour table (red, green, blue, yellow)
    // to a ShaderStorageBuffer; a ComputePass paints one colour per vertical
    // quarter of the output image based on it.
    QScopedPointer<QQuickView> view(createView(QLatin1String("lut_buffer.qml"), QSize(400, 400)));
    QVERIFY(view);
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    const QImage result = grab(view.data());
    QVERIFY(!result.isNull());

    QVERIFY(comparePixelNormPos(result, 0.125, 0.5, Qt::red, FUZZ));
    QVERIFY(comparePixelNormPos(result, 0.375, 0.5, Qt::green, FUZZ));
    QVERIFY(comparePixelNormPos(result, 0.625, 0.5, Qt::blue, FUZZ));
    QVERIFY(comparePixelNormPos(result, 0.875, 0.5, Qt::yellow, FUZZ));
}

void tst_ComputePasses::testBufferShrink()
{
    // After the ShaderStorageBuffer's bufferData shrinks, the length() of a
    // runtime-sized array must follow, rather than still count the old,
    // larger buffer.
    QScopedPointer<QQuickView> view(createView(QLatin1String("buffer_shrink.qml"), QSize(400, 400)));
    QVERIFY(view);
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    QImage result = grab(view.data());
    QVERIFY(!result.isNull());
    QVERIFY(comparePixelNormPos(result, 0.5, 0.5, Qt::green, FUZZ));

    // 2 elements is smaller than any minimum buffer size the engine used to
    // apply, and 0 leaves the buffer empty.
    QQuickItem *root = view->rootObject();
    for (int count : { 5, 2, 0 }) {
        QVERIFY(root->setProperty("count", count));
        QVERIFY(root->setProperty("expectedCount", count));
        for (int i = 0; i < 3; ++i)
            result = grab(view.data());
        QVERIFY(!result.isNull());
        QVERIFY2(comparePixelNormPos(result, 0.5, 0.5, Qt::green, FUZZ), qPrintable(QString::number(count)));
    }
}

void tst_ComputePasses::testChainedPasses()
{
    // producerPass writes green into a ShaderStorageBuffer; consumerPass reads
    // it, inverts it to magenta, and writes that into the displayed image.
    // If pass ordering/synchronization were broken, the buffer would still
    // be zero-initialized when consumerPass reads it, producing white
    // (inverted black) instead of magenta.
    QScopedPointer<QQuickView> view(createView(QLatin1String("chained_passes.qml"), QSize(400, 400)));
    QVERIFY(view);
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    const QImage result = grab(view.data());
    QVERIFY(!result.isNull());

    QVERIFY(comparePixelNormPos(result, 0.5, 0.5, Qt::magenta, FUZZ));
}

void tst_ComputePasses::testAtomicAccess()
{
    // Three chained ComputePasses: resetPass zero-initializes a buffer
    // counter and a 4-bin r32ui histogram image; countPass atomically
    // increments both once per output pixel ("coherent restrict"
    // buffer/image); verifyPass reads them back ("volatile readonly") and
    // paints the screen green only if every atomic total is exactly what
    // 400x400 invocations bucketed into 4 equal columns should produce.
    // Any lost atomic update (race condition, missing coherency) undercounts
    // and turns the screen red instead.
    //
    // Not supported on Metal: native image atomics require MSL 3.1 (macOS 14 /
    // iOS 17). Below that, SPIRV-Cross emulates imageAtomic*() by adding a
    // hidden "device atomic" helper buffer argument to the entry point,
    // reported as the secondary native binding of the storage image. The Metal
    // QRhi backend only ever binds the primary (texture) half of that mapping
    // for ImageLoad/ImageStore/ImageLoadStore, so the helper buffer is left
    // unbound. Qt Quick 3D also targets MSL 1.2 unconditionally when baking at
    // runtime (both baker setups in qssgrendershadercache.cpp), so the
    // emulation path is taken on every macOS version, not just old ones.
    // The buffer atomic (atomicAdd on a storage buffer) in count.comp is fine
    // on Metal; only the imageAtomicAdd half is not.
    if (graphicsApi() == QSGRendererInterface::Metal)
        QSKIP("imageAtomic*() needs MSL 3.1 and a QRhi Metal backend that binds the emulated atomic helper buffer");

    QScopedPointer<QQuickView> view(createView(QLatin1String("atomic_counters.qml"), QSize(400, 400)));
    QVERIFY(view);
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    const QImage result = grab(view.data());
    QVERIFY(!result.isNull());

    // Sample the middle of each of the 4 histogram buckets.
    QVERIFY(comparePixelNormPos(result, 0.125, 0.5, Qt::green, FUZZ));
    QVERIFY(comparePixelNormPos(result, 0.375, 0.5, Qt::green, FUZZ));
    QVERIFY(comparePixelNormPos(result, 0.625, 0.5, Qt::green, FUZZ));
    QVERIFY(comparePixelNormPos(result, 0.875, 0.5, Qt::green, FUZZ));
}

void tst_ComputePasses::testBuiltinUniforms()
{
    // A ComputePass checks camera matrices/properties, RHI sentinel
    // properties, the frame counter, and per-light-type counts/diffuse
    // colours/positions/attenuation/cones against this scene's known
    // setup, plus screen/depth/AO/normal-roughness texture sanity. Each
    // check is painted into its own cell of a grid (green = pass, red =
    // fail; see kBuiltinUniformCheckNames), so a failure here names the
    // exact uniform at fault instead of just "something is wrong".
    QScopedPointer<QQuickView> view(createView(QLatin1String("builtin_uniforms.qml"), QSize(400, 400)));
    QVERIFY(view);
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    const QImage result = grab(view.data());
    QVERIFY(!result.isNull());

    for (int i = 0; i < kBuiltinUniformCheckCount; ++i) {
        const int col = i % kBuiltinUniformGridCols;
        const int row = i / kBuiltinUniformGridCols;
        const qreal normX = (col + 0.5) / kBuiltinUniformGridCols;
        const qreal normY = (row + 0.5) / kBuiltinUniformGridRows;
        QVERIFY2(comparePixelNormPos(result, normX, normY, Qt::green, FUZZ),
                 qPrintable(QString("Builtin uniform check failed: %1").arg(kBuiltinUniformCheckNames[i])));
    }
}

void tst_ComputePasses::testShadowUniforms()
{
    // A ComputePass checks that QSSG_SHADOW_SOFTNESS resolves to the
    // expected compile-time value and that DIRECTIONAL_LIGHT_SHADOW/
    // POINT_LIGHT_SHADOW/SPOT_LIGHT_SHADOW are bound and return a valid
    // illumination factor. Each check is painted into its own cell of a
    // grid (green = pass, red = fail; see kShadowUniformCheckNames), so a
    // failure here names the exact check at fault.
    QScopedPointer<QQuickView> view(createView(QLatin1String("shadow_uniforms.qml"), QSize(400, 400)));
    QVERIFY(view);
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    const QImage result = grab(view.data());
    QVERIFY(!result.isNull());

    for (int i = 0; i < kShadowUniformCheckCount; ++i) {
        const int col = i % kShadowUniformGridCols;
        const int row = i / kShadowUniformGridCols;
        const qreal normX = (col + 0.5) / kShadowUniformGridCols;
        const qreal normY = (row + 0.5) / kShadowUniformGridRows;
        QVERIFY2(comparePixelNormPos(result, normX, normY, Qt::green, FUZZ),
                 qPrintable(QString("Shadow uniform check failed: %1").arg(kShadowUniformCheckNames[i])));
    }
}

void tst_ComputePasses::testShadowOnly()
{
    // A ComputePass that uses only the *_LIGHT_SHADOW macros, without any of
    // the light identifiers, still gets the light data and shadow maps bound.
    // The whole image is green when all three macros return a valid factor.
    QScopedPointer<QQuickView> view(createView(QLatin1String("shadow_only.qml"), QSize(400, 400)));
    QVERIFY(view);
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    const QImage result = grab(view.data());
    QVERIFY(!result.isNull());

    QVERIFY(comparePixelNormPos(result, 0.5, 0.5, Qt::green, FUZZ));
}

void tst_ComputePasses::testImageShapes_data()
{
    QTest::addColumn<int>("shape");
    QTest::addColumn<QString>("shader");
    QTest::addColumn<int>("feature");
    QTest::addColumn<bool>("direct3D");

    QTest::newRow("Image1D") << int(QQuick3DShaderImage::Image1D) << QStringLiteral("shape_image1D.comp")
                             << int(QRhi::OneDimensionalTextures) << false;
    QTest::newRow("Image2D") << int(QQuick3DShaderImage::Image2D) << QStringLiteral("shape_image2D.comp") << -1 << true;
    QTest::newRow("Image3D") << int(QQuick3DShaderImage::Image3D) << QStringLiteral("shape_image3D.comp")
                             << int(QRhi::ThreeDimensionalTextures) << true;
    QTest::newRow("Image2DArray") << int(QQuick3DShaderImage::Image2DArray) << QStringLiteral("shape_image2DArray.comp")
                                  << int(QRhi::TextureArrays) << true;
    QTest::newRow("ImageCube") << int(QQuick3DShaderImage::ImageCube) << QStringLiteral("shape_imageCube.comp") << -1 << false;
}

void tst_ComputePasses::testImageShapes()
{
    // Each shape creates its kind of texture, which the shader declaring the
    // matching image type can write and read back.
    QFETCH(int, shape);
    QFETCH(QString, shader);
    QFETCH(int, feature);
    QFETCH(bool, direct3D);

    // Direct3D has neither load/store views of 1D textures in QRhi nor a
    // read-write cube texture type in HLSL.
    if (!direct3D && (graphicsApi() == QSGRendererInterface::Direct3D11
                      || graphicsApi() == QSGRendererInterface::Direct3D12)) {
        QSKIP("The image shape is not supported with Direct3D");
    }

    QScopedPointer<QQuickView> view(createView(QLatin1String("shape.qml"), QSize(400, 400)));
    QVERIFY(view);
    view->rootObject()->setProperty("computeShader", shader);
    view->rootObject()->setProperty("imageShape", shape);
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));
    if (feature >= 0 && !view->rhi()->isFeatureSupported(QRhi::Feature(feature)))
        QSKIP("The image shape is not supported with this graphics API");

    const QImage result = grab(view.data());
    QVERIFY(!result.isNull());
    QVERIFY(comparePixelNormPos(result, 0.5, 0.5, Qt::green, FUZZ));
}

void tst_ComputePasses::testProceduralTexture()
{
    // A ComputePass writes a 2x2 procedural pattern (red/green/blue/yellow)
    // into a ShaderImage; a CustomMaterial applied to a flat rectangle
    // samples that image directly via imageLoad(), independent of any
    // Effect/full-screen-quad display path.
    QScopedPointer<QQuickView> view(createView(QLatin1String("procedural_texture.qml"), QSize(400, 400)));
    QVERIFY(view);
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    const QImage result = grab(view.data());
    QVERIFY(!result.isNull());

    QVERIFY2(imageContainsColor(result, Qt::red, FUZZ), "Red region of the procedural texture not found");
    QVERIFY2(imageContainsColor(result, Qt::green, FUZZ), "Green region of the procedural texture not found");
    QVERIFY2(imageContainsColor(result, Qt::blue, FUZZ), "Blue region of the procedural texture not found");
    QVERIFY2(imageContainsColor(result, Qt::yellow, FUZZ), "Yellow region of the procedural texture not found");
}

void tst_ComputePasses::testRenderPassDisplay()
{
    // Same ComputePass-generated 2x2 red/green/blue/yellow pattern as
    // testProceduralTexture, but consumed via a RenderPass's OverrideMaterial
    // (rendered into an offscreen RenderPassTexture, then displayed by a
    // SimpleQuadRenderer) instead of being assigned directly as a Model's
    // material or read back through an Effect.
    QScopedPointer<QQuickView> view(createView(QLatin1String("renderpass_display.qml"), QSize(400, 400)));
    QVERIFY(view);
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    const QImage result = grab(view.data());
    QVERIFY(!result.isNull());

    QVERIFY2(imageContainsColor(result, Qt::red, FUZZ), "Red region of the RenderPass-displayed pattern not found");
    QVERIFY2(imageContainsColor(result, Qt::green, FUZZ), "Green region of the RenderPass-displayed pattern not found");
    QVERIFY2(imageContainsColor(result, Qt::blue, FUZZ), "Blue region of the RenderPass-displayed pattern not found");
    QVERIFY2(imageContainsColor(result, Qt::yellow, FUZZ), "Yellow region of the RenderPass-displayed pattern not found");
}

void tst_ComputePasses::testEffectWrite()
{
    // writeEffect atomically increments a ShaderStorageBuffer counter and
    // imageStore()s a marker into a ShaderImage once per output pixel;
    // verifyEffect (the next stage in the same Effects chain) reads both
    // back and paints the screen green only if the counter reached exactly
    // the output pixel count and the marker is present everywhere. Unlike
    // every other test in this suite, the write side here is an Effect, not
    // a ComputePass.
    //
    // NOTE: the buffer-counter half of this test exercises a path QRhi's own
    // documentation does not universally guarantee (shader storage buffer
    // read/write from a graphics, non-compute, pipeline stage); see the
    // CAVEAT comment in effect_write.frag. A failure here may point to a
    // backend limitation rather than an engine bug.
    QScopedPointer<QQuickView> view(createView(QLatin1String("effect_write.qml"), QSize(400, 400)));
    QVERIFY(view);
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    const QImage result = grab(view.data());
    QVERIFY(!result.isNull());

    QVERIFY(comparePixelNormPos(result, 0.5, 0.5, Qt::green, FUZZ));
}

void tst_ComputePasses::testCustomMaterialWrite()
{
    // Same idea as testEffectWrite, but the writer is a CustomMaterial
    // rendering a rectangle (real geometry, so coverage isn't exactly
    // countable like a full-screen Effect) instead of a full-screen Effect.
    // verifyEffect (running after the main opaque pass, in the same frame)
    // checks that enough fragments wrote via atomicAdd to prove the write
    // path works at all, and that the marker image shows the write where
    // the rectangle covers the screen (centre) but not where it doesn't
    // (background corner, still the pre-cleared sentinel).
    //
    // NOTE: the buffer-counter half of this test exercises a path QRhi's own
    // documentation does not universally guarantee (shader storage buffer
    // read/write from a graphics, non-compute, pipeline stage); see the
    // CAVEAT comment in custommaterial_write.frag. A failure here may point
    // to a backend limitation rather than an engine bug.
    QScopedPointer<QQuickView> view(createView(QLatin1String("custommaterial_write.qml"), QSize(400, 400)));
    QVERIFY(view);
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    const QImage result = grab(view.data());
    QVERIFY(!result.isNull());

    QVERIFY2(comparePixelNormPos(result, 0.5, 0.5, Qt::green, FUZZ),
             "Centre (covered by the rectangle) should show the CustomMaterial's write");
    QVERIFY2(comparePixelNormPos(result, 0.02, 0.02, Qt::red, FUZZ),
             "Corner (background, not covered by the rectangle) should remain the unwritten sentinel");
}

void tst_ComputePasses::testBufferProvider()
{
    // Identical to testChainedPasses, except the ShaderStorageBuffer's backing
    // GPU resource comes from a BufferProviderExtension (a C++ RenderExtension
    // that creates its own QRhiBuffer and registers it via
    // QSSGRenderExtensionHelpers::registerRenderResultBuffer()) instead of
    // being allocated by the engine via ShaderStorageBuffer.size. producerPass
    // writes green into it, consumerPass reads it back and inverts it to
    // magenta.
    QScopedPointer<QQuickView> view(createView(QLatin1String("bufferprovider.qml"), QSize(400, 400)));
    QVERIFY(view);
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    const QImage result = grab(view.data());
    QVERIFY(!result.isNull());

    QVERIFY(comparePixelNormPos(result, 0.5, 0.5, Qt::magenta, FUZZ));
}

void tst_ComputePasses::testBufferProviderUnregistered()
{
    // A provider that registers nullptr has no buffer, so the
    // ShaderStorageBuffer falls back to its own, sized by its bufferData: the
    // producer/consumer pair still works through it.
    QTest::failOnWarning(QRegularExpression(QStringLiteral("not bound")));
    QScopedPointer<QQuickView> view(createView(QLatin1String("bufferprovider_null.qml"), QSize(400, 400)));
    QVERIFY(view);
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    const QImage result = grab(view.data());
    QVERIFY(!result.isNull());

    QVERIFY(comparePixelNormPos(result, 0.5, 0.5, Qt::magenta, FUZZ));
}

QTEST_MAIN(tst_ComputePasses)
#include "tst_computepasses.moc"
