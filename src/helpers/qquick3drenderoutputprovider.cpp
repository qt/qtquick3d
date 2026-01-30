// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default


#include "qquick3drenderoutputprovider_p.h"

#include "private/qquick3dobject_p.h"

#include <QtQuick3DRuntimeRender/private/qssglayerrenderdata_p.h>

#include <ssg/qssgrenderextensions.h>
#include <ssg/qssgrenderhelpers.h>
#include <ssg/qssgrendercontextcore.h>
#include <ssg/qssgrhicontext.h>
#include <ssg/qquick3dextensionhelpers.h>

QT_BEGIN_NAMESPACE

class QSSGRenderOutputProviderExtension : public QSSGRenderTextureProviderExtension
{
public:
    enum class SourceType {
        None,
        BuiltInPass,
        UserPass,
    };

    explicit QSSGRenderOutputProviderExtension(QQuick3DRenderOutputProvider *ext);
    ~QSSGRenderOutputProviderExtension() override;
    bool prepareData(QSSGFrameData &data) override;
    void prepareRender(QSSGFrameData &data) override;
    void render(QSSGFrameData &data) override;
    void resetForFrame() override;

    SourceType m_sourceType = SourceType::BuiltInPass;
    QSSGFrameData::RenderResult m_builtInPass = QSSGFrameData::RenderResult::DepthTexture;
    QSSGResourceId userPassId = QSSGResourceId::Invalid;
    QSSGFrameData::AttachmentSelector attachmentSelector = QSSGFrameData::AttachmentSelector::Attachment0;
    bool m_isDirty = true;

    QPointer<QQuick3DRenderOutputProvider> m_ext;
    QSSGExtensionId m_extensionId {};
};

QSSGRenderOutputProviderExtension::QSSGRenderOutputProviderExtension(QQuick3DRenderOutputProvider *ext)
    : m_ext(ext)
{

}

QSSGRenderOutputProviderExtension::~QSSGRenderOutputProviderExtension()
{

}

bool QSSGRenderOutputProviderExtension::prepareData(QSSGFrameData &data)
{
    // Update the extension ID.
    // NOTE: This needs to be done on each call, the extension object may have changed.
    //       If the extension is destroyed (m_ext == nullptr), we cannot proceed and should return false!
    m_extensionId = m_ext ? QQuick3DExtensionHelpers::getExtensionId(*m_ext) : QSSGExtensionId::Invalid;
    if (QQuick3DExtensionHelpers::isNull(m_extensionId))
        return false;

    const bool wasDirty = m_isDirty;

    switch (m_sourceType) {
    case SourceType::None:
        // Nothing to do
        m_isDirty = false;
        break;
    case SourceType::BuiltInPass:
    {
        // Make sure we schedule the pass to run this frame
        data.scheduleRenderResults(QSSGFrameData::RenderResults(m_builtInPass));

        // Check if a texture exists for the result already
        QSSGFrameData::Result extResult = data.getRenderResult(m_builtInPass);
        if (extResult.texture) {
            QSSGRenderExtensionHelpers::registerRenderResult(data, m_extensionId, extResult.texture);
            m_isDirty = false;

        }
        // No "else" in this case since its likely the texture for the pass doesn't exist yet, try again in prepareRender.
    }
        break;
    case SourceType::UserPass:
    {
        if (userPassId != QSSGResourceId::Invalid) {
            // Make sure we schedule the pass to run this frame
            data.scheduleRenderResults(userPassId);

            // Check if a texture exists for the result already
            QSSGFrameData::Result extResult = data.getRenderResult(userPassId, attachmentSelector);
            if (extResult.texture) {
                // Associate the texture with this extension and expose it so it can be used.
                QSSGRenderExtensionHelpers::registerRenderResult(data, m_extensionId, extResult.texture);
                m_isDirty = false;

            }
        }
        // No "else" in this case since its likely the texture for the pass doesn't exist yet, try again in prepareRender.
    }
        break;
    }

    return wasDirty;
}

