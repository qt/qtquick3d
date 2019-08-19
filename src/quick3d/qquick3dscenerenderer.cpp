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

#include <private/qopenglvertexarrayobject_p.h>

#include <QtQuick3DRender/private/qssgrenderframebuffer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderlayer_p.h>
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
        QElapsedTimer renderTimer;
        renderTimer.start();
        renderPending = false;
        GLuint textureId = renderer->render();

        cleanupOpenGLState();

        if (texture() && (GLuint(texture()->textureId()) != textureId || texture()->textureSize() != renderer->surfaceSize())) {
            delete texture();
            setTexture(window->createTextureFromId(textureId, renderer->surfaceSize(), QQuickWindow::TextureHasAlphaChannel));
        }
        if (!texture())
            setTexture(window->createTextureFromId(textureId, renderer->surfaceSize(), QQuickWindow::TextureHasAlphaChannel));


        markDirty(QSGNode::DirtyMaterial);
        emit textureChanged();
        if (dumpRenderTimes) {
            QOpenGLContext::currentContext()->functions()->glFinish();
            qDebug() << "FBO: Render took: " << renderTimer.elapsed() << "ms";
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

    dumpPerfTiming = !qgetenv("QUICK3D_PERFTIMERS").isEmpty();
    dumpRenderTimes = !qgetenv("QUICK3D_RENDERTIMES").isEmpty();
    if (dumpPerfTiming)
        m_sgContext->renderer()->enableLayerGpuProfiling(true);
}

QQuick3DSceneRenderer::~QQuick3DSceneRenderer()
{
    delete m_layer;
}

