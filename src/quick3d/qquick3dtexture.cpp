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
#include <QtQuick3DRuntimeRender/private/qssgrendertexturedata_p.h>
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
    \l source property, a Quick item using the sourceItem property, or by
    setting the \l textureData property to a \l TextureData item subclass
    for defining the custom texture contents.
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

    for (const auto &connection : m_connections.values())
        disconnect(connection);
}

/*!
    \qmlproperty url QtQuick3D::Texture::source

    This property holds the location of an image file containing the data used
    by the texture.

    \sa sourceItem
    \sa textureData
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
    \sa textureData
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

    By default, this property is set to \c Texture.Repeat

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

    By default, this property is set to \c Texture.Repeat

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
    \qmlproperty int QtQuick3D::Texture::indexUV

    This property sets the UV coordinate index used by this texture.
    Since QtQuick3D supports 2 UV sets(0 or 1) for now,
    the value will be saturated to the range.
*/
int QQuick3DTexture::indexUV() const
{
    return m_indexUV;
}

/*!
    \qmlproperty enumeration QtQuick3D::Texture::magFilter

    This property determines how the texture is sampled when a texel covers
    more than one pixel.

    The default value is \c Texture.Linear

    \value Texture.None defaults to \c Texture.Linear since it is not possible to disable
    \value Texture.Nearest uses the value of the closest texel
    \value Texture.Linear takes the four closest texels and bilinearly interpolates them.

    \note Using \c Texture.None is not possible and will default to \c Texture.Linear

    \sa minFilter
    \sa mipFilter
*/
QQuick3DTexture::Filter QQuick3DTexture::magFilter() const
{
    return m_magFilter;
}

/*!
    \qmlproperty enumeration QtQuick3D::Texture::minFilter

    This property determines how the texture is sampled when a texel covers
    more than one pixel.

    The default value is \c Texture.Linear

    \value Texture.Nearest uses the value of the closest texel
    \value Texture.Linear takes the four closest texels and bilinearly interpolates them.

    \note Using \c Texture.None is not possible and will default to \c Texture.Linear

    \sa magFilter
    \sa mipFilter
*/
QQuick3DTexture::Filter QQuick3DTexture::minFilter() const
{
    return m_minFilter;
}

/*!
    \qmlproperty enumeration QtQuick3D::Texture::mipFilter

    This property determines how the texture mipmaps are sampled when a texel covers
    less than one pixel.

    The default value is \c Texture.None

    \value Texture.None disables the usage of mipmap sampling
    \value Texture.Nearest uses mipmapping and samples the value of the closest texel
    \value Texture.Linear uses mipmapping and interpolates between multiple texel values.

    \note This property will have no effect on Textures that do not have mipmaps.

    \sa minFilter
    \sa magFilter
*/
QQuick3DTexture::Filter QQuick3DTexture::mipFilter() const
{
    return m_mipFilter;
}

/*!
    \qmlproperty TextureData QtQuick3D::Texture::textureData

    This property holds a reference to a \l TextureData component which
    defines the contents and properties of raw texture data.

    If this property is used, then the value of \l source will be ignored.

    \sa source
    \sa sourceItem
*/

QQuick3DTextureData *QQuick3DTexture::textureData() const
{
    return m_textureData;
}

