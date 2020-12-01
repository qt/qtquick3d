/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QTest>
#include <QQuickWindow>
#include <QQuickRenderControl>
#include <QQuickRenderTarget>
#include <QQuickGraphicsDevice>
#include <QQuickItem>
#include <QQmlEngine>
#include <QQmlComponent>

#if QT_CONFIG(vulkan)
#include <QVulkanInstance>
#endif

#include "../shared/util.h"

class tst_RenderControl : public QQuick3DDataTest
{
    Q_OBJECT

private slots:
    void initTestCase() override;
    void cube();
    void textureSourceItem();

private:
    bool initRenderer(QQuick3DTestOffscreenRenderer *renderer, const char *filename);
#if QT_CONFIG(vulkan)
    QVulkanInstance vulkanInstance;
#endif
};

void tst_RenderControl::initTestCase()
{
    QQuick3DDataTest::initTestCase();
    if (!initialized())
        return;

#if QT_CONFIG(vulkan)
    vulkanInstance.setLayers({ "VK_LAYER_LUNARG_standard_validation" });
    vulkanInstance.create(); // may fail, which is fine is Vulkan is not used in the first place
#endif
}

bool tst_RenderControl::initRenderer(QQuick3DTestOffscreenRenderer *renderer, const char *filename)
{
    const bool initSuccess = renderer->init(testFileUrl(QString::fromLatin1(filename)),
#if QT_CONFIG(vulkan)
                                     &vulkanInstance
#else
                                     nullptr
#endif
        );
    return initSuccess;
}

const int FUZZ = 5;

void tst_RenderControl::cube()
{
    QQuick3DTestOffscreenRenderer renderer;
    QVERIFY(initRenderer(&renderer, "cube_with_size.qml"));

    renderer.renderControl->polishItems();
    renderer.renderControl->beginFrame();
    renderer.renderControl->sync();
    renderer.renderControl->render();
    bool readCompleted = false;
    QRhiReadbackResult readResult;
    QImage result;
    renderer.enqueueReadback(&readCompleted, &readResult, &result);
    renderer.renderControl->endFrame();

    QVERIFY(readCompleted);
    QVERIFY(!result.isNull());
    QCOMPARE(result.size(), QSize(640, 480)); // no trouble with scale factors since it's all offscreen

    QVERIFY(comparePixel(result, 50, 50, 1, Qt::black, FUZZ));

    // front face of the cube is lighter gray-ish
    QVERIFY(comparePixelNormPos(result, 0.5, 0.5, QColor::fromRgb(239, 239, 239), FUZZ));

    // the top is darker
    QVERIFY(comparePixelNormPos(result, 0.5, 0.375, QColor::fromRgb(181, 181, 181), FUZZ));
}

