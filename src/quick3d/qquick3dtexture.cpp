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

#include "qquick3dtexture_p.h"
#include <QtQuick3DRuntimeRender/private/qssgrenderimage_p.h>
#include <QtQml/QQmlFile>
#include <QtQuick/QQuickItem>
#include <QtQuick/private/qquickitem_p.h>
#include <QtCore/qmath.h>

#include "qquick3dobject_p.h"
#include "qquick3dscenemanager_p.h"
#include "qquick3dutils_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype Texture
    \inherits Object3D
    \inqmlmodule QtQuick3D
    \brief Defines a texture for use in 3D scenes.

    Texture defines an image and how it is mapped to meshes in a 3d scene.

    Texture components can use image data either from a file using the
    \l source property, or a Qt Quick item using the sourceItem property.
*/

QQuick3DTexture::QQuick3DTexture(QQuick3DObject *parent)
    : QQuick3DObject(*(new QQuick3DObjectPrivate(QQuick3DObjectPrivate::Type::Image)), parent) {}

QQuick3DTexture::~QQuick3DTexture()
{
    if (m_layer && m_sceneManagerForLayer) {
        m_sceneManagerForLayer->qsgDynamicTextures.removeAll(m_layer);
        m_layer->deleteLater();
    }

    if (m_sourceItem) {
        QQuickItemPrivate *sourcePrivate = QQuickItemPrivate::get(m_sourceItem);
        sourcePrivate->removeItemChangeListener(this, QQuickItemPrivate::Geometry);
    }
}

/*!
    \qmlproperty url QtQuick3D::Texture::source

    This property holds the location of an image file containing the data used
    by the texture.

    \sa sourceItem
*/
QUrl QQuick3DTexture::source() const
{
    return m_source;
}

/*!
    \qmlproperty Item QtQuick3D::Texture::sourceItem

    This property defines a Item to be used as the source of the texture. Using
    this property allows any 2D Qt Quick content to be used as a texture source
    by renderind that item as an offscreen layer.

    If this property is used, then the value of \l source will be ignored.

    \note Currently there is no way to forward input events to the Item used as
    a texture source.

    \sa source
*/
QQuickItem *QQuick3DTexture::sourceItem() const
{
    return m_sourceItem;
}

/*!
    \qmlproperty float QtQuick3D::Texture::scaleU

    This property defines how to scale the U texture coordinate when mapping to
    a mesh's UV coordinates.

    Scaling the U value when using horizontal tiling will define how many times the
    texture is repeated from left to right.

    \sa tilingModeHorizontal
 */
float QQuick3DTexture::scaleU() const
{
    return m_scaleU;
}

/*!
    \qmlproperty float QtQuick3D::Texture::scaleV

    This property defines how to scale the V texture coordinate when mapping to
    a mesh's UV coordinates.

    Scaling the V value when using vertical tiling will define how many times a
    texture is repeated from bottom to top.

    \sa tilingModeVertical
*/
float QQuick3DTexture::scaleV() const
{
    return m_scaleV;
}

/*!
    \qmlproperty enumeration QtQuick3D::Texture::mappingMode

    This property defines which method of mapping to use when sampling this
    texture.

    \value Texture.UV The default for diffuse and opacity maps,
        this causes the image to be stuck to the mesh. The same portion of the
        image will always appear on the same vertex (unless the UV properties are
        animated).
    \value Texture.Environment The default for specular reflection,
        this causes the image to be ‘projected’ onto the material as though it is
        being reflected. Using Environmental Mapping for diffuse maps provides a
        mirror effect.
    \value Texture.LightProbe The default for HDRI sphere maps used by light probes.
*/
QQuick3DTexture::MappingMode QQuick3DTexture::mappingMode() const
{
    return m_mappingMode;
}

/*!
    \qmlproperty enumeration QtQuick3D::Texture::tilingModeHorizontal

    Controls how the texture is mapped when the U scaling value is greater than 1.

    \value Texture.ClampToEdge Texture is not tiled, but the value on the edge is used instead.
    \value Texture.MirroredRepeat Texture is repeated and mirrored over the X axis.
    \value Texture.Repeat Texture is repeated over the X axis.

    \sa scaleU
*/
QQuick3DTexture::TilingMode QQuick3DTexture::horizontalTiling() const
{
    return m_tilingModeHorizontal;
}

