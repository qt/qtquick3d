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
    if (!value.dptr)
        return;

    {
        // Built-in types
        switch (value.mt.id()) {
        case QMetaType::QVector2D: {
            delete reinterpret_cast<QVector2D *>(value.dptr);
            return;
        }
        case QMetaType::QVector3D: {
            delete reinterpret_cast<QVector3D *>(value.dptr);
            return;
        }
        case QMetaType::QVector4D: {
            delete reinterpret_cast<QVector4D *>(value.dptr);
            return;
        }
        case QMetaType::QColor: {
            delete reinterpret_cast<QColor *>(value.dptr);
            return;
        }
        case QMetaType::QQuaternion: {
            delete reinterpret_cast<QQuaternion *>(value.dptr);
            return;
        }
        case QMetaType::QMatrix4x4: {
            delete reinterpret_cast<QMatrix4x4 *>(value.dptr);
            return;
        }
        case QMetaType::QUrl:
            qWarning("Don't know how to destruct QUrl");
            return;
        case QMetaType::Float:
            delete reinterpret_cast<float *>(value.dptr);
            return;
        case QMetaType::Double:
            delete reinterpret_cast<double *>(value.dptr);
            return;
        case QMetaType::Int:
            delete reinterpret_cast<int *>(value.dptr);
            return;
        case QMetaType::Char:
            delete reinterpret_cast<char *>(value.dptr);
            return;
        case QMetaType::Bool:
            delete reinterpret_cast<bool *>(value.dptr);
            return;
        case QMetaType::Long:
        case QMetaType::LongLong:
        case QMetaType::ULong:
        case QMetaType::ULongLong:
            qWarning() << "Not implemented yet: destruct simple type" << value.mt;
            return;
        default:
            break;
        }
    }

    if (value.mt.flags() & (QMetaType::IsEnumeration | QMetaType::IsUnsignedEnumeration)) {
        delete reinterpret_cast<QSSGSceneDesc::Flag *>(value.dptr);
        return;
    }

    if (value.mt.id() == qMetaTypeId<QSSGSceneDesc::NodeList *>()) {
        delete reinterpret_cast<QSSGSceneDesc::NodeList *>(value.dptr);
        return;
    }

    if (value.mt.id() == qMetaTypeId<QSSGSceneDesc::ListView>()) {
        auto listView = reinterpret_cast<QSSGSceneDesc::ListView *>(value.dptr);
        delete listView;
        return;
    }

    if (value.mt.id() == qMetaTypeId<QSSGSceneDesc::Node *>()) {
        // Node properties are pointers, and may be used multiple times.
        // We need some sort of refcounting/garbage collection for these.
        return;
    }

    if (value.mt == QMetaType::fromType<QSSGSceneDesc::Mesh>()) {
        qDebug() << "Mesh node property: not deleted.";
    }

    if (value.mt == QMetaType::fromType<QSSGSceneDesc::UrlView>()) {
        delete reinterpret_cast<QSSGSceneDesc::UrlView *>(value.dptr);
        return;
    }

    qWarning() << "Unknown type in destructValue:" << value.mt;
}

void QSSGSceneDesc::destructNode(Node &node)
{
    for (auto *prop : node.properties)
        delete prop;
    // Not necessary to clear the list as long as we only call this from the destructor
}

QT_END_NAMESPACE
