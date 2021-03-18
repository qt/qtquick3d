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
#include "qquick3dnode_p_p.h"

#include <QtQuick3DRuntimeRender/private/qssgrendernode_p.h>
#include <QtQuick3DUtils/private/qssgutils_p.h>
#include <QtQuick3D/private/qquick3dobject_p.h>

#include <QtMath>

QT_BEGIN_NAMESPACE

QQuick3DNodePrivate::QQuick3DNodePrivate(QQuick3DObjectPrivate::Type t)
    : QQuick3DObjectPrivate(t)
{

}

QQuick3DNodePrivate::~QQuick3DNodePrivate()
{

}

void QQuick3DNodePrivate::init()
{

}

void QQuick3DNodePrivate::setIsHiddenInEditor(bool isHidden)
{
    Q_Q(QQuick3DNode);
    if (isHidden == m_isHiddenInEditor)
        return;
    m_isHiddenInEditor = isHidden;
    q->update();
}

/*!
    \qmltype Node
    \inherits Object3D
    \inqmlmodule QtQuick3D
    \brief The base component for an object that exists in a 3D Scene.

    Node type can be used to wrap other objects for the purpose of grouping them, or animating them.
    This snippet shows how to use Node to animate a camera.

    \qml
    Node {
        PerspectiveCamera {
            position: Qt.vector3d(0, 0, -600)
        }

        SequentialAnimation on eulerRotation.y {
            loops: Animation.Infinite
            PropertyAnimation {
                duration: 5000
                from: 0
                to: 360
            }
        }
    }
    \endqml

    Node has to be used also if creating a scene outside of \l View3D, for example for the purpose
    of switching scenes on the fly, or showing the same scene on multiple views.

    \qml
    Node {
        id: standAloneScene

        DirectionalLight {}

        Model {
            source: "#Sphere"
            materials: [ CopperMaterial {} ]
        }

        PerspectiveCamera {
            z: -600
        }
    }

    View3D {
        importScene: standAloneScene
    }
    \endqml
*/

QQuick3DNode::QQuick3DNode(QQuick3DNode *parent)
    : QQuick3DObject(*(new QQuick3DNodePrivate(QQuick3DNodePrivate::Type::Node)), parent)
{
    Q_D(QQuick3DNode);
    d->init();
}

QQuick3DNode::QQuick3DNode(QQuick3DNodePrivate &dd, QQuick3DNode *parent)
    : QQuick3DObject(dd, parent)
{
    Q_D(QQuick3DNode);
    d->init();
}

QQuick3DNode::~QQuick3DNode() {}

/*!
    \qmlproperty real QtQuick3D::Node::x

    This property contains the x value of the position translation in
    local coordinate space.

    \sa position
*/
float QQuick3DNode::x() const
{
    Q_D(const QQuick3DNode);
    return d->m_position.x();
}

/*!
    \qmlproperty real QtQuick3D::Node::y

    This property contains the y value of the position translation in
    local coordinate space.

    \sa position
*/
float QQuick3DNode::y() const
{
    Q_D(const QQuick3DNode);
    return d->m_position.y();
}

/*!
    \qmlproperty real QtQuick3D::Node::z

    This property contains the z value of the position translation in
    local coordinate space.

    \sa position
*/
float QQuick3DNode::z() const
{
    Q_D(const QQuick3DNode);
    return d->m_position.z();
}

/*!
    \qmlproperty quaternion QtQuick3D::Node::rotation

    This property contains the rotation values for the node.
    These values are stored as a quaternion.
*/
QQuaternion QQuick3DNode::rotation() const
{
    Q_D(const QQuick3DNode);
    return d->m_rotation;
}

/*!
    \qmlproperty vector3d QtQuick3D::Node::position

    This property contains the position translation in local coordinate space.

    \sa x, y, z
*/
QVector3D QQuick3DNode::position() const
{
    Q_D(const QQuick3DNode);
    return d->m_position;
}


/*!
    \qmlproperty vector3d QtQuick3D::Node::scale

    This property contains the scale values for the x, y, and z axis.
*/
QVector3D QQuick3DNode::scale() const
{
    Q_D(const QQuick3DNode);
    return d->m_scale;
}

