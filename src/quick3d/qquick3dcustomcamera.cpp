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

#include "qquick3dcustomcamera_p.h"

#include <QtQuick3DRuntimeRender/private/qssgrendercamera_p.h>

#include <QtMath>
#include <QtQuick3DUtils/private/qssgutils_p.h>

#include "qquick3dutils_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype CustomCamera
    \inherits Camera
    \inqmlmodule QtQuick3D
    \brief Defines a Custom Camera for viewing the content of a 3D scene.

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

    If you need full-control over how the projection matrix is created, this is the camera
    to use.

    \sa PerspectiveCamera, OrthographicCamera, FrustumCamera
*/

/*!
 * \internal
 */
QQuick3DCustomCamera::QQuick3DCustomCamera() {}

/*!
    \qmlproperty matrix4x4 CustomCamera::projection

    This property defines a custom projection matrix.
*/
QMatrix4x4 QQuick3DCustomCamera::projection() const
{
    return m_projection;
}

void QQuick3DCustomCamera::setProjection(const QMatrix4x4 &projection)
{
    if (m_projection == projection)
        return;

    m_projection = projection;
    emit projectionChanged();
    update();
}

bool QQuick3DCustomCamera::checkSpatialNode(QSSGRenderCamera *camera)
{
    camera->flags.setFlag(QSSGRenderNode::Flag::CameraCustomProjection, true);

    bool changed = false;
    changed |= qUpdateIfNeeded(camera->projection, m_projection);
    changed |= qUpdateIfNeeded(camera->enableFrustumClipping, frustumCullingEnabled());

    return changed;
}

QT_END_NAMESPACE
