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

#include <QtQuick3DParticles/private/qquick3dparticletargetdirection_p.h>


class tst_QQuick3DParticleTargetDirection : public QObject
{
    Q_OBJECT

private slots:
    void testTargetDirection();
};

void tst_QQuick3DParticleTargetDirection::testTargetDirection()
{
    QQuick3DParticleTargetDirection *targetDir = new QQuick3DParticleTargetDirection();

    delete targetDir;

    targetDir = new QQuick3DParticleTargetDirection();

    QVERIFY(qFuzzyCompare(targetDir->position(), QVector3D()));
    QVERIFY(qFuzzyCompare(targetDir->positionVariation(), QVector3D()));
    QCOMPARE(targetDir->normalized(), false);
    QVERIFY(qFuzzyCompare(targetDir->magnitude(), 1.0f));
    QVERIFY(qFuzzyCompare(targetDir->magnitudeVariation(), 0.0f));

    const QVector3D vecVal(1.0f, 2.0f, 3.0f);
    targetDir->setPosition(vecVal);
    QVERIFY(qFuzzyCompare(targetDir->position(), vecVal));
    targetDir->setPositionVariation(vecVal);
    QVERIFY(qFuzzyCompare(targetDir->positionVariation(), vecVal));

    targetDir->setNormalized(true);
    QCOMPARE(targetDir->normalized(), true);

    targetDir->setMagnitude(10.0f);
    QVERIFY(qFuzzyCompare(targetDir->magnitude(), 10.0f));

    targetDir->setMagnitudeVariation(5.0f);
    QVERIFY(qFuzzyCompare(targetDir->magnitudeVariation(), 5.0f));

    delete targetDir;
}

QTEST_APPLESS_MAIN(tst_QQuick3DParticleTargetDirection)
#include "tst_qquick3dparticletargetdirection.moc"
