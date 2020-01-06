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

#include "qquick3dperspectivecamera_p.h"

#include <QtQuick3DRuntimeRender/private/qssgrendercamera_p.h>

#include <QtMath>
#include <QtQuick3DUtils/private/qssgutils_p.h>

#include "qquick3dutils_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype PerspectiveCamera
    \inherits Camera
    \inqmlmodule QtQuick3D
    \brief Defines a Perspective Camera for viewing the content of a 3D scene.

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

    Perspective camera is the standard camera type, which uses field of view and near and far
    clip planes to specify the projection.

    \sa OrthographicCamera, FrustumCamera, CustomCamera
*/

/*!
    \internal
*/
QQuick3DPerspectiveCamera::QQuick3DPerspectiveCamera() {}

/*!
    \qmlproperty real PerspectiveCamera::clipNear

    This property holds the near value of the camara's view frustum. This value determines
    what the closest distance to the camera that items will be shown.
*/

float QQuick3DPerspectiveCamera::clipNear() const
{
    return m_clipNear;
}

/*!
    \qmlproperty real PerspectiveCamera::clipFar

    This property holds the far value of the camara's view frustum. This value determines
    what the furthest distance to the camera that items will be shown.
*/

float QQuick3DPerspectiveCamera::clipFar() const
{
    return m_clipFar;
}

/*!
    \qmlproperty real PerspectiveCamera::fieldOfView

    This property holds the field of view of the camera in degrees. This can be either the
    vertical or horizontal field of view depending on if the fieldOfViewOrientation property
    is set to \c {Camera.Vertical} or \c {Camera.Horizontal}.
 */

float QQuick3DPerspectiveCamera::fieldOfView() const
{
    return m_fieldOfView;
}

/*!
    \qmlproperty enumeration PerspectiveCamera::fieldOfViewOrientation

    This property determines if the field of view property reflects the vertical or the horizontal
    field of view. The default value is \c {Camera.Vertical}.
*/

QQuick3DCamera::FieldOfViewOrientation QQuick3DPerspectiveCamera::fieldOfViewOrientation() const
{
    return m_fieldOfViewOrientation;
}

void QQuick3DPerspectiveCamera::setClipNear(float clipNear)
{
    if (qFuzzyCompare(m_clipNear, clipNear))
        return;

    m_clipNear = clipNear;
    emit clipNearChanged();
    update();
}

void QQuick3DPerspectiveCamera::setClipFar(float clipFar)
{
    if (qFuzzyCompare(m_clipFar, clipFar))
        return;

    m_clipFar = clipFar;
    emit clipFarChanged();
    update();
}

void QQuick3DPerspectiveCamera::setFieldOfView(float fieldOfView)
{
    if (qFuzzyCompare(m_fieldOfView, fieldOfView))
        return;

    m_fieldOfView = fieldOfView;
    emit fieldOfViewChanged();
    update();
}

void QQuick3DPerspectiveCamera::setFieldOfViewOrientation(QQuick3DCamera::FieldOfViewOrientation
                                                          fieldOfViewOrientation)
{
    if (m_fieldOfViewOrientation == fieldOfViewOrientation)
        return;

    m_fieldOfViewOrientation = fieldOfViewOrientation;
    emit fieldOfViewOrientationChanged();
    update();
}

bool QQuick3DPerspectiveCamera::checkSpatialNode(QSSGRenderCamera *camera)
{
    bool changed = false;
    changed |= qUpdateIfNeeded(camera->clipNear, m_clipNear);
    changed |= qUpdateIfNeeded(camera->clipFar, m_clipFar);
    changed |= qUpdateIfNeeded(camera->fov, qDegreesToRadians(m_fieldOfView));
    changed |= qUpdateIfNeeded(camera->fovHorizontal, m_fieldOfViewOrientation
                               == QQuick3DCamera::FieldOfViewOrientation::Horizontal);
    changed |= qUpdateIfNeeded(camera->enableFrustumClipping, frustumCullingEnabled());

    return changed;
}

QT_END_NAMESPACE
