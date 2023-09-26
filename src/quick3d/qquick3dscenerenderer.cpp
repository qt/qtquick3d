// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qquick3dcubemaptexture_p.h"
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
#include "qquick3ddebugsettings_p.h"
#include "extensions/qquick3drenderextensions_p.h"
#include <QtQuick3DUtils/private/qquick3dprofiler_p.h>

#include <QtQuick3DRuntimeRender/private/qssgrendererutil_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderer_p.h>

#include <QtQuick/private/qquickwindow_p.h>
#include <QtQuick/private/qsgdefaultrendercontext_p.h>
#include <QtQuick/private/qsgtexture_p.h>
#include <QtQuick/private/qsgplaintexture_p.h>
#include <QtQuick/private/qsgrendernode_p.h>

#include <QtQuick3DRuntimeRender/private/qssgrendereffect_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrhieffectsystem_p.h>
#include <QtQuick3DRuntimeRender/private/qssglayerrenderdata_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrhiquadrenderer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrhicontext_p.h>
#include <QtQuick3DRuntimeRender/private/qssgcputonemapper_p.h>
#include <QtQuick3DUtils/private/qssgutils_p.h>
#include <QtQuick3DUtils/private/qssgassert_p.h>


#include <qtquick3d_tracepoints_p.h>

#include <QtCore/QObject>
#include <QtCore/qqueue.h>

QT_BEGIN_NAMESPACE

Q_TRACE_PREFIX(qtquick3d,
    "QT_BEGIN_NAMESPACE" \
    "class QQuick3DViewport;" \
    "QT_END_NAMESPACE"
)

Q_TRACE_POINT(qtquick3d, QSSG_prepareFrame_entry, int width, int height);
Q_TRACE_POINT(qtquick3d, QSSG_prepareFrame_exit);
Q_TRACE_POINT(qtquick3d, QSSG_renderFrame_entry, int width, int height);
Q_TRACE_POINT(qtquick3d, QSSG_renderFrame_exit);
Q_TRACE_POINT(qtquick3d, QSSG_synchronize_entry, QQuick3DViewport *view3D, const QSize &size, float dpr);
Q_TRACE_POINT(qtquick3d, QSSG_synchronize_exit);
Q_TRACE_POINT(qtquick3d, QSSG_renderPass_entry, const QString &renderPass);
Q_TRACE_POINT(qtquick3d, QSSG_renderPass_exit);

static bool dumpRenderTimes = false;

#if QT_CONFIG(qml_debug)

static inline quint64 statDrawCallCount(const QSSGRhiContextStats &stats)
{
    quint64 count = 0;
    const QSSGRhiContextStats::PerLayerInfo &info(stats.perLayerInfo[stats.layerKey]);
    for (const auto &pass : info.renderPasses)
        count += QSSGRhiContextStats::totalDrawCallCountForPass(pass);
    count += QSSGRhiContextStats::totalDrawCallCountForPass(info.externalRenderPass);
    return count;
}

#define STAT_PAYLOAD(stats) \
    (statDrawCallCount(stats) | (quint64(stats.perLayerInfo[stats.layerKey].renderPasses.size()) << 32))

#endif

