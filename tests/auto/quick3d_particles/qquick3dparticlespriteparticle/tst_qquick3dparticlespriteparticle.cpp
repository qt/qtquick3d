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
#include <QtQuick3DParticles/private/qquick3dparticlespriteparticle_p.h>
#include <QtQuick3DParticles/private/qquick3dparticlespritesequence_p.h>


class tst_QQuick3DParticleSpriteParticle : public QObject
{
    Q_OBJECT

private slots:
    void testInitialization();
    void testParticle();
};

void tst_QQuick3DParticleSpriteParticle::testInitialization()
{
    QQuick3DParticleSpriteParticle *particle = new QQuick3DParticleSpriteParticle();
    QQuick3DParticleSpriteSequence *sequence = new QQuick3DParticleSpriteSequence();

    QCOMPARE(particle->blendMode(), QQuick3DParticleSpriteParticle::SourceOver);
    QCOMPARE(particle->sprite(), nullptr);
    QCOMPARE(sequence->frameCount(), 1);
    QCOMPARE(sequence->interpolate(), true);
    QCOMPARE(particle->billboard(), false);
    QCOMPARE(particle->colorTable(), nullptr);
    QVERIFY(qFuzzyCompare(particle->particleScale(), 5.0f));

    delete sequence;
    delete particle;
}

void tst_QQuick3DParticleSpriteParticle::testParticle()
{
    QQuick3DParticleSpriteParticle *particle = new QQuick3DParticleSpriteParticle();
    QQuick3DParticleSpriteSequence *sequence = new QQuick3DParticleSpriteSequence();

    particle->setBlendMode(QQuick3DParticleSpriteParticle::Multiply);
    QCOMPARE(particle->blendMode(), QQuick3DParticleSpriteParticle::Multiply);

    sequence->setFrameCount(1000);
    QCOMPARE(sequence->frameCount(), 1000);

    sequence->setInterpolate(false);
    QCOMPARE(sequence->interpolate(), false);

    particle->setBillboard(true);
    QCOMPARE(particle->billboard(), true);

    particle->setParticleScale(20.0f);
    QVERIFY(qFuzzyCompare(particle->particleScale(), 20.0f));

    QQuick3DTexture *texture = new QQuick3DTexture();
    particle->setSprite(texture);
    QCOMPARE(particle->sprite(), texture);

    particle->setColorTable(texture);
    QCOMPARE(particle->colorTable(), texture);

    delete texture;
    delete sequence;
    delete particle;
}

QTEST_APPLESS_MAIN(tst_QQuick3DParticleSpriteParticle)
#include "tst_qquick3dparticlespriteparticle.moc"
