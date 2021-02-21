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

#include <QtQuick3D/private/qquick3dquaternionutils_p.h>
#include "qquick3dparticlesystem_p.h"
#include "qquick3dparticleemitter_p.h"
#include "qquick3dparticletrailemitter_p.h"
#include "qquick3dparticlemodelparticle_p.h"
#include "qquick3dparticleaffector_p.h"
#include <private/qqmldelegatemodel_p.h>
#include "qquick3dparticlerandomizer_p.h"
#include "qquick3dparticlespriteparticle_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype ParticleSystem3D
    \inherits Node
    \inqmlmodule QtQuick3D.Particles3D
    \brief A system which includes particle, emitter, and affector types.

    This element is the root of the particle system, which handles the system timing and groups all
    the other related elements like particles, emitters, and affectors together. To group the system
    elements, they either need to be direct children of the ParticleSystem3D like this:

    \qml,
    ParticleSystem3D {
        ParticleEmitter3D {
            ...
        }
        SpriteParticle3D {
            ...
        }
    }
    \endqml

    Or if the system elements are not direct children, they need to use \c system property to point
    which ParticleSystem3D they belong to. Like this:

    \qml
    ParticleSystem3D {
        id: psystem
    }
    ParticleEmitter3D {
        system: psystem
        ...
    }
    SpriteParticle3D {
        system: psystem
        ...
    }
    \endqml
*/

QQuick3DParticleSystem::QQuick3DParticleSystem(QQuick3DNode *parent)
    : QQuick3DNode(parent)
    , m_running(true)
    , m_paused(false)
    , m_initialized(false)
    , m_componentComplete(false)
    , m_animation(nullptr)
    , m_logging(false)
    , m_loggingData(new QQuick3DParticleSystemLogging(this))
{
    connect(m_loggingData, &QQuick3DParticleSystemLogging::loggingIntervalChanged, [this]() {
        m_loggingTimer.setInterval(m_loggingData->m_loggingInterval);
    });

}

QQuick3DParticleSystem::~QQuick3DParticleSystem()
{
    if (m_animation)
        m_animation->stop();
    for (const auto &connection : qAsConst(m_connections))
        QObject::disconnect(connection);
    // purposeful copy
    const auto particles = m_particles;
    const auto emitters = m_emitters;
    const auto trailEmitters = m_trailEmitters;
    const auto affectors = m_affectors;
    for (auto *particle : particles)
        particle->setSystem(nullptr);
    for (auto *emitter : emitters)
        emitter->setSystem(nullptr);
    for (auto *emitter : trailEmitters)
        emitter->setSystem(nullptr);
    for (auto *affector : affectors)
        affector->setSystem(nullptr);
}

/*!
    \qmlproperty bool ParticleSystem3D::running

    This property defines if system is currently running. If running is set to \c false,
    the particle system will stop the simulation. All particles will be destroyed when
    the system is set to running again.

    Running should be set to \c false when manually modifying/animating the \l {ParticleSystem3D::time}{time} property.

    The default value is \c true.
*/
bool QQuick3DParticleSystem::isRunning() const
{
    return m_running;
}

/*!
    \qmlproperty bool ParticleSystem3D::paused

    This property defines if system is currently paused. If paused is set to \c true, the
    particle system will not advance the simulation. When paused is set to \c false again,
    the simulation will resume from the same point where it was paused.

    The default value is \c false.
*/
bool QQuick3DParticleSystem::isPaused() const
{
    return m_paused;
}

/*!
    \qmlproperty int ParticleSystem3D::startTime

    This property defines time in milliseconds where the system starts. This can be useful
    to warm up the system so that a set of particles has already been emitted. If for example
    \l startTime is set to 2000 and system \l time is animating from 0 to 1000, actually
    animation shows particles from 2000 to 3000ms.

    The default value is \c 0.
*/
int QQuick3DParticleSystem::startTime() const
{
    return m_startTime;
}

/*!
    \qmlproperty int ParticleSystem3D::time

    This property defines time in milliseconds for the system.
    \note When modifying the time property, \l {ParticleSystem3D::running}{running}
    should usually be set to \c false.

    Here is an example how to manually animate the system for 3 seconds, in a loop, at half speed:

    \qml
    ParticleSystem3D {
        running: false
        NumberAnimation on time {
            loops: Animation.Infinite
            from: 0
            to: 3000
            duration: 6000
        }
    }
    \endqml
*/
int QQuick3DParticleSystem::time() const
{
    return m_time;
}

