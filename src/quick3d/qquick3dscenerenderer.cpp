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
#include "qquick3dobject_p.h"
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

#include <QtQuick3DRuntimeRender/private/qssgrendereffect_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendererimpllayerrenderpreparationdata_p.h>

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
    , requestedFramesCount(0)
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
            if (dumpRenderTimes)
                QOpenGLContext::currentContext()->functions()->glFinish();
            renderer->renderStats()->endRender(dumpRenderTimes);
        }
        if (renderer->m_sgContext->renderer()->rendererRequestsFrames()
                || requestedFramesCount > 0) {
            scheduleRender();
            window->update();
            if (requestedFramesCount > 0)
                requestedFramesCount--;
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
    , m_antialiasingFbo(nullptr)
    , m_fbo(nullptr)
{
    QOpenGLContext *openGLContext = QOpenGLContext::currentContext();

    // There is only one Render context per window, so check if one exists for this window already
    m_sgContext = QSSGRenderContextInterface::getRenderContextInterface(quintptr(window));

    // If there was no render context, then set it up for this window
    if (m_sgContext.isNull())
        m_sgContext = QSSGRenderContextInterface::getRenderContextInterface(QSSGRenderContext::createGl(openGLContext->format()), QString::fromLatin1("./"), quintptr(window));


    dumpPerfTiming = (qEnvironmentVariableIntValue("QT_QUICK3D_DUMP_PERFTIMERS") > 0);
    dumpRenderTimes = (qEnvironmentVariableIntValue("QT_QUICK3D_DUMP_RENDERTIMES") > 0);
    if (dumpPerfTiming) {
        m_sgContext->renderer()->enableLayerGpuProfiling(true);
        m_sgContext->performanceTimer()->setEnabled(true);
    }
}

QQuick3DSceneRenderer::~QQuick3DSceneRenderer()
{
    delete m_layer;
    delete m_fbo;
    delete m_antialiasingFbo;
}

GLuint QQuick3DSceneRenderer::render()
{
    if (!m_layer)
        return 0;

    const bool hasMsSupport = m_sgContext->renderContext()->supportsMultisampleTextures();
    const bool ssaaEnabled = m_layer->antialiasingMode == QSSGRenderLayer::AAMode::SSAA;
    const bool msaaEnabled = hasMsSupport && m_layer->antialiasingMode == QSSGRenderLayer::AAMode::MSAA;

    m_sgContext->beginFrame();

    // select correct fbo for aa
    const bool useMSAA = msaaEnabled && m_antialiasingFbo;
    const bool useSSAA = ssaaEnabled && m_antialiasingFbo;
    const bool useAA = (useMSAA || useSSAA);
    auto fbo = useAA ? m_antialiasingFbo : m_fbo;

    const auto &renderContext = m_sgContext->renderContext();
    renderContext->setRenderTarget(fbo->fbo);
    QSize surfaceSize = m_surfaceSize;
    if (useSSAA)
        surfaceSize *= m_ssaaMultiplier;

    m_sgContext->setViewport(QRect(0, 0, surfaceSize.width(), surfaceSize.height()));
    m_sgContext->setScissorRect(QRect());
    m_sgContext->setWindowDimensions(m_surfaceSize);
    m_sgContext->setSceneColor(QColor(Qt::black));

    m_sgContext->prepareLayerForRender(*m_layer);
    m_sgContext->renderLayer(*m_layer, true);

    m_sgContext->endFrame();

    if (useAA) {
        renderContext->setRenderTarget(m_fbo->fbo);
        renderContext->setReadTarget(m_antialiasingFbo->fbo);
        auto magOp = useSSAA ? QSSGRenderTextureMagnifyingOp::Linear
                             : QSSGRenderTextureMagnifyingOp::Nearest;
        renderContext->blitFramebuffer(0, 0, surfaceSize.width(), surfaceSize.height(),
                                         0, 0, m_surfaceSize.width(), m_surfaceSize.height(),
                                         QSSGRenderClearValues::Color,
                                         magOp);
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
    const auto &renderContext = m_sgContext->renderContext();
    renderContext->setRenderTarget(nullptr);

    // set viewport
    m_sgContext->setWindowDimensions(m_surfaceSize);
    m_sgContext->setViewport(viewport);
    m_sgContext->setScissorRect(viewport);

    // set clear color
    m_sgContext->setSceneColor(QColor(Qt::black));

    m_sgContext->prepareLayerForRender(*m_layer);
    m_sgContext->renderLayer(*m_layer, clearFirst);

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
    m_sceneManager->updateBoundingBoxes(m_sgContext->bufferManager());

    QQuick3DNode *importScene = view3D->importScene();
    if (importScene) {
        auto sceneManager = QQuick3DObjectPrivate::get(importScene)->sceneManager;
        sceneManager->updateBoundingBoxes(m_sgContext->bufferManager());
        sceneManager->updateDirtyNodes();
    }

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

        if (importRootNode) {
            // if importScene has the rendered viewport as ancestor, it probably means
            // "importScene: MyScene { }" type of inclusion.
            // In this case don't duplicate content by adding it again.
            QObject *sceneParent = importScene->parent();
            bool isEmbedded = false;
            while (sceneParent) {
                if (sceneParent == item) {
                    isEmbedded = true;
                    break;
                }
                sceneParent = sceneParent->parent();
            }
            if (!isEmbedded)
                m_layer->addChildrenToLayer(*importRootNode);
        }

        m_importRootNode = importRootNode;
    }

    if (useFBO) {
        const auto &renderContext = m_sgContext->renderContext();
        if (!m_fbo || m_layerSizeIsDirty) {
            delete m_fbo;
            m_fbo = new FramebufferObject(m_surfaceSize, renderContext);
        }
        if (m_aaIsDirty || m_layerSizeIsDirty) {
            delete m_antialiasingFbo;
            m_antialiasingFbo = nullptr;

            const bool ssaaEnabled = m_layer->antialiasingMode == QSSGRenderLayer::AAMode::SSAA;
            const bool msaaEnabled = m_layer->antialiasingMode == QSSGRenderLayer::AAMode::MSAA;
            const bool hasMsSupport = m_sgContext->renderContext()->supportsMultisampleTextures();

            if (hasMsSupport && msaaEnabled) {
                const auto samples = int(m_layer->antialiasingQuality);
                m_antialiasingFbo = new FramebufferObject(m_surfaceSize, renderContext, samples);
            } else if (ssaaEnabled) {
                m_antialiasingFbo = new FramebufferObject(m_surfaceSize * m_ssaaMultiplier,
                                                          renderContext);
            }
            m_aaIsDirty = false;
        }
        m_layerSizeIsDirty = false;
    }

    if (m_renderStats)
        m_renderStats->endSync(dumpRenderTimes);
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
    return m_sgContext->renderer()->syncPick(*m_layer,
                                             m_sgContext->bufferManager(),
                                             QVector2D(m_surfaceSize.width(), m_surfaceSize.height()),
                                             QVector2D(float(pos.x()), float(pos.y())));
}

QQuick3DRenderStats *QQuick3DSceneRenderer::renderStats()
{
    return m_renderStats;
}

void QQuick3DSceneRenderer::updateLayerNode(QQuick3DViewport *view3D)
{
    QSSGRenderLayer *layerNode = m_layer;

    QSSGRenderLayer::AAMode aaMode = QSSGRenderLayer::AAMode(view3D->environment()->antialiasingMode());
    if (aaMode != layerNode->antialiasingMode) {
        layerNode->antialiasingMode = aaMode;
        m_aaIsDirty = true;
    }
    QSSGRenderLayer::AAQuality aaQuality = QSSGRenderLayer::AAQuality(view3D->environment()->antialiasingQuality());
    if (aaQuality != layerNode->antialiasingQuality) {
        layerNode->antialiasingQuality = aaQuality;
        m_ssaaMultiplier = (aaQuality == QSSGRenderLayer::AAQuality::Normal) ? 1.2f :
                           (aaQuality == QSSGRenderLayer::AAQuality::High) ? 1.5f :
                                                                             2.0f;
        layerNode->ssaaMultiplier = m_ssaaMultiplier;
        m_aaIsDirty = true;
    }

    bool temporalIsDirty = false;
    bool temporalAAEnabled = view3D->environment()->temporalAAEnabled();
    if (temporalAAEnabled != layerNode->temporalAAEnabled) {
        layerNode->temporalAAEnabled = view3D->environment()->temporalAAEnabled();
        temporalIsDirty = true;
    }
    layerNode->temporalAAStrength = view3D->environment()->temporalAAStrength();

    if ((m_aaIsDirty || temporalIsDirty) && layerNode->temporalAAEnabled) {
        // When temporalAA is on and antialiasing mode changes,
        // layer needs to be re-rendered (at least) MAX_TEMPORAL_AA_LEVELS times
        // to generate temporal antialiasing.
        if (data)
            static_cast<SGFramebufferObjectNode *>(data)->requestedFramesCount
                = QSSGLayerRenderPreparationData::MAX_TEMPORAL_AA_LEVELS;
    }

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

    // Effects need to be rendered in reverse order as described in the file.
    layerNode->firstEffect = nullptr; // We reset the linked list
    const auto &effects = view3D->environment()->m_effects;
    auto rit = effects.crbegin();
    const auto rend = effects.crend();
    for (; rit != rend; ++rit) {
        QQuick3DObjectPrivate *p = QQuick3DObjectPrivate::get(*rit);
        QSSGRenderEffect *effectNode = static_cast<QSSGRenderEffect *>(p->spatialNode);
        if (effectNode)
            layerNode->addEffect(*effectNode);
    }
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

QQuick3DSceneRenderer::FramebufferObject::FramebufferObject(const QSize &s,
                                                            const QSSGRef<QSSGRenderContext> &context,
                                                            int msaaSamples)
{
    size = s;
    renderContext = context;
    samples = renderContext->supportsMultisampleTextures() ? msaaSamples : -1;
    // Limit samples to max supported
    samples = qMin(samples, renderContext->maxSamples());

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
        if (dumpRenderTimes)
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

void QQuick3DSGDirectRenderer::setVisibility(bool visible)
{
    if (m_isVisible == visible)
        return;
    m_isVisible = visible;
    m_window->update();
}

void QQuick3DSGDirectRenderer::render()
{
    if (!m_isVisible)
        return;

#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    m_window->beginExternalCommands();
#endif

    if (m_renderer->renderStats())
        m_renderer->renderStats()->startRender();

    const QRect glViewport = convertQtRectToGLViewport(m_viewport, m_window->size() * m_window->devicePixelRatio());
    m_renderer->render(glViewport, m_mode == Underlay);
    cleanupOpenGLState();

    if (m_renderer->renderStats()) {
        if (dumpRenderTimes)
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
