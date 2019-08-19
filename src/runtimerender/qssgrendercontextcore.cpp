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

#include "qssgrendercontextcore_p.h"
#include <QtQuick3DRuntimeRender/private/qssgrendernode_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderbuffermanager_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderresourcemanager_p.h>
#include <QtQuick3DRender/private/qssgrendercontext_p.h>
#include <QtQuick3DRuntimeRender/private/qssgoffscreenrendermanager_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderinputstreamfactory_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercache_p.h>
#include <QtQuick3DRender/private/qssgrenderframebuffer_p.h>
#include <QtQuick3DRender/private/qssgrenderrenderbuffer_p.h>
#include <QtQuick3DRender/private/qssgrendertexture2d_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercamera_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderthreadpool_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderimagebatchloader_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderdynamicobjectsystem_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercustommaterialsystem_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderpixelgraphicsrenderer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderrenderlist_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderpathmanager_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercodegeneratorv2_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderdefaultmaterialshadergenerator_p.h>
#include <QtQuick3DRuntimeRender/private/qssgperframeallocator_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendererimpl_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendererutil_p.h>

QT_BEGIN_NAMESPACE

QSSGRenderContextInterface::~QSSGRenderContextInterface() = default;

QSSGRenderContextInterface::QSSGRenderContextInterface(const QSSGRef<QSSGRenderContext> &ctx, const QString &inApplicationDirectory)
    : m_renderContext(ctx)
    , m_inputStreamFactory(new QSSGInputStreamFactory)
    , m_bufferManager(new QSSGBufferManager(ctx, m_inputStreamFactory, &m_perfTimer))
    , m_resourceManager(new QSSGResourceManager(ctx))
    , m_offscreenRenderManager(QSSGOffscreenRenderManager::createOffscreenRenderManager(m_resourceManager, this))
    , m_renderer(QSSGRendererInterface::createRenderer(this))
    , m_dynamicObjectSystem(new QSSGDynamicObjectSystem(this))
    , m_effectSystem(new QSSGEffectSystem(this))
    , m_shaderCache(QSSGShaderCache::createShaderCache(ctx, m_inputStreamFactory, &m_perfTimer))
    , m_threadPool(QSSGAbstractThreadPool::createThreadPool(4))
    , m_customMaterialSystem(new QSSGMaterialSystem(this))
    , m_pixelGraphicsRenderer(QSSGPixelGraphicsRendererInterface::createRenderer(this))
    , m_pathManager(QSSGPathManagerInterface::createPathManager(this))
    , m_shaderProgramGenerator(QSSGShaderProgramGeneratorInterface::createProgramGenerator(this))
    , m_defaultMaterialShaderGenerator(QSSGDefaultMaterialShaderGeneratorInterface::createDefaultMaterialShaderGenerator(this))
    , m_customMaterialShaderGenerator(QSSGMaterialShaderGeneratorInterface::createCustomMaterialShaderGenerator(this))
    , m_renderList(QSSGRenderList::createRenderList())
{
    if (!inApplicationDirectory.isEmpty())
        m_inputStreamFactory->addSearchDirectory(inApplicationDirectory);

    const_cast<QSSGRef<IImageBatchLoader> &>(m_imageBatchLoader) = IImageBatchLoader::createBatchLoader(m_inputStreamFactory, m_bufferManager, m_threadPool, &m_perfTimer);
    m_customMaterialSystem->setRenderContextInterface(this);


    const char *versionString = nullptr;
    switch (ctx->renderContextType()) {
    case QSSGRenderContextType::GLES2:
        versionString = "gles2";
        break;
    case QSSGRenderContextType::GL2:
        versionString = "gl2";
        break;
    case QSSGRenderContextType::GLES3:
        versionString = "gles3";
        break;
    case QSSGRenderContextType::GL3:
        versionString = "gl3";
        break;
    case QSSGRenderContextType::GLES3PLUS:
        versionString = "gles3x";
        break;
    case QSSGRenderContextType::GL4:
        versionString = "gl4";
        break;
    default:
        Q_ASSERT(false);
        break;
    }

    dynamicObjectSystem()->setShaderCodeLibraryVersion(versionString);
#if defined(QSSG_SHADER_PLATFORM_LIBRARY_DIR)
    const QString platformDirectory;
#if defined(_WIN32)
    platformDirectory = QStringLiteral("res/platform/win");
#elif defined(_LINUX)
    platformDirectory = QStringLiteral("res/platform/linux");
#elif defined(_MACOSX)
    platformDirectory = QStringLiteral("res/platform/macos");
#endif
    GetDynamicObjectSystem().setShaderCodeLibraryPlatformDirectory(platformDirectory);
#endif
}

