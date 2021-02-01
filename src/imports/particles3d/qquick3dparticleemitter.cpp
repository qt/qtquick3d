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

#include "qquick3dparticleemitter_p.h"
#include "qquick3dparticlemodelparticle_p.h"
#include "qquick3dparticlerandomizer_p.h"
#include "qquick3dparticleshape_p.h"

QT_BEGIN_NAMESPACE

QQuick3DParticleEmitter::QQuick3DParticleEmitter(QQuick3DNode *parent)
    : QQuick3DNode(parent)
{
}

QQuick3DParticleEmitter::~QQuick3DParticleEmitter()
{
    if (m_system)
        m_system->registerParticleEmitter(this);
}

bool QQuick3DParticleEmitter::enabled() const
{
    return m_enabled;
}
void QQuick3DParticleEmitter::setEnabled(bool enabled)
{
    if (m_enabled == enabled)
        return;

    if (enabled && m_system) {
        // When enabling, we need to reset the
        // previous emit time as it might be a long time ago.
        m_prevEmitTime = m_system->timeInt;
    }

    m_enabled = enabled;
    Q_EMIT enabledChanged();
}

QQuick3DParticleDirection *QQuick3DParticleEmitter::velocity() const
{
    return m_velocity;
}

void QQuick3DParticleEmitter::setVelocity(QQuick3DParticleDirection *velocity)
{
    if (m_velocity == velocity)
        return;

    m_velocity = velocity;
    Q_EMIT velocityChanged();
}

QQuick3DParticleSystem* QQuick3DParticleEmitter::system() const
{
    return m_system;
}

void QQuick3DParticleEmitter::setSystem(QQuick3DParticleSystem* system)
{
    if (m_system == system)
        return;

    if (m_system)
        m_system->unRegisterParticleEmitter(this);

    m_system = system;
    if (m_system)
        m_system->registerParticleEmitter(this);

    Q_EMIT systemChanged();
}


int QQuick3DParticleEmitter::emitRate() const
{
    return m_emitRate;
}

void QQuick3DParticleEmitter::setEmitRate(int emitRate)
{
    if (m_emitRate == emitRate)
        return;

    if (m_emitRate == 0 && m_system) {
        // When changing emit rate from 0 we need to reset
        // previous emit time as it may be long time ago
        m_prevEmitTime = m_system->timeInt;
    }
    m_emitRate = emitRate;
    Q_EMIT emitRateChanged();
}


float QQuick3DParticleEmitter::particleScale() const
{
    return m_particleScale;
}

void QQuick3DParticleEmitter::setParticleScale(float particleScale)
{
    if (qFuzzyCompare(m_particleScale, particleScale))
        return;

    m_particleScale = particleScale;
    Q_EMIT particleScaleChanged();
}

float QQuick3DParticleEmitter::particleEndScale() const
{
    return m_particleEndScale;
}

void QQuick3DParticleEmitter::setParticleEndScale(float particleEndScale)
{
    if (qFuzzyCompare(m_particleEndScale, particleEndScale))
        return;

    m_particleEndScale = particleEndScale;
    Q_EMIT particleEndScaleChanged();
}

float QQuick3DParticleEmitter::particleScaleVariation() const
{
    return m_particleScaleVariation;
}

void QQuick3DParticleEmitter::setParticleScaleVariation(float particleScaleVariation)
{
    if (qFuzzyCompare(m_particleScaleVariation, particleScaleVariation))
        return;

    m_particleScaleVariation = particleScaleVariation;
    Q_EMIT particleScaleVariationChanged();
}

int QQuick3DParticleEmitter::lifeSpan() const
{
    return m_lifeSpan;
}

void QQuick3DParticleEmitter::setLifeSpan(int lifeSpan)
{
    if (m_lifeSpan == lifeSpan)
        return;

    m_lifeSpan = lifeSpan;
    Q_EMIT lifeSpanChanged();
}

int QQuick3DParticleEmitter::lifeSpanVariation() const
{
    return m_lifeSpanVariation;
}

void QQuick3DParticleEmitter::setLifeSpanVariation(int lifeSpanVariation)
{
    if (m_lifeSpanVariation == lifeSpanVariation)
        return;

    m_lifeSpanVariation = lifeSpanVariation;
    Q_EMIT lifeSpanVariationChanged();
}

QQuick3DParticle *QQuick3DParticleEmitter::particle() const
{
    return m_particle;
}

void QQuick3DParticleEmitter::setParticle(QQuick3DParticle *particle)
{
    if (m_particle == particle)
        return;

    m_particle = particle;
    Q_EMIT particleChanged();
}


