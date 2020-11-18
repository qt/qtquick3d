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

#include "qquick3dskeleton_p.h"
#include "qquick3dobject_p.h"
#include "qquick3dnode_p_p.h"

#include <QtQuick3DRuntimeRender/private/qssgrenderskeleton_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype Skeleton
    \inherits Node
    \inqmlmodule QtQuick3D
    \brief Defines a skeletal animation hierarchy.

    A skeleton defines how a model can be animated using \l {Vertex Skinning}
    {skeletal animation}. It contains a hierarchy of \l {Joint} nodes. Each joint can be
    transformed for a skinning animation.

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

    \sa {Joint::skeletonRoot}, {Model::skeleton}, {Qt Quick 3D - Simple Skinning Example#skeleton-and-joint-hierarchy}
*/

QQuick3DSkeleton::QQuick3DSkeleton(QQuick3DNode *parent)
    : QQuick3DNode(*(new QQuick3DNodePrivate(QQuick3DNodePrivate::Type::Skeleton)), parent)
{
}

QQuick3DSkeleton::~QQuick3DSkeleton()
{
}

QSSGRenderGraphObject *QQuick3DSkeleton::updateSpatialNode(QSSGRenderGraphObject *node)
{
    if (!node) {
        node = new QSSGRenderSkeleton();
        emit skeletonNodeDirty();
    }
    QQuick3DNode::updateSpatialNode(node);

    auto skeletonNode = static_cast<QSSGRenderSkeleton *>(node);

    return skeletonNode;
}

QT_END_NAMESPACE
