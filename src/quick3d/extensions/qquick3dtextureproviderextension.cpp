// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default


#include "qquick3dtextureproviderextension.h"
#include <QtQuick3D/private/qquick3dobject_p.h>
#include "qquick3dtextureproviderextension_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QQuick3DTextureProviderExtension
    \inmodule QtQuick3D
    \since 6.11

    \brief Abstract class for implementing user side texture provider extensions.

    This is the front-end side of a texture provider extension. The back-end side is implemented
    in \l QSSGRenderExtension. The QQuick3DTextureProviderExtension is a specialization of the
    QQuick3DRenderExtension class that is used to create a custom texture provider extension that can
    both provide additional metadata about what type of texture will be provided and also automatically
    register the extension with the QtQuick3D scene graph. This means it is unecessary to manually add
    the extension to the list of extensions to be used with a \l View3D, and just using the textureProvider
    property of the Texture component is enough to trigger the extension code to be run when necessary.

    The QQuick3DTextureProviderExtension class is an abstract class that should be subclassed and
    exposed to QML. The subclass should implement the \l QQuick3DRenderExtension::updateSpatialNode()
    function and return a QSSGRenderExtension instance that contains the code that should be run.

    \sa QSSGRenderExtension
*/

/*!
    \enum QQuick3DTextureProviderExtension::SamplerHint

    Specifies a hint about the type of texture that will be
    provided by the extension.

    \value Sampler2D A 2D texture
    \value Sampler2DArray An array texture
    \value Sampler3D A 3D texture
    \value SamplerCube A cube map texture
    \value SamplerCubeArray An array of cube map textures
    \value SamplerBuffer A buffer texture

*/

/*!
    \qmltype TextureProviderExtension
    \nativetype QQuick3DTextureProviderExtension
    \inqmlmodule QtQuick3D
    \inherits RenderExtension
    \since 6.11
    \brief Abstract base type for render extensions that produce a texture.

    TextureProviderExtension is an uncreatable abstract base type for render extensions
    that expose a GPU texture to the Qt Quick 3D scene. Subclasses (such as
    \l RenderOutputProvider) can be assigned to the \l {Texture::textureProvider}
    {textureProvider} property of a \l Texture, causing the extension's render code
    to run automatically when the texture is needed by the scene.

    \sa RenderOutputProvider, QQuick3DTextureProviderExtension, QSSGRenderExtension
*/

/*!
    \property QQuick3DTextureProviderExtension::samplerHint
    \since 6.11

    This property contains a hint about the type of texture that will be
    provided by the extension. This is necessary because the texture data will
    not be provided until it is necessary, but materials that use the Texture
    component need to know what type of sampler to provide.

    The default value is \c QQuick3DTextureProviderExtension::Sampler2D.

    \note This property is only used when using CustomMaterials.

    \sa SamplerHint
*/

/*!
    \qmlenum TextureProviderExtension::SamplerHint
    \qmlenumeratorsfrom QQuick3DTextureProviderExtension::SamplerHint
    \since 6.11

Specifies a hint about the type of texture the extension provides.

*/

/*!
    \qmlproperty SamplerHint TextureProviderExtension::samplerHint
    \since 6.11

    This property contains a hint about the type of texture that will be provided by the extension.
    This is necessary because the texture data will not be provided until it is necessary, but materials
    that use the Texture component need to know what type of sampler to provide.

    The default value is \c TextureProviderExtension.Sampler2D.

    \note This property is only used when using CustomMaterials.
*/


QQuick3DTextureProviderExtensionPrivate::QQuick3DTextureProviderExtensionPrivate()
    : QQuick3DObjectPrivate(QQuick3DObjectPrivate::Type::TextureProvider)
{

}


QQuick3DTextureProviderExtension::QQuick3DTextureProviderExtension(QQuick3DObject *parent)
    : QQuick3DRenderExtension(*new QQuick3DTextureProviderExtensionPrivate(), parent)
{

}

QQuick3DTextureProviderExtension::~QQuick3DTextureProviderExtension()
{

}

/*!
    \fn QSSGRenderGraphObject *QQuick3DTextureProviderExtension::updateSpatialNode(QSSGRenderGraphObject *node)

    This function is called during the synchronization of the QtQuick3D scene graph when an item is
    created or when an update is requested, usually as a result of a change in the item's properties.
    The function should return a QSSGRenderTextureProviderExtension instance that contains the code that should be
    run during QtQuick3D's rendering pipeline execution.

    The \a node parameter is the previous QSSGRenderTextureProviderExtension instance that was returned from this
    function, or null if this is the first time the function is called. The function can return the
    same instance, a different instance, or null. If the function returns null, the extension will
    be removed from the rendering pipeline.

    \note The QSSGRenderTextureProviderExtension instance is a resource object and will be owned by the QtQuick3D
           scene graph. If a different instance, or null, is returned, the previous instance will be
           queued for deletion by the renderer.

    \sa QSSGRenderTextureProviderExtension
*/

/*!
    \fn void QQuick3DTextureProviderExtension::surfaceChanged()
    \since 6.12

    This signal is emitted when the underlying rendering surface has changed
    and textures need to be recreated.
*/

QSSGRenderGraphObject *QQuick3DTextureProviderExtension::updateSpatialNode(QSSGRenderGraphObject *node)
{
    return QQuick3DRenderExtension::updateSpatialNode(node);
}

QQuick3DTextureProviderExtension::SamplerHint QQuick3DTextureProviderExtension::samplerHint() const
{
    Q_D(const QQuick3DTextureProviderExtension);
    return d->samplerHint;
}

void QQuick3DTextureProviderExtension::setSamplerHint(SamplerHint newSamplerHint)
{
    Q_D(QQuick3DTextureProviderExtension);
    if (d->samplerHint == newSamplerHint)
        return;
    d->samplerHint = newSamplerHint;
    emit samplerHintChanged();
}

bool QQuick3DTextureProviderExtension::event(QEvent *event)
{
    return QQuick3DRenderExtension::event(event);
}

QT_END_NAMESPACE