/*!
    \qmlproperty bool QtQuick3D::Texture::generateMipmaps

    This property determines if mipmaps are generated for textures that
    do not provide mipmap levels themselves. Using mipmaps along with mip
    filtering gives better visual quality when viewing texturesat a distance
    compared rendering without them, but it may come at a performance
    cost (both when initializing the image and during rendering).

    By default, this property is set to false.

    \note It is necessary to set a \c QQuick3D::Texture::mipFilter mode for the
    generated mipmaps to be be used.

    /sa mipFilter

*/
bool QQuick3DTexture::generateMipmaps() const
{
    return m_generateMipmaps;
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

void QQuick3DTexture::setIndexUV(int indexUV)
{
    if (m_indexUV == indexUV)
        return;

    if (indexUV < 0)
        m_indexUV = 0;
    else if (indexUV > 1)
        m_indexUV = 1;
    else
        m_indexUV = indexUV;

    m_dirtyFlags.setFlag(DirtyFlag::IndexUVDirty);
    emit indexUVChanged();
    update();
}

void QQuick3DTexture::setTextureData(QQuick3DTextureData *textureData)
{
    if (m_textureData == textureData)
        return;

    // Make sure to disconnect if the geometry gets deleted out from under us
    updatePropertyListener(textureData, m_textureData, QQuick3DObjectPrivate::get(this)->sceneManager, QByteArrayLiteral("textureData"), m_connections, [this](QQuick3DObject *n) {
        setTextureData(qobject_cast<QQuick3DTextureData *>(n));
    });

    if (m_textureData)
        QObject::disconnect(m_textureDataConnection);
    m_textureData = textureData;
    m_textureDataConnection
            = QObject::connect(m_textureData, &QQuick3DTextureData::textureDataNodeDirty, [this]() {
        markDirty(DirtyFlag::TextureDataDirty);
    });

    markDirty(DirtyFlag::TextureDataDirty);
    emit textureDataChanged();
}

void QQuick3DTexture::setGenerateMipmaps(bool generateMipmaps)
{
    if (m_generateMipmaps == generateMipmaps)
        return;

    m_generateMipmaps = generateMipmaps;
    m_dirtyFlags.setFlag(DirtyFlag::SamplerDirty);
    emit generateMipmapsChanged();
    update();
}

void QQuick3DTexture::setMagFilter(QQuick3DTexture::Filter magFilter)
{
    if (m_magFilter == magFilter)
        return;

    m_magFilter = magFilter;
    m_dirtyFlags.setFlag(DirtyFlag::SamplerDirty);
    emit magFilterChanged();
    update();
}

void QQuick3DTexture::setMinFilter(QQuick3DTexture::Filter minFilter)
{
    if (m_minFilter == minFilter)
        return;

    m_minFilter = minFilter;
    m_dirtyFlags.setFlag(DirtyFlag::SamplerDirty);
    emit minFilterChanged();
    update();
}

void QQuick3DTexture::setMipFilter(QQuick3DTexture::Filter mipFilter)
{
    if (m_mipFilter == mipFilter)
        return;

    m_mipFilter = mipFilter;
    m_dirtyFlags.setFlag(DirtyFlag::SamplerDirty);
    emit mipFilterChanged();
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
        const QQmlContext *context = qmlContext(this);
        if (!m_source.isEmpty())
            imageNode->m_imagePath = QSSGRenderPath(QQmlFile::urlToLocalFileOrQrc(context ? context->resolvedUrl(m_source) : m_source));
        nodeChanged = true;
    }
    if (m_dirtyFlags.testFlag(DirtyFlag::IndexUVDirty)) {
        m_dirtyFlags.setFlag(DirtyFlag::IndexUVDirty, false);
        imageNode->m_indexUV = m_indexUV;
    }
    nodeChanged |= qUpdateIfNeeded(imageNode->m_mappingMode,
                                  QSSGRenderImage::MappingModes(m_mappingMode));
    nodeChanged |= qUpdateIfNeeded(imageNode->m_horizontalTilingMode,
                                  QSSGRenderTextureCoordOp(m_tilingModeHorizontal));
    nodeChanged |= qUpdateIfNeeded(imageNode->m_verticalTilingMode,
                                  QSSGRenderTextureCoordOp(m_tilingModeVertical));

    if (m_dirtyFlags.testFlag(DirtyFlag::SamplerDirty)) {
        m_dirtyFlags.setFlag(DirtyFlag::SamplerDirty, false);
        nodeChanged |= qUpdateIfNeeded(imageNode->m_minFilterType,
                                       QSSGRenderTextureMinifyingOp(m_minFilter));
        nodeChanged |= qUpdateIfNeeded(imageNode->m_magFilterType,
                                       QSSGRenderTextureMagnifyingOp(m_magFilter));
        nodeChanged |= qUpdateIfNeeded(imageNode->m_mipFilterType,
                                       QSSGRenderTextureMinifyingOp(m_mipFilter));
        nodeChanged |= qUpdateIfNeeded(imageNode->m_generateMipmaps,
                                       m_generateMipmaps);
    }

    if (m_dirtyFlags.testFlag(DirtyFlag::TextureDataDirty)) {
        m_dirtyFlags.setFlag(DirtyFlag::TextureDataDirty, false);
        if (m_textureData)
            imageNode->m_rawTextureData = static_cast<QSSGRenderTextureData *>(QQuick3DObjectPrivate::get(m_textureData)->spatialNode);
        else
            imageNode->m_rawTextureData = nullptr;
        nodeChanged |= true;
    }

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
            m_textureProviderConnection = connect(provider, &QSGTextureProvider::textureChanged, this, [provider, imageNode] () {
                imageNode->m_qsgTexture = provider->texture();
                imageNode->m_flags.setFlag(QSSGRenderImage::Flag::Dirty);
            }, Qt::DirectConnection);

            disconnect(m_textureUpdateConnection);
            auto *sourcePrivate = QQuickItemPrivate::get(m_sourceItem);
            if (sourcePrivate->window) {
                m_textureUpdateConnection = connect(sourcePrivate->window, &QQuickWindow::beforeSynchronizing, this, [this, imageNode]() {
                    if (QSGDynamicTexture *t = qobject_cast<QSGDynamicTexture *>(imageNode->m_qsgTexture)) {
                        if (t->updateTexture())
                            update();
                    }
                }, Qt::DirectConnection);
            } else {
                qWarning("No window for item, texture updates are doomed");
            }

            if (m_layer) {
                delete m_layer;
                m_layer = nullptr;
            }

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
    if (change == QQuick3DObject::ItemChange::ItemSceneChange) {
        // Source item
        if (m_sourceItem) {
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
        // TextureData
        if (m_textureData) {
            const auto &sceneManager = value.sceneManager;
            if (sceneManager)
                QQuick3DObjectPrivate::refSceneManager(m_textureData, sceneManager);
            else
                QQuick3DObjectPrivate::derefSceneManager(m_textureData);
        }
    }
}

void QQuick3DTexture::itemGeometryChanged(QQuickItem *item, QQuickGeometryChange change, const QRectF &geometry)
{
    Q_ASSERT(item == m_sourceItem);
    Q_UNUSED(item);
    Q_UNUSED(geometry);
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
    Q_UNUSED(item);
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
        delete m_layer;
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

void QQuick3DTexture::markDirty(QQuick3DTexture::DirtyFlag type)
{
    if (!m_dirtyFlags.testFlag(type)) {
        m_dirtyFlags.setFlag(type, true);
        update();
    }
}

QSSGRenderImage *QQuick3DTexture::getRenderImage()
{
    QQuick3DObjectPrivate *p = QQuick3DObjectPrivate::get(this);
    return static_cast<QSSGRenderImage *>(p->spatialNode);
}

void QQuick3DTexture::markAllDirty()
{
    m_dirtyFlags = DirtyFlags(DirtyFlag::TransformDirty) |
                   DirtyFlags(DirtyFlag::SourceDirty) |
                   DirtyFlags(DirtyFlag::IndexUVDirty) |
                   DirtyFlags(DirtyFlag::TextureDataDirty) |
                   DirtyFlags(DirtyFlag::SamplerDirty);
    QQuick3DObject::markAllDirty();
}

QT_END_NAMESPACE
