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
#include <QtQuick3DParticles/private/qquick3dparticleemitter_p.h>
#include <QtQuick3DParticles/private/qquick3dparticlespriteparticle_p.h>
#include <QtQuick3DParticles/private/qquick3dparticletargetdirection_p.h>


class tst_QQuick3DParticleEmitter : public QObject
{
    Q_OBJECT

private slots:
    void testInitialization();
    void testEmitter();
};

void tst_QQuick3DParticleEmitter::testInitialization()
{
    QQuick3DParticleEmitter *emitter = new QQuick3DParticleEmitter();

    QCOMPARE(emitter->system(), nullptr);
    QCOMPARE(emitter->velocity(), nullptr);
    QCOMPARE(emitter->particle(), nullptr);
    QCOMPARE(emitter->enabled(), true);
    QCOMPARE(emitter->shape(), nullptr);
    QCOMPARE(emitter->emitRate(), 0);
    QCOMPARE(emitter->lifeSpan(), 1000);
    QCOMPARE(emitter->lifeSpanVariation(), 0);
    QVERIFY(qFuzzyCompare(emitter->particleScale(), 1.0f));
    QVERIFY(qFuzzyCompare(emitter->particleEndScale(), -1.0f));
    QVERIFY(qFuzzyCompare(emitter->particleScaleVariation(), 0.0f));
    QVERIFY(qFuzzyCompare(emitter->particleRotation(), QVector3D()));
    QVERIFY(qFuzzyCompare(emitter->particleRotationVariation(), QVector3D()));
    QVERIFY(qFuzzyCompare(emitter->particleRotationVelocity(), QVector3D()));
    QVERIFY(qFuzzyCompare(emitter->particleRotationVelocityVariation(), QVector3D()));

    delete emitter;
}

void tst_QQuick3DParticleEmitter::testEmitter()
{
    QQuick3DParticleSystem *system = new QQuick3DParticleSystem();
    QQuick3DParticleEmitter *emitter = new QQuick3DParticleEmitter(system);
    QQuick3DParticleTargetDirection *direction = new QQuick3DParticleTargetDirection(system);
    QQuick3DParticleSpriteParticle *particle = new QQuick3DParticleSpriteParticle(system);
    QQuick3DParticleShape *shape = new QQuick3DParticleShape(system);

    emitter->setSystem(system);
    QCOMPARE(emitter->system(), system);

    emitter->setVelocity(direction);
    QCOMPARE(emitter->velocity(), direction);

    emitter->setParticle(particle);
    QCOMPARE(emitter->particle(), particle);

    emitter->setShape(shape);
    QCOMPARE(emitter->shape(), shape);

    emitter->setEnabled(false);
    QCOMPARE(emitter->enabled(), false);

    emitter->setEmitRate(200);
    QCOMPARE(emitter->emitRate(), 200);

    emitter->setLifeSpan(10000);
    QCOMPARE(emitter->lifeSpan(), 10000);

    emitter->setLifeSpan(-10000);
    //QCOMPARE(emitter->lifeSpan(), 10000);

    emitter->setLifeSpanVariation(1000);
    QCOMPARE(emitter->lifeSpanVariation(), 1000);

    emitter->setParticleScale(100.0f);
    QVERIFY(qFuzzyCompare(emitter->particleScale(), 100.0f));

    emitter->setParticleScale(-2.0f);
    //QVERIFY(qFuzzyCompare(emitter->particleScale(), 100.0f));

    emitter->setParticleEndScale(10.0f);
    QVERIFY(qFuzzyCompare(emitter->particleEndScale(), 10.0f));

    emitter->setParticleScaleVariation(0.2f);
    QVERIFY(qFuzzyCompare(emitter->particleScaleVariation(), 0.2f));

    const QVector3D rotation(1.0f, 2.0f, 3.0f);
    emitter->setParticleRotation(rotation);
    QVERIFY(qFuzzyCompare(emitter->particleRotation(), rotation));

    const QVector3D rotationVariation(0.2f, 0.3f, 0.4f);
    emitter->setParticleRotationVariation(rotationVariation);
    QVERIFY(qFuzzyCompare(emitter->particleRotationVariation(), rotationVariation));

    const QVector3D velocity(1.2f, 2.3f, 3.4f);
    emitter->setParticleRotationVelocity(velocity);
    QVERIFY(qFuzzyCompare(emitter->particleRotationVelocity(), velocity));

    const QVector3D velocityVariation(0.1f, 0.3f, 0.4f);
    emitter->setParticleRotationVelocityVariation(velocityVariation);
    QVERIFY(qFuzzyCompare(emitter->particleRotationVelocityVariation(), velocityVariation));

    delete system;
}

QTEST_APPLESS_MAIN(tst_QQuick3DParticleEmitter)
#include "tst_qquick3dparticleemitter.moc"
