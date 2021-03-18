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

#include "qquick3dquaternionutils_p.h"
#include "qquick3dnode_p_p.h"

#include <QtMath>
#include <QtQuick3DUtils/private/qssgutils_p.h>

#include "qquick3dutils_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype Camera
    \inherits Node
    \inqmlmodule QtQuick3D
    \brief Defines an abstract base for Cameras.

    A Camera is always necessary to view the content of a 3D scene. A camera
    defines how to project the content of a 3D scene into a 2D coordinate space,
    which can then be used on a 2D surface. When a camera is present in the scene
    it can be used to direct what is displayed in a View3D.

    To determine the projection of this camera a high level API is provided.
    First it is possible to position this Camera like any other spatial Node in
    the scene. This determines where the Camera is in the scene, and what
    direction it is facing. The default direction of the camera is such that the
    forward vector is looking up the -Z axis, and the up direction vector is up
    the +Y axis. With this in mind any transformation applied to the camera as
    well as the transformations inherited from it's parent Nodes you can define
    exactly where and in what direction your camera is facing.

    The second part of determining the projection of the camera is defining the
    frustum that defines the what parts of the scenes are visible, as well as
    how they are visible. The Camera subtypes provide multiple options
    to determine the shape of the Camera's frustum.

    \sa PerspectiveCamera, OrthographicCamera, FrustumCamera, CustomCamera
*/

/*!
   \qmlproperty enumeration Camera::FieldOfViewOrientation

   This enum type specifies the orientation in which camera field of view is given:

   \value Camera.Vertical
          Camera field of view is vertical, i.e. aspect ratio is adjusted vertically.
          This is the default orientation.
   \value Camera.Horizontal
          Camera field of view is horizontal, i.e. aspect ratio is adjusted horizontally.
  */

/*!
    \internal
*/
QQuick3DCamera::QQuick3DCamera(QQuick3DNode *parent)
    : QQuick3DNode(*(new QQuick3DNodePrivate(QQuick3DNodePrivate::Type::Camera)), parent) {}

/*!
    \internal
*/
QSSGRenderCamera *QQuick3DCamera::cameraNode() const
{
    return m_cameraNode;
}

/*!
    \qmlproperty bool Camera::frustumCullingEnabled

    When this property is \c true object outside the frustum will be culled, meaning they will
    not be rendered. By default this property is set to \c false, but for complex scene where
    a lot of the objects are outside the camera frustum it might be beneficial to enable
    frustum culling.
*/
bool QQuick3DCamera::frustumCullingEnabled() const
{
    return m_frustumCullingEnabled;
}

void QQuick3DCamera::setFrustumCullingEnabled(bool frustumCullingEnabled)
{
    if (m_frustumCullingEnabled == frustumCullingEnabled)
        return;

    m_frustumCullingEnabled = frustumCullingEnabled;
    emit frustumCullingEnabledChanged();
    update();
}

/*!
    \qmlmethod vector3d Camera::mapToViewport(vector3d scenePos)

    Transforms \a scenePos from global scene space (3D) into viewport space (2D).
    The returned position is normalized, with the top-left of the viewport
    being [0,0] and the bottom-right being [1,1]. The returned z-value will contain
    the distance from the near side of the frustum (clipNear) to \a scenePos in view
    coordinates. If the distance is negative, the point is behind camera.
    If \a scenePos cannot be mapped to a position in the viewport, a
    position of [0, 0, 0] is returned.

    \sa mapFromViewport(), {View3D::mapFrom3DScene()}{View3D.mapFrom3DScene()}
*/
QVector3D QQuick3DCamera::mapToViewport(const QVector3D &scenePos) const
{
    if (!m_cameraNode)
        return QVector3D(0, 0, 0);

    QVector4D scenePosRightHand(scenePos, 1);

    // Transform position
    const QMatrix4x4 sceneToCamera = sceneTransform().inverted();
    const QMatrix4x4 projectionViewMatrix = m_cameraNode->projection * sceneToCamera;
    const QVector4D transformedScenePos = mat44::transform(projectionViewMatrix, scenePosRightHand);

    if (qFuzzyIsNull(transformedScenePos.w()))
        return QVector3D(0, 0, 0);

    // Normalize scenePosView between [-1, 1]
    QVector3D scenePosView = transformedScenePos.toVector3D() / transformedScenePos.w();

    // Set z to be the scene distance from clipNear so that the return value
    // can be used as argument to viewportToscene() to reverse the call.
    const QVector4D clipNearPos(scenePosView.x(), scenePosView.y(), -1, 1);
    auto invProj = projectionViewMatrix.inverted();
    const QVector4D clipNearPosTransformed = mat44::transform(invProj, clipNearPos);
    const QVector4D clipNearPosScene = clipNearPosTransformed / clipNearPosTransformed.w();
    QVector4D clipFarPos = clipNearPos;
    clipFarPos.setZ(0);
    const QVector4D clipFarPosTransformed = mat44::transform(invProj, clipFarPos);
    const QVector4D clipFarPosScene = clipFarPosTransformed / clipFarPosTransformed.w();
    const QVector3D direction = (clipFarPosScene - clipNearPosScene).toVector3D();
    const QVector3D scenePosVec = (scenePosRightHand - clipNearPosScene).toVector3D();

    const float distanceToScenePos = scenePosVec.length()
            * (QVector3D::dotProduct(direction, scenePosVec) > 0.f ? 1 : -1);

    scenePosView.setZ(distanceToScenePos);

    // Convert x and y to be between [0, 1]
    scenePosView.setX((scenePosView.x() / 2) + 0.5f);
    scenePosView.setY((scenePosView.y() / 2) + 0.5f);
    // And convert origin from bottom-left to top-left
    scenePosView.setY(1 - scenePosView.y());
    return scenePosView;
}

