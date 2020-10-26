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

#include <QtQuick3D/qquick3dobject.h>
#include <QtQuick3D/private/qquick3ditem2d_p.h>

#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qsgrenderer_p.h>
#include <QtQuick/private/qquickwindow_p.h>

#include <QtQuick3DRuntimeRender/private/qssgrenderitem2d_p.h>
#include "qquick3dnode_p_p.h"

QT_BEGIN_NAMESPACE

/*
internal
*/

QQuick3DItem2D::QQuick3DItem2D(QQuickItem *item, QQuick3DNode *parent)
    : QQuick3DNode(*(new QQuick3DNodePrivate(QQuick3DNodePrivate::Type::Item2D)), parent)
{
    m_contentItem = new QQuickItem();
    QQmlEngine::setObjectOwnership(m_contentItem, QQmlEngine::CppOwnership);

    connect(m_contentItem, &QQuickItem::childrenChanged, this, &QQuick3DObject::update);
    addChildItem(item);
}

QQuick3DItem2D::~QQuick3DItem2D()
{
    delete m_contentItem;

    // This is sketchy. Similarly to the problems QQuick3DTexture has with its
    // QSGTexture, the same problems arise here with the QSGRenderer. The
    // associated scenegraph resource must be destroyed on the render thread,
    // if there is one. If the scenegraph gets invalidated, that's easy due to
    // signals/slots, but there's no such signal if an object with Item2Ds in
    // it gets dynamically destroyed.
    // Here on the gui thread in this dtor there's no way to properly manage
    // the QSG resource's releasing anymore. Rather, as QSGRenderer is a
    // QObject, do a deleteLater(), which typically works, but is not a 100%
    // guarantee that the object will get destroyed on the render thread
    // eventually, since in theory it could happen that the render thread is
    // not even running at this point anymore (if the window is closing / the
    // app is shutting down) - although in practice that won't be an issue
    // since that case is taken care of the sceneGraphInvalidated signal.
    // So while unlikely, a leak may still occur under certain circumstances.
    if (m_renderer)
        m_renderer->deleteLater();
}

void QQuick3DItem2D::addChildItem(QQuickItem *item)
{
    item->setParent(m_contentItem);
    item->setParentItem(m_contentItem);
    connect(item, SIGNAL(destroyed(QObject*)), this, SLOT(sourceItemDestroyed(QObject*)));
    m_sourceItems.append(item);
    update();
}
void QQuick3DItem2D::removeChildItem(QQuickItem *item)
{
    m_sourceItems.removeOne(item);
    if (m_sourceItems.isEmpty())
        emit allChildrenRemoved();
    else
        update();
}

void QQuick3DItem2D::sourceItemDestroyed(QObject *item)
{
    disconnect(item, SIGNAL(destroyed(QObject*)), this, SLOT(sourceItemDestroyed(QObject*)));
    auto quickItem = qmlobject_cast<QQuickItem*>(item);
    removeChildItem(quickItem);
}

void QQuick3DItem2D::invalidated()
{
    // clean up the renderer
    if (m_renderer) {
        delete m_renderer;
        m_renderer = nullptr;
    }
}

QSSGRenderGraphObject *QQuick3DItem2D::updateSpatialNode(QSSGRenderGraphObject *node)
{
    auto *sourceItemPrivate = QQuickItemPrivate::get(m_contentItem);
    QQuickWindow *window = m_contentItem->window();
    if (!window) {
        const auto &manager = QQuick3DObjectPrivate::get(this)->sceneManager;
        window = manager->window();
    }


    if (!node) {
        markAllDirty();
        node = new QSSGRenderItem2D();
    }

    QQuick3DNode::updateSpatialNode(node);

    auto itemNode = static_cast<QSSGRenderItem2D *>(node);
    QSGRenderContext *rc = sourceItemPrivate->sceneGraphRenderContext();


    m_rootNode = sourceItemPrivate->rootNode();
    if (!m_rootNode) {
        return nullptr;
    }

    if (!m_renderer) {
        m_renderer = rc->createRenderer(QSGRendererInterface::RenderMode3D);
        connect(window, SIGNAL(sceneGraphInvalidated()), this, SLOT(invalidated()), Qt::DirectConnection);
    }
    m_renderer->setRootNode(m_rootNode);
    m_rootNode->markDirty(QSGNode::DirtyForceUpdate); // Force matrix, clip and opacity update.
    m_renderer->nodeChanged(m_rootNode, QSGNode::DirtyForceUpdate); // Force render list update.

    itemNode->m_renderer = m_renderer;

    return node;
}

void QQuick3DItem2D::markAllDirty()
{
    QQuick3DNode::markAllDirty();
}

void QQuick3DItem2D::preSync()
{
    const auto &manager = QQuick3DObjectPrivate::get(this)->sceneManager;
    auto *sourcePrivate = QQuickItemPrivate::get(m_contentItem);
    auto *window = manager->window();
    if (m_window != window) {
        if (m_window) {
            sourcePrivate->derefWindow();
        }
        m_window = window;
        sourcePrivate->refWindow(window);
        sourcePrivate->refFromEffectItem(true);
    }
}

QT_END_NAMESPACE
