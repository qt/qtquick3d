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
    return QMetaType::fromType<QSSGSceneDesc::ListView>();
}

QMetaType QSSGSceneDesc::flagMetaType()
{
    return QMetaType::fromType<QSSGSceneDesc::Flag>();
}

void QSSGSceneDesc::destructValue(Value &value)
{
    // Node and mesh properties are pointers, and may be used multiple times.
    // We need some sort of refcounting/garbage collection for these.
    if (value.mt.id() == qMetaTypeId<QSSGSceneDesc::Node *>() || value.mt == QMetaType::fromType<QSSGSceneDesc::Mesh>())
        return;

    if (!can_be_stored_in_pointer(value.mt))
        value.mt.destroy(value.dptr);
}

bool QSSGSceneDesc::can_be_stored_in_pointer(const QMetaType &mt)
{
    switch (mt.id()) {
    // Built-in types
    case QMetaType::QVector2D:
        return QSSGSceneDesc::can_be_stored_in_pointer<QVector2D>();
    case QMetaType::QVector3D:
        return QSSGSceneDesc::can_be_stored_in_pointer<QVector3D>();
    case QMetaType::QVector4D:
        return QSSGSceneDesc::can_be_stored_in_pointer<QVector4D>();
    case QMetaType::QColor:
        return QSSGSceneDesc::can_be_stored_in_pointer<QColor>();
    case QMetaType::QQuaternion:
        return QSSGSceneDesc::can_be_stored_in_pointer<QQuaternion>();
    case QMetaType::QMatrix4x4:
        return QSSGSceneDesc::can_be_stored_in_pointer<QMatrix4x4>();
    case QMetaType::QUrl:
        return QSSGSceneDesc::can_be_stored_in_pointer<QUrl>();
    case QMetaType::Float:
        return QSSGSceneDesc::can_be_stored_in_pointer<float>();
    case QMetaType::Double:
        return QSSGSceneDesc::can_be_stored_in_pointer<double>();
    case QMetaType::Int:
        return QSSGSceneDesc::can_be_stored_in_pointer<int>();
    case QMetaType::Char:
        return QSSGSceneDesc::can_be_stored_in_pointer<char>();
    case QMetaType::Bool:
        return QSSGSceneDesc::can_be_stored_in_pointer<bool>();
    case QMetaType::Long:
    case QMetaType::LongLong:
    case QMetaType::ULong:
    case QMetaType::ULongLong:
        qWarning() << "Not implemented yet: simple type" << mt;
    default:
        break;
    }
    if (mt.flags() & (QMetaType::IsEnumeration | QMetaType::IsUnsignedEnumeration))
        return true;

    return false;
}

void QSSGSceneDesc::destructNode(Node &node)
{
    for (auto *prop : node.properties)
        delete prop;
    // Not necessary to clear the list as long as we only call this from the destructor
}

QT_END_NAMESPACE