/*!
    \qmlproperty enumeration QtQuick3D::Texture::tilingModeVertical

    This property controls how the texture is mapped when the V scaling value
    is greater than 1.

    \value Texture.ClampToEdge Texture is not tiled, but the value on the edge is used instead.
    \value Texture.MirroredRepeat Texture is repeated and mirrored over the Y axis.
    \value Texture.Repeat Texture is repeated over the Y axis.

    \sa scaleV
*/
QQuick3DTexture::TilingMode QQuick3DTexture::verticalTiling() const
{
    return m_tilingModeVertical;
}

/*!
    \qmlproperty float QtQuick3D::Texture::rotationUV

    This property rotates the texture around the pivot point. This is defined
    using euler angles and for a positve value rotation is clockwise.

    \sa pivotU, pivotV
*/
float QQuick3DTexture::rotationUV() const
{
    return m_rotationUV;
}

/*!
    \qmlproperty float QtQuick3D::Texture::positionU

    This property offsets the U coordinate mapping from left to right.
*/
float QQuick3DTexture::positionU() const
{
    return m_positionU;
}

/*!
    \qmlproperty float QtQuick3D::Texture::positionV

    This property offsets the V coordinate mapping from bottom to top.
*/
float QQuick3DTexture::positionV() const
{
    return m_positionV;
}

/*!
    \qmlproperty float QtQuick3D::Texture::pivotU

    This property sets the pivot U position.

    \sa rotationUV
*/
float QQuick3DTexture::pivotU() const
{
    return m_pivotU;
}

/*!
    \qmlproperty float QtQuick3D::Texture::pivotV

    This property sets the pivot V position.

    \sa rotationUV
*/
float QQuick3DTexture::pivotV() const
{
    return m_pivotV;
}

/*!
    \qmlproperty bool QtQuick3D::Texture::flipV

    This property sets the use of the vertically flipped coordinates.
*/
bool QQuick3DTexture::flipV() const
{
    return m_flipV;
}

/*!
    \qmlproperty enumeration QtQuick3D::Texture::format

    This property controls the color format of the texture assigned in \l source property.

    By default, it is automatically determined.
    However, it can be manually set if the automatic format is not what is wanted.

    \value Texture.Automatic The color format will be automatically determined (default).
    \value Texture.R8 The color format is considered as 8-bit integer in R channel.
    \value Texture.R16 The color format is considered as 16-bit integer in R channel.
    \value Texture.R16F The color format is considered as 16-bit float in R channel.
    \value Texture.R32I The color format is considered as 32-bit integer in R channel.
    \value Texture.R32UI The color format is considered as 32-bit unsigned integer in R channel.
    \value Texture.R32F The color format is considered as 32-bit float R channel.
    \value Texture.RG8 The color format is considered as 8-bit integer in R and G channels.
    \value Texture.RGBA8 The color format is considered as 8-bit integer in R, G, B and alpha channels.
    \value Texture.RGB8 The color format is considered as 8-bit integer in R, G and B channels.
    \value Texture.SRGB8 The color format is considered as 8-bit integer in R, G and B channels in standard RGB color space.
    \value Texture.SRGB8A8 The color format is considered as 8-bit integer in R, G, B and alpha channels in standard RGB color space.
    \value Texture.RGB565 The color format is considered as 5-bit integer in R and B channels and 6-bit integer in G channel.
    \value Texture.RGBA5551 The color format is considered as 5-bit integer in R, G, B channels and boolean alpha channel.
    \value Texture.Alpha8 The color format is considered as 8-bit alpha map.
    \value Texture.Luminance8 The color format is considered as 8-bit luminance map.
    \value Texture.Luminance16 The color format is considered as 16-bit luminance map.
    \value Texture.LuminanceAlpha8 The color format is considered as 8-bit luminance and alpha map.
    \value Texture.RGBA16F The color format is considered as 16-bit float in R,G,B and alpha channels.
    \value Texture.RG16F The color format is considered as 16-bit float in R and G channels.
    \value Texture.RG32F The color format is considered as 32-bit float in R and G channels.
    \value Texture.RGB32F The color format is considered as 32-bit float in R, G and B channels.
    \value Texture.RGBA32F The color format is considered as 32-bit float in R, G, B and alpha channels.
    \value Texture.R11G11B10 The color format is considered as 11-bit integer in R and G channels and 10-bit integer in B channel.
    \value Texture.RGB9E5 The color format is considered as 9-bit mantissa in R, G and B channels and 5-bit shared exponent.
    \value Texture.RGBA_DXT1 The color format is considered as DXT1 compressed format with R, G, B and alpha channels.
    \value Texture.RGB_DXT1 The color format is considered as DXT1 compressed format with R, G and B channels.
    \value Texture.RGBA_DXT3 The color format is considered as DXT3 compressed format with R, G, B and alpha channels.
    \value Texture.RGBA_DXT5 The color format is considered as DXT5 compressed format with R, G, B and alpha channels.
    \value Texture.Depth16 The color format is considered as 16-bit depth map.
    \value Texture.Depth24 The color format is considered as 24-bit depth map.
    \value Texture.Depth32 The color format is considered as 32-bit depth map.
    \value Texture.Depth24Stencil8 The color format is considered as 24-bit depth and 8-bit stencil map.
*/
QQuick3DTexture::Format QQuick3DTexture::format() const
{
    return m_format;
}

