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
    m_contentItem->setObjectName(QLatin1String("parent of ") + item->objectName()); // for debugging
    // No size is set for m_contentItem. This is intentional, otherwise item2d anchoring breaks.
    QQuickItemPrivate::get(m_contentItem)->ensureSubsceneDeliveryAgent();
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
    connect(item, &QQuickItem::enabledChanged, this, &QQuick3DItem2D::updatePicking);
    connect(item, &QQuickItem::visibleChanged, this, &QQuick3DItem2D::updatePicking);
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

QQuickItem *QQuick3DItem2D::contentItem() const
{
    return m_contentItem;
}

void QQuick3DItem2D::sourceItemDestroyed(QObject *item)
{
    disconnect(item, SIGNAL(destroyed(QObject*)), this, SLOT(sourceItemDestroyed(QObject*)));
    auto quickItem = static_cast<QQuickItem*>(item);
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

void QQuick3DItem2D::updatePicking()
{
    m_pickingDirty = true;
    update();
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
    QSGRenderContext *rc = static_cast<QQuickWindowPrivate *>(QObjectPrivate::get(window))->context;

    m_rootNode = sourceItemPrivate->rootNode();
    if (!m_rootNode) {
        return nullptr;
    }

    if (!m_renderer) {
        m_renderer = rc->createRenderer(QSGRendererInterface::RenderMode3D);
        connect(window, SIGNAL(sceneGraphInvalidated()), this, SLOT(invalidated()), Qt::DirectConnection);
        connect(
                m_renderer,
                &QSGAbstractRenderer::sceneGraphChanged,
                this,
                [this]() {
                    if (m_updatingRendererNode)
                        return;
                    // direct connection when rendering is on the main thread, queued with
                    // the threaded render loop
                    QMetaObject::invokeMethod(this, &QQuick3DObject::update);
                },
                Qt::DirectConnection);
        // item2D rendernode has its own render pass descriptor and it should
        // be removed before deleting rhi context.
        // Otherwise, rhi will complain about the unreleased resource.
        connect(
                m_renderer,
                &QObject::destroyed,
                this,
                [this]() {
                    auto itemNode = static_cast<QSSGRenderItem2D *>(QQuick3DObjectPrivate::get(this)->spatialNode);
                    if (itemNode && itemNode->m_rp) {
                        itemNode->m_rp->deleteLater();
                        itemNode->m_rp = nullptr;
                    }
                },
                Qt::DirectConnection);
    }
    // Do not mark this object dirty on m_renderer->nodeChanged(). Otherwise we would end up
    // with constantly updating even when the 2D contents do not change.
    m_updatingRendererNode = true;
    m_renderer->setRootNode(m_rootNode);
    m_rootNode->markDirty(QSGNode::DirtyForceUpdate); // Force matrix, clip and opacity update.
    m_renderer->nodeChanged(m_rootNode, QSGNode::DirtyForceUpdate); // Force render list update.
    m_updatingRendererNode = false;

    if (m_pickingDirty) {
        m_pickingDirty = false;
        bool isPickable = false;
        for (auto item : m_sourceItems) {
            // Enable picking for Item2D if any of its child is visible and enabled.
            if (item->isVisible() && item->isEnabled()) {
                isPickable = true;
                break;
            }
        }
        itemNode->flags.setFlag(QSSGRenderNode::Flag::LocallyPickable, isPickable);
    }

    itemNode->m_renderer = m_renderer;
    if (m_sceneManagerValid) {
        if (itemNode->m_rci != QQuick3DObjectPrivate::get(this)->sceneManager->rci)
            itemNode->m_rci = QQuick3DObjectPrivate::get(this)->sceneManager->rci;
    } else {
        itemNode->m_rci = nullptr;
    }

    return node;
}

void QQuick3DItem2D::markAllDirty()
{
    QQuick3DNode::markAllDirty();
}

void QQuick3DItem2D::itemChange(QQuick3DObject::ItemChange change, const QQuick3DObject::ItemChangeData &value)
{
    QQuick3DNode::itemChange(change, value);
    if (change == QQuick3DObject::ItemSceneChange) {
        if (value.sceneManager)
            m_sceneManagerValid = true;
        else
            m_sceneManagerValid = false;
        markAllDirty();
    }
}

void QQuick3DItem2D::preSync()
{
    const auto &manager = QQuick3DObjectPrivate::get(this)->sceneManager;
    auto *sourcePrivate = QQuickItemPrivate::get(m_contentItem);
    auto *window = manager->window();
    if (m_window != window) {
        if (m_window) {
            disconnect(m_window, SIGNAL(destroyed(QObject*)), this, SLOT(derefWindow(QObject*)));
            sourcePrivate->derefWindow();
        }
        m_window = window;
        sourcePrivate->refWindow(window);
        connect(window, SIGNAL(destroyed(QObject*)), this, SLOT(derefWindow(QObject*)));
        sourcePrivate->refFromEffectItem(true);
    }
}

static void detachWindow(QQuickItem *item, QObject *win)
{
    auto *itemPriv = QQuickItemPrivate::get(item);

    if (win == itemPriv->window) {
        itemPriv->window = nullptr;
        itemPriv->windowRefCount = 0;

        itemPriv->prevDirtyItem = nullptr;
        itemPriv->nextDirtyItem = nullptr;
    }

    for (auto *child: itemPriv->childItems)
        detachWindow(child, win);
}

void QQuick3DItem2D::derefWindow(QObject *win)
{
    detachWindow(m_contentItem, win);
}

QT_END_NAMESPACE
