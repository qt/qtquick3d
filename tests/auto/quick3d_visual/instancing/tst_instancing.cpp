// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QQuickView>

#include "../shared/util.h"

class tst_Instancing : public QQuick3DDataTest
{
    Q_OBJECT

private slots:
    void initTestCase() override;
    void lod_data();
    void lod();
};

void tst_Instancing::initTestCase()
{
    QQuick3DDataTest::initTestCase();
    if (!initialized())
        return;
}

static const int FUZZ = 5;

// Normalized horizontal position of the three instances in lodinstancing.qml.
static const qreal NearPos = 0.2;
static const qreal MidPos = 0.5;
static const qreal FarPos = 0.8;

void tst_Instancing::lod_data()
{
    QTest::addColumn<bool>("depthSorting");
    QTest::addColumn<qreal>("lodMin");
    QTest::addColumn<qreal>("lodMax");
    QTest::addColumn<bool>("nearVisible");
    QTest::addColumn<bool>("midVisible");
    QTest::addColumn<bool>("farVisible");

    // The instances are 612, 900 and 1206 units away from the camera. Keep the
    // tags free of whitespace: re-running a single row means passing
    // "lod:<tag>" on the command line, and on Android that argument is split on
    // whitespace again on the way to the device.
    for (bool depthSorting : { false, true }) {
        const QByteArray tag = depthSorting ? QByteArrayLiteral("depth-sorted-") : QByteArrayLiteral("unsorted-");
        QTest::newRow(tag + "no-culling") << depthSorting << qreal(0) << qreal(-1) << true << true << true;
        QTest::newRow(tag + "nearest-only") << depthSorting << qreal(0) << qreal(750) << true << false << false;
        QTest::newRow(tag + "middle-only") << depthSorting << qreal(750) << qreal(1050) << false << true << false;
        QTest::newRow(tag + "farthest-only") << depthSorting << qreal(1050) << qreal(-1) << false << false << true;
        QTest::newRow(tag + "everything-culled") << depthSorting << qreal(2000) << qreal(-1) << false << false << false;
    }
}

// Verifies that exactly the instances within the LOD thresholds are drawn. The
// depth sorted rows used to overrun the instance buffers, see QTBUG-148789.
void tst_Instancing::lod()
{
    QFETCH(bool, depthSorting);
    QFETCH(qreal, lodMin);
    QFETCH(qreal, lodMax);
    QFETCH(bool, nearVisible);
    QFETCH(bool, midVisible);
    QFETCH(bool, farVisible);

    QQuickView view;
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.resize(QSize(400, 400));
    view.setInitialProperties({ { QLatin1String("depthSorting"), depthSorting },
                                { QLatin1String("lodMin"), lodMin },
                                { QLatin1String("lodMax"), lodMax } });
    view.setSource(testFileUrl(QLatin1String("lodinstancing.qml")));
    QCOMPARE(view.status(), QQuickView::Ready);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    const QImage result = grab(&view);
    if (result.isNull())
        return; // was QFAIL'ed already

    QVERIFY(comparePixelNormPos(result, NearPos, 0.5, nearVisible ? Qt::white : Qt::black, FUZZ));
    QVERIFY(comparePixelNormPos(result, MidPos, 0.5, midVisible ? Qt::white : Qt::black, FUZZ));
    QVERIFY(comparePixelNormPos(result, FarPos, 0.5, farVisible ? Qt::white : Qt::black, FUZZ));
}

QTEST_MAIN(tst_Instancing)
#include "tst_instancing.moc"
