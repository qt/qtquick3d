/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
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

#include <QtQuick3DRuntimeRender/private/qssgoffscreenrendermanager_p.h>
#include <QtQuick3DRender/private/qssgrenderbasetypes_p.h>
#include <QtQuick3DRender/private/qssgrenderframebuffer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderresourcemanager_p.h>
#include <QtQuick3DRender/private/qssgrendercontext_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercontextcore_p.h>
#include "qssgoffscreenrenderkey_p.h"
#include "qssgrenderrenderlist_p.h"
#include <QtQuick3DRuntimeRender/private/qssgrenderresourcetexture2d_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderresourcebufferobjects_p.h>
#include "qssgrendererutil_p.h"
#include <QtQuick3DRender/private/qssgrendertexture2d_p.h>

#include <limits>

QT_BEGIN_NAMESPACE

uint qHash(const QSSGOffscreenRendererKey &key)
{
    if (key.isString())
        return qHash(key.string);
    else
        return qHash(reinterpret_cast<size_t>(key.key));
}

struct QSSGRendererData : QSSGOffscreenRenderResult
{
    QSSGRef<QSSGResourceManager> resourceManager;
    quint32 frameCount = std::numeric_limits<quint32>::max();
    bool rendering = false;

    explicit QSSGRendererData(const QSSGRef<QSSGResourceManager> &inResourceManager)
        : resourceManager(inResourceManager)
    {
    }
};

struct QSSGScopedRenderDataRenderMarker
{
    QSSGRendererData &data;
    explicit QSSGScopedRenderDataRenderMarker(QSSGRendererData &d) : data(d)
    {
        Q_ASSERT(data.rendering == false);
        data.rendering = true;
    }
    ~QSSGScopedRenderDataRenderMarker() { data.rendering = false; }
};

struct QSSGRenderDataReleaser
{
    // TODO:
    QSSGRef<QSSGRendererData> dataPtr;
};

struct QSSGOffscreenRunnable : public QSSGRenderTask
{
    QSSGOffscreenRenderManager &m_renderManager;
    QSSGRendererData &m_data;
    QSSGOffscreenRendererEnvironment m_desiredEnvironment;
    QSSGOffscreenRunnable(QSSGOffscreenRenderManager &rm, QSSGRendererData &data, const QSSGOffscreenRendererEnvironment &env)
        : m_renderManager(rm), m_data(data), m_desiredEnvironment(env)
    {
    }
    void run() override;
};

void QSSGOffscreenRunnable::run()
{
    m_renderManager.renderItem(m_data, m_desiredEnvironment);
}


QSSGRef<QSSGOffscreenRenderManager> QSSGOffscreenRenderManager::createOffscreenRenderManager(
        const QSSGRef<QSSGResourceManager> &inManager,
        QSSGRenderContextInterface *inContext)
{
    return QSSGRef<QSSGOffscreenRenderManager>(new QSSGOffscreenRenderManager(inManager, inContext));
}

QSSGOffscreenRendererInterface::~QSSGOffscreenRendererInterface() = default;

QSSGOffscreenRenderManager::QSSGOffscreenRenderManager(const QSSGRef<QSSGResourceManager> &inManager, QSSGRenderContextInterface *inContext)
    : m_context(inContext), m_resourceManager(inManager), m_frameCount(0)
{
}

QSSGOffscreenRenderManager::~QSSGOffscreenRenderManager() = default;

QSSGOption<bool> QSSGOffscreenRenderManager::maybeRegisterOffscreenRenderer(const QSSGOffscreenRendererKey &inKey, const QSSGRef<QSSGOffscreenRendererInterface> &inRenderer)
{
    TRendererMap::iterator theIter = m_renderers.find(inKey);
    if (theIter != m_renderers.end()) {
        QSSGRendererData &theData = theIter.value();
        if (theData.renderer != inRenderer) {
            if (inKey.isString()) {
                qCCritical(INVALID_OPERATION,
                           "Different renderers registered under same key: %s",
                           inKey.string.toLatin1().constData());
            }
            Q_ASSERT(false);
            return QSSGEmpty();
        }
        return false;
    }
    registerOffscreenRenderer(inKey, inRenderer);
    return true;
}

void QSSGOffscreenRenderManager::registerOffscreenRenderer(const QSSGOffscreenRendererKey &inKey, const QSSGRef<QSSGOffscreenRendererInterface> &inRenderer)
{
    auto inserter = m_renderers.find(inKey);
    if (inserter == m_renderers.end())
        inserter = m_renderers.insert(inKey, QSSGRendererData(m_resourceManager));
    QSSGRendererData &theData = inserter.value();
    theData.renderer = inRenderer;
}

