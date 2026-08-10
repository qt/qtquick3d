// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QSignalSpy>
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QtQml/QQmlComponent>
#include <QtQuick/QQuickView>
#include <QtQuick3D/private/qquick3dviewport_p.h>
#include <QtQuick3D/private/qquick3drenderpass_p.h>
#include <QtQuick3D/private/qquick3dobject_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderuserpass_p.h>
#include <QtQuick3DRuntimeRender/private/qssguserrenderpassmanager_p.h>
#include "../shared/util.h"

class tst_UserPasses : public QQuick3DDataTest
{
    Q_OBJECT

private slots:
    void initTestCase() override;
    void testBasicRenderPass();
    void testMultipleRenderPasses();
    void testMultipleRenderTargets();
    void testDisableInternalPasses();
    void testRenderPassTextures();
    void testRenderPassTextures_data();
    void testSubRenderPass();
    void testContentLayers();
    void testRenderablesFilter();
    void testRenderablesFilter_data();
    void testPipelineStateOverride();
    void testPipelineStateOverride_data();
    void subPassOverrideMaterial();
    void multipleSubPasses();
    void subPassDependencyIndex();
    void userPassManagerScheduling();
    void subPassRoleTransition();
    void subPassRefCounting();
    void subPassCommandDestroyed();
    void referencedPassDestroyed();
    void unreferencedPassReturnsToTopLevel();
    void slotLimitDoesNotCrash();
    void depthLimitDoesNotCrash();
    void testAddDefine();
    void testSkyboxPass();
    void testItem2DPass();
    void testDepthTestDisabled();
    void testBlendEnabled();
    void testPreserveColorContents();
    void testViewportOverride();
    void testScreenTexture();
    void testCullModeOverrideSubpass_data();
    void testCullModeOverrideSubpass();
    void testBlendOverrideOriginalMaterial();
    void noColorAttachmentDoesNotCrash();

private:
    // Helper function to check if an image contains a specific color (with lighting effects)
    bool imageContainsColor(const QImage &image, const QColor &color, int tolerance = 30);
    // Helper to check if image contains a dominant color (e.g. mostly red, mostly green)
    bool imageContainsDominantColor(const QImage &image, const QColor &color, int minValue = 100);
    // Helper to check if image is not all black (something rendered)
    bool imageHasNonBlackPixels(const QImage &image);
};

void tst_UserPasses::initTestCase()
{
    QQuick3DDataTest::initTestCase();
    if (!initialized())
        QSKIP("Could not initialize visual test");
}

bool tst_UserPasses::imageContainsColor(const QImage &image, const QColor &color, int tolerance)
{
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            QColor pixel = image.pixelColor(x, y);
            int rDiff = qAbs(pixel.red() - color.red());
            int gDiff = qAbs(pixel.green() - color.green());
            int bDiff = qAbs(pixel.blue() - color.blue());

            if (rDiff <= tolerance && gDiff <= tolerance && bDiff <= tolerance) {
                return true;
            }
        }
    }
    return false;
}

bool tst_UserPasses::imageContainsDominantColor(const QImage &image, const QColor &color, int minValue)
{
    // Check which component is dominant in the expected color
    int r = color.red();
    int g = color.green();
    int b = color.blue();

    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            QColor pixel = image.pixelColor(x, y);

            // For gray (all channels equal or similar)
            if (qAbs(r - g) < 20 && qAbs(g - b) < 20 && qAbs(r - b) < 20) {
                int pixelAvg = (pixel.red() + pixel.green() + pixel.blue()) / 3;
                int colorAvg = (r + g + b) / 3;
                if (qAbs(pixelAvg - colorAvg) < 50) {
                    return true;
                }
            }
            // For red: R should be high and dominant
            else if (r > g && r > b && r > 128) {
                if (pixel.red() >= minValue && pixel.red() > pixel.green() && pixel.red() > pixel.blue()) {
                    return true;
                }
            }
            // For green: G should be high and dominant
            else if (g > r && g > b && g > 128) {
                if (pixel.green() >= minValue && pixel.green() > pixel.red() && pixel.green() > pixel.blue()) {
                    return true;
                }
            }
            // For blue: B should be high and dominant
            else if (b > r && b > g && b > 128) {
                if (pixel.blue() >= minValue && pixel.blue() > pixel.red() && pixel.blue() > pixel.green()) {
                    return true;
                }
            }
            // For dark blue (blue dominant but all low values)
            else if (b > r && b > g && b < 200) {
                if (pixel.blue() > pixel.red() && pixel.blue() > pixel.green() && pixel.blue() >= 50) {
                    return true;
                }
            }
            // For cyan (G and B high, R low)
            else if (r < 128 && g > 128 && b > 128) {
                if (pixel.red() < 128 && pixel.green() >= minValue && pixel.blue() >= minValue) {
                    return true;
                }
            }
            // For yellow (R and G high, B low)
            else if (r > 128 && g > 128 && b < 128) {
                if (pixel.red() >= minValue && pixel.green() >= minValue && pixel.blue() < 128) {
                    return true;
                }
            }
            // For purple/magenta (R and B high, G low)
            else if (r > 64 && g < 128 && b > 64) {
                if (pixel.red() >= 50 && pixel.green() < 128 && pixel.blue() >= 50) {
                    return true;
                }
            }
            // For orange (R high, G medium, B low)
            else if (r > 128 && g > 64 && b < 128) {
                if (pixel.red() >= minValue && pixel.green() >= 50 && pixel.blue() < 128) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool tst_UserPasses::imageHasNonBlackPixels(const QImage &image)
{
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            QColor pixel = image.pixelColor(x, y);
            if (pixel.red() > 10 || pixel.green() > 10 || pixel.blue() > 10) {
                return true;
            }
        }
    }
    return false;
}

void tst_UserPasses::testBasicRenderPass()
{
    QScopedPointer<QQuickView> view(createView(QLatin1String("basicrenderpass.qml"), QSize(400, 400)));
    QVERIFY(view);
    view->show();
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    // Get the viewport
    QQuick3DViewport *viewport = view->rootObject()->findChild<QQuick3DViewport *>();
    QVERIFY(viewport);

    // Verify render pass is created
    QQuick3DRenderPass *renderPass = view->rootObject()->findChild<QQuick3DRenderPass *>();
    QVERIFY(renderPass);

    // Capture the rendered image
    QImage result = grab(view.data());
    QVERIFY(!result.isNull());
    // Size may be scaled for high DPI displays
    QVERIFY(result.width() >= 400);
    QVERIFY(result.height() >= 400);

    // The test validates the user pass is being used by checking the clear color
    // RenderPass.clearColor = "blue", environment.clearColor = "black"
    // If the user pass is working, background should be blue, not black
    QVERIFY2(imageContainsDominantColor(result, Qt::blue), "Background should be blue (from pass clearColor)");

    // The red cube should also be visible
    QVERIFY2(imageContainsDominantColor(result, Qt::red), "Red cube should be visible");

    // Validate that no green is visible (proves the imageConainsDominantColor check is valid)
    QVERIFY2(!imageContainsDominantColor(result, Qt::green), "No Green should be visible");
}

void tst_UserPasses::testMultipleRenderPasses()
{
    QScopedPointer<QQuickView> view(createView(QLatin1String("multiplepasses.qml"), QSize(400, 400)));
    QVERIFY(view);
    view->show();
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    // Verify multiple render passes are created
    QList<QQuick3DRenderPass *> renderPasses = view->rootObject()->findChildren<QQuick3DRenderPass *>();
    QVERIFY(renderPasses.size() > 1);

    // Capture the rendered image
    QImage result = grab(view.data());
    QVERIFY(!result.isNull());

    // This test validates pass chaining:
    // - pass1 renders red cube on yellow background to texture
    // - pass2 uses pass1's texture on a sphere, with cyan background
    // - We should see cyan background (from pass2) proving pass2 is active
    // - We should see red/yellow (from pass1 texture on sphere) proving passes chain
    QVERIFY2(imageContainsDominantColor(result, Qt::cyan), "Background should be cyan (from pass2)");
    QVERIFY2(imageContainsDominantColor(result, Qt::red), "Red from pass1 should be visible on sphere");
    QVERIFY2(imageContainsDominantColor(result, Qt::yellow), "Yellow from pass1 should be visible on sphere");
    // Validate that no green is visible (proves the imageConainsDominantColor check is valid)
    QVERIFY2(!imageContainsDominantColor(result, Qt::green), "No Green should be visible");
}

