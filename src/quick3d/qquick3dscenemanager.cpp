// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qquick3dscenemanager_p.h"
#include "qquick3dobject_p.h"
#include "qquick3dviewport_p.h"
#include "qquick3dmodel_p.h"

#include <QtQuick/QQuickWindow>

#include <QtQuick3DRuntimeRender/private/qssgrenderlayer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercontextcore_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendermodel_p.h>
QT_BEGIN_NAMESPACE

QQuick3DSceneManager::QQuick3DSceneManager(QObject *parent)
    : QObject(parent)
    , dirtySpatialNodeList(nullptr)
    , dirtyResourceList(nullptr)
    , dirtyImageList(nullptr)
    , dirtyTextureDataList(nullptr)
{
}

QQuick3DSceneManager::~QQuick3DSceneManager()
{
}

void QQuick3DSceneManager::setWindow(QQuickWindow *window)
{
    if (window == m_window)
        return;

    if (window != m_window) {
        if (m_window)
            disconnect(m_window, &QQuickWindow::afterAnimating, this, &QQuick3DSceneManager::preSync);
        m_window = window;
        connect(m_window, &QQuickWindow::afterAnimating, this, &QQuick3DSceneManager::preSync);
        emit windowChanged();
    }
}

QQuickWindow *QQuick3DSceneManager::window()
{
    return m_window;
}

void QQuick3DSceneManager::dirtyItem(QQuick3DObject *item)
{
    Q_UNUSED(item);
    emit needsUpdate();
}

void QQuick3DSceneManager::cleanup(QSSGRenderGraphObject *item)
{
    Q_ASSERT(!cleanupNodeList.contains(item));
    cleanupNodeList.append(item);
}

void QQuick3DSceneManager::polishItems()
{

}

void QQuick3DSceneManager::forcePolish()
{

}

void QQuick3DSceneManager::sync()
{

}

void QQuick3DSceneManager::updateBoundingBoxes(const QSSGRef<QSSGBufferManager> &mgr)
{
    const QList<QQuick3DObject *> dirtyList = dirtyBoundingBoxList;
    for (auto object : dirtyList) {
        QQuick3DObjectPrivate *itemPriv = QQuick3DObjectPrivate::get(object);
        if (itemPriv->sceneManager == nullptr)
            continue;
        auto model = static_cast<QSSGRenderModel *>(itemPriv->spatialNode);
        if (model) {
            QSSGBounds3 bounds = mgr->getModelBounds(model);
            static_cast<QQuick3DModel *>(object)->setBounds(bounds.minimum, bounds.maximum);
        }
        dirtyBoundingBoxList.removeOne(object);
    }
}

void QQuick3DSceneManager::updateDirtyResourceNodes() {
    updateNodes(&dirtyTextureDataList);
    updateNodes(&dirtyImageList);
    updateNodes(&dirtyResourceList);
}

void QQuick3DSceneManager::updateDirtySpatialNodes() {
    updateNodes(&dirtySpatialNodeList);
    // Lights have to be last because of scoped lights
    for (const auto light : dirtyLightList)
        updateDirtyNode(light);
    dirtyLightList.clear();
}

void QQuick3DSceneManager::updateDirtyNode(QQuick3DObject *object)
{
    // Different processing for resource nodes vs hierarchical nodes
    const auto type = QQuick3DObjectPrivate::get(object)->type;
    if (QSSGRenderGraphObject::isNodeType(type)) {
        // handle hierarchical nodes
        QQuick3DNode *spatialNode = qobject_cast<QQuick3DNode *>(object);
        if (spatialNode)
            updateDirtySpatialNode(spatialNode);
    } else if (QSSGRenderGraphObject::isResource(type)) {
        // handle resource nodes
        updateDirtyResource(object);
    } // we don't need to do anything with the other nodes
}

void QQuick3DSceneManager::updateDirtyResource(QQuick3DObject *resourceObject)
{
    QQuick3DObjectPrivate *itemPriv = QQuick3DObjectPrivate::get(resourceObject);
    quint32 dirty = itemPriv->dirtyAttributes;
    Q_UNUSED(dirty);
    itemPriv->dirtyAttributes = 0;
    itemPriv->spatialNode = resourceObject->updateSpatialNode(itemPriv->spatialNode);
    if (itemPriv->spatialNode) {
        m_nodeMap.insert(itemPriv->spatialNode, resourceObject);
        if (itemPriv->spatialNode->type == QSSGRenderGraphObject::Type::ResourceLoader)
            resourceLoaders.insert(itemPriv->spatialNode);
    }

    // resource nodes dont go in the tree, so we dont need to parent them
}

