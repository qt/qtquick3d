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

#include <QtQuick3DParticles/private/qquick3dparticle_p.h>
#include <QtQuick3DParticles/private/qquick3dparticlegravity_p.h>


class tst_QQuick3DParticleGravity : public QObject
{
    Q_OBJECT

    class Gravity : public QQuick3DParticleGravity
    {
        friend class tst_QQuick3DParticleGravity;
    public:
        Gravity() : QQuick3DParticleGravity() {}

        void testAffectParticle(const QQuick3DParticleData &sd, QQuick3DParticleDataCurrent *d, float time)
        {
            QQuick3DParticleGravity::affectParticle(sd, d, time);
        }
    };

private slots:
    void testGravity();
    void testGravityAffect();
};

void tst_QQuick3DParticleGravity::testGravity()
{
    QQuick3DParticleGravity *gravity = new QQuick3DParticleGravity();

    delete gravity;

    gravity = new QQuick3DParticleGravity();

    QVERIFY(qFuzzyCompare(gravity->magnitude(), 100.0f));
    QVERIFY(qFuzzyCompare(gravity->direction(), QVector3D(0.0f, -1.0f, 0.0f)));

    gravity->setMagnitude(50.0f);
    QVERIFY(qFuzzyCompare(gravity->magnitude(), 50.0f));

    const QVector3D dirVal(1.0f, 2.0f, 3.0f);
    gravity->setDirection(dirVal);
    QVERIFY(qFuzzyCompare(gravity->direction(), dirVal));

    delete gravity;
}

void tst_QQuick3DParticleGravity::testGravityAffect()
{
    Gravity *gravity = new Gravity();

    QQuick3DParticleData particleData = {{}, {}, {}, {}, {}, 0.0f, 1.0f, 1.0f, 0};
    QQuick3DParticleDataCurrent particleDataCurrent = {{}, {}, {}, {}, {}};

    gravity->setMagnitude(0.0f);
    gravity->testAffectParticle(particleData, &particleDataCurrent, 0.5f);
    QVERIFY(qFuzzyCompare(particleDataCurrent.position, QVector3D()));

    gravity->setMagnitude(100.0f);
    gravity->testAffectParticle(particleData, &particleDataCurrent, 0.5f);
    QVERIFY(!qFuzzyCompare(particleDataCurrent.position, QVector3D()));

    delete gravity;
}

QTEST_APPLESS_MAIN(tst_QQuick3DParticleGravity)
#include "tst_qquick3dparticlegravity.moc"
