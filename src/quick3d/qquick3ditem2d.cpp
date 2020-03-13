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

#include <QtQuick3DRuntimeRender/private/qssgrenderitem2d_p.h>
#include "qquick3dnode_p_p.h"

QT_BEGIN_NAMESPACE

/*
internal
*/
QQuick3DItem2D::QQuick3DItem2D(QQuickItem *item, QQuick3DNode *parent)
    : QQuick3DNode(*(new QQuick3DNodePrivate(QQuick3DNodePrivate::Type::Item2D)), parent)
    , m_sourceItem(item)
{
    auto *sourcePrivate = QQuickItemPrivate::get(m_sourceItem);

    if (!m_sourceItem->parentItem()) {
        if (const auto &manager = QQuick3DObjectPrivate::get(this)->sceneManager) {
            if (auto *window = manager->window())
                m_sourceItem->setParentItem(window->contentItem());
        }
    }

    sourcePrivate->refFromEffectItem(true);

    connect(m_sourceItem, SIGNAL(destroyed(QObject*)), this, SLOT(sourceItemDestroyed(QObject*)));

    // Connect all QQuickItem changes which require updating.
    // We can't connect to e.g. QQuickWindow::afterRendering directly as that is
    // emitted always when something is scene changes, causing an update loop.
    connect(m_sourceItem, &QQuickItem::childrenChanged, this, &QQuick3DObject::update);
    connect(m_sourceItem, &QQuickItem::opacityChanged, this, &QQuick3DObject::update);
    connect(m_sourceItem, &QQuickItem::visibleChanged, this, &QQuick3DObject::update);
    connect(m_sourceItem, &QQuickItem::visibleChildrenChanged, this, &QQuick3DObject::update);
    connect(m_sourceItem, &QQuickItem::scaleChanged, this, &QQuick3DObject::update);
    connect(m_sourceItem, &QQuickItem::widthChanged, this, &QQuick3DObject::update);
    connect(m_sourceItem, &QQuickItem::heightChanged, this, &QQuick3DObject::update);
    connect(m_sourceItem, &QQuickItem::zChanged, this, &QQuick3DObject::update);
}

QQuick3DItem2D::~QQuick3DItem2D()
{
    if (m_layer && m_sceneManagerForLayer) {
        m_sceneManagerForLayer->qsgDynamicTextures.removeAll(m_layer);
        m_layer->deleteLater();
    }
}

void QQuick3DItem2D::sourceItemDestroyed(QObject *item)
{
    Q_ASSERT(item == m_sourceItem);
    Q_UNUSED(item)

    delete this;
}

// Create layer and update once valid layer texture exists
void QQuick3DItem2D::createLayerTexture()
{
    auto *sourcePrivate = QQuickItemPrivate::get(m_sourceItem);
    QSGRenderContext *rc = sourcePrivate->sceneGraphRenderContext();
    auto *layer = rc->sceneGraphContext()->createLayer(rc);
    const auto &manager = QQuick3DObjectPrivate::get(this)->sceneManager;
    manager->qsgDynamicTextures << layer;
    m_sceneManagerForLayer = manager.get();

    connect(sourcePrivate->window, SIGNAL(sceneGraphInvalidated()), layer, SLOT(invalidated()), Qt::DirectConnection);

    // When layer has been updated, take it into use.
    connect(layer, &QSGLayer::scheduledUpdateCompleted, this, [this, layer]() {
        m_layer = layer;
        update();
    });

    // At this point we definitely should have parent node.
    // Update when parent opacity changes as combined opacity is used.
    auto *parentNode = static_cast<QQuick3DNode *>(parent());
    connect(parentNode, &QQuick3DNode::localOpacityChanged, this, [this]() {
        if (m_layer) {
            m_layer->markDirtyTexture();
            m_layer->scheduleUpdate();
        }
        update();
    });

    layer->markDirtyTexture();
    layer->scheduleUpdate();
    update();
}

QSSGRenderGraphObject *QQuick3DItem2D::updateSpatialNode(QSSGRenderGraphObject *node)
{
    if (!node) {
        markAllDirty();
        node = new QSSGRenderItem2D();
    }

    QQuick3DNode::updateSpatialNode(node);

    auto itemNode = static_cast<QSSGRenderItem2D *>(node);

    Q_ASSERT(m_sourceItem);

    QQuickWindow *window = m_sourceItem->window();
    if (!window) {
        const auto &manager = QQuick3DObjectPrivate::get(this)->sceneManager;
        window = manager->window();
        if (!window) {
            qWarning() << "Unable to get window, this will probably not work";
        } else {
            auto *sourcePrivate = QQuickItemPrivate::get(m_sourceItem);
            sourcePrivate->refWindow(window);
        }
    }

    if (!m_initialized) {
        m_initialized = true;
        // When scene has been rendered for the first time, create layer texture.
        connect(window, &QQuickWindow::afterRendering, this, [this, window]() {
            disconnect(window, &QQuickWindow::afterRendering, this, nullptr);
            createLayerTexture();
        });
    }

    if (!m_layer)
        return node;

    m_layer->setItem(QQuickItemPrivate::get(m_sourceItem)->itemNode());
    QRectF sourceRect = QRectF(0, 0, m_sourceItem->width(), m_sourceItem->height());

    // check if the size is null
    if (sourceRect.width() == 0.0)
        sourceRect.setWidth(256.0);
    if (sourceRect.height() == 0.0)
        sourceRect.setHeight(256.0);
    m_layer->setRect(sourceRect);

    QSize textureSize(qCeil(qAbs(sourceRect.width())), qCeil(qAbs(sourceRect.height())));

    auto *sourcePrivate = QQuickItemPrivate::get(m_sourceItem);
    const QSize minTextureSize = sourcePrivate->sceneGraphContext()->minimumFBOSize();
    // Keep power-of-two by doubling the size.
    while (textureSize.width() < minTextureSize.width())
        textureSize.rwidth() *= 2;
    while (textureSize.height() < minTextureSize.height())
        textureSize.rheight() *= 2;

    m_layer->setSize(textureSize);

    itemNode->zOrder = float(m_sourceItem->z());
    if (m_sourceItem->isVisible())
        itemNode->combinedOpacity = itemNode->globalOpacity * float(m_sourceItem->opacity());
    else
        itemNode->combinedOpacity = 0.0f;
    itemNode->qsgTexture = m_layer;

    return node;
}

void QQuick3DItem2D::markAllDirty()
{
    QQuick3DNode::markAllDirty();
}

QT_END_NAMESPACE