/*!
    \qmlproperty vector3d QtQuick3D::Node::pivot

    This property contains the pivot values for the x, y, and z axis.  These
    values are used as the pivot points when applying rotations to the node.

*/
QVector3D QQuick3DNode::pivot() const
{
    Q_D(const QQuick3DNode);
    return d->m_pivot;
}

/*!
    \qmlproperty real QtQuick3D::Node::opacity

    This property contains the local opacity value of the Node.  Since Node
    objects are not necessarialy visible, this value might not have any affect,
    but this value is inherited by all children of the Node, which might be visible.

*/
float QQuick3DNode::localOpacity() const
{
    Q_D(const QQuick3DNode);
    return d->m_opacity;
}

/*!
    \qmlproperty bool QtQuick3D::Node::visible

    When this property is true, the Node (and its children) can be visible.

*/
bool QQuick3DNode::visible() const
{
    Q_D(const QQuick3DNode);
    return d->m_visible;
}

/*!
    \qmlproperty int QtQuick3D::Node::staticFlags
    \since 5.15

    This property defines the static flags that are used to evaluate how the node is rendered.
    Currently doesn't do anything but act as a placeholder for a future implementation.
*/
int QQuick3DNode::staticFlags() const
{
    Q_D(const QQuick3DNode);
    return d->m_staticFlags;
}

QQuick3DNode *QQuick3DNode::parentNode() const
{
    // The parent of a QQuick3DNode should never be anything else than a (subclass
    // of) QQuick3DNode (but the children/leaf nodes can be something else).
    return static_cast<QQuick3DNode *>(parentItem());
}

/*!
    \qmlproperty vector3d QtQuick3D::Node::forward

    This property returns a normalized vector of the nodes forward direction
    in scene space.

    \sa up, right, mapDirectionToScene
*/
QVector3D QQuick3DNode::forward() const
{
    return mapDirectionToScene(QVector3D(0, 0, -1)).normalized();
}

/*!
    \qmlproperty vector3d QtQuick3D::Node::up

    This property returns a normalized vector of the nodes up direction
    in scene space.

    \sa forward, right, mapDirectionToScene
*/
QVector3D QQuick3DNode::up() const
{
    return mapDirectionToScene(QVector3D(0, 1, 0)).normalized();
}

/*!
    \qmlproperty vector3d QtQuick3D::Node::right

    This property returns a normalized vector of the nodes right direction
    in scene space.

    \sa forward, up, mapDirectionToScene
*/
QVector3D QQuick3DNode::right() const
{
    return mapDirectionToScene(QVector3D(1, 0, 0)).normalized();
}
/*!
    \qmlproperty vector3d QtQuick3D::Node::scenePosition

    This property returns the position of the node in scene space.

    \note This is sometimes also referred to as the global position. But
    then in the meaning "global in the 3D world", and not "global to the
    screen or desktop" (which is usually the interpretation in other Qt APIs).
    \note the position will be reported in the same orientation as the node.

    \sa mapPositionToScene
*/
QVector3D QQuick3DNode::scenePosition() const
{
    return mat44::getPosition(sceneTransform());
}

/*!
    \qmlproperty vector3d QtQuick3D::Node::sceneRotation

    This property returns the rotation of the node in scene space.
*/
QQuaternion QQuick3DNode::sceneRotation() const
{
    Q_D(const QQuick3DNode);
    return QQuaternion::fromRotationMatrix(mat44::getUpper3x3(d->sceneRotationMatrix())).normalized();
}

/*!
    \qmlproperty vector3d QtQuick3D::Node::sceneScale

    This property returns the scale of the node in scene space.
*/
QVector3D QQuick3DNode::sceneScale() const
{
    return mat44::getScale(sceneTransform());
}

/*!
    \qmlproperty matrix4x4 QtQuick3D::Node::sceneTransform

    This property returns the global transform matrix for this node.
    \note the return value will be in right-handed coordinates.
*/
QMatrix4x4 QQuick3DNode::sceneTransform() const
{
    Q_D(const QQuick3DNode);
    if (d->m_sceneTransformDirty)
        const_cast<QQuick3DNodePrivate *>(d)->calculateGlobalVariables();
    return d->m_sceneTransform;
}

