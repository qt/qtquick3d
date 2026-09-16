// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QQuickView>

#include <QtQuick3D/private/qquick3dcontentlayer_p.h>
#include <QtQuick3D/private/qquick3dnode_p.h>

#include "../shared/util.h"

// Content layers at runtime: a node moved onto or off the camera's layers
// after the first frame must appear or disappear without any other change to
// the scene. The node's tag is not part of the transform and opacity update,
// so the layer filter has to be redone when a tag changes.
class tst_Layers : public QQuick3DDataTest
{
    Q_OBJECT

private slots:
    void initTestCase() override;
    void layersChangeAtRuntime();

private:
    bool hasRed(const QImage &image) const;
};

void tst_Layers::initTestCase()
{
    QQuick3DDataTest::initTestCase();
    if (!initialized())
        return;
}

bool tst_Layers::hasRed(const QImage &image) const
{
    // The cube fills the middle of the view; sample the centre.
    const QColor c = image.pixelColor(image.width() / 2, image.height() / 2);
    return c.red() > 150 && c.green() < 80 && c.blue() < 80;
}

void tst_Layers::layersChangeAtRuntime()
{
    QScopedPointer<QQuickView> view(createView(QStringLiteral("layerschange.qml"), QSize(200, 200)));
    QVERIFY(view);
    view->show();
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));
    QVERIFY(waitForFrames(view.data(), 2));

    auto *box = view->rootObject()->findChild<QQuick3DNode *>(QStringLiteral("box"));
    QVERIFY(box);
    QCOMPARE(box->layers(), int(QQuick3DContentLayer::Layer1));

    // Outside the camera's mask: not drawn.
    QVERIFY(!hasRed(grab(view.data())));

    // Onto the camera's layer: drawn on the next frames.
    box->setLayers(int(QQuick3DContentLayer::Layer0));
    QVERIFY(waitForFrames(view.data(), 2));
    QVERIFY(hasRed(grab(view.data())));

    // And off again.
    box->setLayers(int(QQuick3DContentLayer::Layer2));
    QVERIFY(waitForFrames(view.data(), 2));
    QVERIFY(!hasRed(grab(view.data())));
}

QTEST_MAIN(tst_Layers)
#include "tst_layers.moc"