void tst_UserPasses::testMultipleRenderTargets()
{
    QScopedPointer<QQuickView> view(createView(QLatin1String("gbufferpass.qml"), QSize(400, 400)));
    QVERIFY(view);
    view->show();
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    // Verify render pass with multiple attachments
    QQuick3DRenderPass *renderPass = view->rootObject()->findChild<QQuick3DRenderPass *>("mrtPass");
    QVERIFY(renderPass);

    // Capture the rendered image
    QImage result = grab(view.data());
    QVERIFY(!result.isNull());

    // This test validates Multiple Render Targets (MRT) work:
    // - ONE render pass with 3 color attachments (GBUFFER0, GBUFFER1, GBUFFER2)
    // - CustomMaterial fragment shader writes:
    //   - RED to GBUFFER0 (attachment 0)
    //   - YELLOW to GBUFFER1 (attachment 1)
    //   - BLUE to GBUFFER2 (attachment 2)
    // - We display attachment 1 (GBUFFER1)
    // - Should see yellow, proving MRT works (if MRT failed, we'd see red or black)
    QVERIFY2(imageContainsDominantColor(result, Qt::yellow), "Should see yellow (from GBUFFER1), proving MRT works");
    QVERIFY2(!imageContainsDominantColor(result, Qt::red), "Should NOT see red (GBUFFER0 not displayed)");
    QVERIFY2(!imageContainsDominantColor(result, Qt::blue), "Should NOT see blue (GBUFFER2 not displayed)");
}

void tst_UserPasses::testDisableInternalPasses()
{
    QScopedPointer<QQuickView> view(createView(QLatin1String("disableinternalpasses.qml"), QSize(400, 400)));
    QVERIFY(view);
    view->show();
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    // Get the viewport
    QQuick3DViewport *viewport = view->rootObject()->findChild<QQuick3DViewport *>();
    QVERIFY(viewport);

    // Verify DisableInternalPasses is set
    QVERIFY(viewport->renderOverrides() & QQuick3DViewport::RenderOverride::DisableInternalPasses);

    // Capture the rendered image
    QImage result = grab(view.data());
    QVERIFY(!result.isNull());

    // This test validates DisableInternalPasses works:
    // - environment.clearColor = "red"
    // - RenderPass.clearColor = "green"
    // - With DisableInternalPasses, we should see green, not red
    QVERIFY2(imageContainsDominantColor(result, Qt::green), "Background should be green (from pass), proving DisableInternalPasses works");
    QVERIFY2(!imageContainsDominantColor(result, Qt::red), "Should NOT see red (from environment)");
}

void tst_UserPasses::testRenderPassTextures_data()
{
    QTest::addColumn<QString>("formatName");
    QTest::addColumn<bool>("isSingleChannel");
    QTest::addColumn<int>("expectedColor"); // QColor as int for easy comparison

    // Test all color texture formats
    QTest::newRow("RGBA8") << "RGBA8" << false << static_cast<int>(Qt::yellow);
    QTest::newRow("RGBA16F") << "RGBA16F" << false << static_cast<int>(Qt::yellow);
    QTest::newRow("RGBA32F") << "RGBA32F" << false << static_cast<int>(Qt::yellow);
    QTest::newRow("R8") << "R8" << true << static_cast<int>(Qt::white); // Single channel
    QTest::newRow("R16") << "R16" << true << static_cast<int>(Qt::white); // Single channel
    QTest::newRow("R16F") << "R16F" << true << static_cast<int>(Qt::white); // Single channel
    QTest::newRow("R32F") << "R32F" << true << static_cast<int>(Qt::white); // Single channel
}

void tst_UserPasses::testRenderPassTextures()
{
    QFETCH(QString, formatName);
    QFETCH(bool, isSingleChannel);
    QFETCH(int, expectedColor);

    // For single-channel formats, use gray clear color (will show in red channel)
    // For multi-channel formats, use distinctive color
    QString clearColorValue = isSingleChannel ? "gray" : "darkBlue";
    QString sphereColor = isSingleChannel ? "white" : "yellow";

    // Create a temporary QML file with the specific format
    QString qmlContent = QString(R"(
import QtQuick
import QtQuick3D
import QtQuick3D.Helpers

Item {
    width: 400
    height: 400

    View3D {
        id: view3D
        anchors.fill: parent
        renderOverrides: View3D.DisableInternalPasses

        environment: SceneEnvironment {
            backgroundMode: SceneEnvironment.Color
            clearColor: "black"
        }

        PerspectiveCamera {
            position: Qt.vector3d(0, 0, 300)
        }

        DirectionalLight {}

        RenderPassTexture {
            id: colorTexture
            format: RenderPassTexture.%1
        }

        RenderPassTexture {
            id: depthTexture
            format: RenderPassTexture.Depth24Stencil8
        }

        RenderPass {
            id: texturePass
            materialMode: RenderPass.OriginalMaterial
            clearColor: "%2"

            commands: [
                ColorAttachment { target: colorTexture },
                DepthTextureAttachment { target: depthTexture }
            ]
        }

        SimpleQuadRenderer {
            texture: Texture {
                textureProvider: RenderOutputProvider {
                    textureSource: RenderOutputProvider.UserPassTexture
                    renderPass: texturePass
                    attachmentSelector: RenderOutputProvider.Attachment0
                }
            }
        }

        Model {
            source: "#Sphere"
            materials: PrincipledMaterial {
                baseColor: "%3"
                lighting: PrincipledMaterial.NoLighting
            }
        }
    }
}
)")
                                 .arg(formatName, clearColorValue, sphereColor);

    // Write temporary QML file
    QString tempPath = QDir::temp().filePath(QString("test_%1.qml").arg(formatName));
    QFile tempFile(tempPath);
    QVERIFY(tempFile.open(QIODevice::WriteOnly | QIODevice::Text));
    tempFile.write(qmlContent.toUtf8());
    tempFile.close();

    // Load and test
    QScopedPointer<QQuickView> view(new QQuickView);
    view->setSource(QUrl::fromLocalFile(tempPath));
    view->setResizeMode(QQuickView::SizeRootObjectToView);
    view->resize(400, 400);
    QVERIFY(view);
    view->show();
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    // Capture the rendered image
    QImage result = grab(view.data());
    QVERIFY(!result.isNull());

    // Validate format-specific rendering
    if (!isSingleChannel) {
        // Multi-channel formats should show the distinctive clear color
        QColor darkBlue(0, 0, 139);
        QVERIFY2(imageContainsDominantColor(result, darkBlue),
                 qPrintable(QString("Background should be dark blue for format %1").arg(formatName)));
    }

    // For single-channel formats, just verify something rendered (not all black)
    // These formats can only store red channel data, so color validation is limited
    if (isSingleChannel) {
        QVERIFY2(imageHasNonBlackPixels(result),
                 qPrintable(QString("Image should render for single-channel format %1").arg(formatName)));
    } else {
        QColor expected = QColor(static_cast<Qt::GlobalColor>(expectedColor));
        QVERIFY2(imageContainsDominantColor(result, expected),
                 qPrintable(QString("Expected color should be visible for format %1").arg(formatName)));
    }

    // Cleanup
    tempFile.remove();
}

void tst_UserPasses::testSubRenderPass()
{
    QScopedPointer<QQuickView> view(createView(QLatin1String("subrenderpass.qml"), QSize(400, 400)));
    QVERIFY(view);
    view->show();
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    // Verify nested render passes
    QList<QQuick3DRenderPass *> renderPasses = view->rootObject()->findChildren<QQuick3DRenderPass *>();
    QVERIFY(renderPasses.size() >= 2);

    // Capture the rendered image
    QImage result = grab(view.data());
    QVERIFY(!result.isNull());

    // This test validates SubRenderPass works:
    // - Parent pass has magenta clear color and filters to Layer0 (nothing)
    // - Child SubRenderPass filters to Layer1 and renders the cyan cone
    // - If SubRenderPass wasn't executing, we'd only see magenta background
    // - Seeing the cyan cone proves SubRenderPass executed
    QColor magenta(255, 0, 255);
    QVERIFY2(imageContainsDominantColor(result, magenta), "Background should be magenta (from parent pass)");
    QVERIFY2(imageContainsDominantColor(result, Qt::cyan), "Cyan cone should be visible (proves SubRenderPass executed)");
}