template <typename In, typename Out>
static void bfs(In *inExtension, QList<Out *> &outList)
{
    outList.clear();

    QSSG_ASSERT(inExtension, return);

    QQueue<In *> queue { { inExtension } };
    while (queue.size() > 0) {
        if (auto cur = queue.dequeue()) {
            if (auto *ext = static_cast<Out *>(QQuick3DObjectPrivate::get(cur)->spatialNode))
                outList.push_back(ext);
            for (auto &chld : cur->childItems())
                queue.enqueue(qobject_cast<In *>(chld));
        }
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

// QQuickWindow::update() behaves differently depending on whether it's called from the GUI thread
// or the render thread.
// TODO: move this to QQuickWindow::fullUpdate(), if we can't change update()
static void requestFullUpdate(QQuickWindow *window)
{
    if (QThread::currentThread() == QCoreApplication::instance()->thread())
        window->update();
    else
        QCoreApplication::postEvent(window, new QEvent(QEvent::Type(QQuickWindowPrivate::FullUpdateRequest)));
}

void SGFramebufferObjectNode::render()
{
    if (renderPending) {
        if (renderer->renderStats())
            renderer->renderStats()->startRender();

        renderPending = false;

        if (renderer->m_sgContext->rhiContext()->isValid()) {
            QRhiTexture *rhiTexture = renderer->renderToRhiTexture(window);
            bool needsNewWrapper = false;
            if (!texture() || (texture()->textureSize() != renderer->surfaceSize()
                               || texture()->rhiTexture() != rhiTexture))
            {
                needsNewWrapper = true;
            }
            if (needsNewWrapper) {
                delete texture();
                QSGPlainTexture *t = new QSGPlainTexture;
                t->setOwnsTexture(false);
                t->setHasAlphaChannel(true);
                t->setTexture(rhiTexture);
                t->setTextureSize(renderer->surfaceSize());
                setTexture(t);
            }
        }

        markDirty(QSGNode::DirtyMaterial);
        emit textureChanged();

        if (renderer->renderStats())
            renderer->renderStats()->endRender(dumpRenderTimes);

        if (renderer->requestedFramesCount > 0) {
            scheduleRender();
            requestFullUpdate(window);
            renderer->requestedFramesCount--;
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


QQuick3DSceneRenderer::QQuick3DSceneRenderer(const std::shared_ptr<QSSGRenderContextInterface> &rci)
    : m_sgContext(rci)
{
    dumpRenderTimes = (qEnvironmentVariableIntValue("QT_QUICK3D_DUMP_RENDERTIMES") > 0);
}

QQuick3DSceneRenderer::~QQuick3DSceneRenderer()
{
    QSSGRhiContext *rhiCtx = m_sgContext->rhiContext().get();
    rhiCtx->stats().cleanupLayerInfo(m_layer);
    m_sgContext->bufferManager()->releaseResourcesForLayer(m_layer);
    delete m_layer;

    delete m_texture;

    releaseAaDependentRhiResources();
    delete m_effectSystem;
}

void QQuick3DSceneRenderer::releaseAaDependentRhiResources()
{
    QSSGRhiContext *rhiCtx = m_sgContext->rhiContext().get();
    if (!rhiCtx->isValid())
        return;

    delete m_textureRenderTarget;
    m_textureRenderTarget = nullptr;

    delete m_textureRenderPassDescriptor;
    m_textureRenderPassDescriptor = nullptr;

    delete m_depthStencilBuffer;
    m_depthStencilBuffer = nullptr;

    delete m_msaaRenderBuffer;
    m_msaaRenderBuffer = nullptr;

    delete m_ssaaTexture;
    m_ssaaTexture = nullptr;

    delete m_ssaaTextureToTextureRenderTarget;
    m_ssaaTextureToTextureRenderTarget = nullptr;

    delete m_ssaaTextureToTextureRenderPassDescriptor;
    m_ssaaTextureToTextureRenderPassDescriptor = nullptr;

    delete m_temporalAATexture;
    m_temporalAATexture = nullptr;
    delete m_temporalAARenderTarget;
    m_temporalAARenderTarget = nullptr;
    delete m_temporalAARenderPassDescriptor;
    m_temporalAARenderPassDescriptor = nullptr;

    delete m_prevTempAATexture;
    m_prevTempAATexture = nullptr;
}

// Blend factors are in the form of (frame blend factor, accumulator blend factor)
static const QVector2D s_ProgressiveAABlendFactors[QSSGLayerRenderData::MAX_AA_LEVELS] = {
    QVector2D(0.500000f, 0.500000f), // 1x
    QVector2D(0.333333f, 0.666667f), // 2x
    QVector2D(0.250000f, 0.750000f), // 3x
    QVector2D(0.200000f, 0.800000f), // 4x
    QVector2D(0.166667f, 0.833333f), // 5x
    QVector2D(0.142857f, 0.857143f), // 6x
    QVector2D(0.125000f, 0.875000f), // 7x
    QVector2D(0.111111f, 0.888889f), // 8x
};

static const QVector2D s_TemporalAABlendFactors = { 0.5f, 0.5f };

QRhiTexture *QQuick3DSceneRenderer::renderToRhiTexture(QQuickWindow *qw)
{
    if (!m_layer)
        return nullptr;

    QRhiTexture *currentTexture = m_texture; // the result so far

    if (qw) {
        if (m_renderStats)
            m_renderStats->startRenderPrepare();

        Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DPrepareFrame);

        QSSGRhiContext *rhiCtx = m_sgContext->rhiContext().get();

        rhiCtx->setMainRenderPassDescriptor(m_textureRenderPassDescriptor);
        rhiCtx->setRenderTarget(m_textureRenderTarget);

        QRhiCommandBuffer *cb = nullptr;
        QRhiSwapChain *swapchain = qw->swapChain();
        if (swapchain) {
            cb = swapchain->currentFrameCommandBuffer();
            rhiCtx->setCommandBuffer(cb);
        } else {
            QSGRendererInterface *rif = qw->rendererInterface();
            cb = static_cast<QRhiCommandBuffer *>(
                rif->getResource(qw, QSGRendererInterface::RhiRedirectCommandBuffer));
            if (cb)
                rhiCtx->setCommandBuffer(cb);
            else {
                qWarning("Neither swapchain nor redirected command buffer are available.");
                return currentTexture;
            }
        }

        // Graphics pipeline objects depend on the MSAA sample count, so the
        // renderer needs to know the value.
        rhiCtx->setMainPassSampleCount(m_msaaRenderBuffer ? m_msaaRenderBuffer->sampleCount() : 1);

        int ssaaAdjustedWidth = m_surfaceSize.width();
        int ssaaAdjustedHeight = m_surfaceSize.height();
        if (m_layer->antialiasingMode == QSSGRenderLayer::AAMode::SSAA) {
            ssaaAdjustedWidth *= m_ssaaMultiplier;
            ssaaAdjustedHeight *= m_ssaaMultiplier;
        }

        Q_TRACE(QSSG_prepareFrame_entry, ssaaAdjustedWidth, ssaaAdjustedHeight);

        float dpr = m_sgContext->dpr();
        const QRect vp = QRect(0, 0, ssaaAdjustedWidth, ssaaAdjustedHeight);
        beginFrame();
        rhiPrepare(vp, dpr);

        if (m_renderStats)
            m_renderStats->endRenderPrepare();

        Q_QUICK3D_PROFILE_END_WITH_ID(QQuick3DProfiler::Quick3DPrepareFrame, quint64(ssaaAdjustedWidth) | quint64(ssaaAdjustedHeight) << 32, profilingId);

        Q_TRACE(QSSG_prepareFrame_exit);

        Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DRenderFrame);
        Q_TRACE(QSSG_renderFrame_entry, ssaaAdjustedWidth, ssaaAdjustedHeight);

        QColor clearColor = Qt::transparent;
        if (m_backgroundMode == QSSGRenderLayer::Background::Color
                || (m_backgroundMode == QSSGRenderLayer::Background::SkyBoxCubeMap && !m_layer->skyBoxCubeMap)
                || (m_backgroundMode == QSSGRenderLayer::Background::SkyBox && !m_layer->lightProbe))
        {
            // Same logic as with the main render pass and skybox: tonemap
            // based on tonemapMode (unless it is None), unless there are effects.
            clearColor = m_layer->firstEffect ? m_linearBackgroundColor : m_tonemappedBackgroundColor;
        }

        // This is called from the node's preprocess() meaning Qt Quick has not
        // actually began recording a renderpass. Do our own.
        cb->beginPass(m_textureRenderTarget, clearColor, { 1.0f, 0 }, nullptr, QSSGRhiContext::commonPassFlags());
        Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DRenderPass);
        QSSGRHICTX_STAT(rhiCtx, beginRenderPass(m_textureRenderTarget));
        rhiRender();
        cb->endPass();
        QSSGRHICTX_STAT(rhiCtx, endRenderPass());
        Q_QUICK3D_PROFILE_END_WITH_STRING(QQuick3DProfiler::Quick3DRenderPass, quint64(ssaaAdjustedWidth) | quint64(ssaaAdjustedHeight) << 32, QByteArrayLiteral("main"));

        const bool temporalAA = m_layer->temporalAAIsActive;
        const bool progressiveAA = m_layer->progressiveAAIsActive;
        const bool superSamplingAA = m_layer->antialiasingMode == QSSGRenderLayer::AAMode::SSAA;
        QRhi *rhi = rhiCtx->rhi();

        currentTexture = superSamplingAA ? m_ssaaTexture : m_texture;

        // Do effects before antialiasing
        if (m_effectSystem && m_layer->firstEffect && m_layer->renderedCamera) {
            const auto &renderer = m_sgContext->renderer();
            QSSGLayerRenderData *theRenderData = renderer->getOrCreateLayerRenderData(*m_layer);
            Q_ASSERT(theRenderData);
            QRhiTexture *theDepthTexture = theRenderData->getRenderResult(QSSGFrameData::RenderResult::DepthTexture)->texture;
            QVector2D cameraClipRange(m_layer->renderedCamera->clipNear, m_layer->renderedCamera->clipFar);

            currentTexture = m_effectSystem->process(*m_layer->firstEffect,
                                                     currentTexture,
                                                     theDepthTexture,
                                                     cameraClipRange);
        }

        // The only difference between temporal and progressive AA at this point is that tempAA always
        // uses blend factors of 0.5 and copies currentTexture to m_prevTempAATexture, while progAA uses blend
        // factors from a table and copies the blend result to m_prevTempAATexture

        if ((progressiveAA || temporalAA) && m_prevTempAATexture) {
            cb->debugMarkBegin(QByteArrayLiteral("Temporal AA"));
            Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DRenderPass);
            Q_TRACE_SCOPE(QSSG_renderPass, QStringLiteral("Temporal AA"));
            QRhiTexture *blendResult;
            uint *aaIndex = progressiveAA ? &m_layer->progAAPassIndex : &m_layer->tempAAPassIndex; // TODO: can we use only one index?

            if (*aaIndex > 0) {
                if (temporalAA || *aaIndex < quint32(m_layer->antialiasingQuality)) {
                    const auto &renderer = m_sgContext->renderer();

                    // The fragment shader relies on per-target compilation and
                    // QSHADER_ macros of qsb, hence no need to communicate a flip
                    // flag from here.
                    const auto &shaderPipeline = renderer->getRhiProgressiveAAShader();
                    QRhiResourceUpdateBatch *rub = nullptr;

                    QSSGRhiDrawCallData &dcd(rhiCtx->drawCallData({ m_layer, nullptr, nullptr, 0 }));
                    QRhiBuffer *&ubuf = dcd.ubuf;
                    const int ubufSize = 2 * sizeof(float);
                    if (!ubuf) {
                        ubuf = rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, ubufSize);
                        ubuf->create();
                    }

                    rub = rhi->nextResourceUpdateBatch();
                    int idx = *aaIndex - 1;
                    const QVector2D *blendFactors = progressiveAA ? &s_ProgressiveAABlendFactors[idx] : &s_TemporalAABlendFactors;
                    rub->updateDynamicBuffer(ubuf, 0, 2 * sizeof(float), blendFactors);
                    renderer->rhiQuadRenderer()->prepareQuad(rhiCtx, rub);

                    QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                                             QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge, QRhiSampler::Repeat });
                    QSSGRhiShaderResourceBindingList bindings;
                    bindings.addUniformBuffer(0, QRhiShaderResourceBinding::FragmentStage, ubuf);
                    bindings.addTexture(1, QRhiShaderResourceBinding::FragmentStage, currentTexture, sampler);
                    bindings.addTexture(2, QRhiShaderResourceBinding::FragmentStage, m_prevTempAATexture, sampler);

                    QRhiShaderResourceBindings *srb = rhiCtx->srb(bindings);

                    QSSGRhiGraphicsPipelineState ps;
                    const QSize textureSize = currentTexture->pixelSize();
                    ps.viewport = QRhiViewport(0, 0, float(textureSize.width()), float(textureSize.height()));
                    ps.shaderPipeline = shaderPipeline.get();

                    renderer->rhiQuadRenderer()->recordRenderQuadPass(rhiCtx, &ps, srb, m_temporalAARenderTarget, QSSGRhiQuadRenderer::UvCoords);
                    blendResult = m_temporalAATexture;
                } else {
                    blendResult = m_prevTempAATexture;
                }
            } else {
                // For the first frame: no blend, only copy
                blendResult = currentTexture;
            }

            QRhiCommandBuffer *cb = rhiCtx->commandBuffer();

            if (temporalAA || (*aaIndex < quint32(m_layer->antialiasingQuality))) {
                auto *rub = rhi->nextResourceUpdateBatch();
                if (progressiveAA)
                    rub->copyTexture(m_prevTempAATexture, blendResult);
                else
                    rub->copyTexture(m_prevTempAATexture, currentTexture);
                cb->resourceUpdate(rub);
            }

            (*aaIndex)++;
            cb->debugMarkEnd();
            Q_QUICK3D_PROFILE_END_WITH_STRING(QQuick3DProfiler::Quick3DRenderPass, 0, QByteArrayLiteral("temporal_aa"));

            currentTexture = blendResult;
        }

        if (m_layer->antialiasingMode == QSSGRenderLayer::AAMode::SSAA) {
            // With supersampling antialiasing we at this point have the
            // content rendered at a larger size into m_ssaaTexture. Now scale
            // it down to the expected size into m_texture, using linear
            // filtering. Unlike in the OpenGL world, there is no
            // glBlitFramebuffer equivalent available, because APIs like D3D
            // and Metal have no such operation (the generally supported
            // texture copy operations are 1:1 copies, without support for
            // scaling, which is what we would need here). So draw a quad.

            QRhiCommandBuffer *cb = rhiCtx->commandBuffer();
            const auto &renderer = m_sgContext->renderer();

            cb->debugMarkBegin(QByteArrayLiteral("SSAA downsample"));
            Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DRenderPass);

            Q_TRACE_SCOPE(QSSG_renderPass, QStringLiteral("SSAA downsample"));

            renderer->rhiQuadRenderer()->prepareQuad(rhiCtx, nullptr);

            // Instead of passing in a flip flag we choose to rely on qsb's
            // per-target compilation mode in the fragment shader. (it does UV
            // flipping based on QSHADER_ macros) This is just better for
            // performance and the shaders are very simple so introducing a
            // uniform block and branching dynamically would be an overkill.
            const auto &shaderPipeline = renderer->getRhiSupersampleResolveShader();

            QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                                     QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge, QRhiSampler::Repeat });
            QSSGRhiShaderResourceBindingList bindings;
            bindings.addTexture(0, QRhiShaderResourceBinding::FragmentStage, currentTexture, sampler);
            QRhiShaderResourceBindings *srb = rhiCtx->srb(bindings);

            QSSGRhiGraphicsPipelineState ps;
            ps.viewport = QRhiViewport(0, 0, float(m_surfaceSize.width()), float(m_surfaceSize.height()));
            ps.shaderPipeline = shaderPipeline.get();

            renderer->rhiQuadRenderer()->recordRenderQuadPass(rhiCtx, &ps, srb, m_ssaaTextureToTextureRenderTarget, QSSGRhiQuadRenderer::UvCoords);
            cb->debugMarkEnd();
            Q_QUICK3D_PROFILE_END_WITH_STRING(QQuick3DProfiler::Quick3DRenderPass, 0, QByteArrayLiteral("ssaa_downsample"));

            currentTexture = m_texture;
        }
        endFrame();

        Q_QUICK3D_PROFILE_END_WITH_ID(QQuick3DProfiler::Quick3DRenderFrame,
                                           STAT_PAYLOAD(m_sgContext->rhiContext()->stats()),
                                           profilingId);

        Q_TRACE(QSSG_renderFrame_exit);

    }

    return currentTexture;
}

