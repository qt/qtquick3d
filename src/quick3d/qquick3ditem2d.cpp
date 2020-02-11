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

QT_BEGIN_NAMESPACE

/*
internal
*/
QQuick3DItem2D::QQuick3DItem2D(QQuickItem *item)
    : m_sourceItem(item)
{
    auto *sourcePrivate = QQuickItemPrivate::get(m_sourceItem);

    if (!m_sourceItem->parentItem()) {
        if (auto *manager = sceneManager()) {
            if (auto *window = manager->window())
                m_sourceItem->setParentItem(window->contentItem());
        }
    }

    sourcePrivate->refFromEffectItem(true);

    connect(m_sourceItem, SIGNAL(destroyed(QObject*)), this, SLOT(sourceItemDestroyed(QObject*)));
    update();
}

QQuick3DItem2D::~QQuick3DItem2D()
{
    if (m_layer)
        delete m_layer;
}

QQuick3DObject::Type QQuick3DItem2D::type() const
{
    return QQuick3DObject::Item2D;
}

void QQuick3DItem2D::sourceItemDestroyed(QObject *item)
{
    Q_ASSERT(item == m_sourceItem);
    Q_UNUSED(item)

    disconnect(m_textureProviderConnection);

    delete this;
}

void QQuick3DItem2D::ensureTexture()
{
    if (m_layer)
        return;

    auto *sourcePrivate = QQuickItemPrivate::get(m_sourceItem);

    QSGRenderContext *rc = sourcePrivate->sceneGraphRenderContext();
    auto *layer = rc->sceneGraphContext()->createLayer(rc);
    connect(sourcePrivate->window, SIGNAL(sceneGraphInvalidated()), layer, SLOT(invalidated()), Qt::DirectConnection);
    connect(layer, SIGNAL(updateRequested()), this, SLOT(update()));

    auto *manager = sceneManager();
    manager->qsgDynamicTextures << layer;
    m_sceneManagerForLayer = manager;
    connect(layer, &QObject::destroyed, manager, [this, manager, layer]() {
        manager->qsgDynamicTextures.removeAll(layer);
        m_sceneManagerForLayer = nullptr;
    }, Qt::DirectConnection);

    m_layer = layer;
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
        auto *manager = sceneManager();
        auto *window = manager->window();
        if (!window) {
            qWarning() << "Unable to get window, this will probably not work";
        } else {
            auto *sourcePrivate = QQuickItemPrivate::get(m_sourceItem);
            sourcePrivate->refWindow(window);
        }
    }
    if (QSGTextureProvider *provider = m_sourceItem->textureProvider()) {
        itemNode->m_qsgTexture = provider->texture();
        disconnect(m_textureProviderConnection);
        auto connection = connect(provider, &QSGTextureProvider::textureChanged, this, [provider, itemNode] () {
            itemNode->m_qsgTexture = provider->texture();
            itemNode->m_flags.setFlag(QSSGRenderItem2D::Flag::Dirty);
        }, Qt::DirectConnection);
        m_textureProviderConnection = connection;
        if (m_layer) {
            delete m_layer;
            m_layer = nullptr;
        }
    } else {
        ensureTexture();

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

        itemNode->m_qsgTexture = m_layer;
    }

    return node;
}

void QQuick3DItem2D::markAllDirty()
{
    m_dirtyFlags = DirtyFlags(DirtyFlag::SourceDirty);
    QQuick3DNode::markAllDirty();
}

QT_END_NAMESPACE