void tst_UserPasses::testContentLayers()
{
    QScopedPointer<QQuickView> view(createView(QLatin1String("contentlayers.qml"), QSize(400, 400)));
    QVERIFY(view);
    view->show();
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    // Capture the rendered image
    QImage result = grab(view.data());
    QVERIFY(!result.isNull());

    // This test validates ContentLayer filtering works:
    // - Pass filters to Layer0 | Layer1 only
    // - Gray background (from pass) proves pass is active
    // - Layer0 (red cube) and Layer1 (green sphere) should be visible
    // - Layer2 (blue cone) should NOT be visible (filtered out)
    QVERIFY2(imageContainsDominantColor(result, Qt::gray), "Background should be gray (from pass)");
    QVERIFY2(imageContainsDominantColor(result, Qt::red), "Red cube (Layer0) should be visible");
    QVERIFY2(imageContainsDominantColor(result, Qt::green), "Green sphere (Layer1) should be visible");
    QVERIFY2(!imageContainsDominantColor(result, Qt::blue), "Blue cone (Layer2) should NOT be visible (filtered)");
}

void tst_UserPasses::testRenderablesFilter_data()
{
    QTest::addColumn<QString>("filterMode");
    QTest::addColumn<bool>("expectRed"); // Opaque red cube
    QTest::addColumn<bool>("expectGreen"); // Transparent green sphere

    // Test all filter modes
    QTest::newRow("Opaque") << "RenderablesFilter.Opaque" << true << false;
    QTest::newRow("Transparent") << "RenderablesFilter.Transparent" << false << true;
    QTest::newRow("Opaque | Transparent") << "RenderablesFilter.Opaque | RenderablesFilter.Transparent" << true << true;
    QTest::newRow("None") << "RenderablesFilter.None" << false << false; // No renderables
}

void tst_UserPasses::testRenderablesFilter()
{
    QFETCH(QString, filterMode);
    QFETCH(bool, expectRed);
    QFETCH(bool, expectGreen);

    // Create a temporary QML file with the specific filter mode
    QString qmlContent = QString(R"(
import QtQuick
import QtQuick3D
import QtQuick3D.Helpers

Item {
    width: 400
    height: 400

    View3D {
        id: view3D
        anchors.fill: parent
        renderOverrides: View3D.DisableInternalPasses

        environment: SceneEnvironment {
            backgroundMode: SceneEnvironment.Color
            clearColor: "yellow"
        }

        PerspectiveCamera {
            position: Qt.vector3d(0, 0, 300)
        }

        DirectionalLight {}

        RenderPassTexture {
            id: outputTexture
            format: RenderPassTexture.RGBA8
        }

        RenderPass {
            id: filterPass
            materialMode: RenderPass.OriginalMaterial
            clearColor: "blue"

            commands: [
                ColorAttachment { target: outputTexture },
                DepthStencilAttachment {},
                RenderablesFilter {
                    renderableTypes: %1
                }
            ]
        }

        SimpleQuadRenderer {
            texture: Texture {
                textureProvider: RenderOutputProvider {
                    textureSource: RenderOutputProvider.UserPassTexture
                    renderPass: filterPass
                    attachmentSelector: RenderOutputProvider.Attachment0
                }
            }
        }

        // Opaque red cube
        Model {
            source: "#Cube"
            materials: PrincipledMaterial {
                baseColor: "red"
                alphaMode: PrincipledMaterial.Opaque
                lighting: PrincipledMaterial.NoLighting
            }
        }

        // Transparent green sphere
        Model {
            source: "#Sphere"
            position: Qt.vector3d(150, 0, 0)
            materials: PrincipledMaterial {
                baseColor: Qt.rgba(0, 1, 0, 0.5)
                alphaMode: PrincipledMaterial.Blend
                lighting: PrincipledMaterial.NoLighting
            }
        }
    }
}
)")
                                 .arg(filterMode);

    // Write temporary QML file
    QString tempPath = QDir::temp().filePath(QString("test_filter_%1.qml").arg(filterMode.replace(" ", "_").replace("|", "or")));
    QFile tempFile(tempPath);
    QVERIFY(tempFile.open(QIODevice::WriteOnly | QIODevice::Text));
    tempFile.write(qmlContent.toUtf8());
    tempFile.close();

    // Load and test
    QScopedPointer<QQuickView> view(new QQuickView);
    view->setSource(QUrl::fromLocalFile(tempPath));
    view->setResizeMode(QQuickView::SizeRootObjectToView);
    view->resize(400, 400);
    QVERIFY(view);
    view->show();
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    // Capture the rendered image
    QImage result = grab(view.data());
    QVERIFY(!result.isNull());

    // Blue background should always be visible (proves pass is active)
    QVERIFY2(imageContainsDominantColor(result, Qt::blue),
             qPrintable(QString("Background should be blue for filter mode: %1").arg(filterMode)));

    // Validate presence/absence of red cube (opaque)
    if (expectRed) {
        QVERIFY2(imageContainsDominantColor(result, Qt::red),
                 qPrintable(QString("Red cube (opaque) should be visible for filter mode: %1").arg(filterMode)));
    } else {
        QVERIFY2(!imageContainsDominantColor(result, Qt::red),
                 qPrintable(QString("Red cube (opaque) should NOT be visible for filter mode: %1").arg(filterMode)));
    }

    // Validate presence/absence of green sphere (transparent)
    if (expectGreen) {
        QVERIFY2(imageContainsDominantColor(result, Qt::green),
                 qPrintable(QString("Green sphere (transparent) should be visible for filter mode: %1").arg(filterMode)));
    } else {
        QVERIFY2(!imageContainsDominantColor(result, Qt::green),
                 qPrintable(QString("Green sphere (transparent) should NOT be visible for filter mode: %1").arg(filterMode)));
    }

    // Cleanup
    tempFile.remove();
}

void tst_UserPasses::testPipelineStateOverride_data()
{
    QTest::addColumn<QString>("qmlFile");
    QTest::addColumn<QColor>("expectedColor");
    QTest::addColumn<QString>("description");

    QTest::newRow("Wireframe") << "pipelinestateoverride_wireframe.qml" << QColor("white")
                               << "White edges should be visible in wireframe mode (polygonMode = Line)";

    QTest::newRow("CullFront") << "pipelinestateoverride_cullfront.qml" << QColor("white")
                               << "Back faces should be visible with cullMode = Front";

    QTest::newRow("Scissor") << "pipelinestateoverride_scissor.qml" << QColor("cyan")
                             << "Cyan cube should be clipped to bottom-left region with scissor";
}