void QQuick3DSceneRenderer::beginFrame()
{
    m_sgContext->beginFrame(m_layer);
}

void QQuick3DSceneRenderer::endFrame()
{
    m_sgContext->endFrame(m_layer);
}

void QQuick3DSceneRenderer::rhiPrepare(const QRect &viewport, qreal displayPixelRatio)
{
    if (!m_layer)
        return;

    m_sgContext->setDpr(displayPixelRatio);

    m_sgContext->setViewport(viewport);

    m_sgContext->setSceneColor(QColor(Qt::black));

    m_sgContext->prepareLayerForRender(*m_layer);
    // If sync was called the assumption is that the scene is dirty regardless of what
    // the scene prep function says, we still should verify that we have a camera before
    // we call render prep and render.
    const bool renderReady = (m_layer->renderData->camera != nullptr);
    if (renderReady) {
        m_sgContext->rhiPrepare(*m_layer);
        m_prepared = true;
    }
}

void QQuick3DSceneRenderer::rhiRender()
{
    if (m_prepared) {
        // There is no clearFirst flag - the rendering here does not record a
        // beginPass() so it never clears on its own.

        m_sgContext->rhiRender(*m_layer);
    }

    m_prepared = false;
}

static QRhiTexture::Format toRhiTextureFormat(QQuickShaderEffectSource::Format format)
{
    switch (format) {
    case QQuickShaderEffectSource::RGBA8:
        return QRhiTexture::RGBA8;
    case QQuickShaderEffectSource::RGBA16F:
        return QRhiTexture::RGBA16F;
    case QQuickShaderEffectSource::RGBA32F:
        return QRhiTexture::RGBA32F;
    default:
        return QRhiTexture::RGBA8;
    }
}

static QVector3D tonemapRgb(const QVector3D &c, QQuick3DSceneEnvironment::QQuick3DEnvironmentTonemapModes tonemapMode)
{
    switch (tonemapMode) {
    case QQuick3DSceneEnvironment::TonemapModeLinear:
        return QSSGTonemapper::tonemapLinearToSrgb(c);
    case QQuick3DSceneEnvironment::TonemapModeHejlDawson:
        return QSSGTonemapper::tonemapHejlDawson(c);
    case QQuick3DSceneEnvironment::TonemapModeAces:
        return QSSGTonemapper::tonemapAces(c);
    case QQuick3DSceneEnvironment::TonemapModeFilmic:
        return QSSGTonemapper::tonemapFilmic(c);
    default:
        break;
    }
    return c;
}