void QQuick3DTexture::setSource(const QUrl &source)
{
    if (m_source == source)
        return;

    m_source = source;
    m_dirtyFlags.setFlag(DirtyFlag::SourceDirty);
    emit sourceChanged();
    update();
}

void QQuick3DTexture::trySetSourceParent()
{
    if (m_sourceItem->parentItem() && m_sourceItemRefed)
        return;

    auto *sourcePrivate = QQuickItemPrivate::get(m_sourceItem);

    if (!m_sourceItem->parentItem()) {
        if (const auto &manager = QQuick3DObjectPrivate::get(this)->sceneManager) {
            if (auto *window = manager->window()) {
                if (m_sourceItemRefed) {
                    // Item was already refed but probably with hide set to false...
                    // so we need to deref before we ref again below.
                    const bool hide = m_sourceItemReparented;
                    sourcePrivate->derefFromEffectItem(hide);
                    m_sourceItemRefed = false;
                }

                m_sourceItem->setParentItem(window->contentItem());
                m_sourceItemReparented = true;
                update();
            }
        }
    }

    if (!m_sourceItemRefed) {
        const bool hide = m_sourceItemReparented;
        sourcePrivate->refFromEffectItem(hide);
    }
}

void QQuick3DTexture::setSourceItem(QQuickItem *sourceItem)
{
    if (m_sourceItem == sourceItem)
        return;

    //TODO: Also clear the source property?

    if (m_sourceItem) {
        QQuickItemPrivate *sourcePrivate = QQuickItemPrivate::get(m_sourceItem);

        const bool hide = m_sourceItemReparented;
        sourcePrivate->derefFromEffectItem(hide);
        m_sourceItemRefed = false;

        sourcePrivate->removeItemChangeListener(this, QQuickItemPrivate::Geometry);
        disconnect(m_sourceItem, SIGNAL(destroyed(QObject*)), this, SLOT(sourceItemDestroyed(QObject*)));
        if (m_sourceItem->window())
            sourcePrivate->derefWindow();
        if (m_sourceItemReparented) {
            m_sourceItem->setParentItem(nullptr);
            m_sourceItemReparented = false;
        }
    }

    m_sourceItem = sourceItem;

    if (sourceItem) {
        trySetSourceParent();
        QQuickItemPrivate *sourcePrivate = QQuickItemPrivate::get(m_sourceItem);
        sourcePrivate->addItemChangeListener(this, QQuickItemPrivate::Geometry);
        connect(m_sourceItem, SIGNAL(destroyed(QObject*)), this, SLOT(sourceItemDestroyed(QObject*)));
    }

    emit sourceItemChanged();
    update();
}