QQuick3DParticleShape *QQuick3DParticleEmitter::shape() const
{
    return m_shape;
}

void QQuick3DParticleEmitter::setShape(QQuick3DParticleShape *shape)
{
    if (m_shape == shape)
        return;

    m_shape = shape;
    Q_EMIT shapeChanged();
}

QVector3D QQuick3DParticleEmitter::particleRotation() const
{
    return m_particleRotation;
}

void QQuick3DParticleEmitter::setParticleRotation(const QVector3D &particleRotation)
{
    if (m_particleRotation == particleRotation)
        return;

    m_particleRotation = particleRotation;
    Q_EMIT particleRotationChanged();
}

QVector3D QQuick3DParticleEmitter::particleRotationVariation() const
{
    return m_particleRotationVariation;
}

void QQuick3DParticleEmitter::setParticleRotationVariation(const QVector3D &particleRotationVariation)
{
    if (m_particleRotationVariation == particleRotationVariation)
        return;

    m_particleRotationVariation = particleRotationVariation;
    Q_EMIT particleRotationVariationChanged();
}

QVector3D QQuick3DParticleEmitter::particleRotationVelocity() const
{
    return m_particleRotationVelocity;
}

void QQuick3DParticleEmitter::setParticleRotationVelocity(const QVector3D &particleRotationVelocity)
{
    if (m_particleRotationVelocity == particleRotationVelocity)
        return;

    m_particleRotationVelocity = particleRotationVelocity;
    Q_EMIT particleRotationVelocityChanged();
}

QVector3D QQuick3DParticleEmitter::particleRotationVelocityVariation() const
{
    return m_particleRotationVelocityVariation;
}

void QQuick3DParticleEmitter::setParticleRotationVelocityVariation(const QVector3D &particleRotationVelocityVariation)
{
    if (m_particleRotationVelocityVariation == particleRotationVelocityVariation)
        return;

    m_particleRotationVelocityVariation = particleRotationVelocityVariation;
    Q_EMIT particleRotationVariationVelocityChanged();
}

// Called to reset when system stop/continue
void QQuick3DParticleEmitter::reset()
{
    m_prevEmitTime = 0;
    m_unemittedF = 0.0f;
    m_burstAmount = 0;
}

void QQuick3DParticleEmitter::burst(int count)
{
    m_burstAmount += count;
    emitParticles();
}

void QQuick3DParticleEmitter::generateEmitBursts()
{
    if (!m_system)
        return;

    for (auto particle : qAsConst(m_system->m_particles)) {
        if (particle != m_particle)
            continue;

        // Generating burst causes all particle data reseting
        // as bursts take first particles in the list.
        particle->reset();

        // TODO: In trail emitter case centerPos should be calculated
        // taking into account each particle position at emitburst time
        QVector3D centerPos = position();

        for (auto emitBurst : qAsConst(m_emitBursts)) {
            int emitAmount = emitBurst->amount();
            if (emitAmount <= 0)
                return;
            // Distribute start times between burst time and time+duration.
            float startTime = (emitBurst->time() / 1000.0);
            float timeStep = (emitBurst->duration() / 1000.0) / emitAmount;
            for (int i = 0; i < emitAmount; i++) {
                emitParticle(particle, startTime, centerPos);
                startTime += timeStep;
            }
            // Increase burst index (for statically allocated particles)
            particle->updateBurstIndex(emitBurst->amount());
        }
    }
    m_burstGenerated = true;
}

void QQuick3DParticleEmitter::registerEmitBurst(QQuick3DParticleEmitBurst* emitBurst)
{
    if (m_emitBursts.contains(emitBurst))
        m_emitBursts.removeAll(emitBurst);

    m_emitBursts << emitBurst;
    m_burstGenerated = false;
}

void QQuick3DParticleEmitter::unRegisterEmitBurst(QQuick3DParticleEmitBurst* emitBurst)
{
    if (m_emitBursts.contains(emitBurst))
        m_emitBursts.removeAll(emitBurst);
    m_burstGenerated = false;
}

