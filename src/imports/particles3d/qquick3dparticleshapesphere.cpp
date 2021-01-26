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
#define _USE_MATH_DEFINES
#include <math.h>

#include "qquick3dparticleshapesphere_p.h"
#include "qquick3dparticlerandomizer_p.h"
#include <QVector3D>

QT_BEGIN_NAMESPACE

QQuick3DParticleShapeSphere::QQuick3DParticleShapeSphere(QObject *parent)
    : QQuick3DParticleShape(parent)
{
}

const QVector3D QQuick3DParticleShapeSphere::randomPosition()
{
    if (!m_parentNode)
        return QVector3D();

    QVector3D scale = m_parentNode->sceneScale();

    float theta = QPRand::get() * M_PI * 2.0;
    float v = QPRand::get();
    float phi = acos((2*v)-1);
    float r = m_fill ? pow(QPRand::get(), 1.0/3.0) : 1.0f;
    float x = r * sin(phi) * cos(theta);
    float y = r * sin(phi) * sin(theta);
    float z = r * cos(phi);
    QVector3D pos(x, y, z);
    pos *= (scale * 50.0);
    QMatrix4x4 mat;
    mat.rotate(m_parentNode->rotation());
    pos = mat.mapVector(pos);
    return pos;
}

bool QQuick3DParticleShapeSphere::containsPoint(const QVector3D &point)
{
    // TODO: Implement
    return true;
}

QT_END_NAMESPACE