Q_GLOBAL_STATIC(QVector<QSSGRenderContextInterface::QSSGRenderContextInterfacePtr>, g_renderContexts)

void QSSGRenderContextInterface::releaseRenderContextInterface(quintptr wid)
{
    auto it = g_renderContexts->cbegin();
    const auto end = g_renderContexts->cend();
    for (; it != end; ++it) {
        if (it->m_wid == wid)
            break;
    }

    if (it != end)
        g_renderContexts->remove(int(end - it));
}

QSSGRenderContextInterface::QSSGRenderContextInterfacePtr QSSGRenderContextInterface::getRenderContextInterface(const QSSGRef<QSSGRenderContext> &ctx, const QString &inApplicationDirectory, quintptr wid)
{
    auto it = g_renderContexts->cbegin();
    const auto end = g_renderContexts->cend();
    for (; it != end; ++it) {
        if (it->m_wid == wid)
            break;
    }

    if (it != end)
        return *it;

    QSSGRenderContextInterfacePtr ptr { new QSSGRenderContextInterface(ctx, inApplicationDirectory), wid };
    g_renderContexts->push_back(ptr);

    return ptr;
}

QSSGRenderContextInterface::QSSGRenderContextInterfacePtr QSSGRenderContextInterface::getRenderContextInterface(quintptr wid)
{
    auto it = g_renderContexts->cbegin();
    const auto end = g_renderContexts->cend();
    for (; it != end; ++it) {
        if (it->m_wid == wid)
            break;
    }

    if (it != end)
        return *it;

    return QSSGRenderContextInterfacePtr();
}

const QSSGRef<QSSGRendererInterface> &QSSGRenderContextInterface::renderer() const { return m_renderer; }

const QSSGRef<QSSGBufferManager> &QSSGRenderContextInterface::bufferManager() const { return m_bufferManager; }

const QSSGRef<QSSGResourceManager> &QSSGRenderContextInterface::resourceManager() const { return m_resourceManager; }

const QSSGRef<QSSGRenderContext> &QSSGRenderContextInterface::renderContext() const { return m_renderContext; }

const QSSGRef<QSSGOffscreenRenderManager> &QSSGRenderContextInterface::offscreenRenderManager() const
{
    return m_offscreenRenderManager;
}

const QSSGRef<QSSGInputStreamFactory> &QSSGRenderContextInterface::inputStreamFactory() const { return m_inputStreamFactory; }

const QSSGRef<QSSGEffectSystem> &QSSGRenderContextInterface::effectSystem() const { return m_effectSystem; }

const QSSGRef<QSSGShaderCache> &QSSGRenderContextInterface::shaderCache() const { return m_shaderCache; }

const QSSGRef<QSSGAbstractThreadPool> &QSSGRenderContextInterface::threadPool() const { return m_threadPool; }

const QSSGRef<IImageBatchLoader> &QSSGRenderContextInterface::imageBatchLoader() const { return m_imageBatchLoader; }

const QSSGRef<QSSGDynamicObjectSystem> &QSSGRenderContextInterface::dynamicObjectSystem() const { return m_dynamicObjectSystem; }

const QSSGRef<QSSGMaterialSystem> &QSSGRenderContextInterface::customMaterialSystem() const { return m_customMaterialSystem; }

const QSSGRef<QSSGPixelGraphicsRendererInterface> &QSSGRenderContextInterface::pixelGraphicsRenderer() const
{
    return m_pixelGraphicsRenderer;
}

const QSSGRef<QSSGRenderList> &QSSGRenderContextInterface::renderList() const { return m_renderList; }