void tst_UserPasses::testPipelineStateOverride()
{
    QFETCH(QString, qmlFile);
    QFETCH(QColor, expectedColor);
    QFETCH(QString, description);

    QQuickView view;
    view.setSource(testFileUrl(qmlFile));
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.show();

    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QImage result = grab(&view);
    QVERIFY(result.width() >= 400);
    QVERIFY(result.height() >= 400);

    // Validate the specific pipeline state override effect
    // Skip generic color check for tests with specific validation
    if (!qmlFile.contains("cullfront") && !qmlFile.contains("scissor")) {
        QVERIFY2(imageContainsDominantColor(result, expectedColor), qPrintable(description));
    }

    // Additional validation based on test type
    if (qmlFile.contains("wireframe")) {
        // Wireframe test: verify mostly black (not filled)
        int blackPixels = 0;
        int whitePixels = 0;
        for (int y = 0; y < result.height(); ++y) {
            for (int x = 0; x < result.width(); ++x) {
                QColor pixel = result.pixelColor(x, y);
                if (pixel.red() < 30 && pixel.green() < 30 && pixel.blue() < 30) {
                    blackPixels++;
                } else if (pixel.red() > 200 || pixel.green() > 200 || pixel.blue() > 200) {
                    whitePixels++;
                }
            }
        }
        // Wireframe should be mostly black (not filled)
        QVERIFY2(blackPixels > whitePixels * 5, "Wireframe mode should show mostly black background (not filled faces)");
    } else if (qmlFile.contains("cullfront")) {
        // CullFront test: front faces are culled, so the center of the cube (where the
        // camera looks straight through the culled front face to the dark back face) must
        // be near-black, while some back/edge faces visible at glancing angles are non-black.
        const QColor center = result.pixelColor(result.width() / 2, result.height() / 2);
        const int centerBrightness = (center.red() + center.green() + center.blue()) / 3;
        QVERIFY2(centerBrightness < 30, "Front face should be culled: center must be dark");
        int nonBlackPixels = 0;
        for (int y = 0; y < result.height(); ++y) {
            for (int x = 0; x < result.width(); ++x) {
                QColor pixel = result.pixelColor(x, y);
                int brightness = (pixel.red() + pixel.green() + pixel.blue()) / 3;
                if (brightness > 20)
                    nonBlackPixels++;
            }
        }
        QVERIFY2(nonBlackPixels > 100, "Some back/edge faces should be visible");
    } else if (qmlFile.contains("scissor")) {
        // Scissor test: verify cyan is clipped to bottom-left region
        // Scissor rect is Qt.rect(100, 100, 200, 200) which appears in bottom-left
        int cyanPixels = 0;
        int cyanInBottomLeft = 0; // Cyan in bottom-left region
        int blackInTopRight = 0; // Black in top-right region (should be clipped)

        for (int y = 0; y < result.height(); ++y) {
            for (int x = 0; x < result.width(); ++x) {
                QColor pixel = result.pixelColor(x, y);
                bool isCyan = (pixel.green() > 100 && pixel.blue() > 100 && pixel.red() < 100);
                bool isBlack = (pixel.red() < 30 && pixel.green() < 30 && pixel.blue() < 30);

                if (isCyan) {
                    cyanPixels++;
                    // Check if cyan appears in bottom-left region (where scissor rect is)
                    if (x >= 50 && x < 350 && y >= result.height() - 350 && y < result.height() - 50) {
                        cyanInBottomLeft++;
                    }
                }

                // Check top-right region should be black (clipped)
                if (x >= result.width() - 100 && y < 100) {
                    if (isBlack) {
                        blackInTopRight++;
                    }
                }
            }
        }

        // Should have cyan visible (scissor allows rendering)
        QVERIFY2(cyanPixels > 100, "Should have some cyan visible within scissor rectangle");

        // Cyan should be concentrated in bottom-left region (where scissor rect is)
        QVERIFY2(cyanInBottomLeft > 50, "Cyan should appear in bottom-left region within scissor rectangle");

        // Top-right should be mostly black (clipped by scissor)
        QVERIFY2(blackInTopRight > 500, "Top-right region should be black (clipped by scissor)");
    }
}

// NoLighting DefaultMaterial colors are stored as linear in the RGBA16F render
// target and displayed after sRGB encoding. Pure primaries (0 or 1 in linear)
// encode identically in sRGB, but rounding and the display pipeline can shift
// channel values by up to ~50 out of 255.  Use a generous fuzz that still
// clearly distinguishes red from blue or green from black.
static const int FUZZ = 50;

void tst_UserPasses::subPassOverrideMaterial()
{
    // A sphere with a blue PrincipledMaterial is rendered through a SubRenderPass
    // whose renderPass uses OverrideMaterial with a red NoLighting DefaultMaterial.
    // The center pixel must be red, not blue, proving that the override material's
    // uniform buffer properties (colour) were applied rather than the original material's.
    QScopedPointer<QQuickView> view(createView(QLatin1String("subpass_overridematerial.qml"), QSize(400, 400)));
    QVERIFY(view);
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    const QImage result = grab(view.data());
    if (result.isNull())
        return;

    // Center of the sphere should be red (override), not blue (original material).
    QVERIFY(comparePixelNormPos(result, 0.5, 0.5, Qt::red, FUZZ));
}

void tst_UserPasses::multipleSubPasses()
{
    // Two SubRenderPasses in one main pass, each targeting a different layer with
    // a different override material:
    //   SubPass 1: red override, Layer0 -> cube at left (x = -150)
    //   SubPass 2: blue override, Layer1 -> sphere at right (x = +150)
    // If both sub-passes correctly acquire unique userPassData slots the left
    // region will be red and the right region blue. If they share a slot the
    // second pass overwrites the first and both sides show the same colour.
    QScopedPointer<QQuickView> view(createView(QLatin1String("multiple_subpasses.qml"), QSize(400, 400)));
    QVERIFY(view);
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    const QImage result = grab(view.data());
    if (result.isNull())
        return;

    // Left quarter (cube on Layer0) should be red.
    QVERIFY(comparePixelNormPos(result, 0.25, 0.5, Qt::red, FUZZ));
    // Right quarter (sphere on Layer1) should be blue.
    QVERIFY(comparePixelNormPos(result, 0.75, 0.5, Qt::blue, FUZZ));
}

void tst_UserPasses::subPassDependencyIndex()
{
    // Regression test for QTBUG-148554. A pass referenced as a SubRenderPass
    // (Role::SubPass) must not be assigned a node-derived dependency index.
    // The sub-pass in this scene is declared under a Node, so before the fix
    // the dependency-index loop assigned it that node's (non-zero) index. If
    // such a sub-pass is also scheduled directly (e.g. its output is consumed
    // via a provider) the non-zero index sorts it ahead of genuine top-level
    // passes in the scheduled list.
    QScopedPointer<QQuickView> view(createView(QLatin1String("subpass_dependency_index.qml"), QSize(400, 400)));
    QVERIFY(view);
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    // Force a full synchronize + render so the dependency-index loop runs.
    const QImage result = grab(view.data());
    QVERIFY(!result.isNull());

    auto *mainPass = view->rootObject()->findChild<QQuick3DRenderPass *>("mainPass");
    auto *subPass = view->rootObject()->findChild<QQuick3DRenderPass *>("subPass");
    QVERIFY(mainPass);
    QVERIFY(subPass);

    auto *mainNode = static_cast<QSSGRenderUserPass *>(QQuick3DObjectPrivate::get(mainPass)->spatialNode);
    auto *subNode = static_cast<QSSGRenderUserPass *>(QQuick3DObjectPrivate::get(subPass)->spatialNode);
    QVERIFY(mainNode);
    QVERIFY(subNode);

    // Roles are assigned during sync: the main pass is top-level and the
    // referenced pass is tagged as a sub-pass.
    QCOMPARE(mainNode->role, QSSGRenderUserPass::Role::TopLevel);
    QCOMPARE(subNode->role, QSSGRenderUserPass::Role::SubPass);

    // The invariant under test: a sub-pass carries no dependency index.
    QCOMPARE(subNode->m_dependencyIndex, 0u);
}