void QQuick3DTexture::setScaleU(float scaleU)
{
    if (qFuzzyCompare(m_scaleU, scaleU))
        return;

    m_scaleU = scaleU;
    m_dirtyFlags.setFlag(DirtyFlag::TransformDirty);
    emit scaleUChanged();
    update();
}

void QQuick3DTexture::setScaleV(float scaleV)
{
    if (qFuzzyCompare(m_scaleV, scaleV))
        return;

    m_scaleV = scaleV;
    m_dirtyFlags.setFlag(DirtyFlag::TransformDirty);
    emit scaleVChanged();
    update();
}

void QQuick3DTexture::setMappingMode(QQuick3DTexture::MappingMode mappingMode)
{
    if (m_mappingMode == mappingMode)
        return;

    m_mappingMode = mappingMode;
    emit mappingModeChanged();
    update();
}

void QQuick3DTexture::setHorizontalTiling(QQuick3DTexture::TilingMode tilingModeHorizontal)
{
    if (m_tilingModeHorizontal == tilingModeHorizontal)
        return;

    m_tilingModeHorizontal = tilingModeHorizontal;
    emit horizontalTilingChanged();
    update();
}

void QQuick3DTexture::setVerticalTiling(QQuick3DTexture::TilingMode tilingModeVertical)
{
    if (m_tilingModeVertical == tilingModeVertical)
        return;

    m_tilingModeVertical = tilingModeVertical;
    emit verticalTilingChanged();
    update();
}

void QQuick3DTexture::setRotationUV(float rotationUV)
{
    if (qFuzzyCompare(m_rotationUV, rotationUV))
        return;

    m_rotationUV = rotationUV;
    m_dirtyFlags.setFlag(DirtyFlag::TransformDirty);
    emit rotationUVChanged();
    update();
}

void QQuick3DTexture::setPositionU(float positionU)
{
    if (qFuzzyCompare(m_positionU, positionU))
        return;

    m_positionU = positionU;
    m_dirtyFlags.setFlag(DirtyFlag::TransformDirty);
    emit positionUChanged();
    update();
}

void QQuick3DTexture::setPositionV(float positionV)
{
    if (qFuzzyCompare(m_positionV, positionV))
        return;

    m_positionV = positionV;
    m_dirtyFlags.setFlag(DirtyFlag::TransformDirty);
    emit positionVChanged();
    update();
}

void QQuick3DTexture::setPivotU(float pivotU)
{
    if (qFuzzyCompare(m_pivotU, pivotU))
        return;

    m_pivotU = pivotU;
    m_dirtyFlags.setFlag(DirtyFlag::TransformDirty);
    emit pivotUChanged();
    update();
}

void QQuick3DTexture::setPivotV(float pivotV)
{
    if (qFuzzyCompare(m_pivotV, pivotV))
        return;

    m_pivotV = pivotV;
    m_dirtyFlags.setFlag(DirtyFlag::TransformDirty);
    emit pivotVChanged();
    update();
}

void QQuick3DTexture::setFlipV(bool flipV)
{
    if (m_flipV == flipV)
        return;

    m_flipV = flipV;
    m_dirtyFlags.setFlag(DirtyFlag::TransformDirty);
    emit flipVChanged();
    update();
}

void QQuick3DTexture::setFormat(QQuick3DTexture::Format format)
{
    if (m_format == format)
        return;

    m_format = format;
    emit formatChanged();
    update();
}

