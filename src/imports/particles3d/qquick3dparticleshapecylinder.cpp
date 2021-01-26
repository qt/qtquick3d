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

#include "qquick3dparticleshapecylinder_p.h"
#include "qquick3dparticlerandomizer_p.h"
#include <QVector3D>

QT_BEGIN_NAMESPACE

QQuick3DParticleShapeCylinder::QQuick3DParticleShapeCylinder(QObject *parent)
    : QQuick3DParticleShape(parent)
{
}

const QVector3D QQuick3DParticleShapeCylinder::randomPosition()
{
    if (!m_parentNode)
        return QVector3D();

    QVector3D scale = m_parentNode->sceneScale();
    float y = (scale.y() * 50) - (QPRand::get() * scale.y() * 100);

    float r = 1.0f;
    if (m_fill)
        r = sqrt(QPRand::get());
    float theta = QPRand::get() * M_PI * 2.0;
    float x = r * cos(theta);
    float z = r * sin(theta);
    x = x * (scale.x() * 50);
    z = z * (scale.z() * 50);
    QVector3D pos(x, y, z);
    QMatrix4x4 mat;
    mat.rotate(m_parentNode->rotation());
    pos = mat.mapVector(pos);
    return pos;
}

bool QQuick3DParticleShapeCylinder::containsPoint(const QVector3D &point)
{
    // TODO: Implement
    return true;
}

QT_END_NAMESPACE

