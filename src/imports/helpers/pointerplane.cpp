/****************************************************************************
**
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

#include "pointerplane.h"

/*!
    \qmltype PointerPlane
    \inqmlmodule QtQuick3D.Helpers
    \inherits Node
    \brief A node that can be used for calculating where a 2D coordinate
    in a View3D intersects with a plane in the scene.

    The PointerPlane type represents a 3D plane in the scene (without any
    visual geometry). It can be used to calculate the intersection
    between a plane and any arbitrary ray in the scene. The intended use
    for it is to calculate where in the plane a mouse position intersects.

    A PointerPlane is a \l Node, which means that you can make it a child
    of another Node in the scene. The plane will therefore inherit its
    position and rotation in the scene, like any other node.

    See the \l {Qt Quick 3D - Studio Example}{Studio Example} for an example
    on how to use it.

*/
PointerPlane::PointerPlane(QQuick3DNode *parent)
    : QQuick3DNode(parent)
{
}

/*! qmlmethod vector3d QtQuick3D.helpers::PointerPlane::getIntersectPos(rayPos0, rayPos1, planePos, planeNormal)
    Returns the intersection position in scene space between the ray and the
    given plane (all in scene space). If no such position exists (the ray is
    parallel to the plane), a vector with all components set to 0 will be returned.
    \note this function is unaffected by the transform applied to this node.
*/
QVector3D PointerPlane::getIntersectPos(
        const QVector3D &rayPos0,
        const QVector3D &rayPos1,
        const QVector3D &planePos,
        const QVector3D &planeNormal) const
{
    const QVector3D rayDirection = rayPos1 - rayPos0;
    const QVector3D rayPos0RelativeToPlane = rayPos0 - planePos;

    const float dotPlaneRayDirection = QVector3D::dotProduct(planeNormal, rayDirection);
    const float dotPlaneRayPos0 = -QVector3D::dotProduct(planeNormal, rayPos0RelativeToPlane);

    if (qFuzzyIsNull(dotPlaneRayDirection)) {
        // The ray is is parallel to the plane. Note that if dotLinePos0 == 0, it
        // additionally means that the line lies in plane as well. In any case, we
        // signal that we cannot find a single intersection point.
        return QVector3D();
    }

    // Since we work with a ray (that has a start), distanceFromLinePos0ToPlane
    // must be above 0. If it was a line segment (with an end), it also need to be less than 1.
    // (Note: a third option would be a "line", which is different from a ray or segment in that
    // it has neither a start, nor an end). Then we wouldn't need to check the distance at all. But
    // that would also mean that the line could intersect the plane behind the camera.
    const float distanceFromRayPos0ToPlane = dotPlaneRayPos0 / dotPlaneRayDirection;
    if (distanceFromRayPos0ToPlane <= 0)
        return QVector3D();
    return rayPos0 + distanceFromRayPos0ToPlane * rayDirection;
}

/*! qmlmethod vector3d QtQuick3D.helpers::PointerPlane::getIntersectPosFromSceneRay(rayPos0, rayPos1)
    Returns the intersection position in scene space between the ray and
    the plane (both in scene space) that has the same position and transform
    as this node. If no such position exists (the ray is parallel to
    the plane), a vector with all components set to 0 will be returned.
*/
QVector3D PointerPlane::getIntersectPosFromSceneRay(
        const QVector3D &rayPos0,
        const QVector3D &rayPos1) const
{
    return getIntersectPos(rayPos0, rayPos1, scenePosition(), forward());
}

/*! qmlmethod vector3d QtQuick3D.helpers::PointerPlane::getIntersectPosFromView(view, posInView)
    Returns the intersection position in scene space from a ray
    originating from a position a view (e.g a mouse position).
    If no such position exists (the ray is parallel to
    the plane), a vector with all components set to 0 will be returned.
    The plane has the same position and transform as this node.
*/
QVector3D PointerPlane::getIntersectPosFromView(
        QQuick3DViewport *view,
        const QPointF &posInView) const
{
    const QVector3D viewPos1(float(posInView.x()), float(posInView.y()), 0);
    const QVector3D viewPos2(float(posInView.x()), float(posInView.y()), 1);
    const QVector3D rayPos0 = view->mapTo3DScene(viewPos1);
    const QVector3D rayPos1 = view->mapTo3DScene(viewPos2);
    return getIntersectPosFromSceneRay(rayPos0, rayPos1);
}

#include "moc_pointerplane.cpp"