void QQuick3DNodePrivate::calculateGlobalVariables()
{
    Q_Q(QQuick3DNode);
    m_sceneTransformDirty = false;
    QMatrix4x4 localTransform = calculateLocalTransform();
    QQuick3DNode *parent = q->parentNode();
    if (!parent) {
        m_sceneTransform = localTransform;
        m_hasInheritedUniformScale = true;
        return;
    }
    QQuick3DNodePrivate *privateParent = QQuick3DNodePrivate::get(parent);

    if (privateParent->m_sceneTransformDirty)
        privateParent->calculateGlobalVariables();
    m_sceneTransform = privateParent->m_sceneTransform * localTransform;

    // Check if we have an ancestor with non-uniform scale. This will decide whether
    // or not we can use the sceneTransform to extract sceneRotation and sceneScale.
    m_hasInheritedUniformScale = privateParent->m_hasInheritedUniformScale;
    if (m_hasInheritedUniformScale) {
        const QVector3D ps = privateParent->m_scale;
        m_hasInheritedUniformScale = qFuzzyCompare(ps.x(), ps.y()) && qFuzzyCompare(ps.x(), ps.z());
    }
}

QMatrix4x4 QQuick3DNodePrivate::calculateLocalTransform()
{
    const QVector3D pivot = -m_pivot * m_scale;
    QMatrix4x4 localTransform;

    localTransform(0, 0) = m_scale[0];
    localTransform(1, 1) = m_scale[1];
    localTransform(2, 2) = m_scale[2];

    localTransform(0, 3) = pivot[0];
    localTransform(1, 3) = pivot[1];
    localTransform(2, 3) = pivot[2];

    localTransform = localRotationMatrix() * localTransform;

    localTransform(0, 3) += m_position[0];
    localTransform(1, 3) += m_position[1];
    localTransform(2, 3) += m_position[2];

    return localTransform;
}

QMatrix4x4 QQuick3DNodePrivate::localRotationMatrix() const
{
    return QMatrix4x4(m_rotation.toRotationMatrix());
}

QMatrix4x4 QQuick3DNodePrivate::sceneRotationMatrix() const
{
    Q_Q(const QQuick3DNode);

    if (m_sceneTransformDirty) {
        // Ensure m_hasInheritedUniformScale is up to date
        const_cast<QQuick3DNodePrivate *>(this)->calculateGlobalVariables();
    }

    if (m_hasInheritedUniformScale) {
        // When we know that every node up to the root have a uniform scale, we can extract the
        // rotation directly from the sceneTransform(). This is optimizing, since we reuse that
        // matrix for more than just calculating the sceneRotation.
        QMatrix4x4 rotationMatrix = q->sceneTransform();
        mat44::normalize(rotationMatrix);
        return rotationMatrix;
    }

    // When we have an ancestor that has a non-uniform scale, we cannot extract
    // the rotation from the sceneMatrix directly. Instead, we need to calculate
    // it separately, which is slightly more costly.
    const QMatrix4x4 parentRotationMatrix = QQuick3DNodePrivate::get(q->parentNode())->sceneRotationMatrix();
    return localRotationMatrix() * parentRotationMatrix;
}

void QQuick3DNodePrivate::emitChangesToSceneTransform()
{
    Q_Q(QQuick3DNode);
    const QVector3D prevPosition = mat44::getPosition(m_sceneTransform);
    const QQuaternion prevRotation = QQuaternion::fromRotationMatrix(mat44::getUpper3x3(m_sceneTransform)).normalized();
    const QVector3D prevScale = mat44::getScale(m_sceneTransform);

    calculateGlobalVariables();

    const QVector3D newPosition = mat44::getPosition(m_sceneTransform);
    const QQuaternion newRotation = QQuaternion::fromRotationMatrix(mat44::getUpper3x3(m_sceneTransform)).normalized();
    const QVector3D newScale = mat44::getScale(m_sceneTransform);

    const bool positionChanged = prevPosition != newPosition;
    const bool rotationChanged = prevRotation != newRotation;
    const bool scaleChanged = prevScale != newScale;

    if (!positionChanged && !rotationChanged && !scaleChanged)
        return;

    emit q->sceneTransformChanged();

    if (positionChanged)
        emit q->scenePositionChanged();
    if (rotationChanged)
        emit q->sceneRotationChanged();
    if (scaleChanged)
        emit q->sceneScaleChanged();
}