void QSSGRenderOutputProviderExtension::prepareRender(QSSGFrameData &data)
{
    // If no id is set prepareData() should return false and this should not be called!
    Q_ASSERT(!QQuick3DExtensionHelpers::isNull(m_extensionId));

    QSSGFrameData::Result extResult;
    if (m_sourceType == SourceType::BuiltInPass)
        extResult = data.getRenderResult(m_builtInPass);
    else if (m_sourceType == SourceType::UserPass && userPassId != QSSGResourceId::Invalid)
        extResult = data.getRenderResult(userPassId, attachmentSelector);

    if (userPassId != QSSGResourceId::Invalid) {
        if (extResult.texture) {
            QSSGRenderExtensionHelpers::registerRenderResult(data, m_extensionId, extResult.texture);
            m_isDirty = false;
        }
    }
}

void QSSGRenderOutputProviderExtension::render(QSSGFrameData &)
{
    // Nothing to render
}

void QSSGRenderOutputProviderExtension::resetForFrame()
{
    // Nothing to reset
}


/*!
    \qmltype RenderOutputProvider
    \nativetype QQuick3DRenderOutputProvider
    \inqmlmodule QtQuick3D.Helpers
    \inherits TextureProviderExtension
    \since 6.11
    \brief Used as a bridge to access textures provided in passes.

    This type is used to provide textures that are generated by passes in the rendering pipeline.

    Some example of when this is useful is when you want to access the depth texture generated by the ZPrePass,
    or the ambient occlusion texture generated by the SSAO pass. This type can be used to access these generated
    textures and use them in materials or effects.

    \qml
    Texture {
        textureProvider: RenderOutputProvider {
            // Specify the pass buffer texture you want to access
            textureSource: RenderOutputProvider.DepthTexture
        }
    }
    \endqml

    \sa QQuick3DTextureProviderExtension, QSSGRenderExtension
*/

QQuick3DRenderOutputProvider::QQuick3DRenderOutputProvider(QQuick3DObject *parent)
    : QQuick3DTextureProviderExtension(parent)
{
    update();
}

QSSGRenderGraphObject *QQuick3DRenderOutputProvider::updateSpatialNode(QSSGRenderGraphObject *node)
{
    // Create new node if needed
    if (!node)
        node = new QSSGRenderOutputProviderExtension(this);

    QQuick3DTextureProviderExtension::updateSpatialNode(node);

    QSSGRenderOutputProviderExtension *providerNode = static_cast<QSSGRenderOutputProviderExtension *>(node);

    providerNode->m_isDirty = (m_dirtyAttributes != 0);

    if (m_dirtyAttributes & TextureSourceDirty) {
        switch (m_textureSource) {
        case QQuick3DRenderOutputProvider::TextureSource::None:
            providerNode->m_sourceType = QSSGRenderOutputProviderExtension::SourceType::None;
            // m_builtInPass is not used in this case
            break;
        case QQuick3DRenderOutputProvider::TextureSource::UserPassTexture:
            providerNode->m_sourceType = QSSGRenderOutputProviderExtension::SourceType::UserPass;
            break;
        case QQuick3DRenderOutputProvider::TextureSource::AoTexture:
            providerNode->m_sourceType = QSSGRenderOutputProviderExtension::SourceType::BuiltInPass;
            providerNode->m_builtInPass = QSSGFrameData::RenderResult::AoTexture;
            break;
        case QQuick3DRenderOutputProvider::TextureSource::DepthTexture:
            providerNode->m_sourceType = QSSGRenderOutputProviderExtension::SourceType::BuiltInPass;
            providerNode->m_builtInPass = QSSGFrameData::RenderResult::DepthTexture;
            break;
        case QQuick3DRenderOutputProvider::TextureSource::ScreenTexture:
            providerNode->m_sourceType = QSSGRenderOutputProviderExtension::SourceType::BuiltInPass;
            providerNode->m_builtInPass = QSSGFrameData::RenderResult::ScreenTexture;
            break;
        case QQuick3DRenderOutputProvider::TextureSource::NormalTexture:
            providerNode->m_sourceType = QSSGRenderOutputProviderExtension::SourceType::BuiltInPass;
            providerNode->m_builtInPass = QSSGFrameData::RenderResult::NormalTexture;
            break;
        case QQuick3DRenderOutputProvider::TextureSource::MotionVectorTexture:
            providerNode->m_sourceType = QSSGRenderOutputProviderExtension::SourceType::BuiltInPass;
            providerNode->m_builtInPass = QSSGFrameData::RenderResult::MotionVectorTexture;
            break;
        }
    }

    if (m_dirtyAttributes & UserPassTextureDirty) {
         // m_builtInPass is not used in this case
        QSSGResourceId userPassId = m_renderPass ? QQuick3DExtensionHelpers::getResourceId(*m_renderPass) : QSSGResourceId::Invalid;
        providerNode->userPassId = userPassId;
        providerNode->attachmentSelector = QSSGFrameData::AttachmentSelector(m_attachmentSelector);
    }

    m_dirtyAttributes = 0;

    return node;
}

