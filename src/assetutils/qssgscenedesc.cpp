/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
******************************************************************************/

#include "qssgscenedesc_p.h"

QT_BEGIN_NAMESPACE

bool QSSGSceneDesc::PropertyCall::set(QQuick3DObject &, const void *) { return false; }
bool QSSGSceneDesc::PropertyCall::get(const QQuick3DObject &, const void *[]) const { return false; }

static inline quint16 getNextNodeId(QSSGSceneDesc::Scene &scene)
{
    /* root node uses the default value 0 */
    return ++scene.nodeId;
}

void QSSGSceneDesc::addNode(QSSGSceneDesc::Node &parent, QSSGSceneDesc::Node &node)
{
    Q_ASSERT(parent.scene);
    node.scene = parent.scene;
    node.id = getNextNodeId(*parent.scene);

    if (QSSGRenderGraphObject::isResource(node.runtimeType) || node.nodeType == Node::Type::Mesh || node.nodeType == Node::Type::Skeleton)
        node.scene->resources.push_back(&node);

    parent.children.push_back(node);
}

void QSSGSceneDesc::addNode(QSSGSceneDesc::Scene &scene, QSSGSceneDesc::Node &node)
{
    if (scene.root) {
        addNode(*scene.root, node);
    } else {
        Q_ASSERT(node.id == 0);
        node.scene = &scene;
        scene.root = &node;
    }
}

void QSSGSceneDesc::Scene::reset()
{
    id.clear();
    nodeId = 0;
    root = nullptr;
    resources.clear();
    meshStorage.clear();
    allocator.reset();
}

QMetaType QSSGSceneDesc::listViewMetaType()
{
    return QMetaType::fromType<QSSGSceneDesc::ListView>();
}

QT_END_NAMESPACE
