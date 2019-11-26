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

#include "qquick3dscenerenderer_p.h"
#include "qquick3dsceneenvironment_p.h"
#include "qquick3dobject_p_p.h"
#include "qquick3dnode_p.h"
#include "qquick3dscenemanager_p.h"
#include "qquick3dtexture_p.h"
#include "qquick3dcamera_p.h"
#include "qquick3dpickresult_p.h"
#include "qquick3dmodel_p.h"
#include "qquick3drenderstats_p.h"

#include <private/qopenglvertexarrayobject_p.h>

#include <QtQuick3DRender/private/qssgrenderframebuffer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderlayer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendererutil_p.h>
#include <QtQuick/QQuickWindow>

QT_BEGIN_NAMESPACE

static bool dumpPerfTiming = false;
static int frameCount = 0;
static bool dumpRenderTimes = false;

namespace {
void cleanupOpenGLState() {
    QOpenGLFunctions *gl = QOpenGLContext::currentContext()->functions();

    gl->glBindBuffer(GL_ARRAY_BUFFER, 0);
    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    gl->glActiveTexture(GL_TEXTURE0);
    gl->glBindTexture(GL_TEXTURE_2D, 0);

    gl->glDisable(GL_DEPTH_TEST);
    gl->glDisable(GL_STENCIL_TEST);
    gl->glDisable(GL_SCISSOR_TEST);

    gl->glUseProgram(0);

    QOpenGLFramebufferObject::bindDefault();
}

}

SGFramebufferObjectNode::SGFramebufferObjectNode()
    : window(nullptr)
    , renderer(nullptr)
    , renderPending(true)
    , invalidatePending(false)
    , devicePixelRatio(1)
{
    qsgnode_set_description(this, QStringLiteral("fbonode"));
    setFlag(QSGNode::UsePreprocess, true);
}

SGFramebufferObjectNode::~SGFramebufferObjectNode()
{
    delete renderer;
    delete texture();
}

void SGFramebufferObjectNode::scheduleRender()
{
    renderPending = true;
    markDirty(DirtyMaterial);
}

QSGTexture *SGFramebufferObjectNode::texture() const
{
    return QSGSimpleTextureNode::texture();
}

void SGFramebufferObjectNode::preprocess()
{
    render();
}

void SGFramebufferObjectNode::render()
{
    if (renderPending) {
        if (renderer->renderStats())
            renderer->renderStats()->startRender();

        renderPending = false;
        GLuint textureId = renderer->render();

        cleanupOpenGLState();

#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
        if (texture() && (GLuint(texture()->textureId()) != textureId || texture()->textureSize() != renderer->surfaceSize())) {
            delete texture();
            QSGTexture *wrapper = window->createTextureFromNativeObject(QQuickWindow::NativeObjectTexture, &textureId, 0,
                                                                        renderer->surfaceSize(), QQuickWindow::TextureHasAlphaChannel);
            setTexture(wrapper);
        }
        if (!texture()) {
            QSGTexture *wrapper = window->createTextureFromNativeObject(QQuickWindow::NativeObjectTexture, &textureId, 0,
                                                                        renderer->surfaceSize(), QQuickWindow::TextureHasAlphaChannel);
            setTexture(wrapper);
        }
#else
        if (texture() && (GLuint(texture()->textureId()) != textureId || texture()->textureSize() != renderer->surfaceSize())) {
            delete texture();
            setTexture(window->createTextureFromId(textureId, renderer->surfaceSize(), QQuickWindow::TextureHasAlphaChannel));
        }
        if (!texture())
            setTexture(window->createTextureFromId(textureId, renderer->surfaceSize(), QQuickWindow::TextureHasAlphaChannel));
#endif

        markDirty(QSGNode::DirtyMaterial);
        emit textureChanged();

        if (renderer->renderStats()) {
            QOpenGLContext::currentContext()->functions()->glFinish();
            renderer->renderStats()->endRender(dumpRenderTimes);
        }
        if (renderer->m_sgContext->renderer()->rendererRequestsFrames()) {
            scheduleRender();
            window->update();
        }
    }
}

void SGFramebufferObjectNode::handleScreenChange()
{
    if (!qFuzzyCompare(window->effectiveDevicePixelRatio(), devicePixelRatio)) {
        renderer->invalidateFramebufferObject();
        quickFbo->update();
    }
}


