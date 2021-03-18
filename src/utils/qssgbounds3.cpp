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

#include "qssgbounds3_p.h"
#include <private/qssgutils_p.h>

QT_BEGIN_NAMESPACE

void QSSGBounds3::include(const QVector3D &v)
{
    minimum = vec3::minimum(minimum, v);
    maximum = vec3::maximum(maximum, v);
}

void QSSGBounds3::include(const QSSGBounds3 &b)
{
    minimum = vec3::minimum(minimum, b.minimum);
    maximum = vec3::maximum(maximum, b.maximum);
}

bool QSSGBounds3::isFinite() const
{
    return vec3::isFinite(minimum) && vec3::isFinite(maximum);
}

QSSGBounds3 QSSGBounds3::boundsOfPoints(const QVector3D &v0, const QVector3D &v1)
{
    return QSSGBounds3(vec3::minimum(v0, v1), vec3::maximum(v0, v1));
}

QSSGBounds3 QSSGBounds3::basisExtent(const QVector3D &center, const QMatrix3x3 &basis, const QVector3D &extent)
{
    // extended basis vectors
    QVector3D c0 = QVector3D(basis(0, 0), basis(1, 0), basis(2, 0)) * extent.x();
    QVector3D c1 = QVector3D(basis(0, 1), basis(1, 1), basis(2, 1)) * extent.y();
    QVector3D c2 = QVector3D(basis(0, 2), basis(1, 2), basis(2, 2)) * extent.z();

    QVector3D w;
    // find combination of base vectors that produces max. distance for each component = sum of
    // abs()
    w.setX(qAbs(c0.x()) + qAbs(c1.x()) + qAbs(c2.x()));
    w.setY(qAbs(c0.y()) + qAbs(c1.y()) + qAbs(c2.y()));
    w.setZ(qAbs(c0.z()) + qAbs(c1.z()) + qAbs(c2.z()));

    return QSSGBounds3(center - w, center + w);
}

QSSGBounds3 QSSGBounds3::transform(const QMatrix3x3 &matrix, const QSSGBounds3 &bounds)
{
    return bounds.isEmpty()
            ? bounds
            : QSSGBounds3::basisExtent(mat33::transform(matrix, bounds.center()), matrix, bounds.extents());
}

void QSSGBounds3::transform(const QMatrix4x4 &inMatrix)
{
    if (!isEmpty()) {
        QSSGBounds2BoxPoints thePoints;
        expand(thePoints);
        setEmpty();
        for (quint32 idx = 0; idx < 8; ++idx)
            include(inMatrix * thePoints[idx]);
    }
}

QT_END_NAMESPACE