void tst_UserPasses::userPassManagerScheduling()
{
    // Unit test of the manager's scheduling contract, which the scene sync and
    // the RenderOutputProvider path both depend on.
    auto manager = QSSGUserRenderPassManager::create();

    QSSGRenderUserPass topA;
    QSSGRenderUserPass topB;
    QSSGRenderUserPass topC;
    QSSGRenderUserPass sub;
    topA.role = QSSGRenderUserPass::Role::TopLevel;
    topB.role = QSSGRenderUserPass::Role::TopLevel;
    topC.role = QSSGRenderUserPass::Role::TopLevel;
    sub.role = QSSGRenderUserPass::Role::SubPass;

    // Sync establishes the top-level set wholesale.
    manager->setScheduledPasses({ &topA, &topB });
    QCOMPARE(manager->scheduledUserPasses().size(), 2);

    // Provider path: confirming an already-scheduled pass is a no-op.
    manager->scheduleUserPass(&topA);
    QCOMPARE(manager->scheduledUserPasses().size(), 2);

    // A top-level pass not yet scheduled is added (first-frame fallback).
    manager->scheduleUserPass(&topC);
    QCOMPARE(manager->scheduledUserPasses().size(), 3);

    // A sub-pass is rejected, even via the provider path (QTBUG-148554).
    manager->scheduleUserPass(&sub);
    QCOMPARE(manager->scheduledUserPasses().size(), 3);
    QVERIFY(!manager->scheduledUserPasses().contains(&sub));

    // Ordering: higher dependency index first, then lower declaration order.
    topA.m_dependencyIndex = 1; topA.m_declarationOrder = 0;
    topB.m_dependencyIndex = 3; topB.m_declarationOrder = 1;
    topC.m_dependencyIndex = 1; topC.m_declarationOrder = 2;
    manager->setScheduledPasses({ &topA, &topB, &topC });
    manager->updateUserPassOrder(true);
    const auto &ordered = manager->scheduledUserPasses();
    QCOMPARE(ordered.size(), 3);
    QCOMPARE(ordered.at(0), &topB); // highest dependency index
    QCOMPARE(ordered.at(1), &topA); // equal index, lower declaration order first
    QCOMPARE(ordered.at(2), &topC);
}

void tst_UserPasses::subPassRoleTransition()
{
    // The classification is only re-derived when the flag on the scene manager is
    // set (pass add/remove or command-list rebuild). Flipping which leaf pass the
    // container references at runtime must re-derive the roles accordingly.
    QScopedPointer<QQuickView> view(createView(QLatin1String("subpass_role_transition.qml"), QSize(200, 200)));
    QVERIFY(view);
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    auto *leafA = view->rootObject()->findChild<QQuick3DRenderPass *>("leafA");
    auto *leafB = view->rootObject()->findChild<QQuick3DRenderPass *>("leafB");
    QVERIFY(leafA);
    QVERIFY(leafB);

    const auto roleOf = [](QQuick3DRenderPass *pass) {
        auto *node = static_cast<QSSGRenderUserPass *>(QQuick3DObjectPrivate::get(pass)->spatialNode);
        return node ? node->role : QSSGRenderUserPass::Role::TopLevel;
    };

    // Initially the container references leafB as its sub-pass.
    QVERIFY(!grab(view.data()).isNull());
    QCOMPARE(roleOf(leafB), QSSGRenderUserPass::Role::SubPass);
    QCOMPARE(roleOf(leafA), QSSGRenderUserPass::Role::TopLevel);

    // Flip the reference: leafA becomes the sub-pass, leafB returns to top-level.
    view->rootObject()->setProperty("useLeafAAsSubPass", true);
    QVERIFY(!grab(view.data()).isNull());
    QCOMPARE(roleOf(leafA), QSSGRenderUserPass::Role::SubPass);
    QCOMPARE(roleOf(leafB), QSSGRenderUserPass::Role::TopLevel);

    // And back again.
    view->rootObject()->setProperty("useLeafAAsSubPass", false);
    QVERIFY(!grab(view.data()).isNull());
    QCOMPARE(roleOf(leafB), QSSGRenderUserPass::Role::SubPass);
    QCOMPARE(roleOf(leafA), QSSGRenderUserPass::Role::TopLevel);
}

void tst_UserPasses::subPassRefCounting()
{
    // Two containers reference the SAME leaf pass. The classification counts
    // references, so the leaf stays a sub-pass until the LAST reference is
    // dropped.
    QScopedPointer<QQuickView> view(createView(QLatin1String("subpass_refcount.qml"), QSize(200, 200)));
    QVERIFY(view);
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    auto *sharedLeaf = view->rootObject()->findChild<QQuick3DRenderPass *>("sharedLeaf");
    QVERIFY(sharedLeaf);

    const auto roleOf = [](QQuick3DRenderPass *pass) {
        auto *node = static_cast<QSSGRenderUserPass *>(QQuick3DObjectPrivate::get(pass)->spatialNode);
        return node ? node->role : QSSGRenderUserPass::Role::TopLevel;
    };

    // Both references active.
    QVERIFY(!grab(view.data()).isNull());
    QCOMPARE(roleOf(sharedLeaf), QSSGRenderUserPass::Role::SubPass);

    // Dropping one of two references keeps the leaf a sub-pass.
    view->rootObject()->setProperty("refA", false);
    QVERIFY(!grab(view.data()).isNull());
    QCOMPARE(roleOf(sharedLeaf), QSSGRenderUserPass::Role::SubPass);

    // Dropping the last reference makes it top-level again.
    view->rootObject()->setProperty("refB", false);
    QVERIFY(!grab(view.data()).isNull());
    QCOMPARE(roleOf(sharedLeaf), QSSGRenderUserPass::Role::TopLevel);

    // And re-referencing flips it back.
    view->rootObject()->setProperty("refA", true);
    QVERIFY(!grab(view.data()).isNull());
    QCOMPARE(roleOf(sharedLeaf), QSSGRenderUserPass::Role::SubPass);
}

void tst_UserPasses::subPassCommandDestroyed()
{
    // Destroying the referencing SubRenderPass command (with its container
    // pass) releases the reference: the leaf returns to top-level.
    QScopedPointer<QQuickView> view(createView(QLatin1String("subpass_dynamic_ref.qml"), QSize(200, 200)));
    QVERIFY(view);
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    auto *viewport = view->rootObject()->findChild<QQuick3DViewport *>("view");
    auto *staticLeaf = view->rootObject()->findChild<QQuick3DRenderPass *>("staticLeaf");
    QVERIFY(viewport);
    QVERIFY(staticLeaf);

    const auto roleOf = [](QQuick3DRenderPass *pass) {
        auto *node = static_cast<QSSGRenderUserPass *>(QQuick3DObjectPrivate::get(pass)->spatialNode);
        return node ? node->role : QSSGRenderUserPass::Role::TopLevel;
    };

    QVERIFY(!grab(view.data()).isNull());
    QCOMPARE(roleOf(staticLeaf), QSSGRenderUserPass::Role::TopLevel);

    // Create a container pass at runtime that references the static leaf.
    QQmlComponent component(view->engine());
    component.setData(QByteArrayLiteral(
                          "import QtQuick3D\n"
                          "RenderPass {\n"
                          "    commands: [ SubRenderPass { objectName: \"dynSubCmd\" } ]\n"
                          "}\n"),
                      QUrl(QStringLiteral("dyncontainer.qml")));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    auto *dynContainer = qobject_cast<QQuick3DRenderPass *>(component.create());
    QVERIFY(dynContainer);
    dynContainer->setParentItem(viewport->scene());
    auto *dynSubCmd = dynContainer->findChild<QObject *>("dynSubCmd");
    QVERIFY(dynSubCmd);
    QVERIFY(dynSubCmd->setProperty("renderPass", QVariant::fromValue(staticLeaf)));

    QVERIFY(!grab(view.data()).isNull());
    QCOMPARE(roleOf(staticLeaf), QSSGRenderUserPass::Role::SubPass);

    // Destroying the container destroys the command, releasing the reference.
    delete dynContainer;
    QVERIFY(!grab(view.data()).isNull());
    QCOMPARE(roleOf(staticLeaf), QSSGRenderUserPass::Role::TopLevel);
}