void QQuick3DParticleEmitter::emitParticle(QQuick3DParticle *particle, float startTime, const QVector3D &centerPos)
{
    int particleIndex = particle->nextCurrentIndex();

    auto d = &particle->m_particleData[particleIndex];
    *d = m_clearData; // Reset the data as it might be reused
    d->index = particleIndex;
    d->startTime = startTime;

    // Life time in seconds
    float lifeSpanMs = m_lifeSpanVariation / 1000.0;
    float lifeSpanVariationMs = lifeSpanMs - 2 * QPRand::get() * lifeSpanMs;
    d->lifetime = (m_lifeSpan / 1000.0) + lifeSpanVariationMs;

    // Size
    float sVar = m_particleScaleVariation - 2 * QPRand::get() * m_particleScaleVariation;
    float endScale = m_particleEndScale < 0 ? m_particleScale : m_particleEndScale;
    d->startSize = std::max(0.0f, float(m_particleScale + sVar));
    d->endSize = std::max(0.0f, float(endScale + sVar));

    // Emiting area/shape
    if (m_shape) {
        d->startPosition = centerPos + m_shape->randomPosition();
    } else {
        // When shape is not set, default to node center point.
        d->startPosition = centerPos;
    }

    // Velocity
    if (m_velocity)
        d->startVelocity = m_velocity->sample(d->startPosition);

    // Rotation
    if (!m_particleRotation.isNull() || !m_particleRotationVariation.isNull()) {
        Vector3b rot;
        float step = 127.0f/360.0f;
        rot.x = m_particleRotation.x() * step;
        rot.y = m_particleRotation.y() * step;
        rot.z = m_particleRotation.z() * step;
        rot.x += (m_particleRotationVariation.x() - 2 * QPRand::get() * m_particleRotationVariation.x()) * step;
        rot.y += (m_particleRotationVariation.y() - 2 * QPRand::get() * m_particleRotationVariation.y()) * step;
        rot.z += (m_particleRotationVariation.z() - 2 * QPRand::get() * m_particleRotationVariation.z()) * step;
        d->startRotation = rot;
    }
    // Rotation velocity
    if (!m_particleRotationVelocity.isNull() || !m_particleRotationVelocityVariation.isNull()) {
        float rotVelX = m_particleRotationVelocity.x();
        float rotVelY = m_particleRotationVelocity.y();
        float rotVelZ = m_particleRotationVelocity.z();
        rotVelX += (m_particleRotationVelocityVariation.x() - 2 * QPRand::get() * m_particleRotationVelocityVariation.x());
        rotVelY += (m_particleRotationVelocityVariation.y() - 2 * QPRand::get() * m_particleRotationVelocityVariation.y());
        rotVelZ += (m_particleRotationVelocityVariation.z() - 2 * QPRand::get() * m_particleRotationVelocityVariation.z());
        // Particle data rotations are in char vec3 to save memory, consider if this is worth it.
        // max value 127*127 = 16129 degrees/second
        float sign;
        sign = rotVelX < 0 ? -1.0f : 1.0f;
        rotVelX = std::max(-127.0f, std::min(127.0f, sign * std::sqrt(abs(rotVelX))));
        sign = rotVelY < 0 ? -1.0f : 1.0f;
        rotVelY = std::max(-127.0f, std::min(127.0f, sign * std::sqrt(abs(rotVelY))));
        sign = rotVelZ < 0 ? -1.0f : 1.0f;
        rotVelZ = std::max(-127.0f, std::min(127.0f, sign * std::sqrt(abs(rotVelZ))));
        d->startRotationVelocity = { char(rotVelX), char(rotVelY), char(rotVelZ) };
    }

    // Colors
    QColor pc = particle->color();
    QVector4D pcv = particle->colorVariation();
    uchar r = pc.red() * (1.0 - pcv.x()) + int(QPRand::get() * 256) * pcv.x();
    uchar g = pc.green() * (1.0 - pcv.y()) + int(QPRand::get() * 256) * pcv.y();
    uchar b = pc.blue() * (1.0 - pcv.z()) + int(QPRand::get() * 256) * pcv.z();
    uchar a = pc.alpha() * (1.0 - pcv.w()) + int(QPRand::get() * 256) * pcv.w();
    d->startColor = {r, g, b, a};
}

int QQuick3DParticleEmitter::getEmitAmount()
{
    if (!m_system)
        return 0;

    if (!m_enabled)
        return 0;

    float timeChange = m_system->timeInt - m_prevEmitTime;

    float emitAmountF = timeChange / (1000.0 / m_emitRate);

    if (m_burstAmount > 0) {
        if (emitAmountF < 1.0f) {
            // If currently only emitting burst, don't distribute particles
            // between previous emit time as that can be long time ago.
            m_prevEmitTime = m_system->timeInt;
        }
        emitAmountF += m_burstAmount;
        m_burstAmount = 0;
    }

    int emitAmount = floorf(emitAmountF);
    // Store the partly unemitted particles
    if (emitAmount > 0)
        m_unemittedF += (emitAmountF - emitAmount);

    // When unemitted grow to a full particle, emit it
    // This way if emit rate is 140 emitAmounts can be e.g. 2,2,3,2,2,3 etc.
    if (m_unemittedF >= 1.0) {
        emitAmount += m_unemittedF;
        m_unemittedF--;
    }
    return emitAmount;
}