QQuick3DSceneRenderer::QQuick3DSceneRenderer(QWindow *window)
    : m_window(window)
    , m_multisampleFbo(nullptr)
    , m_supersampleFbo(nullptr)
    , m_fbo(nullptr)
{
    QOpenGLContext *openGLContext = QOpenGLContext::currentContext();

    // There is only one Render context per window, so check if one exists for this window already
    auto renderContextInterface = QSSGRenderContextInterface::getRenderContextInterface(quintptr(window));
    if (!renderContextInterface.isNull()) {
        m_sgContext = renderContextInterface;
        m_renderContext = renderContextInterface->renderContext();
    }

    // If there was no render context, then set it up for this window
    if (m_renderContext.isNull())
        m_renderContext = QSSGRenderContext::createGl(openGLContext->format());
    if (m_sgContext.isNull())
        m_sgContext = QSSGRenderContextInterface::getRenderContextInterface(m_renderContext, QString::fromLatin1("./"), quintptr(window));


    dumpPerfTiming = (qEnvironmentVariableIntValue("QUICK3D_PERFTIMERS") > 0);
    dumpRenderTimes = (qEnvironmentVariableIntValue("QUICK3D_RENDERTIMES") > 0);
    if (dumpPerfTiming) {
        m_sgContext->renderer()->enableLayerGpuProfiling(true);
        m_sgContext->performanceTimer()->setEnabled(true);
    }
}

QQuick3DSceneRenderer::~QQuick3DSceneRenderer()
{
    delete m_layer;
    delete m_fbo;
    delete m_multisampleFbo;
    delete m_supersampleFbo;
}

GLuint QQuick3DSceneRenderer::render()
{
    if (!m_layer)
        return 0;

    const bool hasMsSupport = m_sgContext->renderContext()->supportsMultisampleTextures();
    const bool ssaaEnabled = hasMsSupport && m_layer->multisampleAAMode == QSSGRenderLayer::AAMode::SSAA;
    const bool msaaEnabled = hasMsSupport && m_layer->multisampleAAMode > QSSGRenderLayer::AAMode::SSAA;

    m_sgContext->beginFrame();

    // select correct fbo for aa
    auto fbo = m_multisampleFbo ? m_multisampleFbo : m_fbo;
    fbo = m_supersampleFbo ? m_supersampleFbo : fbo;
    fbo = ssaaEnabled || msaaEnabled ? fbo : m_fbo;

    m_renderContext->setRenderTarget(fbo->fbo);
    QSize surfaceSize = m_surfaceSize;
    if (ssaaEnabled && m_supersampleFbo)
        surfaceSize *= SSAA_Multiplier;
    m_sgContext->renderList()->setViewport(QRect(0, 0, surfaceSize.width(), surfaceSize.height()));
    m_sgContext->setWindowDimensions(m_surfaceSize);

    m_sgContext->renderer()->prepareLayerForRender(*m_layer, surfaceSize, false, nullptr, true);
    m_sgContext->runRenderTasks();
    m_sgContext->renderer()->renderLayer(*m_layer, surfaceSize, true, QVector3D(0, 0, 0), false);
    m_sgContext->endFrame();

    if ((msaaEnabled && m_multisampleFbo) || (ssaaEnabled && m_supersampleFbo)) {
        m_renderContext->setRenderTarget(m_fbo->fbo);
        m_renderContext->setReadTarget(m_supersampleFbo ? m_supersampleFbo->fbo
                                                        : m_multisampleFbo->fbo);
        if (m_supersampleFbo) {
            m_renderContext->blitFramebuffer(0, 0, surfaceSize.width(), surfaceSize.height(),
                                             0, 0, m_surfaceSize.width(), m_surfaceSize.height(),
                                             QSSGRenderClearValues::Color,
                                             QSSGRenderTextureMagnifyingOp::Linear);
        } else {
            m_renderContext->blitFramebuffer(0, 0, m_surfaceSize.width(), m_surfaceSize.height(),
                                             0, 0, m_surfaceSize.width(), m_surfaceSize.height(),
                                             QSSGRenderClearValues::Color,
                                             QSSGRenderTextureMagnifyingOp::Nearest);
        }
    }

    if (dumpPerfTiming) {
        if (++frameCount == 60) {
            m_sgContext->performanceTimer()->dump();
            frameCount = 0;
        }
    }

    return HandleToID_cast(GLuint, size_t, m_fbo->color0->handle());
}

