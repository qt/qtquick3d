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
    void sharedSceneChangesAtRuntime();

private:
    bool hasRed(const QImage &image) const;
    bool hasRedAt(const QImage &image, int x, int y) const;
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
    return hasRedAt(image, image.width() / 2, image.height() / 2);
}

bool tst_Layers::hasRedAt(const QImage &image, int x, int y) const
{
    const QColor c = image.pixelColor(x, y);
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

// Two views share a scene through importScene, so both layers walk the same
// render nodes. The dirty flags on a node are cleared by the first layer that
// prepares, so the change has to be passed on to the other layer, or that layer
// keeps its stale filtered node list (QTBUG-150709). The view that cannot see
// the box is declared first on purpose, so it is the one consuming the flags.
void tst_Layers::sharedSceneChangesAtRuntime()
{
    QScopedPointer<QQuickView> view(createView(QStringLiteral("sharedscene.qml"), QSize(400, 200)));
    QVERIFY(view);
    view->show();
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));
    QVERIFY(waitForFrames(view.data(), 2));

    auto *box = view->rootObject()->findChild<QQuick3DNode *>(QStringLiteral("box"));
    QVERIFY(box);
    QVERIFY(!box->visible());
    QCOMPARE(box->layers(), int(QQuick3DContentLayer::Layer1));

    // The left view (x < 200) sees Layer2, the right view (x >= 200) sees Layer1.
    const int leftX = 100;
    const int rightX = 300;
    const int y = 100;

    // Hidden: drawn in neither view.
    QImage image = grab(view.data());
    QVERIFY(!hasRedAt(image, leftX, y));
    QVERIFY(!hasRedAt(image, rightX, y));

    // Changes to the shared scene reach the views through the scene manager, which
    // can take more than one frame, so poll for the expected result. Each grab
    // renders a frame, and a view stuck with a stale node list never recovers on
    // its own, so a polled check cannot pass by accident.

    // Shown: only the view whose camera sees Layer1 draws it, even though the
    // other view's layer is the one that consumed the active change.
    box->setVisible(true);
    QTRY_VERIFY(hasRedAt(grab(view.data()), rightX, y));
    QVERIFY(!hasRedAt(grab(view.data()), leftX, y));

    // Moved to Layer2: the left view picks it up and the right view drops it. The
    // left layer consumes the tag change, so the right view depends on it being
    // passed on.
    box->setLayers(int(QQuick3DContentLayer::Layer2));
    QTRY_VERIFY(hasRedAt(grab(view.data()), leftX, y));
    QTRY_VERIFY(!hasRedAt(grab(view.data()), rightX, y));

    // Hidden again: gone from the left view too.
    box->setVisible(false);
    QTRY_VERIFY(!hasRedAt(grab(view.data()), leftX, y));
    QVERIFY(!hasRedAt(grab(view.data()), rightX, y));
}

QTEST_MAIN(tst_Layers)
#include "tst_layers.moc"