/*!
    \qmlproperty bool ParticleSystem3D::useRandomSeed

    This property defines if particle system seed should be random or user defined.
    When \c true, a new random value for \l {ParticleSystem3D::seed}{seed} is generated every time particle
    system is restarted.

    The default value is \c true.

    \note This property should not be modified during the particle animations.

    \sa seed
*/
bool QQuick3DParticleSystem::useRandomSeed() const
{
    return m_useRandomSeed;
}

/*!
    \qmlproperty int ParticleSystem3D::seed

    This property defines the seed value used for particles randomization. With the same seed,
    particles effect will be identical on every run. This is useful when deterministic behavior
    is desired over random behavior.

    The default value is \c 0 when \l {ParticleSystem3D::useRandomSeed}{useRandomSeed} is set to
    \c false, and something in between \c 1..INT32_MAX when \l {ParticleSystem3D::useRandomSeed}{useRandomSeed}
    is set to \c true.

    \note This property should not be modified during the particle animations.

    \sa useRandomSeed
*/
int QQuick3DParticleSystem::seed() const
{
    return m_seed;
}

/*!
    \qmlproperty bool ParticleSystem3D::logging

    Set this to true to collect \l {ParticleSystem3D::loggingData}{loggingData}.

    \note This property has some performance impact, so it should not be enabled in releases.

    The default value is \c false.

    \sa loggingData
*/
bool QQuick3DParticleSystem::logging() const
{
    return m_logging;
}

/*!
    \qmlproperty ParticleSystem3DLogging ParticleSystem3D::loggingData
    \readonly

    This property contains logging data which can be useful when developing and optimizing
    the particle effects.

    \note This property contains correct data only when \l {ParticleSystem3D::logging}{logging} is set
    to \c true and particle system is running.

    \sa logging
*/
QQuick3DParticleSystemLogging *QQuick3DParticleSystem::loggingData() const
{
    return m_loggingData;
}

void QQuick3DParticleSystem::setRunning(bool arg)
{
    if (m_running != arg) {
        m_running = arg;
        Q_EMIT runningChanged();
        setPaused(false);

        if (m_running) {
            for (auto emitter : qAsConst(m_emitters))
                emitter->reset();
            for (auto emitter : qAsConst(m_trailEmitters))
                emitter->reset();
            for (auto particle : qAsConst(m_particles))
                particle->reset();
        }

        if (!m_running && m_useRandomSeed)
            doSeedRandomization();

        if (m_animation)
            m_running ? m_animation->start() : m_animation->stop();
    }
}

void QQuick3DParticleSystem::setPaused(bool arg) {
    if (m_paused != arg) {
        m_paused = arg;
        if (m_animation && m_animation->state() != QAbstractAnimation::Stopped)
            m_paused ? m_animation->pause() : m_animation->resume();
        Q_EMIT pausedChanged();
    }
}

void QQuick3DParticleSystem::setStartTime(int startTime)
{
    if (m_startTime == startTime)
        return;

    m_startTime = startTime;
    Q_EMIT startTimeChanged();
}

void QQuick3DParticleSystem::setTime(int time)
{
    if (m_time == time)
        return;

    m_time = time;

    updateCurrentTime(m_time + m_startTime);

    Q_EMIT timeChanged();
}

void QQuick3DParticleSystem::setUseRandomSeed(bool randomize)
{
    if (m_useRandomSeed == randomize)
        return;

    m_useRandomSeed = randomize;
    // When set to true, random values are recalculated with a random seed
    // and random values will become independent of particle index when possible.
    if (m_useRandomSeed)
        doSeedRandomization();
    m_rand.setDeterministic(!m_useRandomSeed);
    Q_EMIT useRandomSeedChanged();
}

void QQuick3DParticleSystem::setSeed(int seed)
{
    if (m_seed == seed)
        return;

    m_seed = seed;
    m_rand.init(m_seed);
    Q_EMIT seedChanged();
}

