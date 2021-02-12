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

#include "qquick3dparticleshape_p.h"
#include "qquick3dparticlerandomizer_p.h"

QT_BEGIN_NAMESPACE

QQuick3DParticleShape::QQuick3DParticleShape(QObject *parent)
    : QObject(parent)
{
    m_parentNode = qobject_cast<QQuick3DNode*>(parent);
}

bool QQuick3DParticleShape::fill() const
{
    return m_fill;
}

void QQuick3DParticleShape::setFill(bool fill)
{
    if (m_fill == fill)
        return;

    m_fill = fill;
    Q_EMIT fillChanged();
}

QQuick3DParticleShape::ShapeType QQuick3DParticleShape::type() const
{
    return m_type;
}

void QQuick3DParticleShape::setType(QQuick3DParticleShape::ShapeType type)
{
    if (m_type == type)
        return;

    m_type = type;
    Q_EMIT typeChanged();
}

void QQuick3DParticleShape::componentComplete()
{
    m_parentNode = qobject_cast<QQuick3DNode*>(parent());
    if (!m_parentNode) {
        qWarning() << "Shape requires parent Node to function correctly!";
    }
}

QVector3D QQuick3DParticleShape::randomPosition() const
{
    if (!m_parentNode)
        return QVector3D();

    switch (m_type) {
    case QQuick3DParticleShape::ShapeType::Cube:
        return randomPositionCube();
    case QQuick3DParticleShape::ShapeType::Sphere:
        return randomPositionSphere();
    case QQuick3DParticleShape::ShapeType::Cylinder:
        return randomPositionCylinder();
    default:
        Q_ASSERT(false);
    }
    return QVector3D();
}

QVector3D QQuick3DParticleShape::randomPositionCube() const
{
    QVector3D s = m_parentNode->sceneScale() * 50.0f;
    float x = s.x() - (QPRand::get() * s.x() * 2.0f);
    float y = s.y() - (QPRand::get() * s.y() * 2.0f);
    float z = s.z() - (QPRand::get() * s.z() * 2.0f);
    if (!m_fill) {
        // Random 0..5 for cube sides
        int side = int(QPRand::get() * 6);
        if (side == 0)
            x = -s.x();
        else if (side == 1)
            x = s.x();
        else if (side == 2)
            y = -s.y();
        else if (side == 3)
            y = s.y();
        else if (side == 4)
            z = -s.z();
        else
            z = s.z();
    }
    QMatrix4x4 mat;
    mat.rotate(m_parentNode->rotation());
    return mat.mapVector(QVector3D(x, y, z));
}

QVector3D QQuick3DParticleShape::randomPositionSphere() const
{
    QVector3D scale = m_parentNode->sceneScale();
    float theta = QPRand::get() * float(M_PI) * 2.0f;
    float v = QPRand::get();
    float phi = acos((2.0f * v) - 1.0f);
    float r = m_fill ? pow(QPRand::get(), 1.0f / 3.0f) : 1.0f;
    float x = r * sin(phi) * cos(theta);
    float y = r * sin(phi) * sin(theta);
    float z = r * cos(phi);
    QVector3D pos(x, y, z);
    pos *= (scale * 50.0f);
    QMatrix4x4 mat;
    mat.rotate(m_parentNode->rotation());
    return mat.mapVector(pos);
}

QVector3D QQuick3DParticleShape::randomPositionCylinder() const
{
    QVector3D scale = m_parentNode->sceneScale();
    float y = (scale.y() * 50.0f) - (QPRand::get() * scale.y() * 100.0f);
    float r = 1.0f;
    if (m_fill)
        r = sqrt(QPRand::get());
    float theta = QPRand::get() * float(M_PI) * 2.0f;
    float x = r * cos(theta);
    float z = r * sin(theta);
    x = x * (scale.x() * 50.0f);
    z = z * (scale.z() * 50.0f);
    QVector3D pos(x, y, z);
    QMatrix4x4 mat;
    mat.rotate(m_parentNode->rotation());
    return mat.mapVector(pos);
}

QT_END_NAMESPACE