void QQuick3DSceneRenderer::render(const QRect &viewport, bool clearFirst)
{
    if (!m_layer)
        return;

    m_sgContext->beginFrame();

    // set render target to be current window (default)
    m_renderContext->setRenderTarget(nullptr);

    // set viewport
    m_sgContext->renderList()->setViewport(viewport);
    m_sgContext->renderList()->setScissorRect(viewport);
    m_sgContext->setWindowDimensions(m_surfaceSize);

    m_sgContext->renderer()->prepareLayerForRender(*m_layer, m_surfaceSize, false, nullptr, true);
    m_sgContext->runRenderTasks();
    m_sgContext->renderer()->renderLayer(*m_layer, m_surfaceSize, clearFirst, QVector3D(0, 0, 0), false);
    m_sgContext->endFrame();

    if (dumpPerfTiming) {
        if (++frameCount == 60) {
            m_sgContext->performanceTimer()->dump();
            frameCount = 0;
        }
    }
}

void QQuick3DSceneRenderer::synchronize(QQuick3DViewport *item, const QSize &size, bool useFBO)
{
    if (!item)
        return;

    if (!m_renderStats)
        m_renderStats = item->renderStats();

    if (m_renderStats)
        m_renderStats->startSync();

    if (m_surfaceSize != size) {
        m_layerSizeIsDirty = true;
        m_surfaceSize = size;
    }

    auto view3D = static_cast<QQuick3DViewport*>(item);
    m_sceneManager = QQuick3DObjectPrivate::get(view3D->scene())->sceneManager;
    m_sceneManager->updateDirtyNodes();

    QQuick3DNode *importScene = view3D->importScene();
    if (importScene)
        QQuick3DObjectPrivate::get(importScene)->sceneManager->updateDirtyNodes();

    // Generate layer node
    if (!m_layer)
        m_layer = new QSSGRenderLayer();

    // Update the layer node properties
    updateLayerNode(view3D);

    // Set the root item for the scene to the layer
    auto rootNode = static_cast<QSSGRenderNode*>(QQuick3DObjectPrivate::get(view3D->scene())->spatialNode);
    if (rootNode != m_sceneRootNode) {
        if (m_sceneRootNode)
            removeNodeFromLayer(m_sceneRootNode);

        if (rootNode)
            addNodeToLayer(rootNode);

        m_sceneRootNode = rootNode;
    }

    // Add the referenced scene root node to the layer as well if available
    QSSGRenderNode* importRootNode = nullptr;
    if (importScene) {
        importRootNode = static_cast<QSSGRenderNode*>(
                                QQuick3DObjectPrivate::get(importScene)->spatialNode);
    }
    if (importRootNode != m_importRootNode) {
        if (m_importRootNode)
            removeNodeFromLayer(m_importRootNode);

        if (importRootNode)
            m_layer->addChildrenToLayer(*importRootNode);

        m_importRootNode = importRootNode;
    }

    if (useFBO) {
        if (!m_fbo || m_layerSizeIsDirty) {
            if (m_fbo)
                delete m_fbo;

            static const auto msaaModeSamples = [](QSSGRenderLayer::AAMode mode) -> int {
                switch (mode) {
                case QSSGRenderLayer::AAMode::X2:
                    return 2;
                case QSSGRenderLayer::AAMode::X4:
                case QSSGRenderLayer::AAMode::X8:
                    return 4;
                case QSSGRenderLayer::AAMode::NoAA:
                default:
                    break;
                }
                return 1;
            };

            const bool hasMsSupport = m_sgContext->renderContext()->supportsMultisampleTextures();
            const auto msaaMode = hasMsSupport ? m_layer->multisampleAAMode : QSSGRenderLayer::AAMode::NoAA;
            const auto samples = msaaModeSamples(msaaMode);
            if (samples > 1) {
                m_multisampleFbo = new FramebufferObject(m_surfaceSize, m_renderContext, samples);
            } else if (msaaMode == QSSGRenderLayer::AAMode::SSAA) {
                m_supersampleFbo = new FramebufferObject(m_surfaceSize * SSAA_Multiplier,
                                                         m_renderContext);
            }
            m_fbo = new FramebufferObject(m_surfaceSize, m_renderContext);
            m_layerSizeIsDirty = false;
        }
    }

    if (m_renderStats)
        m_renderStats->endSync(m_sgContext, dumpRenderTimes);
}

void QQuick3DSceneRenderer::update()
{
    if (data)
        static_cast<SGFramebufferObjectNode *>(data)->scheduleRender();
}

void QQuick3DSceneRenderer::invalidateFramebufferObject()
{
    if (data)
        static_cast<SGFramebufferObjectNode *>(data)->invalidatePending = true;
}

QSSGRenderPickResult QQuick3DSceneRenderer::pick(const QPointF &pos)
{
    return m_sgContext->renderer()->pick(*m_layer, QVector2D(m_surfaceSize.width(), m_surfaceSize.height()), QVector2D(float(pos.x()), float(pos.y())));
}