void QQuick3DParticleSystem::setLogging(bool logging)
{
    if (m_logging == logging)
        return;

    m_logging = logging;

    resetLoggingVariables();
    m_loggingData->resetData();

    if (m_logging) {
        m_loggingTimer.start();
    } else {
        m_loggingTimer.stop();
    }
    Q_EMIT loggingChanged();
}

void QQuick3DParticleSystem::componentComplete()
{
    QQuick3DNode::componentComplete();
    m_componentComplete = true;
    m_animation = new QQuick3DParticleSystemAnimation(this);

    connect(&m_loggingTimer, &QTimer::timeout, this, &QQuick3DParticleSystem::updateLoggingData);
    m_loggingTimer.setInterval(m_loggingData->m_loggingInterval);

    if (m_useRandomSeed)
        doSeedRandomization();
    else
        m_rand.init(m_seed);

    reset();//restarts animation as well
}

void QQuick3DParticleSystem::reset()
{
    if (!m_componentComplete)
        return;

    timeInt = 0;

    // TODO: Reset particles & emitters

    if (m_animation) {
        //reset restarts animation (if running)
        if ((m_animation->state() == QAbstractAnimation::Running))
            m_animation->stop();
        if (m_running)
            m_animation->start();
        if (m_paused)
            m_animation->pause();
    }

    m_initialized = true;
}

void QQuick3DParticleSystem::refresh() {
    // Call with the same time
    updateCurrentTime(timeInt);
}

int QQuick3DParticleSystem::particleCount() const
{
    int pCount = 0;
    for (auto particle : qAsConst(m_particles)) {
        pCount += particle->maxAmount();
    }
    return pCount;
}

void QQuick3DParticleSystem::registerParticle(QQuick3DParticle *particle)
{
    auto *model = qobject_cast<QQuick3DParticleModelParticle *>(particle);
    if (model) {
        registerParticleModel(model);
        return;
    }
    auto *sprite = qobject_cast<QQuick3DParticleSpriteParticle *>(particle);
    if (sprite) {
        registerParticleSprite(sprite);
        return;
    }
}

void QQuick3DParticleSystem::registerParticleModel(QQuick3DParticleModelParticle* m)
{
    m_modelParticles << m;
    m_particles << m;
}

void QQuick3DParticleSystem::registerParticleSprite(QQuick3DParticleSpriteParticle* m)
{
    m_spriteParticles << m;
    m_particles << m;
}

void QQuick3DParticleSystem::unRegisterParticle(QQuick3DParticle *particle)
{
    auto *model = qobject_cast<QQuick3DParticleModelParticle *>(particle);
    if (model) {
        m_modelParticles.removeAll(particle);
        m_particles.removeAll(particle);
        return;
    }
    auto *sprite = qobject_cast<QQuick3DParticleSpriteParticle *>(particle);
    if (sprite) {
        m_spriteParticles.removeAll(particle);
        m_particles.removeAll(particle);
        return;
    }
}

void QQuick3DParticleSystem::registerParticleEmitter(QQuick3DParticleEmitter* e)
{
    auto te = qobject_cast<QQuick3DParticleTrailEmitter*>(e);
    if (te)
        m_trailEmitters << te;
    else
        m_emitters << e;
}

void QQuick3DParticleSystem::unRegisterParticleEmitter(QQuick3DParticleEmitter* e)
{
    auto te = qobject_cast<QQuick3DParticleTrailEmitter*>(e);
    if (te)
        m_trailEmitters.removeAll(te);
    else
        m_emitters.removeAll(e);
}

void QQuick3DParticleSystem::registerParticleAffector(QQuick3DParticleAffector* a)
{
    m_affectors << a;
    m_connections.insert(a, connect(a, &QQuick3DParticleAffector::update, this, &QQuick3DParticleSystem::refresh));
}

void QQuick3DParticleSystem::unRegisterParticleAffector(QQuick3DParticleAffector* a)
{
    QObject::disconnect(m_connections[a]);
    m_connections.remove(a);
    m_affectors.removeAll(a);
}