QSSGRenderGraphObject *QQuick3DTexture::updateSpatialNode(QSSGRenderGraphObject *node)
{
    if (!node) {
        markAllDirty();
        node = new QSSGRenderImage();
    }

    auto imageNode = static_cast<QSSGRenderImage *>(node);

    // When texture is QQuickItem, flip it automatically
    bool flipTexture = m_flipV;
    if (m_sourceItem)
        flipTexture = !flipTexture;

    if (m_dirtyFlags.testFlag(DirtyFlag::TransformDirty)) {
        m_dirtyFlags.setFlag(DirtyFlag::TransformDirty, false);
        imageNode->m_flipV = flipTexture;
        imageNode->m_scale = QVector2D(m_scaleU, m_scaleV);
        imageNode->m_pivot = QVector2D(m_pivotU, m_pivotV);
        imageNode->m_rotation = m_rotationUV;
        imageNode->m_position = QVector2D(m_positionU, m_positionV);

        imageNode->m_flags.setFlag(QSSGRenderImage::Flag::TransformDirty);
    }

    bool nodeChanged = false;
    if (m_dirtyFlags.testFlag(DirtyFlag::SourceDirty)) {
        m_dirtyFlags.setFlag(DirtyFlag::SourceDirty, false);
        imageNode->m_imagePath = QQmlFile::urlToLocalFileOrQrc(m_source);
        nodeChanged = true;
    }

    nodeChanged |= qUpdateIfNeeded(imageNode->m_mappingMode,
                                  QSSGRenderImage::MappingModes(m_mappingMode));
    nodeChanged |= qUpdateIfNeeded(imageNode->m_horizontalTilingMode,
                                  QSSGRenderTextureCoordOp(m_tilingModeHorizontal));
    nodeChanged |= qUpdateIfNeeded(imageNode->m_verticalTilingMode,
                                  QSSGRenderTextureCoordOp(m_tilingModeVertical));
    QSSGRenderTextureFormat format{QSSGRenderTextureFormat::Format(m_format)};
    nodeChanged |= qUpdateIfNeeded(imageNode->m_format, format);

    if (m_sourceItem) { // TODO: handle width == 0 || height == 0
        QQuickWindow *window = m_sourceItem->window();
        if (!window) {
            // Do a hack to set the window
            const auto &manager = QQuick3DObjectPrivate::get(this)->sceneManager;
            window = manager->window();
            if (!window) {
                qWarning() << "Unable to get window, this will probably not work";
            } else {
                auto *sourcePrivate = QQuickItemPrivate::get(m_sourceItem);
                sourcePrivate->refWindow(window); // TODO: deref window as well
            }
        }

        // Quick Items are considered to always have transparency
        imageNode->m_textureData.m_textureFlags.setHasTransparency(true);

        if (QSGTextureProvider *provider = m_sourceItem->textureProvider()) {
            imageNode->m_qsgTexture = provider->texture();
            disconnect(m_textureProviderConnection);
            auto connection = connect(provider, &QSGTextureProvider::textureChanged, this, [provider, imageNode] () {
                imageNode->m_qsgTexture = provider->texture();
                imageNode->m_flags.setFlag(QSSGRenderImage::Flag::Dirty);
            }, Qt::DirectConnection);
            m_textureProviderConnection = connection;
            if (m_layer) {
                delete m_layer;
                m_layer = nullptr;
            }

            // TODO: handle dynamic textures
            // TODO: handle textureProvider property changed
        } else {
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

            // TODO: create larger textures on high-dpi displays?

            auto *sourcePrivate = QQuickItemPrivate::get(m_sourceItem);
            const QSize minTextureSize = sourcePrivate->sceneGraphContext()->minimumFBOSize();
            // Keep power-of-two by doubling the size.
            while (textureSize.width() < minTextureSize.width())
                textureSize.rwidth() *= 2;
            while (textureSize.height() < minTextureSize.height())
                textureSize.rheight() *= 2;

            m_layer->setSize(textureSize);

            // TODO: set mipmapFiltering, filtering, hWrap, vWrap?

            imageNode->m_qsgTexture = m_layer;
        }
        // TODO: Move inside block above?
        nodeChanged = true; // @todo: make more granular
    } else {
        if (m_layer) {
            m_layer->setItem(nullptr);
            delete m_layer;
            m_layer = nullptr;
        }
        nodeChanged |= qUpdateIfNeeded(imageNode->m_qsgTexture, static_cast<QSGTexture *>(nullptr));
    }

    if (nodeChanged)
        imageNode->m_flags.setFlag(QSSGRenderImage::Flag::Dirty);

    return imageNode;
}

