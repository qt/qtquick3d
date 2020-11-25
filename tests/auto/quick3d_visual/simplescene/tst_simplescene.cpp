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

#include "../shared/util.h"

class tst_SimpleScene : public QQuick3DDataTest
{
    Q_OBJECT

private slots:
    void initTestCase() override;
    void cube();
};

void tst_SimpleScene::initTestCase()
{
    QQuick3DDataTest::initTestCase();
    if (!initialized())
        return;
}

const int FUZZ = 5;

void tst_SimpleScene::cube()
{
    QScopedPointer<QQuickView> view(createView(QLatin1String("cube.qml"), QSize(640, 480)));
    QVERIFY(view);
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    const QImage result = grab(view.data());
    if (result.isNull())
        return; // was QFAIL'ed already

    const qreal dpr = view->devicePixelRatio();

    QVERIFY(comparePixel(result, 50, 50, dpr, Qt::black, FUZZ));

    // front face of the cube is lighter gray-ish
    QVERIFY(comparePixelNormPos(result, 0.5, 0.5, QColor::fromRgb(239, 239, 239), FUZZ));

    // the top is darker
    QVERIFY(comparePixelNormPos(result, 0.5, 0.375, QColor::fromRgb(181, 181, 181), FUZZ));
}

QTEST_MAIN(tst_SimpleScene)
#include "tst_simplescene.moc"