void QQuick3DSceneRenderer::synchronize(QQuick3DViewport *view3D, const QSize &size, float dpr)
{
    Q_TRACE_SCOPE(QSSG_synchronize, view3D, size, dpr);

    Q_ASSERT(view3D != nullptr); // This is not an option!
    QSSGRhiContext *rhiCtx = m_sgContext->rhiContext().get();
    Q_ASSERT(rhiCtx != nullptr);

    bool newRenderStats = false;
    if (!m_renderStats) {
        m_renderStats = view3D->renderStats();
        newRenderStats = true;
    }

    if (m_renderStats)
        m_renderStats->startSync();

    Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DSynchronizeFrame);

    m_sgContext->setDpr(dpr);
    bool layerSizeIsDirty = m_surfaceSize != size;
    m_surfaceSize = size;

    // Synchronize scene managers under this window
    QSet<QSSGRenderGraphObject *> resourceLoaders;
    bool requestSharedUpdate = false;
    if (auto window = view3D->window()) {
        if (!winAttacment || winAttacment->window() != window)
            winAttacment = QQuick3DSceneManager::getOrSetWindowAttachment(*window);

        if (winAttacment && winAttacment->rci() != m_sgContext)
            winAttacment->setRci(m_sgContext);

        if (winAttacment)
            requestSharedUpdate = winAttacment->synchronize(resourceLoaders);
    }

    // Import scenes used in a multi-window application...
    QQuick3DNode *importScene = view3D->importScene();
    if (importScene) {
        QQuick3DSceneManager *importSceneManager = QQuick3DObjectPrivate::get(importScene)->sceneManager;
        // If the import scene is used with 3D views under a different window, then we'll
        // need to trigger updates for those as well.
        if (auto window = importSceneManager->window(); window && window != view3D->window()) {
            if (auto winAttacment = importSceneManager->wattached) {
                // Not the same window but backed by the same rhi?
                auto rci = winAttacment->rci();
                const bool inlineSync = (rci && rci->rhi() && (rci->rhi()->thread() == m_sgContext->rhi()->thread()));
                if (inlineSync) {
                    // Given that we're on the same thread, we can do an immediate sync
                    // (rhi instances can differ, e.g., basic renderloop).
                    winAttacment->synchronize(resourceLoaders);
                } else if (rci && !window->isExposed()) { // Forced sync of non-exposed windows
                    // Not exposed, so not rendering (playing with fire here)...
                    winAttacment->synchronize(resourceLoaders);
                } else if (!rci || requestSharedUpdate) {
                    // If there's no RCI for the importscene we'll request an update, which should
                    // mean we only get here once. It also means the update to any secondary windows
                    // will be delayed. Note that calling this function on each sync would cause the
                    // different views to ping-pong for updated forever...
                    winAttacment->requestUpdate();
                }
            }
        }
    }

    // Generate layer node
    if (!m_layer)
        m_layer = new QSSGRenderLayer();

    if (newRenderStats)
        m_renderStats->setRhiContext(rhiCtx, m_layer);

    // if the list is dirty we rebuild (assumption is that this won't happen frequently).
    if (view3D->extensionListDirty()) {
        // All items in the extension list are root items,
        const auto &extensions = view3D->extensionList();
        for (const auto &ext : extensions) {
            const auto type = QQuick3DObjectPrivate::get(ext)->type;
            if (QSSGRenderGraphObject::isExtension(type)) {
                if (type == QSSGRenderGraphObject::Type::RenderExtension) {
                    if (auto *renderExt = qobject_cast<QQuick3DRenderExtension *>(ext)) {
                        const auto mode = static_cast<QSSGRenderExtension *>(QQuick3DObjectPrivate::get(renderExt)->spatialNode)->mode();
                        QSSG_ASSERT(size_t(mode) < std::size(m_layer->renderExtensions), continue);
                        auto &list = m_layer->renderExtensions[size_t(mode)];
                        bfs(qobject_cast<QQuick3DRenderExtension *>(ext), list);
                    }
                }
            }
        }

        view3D->clearExtensionListDirty();
    }

    // Update the layer node properties
    updateLayerNode(view3D, resourceLoaders.values());

    bool postProcessingNeeded = m_layer->firstEffect;
    bool postProcessingWasActive = m_effectSystem;
    QSSGRenderTextureFormat::Format effectOutputFormatOverride = QSSGRenderTextureFormat::Unknown;
    if (postProcessingNeeded) {
        QSSGRenderEffect *lastEffect = m_layer->firstEffect;
        while (lastEffect->m_nextEffect)
            lastEffect = lastEffect->m_nextEffect;
        effectOutputFormatOverride = QSSGRhiEffectSystem::overriddenOutputFormat(lastEffect);
    }
    const auto layerTextureFormat = [effectOutputFormatOverride, view3D](QRhi *rhi, bool postProc) {
        if (effectOutputFormatOverride != QSSGRenderTextureFormat::Unknown)
            return QSSGBufferManager::toRhiFormat(effectOutputFormatOverride);

        // Our standard choice for the postprocessing input/output textures'
        // format is a floating point one. (unlike intermediate Buffers, which
        // default to RGBA8 unless the format is explicitly specified)
        // This is intentional since a float format allows passing in
        // non-tonemapped content without colors being clamped when written out
        // to the render target.
        //
        // When it comes to the output, this applies to that too due to
        // QSSGRhiEffectSystem picking it up unless overridden (with a Buffer
        // an empty 'name'). Here too a float format gives more flexibility:
        // the effect may or may not do its own tonemapping and this approach
        // is compatible with potential future on-screen HDR output support.

        const QRhiTexture::Format preferredPostProcFormat = QRhiTexture::RGBA16F;
        if (postProc && rhi->isTextureFormatSupported(preferredPostProcFormat))
            return preferredPostProcFormat;

        const QRhiTexture::Format preferredView3DFormat = toRhiTextureFormat(view3D->renderFormat());
        if (rhi->isTextureFormatSupported(preferredView3DFormat))
            return preferredView3DFormat;

        return QRhiTexture::RGBA8;
    };
    bool postProcessingStateDirty = postProcessingNeeded != postProcessingWasActive;

    // Store from the layer properties the ones we need to handle ourselves (with the RHI code path)
    m_backgroundMode = QSSGRenderLayer::Background(view3D->environment()->backgroundMode());

    // This is stateful since we only want to recalculate the tonemapped color
    // when the color changes, not in every frame.
    QColor currentUserBackgroundColor = view3D->environment()->clearColor();
    if (m_userBackgroundColor != currentUserBackgroundColor) {
        m_userBackgroundColor = currentUserBackgroundColor;
        m_linearBackgroundColor = QSSGUtils::color::sRGBToLinearColor(m_userBackgroundColor);
        const QVector3D tc = tonemapRgb(QVector3D(m_linearBackgroundColor.redF(),
                                                  m_linearBackgroundColor.greenF(),
                                                  m_linearBackgroundColor.blueF()),
                                        view3D->environment()->tonemapMode());
        m_tonemappedBackgroundColor = QColor::fromRgbF(tc.x(), tc.y(), tc.z(), m_linearBackgroundColor.alphaF());
    }
    m_layer->scissorRect = QRect(view3D->environment()->scissorRect().topLeft() * dpr,
                                 view3D->environment()->scissorRect().size() * dpr);

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
    QSSGRenderNode *importRootNode = nullptr;
    if (importScene)
        importRootNode = static_cast<QSSGRenderNode*>(QQuick3DObjectPrivate::get(importScene)->spatialNode);

    if (importRootNode != m_importRootNode) {
        if (m_importRootNode)
            m_layer->removeImportScene(*m_importRootNode);

        if (importRootNode) {
            // if importScene has the rendered viewport as ancestor, it probably means
            // "importScene: MyScene { }" type of inclusion.
            // In this case don't duplicate content by adding it again.
            QObject *sceneParent = importScene->parent();
            bool isEmbedded = false;
            while (sceneParent) {
                if (sceneParent == view3D) {
                    isEmbedded = true;
                    break;
                }
                sceneParent = sceneParent->parent();
            }
            if (!isEmbedded)
                m_layer->setImportScene(*importRootNode);
        }

        m_importRootNode = importRootNode;
    }

    if (auto lightmapBaker = view3D->maybeLightmapBaker()) {
        if (lightmapBaker->m_bakingRequested) {
            m_layer->renderData->interactiveLightmapBakingRequested = true;

            QQuick3DLightmapBaker::Callback qq3dCallback = lightmapBaker->m_callback;
            QQuick3DLightmapBaker::BakingControl *qq3dBakingControl = lightmapBaker->m_bakingControl;
            QSSGLightmapper::Callback callback =
                    [qq3dCallback, qq3dBakingControl](
                    QSSGLightmapper::BakingStatus qssgBakingStatus,
                    std::optional<QString> msg,
                    QSSGLightmapper::BakingControl *qssgBakingControl)
            {
                QQuick3DLightmapBaker::BakingStatus qq3dBakingStatus = QQuick3DLightmapBaker::BakingStatus::None;
                switch (qssgBakingStatus)
                {
                case QSSGLightmapper::BakingStatus::None:
                    break;
                case QSSGLightmapper::BakingStatus::Progress:
                    qq3dBakingStatus = QQuick3DLightmapBaker::BakingStatus::Progress;
                    break;
                case QSSGLightmapper::BakingStatus::Warning:
                    qq3dBakingStatus = QQuick3DLightmapBaker::BakingStatus::Warning;
                    break;
                case QSSGLightmapper::BakingStatus::Error:
                    qq3dBakingStatus = QQuick3DLightmapBaker::BakingStatus::Error;
                    break;
                case QSSGLightmapper::BakingStatus::Cancelled:
                    qq3dBakingStatus = QQuick3DLightmapBaker::BakingStatus::Cancelled;
                    break;
                case QSSGLightmapper::BakingStatus::Complete:
                    qq3dBakingStatus = QQuick3DLightmapBaker::BakingStatus::Complete;
                    break;
                }

                qq3dCallback(qq3dBakingStatus, msg, qq3dBakingControl);

                if (qq3dBakingControl->isCancelled() && !qssgBakingControl->cancelled)
                    qssgBakingControl->cancelled = true;
            };

            m_layer->renderData->lightmapBakingOutputCallback = callback;
            lightmapBaker->m_bakingRequested = false;
        }
    }

    const bool progressiveAA = m_layer->antialiasingMode == QSSGRenderLayer::AAMode::ProgressiveAA;
    const bool multiSamplingAA = m_layer->antialiasingMode == QSSGRenderLayer::AAMode::MSAA;
    const bool temporalAA = m_layer->temporalAAEnabled && !multiSamplingAA;
    const bool superSamplingAA = m_layer->antialiasingMode == QSSGRenderLayer::AAMode::SSAA;
    const bool timeBasedAA = progressiveAA || temporalAA;
    m_postProcessingStack = m_layer->firstEffect || timeBasedAA  || superSamplingAA;
    bool useFBO = view3D->renderMode() == QQuick3DViewport::RenderMode::Offscreen ||
                                          ((view3D->renderMode() == QQuick3DViewport::RenderMode::Underlay || view3D->renderMode() == QQuick3DViewport::RenderMode::Overlay)
                                           && m_postProcessingStack);
    if (useFBO && rhiCtx->isValid()) {
        QRhi *rhi = rhiCtx->rhi();
        const QSize renderSize = superSamplingAA ? m_surfaceSize * m_ssaaMultiplier : m_surfaceSize;

        if (m_texture) {
            // the size changed, or the AA settings changed, or toggled between some effects - no effect
            if (layerSizeIsDirty || postProcessingStateDirty) {
                m_texture->setPixelSize(m_surfaceSize);
                m_texture->setFormat(layerTextureFormat(rhi, postProcessingNeeded));
                m_texture->create();

                // If AA settings changed, then we drop and recreate all
                // resources, otherwise use a lighter path if just the size
                // changed.
                if (!m_aaIsDirty) {
                    // A special case: when toggling effects and AA is on,
                    // use the heavier AA path because the renderbuffer for
                    // MSAA and texture for SSAA may need a different
                    // format now since m_texture's format could have
                    // changed between RBGA8 and RGBA16F (due to layerTextureFormat()).
                    if (postProcessingStateDirty && (m_layer->antialiasingMode != QSSGRenderLayer::AAMode::NoAA || temporalAA)) {
                        releaseAaDependentRhiResources();
                    } else {
                        if (m_ssaaTexture) {
                            m_ssaaTexture->setPixelSize(renderSize);
                            m_ssaaTexture->create();
                        }
                        m_depthStencilBuffer->setPixelSize(renderSize);
                        m_depthStencilBuffer->create();
                        if (m_msaaRenderBuffer) {
                            m_msaaRenderBuffer->setPixelSize(renderSize);
                            m_msaaRenderBuffer->create();
                        }
                        // Toggling effects on and off will change the format
                        // (assuming effects default to a floating point
                        // format) and that needs on a different renderpass on
                        // Vulkan. Hence renewing m_textureRenderPassDescriptor as well.
                        if (postProcessingStateDirty) {
                            delete m_textureRenderPassDescriptor;
                            m_textureRenderPassDescriptor = m_textureRenderTarget->newCompatibleRenderPassDescriptor();
                            m_textureRenderTarget->setRenderPassDescriptor(m_textureRenderPassDescriptor);
                        }
                        m_textureRenderTarget->create();
                        if (m_ssaaTextureToTextureRenderTarget)
                            m_ssaaTextureToTextureRenderTarget->create();

                        if (m_temporalAATexture) {
                            m_temporalAATexture->setPixelSize(renderSize);
                            m_temporalAATexture->create();
                        }
                        if (m_prevTempAATexture) {
                            m_prevTempAATexture->setPixelSize(renderSize);
                            m_prevTempAATexture->create();
                        }
                        if (m_temporalAARenderTarget)
                            m_temporalAARenderTarget->create();
                    }
                }
            } else if (m_aaIsDirty && rhi->backend() == QRhi::Metal) { // ### to avoid garbage upon enabling MSAA with macOS 10.14 (why is this needed?)
                m_texture->create();
            }

            if (m_aaIsDirty)
                releaseAaDependentRhiResources();
        }

        const QRhiTexture::Flags textureFlags = QRhiTexture::RenderTarget
                | QRhiTexture::UsedAsTransferSource; // transfer source is for progressive/temporal AA
        const QRhiTexture::Format textureFormat = layerTextureFormat(rhi, postProcessingNeeded);

        if (!m_texture) {
            m_texture = rhi->newTexture(textureFormat, m_surfaceSize, 1, textureFlags);
            m_texture->create();
        }

        if (!m_ssaaTexture && superSamplingAA) {
            m_ssaaTexture = rhi->newTexture(textureFormat, renderSize, 1, textureFlags);
            m_ssaaTexture->create();
        }

        if (timeBasedAA && !m_temporalAATexture) {
            m_temporalAATexture = rhi->newTexture(textureFormat, renderSize, 1, textureFlags);
            m_temporalAATexture->create();
            m_prevTempAATexture = rhi->newTexture(textureFormat, renderSize, 1, textureFlags);
            m_prevTempAATexture->create();
        }

        // we need to re-render time-based AA not only when AA state changes, but also when resized
        if (m_aaIsDirty || layerSizeIsDirty)
            m_layer->tempAAPassIndex = m_layer->progAAPassIndex = 0;

        if (m_aaIsDirty) {
            m_samples = 1;
            if (m_layer->antialiasingMode == QSSGRenderLayer::AAMode::MSAA) {
                if (rhi->isFeatureSupported(QRhi::MultisampleRenderBuffer)) {
                    m_samples = qMax(1, int(m_layer->antialiasingQuality));
                    // The Quick3D API exposes high level values such as
                    // Medium, High, VeryHigh instead of direct sample
                    // count values. Therefore, be nice and find a sample
                    // count that's actually supported in case the one
                    // associated by default is not.
                    const QVector<int> supported = rhi->supportedSampleCounts(); // assumed to be sorted
                    if (!supported.contains(m_samples)) {
                        if (!supported.isEmpty()) {
                            auto it = std::lower_bound(supported.cbegin(), supported.cend(), m_samples);
                            m_samples = it == supported.cend() ? supported.last() : *it;
                        } else {
                            m_samples = 1;
                        }
                    }
                } else {
                    static bool warned = false;
                    if (!warned) {
                        warned = true;
                        qWarning("Multisample renderbuffers are not supported, disabling MSAA for Offscreen View3D");
                    }
                }
            }
        }

        if (!m_depthStencilBuffer) {
            m_depthStencilBuffer = rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, renderSize, m_samples);
            m_depthStencilBuffer->create();
        }

        if (!m_textureRenderTarget) {
            QRhiTextureRenderTargetDescription rtDesc;
            if (m_samples > 1) {
                // pass in the texture's format (which may be a floating point one!) as the preferred format hint
                m_msaaRenderBuffer = rhi->newRenderBuffer(QRhiRenderBuffer::Color, renderSize, m_samples, {}, m_texture->format());
                m_msaaRenderBuffer->create();
                QRhiColorAttachment att;
                att.setRenderBuffer(m_msaaRenderBuffer);
                att.setResolveTexture(m_texture);
                rtDesc.setColorAttachments({ att });
            } else {
                if (m_layer->antialiasingMode == QSSGRenderLayer::AAMode::SSAA)
                    rtDesc.setColorAttachments({ m_ssaaTexture });
                else
                    rtDesc.setColorAttachments({ m_texture });
            }
            rtDesc.setDepthStencilBuffer(m_depthStencilBuffer);

            m_textureRenderTarget = rhi->newTextureRenderTarget(rtDesc);
            m_textureRenderTarget->setName(QByteArrayLiteral("View3D"));
            m_textureRenderPassDescriptor = m_textureRenderTarget->newCompatibleRenderPassDescriptor();
            m_textureRenderTarget->setRenderPassDescriptor(m_textureRenderPassDescriptor);
            m_textureRenderTarget->create();
        }

        if (!m_ssaaTextureToTextureRenderTarget && m_layer->antialiasingMode == QSSGRenderLayer::AAMode::SSAA) {
            m_ssaaTextureToTextureRenderTarget = rhi->newTextureRenderTarget({ m_texture });
            m_ssaaTextureToTextureRenderTarget->setName(QByteArrayLiteral("SSAA texture"));
            m_ssaaTextureToTextureRenderPassDescriptor = m_ssaaTextureToTextureRenderTarget->newCompatibleRenderPassDescriptor();
            m_ssaaTextureToTextureRenderTarget->setRenderPassDescriptor(m_ssaaTextureToTextureRenderPassDescriptor);
            m_ssaaTextureToTextureRenderTarget->create();
        }

        if (m_layer->firstEffect) {
            if (!m_effectSystem)
                m_effectSystem = new QSSGRhiEffectSystem(m_sgContext);
            m_effectSystem->setup(renderSize);
        } else if (m_effectSystem) {
            delete m_effectSystem;
            m_effectSystem = nullptr;
        }

        if (timeBasedAA && !m_temporalAARenderTarget) {
            m_temporalAARenderTarget = rhi->newTextureRenderTarget({ m_temporalAATexture });
            m_temporalAARenderTarget->setName(QByteArrayLiteral("Temporal AA texture"));
            m_temporalAARenderPassDescriptor = m_temporalAARenderTarget->newCompatibleRenderPassDescriptor();
            m_temporalAARenderTarget->setRenderPassDescriptor(m_temporalAARenderPassDescriptor);
            m_temporalAARenderTarget->create();
        }

        m_textureNeedsFlip = rhi->isYUpInFramebuffer();
        m_aaIsDirty = false;
    }

    if (m_renderStats)
        m_renderStats->endSync(dumpRenderTimes);

    Q_QUICK3D_PROFILE_END_WITH_ID(QQuick3DProfiler::Quick3DSynchronizeFrame, quint64(m_surfaceSize.width()) | quint64(m_surfaceSize.height()) << 32, profilingId);
}

