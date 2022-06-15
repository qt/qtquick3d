// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qssgscenedesc_p.h"

QT_BEGIN_NAMESPACE

bool QSSGSceneDesc::PropertyCall::set(QQuick3DObject &, const char *, const void *) { return false; }
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

QMetaType QSSGSceneDesc::flagMetaType()
{
    return QMetaType::fromType<QSSGSceneDesc::Flag>();
}

QT_END_NAMESPACE
