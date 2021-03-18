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

#include "qssgrenderclippingfrustum_p.h"

#include <QtQuick3DUtils/private/qssgutils_p.h>

QT_BEGIN_NAMESPACE

QSSGClippingFrustum::QSSGClippingFrustum(const QMatrix4x4 &modelviewprojection, const QSSGClipPlane &nearPlane)
{
    QSSGClipPlane *_cullingPlanes = mPlanes;
    const QMatrix4x4 &modelViewProjectionMat(modelviewprojection);
    const float *modelviewProjection = modelViewProjectionMat.data();

    // update planes (http://read.pudn.com/downloads128/doc/542641/Frustum.pdf)
    // Google for Gribb plane extraction if that link doesn't work.
    // http://www.google.com/search?q=ravensoft+plane+extraction
#define M(_x, _y) modelviewProjection[(4 * (_y)) + (_x)]
    // left plane
    _cullingPlanes[0].normal.setX(M(3, 0) + M(0, 0));
    _cullingPlanes[0].normal.setY(M(3, 1) + M(0, 1));
    _cullingPlanes[0].normal.setZ(M(3, 2) + M(0, 2));
    _cullingPlanes[0].d = M(3, 3) + M(0, 3);
    _cullingPlanes[0].d /= vec3::normalize(_cullingPlanes[0].normal);

    // right plane
    _cullingPlanes[1].normal.setX(M(3, 0) - M(0, 0));
    _cullingPlanes[1].normal.setY(M(3, 1) - M(0, 1));
    _cullingPlanes[1].normal.setZ(M(3, 2) - M(0, 2));
    _cullingPlanes[1].d = M(3, 3) - M(0, 3);
    _cullingPlanes[1].d /= vec3::normalize(_cullingPlanes[1].normal);

    // far plane
    _cullingPlanes[2].normal.setX(M(3, 0) - M(2, 0));
    _cullingPlanes[2].normal.setY(M(3, 1) - M(2, 1));
    _cullingPlanes[2].normal.setZ(M(3, 2) - M(2, 2));
    _cullingPlanes[2].d = M(3, 3) - M(2, 3);
    _cullingPlanes[2].d /= vec3::normalize(_cullingPlanes[2].normal);

    // bottom plane
    _cullingPlanes[3].normal.setX(M(3, 0) + M(1, 0));
    _cullingPlanes[3].normal.setY(M(3, 1) + M(1, 1));
    _cullingPlanes[3].normal.setZ(M(3, 2) + M(1, 2));
    _cullingPlanes[3].d = M(3, 3) + M(1, 3);
    _cullingPlanes[3].d /= vec3::normalize(_cullingPlanes[3].normal);

    // top plane
    _cullingPlanes[4].normal.setX(M(3, 0) - M(1, 0));
    _cullingPlanes[4].normal.setY(M(3, 1) - M(1, 1));
    _cullingPlanes[4].normal.setZ(M(3, 2) - M(1, 2));
    _cullingPlanes[4].d = M(3, 3) - M(1, 3);
    _cullingPlanes[4].d /= vec3::normalize(_cullingPlanes[4].normal);
#undef M
    _cullingPlanes[5] = nearPlane;
    // http://www.openscenegraph.org/projects/osg/browser/OpenSceneGraph/trunk/include/osg/Plane?rev=5328
    // setup the edges of the plane that we will clip against an axis-aligned bounding box.
    for (quint32 idx = 0; idx < 6; ++idx)
        _cullingPlanes[idx].calculateBBoxEdges();
}

QT_END_NAMESPACE