void QQuick3DSceneRenderer::invalidateFramebufferObject()
{
    if (fboNode)
        fboNode->invalidatePending = true;
}

void QQuick3DSceneRenderer::releaseCachedResources()
{
    if (m_layer && m_layer->renderData) {
        if (const auto &mgr = m_layer->renderData->getShadowMapManager())
            mgr->releaseCachedResources();
        if (const auto &mgr = m_layer->renderData->getReflectionMapManager())
            mgr->releaseCachedResources();
    }
}

std::optional<QSSGRenderRay> QQuick3DSceneRenderer::getRayFromViewportPos(const QPointF &pos)
{
    if (!m_layer || !m_layer->renderedCamera)
        return std::nullopt;

    const QVector2D viewportSize(m_surfaceSize.width(), m_surfaceSize.height());
    const QVector2D position(float(pos.x()), float(pos.y()));
    const QRectF viewportRect(QPointF{}, QSizeF(m_surfaceSize));

    // First invert the y so we are dealing with numbers in a normal coordinate space.
    // Second, move into our layer's coordinate space
    QVector2D correctCoords(position.x(), viewportSize.y() - position.y());
    QVector2D theLocalMouse = QSSGUtils::rect::toRectRelative(viewportRect, correctCoords);
    if ((theLocalMouse.x() < 0.0f || theLocalMouse.x() >= viewportSize.x() || theLocalMouse.y() < 0.0f
         || theLocalMouse.y() >= viewportSize.y()))
        return std::nullopt;

    return m_layer->renderedCamera->unproject(theLocalMouse, viewportRect);
}