const QSSGRef<QSSGPathManagerInterface> &QSSGRenderContextInterface::pathManager() const { return m_pathManager; }

const QSSGRef<QSSGShaderProgramGeneratorInterface> &QSSGRenderContextInterface::shaderProgramGenerator() const
{
    return m_shaderProgramGenerator;
}

const QSSGRef<QSSGDefaultMaterialShaderGeneratorInterface> &QSSGRenderContextInterface::defaultMaterialShaderGenerator() const
{
    return m_defaultMaterialShaderGenerator;
}

const QSSGRef<QSSGMaterialShaderGeneratorInterface> &QSSGRenderContextInterface::customMaterialShaderGenerator() const
{
    return m_customMaterialShaderGenerator;
}

QSSGRef<QSSGRendererImpl> QSSGRenderContextInterface::renderWidgetContext()
{
    return static_cast<QSSGRendererImpl *>(m_renderer.get());
}

QPair<QRect, QRect> QSSGRenderContextInterface::getPresentationViewportAndOuterViewport() const
{
    QSize thePresentationDimensions(m_presentationDimensions);
    QRect theOuterViewport(contextViewport());
    // Calculate the presentation viewport perhaps with the window width and height swapped.
    return QPair<QRect, QRect>(presentationViewport(theOuterViewport, m_scaleMode, thePresentationDimensions), theOuterViewport);
}

QVector2D QSSGRenderContextInterface::mousePickViewport() const
{
    return QVector2D((float)m_windowDimensions.width(), (float)m_windowDimensions.height());
}

QRect QSSGRenderContextInterface::contextViewport() const
{
    QRect retval;
    if (m_viewport.hasValue())
        retval = *m_viewport;
    else
        retval = QRect(0, 0, m_windowDimensions.width(), m_windowDimensions.height());

    return retval;
}

QVector2D QSSGRenderContextInterface::mousePickMouseCoords(const QVector2D &inMouseCoords) const
{
    return inMouseCoords;
}

QRect QSSGRenderContextInterface::presentationViewport(const QRect &inViewerViewport, ScaleModes inScaleToFit, const QSize &inPresDimensions) const
{
    const qint32 viewerViewportWidth = inViewerViewport.width();
    const qint32 viewerViewportHeight = inViewerViewport.height();
    qint32 width, height, x, y;
    if (inPresDimensions.width() == 0 || inPresDimensions.height() == 0)
        return QRect(0, 0, 0, 0);
    // Setup presentation viewport.  This may or may not match the physical viewport that we
    // want to setup.
    // Avoiding scaling keeps things as sharp as possible.
    if (inScaleToFit == ScaleModes::ExactSize) {
        width = inPresDimensions.width();
        height = inPresDimensions.height();
        x = (viewerViewportWidth - (qint32)inPresDimensions.width()) / 2;
        y = (viewerViewportHeight - (qint32)inPresDimensions.height()) / 2;
    } else if (inScaleToFit == ScaleModes::ScaleToFit || inScaleToFit == ScaleModes::FitSelected) {
        // Scale down in such a way to preserve aspect ratio.
        float screenAspect = (float)viewerViewportWidth / (float)viewerViewportHeight;
        float thePresentationAspect = (float)inPresDimensions.width() / (float)inPresDimensions.height();
        if (screenAspect >= thePresentationAspect) {
            // if the screen height is the limiting factor
            y = 0;
            height = viewerViewportHeight;
            width = (qint32)(thePresentationAspect * height);
            x = (viewerViewportWidth - width) / 2;
        } else {
            x = 0;
            width = viewerViewportWidth;
            height = (qint32)(width / thePresentationAspect);
            y = (viewerViewportHeight - height) / 2;
        }
    } else {
        // Setup the viewport for everything and let the presentations figure it out.
        x = 0;
        y = 0;
        width = viewerViewportWidth;
        height = viewerViewportHeight;
    }
    x += inViewerViewport.x();
    y += inViewerViewport.y();
    return { x, y, width, height };
}

void QSSGRenderContextInterface::dumpGpuProfilerStats()
{
    m_renderer->dumpGpuProfilerStats();
}

