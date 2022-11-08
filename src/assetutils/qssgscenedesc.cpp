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
    else // Here goes nothing: kick all the resources out of the tree...
        parent.children.push_back(&node);
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
}

QMetaType QSSGSceneDesc::listViewMetaType()
{
    return QMetaType::fromType<QSSGSceneDesc::ListView *>();
}

void QSSGSceneDesc::destructValue(QVariant &value)
{
    if (!value.isValid())
        return;

    if (!(value.metaType().flags() & QMetaType::TypeFlag::IsPointer))
        return; // Non-pointer types are destructed by ~QVariant

    if (value.metaType() == QMetaType::fromType<QSSGSceneDesc::Mesh *>()) {
        qDebug() << "Mesh node property: not deleted.";
    }

    if (value.metaType().id() == qMetaTypeId<QSSGSceneDesc::Node *>()) {
        // Node properties are pointers, and may be used multiple times.
        // We need some sort of refcounting/garbage collection for these.
        return;
    }

    // All other pointer types are supposed to be deleted. QVariant::data()
    // gives us a pointer to the pointer.

    void *pointer = *static_cast<void**>(value.data());
    value.metaType().destroy(pointer);
}

void QSSGSceneDesc::destructNode(Node &node)
{
    for (auto *prop : node.properties)
        delete prop;
    // Not necessary to clear the list as long as we only call this from the destructor
}

QT_END_NAMESPACE