QSSGRenderPickResult QQuick3DSceneRenderer::syncPick(const QSSGRenderRay &ray)
{
    if (!m_layer)
        return QSSGRenderPickResult();

    return m_sgContext->renderer()->syncPick(*m_layer,
                                             *m_sgContext->bufferManager(),
                                             ray);
}

QSSGRenderPickResult QQuick3DSceneRenderer::syncPickOne(const QSSGRenderRay &ray, QSSGRenderNode *node)
{
    if (!m_layer)
        return QSSGRenderPickResult();

    return m_sgContext->renderer()->syncPick(*m_layer,
                                             *m_sgContext->bufferManager(),
                                             ray,
                                             node);
}

QQuick3DSceneRenderer::PickResultList QQuick3DSceneRenderer::syncPickAll(const QSSGRenderRay &ray)
{
    if (!m_layer)
        return QQuick3DSceneRenderer::PickResultList();

    return m_sgContext->renderer()->syncPickAll(*m_layer,
                                                *m_sgContext->bufferManager(),
                                                ray);
}

void QQuick3DSceneRenderer::setGlobalPickingEnabled(bool isEnabled)
{
    m_sgContext->renderer()->setGlobalPickingEnabled(isEnabled);
}

QQuick3DRenderStats *QQuick3DSceneRenderer::renderStats()
{
    return m_renderStats;
}

void QQuick3DRenderLayerHelpers::updateLayerNodeHelper(const QQuick3DViewport &view3D, QSSGRenderLayer &layerNode, bool &aaIsDirty, bool &temporalIsDirty, float &ssaaMultiplier)
{
    QQuick3DSceneEnvironment *environment = view3D.environment();

    QSSGRenderLayer::AAMode aaMode = QSSGRenderLayer::AAMode(environment->antialiasingMode());
    if (aaMode != layerNode.antialiasingMode) {
        layerNode.antialiasingMode = aaMode;
        layerNode.progAAPassIndex = 0;
        aaIsDirty = true;
    }
    QSSGRenderLayer::AAQuality aaQuality = QSSGRenderLayer::AAQuality(environment->antialiasingQuality());
    if (aaQuality != layerNode.antialiasingQuality) {
        layerNode.antialiasingQuality = aaQuality;
        ssaaMultiplier = (aaQuality == QSSGRenderLayer::AAQuality::Normal) ? 1.2f :
                                                                               (aaQuality == QSSGRenderLayer::AAQuality::High) ? 1.5f :
                                                                                                                                 2.0f;
        layerNode.ssaaMultiplier = ssaaMultiplier;
        aaIsDirty = true;
    }

    bool temporalAAEnabled = environment->temporalAAEnabled();
    if (temporalAAEnabled != layerNode.temporalAAEnabled) {
        layerNode.temporalAAEnabled = environment->temporalAAEnabled();
        temporalIsDirty = true;

        layerNode.tempAAPassIndex = 0;
        aaIsDirty = true;
    }
    layerNode.ssaaEnabled = environment->antialiasingMode()
            == QQuick3DSceneEnvironment::QQuick3DEnvironmentAAModeValues::SSAA;

    layerNode.temporalAAStrength = environment->temporalAAStrength();

    layerNode.specularAAEnabled = environment->specularAAEnabled();

    layerNode.background = QSSGRenderLayer::Background(environment->backgroundMode());
    layerNode.clearColor = QVector3D(float(environment->clearColor().redF()),
                                      float(environment->clearColor().greenF()),
                                      float(environment->clearColor().blueF()));

    layerNode.gridEnabled = environment->gridEnabled();
    layerNode.gridScale = environment->gridScale();
    layerNode.gridFlags = environment->gridFlags();

    layerNode.aoStrength = environment->aoStrength();
    layerNode.aoDistance = environment->aoDistance();
    layerNode.aoSoftness = environment->aoSoftness();
    layerNode.aoEnabled = environment->aoEnabled();
    layerNode.aoBias = environment->aoBias();
    layerNode.aoSamplerate = environment->aoSampleRate();
    layerNode.aoDither = environment->aoDither();

    // ### These images will not be registered anywhere
    if (environment->lightProbe())
        layerNode.lightProbe = environment->lightProbe()->getRenderImage();
    else
        layerNode.lightProbe = nullptr;
    if (view3D.environment()->skyBoxCubeMap())
        layerNode.skyBoxCubeMap = view3D.environment()->skyBoxCubeMap()->getRenderImage();
    else
        layerNode.skyBoxCubeMap = nullptr;

    layerNode.probeExposure = environment->probeExposure();
    // Remap the probeHorizon to the expected Range
    layerNode.probeHorizon = qMin(environment->probeHorizon() - 1.0f, -0.001f);
    layerNode.setProbeOrientation(environment->probeOrientation());

    if (view3D.camera())
        layerNode.explicitCamera = static_cast<QSSGRenderCamera *>(QQuick3DObjectPrivate::get(view3D.camera())->spatialNode);
    else
        layerNode.explicitCamera = nullptr;

    layerNode.layerFlags.setFlag(QSSGRenderLayer::LayerFlag::EnableDepthTest, environment->depthTestEnabled());
    layerNode.layerFlags.setFlag(QSSGRenderLayer::LayerFlag::EnableDepthPrePass, environment->depthPrePassEnabled());

    layerNode.tonemapMode = QQuick3DSceneRenderer::getTonemapMode(*environment);
    layerNode.skyboxBlurAmount = environment->skyboxBlurAmount();
    if (auto debugSettings = view3D.environment()->debugSettings()) {
        layerNode.debugMode = QSSGRenderLayer::MaterialDebugMode(debugSettings->materialOverride());
        layerNode.wireframeMode = debugSettings->wireframeEnabled();
    } else {
        layerNode.debugMode = QSSGRenderLayer::MaterialDebugMode::None;
        layerNode.wireframeMode = false;
    }

    if (environment->lightmapper()) {
        QQuick3DLightmapper *lightmapper = environment->lightmapper();
        layerNode.lmOptions.opacityThreshold = lightmapper->opacityThreshold();
        layerNode.lmOptions.bias = lightmapper->bias();
        layerNode.lmOptions.useAdaptiveBias = lightmapper->isAdaptiveBiasEnabled();
        layerNode.lmOptions.indirectLightEnabled = lightmapper->isIndirectLightEnabled();
        layerNode.lmOptions.indirectLightSamples = lightmapper->samples();
        layerNode.lmOptions.indirectLightWorkgroupSize = lightmapper->indirectLightWorkgroupSize();
        layerNode.lmOptions.indirectLightBounces = lightmapper->bounces();
        layerNode.lmOptions.indirectLightFactor = lightmapper->indirectLightFactor();
    } else {
        layerNode.lmOptions = {};
    }

    if (environment->fog() && environment->fog()->isEnabled()) {
        layerNode.fog.enabled = true;
        const QQuick3DFog *fog = environment->fog();
        layerNode.fog.color = QSSGUtils::color::sRGBToLinear(fog->color()).toVector3D();
        layerNode.fog.density = fog->density();
        layerNode.fog.depthEnabled = fog->isDepthEnabled();
        layerNode.fog.depthBegin = fog->depthNear();
        layerNode.fog.depthEnd = fog->depthFar();
        layerNode.fog.depthCurve = fog->depthCurve();
        layerNode.fog.heightEnabled = fog->isHeightEnabled();
        layerNode.fog.heightMin = fog->leastIntenseY();
        layerNode.fog.heightMax = fog->mostIntenseY();
        layerNode.fog.heightCurve = fog->heightCurve();
        layerNode.fog.transmitEnabled = fog->isTransmitEnabled();
        layerNode.fog.transmitCurve = fog->transmitCurve();
    } else {
        layerNode.fog.enabled = false;
    }
}

