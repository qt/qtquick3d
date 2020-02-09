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

#include "qquick3dfrustumcamera_p.h"

#include <QtQuick3DRuntimeRender/private/qssgrendercamera_p.h>

#include <QtMath>
#include <QtQuick3DUtils/private/qssgutils_p.h>

#include "qquick3dutils_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype FrustumCamera
    \inherits PerspectiveCamera
    \inqmlmodule QtQuick3D
    \brief Defines a Perspective Frustum Camera for viewing the content of a 3D scene.

    A Camera is always necessary to view the content of a 3D scene. A camera
    defines how to project the content of a 3D scene into a 2D coordinate space,
    which can then be used on a 2D surface. When a camera is present in the scene
    it can be used to direct what is displayed in a View3D.

    To determine the projection of this camera a high level API is provided.
    First it is possible to position this Camera like any other spatial Node in
    the scene. This determines where the Camera is in the scene, and what
    direction it is facing. The default direction of the camera is such that the
    forward vector is looking up the +Z axis, and the up direction vector is up
    the +Y axis. With this in mind any transformation applied to the camera as
    well as the transformations inherited from it's parent Nodes you can define
    exactly where and in what direction your camera is facing.

    For finer grain control of how the frustum is defined, this is the camera to use.
    FrustumCamera allows for setting the FrustumCamera::top, FrustumCamera::bottom,
    FrustumCamera::right, and FrustumCamera::left properties. This is useful in
    creating asymmetrical frustums.

    \sa PerspectiveCamera, OrthographicCamera, CustomCamera
*/

/*!
 * \internal
 */
QQuick3DFrustumCamera::QQuick3DFrustumCamera() {}

/*!
    \qmlproperty real FrustumCamera::top

    This property defines the top plane of the camera view frustum.
*/
float QQuick3DFrustumCamera::top() const
{
    return m_top;
}

/*!
    \qmlproperty real FrustumCamera::bottom

    This property defines the bottom plane of the camera view frustum.
*/
float QQuick3DFrustumCamera::bottom() const
{
    return m_bottom;
}

/*!
    \qmlproperty real FrustumCamera::right

    This property defines the right plane of the camera view frustum.
*/
float QQuick3DFrustumCamera::right() const
{
    return m_right;
}

/*!
    \qmlproperty real FrustumCamera::left

    This property defines the left plane of the camera view frustum.
*/
float QQuick3DFrustumCamera::left() const
{
    return m_left;
}

void QQuick3DFrustumCamera::setTop(float top)
{
    if (qFuzzyCompare(m_top, top))
        return;

    m_top = top;
    emit topChanged();
    update();
}

void QQuick3DFrustumCamera::setBottom(float bottom)
{
    if (qFuzzyCompare(m_bottom, bottom))
        return;

    m_bottom = bottom;
    emit bottomChanged();
    update();
}

void QQuick3DFrustumCamera::setRight(float right)
{
    if (qFuzzyCompare(m_right, right))
        return;

    m_right = right;
    emit rightChanged();
    update();
}

void QQuick3DFrustumCamera::setLeft(float left)
{
    if (qFuzzyCompare(m_left, left))
        return;

    m_left = left;
    emit leftChanged();
    update();
}

bool QQuick3DFrustumCamera::checkSpatialNode(QSSGRenderCamera *camera)
{
    camera->flags.setFlag(QSSGRenderNode::Flag::CameraFrustumProjection, true);

    bool changed = false;
    changed |= qUpdateIfNeeded(camera->clipNear, clipNear());
    changed |= qUpdateIfNeeded(camera->clipFar, clipFar());
    changed |= qUpdateIfNeeded(camera->fov, qDegreesToRadians(fieldOfView()));
    changed |= qUpdateIfNeeded(camera->fovHorizontal, fieldOfViewOrientation()
                               == QQuick3DCamera::FieldOfViewOrientation::Horizontal);
    changed |= qUpdateIfNeeded(camera->enableFrustumClipping, frustumCullingEnabled());
    changed |= qUpdateIfNeeded(camera->top, m_top);
    changed |= qUpdateIfNeeded(camera->bottom, m_bottom);
    changed |= qUpdateIfNeeded(camera->right, m_right);
    changed |= qUpdateIfNeeded(camera->left, m_left);

    return changed;
}

QT_END_NAMESPACE