void QSSGRenderContextInterface::beginFrame()
{
    m_preRenderPresentationDimensions = m_presentationDimensions;
    QSize thePresentationDimensions(m_preRenderPresentationDimensions);
    QRect theContextViewport(contextViewport());
    m_perFrameAllocator.reset();
    QSSGRenderList &theRenderList(*m_renderList);
    theRenderList.beginFrame();
    if (m_viewport.hasValue()) {
        theRenderList.setScissorTestEnabled(true);
        theRenderList.setScissorRect(theContextViewport);
    } else {
        theRenderList.setScissorTestEnabled(false);
    }
    QPair<QRect, QRect> thePresViewportAndOuterViewport = getPresentationViewportAndOuterViewport();
    QRect theOuterViewport = thePresViewportAndOuterViewport.second;
    // Calculate the presentation viewport perhaps with the window width and height swapped.
    QRect thePresentationViewport = thePresViewportAndOuterViewport.first;
    m_presentationViewport = thePresentationViewport;
    m_presentationScale = QVector2D((float)thePresentationViewport.width() / (float)thePresentationDimensions.width(),
                                    (float)thePresentationViewport.height() / (float)thePresentationDimensions.height());
    QSize fboDimensions;
    if (thePresentationViewport.width() > 0 && thePresentationViewport.height() > 0) {
        m_presentationDimensions = QSize(thePresentationViewport.width(), thePresentationViewport.height());
        m_renderList->setViewport(thePresentationViewport);
        if (thePresentationViewport.x() || thePresentationViewport.y()
                || thePresentationViewport.width() != (qint32)theOuterViewport.width()
                || thePresentationViewport.height() != (qint32)theOuterViewport.height()) {
            m_renderList->setScissorRect(thePresentationViewport);
            m_renderList->setScissorTestEnabled(true);
        }
    }

    m_beginFrameResult = BeginFrameResult(false,
                                          m_presentationDimensions,
                                          m_renderList->isScissorTestEnabled(),
                                          m_renderList->getScissor(),
                                          m_renderList->getViewport(),
                                          fboDimensions);

    m_renderer->beginFrame();
    m_offscreenRenderManager->beginFrame();
    m_imageBatchLoader->beginFrame();
}

void QSSGRenderContextInterface::setupRenderTarget()
{
    QRect theContextViewport(contextViewport());
    if (m_viewport.hasValue()) {
        m_renderContext->setScissorTestEnabled(true);
        m_renderContext->setScissorRect(theContextViewport);
    } else {
        m_renderContext->setScissorTestEnabled(false);
    }
    {
        QVector4D theClearColor;
        if (m_matteColor.hasValue())
            theClearColor = m_matteColor;
        else if (m_sceneColor.hasValue())
            theClearColor = m_sceneColor;
        else
            theClearColor = QVector4D(0.0, 0.0, 0.0, 0.0);
        if (m_sceneColor.hasValue() && m_sceneColor.getValue().w() != 0.0f) {
            m_renderContext->setClearColor(theClearColor);
            m_renderContext->clear(QSSGRenderClearValues::Color);
        }
    }
    bool renderOffscreen = m_beginFrameResult.renderOffscreen;
    m_renderContext->setViewport(m_beginFrameResult.viewport);
    m_renderContext->setScissorRect(m_beginFrameResult.scissorRect);
    m_renderContext->setScissorTestEnabled(m_beginFrameResult.scissorTestEnabled);

    if (m_presentationViewport.width() > 0 && m_presentationViewport.height() > 0) {
        if (renderOffscreen == false) {
            if (m_rotationFbo != nullptr) {
                m_resourceManager->release(m_rotationFbo);
                m_resourceManager->release(m_rotationTexture);
                m_resourceManager->release(m_rotationDepthBuffer);
                m_rotationFbo = nullptr;
                m_rotationTexture = nullptr;
                m_rotationDepthBuffer = nullptr;
            }
            if (m_sceneColor.hasValue() && m_sceneColor.getValue().w() != 0.0f) {
                m_renderContext->setClearColor(m_sceneColor);
                m_renderContext->clear(QSSGRenderClearValues::Color);
            }
        } else {
            qint32 imageWidth = m_beginFrameResult.fboDimensions.width();
            qint32 imageHeight = m_beginFrameResult.fboDimensions.height();
            QSSGRenderTextureFormat theColorBufferFormat = QSSGRenderTextureFormat::RGBA8;
            QSSGRenderRenderBufferFormat theDepthBufferFormat = QSSGRenderRenderBufferFormat::Depth16;
            m_contextRenderTarget = m_renderContext->renderTarget();
            if (m_rotationFbo == nullptr) {
                m_rotationFbo = m_resourceManager->allocateFrameBuffer();
                m_rotationTexture = m_resourceManager->allocateTexture2D(imageWidth, imageHeight, theColorBufferFormat);
                m_rotationDepthBuffer = m_resourceManager->allocateRenderBuffer(imageWidth, imageHeight, theDepthBufferFormat);
                m_rotationFbo->attach(QSSGRenderFrameBufferAttachment::Color0, m_rotationTexture);
                m_rotationFbo->attach(QSSGRenderFrameBufferAttachment::Depth, m_rotationDepthBuffer);
            } else {
                QSSGTextureDetails theDetails = m_rotationTexture->textureDetails();
                if (theDetails.width != imageWidth || theDetails.height != imageHeight) {
                    m_rotationTexture->setTextureData(QSSGByteView(), 0, imageWidth, imageHeight, theColorBufferFormat);
                    m_rotationDepthBuffer->setSize(QSize(imageWidth, imageHeight));
                }
            }
            m_renderContext->setRenderTarget(m_rotationFbo);
            if (m_sceneColor.hasValue()) {
                m_renderContext->setClearColor(m_sceneColor);
                m_renderContext->clear(QSSGRenderClearValues::Color);
            }
        }
    }
}

