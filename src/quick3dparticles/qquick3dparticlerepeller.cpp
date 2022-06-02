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

#include "qquick3dparticlerepeller_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype Repeller3D
    \inherits Affector3D
    \inqmlmodule QtQuick3D.Particles3D
    \brief Particle repeller.
    \since 6.4

    Repeller affector repels particles from the position it is placed.
*/

QQuick3DParticleRepeller::QQuick3DParticleRepeller(QQuick3DNode *parent)
    : QQuick3DParticleAffector(parent)
{

}

/*!
    \qmlproperty real Repeller3D::radius

    This property holds the radius(or inner radius) of the repeller. Particle is repelled
    at full strength when it gets inside this radius. The default value is zero.
*/
float QQuick3DParticleRepeller::radius() const
{
    return m_radius;
}

/*!
    \qmlproperty real Repeller3D::outerRadius

    This property holds the outer radius of the repeller. The particle is not affected
    until it gets inside this radius and the repel strength grows smoothly until the particle
    gets to radius distance from the repeller. The default value is 50.0;
*/
float QQuick3DParticleRepeller::outerRadius() const
{
    return m_outerRadius;
}

/*!
    \qmlproperty real Repeller3D::strength

    This property holds the strength of the repeller. The default value is 50.0;
*/
float QQuick3DParticleRepeller::strength() const
{
    return m_strength;
}

void QQuick3DParticleRepeller::setRadius(float radius)
{
    radius = qMax(0.0f, radius);
    if (qFuzzyCompare(radius, m_radius)) return;

    m_radius = radius;
    Q_EMIT radiusChanged();
}

void QQuick3DParticleRepeller::setOuterRadius(float radius)
{
    radius = qMax(0.0f, radius);
    if (qFuzzyCompare(radius, m_outerRadius)) return;

    m_outerRadius = radius;
    Q_EMIT outerRadiusChanged();
}

void QQuick3DParticleRepeller::setStrength(float strength)
{
    strength = qMax(0.0f, strength);
    if (qFuzzyCompare(strength, m_strength)) return;

    m_strength = strength;
    Q_EMIT strengthChanged();
}

void QQuick3DParticleRepeller::prepareToAffect()
{

}

static float qt_smoothstep(float edge0, float edge1, float x)
{
    float t;
    t = qBound(0.0f, (x - edge0) / (edge1 - edge0), 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

void QQuick3DParticleRepeller::affectParticle(const QQuick3DParticleData &, QQuick3DParticleDataCurrent *d, float )
{
    QVector3D pos = position();
    QVector3D dir = d->position - pos;
    float radius = dir.length();
    float outerRadius = qMax(m_outerRadius, m_radius);
    if (radius > outerRadius || qFuzzyIsNull(radius))
        return;

    if (radius < m_radius)
        d->position += dir * m_strength / radius;
    else
        d->position += dir * m_strength * (1.0f - qt_smoothstep(m_radius, outerRadius, radius)) / radius;
}

QT_END_NAMESPACE
