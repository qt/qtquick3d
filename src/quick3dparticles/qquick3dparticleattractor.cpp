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

#include "qquick3dparticleattractor_p.h"
#include "qquick3dparticlerandomizer_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype Attractor3D
    \inherits Affector3D
    \inqmlmodule QtQuick3D.Particles3D
    \brief Attracts particles towards a position or a shape.

    This element attracts particles towards a position inside the 3D view. To model
    the gravity of a massive object whose center of gravity is far away, use \l Gravity3D.

    The attraction position is defined either with the \l {Node::position}{position} and
    \l positionVariation or with \l shape. If both are defined, \l shape is used.
*/

// Minimum duration in seconds
const float MIN_DURATION = 0.001f;

QQuick3DParticleAttractor::QQuick3DParticleAttractor(QQuick3DNode *parent)
    : QQuick3DParticleAffector(parent)
    , m_duration(-1)
    , m_durationVariation(0)
{
}

/*!
    \qmlproperty vector3d Attractor3D::positionVariation

    This property defines the variation on attract position. It can be used to not attract
    into a single point, but randomly towards a wider area. Here is an example how to attract
    particles into some random point inside (50, 50, 50) cube at position (100, 0, 0) within
    2 to 4 seconds:

    \qml
    Attractor3D {
        position: Qt.vector3d(100, 0, 0)
        positionVariation: Qt.vector3d(50, 50, 50)
        duration: 3000
        durationVariation: 1000
    }
    \endqml

    The default value is \c (0, 0, 0) (no variation).

    \sa {Node::position}, shape
*/
QVector3D QQuick3DParticleAttractor::positionVariation() const
{
    return m_positionVariation;
}

void QQuick3DParticleAttractor::setPositionVariation(const QVector3D &positionVariation)
{
    if (m_positionVariation == positionVariation)
        return;

    m_positionVariation = positionVariation;
    Q_EMIT positionVariationChanged();
    update();
}

/*!
    \qmlproperty ParticleShape3D Attractor3D::shape

    This property defines a \l ParticleShape3D for particles attraction.
    Each particle will be attracted into a random position inside this shape. This is an
    alternative for defining \l {Node::position}{position} and \l positionVariation. Here
    is an example how to attract particles into some random point inside sphere by the end
    of the particles \l {Particle3D::lifeSpan}{lifeSpan}:

    \qml
    Attractor3D {
        position: Qt.vector3d(100, 0, 0)
        shape: ParticleShape3D {
            type: ParticleShape3D.Sphere
            fill: true
        }
    }
    \endqml

    \sa {Node::position}, positionVariation
*/
QQuick3DParticleShape *QQuick3DParticleAttractor::shape() const
{
    return m_shape;
}

void QQuick3DParticleAttractor::setShape(QQuick3DParticleShape *shape)
{
    if (m_shape == shape)
        return;

    m_shape = shape;
    m_shapeDirty = true;
    Q_EMIT shapeChanged();
    update();
}

/*!
    \qmlproperty int Attractor3D::duration

    This property defines the duration in milliseconds how long it takes for particles to
    reach the attaction position. When the value is -1, particle lifeSpan is used
    as the duration.

    The default value is \c -1.
*/
int QQuick3DParticleAttractor::duration() const
{
    return m_duration;
}

void QQuick3DParticleAttractor::setDuration(int duration)
{
    if (m_duration == duration)
        return;

    m_duration = duration;
    Q_EMIT durationChanged();
    update();
}

/*!
    \qmlproperty int Attractor3D::durationVariation

    This property defines the duration variation in milliseconds. The actual duration to
    reach attractor is between \c duration - \c durationVariation and \c duration + \c durationVariation.

    The default value is \c 0 (no variation).
*/
int QQuick3DParticleAttractor::durationVariation() const
{
    return m_durationVariation;
}

void QQuick3DParticleAttractor::setDurationVariation(int durationVariation)
{
    if (m_durationVariation == durationVariation)
        return;

    m_durationVariation = durationVariation;
    Q_EMIT durationVariationChanged();
    update();
}

/*!
    \qmlproperty bool Attractor3D::hideAtEnd

    This property defines if the particle should disappear when it reaches the attractor.

    The default value is \c false.
*/
bool QQuick3DParticleAttractor::hideAtEnd() const
{
    return m_hideAtEnd;
}

void QQuick3DParticleAttractor::setHideAtEnd(bool hideAtEnd)
{
    if (m_hideAtEnd == hideAtEnd)
        return;

    m_hideAtEnd = hideAtEnd;
    Q_EMIT hideAtEndChanged();
    update();
}

void QQuick3DParticleAttractor::updateShapePositions()
{
    m_shapePositionList.clear();
    if (!m_system || !m_shape)
        return;

    m_shape->m_system = m_system;

    // Get count of particles positions needed
    int pCount = 0;
    if (!m_particles.isEmpty()) {
        for (auto p : m_particles) {
            auto pp = qobject_cast<QQuick3DParticle *>(p);
            pCount += pp->maxAmount();
        }
    } else {
        pCount = m_system->particleCount();
    }

    m_shapePositionList.reserve(pCount);
    for (int i = 0; i < pCount; i++)
        m_shapePositionList << m_shape->randomPosition(i);

    m_shapeDirty = false;
}

void QQuick3DParticleAttractor::affectParticle(const QQuick3DParticleData &sd, QQuick3DParticleDataCurrent *d, float time)
{
    if (!m_system)
        return;

    if (m_shapeDirty)
        updateShapePositions();

    auto rand = m_system->rand();
    float duration = m_duration < 0 ? sd.lifetime : (m_duration / 1000.0f);
    float durationVariation = m_durationVariation == 0
            ? 0.0f
            : (m_durationVariation / 1000.0f) - 2.0f * rand->get(sd.index, QPRand::AttractorDurationV) * (m_durationVariation / 1000.0f);
    duration = std::max(duration + durationVariation, MIN_DURATION);
    float pEnd = std::min(1.0f, std::max(0.0f, time / duration));
    // TODO: Should we support easing?
    //pEnd = easeInOutQuad(pEnd);

    if (m_hideAtEnd && pEnd >= 1.0f)
        d->color.a = 0;

    float pStart = 1.0f - pEnd;
    if (m_shape) {
        d->position = (pStart * d->position) + (pEnd * (position() + m_shapePositionList[sd.index]));
    } else {
        if (m_positionVariation.isNull()) {
            d->position =  (pStart * d->position) + (pEnd * position());
        } else {
            QVector3D pos = position();
            if (!m_positionVariation.isNull()) {
                pos.setX(pos.x() + m_positionVariation.x() - 2.0f * rand->get(sd.index, QPRand::AttractorPosVX) * m_positionVariation.x());
                pos.setY(pos.y() + m_positionVariation.y() - 2.0f * rand->get(sd.index, QPRand::AttractorPosVY) * m_positionVariation.y());
                pos.setZ(pos.z() + m_positionVariation.z() - 2.0f * rand->get(sd.index, QPRand::AttractorPosVZ) * m_positionVariation.z());
            }
            d->position =  (pStart * d->position) + (pEnd * pos);
        }
    }
}

QT_END_NAMESPACE
