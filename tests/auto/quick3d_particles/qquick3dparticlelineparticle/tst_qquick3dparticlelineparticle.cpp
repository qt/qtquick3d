/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include <QtQuick3DParticles/private/qquick3dparticlelineparticle_p.h>

class tst_QQuick3DParticleLineParticle : public QObject
{
    Q_OBJECT

private slots:
    void testInitialization();
    void testParticle();
};

void tst_QQuick3DParticleLineParticle::testInitialization()
{
    QQuick3DParticleLineParticle *particle = new QQuick3DParticleLineParticle();

    QCOMPARE(particle->segmentCount(), 1);
    QVERIFY(qFuzzyCompare(particle->alphaFade(), 0.0f));
    QVERIFY(qFuzzyCompare(particle->scaleMultiplier(), 1.0f));
    QVERIFY(qFuzzyCompare(particle->texcoordMultiplier(), 1.0f));
    QVERIFY(qFuzzyCompare(particle->length(), -1.0f));
    QVERIFY(qFuzzyCompare(particle->lengthVariation(), 0.0f));
    QVERIFY(qFuzzyCompare(particle->lengthDeltaMin(), 10.0f));
    QCOMPARE(particle->eolFadeOutDuration(), 0);
    QCOMPARE(particle->texcoordMode(), QQuick3DParticleLineParticle::Absolute);

    delete particle;
}

void tst_QQuick3DParticleLineParticle::testParticle()
{
    QQuick3DParticleLineParticle *particle = new QQuick3DParticleLineParticle();

    particle->setSegmentCount(100);
    QCOMPARE(particle->segmentCount(), 100);

    particle->setSegmentCount(-100);
    QCOMPARE(particle->segmentCount(), 1);

    particle->setAlphaFade(0.5f);
    QVERIFY(qFuzzyCompare(particle->alphaFade(), 0.5f));

    particle->setAlphaFade(1.5f);
    QVERIFY(qFuzzyCompare(particle->alphaFade(), 1.0f));

    particle->setAlphaFade(-1.5f);
    QVERIFY(qFuzzyCompare(particle->alphaFade(), 0.0f));

    particle->setScaleMultiplier(0.5f);
    QVERIFY(qFuzzyCompare(particle->scaleMultiplier(), 0.5f));

    particle->setScaleMultiplier(3.5f);
    QVERIFY(qFuzzyCompare(particle->scaleMultiplier(), 2.0f));

    particle->setScaleMultiplier(-1.5f);
    QVERIFY(qFuzzyCompare(particle->scaleMultiplier(), 0.0f));

    particle->setTexcoordMultiplier(0.5f);
    QVERIFY(qFuzzyCompare(particle->texcoordMultiplier(), 0.5f));

    particle->setTexcoordMultiplier(3.5f);
    QVERIFY(qFuzzyCompare(particle->texcoordMultiplier(), 3.5f));

    particle->setTexcoordMultiplier(-1.5f);
    QVERIFY(qFuzzyCompare(particle->texcoordMultiplier(), -1.5f));

    particle->setLength(100.0f);
    QVERIFY(qFuzzyCompare(particle->length(), 100.0f));

    particle->setLength(-1.0f);
    QVERIFY(qFuzzyCompare(particle->length(), -1.0f));

    particle->setLength(-1.5f);
    QVERIFY(qFuzzyCompare(particle->length(), 0.0f));

    particle->setLengthVariation(100.0f);
    QVERIFY(qFuzzyCompare(particle->lengthVariation(), 100.0f));

    particle->setLengthVariation(-1.0f);
    QVERIFY(qFuzzyCompare(particle->lengthVariation(), 0.0f));

    particle->setLengthDeltaMin(100.0f);
    QVERIFY(qFuzzyCompare(particle->lengthDeltaMin(), 100.0f));

    particle->setLengthDeltaMin(-1.0f);
    QVERIFY(qFuzzyCompare(particle->lengthDeltaMin(), 0.0f));

    particle->setEolFadeOutDuration(100);
    QCOMPARE(particle->eolFadeOutDuration(), 100);

    particle->setEolFadeOutDuration(-100);
    QCOMPARE(particle->eolFadeOutDuration(), 0);

    particle->setTexcoordMode(QQuick3DParticleLineParticle::Fill);
    QCOMPARE(particle->texcoordMode(), QQuick3DParticleLineParticle::Fill);

    delete particle;
}

QTEST_APPLESS_MAIN(tst_QQuick3DParticleLineParticle)
#include "tst_qquick3dparticlelineparticle.moc"