// Called to emit set of particles
void QQuick3DParticleEmitter::emitParticles()
{
    if (!m_system)
        return;

    if (!m_enabled)
        return;

    // If bursts have changed, generate them first in the beginning
    if (!m_burstGenerated)
       generateEmitBursts();

    int emitAmount = getEmitAmount();

    // With lower emitRates, let timeChange grow until at least 1 particle is emitted
    if (emitAmount < 1)
            return;

    QVector3D centerPos = position();

    for (auto particle : qAsConst(m_system->m_particles)) {
        if (particle == m_particle) {
            emitAmount = std::min(emitAmount, int(particle->maxAmount()));
            for (int i = 0; i < emitAmount; i++) {
                // Distribute evenly between previous and current time, important especially
                // when time has jumped a lot (like a starttime).
                float startTime = (m_prevEmitTime / 1000.0) + (float(1+i) / emitAmount) * ((m_system->timeInt - m_prevEmitTime) / 1000.0);
                emitParticle(particle, startTime, centerPos);
            }
        }
    }

    m_prevEmitTime = m_system->timeInt;
}

void QQuick3DParticleEmitter::componentComplete()
{
    if (!m_system && qobject_cast<QQuick3DParticleSystem*>(parentItem()))
        setSystem(qobject_cast<QQuick3DParticleSystem*>(parentItem()));
    QQuick3DNode::componentComplete();
}

// EmitBursts - list handling

QQmlListProperty<QQuick3DParticleEmitBurst> QQuick3DParticleEmitter::emitBursts()
{
    return {this, this,
             &QQuick3DParticleEmitter::appendEmitBurst,
             &QQuick3DParticleEmitter::emitBurstCount,
             &QQuick3DParticleEmitter::emitBurst,
             &QQuick3DParticleEmitter::clearEmitBursts,
             &QQuick3DParticleEmitter::replaceEmitBurst,
             &QQuick3DParticleEmitter::removeLastEmitBurst};
}

void QQuick3DParticleEmitter::appendEmitBurst(QQuick3DParticleEmitBurst* n) {
    m_emitBursts.append(n);
}

qsizetype QQuick3DParticleEmitter::emitBurstCount() const
{
    return m_emitBursts.count();
}

QQuick3DParticleEmitBurst *QQuick3DParticleEmitter::emitBurst(qsizetype index) const
{
    return m_emitBursts.at(index);
}

void QQuick3DParticleEmitter::clearEmitBursts() {
    m_emitBursts.clear();
}

void QQuick3DParticleEmitter::replaceEmitBurst(qsizetype index, QQuick3DParticleEmitBurst *n)
{
    m_emitBursts[index] = n;
}

void QQuick3DParticleEmitter::removeLastEmitBurst()
{
    m_emitBursts.removeLast();
}

// EmitBursts - static
void QQuick3DParticleEmitter::appendEmitBurst(QQmlListProperty<QQuick3DParticleEmitBurst>* list, QQuick3DParticleEmitBurst* p) {
    reinterpret_cast< QQuick3DParticleEmitter* >(list->data)->appendEmitBurst(p);
}

void QQuick3DParticleEmitter::clearEmitBursts(QQmlListProperty<QQuick3DParticleEmitBurst>* list) {
    reinterpret_cast< QQuick3DParticleEmitter* >(list->data)->clearEmitBursts();
}

void QQuick3DParticleEmitter::replaceEmitBurst(QQmlListProperty<QQuick3DParticleEmitBurst> *list, qsizetype i, QQuick3DParticleEmitBurst *p)
{
    reinterpret_cast< QQuick3DParticleEmitter* >(list->data)->replaceEmitBurst(i, p);
}

void QQuick3DParticleEmitter::removeLastEmitBurst(QQmlListProperty<QQuick3DParticleEmitBurst> *list)
{
    reinterpret_cast< QQuick3DParticleEmitter* >(list->data)->removeLastEmitBurst();
}

QQuick3DParticleEmitBurst* QQuick3DParticleEmitter::emitBurst(QQmlListProperty<QQuick3DParticleEmitBurst>* list, qsizetype i) {
    return reinterpret_cast< QQuick3DParticleEmitter* >(list->data)->emitBurst(i);
}

qsizetype QQuick3DParticleEmitter::emitBurstCount(QQmlListProperty<QQuick3DParticleEmitBurst>* list) {
    return reinterpret_cast< QQuick3DParticleEmitter* >(list->data)->emitBurstCount();
}

QT_END_NAMESPACE
