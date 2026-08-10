// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QQuickView>
#include <QQuickWindow>

#include "../shared/util.h"

class tst_SkyMaterial : public QQuick3DDataTest
{
    Q_OBJECT

private slots:
    void initTestCase() override;
    void reloadDoesNotCrash();
};

void tst_SkyMaterial::initTestCase()
{
    QQuick3DDataTest::initTestCase();
    if (!initialized())
        return;
}

void tst_SkyMaterial::reloadDoesNotCrash()
{
    // Regression test for QTBUG-149007 (see skymaterial_reload.qml). It's a
    // use-after-free, so a clean run here isn't a guarantee -- this mainly
    // pays off under ASan/UBSan or QSG_RHI_DEBUG_LAYER=1 QSG_RHI_BACKEND=vulkan.
    QScopedPointer<QQuickView> view(createView(QLatin1String("skymaterial_reload.qml"), QSize(64, 64)));
    QVERIFY(view);
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    QObject *rootObject = view->rootObject();
    QVERIFY(rootObject);

    // Toggle every frame to repeatedly hit the crash window.
    for (int i = 0; i < 20; ++i) {
        rootObject->setProperty("useReadyMaterial", (i % 2) == 0);
        QImage frame = grab(view.data());
        QVERIFY(!frame.isNull());
    }
}

QTEST_MAIN(tst_SkyMaterial)

#include "tst_skymaterial.moc"
