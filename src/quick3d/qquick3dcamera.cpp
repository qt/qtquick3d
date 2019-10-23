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

#include "qquick3dcamera_p.h"

#include <QtQuick3DRuntimeRender/private/qssgrendercamera_p.h>

#include <QtMath>
#include <QtQuick3DUtils/private/qssgutils_p.h>

#include "qquick3dutils_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype Camera
    \inherits Node
    \instantiates QQuick3DCamera
    \inqmlmodule QtQuick3D
    \brief Defines a Camera for viewing the content of a 3D scene.

    A Camera is always necessary view the content of a 3D scene. A camera
    defines how to project the content of a 3D scene into a 2D coordinate space
    which can then be used on a 2D surface. When a camera is present in the scene
    it can be used to direct what is displayed in a View3D.

    To determine the projection of this camera a high level API is provided.
    First it is possible to position this Camera like any other spatial Node in
    the scene. This determines where the Camera is in the scene, and what
    direction it is facing. The default direction of the camera is such that the
    forward vector is looking up the +Z axis, and the up direction vector is up
    the +Y axis.  With this in mind any transformation applied to the camera as
    well as the transformations inherited from it's parent Nodes you can define
    exactly where and in what direction your camera is facing.

    The second part of determining the projection of the camera is defining the
    frustum that defines the what parts of the scenes are visible, as well as
    how they are visible. The Camera API provides multiple levels of abstraction
    to determine the shape of the Camera's frustum.  By setting the
    Camera::projectionMode property it is possible to control what type of
    projection is created and how much control is needed.

    The high level API for Camera is used by either selection the
    Camera::Perspective or Camera::Orthographic projectionModes. This allows for
    a sensible default for either of the two main projection types.

    For finer grain control of how the frustum is defined, there is the
    Camera::Frustum projectionMode. This allows for setting the
    Camera::frustumTop, Camera::frustumBottom, Camera::frustomRight, and
    Camera::frustumLeft properties. This is useful in creating asymetrical
    frustums.

    If you need full-control over how the projection matrix is created there is
    also the Camera::Custom mode which lets you define the projection matrix
    directly.
*/

/*!
 * \internal
 */
QQuick3DCamera::QQuick3DCamera() {}


/*!
 * \qmlproperty real Camera::clipNear
 *
 * This property holds the near value of the camara's view frustum.  This value determines
 * what the closest distance to the camera that items will be shown.
 *
 */

float QQuick3DCamera::clipNear() const
{
    return m_clipNear;
}

/*!
 * \qmlproperty real Camera::clipFar
 *
 * This property holds the far value of the camara's view frustum.  This value determines
 * what the furthest distance to the camera that items will be shown.
 *
 */

float QQuick3DCamera::clipFar() const
{
    return m_clipFar;
}

/*!
 * \qmlproperty real Camera::fieldOfView
 *
 * This property holds the field of view of the camera in degrees. This can be either the
 * vertical or horizontal field of view depending on if the isFieldOfViewHorizontal property
 * is set to \c true or not.
 *
 *
 */

float QQuick3DCamera::fieldOfView() const
{
    return m_fieldOfView;
}

/*!
 * \qmlproperty bool Camera::isFieldOfViewHorizontal
 *
 * This property determines if the field of view property reflects the horizontal
 * field of view. If this value is \c false then it is assumed field of view is vertical.
 *
 */

bool QQuick3DCamera::isFieldOfViewHorizontal() const
{
    return m_isFieldOfViewHorizontal;
}

/*!
 * \internal
 */\
QQuick3DObject::Type QQuick3DCamera::type() const
{
    return QQuick3DObject::Camera;
}

/*!
 * \internal
 */\
QSSGRenderCamera *QQuick3DCamera::getCameraNode() const
{
    return m_cameraNode;
}

/*!
    \qmlproperty float Camera::frustumTop

    This property defines the top plane of the camera view frustum.

    \note This property only has an effect when using \c Camera.Frustum for the
    Camera::projectionMode property
*/
float QQuick3DCamera::frustumTop() const
{
    return m_frustumTop;
}