void QQuick3DSceneRenderer::updateLayerNode(QQuick3DViewport *view3D, const QList<QSSGRenderGraphObject *> &resourceLoaders)
{
    QSSGRenderLayer *layerNode = m_layer;

    bool temporalIsDirty = false;
    QQuick3DRenderLayerHelpers::updateLayerNodeHelper(*view3D, *m_layer, m_aaIsDirty, temporalIsDirty, m_ssaaMultiplier);

    int extraFramesToRender = 0;

    if (layerNode->antialiasingMode == QSSGRenderLayer::AAMode::ProgressiveAA) {
        // with progressive AA, we need a number of extra frames after the last dirty one
        // if we always reset requestedFramesCount when dirty, we will get the extra frames eventually
        // +1 since we need a normal frame to start with, and we're not copying that from the screen
        extraFramesToRender = int(layerNode->antialiasingQuality) + 1;
    } else if (layerNode->temporalAAEnabled) {
        // When temporalAA is on and antialiasing mode changes,
        // layer needs to be re-rendered (at least) MAX_TEMPORAL_AA_LEVELS times
        // to generate temporal antialiasing.
        // Also, we need to do an extra render when animation stops
        extraFramesToRender = (m_aaIsDirty || temporalIsDirty) ? QSSGLayerRenderData::MAX_TEMPORAL_AA_LEVELS : 1;
    }

    requestedFramesCount = extraFramesToRender;
    // Effects need to be rendered in reverse order as described in the file.
    layerNode->firstEffect = nullptr; // We reset the linked list
    const auto &effects = view3D->environment()->effectList();
    auto rit = effects.crbegin();
    const auto rend = effects.crend();
    for (; rit != rend; ++rit) {
        QQuick3DObjectPrivate *p = QQuick3DObjectPrivate::get(*rit);
        QSSGRenderEffect *effectNode = static_cast<QSSGRenderEffect *>(p->spatialNode);
        if (effectNode) {
            if (layerNode->hasEffect(effectNode)) {
                qWarning() << "Duplicate effect found, skipping!";
            } else {
                effectNode->className = (*rit)->metaObject()->className(); //### persistent, but still icky to store a const char* returned from a function
                layerNode->addEffect(*effectNode);
            }
        }
    }

    // Now that we have the effect list used for rendering, finalize the shader
    // code based on the layer (scene.env.) settings.
    for (QSSGRenderEffect *effectNode = layerNode->firstEffect; effectNode; effectNode = effectNode->m_nextEffect)
        effectNode->finalizeShaders(*layerNode, m_sgContext.get());

    // ResourceLoaders
    layerNode->resourceLoaders.clear();
    layerNode->resourceLoaders = resourceLoaders;
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

QSGRenderNode::StateFlags QQuick3DSGRenderNode::changedStates() const
{
    return BlendState | StencilState | DepthState | ScissorState | ColorState | CullState | ViewportState | RenderTargetState;
}

namespace {
inline QRect convertQtRectToGLViewport(const QRectF &rect, const QSize surfaceSize)
{
    const int x = int(rect.x());
    const int y = surfaceSize.height() - (int(rect.y()) + int(rect.height()));
    const int width = int(rect.width());
    const int height = int(rect.height());
    return QRect(x, y, width, height);
}

inline void queryMainRenderPassDescriptorAndCommandBuffer(QQuickWindow *window, QSSGRhiContext *rhiCtx)
{
    if (rhiCtx->isValid()) {
        // Query from the rif because that is available in the sync
        // phase (updatePaintNode) already.  QSGDefaultRenderContext's
        // copies of the rp and cb are not there until the render
        // phase of the scenegraph.
        int sampleCount = 1;
        QRhiSwapChain *swapchain = window->swapChain();
        if (swapchain) {
            rhiCtx->setMainRenderPassDescriptor(swapchain->renderPassDescriptor());
            rhiCtx->setCommandBuffer(swapchain->currentFrameCommandBuffer());
            rhiCtx->setRenderTarget(swapchain->currentFrameRenderTarget());
            sampleCount = swapchain->sampleCount();
        } else {
            QSGRendererInterface *rif = window->rendererInterface();
            // no swapchain when using a QQuickRenderControl (redirecting to a texture etc.)
            QRhiCommandBuffer *cb = static_cast<QRhiCommandBuffer *>(
                rif->getResource(window, QSGRendererInterface::RhiRedirectCommandBuffer));
            QRhiTextureRenderTarget *rt = static_cast<QRhiTextureRenderTarget *>(
                rif->getResource(window, QSGRendererInterface::RhiRedirectRenderTarget));
            if (cb && rt) {
                rhiCtx->setMainRenderPassDescriptor(rt->renderPassDescriptor());
                rhiCtx->setCommandBuffer(cb);
                rhiCtx->setRenderTarget(rt);
                const QRhiColorAttachment *color0 = rt->description().cbeginColorAttachments();
                if (color0 && color0->texture())
                    sampleCount = color0->texture()->sampleCount();
            } else {
                qWarning("Neither swapchain nor redirected command buffer and render target are available.");
            }
        }

        // MSAA is out of our control on this path: it is up to the
        // QQuickWindow and the scenegraph to set up the swapchain based on the
        // QSurfaceFormat's samples(). The only thing we need to do here is to
        // pass the sample count to the renderer because it is needed when
        // creating graphics pipelines.
        rhiCtx->setMainPassSampleCount(sampleCount);
    }
}

// The alternative to queryMainRenderPassDescriptorAndCommandBuffer()
// specifically for the Inline render mode when there is a QSGRenderNode.
inline void queryInlineRenderPassDescriptorAndCommandBuffer(QSGRenderNode *node, QSSGRhiContext *rhiCtx)
{
    QSGRenderNodePrivate *d = QSGRenderNodePrivate::get(node);
    rhiCtx->setMainRenderPassDescriptor(d->m_rt.rpDesc);
    rhiCtx->setCommandBuffer(d->m_rt.cb);
    rhiCtx->setRenderTarget(d->m_rt.rt);
    rhiCtx->setMainPassSampleCount(d->m_rt.rt->sampleCount());
}
}

QQuick3DSGRenderNode::~QQuick3DSGRenderNode()
{
    delete renderer;
}

void QQuick3DSGRenderNode::prepare()
{
    // this is outside the main renderpass

    if (!renderer->m_sgContext->rhiContext()->isValid())
        return;
    Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DPrepareFrame);

    queryInlineRenderPassDescriptorAndCommandBuffer(this, renderer->m_sgContext->rhiContext().get());

    qreal dpr = window->effectiveDevicePixelRatio();
    const QSizeF itemSize = renderer->surfaceSize() / dpr;
    QRectF viewport = matrix()->mapRect(QRectF(QPoint(0, 0), itemSize));
    viewport = QRectF(viewport.topLeft() * dpr, viewport.size() * dpr);
    const QRect vp = convertQtRectToGLViewport(viewport, window->size() * dpr);

    Q_TRACE_SCOPE(QSSG_prepareFrame, vp.width(), vp.height());

    renderer->beginFrame();
    renderer->rhiPrepare(vp, dpr);
    Q_QUICK3D_PROFILE_END_WITH_ID(QQuick3DProfiler::Quick3DPrepareFrame, quint64(vp.width()) | quint64(vp.height()) << 32, renderer->profilingId);
}