QSSGRenderPickResult QQuick3DSceneRenderer::syncPick(const QPointF &pos)
{
    return m_sgContext->renderer()->syncPick(*m_layer, QVector2D(m_surfaceSize.width(), m_surfaceSize.height()), QVector2D(float(pos.x()), float(pos.y())));
}

QQuick3DRenderStats *QQuick3DSceneRenderer::renderStats()
{
    return m_renderStats;
}

void QQuick3DSceneRenderer::updateLayerNode(QQuick3DViewport *view3D)
{
    QSSGRenderLayer *layerNode = m_layer;
    layerNode->progressiveAAMode = QSSGRenderLayer::AAMode(view3D->environment()->progressiveAAMode());
    layerNode->multisampleAAMode = QSSGRenderLayer::AAMode(view3D->environment()->multisampleAAMode());
    layerNode->temporalAAEnabled = view3D->environment()->temporalAAEnabled();

    layerNode->background = QSSGRenderLayer::Background(view3D->environment()->backgroundMode());
    layerNode->clearColor = QVector3D(float(view3D->environment()->clearColor().redF()),
                                      float(view3D->environment()->clearColor().greenF()),
                                      float(view3D->environment()->clearColor().blueF()));

    layerNode->m_width = 100.f;
    layerNode->m_height = 100.f;
    layerNode->widthUnits = QSSGRenderLayer::UnitType::Percent;
    layerNode->heightUnits = QSSGRenderLayer::UnitType::Percent;

    layerNode->aoStrength = view3D->environment()->aoStrength();
    layerNode->aoDistance = view3D->environment()->aoDistance();
    layerNode->aoSoftness = view3D->environment()->aoSoftness();
    layerNode->aoBias = view3D->environment()->aoBias();
    layerNode->aoSamplerate = view3D->environment()->aoSampleRate();
    layerNode->aoDither = view3D->environment()->aoDither();

    // ### These images will not be registered anywhere
    if (view3D->environment()->lightProbe())
        layerNode->lightProbe = view3D->environment()->lightProbe()->getRenderImage();
    else
        layerNode->lightProbe = nullptr;

    layerNode->probeBright = view3D->environment()->probeBrightness();
    layerNode->fastIbl = view3D->environment()->fastImageBasedLightingEnabled();
    layerNode->probeHorizon = view3D->environment()->probeHorizon();
    layerNode->probeFov = view3D->environment()->probeFieldOfView();

    layerNode->lightProbe2 = nullptr;

    if (view3D->camera())
        layerNode->activeCamera = view3D->camera()->cameraNode();

    if (view3D->environment()->depthTestEnabled())
        layerNode->flags.setFlag(QSSGRenderNode::Flag::LayerEnableDepthTest, true);
    else
        layerNode->flags.setFlag(QSSGRenderNode::Flag::LayerEnableDepthTest, false);

    if (view3D->environment()->depthPrePassEnabled())
        layerNode->flags.setFlag(QSSGRenderNode::Flag::LayerEnableDepthPrePass, true);
    else
        layerNode->flags.setFlag(QSSGRenderNode::Flag::LayerEnableDepthPrePass, false);

    layerNode->markDirty(QSSGRenderNode::TransformDirtyFlag::TransformNotDirty);
}

void QQuick3DSceneRenderer::removeNodeFromLayer(QSSGRenderNode *node)
{
    if (!m_layer)
        return;

    m_layer->removeChild(*node);
}

void QQuick3DSceneRenderer::addNodeToLayer(QSSGRenderNode *node)
{
    if (!m_layer)
        return;

    m_layer->addChild(*node);
}

QQuick3DSceneRenderer::FramebufferObject::FramebufferObject(const QSize &s, const QSSGRef<QSSGRenderContext> &context, int msaaSamples)
{
    size = s;
    renderContext = context;
    samples = renderContext->supportsMultisampleTextures() ? msaaSamples : -1;

    depthStencil = new QSSGRenderTexture2D(renderContext);
    if (samples > 1)
        depthStencil->setTextureDataMultisample(samples, size.width(), size.height(), QSSGRenderTextureFormat::Depth24Stencil8);
    else
        depthStencil->setTextureData(QSSGByteView(), 0, size.width(), size.height(), QSSGRenderTextureFormat::Depth24Stencil8);
    color0 = new QSSGRenderTexture2D(renderContext);
    if (samples > 1)
        color0->setTextureDataMultisample(samples, size.width(), size.height(), QSSGRenderTextureFormat::RGBA8);
    else
        color0->setTextureData(QSSGByteView(), 0, size.width(), size.height(), QSSGRenderTextureFormat::RGBA8);
    fbo = new QSSGRenderFrameBuffer(renderContext);
    fbo->attach(QSSGRenderFrameBufferAttachment::Color0, color0, color0->target());
    fbo->attach(QSSGRenderFrameBufferAttachment::DepthStencil, depthStencil, depthStencil->target());
}