/*!
    \qmlproperty float Camera::frustumBottom

    This property defines the bottom plane of the camera view frustum.

    \note This property only has an effect when using \c Camera.Frustum for the
    Camera::projectionMode property
*/
float QQuick3DCamera::frustumBottom() const
{
    return m_frustumBottom;
}

/*!
    \qmlproperty float Camera::frustumRight

    This property defines the right plane of the camera view frustum.

    \note This property only has an effect when using \c Camera.Frustum for the
    Camera::projectionMode property
*/
float QQuick3DCamera::frustumRight() const
{
    return m_frustumRight;
}

/*!
    \qmlproperty float Camera::frustumLeft

    This property defines the left plane of the camera view frustum.

    \note This property only has an effect when using \c Camera.Frustum for the
    Camera::projectionMode property
*/
float QQuick3DCamera::frustumLeft() const
{
    return m_frustumLeft;
}

/*!
    \qmlproperty float Camera::customProjection

    This property defines a custom projection matrix. This property should only
    be used for handling more advanced projections.

    \note This property only has an effect when using \c Camera.Custom for the
    Camera::projectionMode property
*/
QMatrix4x4 QQuick3DCamera::customProjection() const
{
    return m_customProjection;
}

/*!
 * \qmlproperty enumeration Camera::projectionMode
 *
 * This property defines the type of projection that will be to render the
 * scene. The most common cases are \c Camera.Perspective and
 * \c Camera.Orthographic which will automatically generate a projection
 * matrix.
 *
 * It also possible to use either \c Camera.Frustum or \c Camera.Custom to
 * have finer control over how the projection is defined.
 *
 * \list
 * \li Camera.Perspective
 * \li Camera.Orthographic
 * \li Camera.Frustum
 * \li Camera.Custom
 * \endlist
 *
 */

QQuick3DCamera::QSSGCameraProjectionMode QQuick3DCamera::projectionMode() const
{
    return m_projectionMode;
}

/*!
 * \qmlproperty bool Camera::enableFrustumCulling
 *
 * When enabled this property determines whether frustum culling is enabled for
 * this camera. What this means is that when the scene is being rendered only
 * items that are within the bounds of the fustum are rendered.  For scenes
 * where there are lots of expensive items outside of the view of the camera
 * time will not be spent rendering content that will never be shown. There is
 * however a cost for doing this as the scene has to be iterated on the CPU to
 * determine what is and isn't inside the frustum. If you know that everything
 * in the scene will always be indie the camera frustum, this step is an
 * unnecessary use of resources.
 *
 * There are also cases with shadowing where shadows can disapear before they
 * are out of view because the item causing the shadow is outside of the camera
 * frustum, but the shadow it is casting still is.
 *
 */

bool QQuick3DCamera::enableFrustumCulling() const
{
    return m_enableFrustumCulling;
}

void QQuick3DCamera::setClipNear(float clipNear)
{
    if (qFuzzyCompare(m_clipNear, clipNear))
        return;

    m_clipNear = clipNear;
    emit clipNearChanged(m_clipNear);
    update();
}

void QQuick3DCamera::setClipFar(float clipFar)
{
    if (qFuzzyCompare(m_clipFar, clipFar))
        return;

    m_clipFar = clipFar;
    emit clipFarChanged(m_clipFar);
    update();
}

void QQuick3DCamera::setFieldOfView(float fieldOfView)
{
    if (qFuzzyCompare(m_fieldOfView, fieldOfView))
        return;

    m_fieldOfView = fieldOfView;
    emit fieldOfViewChanged(m_fieldOfView);
    update();
}

void QQuick3DCamera::setIsFieldOfViewHorizontal(bool isFieldOfViewHorizontal)
{
    if (m_isFieldOfViewHorizontal == isFieldOfViewHorizontal)
        return;

    m_isFieldOfViewHorizontal = isFieldOfViewHorizontal;
    emit isFieldOfViewHorizontalChanged(m_isFieldOfViewHorizontal);
    update();
}