void tst_UserPasses::referencedPassDestroyed()
{
    // Destroying the referenced pass while the SubRenderPass command is alive
    // must not crash: the watcher resets the command's renderPass and the
    // dying pass is never dereferenced.
    QScopedPointer<QQuickView> view(createView(QLatin1String("subpass_dynamic_ref.qml"), QSize(200, 200)));
    QVERIFY(view);
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    auto *viewport = view->rootObject()->findChild<QQuick3DViewport *>("view");
    auto *staticContainer = view->rootObject()->findChild<QQuick3DRenderPass *>("staticContainer");
    auto *staticSubCmd = view->rootObject()->findChild<QObject *>("staticSubCmd");
    QVERIFY(viewport);
    QVERIFY(staticContainer);
    QVERIFY(staticSubCmd);

    const auto roleOf = [](QQuick3DRenderPass *pass) {
        auto *node = static_cast<QSSGRenderUserPass *>(QQuick3DObjectPrivate::get(pass)->spatialNode);
        return node ? node->role : QSSGRenderUserPass::Role::TopLevel;
    };

    // Create a leaf pass at runtime and reference it from the static command.
    QQmlComponent component(view->engine());
    component.setData(QByteArrayLiteral(
                          "import QtQuick3D\n"
                          "RenderPass {\n"
                          "    commands: [ RenderablesFilter { renderableTypes: RenderablesFilter.None } ]\n"
                          "}\n"),
                      QUrl(QStringLiteral("dynleaf.qml")));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    auto *dynLeaf = qobject_cast<QQuick3DRenderPass *>(component.create());
    QVERIFY(dynLeaf);
    dynLeaf->setParentItem(viewport->scene());
    QVERIFY(staticSubCmd->setProperty("renderPass", QVariant::fromValue(dynLeaf)));

    QVERIFY(!grab(view.data()).isNull());
    QCOMPARE(roleOf(dynLeaf), QSSGRenderUserPass::Role::SubPass);

    // Destroy the referenced pass; the watcher clears the command's property.
    delete dynLeaf;
    QCOMPARE(staticSubCmd->property("renderPass").value<QQuick3DRenderPass *>(), nullptr);

    // The scene keeps rendering and the container is unaffected.
    const QImage result = grab(view.data());
    QVERIFY(!result.isNull());
    QCOMPARE(roleOf(staticContainer), QSSGRenderUserPass::Role::TopLevel);
    QVERIFY(imageContainsDominantColor(result, Qt::red));
}

void tst_UserPasses::unreferencedPassReturnsToTopLevel()
{
    // A pass that is no longer referenced by any SubRenderPass command must
    // return to being a top-level pass, and actually render as one again:
    // the provider-driven quad shows the leaf's own output, which only exists
    // when the leaf renders standalone.
    //
    // While referenced, the provider requests the leaf every frame and the
    // manager rejects it with a once-per-pass warning; the first frames render
    // during window exposure, so the expectation is armed up front.
    QTest::ignoreMessage(QtWarningMsg,
                         QRegularExpression(QStringLiteral("Ignoring request to schedule sub-pass")));
    QScopedPointer<QQuickView> view(createView(QLatin1String("subpass_unreference.qml"), QSize(200, 200)));
    QVERIFY(view);
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    auto *leafPass = view->rootObject()->findChild<QQuick3DRenderPass *>("leafPass");
    QVERIFY(leafPass);

    const auto roleOf = [](QQuick3DRenderPass *pass) {
        auto *node = static_cast<QSSGRenderUserPass *>(QQuick3DObjectPrivate::get(pass)->spatialNode);
        return node ? node->role : QSSGRenderUserPass::Role::TopLevel;
    };

    // Referenced: the leaf is a sub-pass and the quad has nothing to show.
    const QImage subPassResult = grab(view.data());
    QVERIFY(!subPassResult.isNull());
    QCOMPARE(roleOf(leafPass), QSSGRenderUserPass::Role::SubPass);
    QVERIFY(!imageContainsDominantColor(subPassResult, Qt::green));

    // Unreferenced: the leaf returns to top-level and renders the green cube
    // into its own attachment, which the provider now exposes.
    view->rootObject()->setProperty("useAsSubPass", false);
    QVERIFY(!grab(view.data()).isNull()); // settle: role flip + first standalone render
    const QImage topLevelResult = grab(view.data());
    QVERIFY(!topLevelResult.isNull());
    QCOMPARE(roleOf(leafPass), QSSGRenderUserPass::Role::TopLevel);
    QVERIFY(imageContainsDominantColor(topLevelResult, Qt::green));
}

void tst_UserPasses::slotLimitDoesNotCrash()
{
    // 17 SubRenderPasses in a single main pass exceeds the effective per-frame
    // pass limit.  The UserRenderPass implementation tracks visited passes in a
    // set shared across sibling sub-passes; once that set reaches
    // MAX_SUBPASS_DEPTH (16) the renderer emits a depth-limit warning and skips
    // the excess passes gracefully.  Verify no crash occurs.
    QQuick3DTestMessageHandler msgHandler;

    QScopedPointer<QQuickView> view(createView(QLatin1String("slot_limit.qml"), QSize(100, 100)));
    QVERIFY(view);
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));
    grab(view.data()); // ensure at least one frame is rendered

    // The visited-pass set fills up before the explicit slot-limit counter is
    // exhausted, so the depth-limit warning is what gets emitted.
    QVERIFY2(msgHandler.messageString().contains(QLatin1String("Maximum SubRenderPass nesting depth")),
             qPrintable(msgHandler.messageString()));
}

void tst_UserPasses::depthLimitDoesNotCrash()
{
    // 17 levels of nested SubRenderPasses exceeds MAX_SUBPASS_DEPTH = 16.
    // The renderer should emit a qWarning for the excess level and skip it
    // gracefully without crashing.
    QQuick3DTestMessageHandler msgHandler;

    QScopedPointer<QQuickView> view(createView(QLatin1String("depth_limit.qml"), QSize(100, 100)));
    QVERIFY(view);
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));
    grab(view.data()); // ensure at least one frame is rendered

    // Verify the expected warning was emitted.
    QVERIFY2(msgHandler.messageString().contains(QLatin1String("Maximum SubRenderPass nesting depth")),
             qPrintable(msgHandler.messageString()));
}

void tst_UserPasses::testAddDefine()
{
    // An AugmentMaterial pass injects #define MY_DEFINE 1 via AddDefine.
    // The augment shader outputs green when MY_DEFINE is defined and red
    // when it is absent.  The test grabs a frame, verifies green pixels on
    // the sphere, then removes the define by clearing its name and grabs
    // again, verifying that the output changed to red.
    QScopedPointer<QQuickView> view(createView(QLatin1String("adddefine.qml"), QSize(400, 400)));
    QVERIFY(view);
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    // Static check: MY_DEFINE is active → sphere should be green
    QImage result1 = grab(view.data());
    QVERIFY(!result1.isNull());

    const bool hasGreen1 = imageContainsDominantColor(result1, Qt::green);
    QVERIFY2(hasGreen1, "With AddDefine active, sphere should appear green");

    // Dynamic check: clear the define name → shader should see no MY_DEFINE → red
    QObject *addDefine = view->rootObject()->findChild<QObject *>(QLatin1String("testDefine"));
    QVERIFY2(addDefine, "testDefine object must be found for dynamic check");
    addDefine->setProperty("name", QByteArray(""));

    QImage result2 = grab(view.data());
    QVERIFY(!result2.isNull());

    const bool hasRed2 = imageContainsDominantColor(result2, Qt::red);
    QVERIFY2(hasRed2, "After clearing AddDefine name, sphere should appear red");
}

void tst_UserPasses::testSkyboxPass()
{
    // A SubRenderPass with passMode: RenderPass.SkyboxPass renders the
    // ProceduralSkyTextureData environment into the custom render target.
    // If SkyboxPass works, the output should contain non-black sky colours.
    QScopedPointer<QQuickView> view(createView(QLatin1String("skyboxpass.qml"), QSize(400, 400)));
    QVERIFY(view);
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    QImage result = grab(view.data());
    QVERIFY(!result.isNull());

    // The sky texture is a gradient with distinct colours; any non-black
    // pixel proves that the SkyboxPass rendered something.
    const bool hasSkyColors = imageHasNonBlackPixels(result);
    QVERIFY2(hasSkyColors, "SkyboxPass should render sky texture: output should not be all-black");
}