QQuick3DSceneRenderer::FramebufferObject::~FramebufferObject()
{

}

QSGRenderNode::StateFlags QQuick3DSGRenderNode::changedStates() const
{
    return BlendState | StencilState | DepthState | ScissorState | ColorState | CullState | ViewportState | RenderTargetState;
}

namespace {
QRect convertQtRectToGLViewport(const QRectF &rect, const QSize surfaceSize) {
    //
    const int x = int(rect.x());
    const int y = surfaceSize.height() - (int(rect.y()) + int(rect.height()));
    const int width = int(rect.width());
    const int height = int(rect.height());
    return QRect(x, y, width, height);
}
}

void QQuick3DSGRenderNode::render(const QSGRenderNode::RenderState *state)
{
    if (renderer->renderStats())
        renderer->renderStats()->startRender();

    Q_UNUSED(state)
    // calculate viewport
    const double dpr = renderer->m_window->devicePixelRatio();
    const QSizeF itemSize = renderer->surfaceSize() / dpr;

    QRectF viewport = matrix()->mapRect(QRectF(QPoint(0, 0), itemSize));
    viewport = QRectF(viewport.topLeft() * dpr, viewport.size() * dpr);

    // render
    renderer->render(convertQtRectToGLViewport(viewport, window->size() * dpr));
    markDirty(QSGNode::DirtyMaterial);

    // reset some state
    cleanupOpenGLState();

    if (renderer->renderStats()) {
        QOpenGLContext::currentContext()->functions()->glFinish();
        renderer->renderStats()->endRender(dumpRenderTimes);
    }
    if (renderer->m_sgContext->renderer()->rendererRequestsFrames())
        window->update();
}

void QQuick3DSGRenderNode::releaseResources()
{
}

QSGRenderNode::RenderingFlags QQuick3DSGRenderNode::flags() const
{
    return QSGRenderNode::RenderingFlags();
}

QQuick3DSGDirectRenderer::QQuick3DSGDirectRenderer(QQuick3DSceneRenderer *renderer, QQuickWindow *window, QQuick3DSGDirectRenderer::QQuick3DSGDirectRendererMode mode)
    : m_renderer(renderer)
    , m_window(window)
    , m_mode(mode)
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    if (QSGRendererInterface::isApiRhiBased(window->rendererInterface()->graphicsApi())) {
        if (mode == Underlay)
            connect(window, &QQuickWindow::beforeRenderPassRecording, this, &QQuick3DSGDirectRenderer::render, Qt::DirectConnection);
        else
            connect(window, &QQuickWindow::afterRenderPassRecording, this, &QQuick3DSGDirectRenderer::render, Qt::DirectConnection);
    } else
#endif
    {
        if (mode == Underlay)
            connect(window, &QQuickWindow::beforeRendering, this, &QQuick3DSGDirectRenderer::render, Qt::DirectConnection);
        else
            connect(window, &QQuickWindow::afterRendering, this, &QQuick3DSGDirectRenderer::render, Qt::DirectConnection);
    }
}

QQuick3DSGDirectRenderer::~QQuick3DSGDirectRenderer()
{
    delete m_renderer;
}

void QQuick3DSGDirectRenderer::setViewport(const QRectF &viewport)
{
    m_viewport = viewport;
}

void QQuick3DSGDirectRenderer::requestRender()
{
    m_window->update();
}

void QQuick3DSGDirectRenderer::render()
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    m_window->beginExternalCommands();
#endif

    if (m_renderer->renderStats())
        m_renderer->renderStats()->startRender();

    const QRect glViewport = convertQtRectToGLViewport(m_viewport, m_window->size() * m_window->devicePixelRatio());
    m_renderer->render(glViewport, m_mode == Underlay);
    cleanupOpenGLState();

    if (m_renderer->renderStats()) {
        QOpenGLContext::currentContext()->functions()->glFinish();
        m_renderer->renderStats()->endRender(dumpRenderTimes);
    }
    if (m_renderer->m_sgContext->renderer()->rendererRequestsFrames())
        m_window->update();

#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    m_window->endExternalCommands();
#endif
}

QT_END_NAMESPACE
