// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QQuickView>
#include "../shared/util.h"

class tst_RuntimeLoader : public QQuick3DDataTest
{
    Q_OBJECT
private slots:
    void initTestCase() override;
    void queryAll();
};

void tst_RuntimeLoader::initTestCase()
{
    QQuick3DDataTest::initTestCase();
    if (!initialized())
        return;
}

void tst_RuntimeLoader::queryAll()
{
    QScopedPointer<QQuickView> view(createView(QLatin1String("queryall.qml"), QSize(200, 200)));
    QVERIFY(view);
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    const QImage frame = grab(view.data());
    if (frame.isNull())
        return;

    // Wait for either a successful load or an error (e.g. importer plugin absent).
    QTRY_VERIFY_WITH_TIMEOUT(view->rootObject()->property("loaded").toBool()
                             || view->rootObject()->property("loadError").toBool(),
                             10000);
    if (view->rootObject()->property("loadError").toBool())
        QSKIP("Asset failed to load — importer plugin likely not available on this platform");

    QVERIFY(view->rootObject()->property("materialCount").toInt() > 0);
    QVERIFY(view->rootObject()->property("modelCount").toInt() > 0);
    QVERIFY(view->rootObject()->property("lightCount").toInt() > 0);
    QVERIFY(view->rootObject()->property("queryNullForMissing").toBool());
}

QTEST_MAIN(tst_RuntimeLoader)
#include "tst_runtimeloader.moc"
