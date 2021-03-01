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

#include <QtQuick3DParticles/private/qquick3dparticlespriteparticle_p.h>
#include <QtQuick3DParticles/private/qquick3dparticlemodelparticle_p.h>
#include <QtQuick3DParticles/private/qquick3dparticlesystem_p.h>


class tst_QQuick3DParticleSystem : public QObject
{
    Q_OBJECT

private slots:
    void testInitialization();
    void testSystem();
};

void tst_QQuick3DParticleSystem::testInitialization()
{
    QQuick3DParticleSystem *system = new QQuick3DParticleSystem();

    QCOMPARE(system->isRunning(), true);
    QCOMPARE(system->isPaused(), false);
    QCOMPARE(system->particleCount(), 0);
    QCOMPARE(system->startTime(), 0);
    QCOMPARE(system->time(), 0);
    QCOMPARE(system->logging(), false);
    QCOMPARE(system->useRandomSeed(), true);
    QCOMPARE(system->seed(), 0);

    delete system;
}

void tst_QQuick3DParticleSystem::testSystem()
{
    QQuick3DParticleSystem *system = new QQuick3DParticleSystem();

    QQuick3DParticleSpriteParticle *sp = new QQuick3DParticleSpriteParticle(system);
    sp->setMaxAmount(10);

    system->registerParticle(sp);
    QCOMPARE(system->particleCount(), 10);

    QQuick3DParticleModelParticle *mp = new QQuick3DParticleModelParticle(system);
    mp->setMaxAmount(10);

    system->registerParticle(mp);
    QCOMPARE(system->particleCount(), 20);

    system->setStartTime(2000);
    QCOMPARE(system->startTime(), 2000);

    system->setTime(2000);
    QCOMPARE(system->time(), 2000);

    system->setUseRandomSeed(false);
    QCOMPARE(system->useRandomSeed(), false);

    system->setSeed(1234);
    QCOMPARE(system->seed(), 1234);

    delete system;
}


QTEST_APPLESS_MAIN(tst_QQuick3DParticleSystem)
#include "tst_qquick3dparticlesystem.moc"