void QQuick3DSGRenderNode::render(const QSGRenderNode::RenderState *state)
{
    Q_UNUSED(state);

    if (renderer->m_sgContext->rhiContext()->isValid()) {
        Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DRenderFrame);
        Q_TRACE_SCOPE(QSSG_renderFrame, 0, 0);

        queryInlineRenderPassDescriptorAndCommandBuffer(this, renderer->m_sgContext->rhiContext().get());

        renderer->rhiRender();
        renderer->endFrame();
        Q_QUICK3D_PROFILE_END_WITH_ID(QQuick3DProfiler::Quick3DRenderFrame,
                                        STAT_PAYLOAD(renderer->m_sgContext->rhiContext()->stats()), renderer->profilingId);
    }
}

void QQuick3DSGRenderNode::releaseResources()
{
}

QSGRenderNode::RenderingFlags QQuick3DSGRenderNode::flags() const
{
    // don't want begin/endExternal() to be called by Quick
    return NoExternalRendering;
}

QQuick3DSGDirectRenderer::QQuick3DSGDirectRenderer(QQuick3DSceneRenderer *renderer, QQuickWindow *window, QQuick3DSGDirectRenderer::QQuick3DSGDirectRendererMode mode)
    : m_renderer(renderer)
    , m_window(window)
    , m_mode(mode)
{
    if (QSGRendererInterface::isApiRhiBased(window->rendererInterface()->graphicsApi())) {
        connect(window, &QQuickWindow::beforeRendering, this, &QQuick3DSGDirectRenderer::prepare, Qt::DirectConnection);
        if (mode == Underlay)
            connect(window, &QQuickWindow::beforeRenderPassRecording, this, &QQuick3DSGDirectRenderer::render, Qt::DirectConnection);
        else
            connect(window, &QQuickWindow::afterRenderPassRecording, this, &QQuick3DSGDirectRenderer::render, Qt::DirectConnection);
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

void QQuick3DSGDirectRenderer::setVisibility(bool visible)
{
    if (m_isVisible == visible)
        return;
    m_isVisible = visible;
    m_window->update();
}

void QQuick3DSGDirectRenderer::requestRender()
{
    renderPending = true;
    requestFullUpdate(m_window);
}

void QQuick3DSGDirectRenderer::prepare()
{
    if (!m_isVisible || !m_renderer)
        return;

    if (m_renderer->m_sgContext->rhiContext()->isValid()) {
        // this is outside the main renderpass
        if (m_renderer->m_postProcessingStack) {
            if (renderPending) {
                renderPending = false;
                m_rhiTexture = m_renderer->renderToRhiTexture(m_window);
                queryMainRenderPassDescriptorAndCommandBuffer(m_window, m_renderer->m_sgContext->rhiContext().get());
                auto quadRenderer = m_renderer->m_sgContext->renderer()->rhiQuadRenderer();
                quadRenderer->prepareQuad(m_renderer->m_sgContext->rhiContext().get(), nullptr);
                if (m_renderer->requestedFramesCount > 0) {
                    requestRender();
                    m_renderer->requestedFramesCount--;
                }
            }
        }
        else
        {
            QQuick3DRenderStats *renderStats = m_renderer->renderStats();
            if (renderStats) {
                renderStats->startRender();
                renderStats->startRenderPrepare();
            }

            Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DPrepareFrame);
            queryMainRenderPassDescriptorAndCommandBuffer(m_window, m_renderer->m_sgContext->rhiContext().get());
            const QRect vp = convertQtRectToGLViewport(m_viewport, m_window->size() * m_window->effectiveDevicePixelRatio());

            Q_TRACE_SCOPE(QSSG_prepareFrame, vp.width(), vp.height());
            m_renderer->beginFrame();
            m_renderer->rhiPrepare(vp, m_window->effectiveDevicePixelRatio());
            Q_QUICK3D_PROFILE_END_WITH_ID(QQuick3DProfiler::Quick3DPrepareFrame, quint64(vp.width()) | quint64(vp.height()) << 32, m_renderer->profilingId);

            if (renderStats)
                renderStats->endRenderPrepare();
        }
    }
}

void QQuick3DSGDirectRenderer::render()
{
    if (!m_isVisible || !m_renderer)
        return;

    if (m_renderer->m_sgContext->rhiContext()->isValid()) {
        // the command buffer is recording the main renderpass at this point

        // No m_window->beginExternalCommands() must be done here. When the
        // renderer is using the same
        // QRhi/QRhiCommandBuffer/QRhiRenderPassDescriptor as the Qt Quick
        // scenegraph, there is no difference from the RHI's perspective. There are
        // no external (native) commands here.

        // Requery the command buffer and co. since Offscreen mode View3Ds may
        // have altered these on the context.
        if (m_renderer->m_postProcessingStack) {
            if (m_rhiTexture) {
                queryMainRenderPassDescriptorAndCommandBuffer(m_window, m_renderer->m_sgContext->rhiContext().get());
                auto rhiCtx = m_renderer->m_sgContext->rhiContext().get();
                const auto &renderer = m_renderer->m_sgContext->renderer();

                // Instead of passing in a flip flag we choose to rely on qsb's
                // per-target compilation mode in the fragment shader. (it does UV
                // flipping based on QSHADER_ macros) This is just better for
                // performance and the shaders are very simple so introducing a
                // uniform block and branching dynamically would be an overkill.
                QRect vp = convertQtRectToGLViewport(m_viewport, m_window->size() * m_window->effectiveDevicePixelRatio());

                const auto &shaderPipeline = renderer->getRhiSimpleQuadShader();

                QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                                         QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge });
                QSSGRhiShaderResourceBindingList bindings;
                bindings.addTexture(0, QRhiShaderResourceBinding::FragmentStage, m_rhiTexture, sampler);
                QRhiShaderResourceBindings *srb = rhiCtx->srb(bindings);

                QSSGRhiGraphicsPipelineState ps;
                ps.viewport = QRhiViewport(float(vp.x()), float(vp.y()), float(vp.width()), float(vp.height()));
                ps.shaderPipeline = shaderPipeline.get();
                renderer->rhiQuadRenderer()->recordRenderQuad(rhiCtx, &ps, srb, rhiCtx->mainRenderPassDescriptor(), QSSGRhiQuadRenderer::UvCoords | QSSGRhiQuadRenderer::PremulBlend);
            }
        }
        else
        {
            Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DRenderFrame);
            Q_TRACE_SCOPE(QSSG_renderFrame, 0, 0);

            queryMainRenderPassDescriptorAndCommandBuffer(m_window, m_renderer->m_sgContext->rhiContext().get());

            m_renderer->rhiRender();
            m_renderer->endFrame();

            Q_QUICK3D_PROFILE_END_WITH_ID(QQuick3DProfiler::Quick3DRenderFrame,
                                            STAT_PAYLOAD(m_renderer->m_sgContext->rhiContext()->stats()),
                                            m_renderer->profilingId);

            if (m_renderer->renderStats())
                m_renderer->renderStats()->endRender(dumpRenderTimes);
        }
    }
}

QT_END_NAMESPACE
