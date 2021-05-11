/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include <QTest>
#include <QSignalSpy>
#include <QScopedPointer>

#include <QtQuick3DParticles/private/qquick3dparticlevectordirection_p.h>


class tst_QQuick3DParticleVectorDirection : public QObject
{
    Q_OBJECT

private slots:
    void testVectorDirection();
};

void tst_QQuick3DParticleVectorDirection::testVectorDirection()
{
    QQuick3DParticleVectorDirection *vectorDir = new QQuick3DParticleVectorDirection();

    delete vectorDir;

    vectorDir = new QQuick3DParticleVectorDirection();

    QVERIFY(qFuzzyCompare(vectorDir->direction(), QVector3D(0.0f, 100.0f, 0.0f)));
    QVERIFY(qFuzzyCompare(vectorDir->directionVariation(), QVector3D()));
    QVERIFY(vectorDir->normalized() == false);

    const QVector3D vecVal(100.0f, 200.0f, 300.0f);
    vectorDir->setDirection(vecVal);
    QVERIFY(qFuzzyCompare(vectorDir->direction(), vecVal));
    vectorDir->setDirectionVariation(vecVal);
    QVERIFY(qFuzzyCompare(vectorDir->directionVariation(), vecVal));
    vectorDir->setNormalized(true);
    QVERIFY(vectorDir->normalized() == true);

    delete vectorDir;
}

QTEST_APPLESS_MAIN(tst_QQuick3DParticleVectorDirection)
#include "tst_qquick3dparticlevectordirection.moc"
