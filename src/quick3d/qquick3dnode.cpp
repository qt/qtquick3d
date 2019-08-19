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

#include "qquick3dnode_p.h"

#include <QtQuick3DRuntimeRender/private/qssgrendereulerangles_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendernode_p.h>
#include <QtQuick3DUtils/private/qssgutils_p.h>

#include <QtMath>

QT_BEGIN_NAMESPACE

/*!
    \qmltype Node
    \inherits Object3D
    \instantiates QQuick3DNode
    \inqmlmodule QtQuick3D
    \brief The base component for an object that exists in a 3D Scene.


*/

QQuick3DNode::QQuick3DNode()
{
}

QQuick3DNode::~QQuick3DNode() {}

/*!
    \qmlproperty float QtQuick3D::Node::x

    This property contains the x value of the position translation in
    local coordinate space.

    \sa QtQuick3D::Node::position
*/
float QQuick3DNode::x() const
{
    return m_position.x();
}

/*!
    \qmlproperty float QtQuick3D::Node::y

    This property contains the y value of the position translation in
    local coordinate space.

    \sa QtQuick3D::Node::position
*/
float QQuick3DNode::y() const
{
    return m_position.y();
}

/*!
    \qmlproperty float QtQuick3D::Node::z

    This property contains the z value of the position translation in
    local coordinate space.

    \sa QtQuick3D::Node::position
*/
float QQuick3DNode::z() const
{
    return m_position.z();
}

/*!
    \qmlproperty vector3d QtQuick3D::Node::rotation

    This property contains the rotation values for the x, y, and z axis.
    These values are stored as euler angles.
*/
QVector3D QQuick3DNode::rotation() const
{
    return m_rotation;
}

/*!
    \qmlproperty vector3d QtQuick3D::Node::position

    This property contains the position translation in local coordinate space.

    \sa QtQuick3D::Node::x, QtQuick3D::Node::y, QtQuick3D::Node::z
*/
QVector3D QQuick3DNode::position() const
{
    return m_position;
}


/*!
    \qmlproperty vector3d QtQuick3D::Node::scale

    This property contains the scale values for the x, y, and z axis.
*/
QVector3D QQuick3DNode::scale() const
{
    return m_scale;
}

/*!
    \qmlproperty vector3d QtQuick3D::Node::pivot

    This property contains the pivot values for the x, y, and z axis.  These
    values are used as the pivot points when applying rotations to the node.

*/
QVector3D QQuick3DNode::pivot() const
{
    return m_pivot;
}

/*!
    \qmlproperty float QtQuick3D::Node::opacity

    This property contains the local opacity value of the Node.  Since Node
    objects are not necessarialy visible, this value might not have any affect,
    but this value is inherited by all children of the Node, which might be visible.

*/
float QQuick3DNode::localOpacity() const
{
    return m_opacity;
}

/*!
    \qmlproperty int QtQuick3D::Node::boneId

    This property contains the skeletonID used for skeletal animations

    \note This property currently has no effect, since skeletal animations are
    not implimented.

*/
qint32 QQuick3DNode::skeletonId() const
{
    return m_boneid;
}

/*!
    \qmlproperty enumeration QtQuick3D::Node::rotationOrder

    This property defines in what order the Node::rotation properties components
    are applied in.

*/
QQuick3DNode::RotationOrder QQuick3DNode::rotationOrder() const
{
    return m_rotationorder;
}

/*!
    \qmlproperty enumeration QtQuick3D::Node::orientation

    This property defines whether the Node is using a RightHanded or Lefthanded
    coordinate system.


*/
QQuick3DNode::Orientation QQuick3DNode::orientation() const
{
    return m_orientation;
}

/*!
    \qmlproperty bool QtQuick3D::Node::visible

    When this property is true, the Node (and its children) can be visible.

*/
bool QQuick3DNode::visible() const
{
    return m_visible;
}

QQuick3DNode *QQuick3DNode::parentNode() const
{
    // The parent of a QQuick3DNode should never be anything else than a (subclass
    // of) QQuick3DNode (but the children/leaf nodes can be something else).
    return static_cast<QQuick3DNode *>(parentItem());
}

/*!
    \qmlproperty vector3d QtQuick3D::Node::forward

    This property returns a normalized vector of its forward direction.


*/
QVector3D QQuick3DNode::forward() const
{
    QMatrix3x3 theDirMatrix = mat44::getUpper3x3(globalTransform());
    theDirMatrix = mat33::getInverse(theDirMatrix).transposed();

    const QVector3D frontVector(0, 0, 1);
    return mat33::transform(theDirMatrix, frontVector).normalized();
}

/*!
    \qmlproperty vector3d QtQuick3D::Node::up

    This property returns a normalized vector of its up direction.

*/
QVector3D QQuick3DNode::up() const
{
    QMatrix3x3 theDirMatrix = mat44::getUpper3x3(globalTransform());
    theDirMatrix = mat33::getInverse(theDirMatrix).transposed();

    const QVector3D upVector(0, 1, 0);
    return mat33::transform(theDirMatrix, upVector).normalized();
}