void QSSGRenderContextInterface::runRenderTasks()
{
    m_renderList->runRenderTasks();
    setupRenderTarget();
}

void QSSGRenderContextInterface::teardownRenderTarget()
{
    if (m_rotationFbo) {
        ScaleModes theScaleToFit = m_scaleMode;
        QRect theOuterViewport(contextViewport());
        m_renderContext->setRenderTarget(m_contextRenderTarget);
        QSize thePresentationDimensions = currentPresentationDimensions();
        m_renderPresentationDimensions = thePresentationDimensions;
        // Calculate the presentation viewport perhaps with the presentation width and height
        // swapped.
        QRect thePresentationViewport = presentationViewport(theOuterViewport, theScaleToFit, thePresentationDimensions);
        QSSGRenderCamera theCamera;
        float z = theCamera.rotation.z();
        TORAD(z);
        theCamera.rotation.setZ(z);
        theCamera.markDirty(QSSGRenderCamera::TransformDirtyFlag::TransformIsDirty);
        theCamera.flags.setFlag(QSSGRenderCamera::Flag::Orthographic);
        m_renderContext->setViewport(thePresentationViewport);
        theCamera.calculateGlobalVariables(QRectF(0, 0, thePresentationViewport.width(), thePresentationViewport.height()));
        QMatrix4x4 theVP;
        theCamera.calculateViewProjectionMatrix(theVP);
        QSSGRenderNode theTempNode;
        theTempNode.calculateGlobalVariables();
        QMatrix4x4 theMVP;
        QMatrix3x3 theNormalMat;
        theTempNode.calculateMVPAndNormalMatrix(theVP, theMVP, theNormalMat);
        m_renderContext->setCullingEnabled(false);
        m_renderContext->setBlendingEnabled(false);
        m_renderContext->setDepthTestEnabled(false);
        m_renderer->renderQuad(QVector2D((float)m_presentationViewport.width(), (float)m_presentationViewport.height()),
                               theMVP,
                               *m_rotationTexture);
    }
}

void QSSGRenderContextInterface::endFrame()
{
    teardownRenderTarget();
    m_imageBatchLoader->endFrame();
    m_offscreenRenderManager->endFrame();
    m_renderer->endFrame();
    m_customMaterialSystem->endFrame();
    m_presentationDimensions = m_preRenderPresentationDimensions;
    ++m_frameCount;
}

QT_END_NAMESPACE