void QQuick3DTexture::itemChange(QQuick3DObject::ItemChange change, const QQuick3DObject::ItemChangeData &value)
{
    QQuick3DObject::itemChange(change, value);
    if (change == QQuick3DObject::ItemChange::ItemSceneChange && m_sourceItem) {
        if (m_sceneManagerForLayer) {
            m_sceneManagerForLayer->qsgDynamicTextures.removeOne(m_layer);
            m_sceneManagerForLayer = nullptr;
        }
        trySetSourceParent();
        const auto &sceneManager = value.sceneManager;
        Q_ASSERT(QQuick3DObjectPrivate::get(this)->sceneManager == sceneManager);
        if (m_layer) {
            if (sceneManager)
                sceneManager->qsgDynamicTextures << m_layer;
            m_sceneManagerForLayer = sceneManager;
        }
    }
}

void QQuick3DTexture::itemGeometryChanged(QQuickItem *item, QQuickGeometryChange change, const QRectF &geometry)
{
    Q_ASSERT(item == m_sourceItem);
    Q_UNUSED(item)
    Q_UNUSED(geometry)
    if (change.sizeChange()) {
        auto renderImage = getRenderImage();
        if (renderImage)
            renderImage->m_flags.setFlag(QSSGRenderImage::Flag::ItemSizeDirty);
        update();
    }
}

void QQuick3DTexture::sourceItemDestroyed(QObject *item)
{
    Q_ASSERT(item == m_sourceItem);
    Q_UNUSED(item)
    m_sourceItem = nullptr;
    emit sourceItemChanged();
    update();
}

// Create layer and update once valid layer texture exists
void QQuick3DTexture::createLayerTexture()
{
    auto *sourcePrivate = QQuickItemPrivate::get(m_sourceItem);
//    Q_ASSERT_X(sourcePrivate->window && sourcePrivate->sceneGraphRenderContext()
//               && QThread::currentThread() == sourcePrivate->sceneGraphRenderContext()->thread(),
//               Q_FUNC_INFO, "Cannot be used outside the rendering thread");

    QSGRenderContext *rc = sourcePrivate->sceneGraphRenderContext();
    auto *layer = rc->sceneGraphContext()->createLayer(rc);
    connect(sourcePrivate->window, SIGNAL(sceneGraphInvalidated()), layer, SLOT(invalidated()), Qt::DirectConnection);

    const auto &manager = QQuick3DObjectPrivate::get(this)->sceneManager;
    manager->qsgDynamicTextures << layer;
    m_sceneManagerForLayer = manager;
    connect(layer, &QObject::destroyed, manager.data(), [this, manager, layer]() {
        manager->qsgDynamicTextures.removeAll(layer);
        m_sceneManagerForLayer = nullptr;
        m_initialized = false;
    }, Qt::DirectConnection);

    // When layer has been updated, take it into use.
    connect(layer, &QSGLayer::scheduledUpdateCompleted, this, [this, layer]() {
        m_layer = layer;
        update();
    });

    // With every frame (even when QQuickWindow isn't dirty so doesn't render),
    // try to update the texture. If updateTexture() returns false, content hasn't changed.
    connect(sourcePrivate->window, &QQuickWindow::beforeSynchronizing, [this]() {
        if (m_layer) {
            bool textureUpdated = m_layer->updateTexture();
            if (textureUpdated)
                update();
        }
    });

    layer->markDirtyTexture();
    layer->scheduleUpdate();
    update();
}

QSSGRenderImage *QQuick3DTexture::getRenderImage()
{
    QQuick3DObjectPrivate *p = QQuick3DObjectPrivate::get(this);
    return static_cast<QSSGRenderImage *>(p->spatialNode);
}

void QQuick3DTexture::markAllDirty()
{
    m_dirtyFlags = DirtyFlags(DirtyFlag::TransformDirty) | DirtyFlags(DirtyFlag::SourceDirty);
    QQuick3DObject::markAllDirty();
}

QT_END_NAMESPACE