/*!
    \qmlproperty vector3d QtQuick3D::Node::right

    This property returns a normalized vector of its up direction.

*/
QVector3D QQuick3DNode::right() const
{
    QMatrix3x3 theDirMatrix = mat44::getUpper3x3(globalTransform());
    theDirMatrix = mat33::getInverse(theDirMatrix).transposed();

    const QVector3D rightVector(1, 0, 0);
    return mat33::transform(theDirMatrix, rightVector).normalized();
}
/*!
    \qmlproperty vector3d QtQuick3D::Node::globalPosition

    This property returns the position of the node in global coordinate space.
    \note the position will be reported in the same orientation as the node.
*/
QVector3D QQuick3DNode::globalPosition() const
{
    const QMatrix4x4 transform = globalTransform();
    return QVector3D(transform(0, 3), transform(1, 3), transform(2, 3));
}

/*!
    \qmlproperty vector3d QtQuick3D::Node::globalRotation

    This property returns the rotation of the node in global coordinate space.
*/
QVector3D QQuick3DNode::globalRotation() const
{
    const QMatrix3x3 rotationMatrix = mat44::getUpper3x3(globalTransform());
    const QVector3D radians = QSSGEulerAngleConverter::calculateRotationVector(rotationMatrix, false, m_rotationorder);
    const float angleX = qRadiansToDegrees(radians.x());
    const float angleY = qRadiansToDegrees(radians.y());
    const float angleZ = qRadiansToDegrees(radians.z());
    return QVector3D(angleX, angleY, angleZ);
}

/*!
    \qmlproperty vector3d QtQuick3D::Node::globalScale

    This property returns the scale of the node in global coordinate space.
*/
QVector3D QQuick3DNode::globalScale() const
{
    const QMatrix4x4 transform = globalTransform();
    const float scaleX = transform.column(0).length();
    const float scaleY = transform.column(1).length();
    const float scaleZ = transform.column(2).length();
    return QVector3D(scaleX, scaleY, scaleZ);
}

/*!
    \qmlproperty matrix4x4 QtQuick3D::Node::globalTransform

    This property returns the global transform matrix for this node.
    \note the return value will be \l LeftHanded or \l RightHanded
    depending on \l orientation().

    \sa globalTransformRightHanded()
*/
QMatrix4x4 QQuick3DNode::globalTransform() const
{
    return m_orientation == LeftHanded ? globalTransformLeftHanded() : globalTransformRightHanded();
}

/*!
    This function returns the global transform matrix for this node
    as a left-handed coordinate system, regardless of \l orientation().

    \sa globalTransform() globalTransformRightHanded()
*/
QMatrix4x4 QQuick3DNode::globalTransformLeftHanded() const
{
    QMatrix4x4 transform = globalTransformRightHanded();
    mat44::flip(transform);
    return transform;
}

/*!
    This function returns the global transform matrix for this node
    as a right-handed coordinate system, regardless of \l orientation().

    \sa globalTransform() globalTransformLeftHanded()
*/
QMatrix4x4 QQuick3DNode::globalTransformRightHanded() const
{
    // TODO: don't call this if not dirty
    const_cast<QQuick3DNode *>(this)->calculateGlobalVariables();
    return m_globalTransformRightHanded;
}

void QQuick3DNode::calculateGlobalVariables()
{
    QMatrix4x4 localTransformRightHanded = calculateLocalTransformRightHanded();
    QQuick3DNode *parent = parentNode();
    if (!parent) {
        m_globalTransformRightHanded = localTransformRightHanded;
        return;
    }

    parent->calculateGlobalVariables();
    m_globalTransformRightHanded = parent->m_globalTransformRightHanded * localTransformRightHanded;
}

QMatrix4x4 QQuick3DNode::calculateLocalTransformRightHanded()
{
    // Create a right-handed rotation transform from the radians.
    const float radX = qDegreesToRadians(m_rotation.x());
    const float radY = qDegreesToRadians(m_rotation.y());
    const float radZ = qDegreesToRadians(m_rotation.z());
    const QVector3D radians(radX, radY, radZ);
    const QMatrix4x4 rotationTransform = QSSGEulerAngleConverter::createRotationMatrix(radians, quint32(m_rotationorder));

    const QVector3D pivot = -m_pivot * m_scale;
    QMatrix4x4 localTransform;

    localTransform(0, 0) = m_scale[0];
    localTransform(1, 1) = m_scale[1];
    localTransform(2, 2) = m_scale[2];

    localTransform(0, 3) = pivot[0];
    localTransform(1, 3) = pivot[1];
    localTransform(2, 3) = pivot[2];

    localTransform = rotationTransform * localTransform;

    localTransform(0, 3) += m_position[0];
    localTransform(1, 3) += m_position[1];
    localTransform(2, 3) += m_position[2];

    if (Q_LIKELY(m_orientation == LeftHanded))
        mat44::flip(localTransform);

    return localTransform;
}

QQuick3DObject::Type QQuick3DNode::type() const
{
    return QQuick3DObject::Node;
}