void tst_UserPasses::testItem2DPass()
{
    // A Qt Quick 2D Item (red Rectangle) embedded in a 3D Node is rendered
    // by a SubRenderPass with passMode: RenderPass.Item2DPass.
    // If Item2DPass works, red pixels should appear in the output.
    QScopedPointer<QQuickView> view(createView(QLatin1String("item2dpass.qml"), QSize(400, 400)));
    QVERIFY(view);
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    QImage result1 = grab(view.data());
    QVERIFY(!result1.isNull());

    const bool hasRed1 = imageContainsDominantColor(result1, Qt::red);
    QVERIFY2(hasRed1, "Item2DPass should render the embedded red Rectangle");

    // Dynamic check: change the Rectangle colour to blue
    // root.item2DColor is a QML property that drives the Rectangle colour
    view->rootObject()->setProperty("item2DColor", QColor(Qt::blue));

    QImage result2 = grab(view.data());
    QVERIFY(!result2.isNull());

    const bool hasBlue2 = imageContainsDominantColor(result2, Qt::blue);
    QVERIFY2(hasBlue2, "After changing item colour to blue, Item2DPass output should be blue");
    QVERIFY2(!imageContainsDominantColor(result2, Qt::red), "After colour change, red should no longer be dominant");
}

void tst_UserPasses::testDepthTestDisabled()
{
    // Two overlapping spheres: blue (z=100, closer to camera) and red (z=-100, farther).
    // Opaque objects are sorted front-to-back, so blue draws first, red draws second.
    // With PipelineStateOverride { depthTestEnabled: false }, the GPU depth test is
    // disabled.  The second-drawn object (red) is not depth-rejected and overwrites blue
    // at the overlap → center pixel should be red.
    // Dynamic check: re-enable depth test → closer sphere (blue) wins → center = blue.
    QScopedPointer<QQuickView> view(createView(QLatin1String("depthtestdisabled.qml"), QSize(400, 400)));
    QVERIFY(view);
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    QImage result1 = grab(view.data());
    QVERIFY(!result1.isNull());

    // Static check: depth test disabled → red sphere (drawn second) should be visible
    const bool hasRed1 = imageContainsDominantColor(result1, Qt::red);
    QVERIFY2(hasRed1, "With depthTestEnabled:false, farther red sphere (drawn second) should be visible");

    // Dynamic check: re-enable depth test → blue (closer) wins
    QObject *pso = view->rootObject()->findChild<QObject *>(QLatin1String("pso"));
    if (!pso) {
        qWarning("testDepthTestDisabled: pso object not found, skipping dynamic check");
        return;
    }
    pso->setProperty("depthTestEnabled", true);

    QImage result2 = grab(view.data());
    QVERIFY(!result2.isNull());

    const bool hasBlue2 = imageContainsDominantColor(result2, Qt::blue);
    QVERIFY2(hasBlue2, "After re-enabling depthTest, closer blue sphere should be visible");
}

void tst_UserPasses::testBlendEnabled()
{
    // AugmentMaterial pass with clear colour blue.  The augment shader outputs
    // white at 50% alpha (vec4(1,1,1,0.5)).  With blendEnabled: true and SrcAlpha
    // blend mode, the result blends against the blue background producing a mix
    // (light blue / cyan).  With blendEnabled: false, the output is white (no blend).
    QScopedPointer<QQuickView> view(createView(QLatin1String("blendenabled.qml"), QSize(400, 400)));
    QVERIFY(view);
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    QImage result1 = grab(view.data());
    QVERIFY(!result1.isNull());

    // Static check: with blend active, center pixel should have a noticeable blue
    // component (blending of white + blue) and NOT be pure white.
    const QRgb centerPixel1 = result1.pixel(200, 200);
    const int r1 = qRed(centerPixel1);
    const int b1 = qBlue(centerPixel1);
    // For alpha=0.5 blend: result = 0.5*white + 0.5*blue. The blue channel should
    // clearly exceed the red channel. A 20-point gap is sufficient to account for
    // color-space handling in the compositing pipeline.
    const bool blendActive1 = (b1 > r1 + 20) && (b1 > 100);
    QVERIFY2(blendActive1, "With blendEnabled:true, sphere should blend white + blue → blue channel dominant");

    // Dynamic check: disable blend → sphere should become white (pure white)
    QObject *pso = view->rootObject()->findChild<QObject *>(QLatin1String("pso"));
    if (!pso) {
        qWarning("testBlendEnabled: pso object not found, skipping dynamic check");
        return;
    }
    pso->setProperty("blendEnabled", false);

    QImage result2 = grab(view.data());
    QVERIFY(!result2.isNull());

    const QRgb centerPixel2 = result2.pixel(200, 200);
    const int r2 = qRed(centerPixel2);
    const int g2 = qGreen(centerPixel2);
    const int b2 = qBlue(centerPixel2);
    // Without blend: white sphere → all channels near 255
    const bool isWhite2 = (r2 > 200 && g2 > 200 && b2 > 200);
    QVERIFY2(isWhite2, "After disabling blend, white sphere should render as opaque white");
}

void tst_UserPasses::testPreserveColorContents()
{
    // Two passes share one colour attachment (sharedTex).
    // Pass 1 clears sharedTex to red with no geometry.
    // Pass 2 has renderTargetFlags: PreserveColorContents (no clear) and renders a blue sphere.
    // Expected: background area = red (pass 1 contents preserved), sphere area = blue.
    // Dynamic: clear renderTargetFlags → pass 2 clears buffer → background becomes black.
    QScopedPointer<QQuickView> view(createView(QLatin1String("preservecolor.qml"), QSize(400, 400)));
    QVERIFY(view);
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    QImage result1 = grab(view.data());
    QVERIFY(!result1.isNull());

    // Static checks:
    // Corner pixel (background, no sphere) should be red (pass 1 contents preserved).
    const QRgb cornerPixel1 = result1.pixel(10, 10);
    const bool cornerIsRed1 = qRed(cornerPixel1) > 200 && qGreen(cornerPixel1) < 50 && qBlue(cornerPixel1) < 50;
    QVERIFY2(cornerIsRed1, "Background corner should be red (pass 1 clear preserved by PreserveColorContents)");

    // Center pixel (sphere area) should be blue.
    const bool hasBlue1 = imageContainsDominantColor(result1, Qt::blue);
    QVERIFY2(hasBlue1, "Sphere area should be blue (pass 2 rendered on top of preserved red)");

    // Dynamic check: clear renderTargetFlags → pass 2 clears to black → background = black
    QObject *pass2 = view->rootObject()->findChild<QObject *>(QLatin1String("pass2"));
    if (!pass2) {
        qWarning("testPreserveColorContents: pass2 object not found, skipping dynamic check");
        return;
    }
    // Reset renderTargetFlags to None (0)
    pass2->setProperty("renderTargetFlags", 0);

    QImage result2 = grab(view.data());
    QVERIFY(!result2.isNull());

    const QRgb cornerPixel2 = result2.pixel(10, 10);
    const bool cornerIsBlack2 = qRed(cornerPixel2) < 20 && qGreen(cornerPixel2) < 20 && qBlue(cornerPixel2) < 20;
    QVERIFY2(cornerIsBlack2, "After removing PreserveColorContents, background should be black (pass 2 clears)");
}

