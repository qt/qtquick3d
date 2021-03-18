/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "qssgutils_p.h"

#include <QtCore/QDir>

#include <cmath>

QT_BEGIN_NAMESPACE

float vec2::magnitude(const QVector2D &v)
{
    return ::sqrtf(v.x() * v.x() + v.y() * v.y());
}

bool vec3::isFinite(const QVector3D &v)
{
    return qIsFinite(v.x()) && qIsFinite(v.y()) && qIsFinite(v.z());
}

float vec3::magnitude(const QVector3D &v)
{
    return sqrtf(v.x() * v.x() + v.y() * v.y() + v.z() * v.z());
}

float vec3::magnitudeSquared(const QVector3D &v)
{
    return v.x() * v.x() + v.y() * v.y() + v.z() * v.z();
}

// This special normalize function normalizes a vector in place
// and returns the magnnitude (needed for compatiblity)
float vec3::normalize(QVector3D &v)
{
    const float m = vec3::magnitude(v);
    if (m > 0)
        v /= m;
    return m;
}

QVector3D mat33::transform(const QMatrix3x3 &m, const QVector3D &v)
{
    const QVector3D c0 = QVector3D(m(0, 0), m(1, 0), m(2, 0));
    const QVector3D c1 = QVector3D(m(0, 1), m(1, 1), m(2, 1));
    const QVector3D c2 = QVector3D(m(0, 2), m(1, 2), m(2, 2));
    return c0 * v.x() + c1 * v.y() + c2 * v.z();
}

static Q_DECL_CONSTEXPR inline float dotProduct(const float (&v1)[3], const float (&v2)[3])
{
    return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}

static Q_DECL_CONSTEXPR inline QVector3D crossProduct(const float (&v1)[3], const float (&v2)[3])
{
    return QVector3D{v1[1] * v2[2] - v1[2] * v2[1], v1[2] * v2[0] - v1[0] * v2[2], v1[0] * v2[1] - v1[1] * v2[0]};
};

QMatrix3x3 mat33::getInverse(const QMatrix3x3 &m)
{
    //return column0.dot(column1.cross(column2));
    const QVector3D c0(m(0, 0), m(1, 0), m(2, 0));
    const QVector3D c1(m(0, 1), m(1, 1), m(2, 1));
    const QVector3D c2(m(0, 2), m(1, 2), m(2, 2));

    const float (&c0d)[3] = reinterpret_cast<const float (&)[3]>(c0);
    const float (&c1d)[3] = reinterpret_cast<const float (&)[3]>(c1);
    const float (&c2d)[3] = reinterpret_cast<const float (&)[3]>(c2);

    const auto &xp = crossProduct(c1d, c2d);
    const float det = dotProduct(c0d, reinterpret_cast<const float (&)[3]>(xp));

    QMatrix3x3 inverse;
    if (!qFuzzyIsNull(det)) {
        const float invDet = 1.0f / det;

        inverse(0, 0) = invDet * (m(1, 1) * m(2, 2) - m(2, 1) * m(1, 2));
        inverse(0, 1) = invDet * -(m(0, 1) * m(2, 2) - m(2, 1) * m(0, 2));
        inverse(0, 2) = invDet * (m(0, 1) * m(1, 2) - m(0, 2) * m(1, 1));

        inverse(1, 0) = invDet * -(m(1, 0) * m(2, 2) - m(1, 2) * m(2, 0));
        inverse(1, 1) = invDet * (m(0, 0) * m(2, 2) - m(0, 2) * m(2, 0));
        inverse(1, 2) = invDet * -(m(0, 0) * m(1, 2) - m(0, 2) * m(1, 0));

        inverse(2, 0) = invDet * (m(1, 0) * m(2, 1) - m(1, 1) * m(2, 0));
        inverse(2, 1) = invDet * -(m(0, 0) * m(2, 1) - m(0, 1) * m(2, 0));
        inverse(2, 2) = invDet * (m(0, 0) * m(1, 1) - m(1, 0) * m(0, 1));
    }

    return inverse;
}

QMatrix3x3 mat44::getUpper3x3(const QMatrix4x4 &m)
{
    const float values[9] = { m(0, 0), m(0, 1), m(0, 2), m(1, 0), m(1, 1), m(1, 2), m(2, 0), m(2, 1), m(2, 2) };
    return QMatrix3x3(values);
}

