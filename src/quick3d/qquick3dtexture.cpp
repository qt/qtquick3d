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

    A texture is technically any array of pixels (1D, 2D or 3D) and its related
    settings, such as minification and magnification filters, scaling and UV
    transformations.

    The Texture type in Qt Quick 3D represents a two-dimensional image. Its use
    is typically to map onto / wrap around three-dimensional geometry to emulate
    additional detail which cannot be efficiently modelled in 3D. It can also be
    used to emulate other lighting effects, such as reflections.

    When the geometry is being rendered, each location on its surface will be
    transformed to a corresponding location in the texture by transforming and
    interpolating the UV coordinates (texture coordinate) that have been set for
    the mesh's vertexes. The fragment shader program that is being used to render
    the active material will then typically sample the material's texture(s) at
    the given coordinates and use the sampled data in its light calculations.

    \note A Material may use multiple textures to give the desired interaction with
    light in the 3D scene. It can represent the color of each texel on the geometry
    surface, but also other attributes of the surface. For instance, a "normal map"
    can represent the deviation from the geometry normals for each texel on the
    surface, emulating light interaction with finer details on the surface, such
    as cracks or bumps. See \l{Qt Quick 3D - Principled Material Example}{the principled
    material example} for a demonstration of a material with multiple texture maps.

    Texture objects can source image data from:
    \list
    \li an image or texture file by using the \l source property,
    \li a Qt Quick \l Item by using the sourceItem property,
    \li or by setting the \l textureData property to a \l TextureData item
    subclass for defining the custom texture contents.
    \endlist

    The following example maps the image "madewithqt.png" onto the default sphere
    mesh, and scales the UV coordinates to tile the image on the sphere surface.
    \qml
    Model {
        source: "#Sphere"
        materials: [ PrincipledMaterial {
                baseColorMap: Texture {
                    source: "madewithqt.png"
                    scaleU: 4.0
                    scaleV: 4.0
                }
            }
        ]
    }
    \endqml

    The result looks as follows:
    \table
    \header
    \li Original image
    \li Mapped onto a sphere
    \row
    \li \image madewithqt.png
    \li \image spheremap.png
    \endtable

    \sa {Qt Quick 3D - Dynamic Texture Example}, {Qt Quick 3D - Procedural Texture Example}
*/

QQuick3DTexture::QQuick3DTexture(QQuick3DObject *parent)
    : QQuick3DObject(*(new QQuick3DObjectPrivate(QQuick3DObjectPrivate::Type::Image)), parent) {}