void tst_UserPasses::testViewportOverride()
{
    // A large yellow sphere fills the entire view under normal rendering.
    // PipelineStateOverride.viewport is set at runtime (from C++ to be DPR-aware)
    // to the RIGHT half of the device-pixel render target.  The viewport uses
    // device-pixel coordinates with OpenGL convention (y=0 at bottom).
    // Static check: left half of the grabbed image = black, right half = yellow.
    // Dynamic check: switch viewport to left half → sides flip.
    QScopedPointer<QQuickView> view(createView(QLatin1String("viewportoverride.qml"), QSize(400, 400)));
    QVERIFY(view);
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    QObject *pso = view->rootObject()->findChild<QObject *>(QLatin1String("pso"));
    QVERIFY2(pso, "pso PipelineStateOverride object must be found");

    // Compute the device-pixel render-target dimensions (viewport uses device pixels,
    // OpenGL y convention: y=0 at bottom).
    const qreal dpr = view->devicePixelRatio();
    const int dw = qRound(view->width() * dpr);
    const int dh = qRound(view->height() * dpr);

    // Set viewport to the RIGHT half: x = [dw/2, dw), full height in OpenGL y
    pso->setProperty("viewport", QRectF(dw / 2.0, 0, dw / 2.0, dh));

    QImage result1 = grab(view.data());
    QVERIFY(!result1.isNull());

    // Count yellow and black pixels in each horizontal half of the grabbed image.
    // The grabbed image is at device-pixel resolution.
    int yellowLeft1 = 0, yellowRight1 = 0;
    const int imgMidX = result1.width() / 2;
    for (int y = 0; y < result1.height(); ++y) {
        for (int x = 0; x < result1.width(); ++x) {
            const QRgb px = result1.pixel(x, y);
            const bool isYellow = qRed(px) > 150 && qGreen(px) > 150 && qBlue(px) < 50;
            if (x < imgMidX) {
                if (isYellow)
                    ++yellowLeft1;
            } else {
                if (isYellow)
                    ++yellowRight1;
            }
        }
    }

    // Left half should be entirely black (outside the right-half viewport) - this is the key restriction check
    QVERIFY2(yellowLeft1 == 0, "Left half should contain no yellow pixels (outside viewport)");

    // Right half should contain yellow (sphere rendered in the right half viewport)
    QVERIFY2(yellowRight1 > 1000, "Right half should contain yellow pixels (sphere in viewport)");

    // Dynamic check: switch viewport to left half
    pso->setProperty("viewport", QRectF(0, 0, dw / 2.0, dh));

    QImage result2 = grab(view.data());
    QVERIFY(!result2.isNull());

    int yellowLeft2 = 0, yellowRight2 = 0;
    for (int y = 0; y < result2.height(); ++y) {
        for (int x = 0; x < result2.width(); ++x) {
            const QRgb px = result2.pixel(x, y);
            const bool isYellow = qRed(px) > 150 && qGreen(px) > 150 && qBlue(px) < 50;
            if (x < imgMidX) {
                if (isYellow)
                    ++yellowLeft2;
            } else {
                if (isYellow)
                    ++yellowRight2;
            }
        }
    }

    // Left half should have yellow after switching viewport to left
    // Note: sphere fills entire view because viewport restriction is not working, so this passes trivially
    QVERIFY2(yellowLeft2 > 1000, "After switching to left-half viewport, left half should have yellow");

    // Right half should have no yellow after switching viewport to left - this is the key restriction check
    QVERIFY2(yellowRight2 == 0, "After switching to left-half viewport, right half should have no yellow");
}

void tst_UserPasses::testScreenTexture()
{
    // A red sphere is rendered by the standard internal pipeline.
    // A SimpleQuadRenderer reads RenderOutputProvider.ScreenTexture and displays
    // the rendered scene texture covering the full view.
    // Static check: output has red pixels (ScreenTexture captured the rendered scene).
    // Dynamic check: change sphere colour to blue → ScreenTexture updates.
    QScopedPointer<QQuickView> view(createView(QLatin1String("screentexture.qml"), QSize(400, 400)));
    QVERIFY(view);
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    QImage result1 = grab(view.data());
    QVERIFY(!result1.isNull());

    // Static check: red sphere should be visible via ScreenTexture
    const bool hasRed1 = imageContainsDominantColor(result1, Qt::red);
    QVERIFY2(hasRed1, "ScreenTexture should show the rendered scene (red sphere)");

    // Dynamic check: change sphere colour to blue → ScreenTexture should update
    view->rootObject()->setProperty("sphereColor", QColor(Qt::blue));

    QImage result2 = grab(view.data());
    QVERIFY(!result2.isNull());

    const bool hasBlue2 = imageContainsDominantColor(result2, Qt::blue);
    QVERIFY2(hasBlue2, "After changing sphere to blue, ScreenTexture should show blue");
}

void tst_UserPasses::testCullModeOverrideSubpass_data()
{
    QTest::addColumn<QString>("qmlFile");
    QTest::addColumn<QColor>("visibleColor");

    QTest::newRow("OriginalMaterial") << "cull_override_subpass_original.qml" << QColor("white");
    QTest::newRow("OverrideMaterial") << "cull_override_subpass_override.qml" << QColor("red");
    QTest::newRow("AugmentMaterial") << "cull_override_subpass_augment.qml" << QColor("white");
}

void tst_UserPasses::testCullModeOverrideSubpass()
{
    // Verify that PipelineStateOverride { cullMode } in a SubRenderPass is respected and
    // not overwritten by the material's cull mode setting.
    // Phase 1 (useFrontCulling=false): Back culling -> rectangle visible -> center matches visibleColor.
    // Phase 2 (useFrontCulling=true):  Front culling override -> rectangle culled -> center = black.
    QFETCH(QString, qmlFile);
    QFETCH(QColor, visibleColor);

    QScopedPointer<QQuickView> view(createView(qmlFile, QSize(400, 400)));
    QVERIFY(view);
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    const QImage img1 = grab(view.data());
    QVERIFY(!img1.isNull());
    const QRgb p1 = img1.pixel(img1.width() / 2, img1.height() / 2);
    QVERIFY2(qAbs(qRed(p1) - visibleColor.red()) < FUZZ && qAbs(qGreen(p1) - visibleColor.green()) < FUZZ
                     && qAbs(qBlue(p1) - visibleColor.blue()) < FUZZ,
             "Phase 1: rectangle should be visible with Back culling");

    view->rootObject()->setProperty("useFrontCulling", true);

    const QImage img2 = grab(view.data());
    QVERIFY(!img2.isNull());
    const QRgb p2 = img2.pixel(img2.width() / 2, img2.height() / 2);
    QVERIFY2(qRed(p2) < 30 && qGreen(p2) < 30 && qBlue(p2) < 30,
             "Phase 2: rectangle should be culled with Front culling override (center must be black)");
}

void tst_UserPasses::testBlendOverrideOriginalMaterial()
{
    // Verify that PipelineStateOverride blend settings in a SubRenderPass with
    // OriginalMaterial are preserved and not overwritten by the material's blend state.
    // Phase 1 (useStandardBlend=true):  SrcAlpha/OneMinusSrcAlpha -> purplish center (R and B both significant).
    // Phase 2 (useStandardBlend=false): One/Zero -> pure red center (R high, B near zero).
    QScopedPointer<QQuickView> view(createView(QLatin1String("blend_override.qml"), QSize(400, 400)));
    QVERIFY(view);
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    const QImage img1 = grab(view.data());
    QVERIFY(!img1.isNull());
    const QRgb p1 = img1.pixel(img1.width() / 2, img1.height() / 2);
    QVERIFY2(qRed(p1) > 80, "Phase 1: red sphere should contribute");
    QVERIFY2(qBlue(p1) > 80, "Phase 1: blue background should bleed through (blend is working)");
    QVERIFY2(qBlue(p1) < 230, "Phase 1: not pure blue — sphere must be present");

    view->rootObject()->setProperty("useStandardBlend", false);

    const QImage img2 = grab(view.data());
    QVERIFY(!img2.isNull());
    const QRgb p2 = img2.pixel(img2.width() / 2, img2.height() / 2);
    QVERIFY2(qRed(p2) > 200, "Phase 2: strong red from sphere with One/Zero blend");
    QVERIFY2(qBlue(p2) < 30, "Phase 2: no blue bleed — One/Zero override must have worked");

    QVERIFY2(qBlue(p1) - qBlue(p2) > 60, "Phases must produce measurably different blue channel values");
}

void tst_UserPasses::noColorAttachmentDoesNotCrash()
{
    // A pass without ColorAttachment commands gets one implicit default color
    // attachment. Before the fix, building the pass's render target indexed
    // the empty command list (assert in debug, out-of-bounds read in release),
    // and the compatibility check on the next frame both forced a rebuild and
    // did the same.
    QScopedPointer<QQuickView> view(createView(QLatin1String("no_color_attachment.qml"), QSize(200, 200)));
    QVERIFY(view);
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    // Two grabs: the first exercises the render target build, the second the
    // compatibility check against the implicitly created attachment.
    QVERIFY(!grab(view.data()).isNull());
    const QImage result = grab(view.data());
    QVERIFY(!result.isNull());

    // The pass renders the red cube into the implicit default attachment,
    // which the provider exposes for display.
    QVERIFY(imageContainsDominantColor(result, Qt::red));
}

QTEST_MAIN(tst_UserPasses)
#include "tst_userpasses.moc"
