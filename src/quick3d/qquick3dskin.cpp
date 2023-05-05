// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qquick3dskin_p.h"
#include "qquick3dobject_p.h"
#include "qquick3dscenemanager_p.h"

#include <QtQuick3DRuntimeRender/private/qssgrendergraphobject_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderskin_p.h>

#include <QtQuick3DUtils/private/qssgutils_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype Skin
    \inherits Object3D
    \inqmlmodule QtQuick3D
    \brief Defines a skinning animation.

    A skin defines how a model can be animated using \l {Vertex Skinning}
    {skeletal animation}. It contains a list of \l {Node}s and an optional list
    of the Inverse Bind Pose Matrices.
    Each \l {Node}'s transform becomes a transform of the bone with the
    corresponding index in the list.

    \qml
    Skin {
        id: skin0
        joints: [
            node0,
            node1,
            node2
        ]
        inverseBindPoses: [
            Qt.matrix4x4(...),
            Qt.matrix4x4(...),
            Qt.matrix4x4(...)
        ]
    }
    \endqml

    \note \l {Skeleton} and \l {Joint} will be deprecated.
*/

QQuick3DSkin::QQuick3DSkin(QQuick3DObject *parent)
    : QQuick3DObject(*(new QQuick3DObjectPrivate(QQuick3DObjectPrivate::Type::Skin)), parent)
{
}

QQuick3DSkin::~QQuick3DSkin()
{
}

/*!
    \qmlproperty List<QtQuick3D::Node> Skin::joints

    This property contains a list of nodes used for a hierarchy of joints.
    The order in the list becomes the index of the joint, which is used in the
    \c SkinSemantic \l {QQuick3DGeometry::addAttribute}{custom geometry attribute}.

    \note A value 'undefined' will be ignored and if a node which doesn't exist is
    described, the result is unpredictable.

    \sa {QQuick3DGeometry::addAttribute}, {Qt Quick 3D - Simple Skinning Example}
*/
QQmlListProperty<QQuick3DNode> QQuick3DSkin::joints()
{
    return QQmlListProperty<QQuick3DNode>(this,
                                            nullptr,
                                            QQuick3DSkin::qmlAppendJoint,
                                            QQuick3DSkin::qmlJointsCount,
                                            QQuick3DSkin::qmlJointAt,
                                            QQuick3DSkin::qmlClearJoints);
}

#define POS4BONETRANS(x)    (sizeof(float) * 16 * (x) * 2)
#define POS4BONENORM(x)     (sizeof(float) * 16 * ((x) * 2 + 1))

void QQuick3DSkin::onJointChanged(QQuick3DNode *node)
{
    for (int i = 0; i < m_joints.size(); ++i) {
        if (m_joints.at(i) == node) {
            QMatrix4x4 jointGlobal = m_joints.at(i)->sceneTransform();
            if (m_inverseBindPoses.size() > i)
                jointGlobal *= m_inverseBindPoses.at(i);
            memcpy(m_boneData.data() + POS4BONETRANS(i),
                   reinterpret_cast<const void *>(jointGlobal.constData()),
                   sizeof(float) * 16);
            memcpy(m_boneData.data() + POS4BONENORM(i),
                   reinterpret_cast<const void *>(QMatrix4x4(jointGlobal.normalMatrix()).constData()),
                   sizeof(float) * 11);
            markDirty();
        }
    }
}

void QQuick3DSkin::onJointDestroyed(QObject *object)
{
    for (int i = 0; i < m_joints.size(); ++i) {
        if (m_joints.at(i) == object) {
            m_joints.removeAt(i);
            // remove both transform and normal together
            m_boneData.remove(POS4BONETRANS(i),
                              sizeof(float) * 16 * 2);
            markDirty();
            break;
        }
    }
}