void QQuick3DSceneManager::updateDirtySpatialNode(QQuick3DNode *spatialNode)
{
    QQuick3DObjectPrivate *itemPriv = QQuick3DObjectPrivate::get(spatialNode);
    quint32 dirty = itemPriv->dirtyAttributes;
    itemPriv->dirtyAttributes = 0;
    itemPriv->spatialNode = spatialNode->updateSpatialNode(itemPriv->spatialNode);
    if (itemPriv->spatialNode)
        m_nodeMap.insert(itemPriv->spatialNode, spatialNode);

    QSSGRenderNode *graphNode = static_cast<QSSGRenderNode *>(itemPriv->spatialNode);

    if (graphNode && graphNode->parent && dirty & QQuick3DObjectPrivate::ParentChanged) {
        QQuick3DNode *nodeParent = qobject_cast<QQuick3DNode *>(spatialNode->parentItem());
        if (nodeParent) {
            QSSGRenderNode *parentGraphNode = static_cast<QSSGRenderNode *>(
                        QQuick3DObjectPrivate::get(nodeParent)->spatialNode);
            if (parentGraphNode) {
                graphNode->parent->removeChild(*graphNode);
                parentGraphNode->addChild(*graphNode);
            }
        }
    }

    if (graphNode && graphNode->parent == nullptr) {
        QQuick3DNode *nodeParent = qobject_cast<QQuick3DNode *>(spatialNode->parentItem());
        if (nodeParent) {
            QSSGRenderNode *parentGraphNode = static_cast<QSSGRenderNode *>(QQuick3DObjectPrivate::get(nodeParent)->spatialNode);
            if (!parentGraphNode) {
                // The parent spatial node hasn't been created yet
                auto parentNode = QQuick3DObjectPrivate::get(nodeParent);
                parentNode->spatialNode = nodeParent->updateSpatialNode(parentNode->spatialNode);
                if (parentNode->spatialNode)
                    m_nodeMap.insert(parentNode->spatialNode, nodeParent);
                parentGraphNode = static_cast<QSSGRenderNode *>(parentNode->spatialNode);
            }
            if (parentGraphNode)
                parentGraphNode->addChild(*graphNode);
        } else {
            QQuick3DViewport *viewParent = qobject_cast<QQuick3DViewport *>(spatialNode->parent());
            if (viewParent) {
                auto sceneRoot = QQuick3DObjectPrivate::get(viewParent->scene());
                if (!sceneRoot->spatialNode) // must have a scene root spatial node first
                    sceneRoot->spatialNode = viewParent->scene()->updateSpatialNode(sceneRoot->spatialNode);
                if (sceneRoot->spatialNode) {
                    m_nodeMap.insert(sceneRoot->spatialNode, viewParent->scene());
                    static_cast<QSSGRenderNode *>(sceneRoot->spatialNode)->addChild(*graphNode);
                }
            }
        }
    }

    if (graphNode) {
        if (auto *model = qobject_cast<QQuick3DModel*>(spatialNode)) {
            if (auto *root = model->instanceRoot() ) {
                auto *rootGraphNode = root ? static_cast<QSSGRenderNode *>(QQuick3DObjectPrivate::get(root)->spatialNode) : nullptr;
                graphNode->instanceRoot = rootGraphNode;
            } else {
                graphNode->instanceRoot = nullptr;
            }
        }
    }
}

QQuick3DObject *QQuick3DSceneManager::lookUpNode(const QSSGRenderGraphObject *node) const
{
    /* Check if the node is already in the Clean Up List or not. If it is on the list this means the node is destroyed and the pointer is invalidated */
    QList<QSSGRenderGraphObject *>::const_iterator it = std::find(cleanupNodeList.begin(), cleanupNodeList.end(), node);
    if (it != cleanupNodeList.end())
        return nullptr;
    else
        return m_nodeMap[node];
}

void QQuick3DSceneManager::cleanupNodes()
{
    for (auto node : cleanupNodeList) {
        // Remove "spatial" nodes from scenegraph
        if (QSSGRenderGraphObject::isNodeType(node->type)) {
            QSSGRenderNode *spatialNode = static_cast<QSSGRenderNode *>(node);
            spatialNode->removeFromGraph();
        }

        // Remove all nodes from the node map because they will no
        // longer be usable from this point from the frontend
        m_nodeMap.remove(node);

        // Some nodes will trigger resource cleanups that need to
        // happen at a specified time (when graphics backend is active)
        // So build another queue for graphics assets marked for removal
        if (QSSGRenderGraphObject::hasGraphicsResources(node->type)) {
            resourceCleanupQueue.append(node);
            if (node->type == QSSGRenderGraphObject::Type::ResourceLoader)
                resourceLoaders.remove(node);
        } else {
            delete node;
        }
    }

    // Nodes are now "cleaned up" so clear the cleanup list
    cleanupNodeList.clear();
}

void QQuick3DSceneManager::updateNodes(QQuick3DObject **listHead)
{
    // Detach the current list head first, and consume all reachable entries.
    // New entries may be added to the new list while traversing, which will be
    // visited on the next updateDirtyNodes() call.
    QQuick3DObject *updateList = *listHead;
    *listHead = nullptr;
    if (updateList)
        QQuick3DObjectPrivate::get(updateList)->prevDirtyItem = &updateList;

    while (updateList) {
        QQuick3DObject *item = updateList;
        QQuick3DObjectPrivate *itemPriv = QQuick3DObjectPrivate::get(item);
        itemPriv->removeFromDirtyList();

        updateDirtyNode(item);
    }
}

void QQuick3DSceneManager::preSync()
{
    QQuick3DObject *next = dirtySpatialNodeList;

    while (next) {
        next->preSync();
        next = QQuick3DObjectPrivate::get(next)->nextDirtyItem;
    }
}

QT_END_NAMESPACE
