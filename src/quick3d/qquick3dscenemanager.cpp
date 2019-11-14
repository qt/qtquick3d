/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "qquick3dscenemanager_p.h"
#include "qquick3dobject_p_p.h"
#include "qquick3dviewport_p.h"

#include <QtQuick3DRuntimeRender/private/qssgrenderlayer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercontextcore_p.h>
QT_BEGIN_NAMESPACE

QQuick3DSceneManager::QQuick3DSceneManager(QObject *parent)
    : QObject(parent)
    , dirtySpatialNodeList(nullptr)
    , dirtyResourceList(nullptr)
    , dirtyImageList(nullptr)
{
}

void QQuick3DSceneManager::setWindow(QQuickWindow *window)
{
    if (window == m_window)
        return;

    m_window = window;
}

QQuickWindow *QQuick3DSceneManager::window()
{
    return m_window;
}

void QQuick3DSceneManager::dirtyItem(QQuick3DObject *item)
{
    Q_UNUSED(item)
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

void QQuick3DSceneManager::updateDirtyNodes()
{
    cleanupNodes();

    auto updateNodes = [this](QQuick3DObject *updateList) {
        if (updateList)
            QQuick3DObjectPrivate::get(updateList)->prevDirtyItem = &updateList;

        while (updateList) {
            QQuick3DObject *item = updateList;
            QQuick3DObjectPrivate *itemPriv = QQuick3DObjectPrivate::get(item);
            itemPriv->removeFromDirtyList();

            updateDirtyNode(item);
        }
    };

    updateNodes(dirtyImageList);
    updateNodes(dirtyResourceList);
    updateNodes(dirtySpatialNodeList);
    // Lights have to be last because of scoped lights
    for (const auto light : dirtyLightList)
        updateDirtyNode(light);

    dirtyImageList = nullptr;
    dirtyResourceList = nullptr;
    dirtySpatialNodeList = nullptr;
    dirtyLightList.clear();
}

void QQuick3DSceneManager::updateDirtyNode(QQuick3DObject *object)
{
    // Different processing for resource nodes vs hierarchical nodes
    switch (object->type()) {
    case QQuick3DObject::Light:
    case QQuick3DObject::Node:
    case QQuick3DObject::Camera:
    case QQuick3DObject::Model:
    case QQuick3DObject::Text: {
        // handle hierarchical nodes
        QQuick3DNode *spatialNode = qobject_cast<QQuick3DNode *>(object);
        if (spatialNode)
            updateDirtySpatialNode(spatialNode);
    } break;
    case QQuick3DObject::SceneEnvironment:
    case QQuick3DObject::DefaultMaterial:
    case QQuick3DObject::PrincipledMaterial:
    case QQuick3DObject::Image:
    case QQuick3DObject::CustomMaterial:
    case QQuick3DObject::Lightmaps:
    case QQuick3DObject::Geometry:
        // handle resource nodes
        updateDirtyResource(object);
        break;
    default:
        // we dont need to do anything with the other nodes
        break;
    }
}

void QQuick3DSceneManager::updateDirtyResource(QQuick3DObject *resourceObject)
{
    QQuick3DObjectPrivate *itemPriv = QQuick3DObjectPrivate::get(resourceObject);
    quint32 dirty = itemPriv->dirtyAttributes;
    Q_UNUSED(dirty)
    itemPriv->dirtyAttributes = 0;
    itemPriv->spatialNode = resourceObject->updateSpatialNode(itemPriv->spatialNode);
    if (itemPriv->spatialNode)
        m_nodeMap.insert(itemPriv->spatialNode, resourceObject);

    // resource nodes dont go in the tree, so we dont need to parent them
}

void QQuick3DSceneManager::updateDirtySpatialNode(QQuick3DNode *spatialNode)
{
    QQuick3DObjectPrivate *itemPriv = QQuick3DObjectPrivate::get(spatialNode);
    quint32 dirty = itemPriv->dirtyAttributes;
    Q_UNUSED(dirty)
    itemPriv->dirtyAttributes = 0;
    itemPriv->spatialNode = spatialNode->updateSpatialNode(itemPriv->spatialNode);
    if (itemPriv->spatialNode)
        m_nodeMap.insert(itemPriv->spatialNode, spatialNode);

    QSSGRenderNode *graphNode = static_cast<QSSGRenderNode *>(itemPriv->spatialNode);

    if (graphNode && graphNode->parent == nullptr) {
        QQuick3DNode *nodeParent = qobject_cast<QQuick3DNode *>(spatialNode->parent());
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
            parentGraphNode->addChild(*graphNode);
        } else {
            QQuick3DViewport *viewParent = qobject_cast<QQuick3DViewport *>(spatialNode->parent());
            if (viewParent) {
                auto sceneRoot = QQuick3DObjectPrivate::get(viewParent->scene());
                if (!sceneRoot->spatialNode) {
                    // must have a sceen root spatial node first
                    sceneRoot->spatialNode = viewParent->scene()->updateSpatialNode(sceneRoot->spatialNode);
                    if (sceneRoot->spatialNode)
                        m_nodeMap.insert(sceneRoot->spatialNode, viewParent->scene());
                }
                static_cast<QSSGRenderNode *>(sceneRoot->spatialNode)->addChild(*graphNode);
            }
        }
    }
}

QQuick3DObject *QQuick3DSceneManager::lookUpNode(const QSSGRenderGraphObject *node) const
{
    return m_nodeMap[node];
}

void QQuick3DSceneManager::cleanupNodes()
{
    for (int ii = 0; ii < cleanupNodeList.count(); ++ii) {
        QSSGRenderGraphObject *node = cleanupNodeList.at(ii);
        // Different processing for resource nodes vs hierarchical nodes
        switch (node->type) {
        case QSSGRenderGraphObject::Type::Node:
        case QSSGRenderGraphObject::Type::Light:
        case QSSGRenderGraphObject::Type::Camera:
        case QSSGRenderGraphObject::Type::Model: {
            // handle hierarchical nodes
            QSSGRenderNode *spatialNode = static_cast<QSSGRenderNode *>(node);
            spatialNode->removeFromGraph();
        } break;
        case QSSGRenderGraphObject::Type::Presentation:
        case QSSGRenderGraphObject::Type::Scene:
        case QSSGRenderGraphObject::Type::DefaultMaterial:
        case QSSGRenderGraphObject::Type::PrincipledMaterial:
        case QSSGRenderGraphObject::Type::Image:
        case QSSGRenderGraphObject::Type::CustomMaterial:
        case QSSGRenderGraphObject::Type::Lightmaps:
        case QSSGRenderGraphObject::Type::Geometry:
            // handle resource nodes
            // ### Handle the case where we are referenced by another node
            break;
        default:
            // we dont need to do anything with the other nodes
            break;
        }

        m_nodeMap.remove(node);
        delete node;
    }
    cleanupNodeList.clear();
}

QT_END_NAMESPACE
