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
    \brief Define model's joint hierarchy.

*/

QQuick3DJoint::QQuick3DJoint(QQuick3DNode *parent)
    : QQuick3DNode(*(new QQuick3DNodePrivate(QQuick3DNodePrivate::Type::Joint)), parent)
{
}

QQuick3DJoint::~QQuick3DJoint()
{
}

qint32 QQuick3DJoint::index() const
{
    return m_index;
}

QMatrix4x4 QQuick3DJoint::offset() const
{
    return m_offset;
}

QQuick3DSkeleton *QQuick3DJoint::skeletonRoot() const
{
    return m_skeletonRoot;
}

void QQuick3DJoint::setIndex(qint32 index)
{
    if (m_index == index)
        return;

    m_index = index;
    m_indexDirty = true;
    emit indexChanged();
}

void QQuick3DJoint::setOffset(QMatrix4x4 offset)
{
    if (m_offset == offset)
        return;

    m_offset = offset;
    m_offsetDirty = true;
    emit offsetChanged();
}

void QQuick3DJoint::setSkeletonRoot(QQuick3DSkeleton *skeleton)
{
    if (skeleton == m_skeletonRoot)
        return;

    m_skeletonRootDirty = true;
    m_skeletonRoot = skeleton;

    emit skeletonRootChanged();
}


void QQuick3DJoint::markAllDirty()
{
    m_indexDirty = true;
    m_offsetDirty = true;
    m_skeletonRootDirty = true;
    QQuick3DNode::markAllDirty();
}

QSSGRenderGraphObject *QQuick3DJoint::updateSpatialNode(QSSGRenderGraphObject *node)
{
    if (!node) {
        markAllDirty();
        node = new QSSGRenderJoint();
    }

    QQuick3DNode::updateSpatialNode(node);

    auto jointNode = static_cast<QSSGRenderJoint *>(node);
    if (m_skeletonRootDirty) {
        QQuick3DObjectPrivate *skeletonPriv = QQuick3DObjectPrivate::get(m_skeletonRoot);
        if (skeletonPriv && skeletonPriv->spatialNode) {
            jointNode->skeletonRoot = static_cast<QSSGRenderSkeleton *>(skeletonPriv->spatialNode);
            jointNode->skeletonRoot->boneTransformsDirty = true;
        }
        m_skeletonRootDirty = false;
    }
    if (m_indexDirty) {
        jointNode->index = m_index;
        m_indexDirty = false;

        if (jointNode->skeletonRoot) {
            jointNode->skeletonRoot->boneTransformsDirty = true;
            if (jointNode->skeletonRoot->maxIndex < m_index) {
                jointNode->skeletonRoot->maxIndex = m_index;
                jointNode->skeletonRoot->maxIndexDirty = true;
            }
        }
    }
    if (m_offsetDirty) {
        jointNode->offset = m_offset;
        m_offsetDirty = false;
        if (jointNode->skeletonRoot)
            jointNode->skeletonRoot->boneTransformsDirty = true;
    }
    return node;
}

QT_END_NAMESPACE