bool QQuick3DNodePrivate::isSceneTransformRelatedSignal(const QMetaMethod &signal) const
{
    // Return true if its likely that we need to emit
    // the given signal when our global transform changes.
    static const QMetaMethod sceneTransformSignal = QMetaMethod::fromSignal(&QQuick3DNode::sceneTransformChanged);
    static const QMetaMethod scenePositionSignal = QMetaMethod::fromSignal(&QQuick3DNode::scenePositionChanged);
    static const QMetaMethod sceneRotationSignal = QMetaMethod::fromSignal(&QQuick3DNode::sceneRotationChanged);
    static const QMetaMethod sceneScaleSignal = QMetaMethod::fromSignal(&QQuick3DNode::sceneScaleChanged);

    return (signal == sceneTransformSignal
            || signal == scenePositionSignal
            || signal == sceneRotationSignal
            || signal == sceneScaleSignal);
}

void QQuick3DNode::connectNotify(const QMetaMethod &signal)
{
    Q_D(QQuick3DNode);
    // Since we want to avoid calculating the global transform in the frontend
    // unnecessary, we keep track of the number of connections/QML bindings
    // that needs it. If there are no connections, we can skip calculating it
    // whenever our geometry changes (unless someone asks for it explicitly).
    if (d->isSceneTransformRelatedSignal(signal))
        d->m_sceneTransformConnectionCount++;
}

void QQuick3DNode::disconnectNotify(const QMetaMethod &signal)
{
    Q_D(QQuick3DNode);
    if (d->isSceneTransformRelatedSignal(signal))
        d->m_sceneTransformConnectionCount--;
}

void QQuick3DNode::componentComplete()
{
    Q_D(QQuick3DNode);
    QQuick3DObject::componentComplete();
    if (d->m_sceneTransformConnectionCount > 0)
        d->emitChangesToSceneTransform();
}

void QQuick3DNodePrivate::markSceneTransformDirty()
{
    Q_Q(QQuick3DNode);
    // Note: we recursively set m_sceneTransformDirty to true whenever our geometry
    // changes. But we only set it back to false if someone actually queries our global
    // transform (because only then do we need to calculate it). This means that if no
    // one ever does that, m_sceneTransformDirty will remain true, perhaps through out
    // the life time of the node. This is in contrast with the backend, which need to
    // update dirty transform nodes for every scene graph sync (and clear the backend
    // dirty transform flags - QQuick3DObjectPrivate::dirtyAttributes).
    // This means that for most nodes, calling markSceneTransformDirty() should be
    // cheap, since we normally expect to return early in the following test.
    if (m_sceneTransformDirty)
        return;

    m_sceneTransformDirty = true;

    if (m_sceneTransformConnectionCount > 0)
        emitChangesToSceneTransform();

    auto children = QQuick3DObjectPrivate::get(q)->childItems;
    for (auto child : children) {
        if (auto node = qobject_cast<QQuick3DNode *>(child)) {
            QQuick3DNodePrivate::get(node)->markSceneTransformDirty();
        }
    }
}

void QQuick3DNode::setX(float x)
{
    Q_D(QQuick3DNode);
    if (qFuzzyCompare(d->m_position.x(), x))
        return;

    d->m_position.setX(x);
    d->markSceneTransformDirty();
    emit positionChanged();
    emit xChanged();
    update();
}

void QQuick3DNode::setY(float y)
{
    Q_D(QQuick3DNode);
    if (qFuzzyCompare(d->m_position.y(), y))
        return;

    d->m_position.setY(y);
    d->markSceneTransformDirty();
    emit positionChanged();
    emit yChanged();
    update();
}