void QQuick3DCamera::setProjectionMode(QQuick3DCamera::QSSGCameraProjectionMode projectionMode)
{
    if (m_projectionMode == projectionMode)
        return;

    m_projectionMode = projectionMode;
    emit projectionModeChanged(m_projectionMode);
    update();
}

void QQuick3DCamera::setEnableFrustumCulling(bool enableFrustumCulling)
{
    if (m_enableFrustumCulling == enableFrustumCulling)
        return;

    m_enableFrustumCulling = enableFrustumCulling;
    emit enableFrustumCullingChanged(m_enableFrustumCulling);
    update();
}

void QQuick3DCamera::setFrustumTop(float frustumTop)
{
    if (qFuzzyCompare(m_frustumTop, frustumTop))
        return;

    m_frustumTop = frustumTop;
    emit frustumTopChanged(m_frustumTop);
    update();
}

void QQuick3DCamera::setFrustumBottom(float frustumBottom)
{
    if (qFuzzyCompare(m_frustumBottom, frustumBottom))
        return;

    m_frustumBottom = frustumBottom;
    emit frustumBottomChanged(m_frustumBottom);
    update();
}

void QQuick3DCamera::setFrustumRight(float frustumRight)
{
    if (qFuzzyCompare(m_frustumRight, frustumRight))
        return;

    m_frustumRight = frustumRight;
    emit frustumRightChanged(m_frustumRight);
    update();
}

void QQuick3DCamera::setFrustumLeft(float frustumLeft)
{
    if (qFuzzyCompare(m_frustumLeft, frustumLeft))
        return;

    m_frustumLeft = frustumLeft;
    emit frustumLeftChanged(m_frustumLeft);
    update();
}

void QQuick3DCamera::setCustomProjection(QMatrix4x4 customProjection)
{
    if (m_customProjection == customProjection)
        return;

    m_customProjection = customProjection;
    emit customProjectionChanged(m_customProjection);
    update();
}

/*!
 * \qmlmethod vector3d Camera::mapToViewport(vector3d scenePos)
 *
 * Transforms \a scenePos from global scene space (3D) into viewport space (2D).
 * The returned position is normalized, with the top-left of the viewport
 * being [0,0] and the bottom-right being [1,1]. The returned z value will contain
 * the distance from the near side of the frustum (clipNear) to \a scenePos in view
 * coordinates. If \a scenePos cannot be mapped to a position in the viewport, a
 * position of [0, 0, 0] is returned.
 *
 * \note \a scenePos should be in the same \l orientation as the camera.
 *
 * \sa QQuick3DCamera::mapFromViewport() QQuick3DViewport::mapFrom3DScene()
 */
QVector3D QQuick3DCamera::mapToViewport(const QVector3D &scenePos) const
{
    if (!m_cameraNode)
        return QVector3D(0, 0, 0);

    QVector4D scenePosRightHand(scenePos, 1);
    if (orientation() == LeftHanded) {
        // Convert from left-handed to right-handed
        scenePosRightHand.setZ(-scenePosRightHand.z());
    }

    // Transform position
    const QMatrix4x4 sceneToCamera = sceneTransformRightHanded().inverted();
    const QMatrix4x4 projectionViewMatrix = m_cameraNode->projection * sceneToCamera;
    const QVector4D transformedScenePos = mat44::transform(projectionViewMatrix, scenePosRightHand);

    if (qFuzzyIsNull(transformedScenePos.w()))
        return QVector3D(0, 0, 0);

    // Normalize scenePosView between [-1, 1]
    QVector3D scenePosView = transformedScenePos.toVector3D() / transformedScenePos.w();

    // Set z to be the scene distance from clipNear so that the return value
    // can be used as argument to viewportToscene() to reverse the call.
    const QVector4D clipNearPos(scenePosView.x(), scenePosView.y(), -1, 1);
    const QVector4D clipNearPosTransformed = mat44::transform(projectionViewMatrix.inverted(), clipNearPos);
    const QVector4D clipNearPosScene = clipNearPosTransformed / clipNearPosTransformed.w();
    const float distanceToScenePos = (scenePosRightHand - clipNearPosScene).length();
    scenePosView.setZ(distanceToScenePos);

    // Convert x and y to be between [0, 1]
    scenePosView.setX((scenePosView.x() / 2) + 0.5f);
    scenePosView.setY((scenePosView.y() / 2) + 0.5f);
    // And convert origin from bottom-left to top-left
    scenePosView.setY(1 - scenePosView.y());
    return scenePosView;
}

