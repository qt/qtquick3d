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

#include <QtQuick3DParticles/private/qquick3dparticlesystem_p.h>
#include <QtQuick3DParticles/private/qquick3dparticle_p.h>


class tst_QQuick3DParticle : public QObject
{
    Q_OBJECT

    class Particle : public QQuick3DParticle
    {
    public:
        Particle(QQuick3DObject *parent) : QQuick3DParticle(parent) {}
        void reset() override
        {

        }
    };

private slots:
    void testParticle();
};

void tst_QQuick3DParticle::testParticle()
{
    QScopedPointer<QQuick3DParticleSystem> system(new QQuick3DParticleSystem());
    Particle *p = new Particle(system.data());
    p->setSystem(system.data());
    QCOMPARE(p->system(), system.data());

    // Test deleting
    delete p;

    p = new Particle(system.data());

    QSignalSpy spy(p, SIGNAL(systemChanged()));

    auto *newSystem = new QQuick3DParticleSystem();
    p->setSystem(newSystem);

    QCOMPARE(spy.count(), 1);

    QCOMPARE(p->maxAmount(), 100);
    p->setMaxAmount(200);
    QCOMPARE(p->maxAmount(), 200);

    QCOMPARE(p->color(), Qt::white);
    p->setColor(Qt::red);
    QCOMPARE(p->color(), Qt::red);

    QCOMPARE(p->colorVariation(), QVector4D());
    p->setColorVariation(QVector4D(0.5f, 0.5f, 0.5f, 0.5f));
    QVERIFY(qFuzzyCompare(p->colorVariation(), QVector4D(0.5f, 0.5f, 0.5f, 0.5f)));

    QCOMPARE(p->fadeInEffect(), QQuick3DParticle::FadeOpacity);
    QCOMPARE(p->fadeOutEffect(), QQuick3DParticle::FadeOpacity);
    p->setFadeInEffect(QQuick3DParticle::FadeNone);
    QCOMPARE(p->fadeInEffect(), QQuick3DParticle::FadeNone);
    p->setFadeOutEffect(QQuick3DParticle::FadeNone);
    QCOMPARE(p->fadeOutEffect(), QQuick3DParticle::FadeNone);

    QCOMPARE(p->fadeInDuration(), 250);
    QCOMPARE(p->fadeOutDuration(), 250);
    p->setFadeInDuration(1000);
    QCOMPARE(p->fadeInDuration(), 1000);
    p->setFadeOutDuration(1000);
    QCOMPARE(p->fadeOutDuration(), 1000);

    QCOMPARE(p->alignMode(), QQuick3DParticle::AlignNone);
    p->setAlignMode(QQuick3DParticle::AlignTowardsTarget);
    QCOMPARE(p->alignMode(), QQuick3DParticle::AlignTowardsTarget);

    QVERIFY(qFuzzyCompare(p->alignTargetPosition(), QVector3D()));
    p->setAlignTargetPosition(QVector3D(100.0f, 100.0f, 100.0f));
    QVERIFY(qFuzzyCompare(p->alignTargetPosition(), QVector3D(100.0f, 100.0f, 100.0f)));

    QCOMPARE(p->sortMode(), QQuick3DParticle::SortNone);
    p->setSortMode(QQuick3DParticle::SortDistance);
    QCOMPARE(p->sortMode(), QQuick3DParticle::SortDistance);

}

QTEST_APPLESS_MAIN(tst_QQuick3DParticle)
#include "tst_qquick3dparticle.moc"