void QQuick3DNode::setZ(float z)
{
    Q_D(QQuick3DNode);
    if (qFuzzyCompare(d->m_position.z(), z))
        return;

    d->m_position.setZ(z);
    d->markSceneTransformDirty();
    emit positionChanged();
    emit zChanged();
    update();
}

void QQuick3DNode::setRotation(const QQuaternion &rotation)
{
    Q_D(QQuick3DNode);
    if (d->m_rotation == rotation)
        return;

    d->m_rotation = rotation;
    d->m_eulerRotationDirty = true;
    d->markSceneTransformDirty();
    emit rotationChanged();
    emit eulerRotationChanged();

    update();
}

void QQuick3DNode::setPosition(const QVector3D &position)
{
    Q_D(QQuick3DNode);
    if (d->m_position == position)
        return;

    const bool xUnchanged = qFuzzyCompare(position.x(), d->m_position.x());
    const bool yUnchanged = qFuzzyCompare(position.y(), d->m_position.y());
    const bool zUnchanged = qFuzzyCompare(position.z(), d->m_position.z());

    d->m_position = position;
    d->markSceneTransformDirty();
    emit positionChanged();

    if (!xUnchanged)
        emit xChanged();
    if (!yUnchanged)
        emit yChanged();
    if (!zUnchanged)
        emit zChanged();

    update();
}

void QQuick3DNode::setScale(const QVector3D &scale)
{
    Q_D(QQuick3DNode);
    if (d->m_scale == scale)
        return;

    d->m_scale = scale;
    d->markSceneTransformDirty();
    emit scaleChanged();
    update();
}

void QQuick3DNode::setPivot(const QVector3D &pivot)
{
    Q_D(QQuick3DNode);
    if (d->m_pivot == pivot)
        return;

    d->m_pivot = pivot;
    d->markSceneTransformDirty();
    emit pivotChanged();
    update();
}

void QQuick3DNode::setLocalOpacity(float opacity)
{
    Q_D(QQuick3DNode);
    if (qFuzzyCompare(d->m_opacity, opacity))
        return;

    d->m_opacity = opacity;
    emit localOpacityChanged();
    update();
}

void QQuick3DNode::setVisible(bool visible)
{
    Q_D(QQuick3DNode);
    if (d->m_visible == visible)
        return;

    d->m_visible = visible;
    emit visibleChanged();
    update();
}

void QQuick3DNode::setStaticFlags(int staticFlags)
{
    Q_D(QQuick3DNode);
    if (d->m_staticFlags == staticFlags)
        return;

    d->m_staticFlags = staticFlags;
    emit staticFlagsChanged();
    update();
}

void QQuick3DNode::setEulerRotation(const QVector3D &eulerRotation) {
    Q_D(QQuick3DNode);
    if (this->eulerRotation() == eulerRotation)
        return;

    d->m_eulerRotationAngles = eulerRotation;

    QQuaternion rotation = QQuaternion::fromEulerAngles(d->m_eulerRotationAngles);
    if (rotation != d->m_rotation) {
        d->m_rotation = rotation;
        emit rotationChanged();
        d->markSceneTransformDirty();
    }

    emit eulerRotationChanged();
    update();
}

/*!
    \qmlproperty enumeration QQuick3D::Node::TransformSpace

    Defines the relationship of the axis.

    \value Node.LocalSpace
           Axis is relative to the local orientation of this node.
    \value Node.ParentSpace
           Axis is relative to the local orientation of the parent node.
    \value Node.SceneSpace
           Axis is relative to the scene.
*/