void mat44::normalize(QMatrix4x4 &m)
{
    QVector4D c0 = m.column(0);
    QVector4D c1 = m.column(1);
    QVector4D c2 = m.column(2);
    QVector4D c3 = m.column(3);

    c0.normalize();
    c1.normalize();
    c2.normalize();
    c3.normalize();

    m.setColumn(0, c0);
    m.setColumn(1, c1);
    m.setColumn(2, c2);
    m.setColumn(3, c3);
}

QVector3D mat44::rotate(const QMatrix4x4 &m, const QVector3D &v)
{
    const QVector4D tmp = mat44::rotate(m, QVector4D(v.x(), v.y(), v.z(), 1.0f));
    return QVector3D(tmp.x(), tmp.y(), tmp.z());
}

QVector4D mat44::rotate(const QMatrix4x4 &m, const QVector4D &v)
{  
    return m.column(0) * v.x() + m.column(1) * v.y() + m.column(2) * v.z();
}

QVector3D mat44::transform(const QMatrix4x4 &m, const QVector3D &v)
{
    const QVector4D tmp = mat44::transform(m, QVector4D(v.x(), v.y(), v.z(), 1.0f));
    return QVector3D(tmp.x(), tmp.y(), tmp.z());
}

QVector4D mat44::transform(const QMatrix4x4 &m, const QVector4D &v)
{
    return m.column(0) * v.x() + m.column(1) * v.y() + m.column(2) * v.z() + m.column(3) * v.w();
}

QVector3D mat44::getPosition(const QMatrix4x4 &m)
{
    return QVector3D(m(0, 3), m(1, 3), m(2, 3));
}

QVector3D mat44::getScale(const QMatrix4x4 &m)
{
    const float scaleX = m.column(0).length();
    const float scaleY = m.column(1).length();
    const float scaleZ = m.column(2).length();
    return QVector3D(scaleX, scaleY, scaleZ);
}

bool quant::isFinite(const QQuaternion &q)
{
    return qIsFinite(q.x()) && qIsFinite(q.y()) && qIsFinite(q.z()) && qIsFinite(q.scalar());
}

float quant::magnitude(const QQuaternion &q)
{
    return std::sqrt(q.x() * q.x() + q.y() * q.y() + q.z() * q.z() + q.scalar() * q.scalar());
}

bool quant::isSane(const QQuaternion &q)
{
    const float unitTolerance = float(1e-2);
    return isFinite(q) && qAbs(magnitude(q) - 1) < unitTolerance;
}

bool quant::isUnit(const QQuaternion &q)
{
    const float unitTolerance = float(1e-4);
    return isFinite(q) && qAbs(magnitude(q) - 1) < unitTolerance;
}

QVector3D quant::rotated(const QQuaternion &q, const QVector3D &v)
{
    const float vx = 2.0f * v.x();
    const float vy = 2.0f * v.y();
    const float vz = 2.0f * v.z();
    const float w2 = q.scalar() * q.scalar() - 0.5f;
    const float dot2 = (q.x() * vx + q.y() * vy + q.z() * vz);
    return QVector3D((vx * w2 + (q.y() * vz - q.z() * vy) * q.scalar() + q.x() * dot2),
                     (vy * w2 + (q.z() * vx - q.x() * vz) * q.scalar() + q.y() * dot2),
                     (vz * w2 + (q.x() * vy - q.y() * vx) * q.scalar() + q.z() * dot2));
}

QVector3D quant::inverseRotated(const QQuaternion &q, const QVector3D &v)
{
    const float vx = 2.0f * v.x();
    const float vy = 2.0f * v.y();
    const float vz = 2.0f * v.z();
    const float w2 = q.scalar() * q.scalar() - 0.5f;
    const float dot2 = (q.x() * vx + q.y() * vy + q.z() * vz);
    return QVector3D((vx * w2 - (q.y() * vz - q.z() * vy) * q.scalar() + q.x() * dot2),
                     (vy * w2 - (q.z() * vx - q.x() * vz) * q.scalar() + q.y() * dot2),
                     (vz * w2 - (q.x() * vy - q.y() * vx) * q.scalar() + q.z() * dot2));
}

const char *nonNull(const char *src)
{
    return src == nullptr ? "" : src;
}

QT_END_NAMESPACE