/*!
 * \qmlmethod vector3d Camera::mapFromViewport(vector3d viewportPos)
 *
 * Transforms \a viewportPos from viewport space (2D) into global scene space (3D).
 * \a The x-, and y values of \a viewportPos needs to be normalized, with the top-left
 * of the viewport being [0,0] and the bottom-right being [1,1]. The z value should be
 * the distance from the near side of the frustum (clipNear) into the scene in scene coordinates.
 * If \a viewportPos cannot be mapped to a position in the scene, a position of
 * [0, 0, 0] is returned.
 *
 * \note the returned position will be in the same \l orientation as the camera.
 *
 * \sa QQuick3DCamera::mapToViewport() QQuick3DViewport::mapTo3DScene()
 */
QVector3D QQuick3DCamera::mapFromViewport(const QVector3D &viewportPos) const
{
    if (!m_cameraNode)
        return QVector3D(0, 0, 0);

    // Pick two positions in the frustum
    QVector4D clipNearPos(viewportPos, 1);
    // Convert origin from top-left to bottom-left
    clipNearPos.setY(1 - clipNearPos.y());
    // Convert to homogenous position between [-1, 1]
    clipNearPos.setX((clipNearPos.x() * 2.0f) - 1.0f);
    clipNearPos.setY((clipNearPos.y() * 2.0f) - 1.0f);
    QVector4D clipFarPos = clipNearPos;
    // clipNear: z = -1, clipFar: z = 1. It's recommended to use 0 as
    // far pos instead of clipFar because of infinite projection issues.
    clipNearPos.setZ(-1);
    clipFarPos.setZ(0);

    // Transform position to scene
    const QMatrix4x4 sceneToCamera = sceneTransformRightHanded().inverted();
    const QMatrix4x4 projectionViewMatrixInv = (m_cameraNode->projection * sceneToCamera).inverted();
    const QVector4D transformedClipNearPos = mat44::transform(projectionViewMatrixInv, clipNearPos);
    const QVector4D transformedClipFarPos = mat44::transform(projectionViewMatrixInv, clipFarPos);

    if (qFuzzyIsNull(transformedClipNearPos.w()))
        return QVector3D(0, 0, 0);

    // Reverse the projection
    const QVector3D clipNearPosScene = transformedClipNearPos.toVector3D() / transformedClipNearPos.w();
    const QVector3D clipFarPosScene = transformedClipFarPos.toVector3D() / transformedClipFarPos.w();

    // Calculate the position in the scene
    const QVector3D direction = (clipFarPosScene - clipNearPosScene).normalized();
    const float distanceFromClipNear = viewportPos.z();
    QVector3D scenePos = clipNearPosScene + (direction * distanceFromClipNear);

    if (orientation() == LeftHanded) {
        // Convert from right-handed to left-handed
        scenePos.setZ(-scenePos.z());
    }

    return scenePos;
}