void QQuick3DSkin::qmlAppendJoint(QQmlListProperty<QQuick3DNode> *list, QQuick3DNode *joint)
{
    if (joint == nullptr)
        return;
    QQuick3DSkin *self = static_cast<QQuick3DSkin *>(list->object);
    int index = self->m_joints.size();
    self->m_joints.push_back(joint);
    QMatrix4x4 jointGlobal = joint->sceneTransform();
    if (index < self->m_inverseBindPoses.size())
        jointGlobal *= self->m_inverseBindPoses.at(index);
    self->m_boneData.append(reinterpret_cast<const char *>(jointGlobal.constData()),
                            sizeof(float) * 16);
    self->m_boneData.append(reinterpret_cast<const char *>(QMatrix4x4(jointGlobal.normalMatrix()).constData()),
                            sizeof(float) * 16);
    self->markDirty();

    connect(joint, &QQuick3DNode::sceneTransformChanged, self,
            [self, joint]() { self->onJointChanged(joint); });
    connect(joint, &QQuick3DNode::destroyed, self, &QQuick3DSkin::onJointDestroyed);
}

QQuick3DNode *QQuick3DSkin::qmlJointAt(QQmlListProperty<QQuick3DNode> *list, qsizetype index)
{
    QQuick3DSkin *self = static_cast<QQuick3DSkin *>(list->object);
    return self->m_joints.at(index);
}

qsizetype QQuick3DSkin::qmlJointsCount(QQmlListProperty<QQuick3DNode> *list)
{
    QQuick3DSkin *self = static_cast<QQuick3DSkin *>(list->object);
    return self->m_joints.size();
}

void QQuick3DSkin::qmlClearJoints(QQmlListProperty<QQuick3DNode> *list)
{
    QQuick3DSkin *self = static_cast<QQuick3DSkin *>(list->object);
    for (const auto &joint : std::as_const(self->m_joints)) {
        joint->disconnect(self, SLOT(onJointDestroyed(QObject*)));
    }
    self->m_joints.clear();
    self->m_boneData.clear();
    self->markDirty();
}


/*!
    \qmlproperty List<matrix4x4> Skin::inverseBindPoses

    This property contains a list of Inverse Bind Pose matrixes used for the
    skinning animation. Each inverseBindPose matrix means the inverse of the
    global transform of the corresponding node in \l {Skin::joints},
    used initially.

    \note This property is an optional property. That is, if some or all of the
    matrices are not set, identity values will be used.
*/
QList<QMatrix4x4> QQuick3DSkin::inverseBindPoses() const
{
    return m_inverseBindPoses;
}

void QQuick3DSkin::setInverseBindPoses(const QList<QMatrix4x4> &poses)
{
    if (m_inverseBindPoses == poses)
        return;

    m_inverseBindPoses = poses;

    for (int i = 0; i < m_joints.size(); ++i) {
        QMatrix4x4 jointGlobal = m_joints.at(i)->sceneTransform();
        if (m_inverseBindPoses.size() > i)
            jointGlobal *= m_inverseBindPoses.at(i);
        memcpy(m_boneData.data() + POS4BONETRANS(i),
               reinterpret_cast<const void *>(jointGlobal.constData()),
               sizeof(float) * 16);
        memcpy(m_boneData.data() + POS4BONENORM(i),
               reinterpret_cast<const void *>(QMatrix4x4(jointGlobal.normalMatrix()).constData()),
               sizeof(float) * 11);
    }

    markDirty();
    emit inverseBindPosesChanged();
}

void QQuick3DSkin::markDirty()
{
    if (!m_dirty) {
        m_dirty = true;
        update();
    }
}


void QQuick3DSkin::markAllDirty()
{
    m_dirty = true;
    QQuick3DObject::markAllDirty();
}

QSSGRenderGraphObject *QQuick3DSkin::updateSpatialNode(QSSGRenderGraphObject *node)
{
    if (!node) {
        markAllDirty();
        node = new QSSGRenderSkin();
    }
    QQuick3DObject::updateSpatialNode(node);
    auto skinNode = static_cast<QSSGRenderSkin *>(node);

    if (m_dirty) {
        m_dirty = false;
        const int boneTexWidth = qCeil(qSqrt(m_joints.size() * 4 * 2));
        const int textureSizeInBytes = boneTexWidth * boneTexWidth * 16;  //NB: Assumes RGBA32F set above (16 bytes per color)
        m_boneData.resize(textureSizeInBytes);
        skinNode->setSize(QSize(boneTexWidth, boneTexWidth));
        skinNode->setTextureData(m_boneData);
        skinNode->boneCount = m_joints.size();
    }

    return node;
}

QT_END_NAMESPACE
