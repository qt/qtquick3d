/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "qquick3dquaternionutils_p.h"
#include <QtQuick3DUtils/private/qssgutils_p.h>
#include <QtMath>

QT_BEGIN_NAMESPACE


/*!
    \qmlmethod quaternion Quick3D::Quaternion::fromAxisAndAngle(vector3d axis, real angle)
    Creates a quaternion from \a axis and \a angle.
    Returns the resulting quaternion.
 */

/*!
    \qmlmethod quaternion Quick3D::Quaternion::fromAxisAndAngle(real x, real y, real z, real angle)
    Creates a quaternion from \a x, \a y, \a z, and \a angle.
    Returns the resulting quaternion.
 */

/*!
    \qmlmethod quaternion Quick3D::Quaternion::fromAxesAndAngles(vector3d axis1, real angle1,
                                                                 vector3d axis2, real angle2)
    Creates a quaternion from \a axis1, \a angle1, \a axis2, and \a angle2.
    Returns the resulting quaternion.
 */

/*!
    \qmlmethod quaternion Quick3D::Quaternion::fromAxesAndAngles(vector3d axis1, real angle1,
                                                                 vector3d axis2, real angle2,
                                                                 vector3d axis3, real angle3)
    Creates a quaternion from \a axis1, \a angle1, \a axis2, \a angle2, \a axis3, and \a angle3.
    Returns the resulting quaternion.
 */

/*!
    \qmlmethod quaternion Quick3D::Quaternion::fromEulerAngles(vector3d eulerAngles)
    Creates a quaternion from \a eulerAngles.
    Returns the resulting quaternion.
 */

/*!
    \qmlmethod quaternion Quick3D::Quaternion::fromEulerAngles(real x, real y, real z)
    Creates a quaternion from \a x, \a y, and \a z.
    Returns the resulting quaternion.
 */

/*!
    \qmlmethod quaternion Quick3D::Quaternion::lookAt(vector3d sourcePosition, vector3d targetPosition,
                                                      vector3d forwardDirection, vector3d upDirection)
    Creates a quaternion from \a sourcePosition, \a targetPosition, \a forwardDirection, and
    \a upDirection.  This is used for getting a rotation value for pointing at a particular target,
    and can be used to point a camera at a position in a scene.

    \a forwardDirection defaults to \c Qt.vector3d(0, 0, -1)
    \a upDirection defaults to \c Qt.vector3d(0, 1, 0)

    Returns the resulting quaternion.
 */

QQuick3DQuaternionUtils::QQuick3DQuaternionUtils(QObject *parent) : QObject(parent)
{

}

QQuaternion QQuick3DQuaternionUtils::fromAxisAndAngle(float x, float y, float z, float angle)
{
    return QQuaternion::fromAxisAndAngle(x, y, z, angle);
}

QQuaternion QQuick3DQuaternionUtils::fromAxisAndAngle(const QVector3D &axis, float angle)
{
    return QQuaternion::fromAxisAndAngle(axis, angle);
}

QQuaternion QQuick3DQuaternionUtils::fromEulerAngles(float x, float y, float z)
{
    return QQuaternion::fromEulerAngles(x, y, z);
}

QQuaternion QQuick3DQuaternionUtils::fromEulerAngles(const QVector3D &eulerAngles)
{
    return QQuaternion::fromEulerAngles(eulerAngles);
}

QQuaternion QQuick3DQuaternionUtils::lookAt(const QVector3D &sourcePosition,
                                            const QVector3D &targetPosition,
                                            const QVector3D &forwardDirection,
                                            const QVector3D &upDirection)
{
    QVector3D targetDirection = targetPosition - sourcePosition;
    targetDirection.normalize();

    QVector3D rotationAxis = QVector3D::crossProduct(forwardDirection, targetDirection);

    const QVector3D normalizedAxis = rotationAxis.normalized();
    if (qFuzzyIsNull(normalizedAxis.lengthSquared()))
        rotationAxis = upDirection;

    float dot = QVector3D::dotProduct(forwardDirection, targetDirection);
    float rotationAngle = qRadiansToDegrees(qAcos(dot));

    return QQuaternion::fromAxisAndAngle(rotationAxis, rotationAngle);
}

QQuaternion QQuick3DQuaternionUtils::fromAxesAndAngles(const QVector3D &axis1,
                                                       float angle1,
                                                       const QVector3D &axis2,
                                                       float angle2,
                                                       const QVector3D &axis3,
                                                       float angle3)
{
    const QQuaternion q1 = QQuaternion::fromAxisAndAngle(axis1, angle1);
    const QQuaternion q2 = QQuaternion::fromAxisAndAngle(axis2, angle2);
    const QQuaternion q3 = QQuaternion::fromAxisAndAngle(axis3, angle3);
    return q3 * q2 * q1;
}

QQuaternion QQuick3DQuaternionUtils::fromAxesAndAngles(const QVector3D &axis1,
                                                       float angle1,
                                                       const QVector3D &axis2,
                                                       float angle2)
{
    const QQuaternion q1 = QQuaternion::fromAxisAndAngle(axis1, angle1);
    const QQuaternion q2 = QQuaternion::fromAxisAndAngle(axis2, angle2);
    return q2 * q1;
}

QT_END_NAMESPACE