/*!
    \qmlmethod QQuick3D::Node::rotate(real degrees, vector3d axis, TransformSpace space)

    Rotates this node around an \a axis by the given \a degrees. The specified
    rotation will be added to the node's current rotation. The axis can
    be specified relative to different \l {TransformSpace}{\a {space}s}.
*/
void QQuick3DNode::rotate(qreal degrees, const QVector3D &axis, TransformSpace space)
{
    Q_D(QQuick3DNode);

    const QQuaternion addRotationQuat = QQuaternion::fromAxisAndAngle(axis, float(degrees));
    const QMatrix4x4 addRotationMatrix = QMatrix4x4(addRotationQuat.toRotationMatrix());
    QMatrix4x4 newRotationMatrix;

    switch (space) {
    case LocalSpace:
        newRotationMatrix = d->localRotationMatrix() * addRotationMatrix;
        break;
    case ParentSpace:
        newRotationMatrix = addRotationMatrix * d->localRotationMatrix();
        break;
    case SceneSpace:
        if (const auto parent = parentNode()) {
            const QMatrix4x4 lrm = d->localRotationMatrix();
            const QMatrix4x4 prm = QQuick3DNodePrivate::get(parent)->sceneRotationMatrix();
            newRotationMatrix = prm.inverted() * addRotationMatrix * prm * lrm;
        } else {
            newRotationMatrix = d->localRotationMatrix() * addRotationMatrix;
        }
        break;
    }

    const QQuaternion newRotationQuaternion = QQuaternion::fromRotationMatrix(mat44::getUpper3x3(newRotationMatrix)).normalized();

    if (d->m_rotation == newRotationQuaternion)
        return;

    d->m_rotation = newRotationQuaternion;
    d->m_eulerRotationDirty = true;
    d->markSceneTransformDirty();

    emit rotationChanged();
    emit eulerRotationChanged();

    update();
}