void QQuick3DNode::setX(float x)
{
    if (qFuzzyCompare(m_position.x(), x))
        return;

    m_position.setX(x);
    emit positionChanged(m_position);
    emit xChanged(x);
    update();
}

void QQuick3DNode::setY(float y)
{
    if (qFuzzyCompare(m_position.y(), y))
        return;

    m_position.setY(y);
    emit positionChanged(m_position);
    emit yChanged(y);
    update();
}

void QQuick3DNode::setZ(float z)
{
    if (qFuzzyCompare(m_position.z(), z))
        return;

    m_position.setZ(z);
    emit positionChanged(m_position);
    emit zChanged(z);
    update();
}

void QQuick3DNode::setRotation(QVector3D rotation)
{
    if (m_rotation == rotation)
        return;

    m_rotation = rotation;
    emit rotationChanged(m_rotation);
    update();
}

void QQuick3DNode::setPosition(QVector3D position)
{
    if (m_position == position)
        return;

    const bool xUnchanged = qFuzzyCompare(position.x(), m_position.x());
    const bool yUnchanged = qFuzzyCompare(position.y(), m_position.y());
    const bool zUnchanged = qFuzzyCompare(position.z(), m_position.z());

    m_position = position;
    emit positionChanged(m_position);

    if (!xUnchanged)
        emit xChanged(m_position.x());
    if (!yUnchanged)
        emit yChanged(m_position.y());
    if (!zUnchanged)
        emit zChanged(m_position.z());

    update();
}

void QQuick3DNode::setScale(QVector3D scale)
{
    if (m_scale == scale)
        return;

    m_scale = scale;
    emit scaleChanged(m_scale);
    update();
}

void QQuick3DNode::setPivot(QVector3D pivot)
{
    if (m_pivot == pivot)
        return;

    m_pivot = pivot;
    emit pivotChanged(m_pivot);
    update();
}

void QQuick3DNode::setLocalOpacity(float opacity)
{
    if (qFuzzyCompare(m_opacity, opacity))
        return;

    m_opacity = opacity;
    emit localOpacityChanged(m_opacity);
    update();
}

void QQuick3DNode::setSkeletonId(qint32 boneid)
{
    if (m_boneid == boneid)
        return;

    m_boneid = boneid;
    emit skeletonIdChanged(m_boneid);
    update();
}

void QQuick3DNode::setRotationOrder(QQuick3DNode::RotationOrder rotationorder)
{
    if (m_rotationorder == rotationorder)
        return;

    m_rotationorder = rotationorder;
    emit rotationOrderChanged(m_rotationorder);
    update();
}

void QQuick3DNode::setOrientation(QQuick3DNode::Orientation orientation)
{
    if (m_orientation == orientation)
        return;

    m_orientation = orientation;
    emit orientationChanged(m_orientation);
    update();
}

void QQuick3DNode::setVisible(bool visible)
{
    if (m_visible == visible)
        return;

    m_visible = visible;
    emit visibleChanged(m_visible);
    update();
}

QSSGRenderGraphObject *QQuick3DNode::updateSpatialNode(QSSGRenderGraphObject *node)
{
    if (!node)
        node = new QSSGRenderNode();

    auto spacialNode = static_cast<QSSGRenderNode *>(node);
    bool transformIsDirty = false;
    if (spacialNode->position != m_position) {
        transformIsDirty = true;
        spacialNode->position = m_position;
    }
    if (spacialNode->rotation != m_rotation) {
        transformIsDirty = true;
        spacialNode->rotation = QVector3D(qDegreesToRadians(m_rotation.x()),
                                          qDegreesToRadians(m_rotation.y()),
                                          qDegreesToRadians(m_rotation.z()));
    }
    if (spacialNode->scale != m_scale) {
        transformIsDirty = true;
        spacialNode->scale = m_scale;
    }
    if (spacialNode->pivot != m_pivot) {
        transformIsDirty = true;
        spacialNode->pivot = m_pivot;
    }

    if (spacialNode->rotationOrder != quint32(m_rotationorder)) {
        transformIsDirty = true;
        spacialNode->rotationOrder = quint32(m_rotationorder);
    }

    spacialNode->localOpacity = m_opacity;
    spacialNode->skeletonId = m_boneid;

    if (m_orientation == LeftHanded)
        spacialNode->flags.setFlag(QSSGRenderNode::Flag::LeftHanded, true);
    else
        spacialNode->flags.setFlag(QSSGRenderNode::Flag::LeftHanded, false);

    spacialNode->flags.setFlag(QSSGRenderNode::Flag::Active, m_visible);

    if (transformIsDirty) {
        spacialNode->markDirty(QSSGRenderNode::TransformDirtyFlag::TransformIsDirty);
        spacialNode->calculateGlobalVariables();
        // Still needs to be marked dirty if it will show up correctly in the backend
        // Note: no longer sure if this is still needed after we now do our own
        // calculation of the global matrix from the front-end.
        spacialNode->flags.setFlag(QSSGRenderNode::Flag::Dirty, true);
    } else {
        spacialNode->markDirty(QSSGRenderNode::TransformDirtyFlag::TransformNotDirty);
    }

    return spacialNode;
}

QT_END_NAMESPACE
