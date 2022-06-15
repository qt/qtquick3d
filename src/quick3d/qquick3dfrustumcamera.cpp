// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qquick3dfrustumcamera_p.h"

#include <QtQuick3DRuntimeRender/private/qssgrendercamera_p.h>

#include <QtMath>
#include <QtQuick3DUtils/private/qssgutils_p.h>

#include "qquick3dutils_p.h"

#include "qquick3dnode_p_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype FrustumCamera
    \inherits PerspectiveCamera
    \inqmlmodule QtQuick3D
    \brief Defines a PerspectiveCamera with a custom frustum.

    A \l Camera defines how the content of the 3D scene is projected onto a 2D surface,
    such as a View3D. A scene needs at least one \l Camera in order to visualize its
    contents.

    It is possible to position and rotate the \l Camera like any other spatial \l{QtQuick3D::Node}{Node} in
    the scene. The \l{QtQuick3D::Node}{Node}'s location and orientation determines where the \l Camera is in
    the scene, and what direction it is facing. The default orientation of the \l Camera
    has its forward vector pointing along the negative Z axis and its up vector along
    the positive Y axis.

    The FrustumCamera type provides a PerspectiveCamera where the frustum bounds can be
    customized. This can be useful for creating asymmetrical frustums.

    The following example creates a FrustumCamera at position [0, 200, 300] in the scene, with a
    30 degree downward pitch, and where the intersection of the frustum and the near plane is
    given by the rectangle that has a bottom left corner at [-5, -5], and a top right corner
    at [5, 5].
    \code
    FrustumCamera {
        position: Qt.vector3d(0, 200, 300)
        eulerRotation.x: -30
        top: 5
        bottom: -5
        left: -5
        right: 5
    }
    \endcode

    \sa PerspectiveCamera, OrthographicCamera, CustomCamera
*/

/*!
 * \internal
 */
QQuick3DFrustumCamera::QQuick3DFrustumCamera(QQuick3DNode *parent)
    : QQuick3DPerspectiveCamera(*(new QQuick3DNodePrivate(QQuick3DNodePrivate::Type::CustomFrustumCamera)), parent)
{}

/*!
    \qmlproperty real FrustumCamera::top

    This property defines the top of the frustum on the near plane, relative to its center.
*/
float QQuick3DFrustumCamera::top() const
{
    return m_top;
}

/*!
    \qmlproperty real FrustumCamera::bottom

    This property defines the bottom of the frustum on the near plane, relative to its center.
*/
float QQuick3DFrustumCamera::bottom() const
{
    return m_bottom;
}

/*!
    \qmlproperty real FrustumCamera::right

    This property defines the right side of the frustum on the near plane, relative to its center.
*/
float QQuick3DFrustumCamera::right() const
{
    return m_right;
}

/*!
    \qmlproperty real FrustumCamera::left

    This property defines the left side of the frustum on the near plane, relative to its center.
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

QSSGRenderGraphObject *QQuick3DFrustumCamera::updateSpatialNode(QSSGRenderGraphObject *node)
{
    // NOTE: The frustum camera extends the perspective camera!
    QSSGRenderCamera *camera = static_cast<QSSGRenderCamera *>(QQuick3DPerspectiveCamera::updateSpatialNode(node));
    if (camera) {
        const bool changed = ((int(qUpdateIfNeeded(camera->top, m_top))
                              | int(qUpdateIfNeeded(camera->bottom, m_bottom))
                              | int(qUpdateIfNeeded(camera->right, m_right))
                              | int(qUpdateIfNeeded(camera->left, m_left))) != 0);
        if (changed)
            camera->markDirty(QSSGRenderCamera::DirtyFlag::CameraDirty);
    }

    return camera;
}

QT_END_NAMESPACE