void tst_RenderControl::textureSourceItem()
{
    QQuick3DTestOffscreenRenderer renderer;
    QVERIFY(initRenderer(&renderer, "sourceitem.qml"));

    // We do expect a perfect frame, meaning that the 2D content in
    // the textures must be rendered in the first frame and should not
    // be deferred. Unlike with an on-screen window, we can verify
    // this in a robust manner here because we are in full control of
    // when exactly a new frame is generated.

    // Frame 1
    renderer.renderControl->polishItems();
    renderer.renderControl->beginFrame();
    renderer.renderControl->sync();
    renderer.renderControl->render();
    bool readCompleted = false;
    QRhiReadbackResult readResult;
    QImage result;
    renderer.enqueueReadback(&readCompleted, &readResult, &result);
    renderer.renderControl->endFrame();
    QVERIFY(readCompleted);
    QCOMPARE(result.size(), QSize(640, 480));

    // from left to right: red, green, blue
    QVERIFY(comparePixelNormPos(result, 0.275, 0.5, QColor::fromRgb(255, 0, 0), FUZZ));
    QVERIFY(comparePixelNormPos(result, 0.5, 0.5, QColor::fromRgb(0, 128, 0), FUZZ));
    QVERIFY(comparePixelNormPos(result, 0.725, 0.5, QColor::fromRgb(0, 0, 255), FUZZ));

    result = QImage();

    // make a Texture.sourceItem refer to something else now
    QMetaObject::invokeMethod(renderer.rootItem, "makeThirdReferToStandaloneSourceItem");

    // Frame 2
    renderer.renderControl->polishItems();
    renderer.renderControl->beginFrame();
    renderer.renderControl->sync();
    renderer.renderControl->render();
    renderer.enqueueReadback(&readCompleted, &readResult, &result);
    renderer.renderControl->endFrame();
    QVERIFY(readCompleted);
    QCOMPARE(result.size(), QSize(640, 480));

    // from left to right: red, green, red
    QVERIFY(comparePixelNormPos(result, 0.275, 0.5, QColor::fromRgb(255, 0, 0), FUZZ));
    QVERIFY(comparePixelNormPos(result, 0.5, 0.5, QColor::fromRgb(0, 128, 0), FUZZ));
    QVERIFY(comparePixelNormPos(result, 0.725, 0.5, QColor::fromRgb(255, 0, 0), FUZZ));

    result = QImage();

    // exercise resizing the render target, the root item, and the scene
    QVERIFY(renderer.resize(QSize(1280, 960)));

    // Frame 3
    renderer.renderControl->polishItems();
    renderer.renderControl->beginFrame();
    renderer.renderControl->sync();
    renderer.renderControl->render();
    renderer.enqueueReadback(&readCompleted, &readResult, &result);
    renderer.renderControl->endFrame();
    QVERIFY(readCompleted);
    QCOMPARE(result.size(), QSize(1280, 960));

    // from left to right: red, green, red (unchanged, compared to
    // frame 2, apart from the increased size, but the normalized
    // positions work as-is)
    QVERIFY(comparePixelNormPos(result, 0.275, 0.5, QColor::fromRgb(255, 0, 0), FUZZ));
    QVERIFY(comparePixelNormPos(result, 0.5, 0.5, QColor::fromRgb(0, 128, 0), FUZZ));
    QVERIFY(comparePixelNormPos(result, 0.725, 0.5, QColor::fromRgb(255, 0, 0), FUZZ));

    result = QImage();

    // switch the third sphere to a different sourceItem, now an Item
    // tree that has an explicit layer.enabled: true on it (meaning no
    // implicit QSGLayer gets created internally on Quick3D side)
    QMetaObject::invokeMethod(renderer.rootItem, "makeThirdReferToExplicitLayerBasedSourceItem");

    // Frame 4
    renderer.renderControl->polishItems();
    renderer.renderControl->beginFrame();
    renderer.renderControl->sync();
    renderer.renderControl->render();
    renderer.enqueueReadback(&readCompleted, &readResult, &result);
    renderer.renderControl->endFrame();
    QVERIFY(readCompleted);
    QCOMPARE(result.size(), QSize(1280, 960));

    // from left to right: red, green, gray
    QVERIFY(comparePixelNormPos(result, 0.275, 0.5, QColor::fromRgb(255, 0, 0), FUZZ));
    QVERIFY(comparePixelNormPos(result, 0.5, 0.5, QColor::fromRgb(0, 128, 0), FUZZ));
    QVERIFY(comparePixelNormPos(result, 0.725, 0.5, QColor::fromRgb(128, 128, 128), FUZZ));

    result = QImage();

    // switch the third sphere to with an Image (which itself is a
    // textureProvider) as the sourceItem
    QMetaObject::invokeMethod(renderer.rootItem, "makeThirdReferToImageSourceItem");

    // Frame 5
    renderer.renderControl->polishItems();
    renderer.renderControl->beginFrame();
    renderer.renderControl->sync();
    renderer.renderControl->render();
    renderer.enqueueReadback(&readCompleted, &readResult, &result);
    renderer.renderControl->endFrame();
    QVERIFY(readCompleted);
    QCOMPARE(result.size(), QSize(1280, 960));

    QVERIFY(comparePixelNormPos(result, 0.275, 0.5, QColor::fromRgb(255, 0, 0), FUZZ));
    QVERIFY(comparePixelNormPos(result, 0.5, 0.5, QColor::fromRgb(0, 128, 0), FUZZ));
    QVERIFY(comparePixelNormPos(result, 0.7, 0.5, QColor::fromRgb(64, 205, 82), FUZZ)); // green from Qt logo image
    QVERIFY(comparePixelNormPos(result, 0.725, 0.5, QColor::fromRgb(255, 255, 255), FUZZ)); // white from Qt logo image

    result = QImage();

    // switch the third sphere to use a sourceItem that has non-opaque content
    QMetaObject::invokeMethod(renderer.rootItem, "makeThirdReferToSemiTransparentSourceItem");

    // Frame 6
    renderer.renderControl->polishItems();
    renderer.renderControl->beginFrame();
    renderer.renderControl->sync();
    renderer.renderControl->render();
    renderer.enqueueReadback(&readCompleted, &readResult, &result);
    renderer.renderControl->endFrame();
    QVERIFY(readCompleted);
    QCOMPARE(result.size(), QSize(1280, 960));

    // from left to right: red, green, transparent/blue
    QVERIFY(comparePixelNormPos(result, 0.275, 0.5, QColor::fromRgb(255, 0, 0), FUZZ));
    QVERIFY(comparePixelNormPos(result, 0.5, 0.5, QColor::fromRgb(0, 128, 0), FUZZ));
    QVERIFY(comparePixelNormPos(result, 0.725, 0.44, QColor::fromRgb(0, 0, 0), FUZZ));
    QVERIFY(comparePixelNormPos(result, 0.725, 0.46, QColor::fromRgb(0, 0, 0), FUZZ));
    QVERIFY(comparePixelNormPos(result, 0.725, 0.5, QColor::fromRgb(0, 0, 255), FUZZ));

    result = QImage();

    // make the sourceItem's size half - this leads to the blue
    // rectangle occupy a larger area on the sphere's surface
    QMetaObject::invokeMethod(renderer.rootItem, "makeSemiTransparentSourceItemSmaller");

    // Frame 7
    renderer.renderControl->polishItems();
    renderer.renderControl->beginFrame();
    renderer.renderControl->sync();
    renderer.renderControl->render();
    renderer.enqueueReadback(&readCompleted, &readResult, &result);
    renderer.renderControl->endFrame();
    QVERIFY(readCompleted);
    QCOMPARE(result.size(), QSize(1280, 960));

    QVERIFY(comparePixelNormPos(result, 0.275, 0.5, QColor::fromRgb(255, 0, 0), FUZZ));
    QVERIFY(comparePixelNormPos(result, 0.5, 0.5, QColor::fromRgb(0, 128, 0), FUZZ));
    QVERIFY(comparePixelNormPos(result, 0.725, 0.44, QColor::fromRgb(0, 0, 0), FUZZ));
    QVERIFY(comparePixelNormPos(result, 0.725, 0.46, QColor::fromRgb(0, 0, 255), FUZZ)); // different from previous frame
    QVERIFY(comparePixelNormPos(result, 0.725, 0.5, QColor::fromRgb(0, 0, 255), FUZZ));
}

QTEST_MAIN(tst_RenderControl)
#include "tst_rendercontrol.moc"
