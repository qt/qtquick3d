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

#include "qquick3dparticlepointrotator_p.h"

QT_BEGIN_NAMESPACE

QQuick3DParticlePointRotator::QQuick3DParticlePointRotator(QObject *parent)
    : QQuick3DParticleAffector(parent)
{
}

// Magnitude is degrees per second
float QQuick3DParticlePointRotator::magnitude() const
{
    return m_magnitude;
}

void QQuick3DParticlePointRotator::setMagnitude(float magnitude)
{
    if (qFuzzyCompare(m_magnitude, magnitude))
        return;

    m_magnitude = magnitude;
    Q_EMIT magnitudeChanged();
    update();
}

// Normalized rotation direction.
QVector3D QQuick3DParticlePointRotator::direction() const
{
    return m_direction;
}

void QQuick3DParticlePointRotator::setDirection(const QVector3D &direction)
{
    if (m_direction == direction)
        return;

    m_direction = direction;
    Q_EMIT directionChanged();
    update();
}

QVector3D QQuick3DParticlePointRotator::pivotPoint() const
{
    return m_pivotPoint;
}

void QQuick3DParticlePointRotator::setPivotPoint(const QVector3D &point)
{
    if (m_pivotPoint == point)
        return;

    m_pivotPoint = point;
    Q_EMIT pivotPointChanged();
    update();
}

void QQuick3DParticlePointRotator::affectParticle(const QQuick3DParticleData &sd, QQuick3DParticleDataCurrent *d, float time)
{
    // Rotate based on the current position
    // Note that this means order of PointRotator element compared to other affectors matters
    if (!qFuzzyIsNull(m_magnitude)) {
        QMatrix4x4 m2;
        m2.translate(m_pivotPoint);
        m2.rotate(time * m_magnitude, m_direction);
        m2.translate(-m_pivotPoint);
        d->position = m2.map(d->position);
    }
}

QT_END_NAMESPACE