void QQuick3DRenderOutputProvider::markAllDirty()
{
    m_dirtyAttributes = AllDirty;
    QQuick3DTextureProviderExtension::markAllDirty();
}

void QQuick3DRenderOutputProvider::markDirty(QQuick3DRenderOutputProvider::DirtyType type)
{
    if (!(m_dirtyAttributes & quint32(type))) {
        m_dirtyAttributes |= quint32(type);
        update();
    }
}

/*!
    \qmlproperty RenderOutputProvider::TextureSource textureSource
    This property holds which pass buffer texture to use.

    \value RenderOutputProvider.None No texture source is selected.
    \value RenderOutputProvider.UserPassTexture A user defined pass. If this is used to access a user defined pass.
    \value RenderOutputProvider.AoTexture The ambient occlusion texture created by QtQuick3D's AO pass.
    \value RenderOutputProvider.DepthTexture The depth texture created by QtQuick3D's Depth map pass.
    \value RenderOutputProvider.ScreenTexture The texture containing the rendered scene.
    \value RenderOutputProvider.NormalTexture The normal texture created by QtQuick3D's Normal map pass.
    \value RenderOutputProvider.MotionVectorTexture The motion vector texture created by QtQuick3D's Motion Vector pass.
 */

QQuick3DRenderOutputProvider::TextureSource QQuick3DRenderOutputProvider::textureSource() const
{
    return m_textureSource;
}

void QQuick3DRenderOutputProvider::setTextureSource(TextureSource newTextureSource)
{
    if (m_textureSource == newTextureSource)
        return;
    m_textureSource = newTextureSource;
    emit textureSourceChanged();
    markDirty(TextureSourceDirty);
}

/*!
    \qmlproperty RenderPass renderPass
    This property holds the user defined render pass to use when accessing a user defined pass texture.

    When this property is set, the \l textureSource property is automatically set to \value RenderOutputProvider.UserPassTexture.

    \sa textureSource, RenderPass
*/

QQuick3DRenderPass *QQuick3DRenderOutputProvider::renderPass() const
{
    return m_renderPass;
}

void QQuick3DRenderOutputProvider::setRenderPass(QQuick3DRenderPass *newRenderPass)
{
    if (m_renderPass == newRenderPass)
        return;

    QQuick3DObjectPrivate::attachWatcher(this, &QQuick3DRenderOutputProvider::setRenderPass, newRenderPass, m_renderPass);

    m_renderPass = newRenderPass;

    setTextureSource(m_renderPass ? TextureSource::UserPassTexture : TextureSource::None);

    markDirty(UserPassTextureDirty);

    emit renderPassChanged();
}

/*!
    \qmlproperty RenderOutputProvider::AttachmentSelector attachmentSelector
    This property holds which attachment to use when accessing a user defined pass texture.

    There are multiple attachments possible when using a user defined render pass,
    starting from \value RenderOutputProvider.Attachment0 up to \value RenderOutputProvider.Attachment3.
*/

QQuick3DRenderOutputProvider::AttachmentSelector QQuick3DRenderOutputProvider::attachmentSelector() const
{
    return m_attachmentSelector;
}

void QQuick3DRenderOutputProvider::setAttachmentSelector(AttachmentSelector newAttachmentSelector)
{
    if (m_attachmentSelector == newAttachmentSelector)
        return;
    m_attachmentSelector = newAttachmentSelector;
    emit attachmentSelectorChanged();
}

QT_END_NAMESPACE