void QQuick3DParticleSystem::updateCurrentTime(int currentTime)
{
    if (!m_initialized)
        return;

    timeInt = currentTime;
    float timeS = float(timeInt / 1000.0);

    m_particlesMax = 0;
    m_particlesUsed = 0;
    m_updates++;

    m_perfTimer.restart();

    // Emit new particles
    for (auto emitter : qAsConst(m_emitters)) {
        emitter->emitParticles();
    }

    // Animate current particles
    for (auto particle : qAsConst(m_particles)) {
        QQuick3DParticleModelParticle *modelParticle = qobject_cast<QQuick3DParticleModelParticle *>(particle);
        QQuick3DParticleSpriteParticle *spriteParticle = qobject_cast<QQuick3DParticleSpriteParticle *>(particle);

        // Collect possible trail emits
        QVector<TrailEmits> trailEmits;
        for (auto emitter : qAsConst(m_trailEmitters)) {
            if (emitter->follow() == particle) {
                int emitAmount = emitter->getEmitAmount();
                if (emitAmount > 0 || emitter->hasBursts()) {
                    TrailEmits e;
                    e.emitter = emitter;
                    e.amount = emitAmount;
                    trailEmits << e;
                }
            }
        }
        int c = particle->maxAmount();

        m_particlesMax += c;

        bool semiTransparent = false;
        if (modelParticle)
            modelParticle->clearInstanceTable();
        for (int i = 0; i < c; i++) {
            auto d = &particle->m_particleData[i];

            float particleTimeEnd = d->startTime + d->lifetime;

            if (timeS < d->startTime || timeS > particleTimeEnd) {
                // Particle not alive currently
                if (spriteParticle) {
                    spriteParticle->setParticleData(i, {}, {},
                                                    {},
                                                    0.0f, 0.0f);
                }
                continue;
            } else {
                // Required currently so that particles come back after moving time backwards
                // visibility is determined by the contents of the instancing table
                m_particlesUsed++;
            }

            float particleTimeS = timeS - d->startTime;
            float particleTimeLeftS = d->lifetime - particleTimeS;

            QQuick3DParticleDataCurrent currentData;
            currentData.position = d->startPosition;
            currentData.color = d->startColor;

            // Initial position from start velocity
            currentData.position += d->startVelocity * particleTimeS;

            // Initial rotation from start velocity
            Vector3b rot = d->startRotation;
            const float step = 360.0f / 127.0f;
            currentData.rotation = QVector3D(rot.x * step, rot.y * step, rot.z * step);
            currentData.rotation.setX(currentData.rotation.x() + abs(d->startRotationVelocity.x) * d->startRotationVelocity.x * particleTimeS);
            currentData.rotation.setY(currentData.rotation.y() + abs(d->startRotationVelocity.y) * d->startRotationVelocity.y * particleTimeS);
            currentData.rotation.setZ(currentData.rotation.z() + abs(d->startRotationVelocity.z) * d->startRotationVelocity.z * particleTimeS);

            // Affectors
            for (auto affector : qAsConst(m_affectors)) {
                // If affector is set to affect only particular particles, check these are included
                if (affector->m_enabled && (affector->m_particles.isEmpty() || affector->m_particles.contains(particle)))
                    affector->affectParticle(*d, &currentData, particleTimeS);
            }

            // Add a base rotation if alignment requested
            if (!spriteParticle || !spriteParticle->m_billboard) {
                if (particle->m_alignMode == QQuick3DParticle::AlignTowardsTarget) {
                    QQuaternion alignQuat = QQuick3DQuaternionUtils::lookAt(particle->alignTargetPosition(), currentData.position);
                    currentData.rotation = (alignQuat * QQuaternion::fromEulerAngles(currentData.rotation)).toEulerAngles();
                } else if (particle->m_alignMode == QQuick3DParticle::AlignTowardsStartVelocity) {
                    QQuaternion alignQuat = QQuick3DQuaternionUtils::lookAt(d->startVelocity, QVector3D());
                    currentData.rotation = (alignQuat * QQuaternion::fromEulerAngles(currentData.rotation)).toEulerAngles();
                }
            }
/*
            // Clip, this should be always last, when position doesn't change anymore
            QVector3D clipCenter(10,10,10);
            float clipSize = 200;
            if (currentData.position.distanceToPoint(clipCenter) < clipSize) {
                //p->setVisible(true);
                currentData.color.r = 255;
                currentData.color.g = 0;
                currentData.color.b = 0;
                currentData.color.a = 255;
            } else {
                //p->setVisible(false);
                currentData.color.r = 100;
                currentData.color.g = 100;
                currentData.color.b = 100;
                currentData.color.a = 100;
            }
*/
            // 0.0 -> 1.0 during the particle lifetime
            float timeChange = particleTimeS / d->lifetime;
            timeChange = std::max(0.0f, std::min(1.0f, timeChange));
            // Scale from initial to endScale
            if (modelParticle)
                currentData.scale = modelParticle->m_initialScale * d->endSize * (timeChange) + modelParticle->m_initialScale * d->startSize * (1.0 - timeChange);
            else {
                float scale = d->endSize * (timeChange) + d->startSize * (1.0 - timeChange);
                currentData.scale = QVector3D(scale, scale, scale);
            }

            // Fade in & out
            float fadeInS = particle->m_fadeInDuration / 1000.0;
            float fadeOutS = particle->m_fadeOutDuration / 1000.0;
            if (particleTimeS < fadeInS) {
                // 0.0 -> 1.0 during the particle fadein
                float fadeIn = particleTimeS / fadeInS;
                if (particle->m_fadeInEffect == QQuick3DParticleModelParticle::FadeOpacity)
                    currentData.color.a *= fadeIn;
                else if (particle->m_fadeInEffect == QQuick3DParticleModelParticle::FadeScale)
                    currentData.scale *= fadeIn;
            }
            if (particleTimeLeftS < fadeOutS) {
                // 1.0 -> 0.0 during the particle fadeout
                float fadeOut = particleTimeLeftS / fadeOutS;
                if (particle->m_fadeOutEffect == QQuick3DParticleModelParticle::FadeOpacity)
                    currentData.color.a *= fadeOut;
                else if (particle->m_fadeOutEffect == QQuick3DParticleModelParticle::FadeScale)
                    currentData.scale *= fadeOut;
            }

            QColor color(currentData.color.r, currentData.color.g, currentData.color.b, currentData.color.a);
            // Set current particle properties
            if (modelParticle)
                modelParticle->addInstance(currentData.position, currentData.scale, currentData.rotation, color);
            if (spriteParticle) {
                spriteParticle->setParticleData(i, currentData.position, currentData.rotation,
                                                QVector4D(color.redF(), color.greenF(), color.blueF(), color.alphaF()),
                                                currentData.scale.x(), timeChange);
            }
            if (currentData.color.a != 255)
                semiTransparent = true;

            // Emit new particles from trails
            for (auto trailEmit : qAsConst(trailEmits))
                trailEmit.emitter->emitTrailParticles(&currentData, trailEmit.amount);
        }
        if (modelParticle) {
            modelParticle->setHasTransparency(semiTransparent);
            modelParticle->commitInstance();
        }
        if (spriteParticle)
            spriteParticle->commitParticles();
    }

    // Clear bursts from trailemitters
    for (auto emitter : qAsConst(m_trailEmitters))
        emitter->clearBursts();

    m_timeAnimation += m_perfTimer.nsecsElapsed();
}