bool QSSGOffscreenRenderManager::hasOffscreenRenderer(const QSSGOffscreenRendererKey &inKey)
{
    return m_renderers.find(inKey) != m_renderers.end();
}

QSSGRef<QSSGOffscreenRendererInterface> QSSGOffscreenRenderManager::getOffscreenRenderer(const QSSGOffscreenRendererKey &inKey)
{
    const auto it = m_renderers.constFind(inKey);
    return (it != m_renderers.cend()) ? it.value().renderer : nullptr;
}

void QSSGOffscreenRenderManager::releaseOffscreenRenderer(const QSSGOffscreenRendererKey &inKey) { m_renderers.remove(inKey); }

void QSSGOffscreenRenderManager::renderItem(QSSGRendererData &theData, QSSGOffscreenRendererEnvironment theDesiredEnvironment)
{
    auto theContext = m_resourceManager->getRenderContext();
    QVector2D thePresScaleFactor = m_context->presentationScaleFactor();
    QSSGOffscreenRendererEnvironment theOriginalDesiredEnvironment(theDesiredEnvironment);
    // Ensure that our overall render context comes back no matter what the client does.
    QSSGRenderContextScopedProperty<QVector4D> __clearColor(*theContext,
                                                              &QSSGRenderContext::clearColor,
                                                              &QSSGRenderContext::setClearColor,
                                                              QVector4D(0, 0, 0, 0));
    QSSGRenderContextScopedProperty<bool> __scissorEnabled(*theContext,
                                                             &QSSGRenderContext::isScissorTestEnabled,
                                                             &QSSGRenderContext::setScissorTestEnabled,
                                                             false);
    QSSGRenderContextScopedProperty<QRect> __scissorRect(*theContext,
                                                           &QSSGRenderContext::scissorRect,
                                                           &QSSGRenderContext::setScissorRect);
    QSSGRenderContextScopedProperty<QRect> __viewportRect(*theContext,
                                                            &QSSGRenderContext::viewport,
                                                            &QSSGRenderContext::setViewport);
    QSSGRenderContextScopedProperty<bool> __depthWrite(*theContext,
                                                         &QSSGRenderContext::isDepthWriteEnabled,
                                                         &QSSGRenderContext::setDepthWriteEnabled,
                                                         false);
    QSSGRenderContextScopedProperty<QSSGRenderBoolOp> __depthFunction(*theContext,
                                                                          &QSSGRenderContext::depthFunction,
                                                                          &QSSGRenderContext::setDepthFunction,
                                                                          QSSGRenderBoolOp::Less);
    QSSGRenderContextScopedProperty<bool> __blendEnabled(*theContext,
                                                           &QSSGRenderContext::isBlendingEnabled,
                                                           &QSSGRenderContext::setBlendingEnabled,
                                                           false);
    QSSGRenderContextScopedProperty<QSSGRenderBlendFunctionArgument> __blendFunction(*theContext,
                                                                                         &QSSGRenderContext::blendFunction,
                                                                                         &QSSGRenderContext::setBlendFunction,
                                                                                         QSSGRenderBlendFunctionArgument());
    QSSGRenderContextScopedProperty<QSSGRenderBlendEquationArgument> __blendEquation(*theContext,
                                                                                         &QSSGRenderContext::blendEquation,
                                                                                         &QSSGRenderContext::setBlendEquation,
                                                                                         QSSGRenderBlendEquationArgument());
    QSSGRenderContextScopedProperty<QSSGRef<QSSGRenderFrameBuffer>> __rendertarget(*theContext,
                                                                                         &QSSGRenderContext::renderTarget,
                                                                                         &QSSGRenderContext::setRenderTarget);

    qint32 theSampleCount = 1;
    bool isMultisamplePass = false;
    if (theDesiredEnvironment.msaaMode != QSSGRenderLayer::AAMode::NoAA) {
        switch (theDesiredEnvironment.msaaMode) {
        case QSSGRenderLayer::AAMode::SSAA:
            theSampleCount = 1;
            isMultisamplePass = true;
            break;
        case QSSGRenderLayer::AAMode::X2:
            theSampleCount = 2;
            isMultisamplePass = true;
            break;
        case QSSGRenderLayer::AAMode::X4:
            theSampleCount = 4;
            isMultisamplePass = true;
            break;
        case QSSGRenderLayer::AAMode::X8:
            theSampleCount = 8;
            isMultisamplePass = true;
            break;
        default:
            Q_ASSERT(false);
            break;
        };

        // adjust render size for SSAA
        if (theDesiredEnvironment.msaaMode == QSSGRenderLayer::AAMode::SSAA) {
            QSSGRendererUtil::getSSAARenderSize(theOriginalDesiredEnvironment.width,
                                                  theOriginalDesiredEnvironment.height,
                                                  theDesiredEnvironment.width,
                                                  theDesiredEnvironment.height);
        }
    }
    QSSGResourceFrameBuffer theFrameBuffer(m_resourceManager);
    theFrameBuffer.ensureFrameBuffer();
    auto &renderTargetTexture = theData.texture;
    QSSGRenderTextureTargetType fboAttachmentType = QSSGRenderTextureTargetType::Texture2D;
    if (isMultisamplePass) {
        renderTargetTexture = nullptr;
        if (theSampleCount > 1)
            fboAttachmentType = QSSGRenderTextureTargetType::Texture2D_MS;
    }

    QSSGResourceTexture2D renderColorTexture(m_resourceManager, renderTargetTexture);

    QSSGResourceTexture2D renderDepthStencilTexture(m_resourceManager);

    if (theSampleCount > 1)
        m_context->renderContext()->setMultisampleEnabled(true);

    QSSGRenderTextureFormat theDepthStencilTextureFormat(QSSGRenderTextureFormat::Unknown);
    QSSGRenderFrameBufferAttachment theAttachmentLocation(QSSGRenderFrameBufferAttachment::Unknown);
    if (theDesiredEnvironment.stencil) {
        theDepthStencilTextureFormat = QSSGRenderTextureFormat::Depth24Stencil8;
        theAttachmentLocation = QSSGRenderFrameBufferAttachment::DepthStencil;
    } else if (theDesiredEnvironment.depth != QSSGOffscreenRendererDepthValues::NoDepthBuffer) {
        theAttachmentLocation = QSSGRenderFrameBufferAttachment::Depth;
        switch (theDesiredEnvironment.depth) {
        case QSSGOffscreenRendererDepthValues::Depth16:
            theDepthStencilTextureFormat = QSSGRenderTextureFormat::Depth16;
            break;
        case QSSGOffscreenRendererDepthValues::Depth24:
            theDepthStencilTextureFormat = QSSGRenderTextureFormat::Depth24;
            break;
        case QSSGOffscreenRendererDepthValues::Depth32:
            theDepthStencilTextureFormat = QSSGRenderTextureFormat::Depth32;
            break;
        default:
            theAttachmentLocation = QSSGRenderFrameBufferAttachment::Unknown;
            theDepthStencilTextureFormat = QSSGRenderTextureFormat::Unknown;
            break;
        }
    }
    renderColorTexture.ensureTexture(theDesiredEnvironment.width,
                                     theDesiredEnvironment.height,
                                     theDesiredEnvironment.format,
                                     theSampleCount);
    theFrameBuffer->attach(QSSGRenderFrameBufferAttachment::Color0, renderColorTexture.getTexture(), fboAttachmentType);

    if (theDepthStencilTextureFormat != QSSGRenderTextureFormat::Unknown) {
        renderDepthStencilTexture.ensureTexture(theDesiredEnvironment.width,
                                                theDesiredEnvironment.height,
                                                theDepthStencilTextureFormat,
                                                theSampleCount);
        theFrameBuffer->attach(theAttachmentLocation, renderDepthStencilTexture.getTexture(), fboAttachmentType);
    }
    // IsComplete check takes a really long time so I am not going to worry about it for now.

    theContext->setRenderTarget(theFrameBuffer);
    theContext->setViewport(QRect(0, 0, theDesiredEnvironment.width, theDesiredEnvironment.height));
    theContext->setScissorTestEnabled(false);

    theContext->setBlendingEnabled(false);
    theData.renderer->render(theDesiredEnvironment, *theContext, thePresScaleFactor, this);

    if (theSampleCount > 1) {
        QSSGResourceTexture2D theResult(m_resourceManager, theData.texture);

        if (theDesiredEnvironment.msaaMode != QSSGRenderLayer::AAMode::SSAA) {
            // Have to downsample the FBO.
            QSSGRendererUtil::resolveMutisampleFBOColorOnly(m_resourceManager,
                                                              theResult,
                                                              *m_context->renderContext(),
                                                              theDesiredEnvironment.width,
                                                              theDesiredEnvironment.height,
                                                              theDesiredEnvironment.format,
                                                              theFrameBuffer);

            m_context->renderContext()->setMultisampleEnabled(false);
        } else {
            // Resolve the FBO to the layer texture
            QSSGRendererUtil::resolveSSAAFBOColorOnly(m_resourceManager,
                                                        theResult,
                                                        theOriginalDesiredEnvironment.width,
                                                        theOriginalDesiredEnvironment.height,
                                                        *m_context->renderContext(),
                                                        theDesiredEnvironment.width,
                                                        theDesiredEnvironment.height,
                                                        theDesiredEnvironment.format,
                                                        theFrameBuffer);
        }

        Q_ASSERT(theData.texture == theResult.getTexture());
        theResult.forgetTexture();
    } else {
        renderColorTexture.forgetTexture();
    }
    theFrameBuffer->attach(QSSGRenderFrameBufferAttachment::Color0, QSSGRenderTextureOrRenderBuffer(), fboAttachmentType);
    if (theAttachmentLocation != QSSGRenderFrameBufferAttachment::Unknown)
        theFrameBuffer->attach(theAttachmentLocation, QSSGRenderTextureOrRenderBuffer(), fboAttachmentType);
}