GLuint QQuick3DSceneRenderer::render()
{
    if (!m_layer)
        return 0;

    m_sgContext->beginFrame();
    m_renderContext->setRenderTarget(m_fbo->fbo);
    m_sgContext->renderList()->setViewport(QRect(0, 0, m_surfaceSize.width(), m_surfaceSize.height()));
    m_sgContext->setWindowDimensions(m_surfaceSize);

    m_sgContext->renderer()->prepareLayerForRender(*m_layer, m_surfaceSize, false, nullptr, true);
    m_sgContext->runRenderTasks();
    m_sgContext->renderer()->renderLayer(*m_layer, m_surfaceSize, true, QVector3D(0, 0, 0), false);
    m_sgContext->endFrame();

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

    QElapsedTimer syncTimer;
    syncTimer.start();

    if (m_surfaceSize != size) {
        m_layerSizeIsDirty = true;
        m_surfaceSize = size;
    }

    // wireframe mode
    if (m_sgContext->wireframeMode() != item->enableWireframeMode())
        m_sgContext->setWireframeMode(item->enableWireframeMode());

    auto view3D = static_cast<QQuick3DViewport*>(item);
    m_sceneManager = QQuick3DObjectPrivate::get(view3D->scene())->sceneManager;
    m_sceneManager->updateDirtyNodes();

    if (view3D->referencedScene()) {
        QQuick3DObjectPrivate::get(view3D->referencedScene())->sceneManager->updateDirtyNodes();
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
    QSSGRenderNode* referencedRootNode = nullptr;
    if (view3D->referencedScene())
        referencedRootNode = static_cast<QSSGRenderNode*>(QQuick3DObjectPrivate::get(view3D->referencedScene())->spatialNode);
    if (referencedRootNode != m_referencedRootNode) {
        if (m_referencedRootNode)
            removeNodeFromLayer(m_referencedRootNode);

        if (referencedRootNode)
            addNodeToLayer(referencedRootNode);

        m_referencedRootNode = referencedRootNode;
    }

    if (useFBO) {
        if (!m_fbo || m_layerSizeIsDirty) {
            if (m_fbo)
                delete m_fbo;

            m_fbo = new FramebufferObject(m_surfaceSize, m_renderContext);
            m_layerSizeIsDirty = false;
        }
    }
    if (dumpRenderTimes) {
        QOpenGLContext::currentContext()->functions()->glFinish();
        qDebug() << "Sync took " << syncTimer.elapsed() << "ms";
    }

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

QQuick3DPickResult QQuick3DSceneRenderer::pick(const QPointF &pos)
{
    auto pickResults = m_sgContext->renderer()->pick(*m_layer,
                                                     QVector2D(m_surfaceSize.width(), m_surfaceSize.height()),
                                                     QVector2D(pos.x(), pos.y()));
    if (pickResults.m_hitObject) {
        QQuick3DModel *model = qobject_cast<QQuick3DModel*>(m_sceneManager->lookUpNode(const_cast<QSSGRenderGraphObject*>(pickResults.m_hitObject)));
        if (model)
            return QQuick3DPickResult(model, ::sqrtf(pickResults.m_cameraDistanceSq), pickResults.m_localUVCoords);
    }

    return QQuick3DPickResult();
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


    layerNode->shadowStrength = view3D->environment()->shadowStrength();
    layerNode->shadowDist = view3D->environment()->shadowDistance();
    layerNode->shadowSoftness = view3D->environment()->shadowSoftness();
    layerNode->shadowBias = view3D->environment()->shadowBias();

    // ### These images will not be registered anywhere
    if (view3D->environment()->lightProbe())
        layerNode->lightProbe = view3D->environment()->lightProbe()->getRenderImage();
    else
        layerNode->lightProbe = nullptr;

    layerNode->probeBright = view3D->environment()->probeBrightness();
    layerNode->fastIbl = view3D->environment()->fastIBL();
    layerNode->probeHorizon = view3D->environment()->probeHorizon();
    layerNode->probeFov = view3D->environment()->probeFieldOfView();


    if (view3D->environment()->lightProbe2())
        layerNode->lightProbe2 = view3D->environment()->lightProbe()->getRenderImage();
    else
        layerNode->lightProbe2 = nullptr;

    if (view3D->camera()) {
        layerNode->activeCamera = view3D->camera()->getCameraNode();
    }


    layerNode->probe2Fade = view3D->environment()->probe2Fade();
    layerNode->probe2Window = view3D->environment()->probe2Window();
    layerNode->probe2Pos = view3D->environment()->probe2Postion();

    if (view3D->environment()->isDepthTestDisabled())
        layerNode->flags.setFlag(QSSGRenderNode::Flag::LayerEnableDepthTest, false);
    else
        layerNode->flags.setFlag(QSSGRenderNode::Flag::LayerEnableDepthTest, true);

    if (view3D->environment()->isDepthPrePassDisabled())
        layerNode->flags.setFlag(QSSGRenderNode::Flag::LayerEnableDepthPrePass, false);
    else
        layerNode->flags.setFlag(QSSGRenderNode::Flag::LayerEnableDepthPrePass, true);

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

QQuick3DSceneRenderer::FramebufferObject::FramebufferObject(const QSize &s, const QSSGRef<QSSGRenderContext> &context)
{
    size = s;
    renderContext = context;

    depthStencil = new QSSGRenderTexture2D(renderContext);
    depthStencil->setTextureData(QSSGByteView(), 0, size.width(), size.height(), QSSGRenderTextureFormat::Depth24Stencil8);
    color0 = new QSSGRenderTexture2D(renderContext);
    color0->setTextureData(QSSGByteView(), 0, size.width(), size.height(), QSSGRenderTextureFormat::RGBA8);
    fbo = new QSSGRenderFrameBuffer(renderContext);
    fbo->attach(QSSGRenderFrameBufferAttachment::Color0, color0);
    fbo->attach(QSSGRenderFrameBufferAttachment::DepthStencil, depthStencil);
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
    QElapsedTimer renderTimer;
    renderTimer.start();
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

    if (dumpRenderTimes) {
        QOpenGLContext::currentContext()->functions()->glFinish();
        qDebug() << "SGRenderNode: Render took: " << renderTimer.elapsed() << "ms";
    }
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
    if (mode == Underlay)
        connect(window, &QQuickWindow::beforeRendering, this, &QQuick3DSGDirectRenderer::render, Qt::DirectConnection);
    else
        connect(window, &QQuickWindow::afterRendering, this, &QQuick3DSGDirectRenderer::render, Qt::DirectConnection);
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
    QElapsedTimer renderTimer;
    renderTimer.start();
    const QRect glViewport = convertQtRectToGLViewport(m_viewport, m_window->size() * m_window->devicePixelRatio());
    m_renderer->render(glViewport, false);
    cleanupOpenGLState();
    if (dumpRenderTimes) {
        QOpenGLContext::currentContext()->functions()->glFinish();
        qDebug() << "Window: Render took: " << renderTimer.elapsed() << "ms";
    }
}

QT_END_NAMESPACE