QQuick3DTexture::~QQuick3DTexture()
{
    if (m_layer && m_sceneManagerForLayer) {
        m_sceneManagerForLayer->qsgDynamicTextures.removeAll(m_layer);
        m_layer->deleteLater(); // uhh...
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

    This property holds the location of an image or texture file containing the data used by the
    texture.

    The property is a URL, with the same rules as other source properties, such
    as \l{Image::source}{Image.source}. With Texture, only the \c qrc and \c
    file schemes are supported. When no scheme is present and the value is a
    relative path, it is assumed to be relative to the component's (i.e. the
    \c{.qml} file's) location.

    The source file can have any conventional image file format
    \l{QImageReader::supportedImageFormats()}{supported by Qt}. In addition, Texture supports the
    same \l [QtQuick]{Compressed Texture Files}{compressed texture file types} as QtQuick::Image.

    \note As Quick has the Y axis pointing downwards, while in Qt Quick 3D it points upwards, the
    orientation of textures may appear different by default. The \l flipV property may be used to
    change the orientation.

    \sa sourceItem, textureData, flipV
*/
QUrl QQuick3DTexture::source() const
{
    return m_source;
}

/*!
    \qmlproperty Item QtQuick3D::Texture::sourceItem

    This property defines a Item to be used as the source of the texture. Using
    this property allows any 2D Qt Quick content to be used as a texture source
    by rendering that item as an offscreen layer.

    If the item is a \l{QQuickItem::textureProvider()}{texture provider}, no
    additional texture is used.

    If this property is set, then the value of \l source will be ignored. A
    Texture should use one method to provide image data, and set only one of
    source, \l sourceItem, or \l textureData.

    \note Currently there is no way to forward input events to the Item used as
    a texture source.

    \sa source, textureData
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

    The default is 1.0.

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

    The default is 1.0.

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
    \value Texture.LightProbe The default for HDRI sphere maps used by light
    probes. Normally this does not need to be set when using a light probe, but
    may need to be set explicitly when using environment maps.
*/
QQuick3DTexture::MappingMode QQuick3DTexture::mappingMode() const
{
    return m_mappingMode;
}

/*!
    \qmlproperty enumeration QtQuick3D::Texture::tilingModeHorizontal

    Controls how the texture is mapped when the U scaling value is greater than 1.

    By default, this property is set to \c{Texture.Repeat}.

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

    By default, this property is set to \c{Texture.Repeat}.

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
    using euler angles and for a positive value rotation is clockwise.

    The default is 0.0.

    \sa pivotU, pivotV
*/
float QQuick3DTexture::rotationUV() const
{
    return m_rotationUV;
}

/*!
    \qmlproperty float QtQuick3D::Texture::positionU

    This property offsets the U coordinate mapping from left to right.

    The default is 0.0.
*/
float QQuick3DTexture::positionU() const
{
    return m_positionU;
}

/*!
    \qmlproperty float QtQuick3D::Texture::positionV

    This property offsets the V coordinate mapping from bottom to top.

    The default is 0.0.

    \note Qt Quick 3D uses OpenGL-style vertex data, regardless of the graphics
    API used at run time. The UV position \c{(0, 0)} is therefore referring to
    the bottom-left corner of the image data.
*/
float QQuick3DTexture::positionV() const
{
    return m_positionV;
}

/*!
    \qmlproperty float QtQuick3D::Texture::pivotU

    This property sets the pivot U position which is used when applying a
    \l{QtQuick3D::Texture::rotationUV}{rotationUV}.

    The default is 0.0.

    \sa rotationUV
*/
float QQuick3DTexture::pivotU() const
{
    return m_pivotU;
}

/*!
    \qmlproperty float QtQuick3D::Texture::pivotV

    This property sets the pivot V position which is used when applying a
    \l{QtQuick3D::Texture::rotationUV}{rotationUV}.

    The default is 0.0.

    \sa rotationUV
*/
float QQuick3DTexture::pivotV() const
{
    return m_pivotV;
}

/*!
    \qmlproperty bool QtQuick3D::Texture::flipV

    This property sets the use of the vertically flipped coordinates.

    The default is false.
*/
bool QQuick3DTexture::flipV() const
{
    return m_flipV;
}

/*!
    \qmlproperty int QtQuick3D::Texture::indexUV

    This property sets the UV coordinate index used by this texture. Since
    QtQuick3D supports 2 UV sets(0 or 1) for now, the value will be saturated
    to the range.

    The default is 0.
*/
int QQuick3DTexture::indexUV() const
{
    return m_indexUV;
}

/*!
    \qmlproperty enumeration QtQuick3D::Texture::magFilter

    This property determines how the texture is sampled when a texel covers
    more than one pixel.

    The default value is \c{Texture.Linear}.

    \value Texture.Nearest uses the value of the closest texel.
    \value Texture.Linear takes the four closest texels and bilinearly interpolates them.

    \note Using \c Texture.None here will default to \c Texture.Linear instead.

    \sa minFilter, mipFilter
*/
QQuick3DTexture::Filter QQuick3DTexture::magFilter() const
{
    return m_magFilter;
}

/*!
    \qmlproperty enumeration QtQuick3D::Texture::minFilter

    This property determines how the texture is sampled when a texel covers
    more than one pixel.

    The default value is \c{Texture.Linear}.

    \value Texture.Nearest uses the value of the closest texel.
    \value Texture.Linear takes the four closest texels and bilinearly interpolates them.

    \note Using \c Texture.None here will default to \c Texture.Linear instead.

    \sa magFilter, mipFilter
*/
QQuick3DTexture::Filter QQuick3DTexture::minFilter() const
{
    return m_minFilter;
}

/*!
    \qmlproperty enumeration QtQuick3D::Texture::mipFilter

    This property determines how the texture mipmaps are sampled when a texel covers
    less than one pixel.

    The default value is \c{Texture.None}.

    \value Texture.None disables the usage of mipmap sampling.
    \value Texture.Nearest uses mipmapping and samples the value of the closest texel.
    \value Texture.Linear uses mipmapping and interpolates between multiple texel values.

    \note This property will have no effect on Textures that do not have mipmaps.

    \sa minFilter, magFilter
*/
QQuick3DTexture::Filter QQuick3DTexture::mipFilter() const
{
    return m_mipFilter;
}

/*!
    \qmlproperty TextureData QtQuick3D::Texture::textureData

    This property holds a reference to a \l TextureData component which
    defines the contents and properties of raw texture data.

    If this property is used, then the value of \l source will be ignored. A
    Texture should use one method to provide image data, and set only one of
    source, \l sourceItem, or \l textureData.

    \sa source, sourceItem, {Qt Quick 3D - Procedural Texture Example}
*/

QQuick3DTextureData *QQuick3DTexture::textureData() const
{
    return m_textureData;
}

/*!
    \qmlproperty bool QtQuick3D::Texture::generateMipmaps

    This property determines if mipmaps are generated for textures that
    do not provide mipmap levels themselves. Using mipmaps along with mip
    filtering gives better visual quality when viewing textures at a distance
    compared rendering without them, but it may come at a performance
    cost (both when initializing the image and during rendering).

    By default, this property is set to false.

    \note It is necessary to set a \l{QtQuick3D::Texture::mipFilter}{mipFilter} mode
    for the generated mipmaps to be be used.

    \sa mipFilter
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
    m_dirtyFlags.setFlag(DirtyFlag::SourceItemDirty);
    m_dirtyFlags.setFlag(DirtyFlag::TextureDataDirty);
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

    disconnect(m_textureProviderConnection);
    disconnect(m_textureUpdateConnection);

    if (m_sourceItem) {
        QQuickItemPrivate *sourcePrivate = QQuickItemPrivate::get(m_sourceItem);

        const bool hide = m_sourceItemReparented;
        sourcePrivate->derefFromEffectItem(hide);
        m_sourceItemRefed = false;

        sourcePrivate->removeItemChangeListener(this, QQuickItemPrivate::Geometry);
        disconnect(m_sourceItem, SIGNAL(destroyed(QObject*)), this, SLOT(sourceItemDestroyed(QObject*)));
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

    if (m_layer) {
        const auto &manager = QQuick3DObjectPrivate::get(this)->sceneManager;
        manager->qsgDynamicTextures.removeAll(m_layer);
        m_sceneManagerForLayer = nullptr;
        // cannot touch m_layer here
    }
    m_initializedSourceItem = nullptr;

    m_dirtyFlags.setFlag(DirtyFlag::SourceDirty);
    m_dirtyFlags.setFlag(DirtyFlag::SourceItemDirty);
    m_dirtyFlags.setFlag(DirtyFlag::TextureDataDirty);
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
    QQuick3DObjectPrivate::updatePropertyListener(textureData, m_textureData, QQuick3DObjectPrivate::get(this)->sceneManager, QByteArrayLiteral("textureData"), m_connections, [this](QQuick3DObject *n) {
        setTextureData(qobject_cast<QQuick3DTextureData *>(n));
    });

    if (m_textureData)
        QObject::disconnect(m_textureDataConnection);
    m_textureData = textureData;

    if (m_textureData) {
        m_textureDataConnection
                = QObject::connect(m_textureData, &QQuick3DTextureData::textureDataNodeDirty, [this]() {
            markDirty(DirtyFlag::TextureDataDirty);
        });
    }

    m_dirtyFlags.setFlag(DirtyFlag::SourceDirty);
    m_dirtyFlags.setFlag(DirtyFlag::SourceItemDirty);
    m_dirtyFlags.setFlag(DirtyFlag::TextureDataDirty);
    emit textureDataChanged();
    update();
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

    if (m_dirtyFlags.testFlag(DirtyFlag::TransformDirty)) {
        m_dirtyFlags.setFlag(DirtyFlag::TransformDirty, false);
        imageNode->m_flipV = m_sourceItem ? !m_flipV : m_flipV;
        imageNode->m_scale = QVector2D(m_scaleU, m_scaleV);
        imageNode->m_pivot = QVector2D(m_pivotU, m_pivotV);
        imageNode->m_rotation = m_rotationUV;
        imageNode->m_position = QVector2D(m_positionU, m_positionV);

        imageNode->m_flags.setFlag(QSSGRenderImage::Flag::TransformDirty);
    }

    bool nodeChanged = false;
    if (m_dirtyFlags.testFlag(DirtyFlag::SourceDirty)) {
        m_dirtyFlags.setFlag(DirtyFlag::SourceDirty, false);
        if (!m_source.isEmpty()) {
            const QQmlContext *context = qmlContext(this);
            imageNode->m_imagePath = QSSGRenderPath(QQmlFile::urlToLocalFileOrQrc(context ? context->resolvedUrl(m_source) : m_source));
        } else {
            imageNode->m_imagePath = QSSGRenderPath();
        }
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
                                       QSSGRenderTextureFilterOp(m_minFilter));
        nodeChanged |= qUpdateIfNeeded(imageNode->m_magFilterType,
                                       QSSGRenderTextureFilterOp(m_magFilter));
        nodeChanged |= qUpdateIfNeeded(imageNode->m_mipFilterType,
                                       QSSGRenderTextureFilterOp(m_mipFilter));
        nodeChanged |= qUpdateIfNeeded(imageNode->m_generateMipmaps,
                                       m_generateMipmaps);
    }

    if (m_dirtyFlags.testFlag(DirtyFlag::TextureDataDirty)) {
        m_dirtyFlags.setFlag(DirtyFlag::TextureDataDirty, false);
        if (m_textureData)
            imageNode->m_rawTextureData = static_cast<QSSGRenderTextureData *>(QQuick3DObjectPrivate::get(m_textureData)->spatialNode);
        else
            imageNode->m_rawTextureData = nullptr;
        nodeChanged = true;
    }

    if (m_dirtyFlags.testFlag(DirtyFlag::SourceItemDirty)) {
        m_dirtyFlags.setFlag(DirtyFlag::SourceItemDirty, false);
        if (m_sourceItem) {
            QQuickWindow *window = m_sourceItem->window();
            if (!window) {
                window = QQuick3DObjectPrivate::get(this)->sceneManager->window();
                if (window)
                    QQuickItemPrivate::get(m_sourceItem)->refWindow(window);
                else
                    qWarning("Unable to get window, this will probably not work");
            }

            // Quick Items are considered to always have transparency
            imageNode->m_textureData.m_textureFlags.setHasTransparency(true);

            // This assumes that the QSGTextureProvider returned never changes,
            // which is hopefully the case for both Image and Item layers.
            if (QSGTextureProvider *provider = m_sourceItem->textureProvider()) {
                imageNode->m_qsgTexture = provider->texture();

                disconnect(m_textureProviderConnection);
                m_textureProviderConnection = connect(provider, &QSGTextureProvider::textureChanged, this, [provider, imageNode] () {
                    // called on the render thread, if there is one; while not
                    // obvious, the gui thread is blocked too because one can
                    // get here only from either the textureProvider() call
                    // above, or from QQuickImage::updatePaintNode()
                    imageNode->m_qsgTexture = provider->texture();
                    // the QSGTexture may be different now, go through loadRenderImage() again
                    imageNode->m_flags.setFlag(QSSGRenderImage::Flag::Dirty);
                }, Qt::DirectConnection);

                disconnect(m_textureUpdateConnection);
                auto *sourcePrivate = QQuickItemPrivate::get(m_sourceItem);
                if (sourcePrivate->window) {
                    QQuickItem *sourceItem = m_sourceItem; // for capturing, recognizing in the lambda that m_sourceItem has changed is essential
                    m_textureUpdateConnection = connect(sourcePrivate->window, &QQuickWindow::beforeSynchronizing, this, [this, imageNode, sourceItem]() {
                        // Called on the render thread with gui blocked (if there is a render thread, that is).
                        if (m_sourceItem != sourceItem) {
                            disconnect(m_textureProviderConnection);
                            disconnect(m_textureUpdateConnection);
                            return;
                        }
                        if (QSGDynamicTexture *t = qobject_cast<QSGDynamicTexture *>(imageNode->m_qsgTexture)) {
                            if (t->updateTexture())
                                update(); // safe because the gui thread is blocked
                        }
                    }, Qt::DirectConnection);
                } else {
                    qWarning("No window for item, texture updates are doomed");
                }

                if (m_layer) {
                    delete m_layer;
                    m_layer = nullptr;
                }
            } else {
                if (m_initializedSourceItem != m_sourceItem) {
                    // If there was a previous sourceItem and m_layer is valid
                    // then set its content to null otherwise things blow up
                    // spectacularly.
                    if (m_layer)
                        m_layer->setItem(nullptr);

                    m_initializedSourceItem = m_sourceItem;

                    // When scene has been rendered for the first time, create layer texture.
                    connect(window, &QQuickWindow::afterRendering, this, [this, window]() {
                        // called on the render thread
                        disconnect(window, &QQuickWindow::afterRendering, this, nullptr);
                        if (m_layer) {
                            const auto &manager = QQuick3DObjectPrivate::get(this)->sceneManager;
                            manager->qsgDynamicTextures.removeAll(m_layer);
                            delete m_layer;
                            m_layer = nullptr;
                        }
                        createLayerTexture();
                    }, Qt::DirectConnection);
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

                imageNode->m_qsgTexture = m_layer;
            }
            if (imageNode->m_flipV != !m_flipV) {
                imageNode->m_flipV = !m_flipV;
                imageNode->m_flags.setFlag(QSSGRenderImage::Flag::TransformDirty);
            }
        } else {
            if (m_layer) {
                m_layer->setItem(nullptr);
                delete m_layer;
                m_layer = nullptr;
            }
            imageNode->m_qsgTexture = nullptr;
            if (imageNode->m_flipV != m_flipV) {
                imageNode->m_flipV = m_flipV;
                imageNode->m_flags.setFlag(QSSGRenderImage::Flag::TransformDirty);
            }
        }
        nodeChanged = true;
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
                QQuick3DObjectPrivate::refSceneManager(m_textureData, *sceneManager);
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

    m_dirtyFlags.setFlag(DirtyFlag::SourceDirty);
    m_dirtyFlags.setFlag(DirtyFlag::SourceItemDirty);
    m_dirtyFlags.setFlag(DirtyFlag::TextureDataDirty);
    emit sourceItemChanged();
    update();
}

// Create layer and update once valid layer texture exists
void QQuick3DTexture::createLayerTexture()
{
    auto *sourcePrivate = QQuickItemPrivate::get(m_sourceItem);

    QSGRenderContext *rc = sourcePrivate->sceneGraphRenderContext();
    Q_ASSERT(QThread::currentThread() == rc->thread()); // must be on the render thread
    auto *layer = rc->sceneGraphContext()->createLayer(rc);
    connect(sourcePrivate->window, SIGNAL(sceneGraphInvalidated()), layer, SLOT(invalidated()), Qt::DirectConnection);

    // This is very definitely incorrect since the gui thread is not blocked.
    const auto &manager = QQuick3DObjectPrivate::get(this)->sceneManager;
    manager->qsgDynamicTextures << layer;
    m_sceneManagerForLayer = manager;

    QQuickItem *sourceItem = m_sourceItem; // for capturing, recognizing in the lambda that m_sourceItem has changed is essential

    connect(
            layer,
            &QObject::destroyed,
            manager,
            [manager, layer]() {
                // this is on the render thread so all borked threading-wise (all data here is gui thread stuff...) but will survive
                manager->qsgDynamicTextures.removeAll(layer);
            },
            Qt::DirectConnection);

    connect(
            layer,
            &QObject::destroyed,
            this,
            [this, sourceItem]() {
                // this is on the render thread so all borked threading-wise (all data here is gui thread stuff...) but will survive
                if (m_initializedSourceItem == sourceItem) {
                    m_sceneManagerForLayer = nullptr;
                    m_initializedSourceItem = nullptr;
                }
            },
            Qt::DirectConnection);

    // When layer has been updated, take it into use.
    connect(layer, &QSGLayer::scheduledUpdateCompleted, this, [this, layer]() {
        // called on the render thread, no guarantee when, gui stuff must not be accessed

        // ...and so much for not accessing gui thread stuff
        m_layer = layer;
        m_dirtyFlags.setFlag(DirtyFlag::SourceItemDirty);  // to get updateSpatialNode do the rest of the m_layer setup

        // at least try to play nice here
        QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection);
    }, Qt::DirectConnection);

    // With every frame (even when QQuickWindow isn't dirty so doesn't render),
    // try to update the texture. If updateTexture() returns false, content hasn't changed.
    // This complements qsgDynamicTextures and QQuick3DViewport::updateDynamicTextures().
    m_textureUpdateConnection = connect(sourcePrivate->window, &QQuickWindow::beforeSynchronizing, this, [this, sourceItem]() {
        // Called on the render thread with gui blocked (if there is a render thread, that is).
        if (!m_layer)
            return;
        if (m_sourceItem != sourceItem) {
            disconnect(m_textureUpdateConnection);
            return;
        }
        if (m_layer->updateTexture())
            update();
    }, Qt::DirectConnection);

    layer->markDirtyTexture();
    layer->scheduleUpdate();

    QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection); // ugh
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
    m_dirtyFlags = DirtyFlags(0xFFFF);
    QQuick3DObject::markAllDirty();
}

QT_END_NAMESPACE
