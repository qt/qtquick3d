/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:GPL$
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
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest>

#include <QtQuick3DUtils/private/qssgbounds3_p.h>

class bounds : public QObject
{
    Q_OBJECT

private slots:
    void test_constructors();
};

void bounds::test_constructors()
{
    const auto boundsDefault = QSSGBounds3();
    const auto boundsUninitialized = QSSGBounds3(Qt::Uninitialized);
    auto boundsEmpty = QSSGBounds3();
    boundsEmpty.setEmpty();

    QVERIFY2(boundsDefault.minimum == boundsEmpty.minimum, "Empty equals default constructor");
    QVERIFY2(boundsDefault.maximum == boundsEmpty.maximum, "Empty equals default constructor");

    QVERIFY2(boundsDefault.minimum != boundsUninitialized.minimum, "Uninitialized is not equal to default");

    QVERIFY2(boundsEmpty.isEmpty(), "Check empty");
    QVERIFY2(boundsDefault.isEmpty(), "Check empty");
}

QTEST_APPLESS_MAIN(bounds)

#include "tst_bounds.moc"