QSSGOffscreenRenderResult QSSGOffscreenRenderManager::getRenderedItem(const QSSGOffscreenRendererKey &inKey)
{
    TRendererMap::iterator theRenderer = m_renderers.find(inKey);
    QVector2D thePresScaleFactor = m_context->presentationScaleFactor();
    if (theRenderer != m_renderers.end() && theRenderer.value().rendering == false) {
        QSSGRendererData &theData = theRenderer.value();
        QSSGScopedRenderDataRenderMarker __renderMarker(theData);

        bool renderedThisFrame = theData.texture && theData.frameCount == m_frameCount;
        theData.frameCount = m_frameCount;
        // Two different quick-out pathways.
        if (renderedThisFrame)
            return theData;

        QSSGOffscreenRendererEnvironment theDesiredEnvironment = theData.renderer->getDesiredEnvironment(thePresScaleFactor);
        // Ensure we get a valid width and height
        theDesiredEnvironment.width = QSSGRendererUtil::nextMultipleOf4(theDesiredEnvironment.width);
        theDesiredEnvironment.height = QSSGRendererUtil::nextMultipleOf4(theDesiredEnvironment.height);
        if (theDesiredEnvironment.width == 0 || theDesiredEnvironment.height == 0) {
            return QSSGOffscreenRenderResult();
        }

        QRect theViewport(0, 0, theDesiredEnvironment.width, theDesiredEnvironment.height);
        const auto &theRenderList = m_context->renderList();
        const auto &theContext = m_context->renderContext();
        // This happens here because if there are any fancy render steps
        QSSGRenderListScopedProperty<bool> scissor(*theRenderList,
                                                     &QSSGRenderList::isScissorTestEnabled,
                                                     &QSSGRenderList::setScissorTestEnabled,
                                                     false);
        QSSGRenderListScopedProperty<QRect> viewport(*theRenderList,
                                                       &QSSGRenderList::getViewport,
                                                       &QSSGRenderList::setViewport,
                                                       theViewport);
        // Some plugins don't use the render list so they need the actual gl context setup.
        QSSGRenderContextScopedProperty<bool> scissorEnabled(*theContext,
                                                               &QSSGRenderContext::isScissorTestEnabled,
                                                               &QSSGRenderContext::setScissorTestEnabled,
                                                               false);
        QSSGRenderContextScopedProperty<QRect> __viewportRect(*theContext,
                                                                &QSSGRenderContext::viewport,
                                                                &QSSGRenderContext::setViewport,
                                                                theViewport);

        quint32 taskId = m_context->renderList()->addRenderTask(
                    QSSGRef<QSSGOffscreenRunnable>(new QSSGOffscreenRunnable(*this, theData, theDesiredEnvironment)));

        QSSGOffscreenRenderFlags theFlags = theData.renderer->needsRender(theDesiredEnvironment, thePresScaleFactor, this);
        theData.hasTransparency = theFlags.hasTransparency;
        theData.hasChangedSinceLastFrame = theFlags.hasChangedSinceLastFrame;
        if (theData.texture) {
            // Quick-out if the renderer doesn't need to render itself.
            if (theData.hasChangedSinceLastFrame == false) {
                m_context->renderList()->discardRenderTask(taskId);
                return theData;
            }
        } else
            theData.hasChangedSinceLastFrame = true;

        // Release existing texture if it doesn't match latest environment request.
        if (theData.texture) {
            QSSGTextureDetails theDetails = theData.texture->textureDetails();
            if (theDesiredEnvironment.width != theDetails.width || theDesiredEnvironment.height != theDetails.height
                    || theDesiredEnvironment.format != theDetails.format) {
                m_resourceManager->release(theData.texture);
                theData.texture = nullptr;
            }
        }

        if (theData.texture == nullptr)
            theData.texture = m_resourceManager->allocateTexture2D(theDesiredEnvironment.width,
                                                                   theDesiredEnvironment.height,
                                                                   theDesiredEnvironment.format);

        // Add the node to the graph and get on with it.

        return theData;
    }
    return QSSGOffscreenRenderResult();
}

void QSSGOffscreenRenderManager::beginFrame()
{ /* TODO: m_PerFrameAllocator.reset();*/
}

void QSSGOffscreenRenderManager::endFrame() { ++m_frameCount; }

QT_END_NAMESPACE

