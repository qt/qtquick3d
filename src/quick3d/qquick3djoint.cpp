/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "qquick3djoint_p.h"
#include "qquick3dskeleton_p.h"
#include "qquick3dobject_p.h"
#include "qquick3dscenemanager_p.h"
#include "qquick3dnode_p_p.h"

#include <QtQuick3DRuntimeRender/private/qssgrendergraphobject_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendernode_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderjoint_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype Joint
    \inherits Node
    \inqmlmodule QtQuick3D
    \brief Defines a node in a skeletal animation hierarchy.

    A joint is a transformable node inside a \l {Skeleton}, used for \l {Vertex Skinning}
    {skeletal animation}. It is called a "joint" because it can be seen as a joint between the bones
    of a skeleton.

    All the joints must be contained inside a Skeleton, and each joint must have a \l skeletonRoot
    pointing back to that skeleton.

    \qml
    Skeleton {
        id: qmlskeleton
        Joint {
            id: joint0
            index: 0
            skeletonRoot: qmlskeleton
            Joint {
                id: joint1
                index: 1
                skeletonRoot: qmlskeleton
            }
        }
    }
    \endqml

*/

QQuick3DJoint::QQuick3DJoint(QQuick3DNode *parent)
    : QQuick3DNode(*(new QQuick3DNodePrivate(QQuick3DNodePrivate::Type::Joint)), parent)
{
}

QQuick3DJoint::~QQuick3DJoint()
{
    disconnect(m_skeletonConnection);
}

/*!
    \qmlproperty int Joint::index

    Specifies the index of this joint. This index value is used in the \c JointSemantic
    \l {QQuick3DGeometry::addAttribute}{custom geometry attribute}.

    \note Index values must be unique within the same \l {Skeleton}.
    \note Negative values cannot be assigned.

    \sa {QQuick3DGeometry::addAttribute}, {Qt Quick 3D - Simple Skinning Example}
*/

qint32 QQuick3DJoint::index() const
{
    return m_index;
}

/*!
    \qmlproperty Skeleton Joint::skeletonRoot

    Specifies the \l {Skeleton} that contains this joint.

    \note All the \l {Joint}s in the \l {Skeleton} must have the same skeletonRoot.
    If not, the animation will be broken.

    \sa {Skeleton}
*/

QQuick3DSkeleton *QQuick3DJoint::skeletonRoot() const
{
    return m_skeletonRoot;
}

void QQuick3DJoint::setIndex(qint32 index)
{
    if (m_index == index)
        return;
    if (index < 0)
        return;

    m_index = index;
    m_indexDirty = true;
    emit indexChanged();
}

void QQuick3DJoint::setSkeletonRoot(QQuick3DSkeleton *skeleton)
{
    if (skeleton == m_skeletonRoot)
        return;

    disconnect(m_skeletonConnection);
    m_skeletonRootDirty = true;
    m_skeletonRoot = skeleton;
    m_skeletonConnection = connect(this, &QQuick3DJoint::sceneTransformChanged,
                                   m_skeletonRoot, &QQuick3DSkeleton::skeletonNodeDirty);
    emit skeletonRootChanged();
}


void QQuick3DJoint::markAllDirty()
{
    m_indexDirty = true;
    m_skeletonRootDirty = true;
    QQuick3DNode::markAllDirty();
}

QSSGRenderGraphObject *QQuick3DJoint::updateSpatialNode(QSSGRenderGraphObject *node)
{
    if (!m_skeletonRoot)
        return node;

    if (!node) {
        markAllDirty();
        node = new QSSGRenderJoint();
    }

    QQuick3DNode::updateSpatialNode(node);

    auto jointNode = static_cast<QSSGRenderJoint *>(node);

    QQuick3DObjectPrivate *skeletonPriv = QQuick3DObjectPrivate::get(m_skeletonRoot);

    if (m_skeletonRootDirty) {
        if (skeletonPriv && skeletonPriv->spatialNode)
            jointNode->skeletonRoot = static_cast<QSSGRenderSkeleton *>(skeletonPriv->spatialNode);
    }

    if (m_indexDirty) {
        jointNode->index = m_index;
        m_indexDirty = false;

        if (jointNode->skeletonRoot) {
            Q_ASSERT(m_skeletonRoot);
            m_skeletonRoot->skeletonNodeDirty();

            if (jointNode->skeletonRoot->maxIndex < m_index) {
                jointNode->skeletonRoot->maxIndex = m_index;
            }
        }
    }
    return node;
}

QT_END_NAMESPACE