namespace  {
    bool updateProjectionFlags(QSSGRenderCamera *camera, QQuick3DCamera::QSSGCameraProjectionMode projectionMode)
    {
        if (projectionMode == QQuick3DCamera::Perspective) {
            if (camera->flags.testFlag(QSSGRenderNode::Flag::Orthographic) ||
                camera->flags.testFlag(QSSGRenderNode::Flag::CameraFrustumProjection) ||
                camera->flags.testFlag(QSSGRenderNode::Flag::CameraCustomProjection)) {
                camera->flags.setFlag(QSSGRenderNode::Flag::CameraFrustumProjection, false);
                camera->flags.setFlag(QSSGRenderNode::Flag::CameraCustomProjection, false);
                camera->flags.setFlag(QSSGRenderNode::Flag::Orthographic, false);
                return true;
            }
        } else if (projectionMode == QQuick3DCamera::Orthographic) {
            if (!camera->flags.testFlag(QSSGRenderNode::Flag::Orthographic) ||
                camera->flags.testFlag(QSSGRenderNode::Flag::CameraFrustumProjection) ||
                camera->flags.testFlag(QSSGRenderNode::Flag::CameraCustomProjection)) {
                camera->flags.setFlag(QSSGRenderNode::Flag::CameraFrustumProjection, false);
                camera->flags.setFlag(QSSGRenderNode::Flag::CameraCustomProjection, false);
                camera->flags.setFlag(QSSGRenderNode::Flag::Orthographic, true);
                return true;
            }
        } else if (projectionMode == QQuick3DCamera::Frustum) {
            if (camera->flags.testFlag(QSSGRenderNode::Flag::Orthographic) ||
                !camera->flags.testFlag(QSSGRenderNode::Flag::CameraFrustumProjection) ||
                camera->flags.testFlag(QSSGRenderNode::Flag::CameraCustomProjection)) {
                camera->flags.setFlag(QSSGRenderNode::Flag::CameraFrustumProjection, true);
                camera->flags.setFlag(QSSGRenderNode::Flag::CameraCustomProjection, false);
                camera->flags.setFlag(QSSGRenderNode::Flag::Orthographic, false);
                return true;
            }
        } else if (projectionMode == QQuick3DCamera::Custom) {
            if (camera->flags.testFlag(QSSGRenderNode::Flag::Orthographic) ||
                camera->flags.testFlag(QSSGRenderNode::Flag::CameraFrustumProjection) ||
                !camera->flags.testFlag(QSSGRenderNode::Flag::CameraCustomProjection)) {
                camera->flags.setFlag(QSSGRenderNode::Flag::CameraFrustumProjection, false);
                camera->flags.setFlag(QSSGRenderNode::Flag::CameraCustomProjection, true);
                camera->flags.setFlag(QSSGRenderNode::Flag::Orthographic, false);
                return true;
            }
        }

        return false;
    }
}

/*!
 * \internal
 */
QSSGRenderGraphObject *QQuick3DCamera::updateSpatialNode(QSSGRenderGraphObject *node)
{
    if (!node)
        node = new QSSGRenderCamera();

    QQuick3DNode::updateSpatialNode(node);

    QSSGRenderCamera *camera = static_cast<QSSGRenderCamera *>(node);

    bool changed = false;
    changed |= qUpdateIfNeeded(camera->clipNear, m_clipNear);
    changed |= qUpdateIfNeeded(camera->clipFar, m_clipFar);
    changed |= qUpdateIfNeeded(camera->fov, qDegreesToRadians(m_fieldOfView));
    changed |= qUpdateIfNeeded(camera->fovHorizontal, m_isFieldOfViewHorizontal);
    changed |= qUpdateIfNeeded(camera->enableFrustumClipping, m_enableFrustumCulling);

    // Set Projection mode based properties
    switch (m_projectionMode) {
    case Orthographic:
        break;
    case Frustum:
        changed |= qUpdateIfNeeded(camera->top, m_frustumTop);
        changed |= qUpdateIfNeeded(camera->bottom, m_frustumBottom);
        changed |= qUpdateIfNeeded(camera->right, m_frustumRight);
        changed |= qUpdateIfNeeded(camera->left, m_frustumLeft);
        break;
    case Custom:
        changed |= qUpdateIfNeeded(camera->projection, m_customProjection);
        break;
    case Perspective:
        break;
    }

    changed |= updateProjectionFlags(camera, m_projectionMode);

    m_cameraNode = camera;

    if (changed)
        camera->flags.setFlag(QSSGRenderNode::Flag::CameraDirty);
    return node;
}

QT_END_NAMESPACE
