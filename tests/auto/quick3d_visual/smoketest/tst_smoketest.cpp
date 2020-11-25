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
#include <QQuickView>
#include <QQmlError>

#include "../shared/util.h"

#include <private/qsgrenderloop_p.h>
#include <private/qsgrhisupport_p.h>

class tst_SmokeTest : public QQuick3DDataTest
{
    Q_OBJECT

private slots:
    void initTestCase() override;
    void bringup();
};

void tst_SmokeTest::initTestCase()
{
    QQuick3DDataTest::initTestCase();
    if (!initialized())
        return;

    QSGRenderLoop *loop = QSGRenderLoop::instance();
    qDebug() << "Render loop:" << loop
             << "RHI backend:" << QSGRhiSupport::instance()->rhiBackendName();

    // Get as many graphics init logs printed as we can, to help
    // troubleshooting both locally and in CI VMs.
    qputenv("QSG_INFO", "1");
}

void tst_SmokeTest::bringup()
{
    QScopedPointer<QQuickView> view(createView(QLatin1String("view.qml"), QSize(640, 480)));
    QVERIFY(view);
    QVERIFY(view->errors().isEmpty());
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));
}

QTEST_MAIN(tst_SmokeTest)
#include "tst_smoketest.moc"
