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

#include "qquick3dparticletrailemitter_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype TrailEmitter3D
    \inherits ParticleEmitter3D
    \inqmlmodule QtQuick3D.Particles3D
    \brief Emitter for logical particles from other particles.
    \since 6.2

    TrailEmitter3D is a special emitter for emitting particles with starting positions inherited from
    other logical particles.
*/

QQuick3DParticleTrailEmitter::QQuick3DParticleTrailEmitter(QQuick3DNode *parent)
    : QQuick3DParticleEmitter(parent)
{
}

/*!
    \qmlproperty Particle3D TrailEmitter3D::follow

    This property defines the logical particle which this emitter follows.
    When the TrailEmitter3D emits particles, center position of those particles
    will become from the \l Particle3D the emitter follows.
*/
QQuick3DParticle *QQuick3DParticleTrailEmitter::follow() const
{
    return m_follow;
}

void QQuick3DParticleTrailEmitter::setFollow(QQuick3DParticle *follow)
{
    if (m_follow == follow)
        return;

    m_follow = follow;
    Q_EMIT followChanged();
}

/*!
    \qmlmethod vector3d TrailEmitter3D::burst(int count)

    This method emits \a count amount of particles from this emitter immediately.

    \note TrailEmitter3D doesn't support other bursting methods. Position always comes
    from the particle defined with the \l follow property.
*/
void QQuick3DParticleTrailEmitter::burst(int count)
{
    if (!system())
        return;
    QQuick3DParticleEmitBurstData burst;
    burst.time = system()->currentTime();
    burst.amount = count;
    m_bursts << burst;
}

bool QQuick3DParticleTrailEmitter::hasBursts() const
{
    return !m_bursts.isEmpty();
}

// Called to emit set of particles
void QQuick3DParticleTrailEmitter::emitTrailParticles(QQuick3DParticleDataCurrent *d, int emitAmount)
{
    if (!system())
        return;

    if (!enabled())
        return;

    const int systemTime = system()->currentTime();
    QVector3D centerPos = d->position;

    for (auto particle : qAsConst(m_system->m_particles)) {
        if (particle == m_particle) {
            emitAmount = std::min(emitAmount, int(particle->maxAmount()));
            for (int i = 0; i < emitAmount; i++) {
                // Distribute evenly between previous and current time, important especially
                // when time has jumped a lot (like a starttime).
                float startTime = (m_prevEmitTime / 1000.0f) + (float(1 + i) / emitAmount) * ((systemTime - m_prevEmitTime) / 1000.0f);
                emitParticle(particle, startTime, QMatrix4x4(), QQuaternion(), centerPos);
            }
            // Emit bursts, if any
            for (auto burst : qAsConst(m_bursts)) {
                int burstAmount = std::min(burst.amount, int(particle->maxAmount()));
                float burstTime = float(burst.time / 1000.0f);
                for (int i = 0; i < burstAmount; i++)
                    emitParticle(particle, burstTime, QMatrix4x4(), QQuaternion(), centerPos);
            }
        }
    }

    m_prevEmitTime = systemTime;
}

void QQuick3DParticleTrailEmitter::clearBursts()
{
    // After bursts have been emitted, clear the list
    m_bursts.clear();
}

QT_END_NAMESPACE