void QQuick3DParticleSystem::updateLoggingData()
{
    if (m_updates == 0)
        return;

    if (m_loggingData->m_particlesMax != m_particlesMax) {
        m_loggingData->m_particlesMax = m_particlesMax;
        Q_EMIT m_loggingData->particlesMaxChanged();
    }
    if (m_loggingData->m_particlesUsed != m_particlesUsed) {
        m_loggingData->m_particlesUsed = m_particlesUsed;
        Q_EMIT m_loggingData->particlesUsedChanged();
    }
    if (m_loggingData->m_updates != m_updates) {
        m_loggingData->m_updates = m_updates;
        Q_EMIT m_loggingData->updatesChanged();
    }

    m_loggingData->updateTimes(m_timeAnimation);

    Q_EMIT loggingDataChanged();
    resetLoggingVariables();
}

void QQuick3DParticleSystem::resetLoggingVariables()
{
    m_particlesMax = 0;
    m_particlesUsed = 0;
    m_updates = 0;
    m_timeAnimation = 0;
}

QPRand *QQuick3DParticleSystem::rand() {
    return &m_rand;
}

void QQuick3DParticleSystem::doSeedRandomization()
{
    // Random 1..INT32_MAX, making sure seed changes from the initial 0.
    setSeed(QRandomGenerator::global()->bounded(1 + (INT32_MAX - 1)));
}

QT_END_NAMESPACE
