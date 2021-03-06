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

#include "qquick3dparticlegravity_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype Gravity3D
    \inherits Affector3D
    \inqmlmodule QtQuick3D.Particles3D
    \brief Accelerates particles to a vector of the specified magnitude in the specified direction.

    This element models the gravity of a massive object whose center of gravity is far away (and thus the
    gravitational pull is effectively constant across the scene). To model the gravity of an object near
    or inside the scene, use \l Attractor3D.
*/

QQuick3DParticleGravity::QQuick3DParticleGravity(QQuick3DNode *parent)
    : QQuick3DParticleAffector(parent)
{
}

/*!
    \qmlproperty real Gravity3D::magnitude

    This property defines the magnitude in position change per second. Negative magnitude
    accelerates the opposite way from the \l {Gravity3D::direction}{direction}.

    The default value is \c 100.0.
*/
float QQuick3DParticleGravity::magnitude() const
{
    return m_magnitude;
}

void QQuick3DParticleGravity::setMagnitude(float magnitude)
{
    if (qFuzzyCompare(m_magnitude, magnitude))
        return;

    m_magnitude = magnitude;
    Q_EMIT magnitudeChanged();
    update();
}

/*!
    \qmlproperty vector3d Gravity3D::direction

    This property defines the direction the gravity will affect toward. Values will be
    automatically normalized to a unit vector.

    The default value is \c (0.0, -1.0, 0.0) (downwards).
*/
const QVector3D &QQuick3DParticleGravity::direction() const
{
    return m_direction;
}

void QQuick3DParticleGravity::setDirection(const QVector3D &direction)
{
    if (m_direction == direction)
        return;

    m_direction = direction;
    m_directionNormalized = m_direction.normalized();
    Q_EMIT directionChanged();
    update();
}

void QQuick3DParticleGravity::affectParticle(const QQuick3DParticleData &sd, QQuick3DParticleDataCurrent *d, float time)
{
    Q_UNUSED(sd);
    float velocity = 0.5f * m_magnitude * (time * time);
    d->position += velocity * m_directionNormalized;
}

QT_END_NAMESPACE