/*!
    \qmlmethod vector3d Camera::mapFromViewport(vector3d viewportPos)

    Transforms \a viewportPos from viewport space (2D) into global scene space (3D).
    The x- and y-values of \a viewportPos must be normalized, with the top-left
    of the viewport being [0,0] and the bottom-right being [1,1]. The z-value should be
    the distance from the near side of the frustum (clipNear) into the scene in scene coordinates.
    If \a viewportPos cannot be mapped to a position in the scene, a position of
    [0, 0, 0] is returned.

    \sa mapToViewport, {View3D::mapTo3DScene()}{View3D.mapTo3DScene()}
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
    const QMatrix4x4 sceneToCamera = sceneTransform().inverted();
    const QMatrix4x4 projectionViewMatrixInv
            = (m_cameraNode->projection * sceneToCamera).inverted();
    const QVector4D transformedClipNearPos = mat44::transform(projectionViewMatrixInv, clipNearPos);
    const QVector4D transformedClipFarPos = mat44::transform(projectionViewMatrixInv, clipFarPos);

    if (qFuzzyIsNull(transformedClipNearPos.w()))
        return QVector3D(0, 0, 0);

    // Reverse the projection
    const QVector3D clipNearPosScene = transformedClipNearPos.toVector3D()
            / transformedClipNearPos.w();
    const QVector3D clipFarPosScene = transformedClipFarPos.toVector3D()
            / transformedClipFarPos.w();

    // Calculate the position in the scene
    const QVector3D direction = (clipFarPosScene - clipNearPosScene).normalized();
    const float distanceFromClipNear = viewportPos.z();
    QVector3D scenePos = clipNearPosScene + (direction * distanceFromClipNear);

    return scenePos;
}

/*!
 * \internal
 */
QVector3D QQuick3DCamera::mapToViewport(const QVector3D &scenePos,
                                        qreal width,
                                        qreal height)
{
    if (!m_cameraNode) {
        m_cameraNode = new QSSGRenderCamera();
        // Ignore the returned dirty because of forcing to call calculateGlobalVariables.
        checkSpatialNode(m_cameraNode);
        m_cameraNode->calculateGlobalVariables(QRect(0, 0, width, height));
    }

    return QQuick3DCamera::mapToViewport(scenePos);
}

/*!
 * \internal
 */
QVector3D QQuick3DCamera::mapFromViewport(const QVector3D &viewportPos,
                                          qreal width,
                                          qreal height)
{
    if (!m_cameraNode) {
        m_cameraNode = new QSSGRenderCamera();
        // Ignore the returned dirty because of forcing to call calculateGlobalVariables.
        checkSpatialNode(m_cameraNode);
        m_cameraNode->calculateGlobalVariables(QRect(0, 0, width, height));
    }

    return QQuick3DCamera::mapFromViewport(viewportPos);
}

/*!
    \qmlmethod vector3d Camera::lookAt(vector3d scenePos)
    \since 5.15

    Sets the rotation value of a camera to be directed at \a scenePos.
*/

void QQuick3DCamera::lookAt(const QVector3D &scenePos)
{
    // Assumption: we never want the camera to roll.
    // We use Euler angles here to avoid roll to sneak in through numerical instability.

    const auto &targetPosition = scenePos;
    auto sourcePosition = scenePosition();

    QVector3D targetVector = sourcePosition - targetPosition;

    float yaw = qRadiansToDegrees(atan2(targetVector.x(), targetVector.z()));

    QVector2D p(targetVector.x(), targetVector.z()); // yaw vector projected to horizontal plane
    float pitch = qRadiansToDegrees(atan2(p.length(), targetVector.y())) - 90;

    const float previousRoll = eulerRotation().z();
    setEulerRotation(QVector3D(pitch, yaw, previousRoll));
}

/*!
    \qmlmethod vector3d Camera::lookAt(QtQuick3D::Node node)
    \since 5.15

    Sets the rotation value of a camera to be directed at \a node.
*/

void QQuick3DCamera::lookAt(QQuick3DNode *node)
{
    if (!node)
        return;
    lookAt(node->scenePosition());
}

void QQuick3DCamera::updateGlobalVariables(const QRectF &inViewport)
{
    QSSGRenderCamera *node = cameraNode();
    if (node)
        node->calculateGlobalVariables(inViewport);
}

/*!
 * \internal
 */
QSSGRenderGraphObject *QQuick3DCamera::updateSpatialNode(QSSGRenderGraphObject *node)
{
    if (!node) {
        markAllDirty();
        node = new QSSGRenderCamera();
    }

    QQuick3DNode::updateSpatialNode(node);

    QSSGRenderCamera *camera = static_cast<QSSGRenderCamera *>(node);

    bool changed = checkSpatialNode(camera);
    setCameraNode(camera);

    if (changed)
        camera->flags.setFlag(QSSGRenderNode::Flag::CameraDirty);
    return node;
}
QT_END_NAMESPACE