QSSGRenderGraphObject *QQuick3DNode::updateSpatialNode(QSSGRenderGraphObject *node)
{
    Q_D(QQuick3DNode);
    if (!node) {
        markAllDirty();
        node = new QSSGRenderNode();
    }

    auto spacialNode = static_cast<QSSGRenderNode *>(node);
    bool transformIsDirty = false;
    if (spacialNode->position != d->m_position) {
        transformIsDirty = true;
        spacialNode->position = d->m_position;
    }

    if (spacialNode->rotation != d->m_rotation) {
        transformIsDirty = true;
        spacialNode->rotation = d->m_rotation;
    }

    if (spacialNode->scale != d->m_scale) {
        transformIsDirty = true;
        spacialNode->scale = d->m_scale;
    }
    if (spacialNode->pivot != d->m_pivot) {
        transformIsDirty = true;
        spacialNode->pivot = d->m_pivot;
    }

    spacialNode->staticFlags = d->m_staticFlags;
    spacialNode->localOpacity = d->m_opacity;

    // The Hidden in Editor flag overrides the visible value
    if (d->m_isHiddenInEditor)
        spacialNode->flags.setFlag(QSSGRenderNode::Flag::Active, false);
    else
        spacialNode->flags.setFlag(QSSGRenderNode::Flag::Active, d->m_visible);

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

/*!
    \qmlmethod vector3d QtQuick3D::Node::mapPositionToScene(vector3d localPosition)

    Transforms \a localPosition from local space to scene space.

    \note "Scene space" is sometimes also referred to as the "global space". But
    then in the meaning "global in the 3D world", and not "global to the
    screen or desktop" (which is usually the interpretation in other Qt APIs).

    \sa mapPositionFromScene, mapPositionToNode, mapPositionFromNode
*/
QVector3D QQuick3DNode::mapPositionToScene(const QVector3D &localPosition) const
{
    return mat44::transform(sceneTransform(), localPosition);
}

/*!
    \qmlmethod vector3d QtQuick3D::Node::mapPositionFromScene(vector3d scenePosition)

    Transforms \a scenePosition from scene space to local space.

    \sa mapPositionToScene, mapPositionToNode, mapPositionFromNode
*/
QVector3D QQuick3DNode::mapPositionFromScene(const QVector3D &scenePosition) const
{
    return mat44::transform(sceneTransform().inverted(), scenePosition);
}

/*!
    \qmlmethod vector3d QtQuick3D::Node::mapPositionToNode(QtQuick3D::Node node, vector3d localPosition)

    Transforms \a localPosition from the local space of this node to
    the local space of \a node.

    \sa mapPositionToScene, mapPositionFromScene, mapPositionFromNode
*/
QVector3D QQuick3DNode::mapPositionToNode(QQuick3DNode *node, const QVector3D &localPosition) const
{
    return node->mapPositionFromScene(mapPositionToScene(localPosition));
}

/*!
    \qmlmethod vector3d QtQuick3D::Node::mapPositionFromNode(QtQuick3D::Node node, vector3d localPosition)

    Transforms \a localPosition from the local space of \a node to
    the local space of this node.

    \sa mapPositionToScene, mapPositionFromScene, mapPositionToNode
*/
QVector3D QQuick3DNode::mapPositionFromNode(QQuick3DNode *node, const QVector3D &localPosition) const
{
    return mapPositionFromScene(node->mapPositionToScene(localPosition));
}

/*!
    \qmlmethod vector3d QtQuick3D::Node::mapDirectionToScene(vector3d localDirection)

    Transforms \a localDirection from local space to scene space.
    The return value is not affected by the (inherited) scale or
    position of the node.

    \note the return value will have the same length as \a localDirection
    (i.e. not normalized).

    \sa mapDirectionFromScene, mapDirectionToNode, mapDirectionFromNode
*/
QVector3D QQuick3DNode::mapDirectionToScene(const QVector3D &localDirection) const
{
    QMatrix3x3 theDirMatrix = mat44::getUpper3x3(sceneTransform());
    theDirMatrix = mat33::getInverse(theDirMatrix).transposed();
    return mat33::transform(theDirMatrix, localDirection);
}

/*!
    \qmlmethod vector3d QtQuick3D::Node::mapDirectionFromScene(vector3d sceneDirection)

    Transforms \a sceneDirection from scene space to local space.
    The return value is not affected by the (inherited) scale or
    position of the node.

    \note the return value will have the same length as \a sceneDirection
    (i.e not normalized).

    \sa mapDirectionToScene, mapDirectionToNode, mapDirectionFromNode
*/
QVector3D QQuick3DNode::mapDirectionFromScene(const QVector3D &sceneDirection) const
{
    QMatrix3x3 theDirMatrix = mat44::getUpper3x3(sceneTransform());
    theDirMatrix = theDirMatrix.transposed();
    return mat33::transform(theDirMatrix, sceneDirection);
}

/*!
    \qmlmethod vector3d QtQuick3D::Node::mapDirectionToNode(QtQuick3D::Node node, vector3d localDirection)

    Transforms \a localDirection from this nodes local space to the
    local space of \a node.
    The return value is not affected by the (inherited) scale or
    position of the node.

    \note the return value will have the same length as \a localDirection
    (i.e. not normalized).

    \sa mapDirectionFromNode, mapDirectionFromScene, mapDirectionToScene
*/
QVector3D QQuick3DNode::mapDirectionToNode(QQuick3DNode *node, const QVector3D &localDirection) const
{
    return node->mapDirectionFromScene(mapDirectionToScene(localDirection));
}

/*!
    \qmlmethod vector3d QtQuick3D::Node::mapDirectionFromNode(QtQuick3D::Node node, vector3d localDirection)

    Transforms \a localDirection from the local space of \a node to the
    local space of this node.
    The return value is not affected by the (inherited) scale or
    position of the node.

    \note the return value will have the same length as \a localDirection
    (i.e. not normalized).

    \sa mapDirectionToNode, mapDirectionFromScene, mapDirectionToScene
*/
QVector3D QQuick3DNode::mapDirectionFromNode(QQuick3DNode *node, const QVector3D &localDirection) const
{
    return mapDirectionFromScene(node->mapDirectionToScene(localDirection));
}

void QQuick3DNode::markAllDirty()
{
    Q_D(QQuick3DNode);

    d->markSceneTransformDirty();
    QQuick3DObject::markAllDirty();
}

/*!
    \qmlproperty vector3d QtQuick3D::Node::eulerRotation

    This property contains the rotation values for the x, y, and z axis.
    These values are stored as a vector3d.  Rotation order is assumed to
    be XYZ.
*/

QVector3D QQuick3DNode::eulerRotation() const
{
    const Q_D(QQuick3DNode);
    if (d->m_eulerRotationDirty) {
        auto dd = QQuick3DNodePrivate::get(const_cast<QQuick3DNode *>(this));
        dd->m_eulerRotationAngles = dd->m_rotation.toEulerAngles();
        dd->m_eulerRotationDirty = false;
    }

    return d->m_eulerRotationAngles;
}

QT_END_NAMESPACE
