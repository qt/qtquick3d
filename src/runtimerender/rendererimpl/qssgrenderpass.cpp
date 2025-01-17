// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qssgrenderpass_p.h"
#include "qssgrhiquadrenderer_p.h"
#include "qssglayerrenderdata_p.h"
#include "qssgrendercontextcore.h"
#include "qssgdebugdrawsystem_p.h"
#include "extensionapi/qssgrenderextensions.h"
#include "qssgrenderhelpers_p.h"

#include "../utils/qssgassert_p.h"

#include <QtQuick/private/qsgrenderer_p.h>
#include <qtquick3d_tracepoints_p.h>

QT_BEGIN_NAMESPACE

static inline QMatrix4x4 correctMVPForScissor(QRectF viewportRect, QRect scissorRect, bool isYUp) {
    const auto &scissorCenter = scissorRect.center();
    const auto &viewCenter = viewportRect.center();
    const float scaleX = viewportRect.width() / float(scissorRect.width());
    const float scaleY = viewportRect.height() / float(scissorRect.height());
    const float dx = 2 * (viewCenter.x() - scissorCenter.x()) / scissorRect.width();
    const float dyRect = isYUp ? (scissorCenter.y() - viewCenter.y())
                                : (viewCenter.y() - scissorCenter.y());
    const float dy = 2 * dyRect / scissorRect.height();

    return QMatrix4x4(scaleX, 0.0f, 0.0f, dx,
                      0.0f, scaleY, 0.0f, dy,
                      0.0f, 0.0f, 1.0f, 0.0f,
                      0.0f, 0.0f, 0.0f, 1.0f);
}

QSSGRenderPass::~QSSGRenderPass()
{

}

// SHADOW PASS

void ShadowMapPass::renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data)
{
    Q_UNUSED(renderer)
    using namespace RenderHelpers;

    QSSG_ASSERT(!data.renderedCameras.isEmpty(), return);
    camera = data.renderedCameras[0];

    const auto &renderedDepthWriteObjects = data.getSortedRenderedDepthWriteObjects(*camera);
    const auto &renderedOpaqueDepthPrepassObjects = data.getSortedrenderedOpaqueDepthPrepassObjects(*camera);

    QSSG_ASSERT(shadowPassObjects.isEmpty(), shadowPassObjects.clear());

    for (const auto &handles : { &renderedDepthWriteObjects, &renderedOpaqueDepthPrepassObjects }) {
        for (const auto &handle : *handles) {
            if (handle.obj->renderableFlags.castsShadows())
                shadowPassObjects.push_back(handle);
        }
    }

    globalLights = data.globalLights;

    enabled = !shadowPassObjects.isEmpty() || !globalLights.isEmpty();

    if (enabled) {
        shadowMapManager = data.requestShadowMapManager();

        ps = data.getPipelineState();
        ps.flags |= { QSSGRhiGraphicsPipelineState::Flag::DepthTestEnabled, QSSGRhiGraphicsPipelineState::Flag::DepthWriteEnabled };
        // Try reducing self-shadowing and artifacts.
        ps.depthBias = 2;
        ps.slopeScaledDepthBias = 1.5f;

        const auto &sortedOpaqueObjects = data.getSortedOpaqueRenderableObjects(*camera);
        const auto &sortedTransparentObjects = data.getSortedTransparentRenderableObjects(*camera);
        const auto [casting, receiving] = calculateSortedObjectBounds(sortedOpaqueObjects,
                                                                      sortedTransparentObjects);
        castingObjectsBox = casting;
        receivingObjectsBox = receiving;

        if (!debugCamera) {
            debugCamera = std::make_unique<QSSGRenderCamera>(QSSGRenderGraphObject::Type::OrthographicCamera);
        }
    }
}

void ShadowMapPass::renderPass(QSSGRenderer &renderer)
{
    using namespace RenderHelpers;

    // INPUT: Sorted opaque and transparent + depth, global lights (scoped lights not supported) and camera.

    // DEPENDECY: None

    // OUTPUT: Textures or cube maps

    // CONDITION: Lights (shadowPassObjects)

    if (enabled) {
        const auto &rhiCtx = renderer.contextInterface()->rhiContext();
        QSSG_ASSERT(rhiCtx->rhi()->isRecordingFrame(), return);
        QRhiCommandBuffer *cb = rhiCtx->commandBuffer();
        cb->debugMarkBegin(QByteArrayLiteral("Quick3D shadow map"));
        Q_TRACE_SCOPE(QSSG_renderPass, QStringLiteral("Quick3D shadow map"));
        Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DRenderPass);

        QSSG_CHECK(shadowMapManager);
        rhiRenderShadowMap(rhiCtx.get(),
                           this,
                           ps,
                           *shadowMapManager,
                           *camera,
                           debugCamera.get(),
                           globalLights, // scoped lights are not relevant here
                           shadowPassObjects,
                           renderer,
                           castingObjectsBox,
                           receivingObjectsBox);

        cb->debugMarkEnd();
        Q_QUICK3D_PROFILE_END_WITH_STRING(QQuick3DProfiler::Quick3DRenderPass, 0, QByteArrayLiteral("shadow_map"));
    }
}

void ShadowMapPass::resetForFrame()
{
    enabled = false;
    camera = nullptr;
    castingObjectsBox = {};
    receivingObjectsBox = {};
    ps = {};
    shadowPassObjects.clear();
    globalLights.clear();
}

// REFLECTIONMAP PASS

void ReflectionMapPass::renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data)
{
    Q_UNUSED(renderer);
    Q_UNUSED(data);

    QSSG_ASSERT(!data.renderedCameras.isEmpty(), return);
    QSSGRenderCamera *camera = data.renderedCameras[0];

    ps = data.getPipelineState();
    ps.flags |= { QSSGRhiGraphicsPipelineState::Flag::DepthTestEnabled,
                  QSSGRhiGraphicsPipelineState::Flag::DepthWriteEnabled,
                  QSSGRhiGraphicsPipelineState::Flag::BlendEnabled };

    reflectionProbes = data.reflectionProbes;
    reflectionMapManager = data.requestReflectionMapManager();

    const auto &sortedOpaqueObjects = data.getSortedOpaqueRenderableObjects(*camera);
    const auto &sortedTransparentObjects = data.getSortedTransparentRenderableObjects(*camera);
    const auto &sortedScreenTextureObjects = data.getSortedScreenTextureRenderableObjects(*camera);

    QSSG_ASSERT(reflectionPassObjects.isEmpty(), reflectionPassObjects.clear());

    // NOTE: We should consider keeping track of the reflection casting objects to avoid
    // filtering this list on each prep.
    for (const auto &handles : { &sortedOpaqueObjects, &sortedTransparentObjects, &sortedScreenTextureObjects }) {
        for (const auto &handle : *handles) {
            if (handle.obj->renderableFlags.testFlag(QSSGRenderableObjectFlag::CastsReflections))
                reflectionPassObjects.push_back(handle);
        }
    }
}

void ReflectionMapPass::renderPass(QSSGRenderer &renderer)
{
    using namespace RenderHelpers;

    // INPUT: Reflection probes, sorted opaque and transparent

    // DEPENDECY: None

    // OUTPUT: Cube maps (1 per probe)

    // NOTE: Full pass with a sky box pass

    // CONDITION: Probes and sorted opaque and transparent

    const auto &rhiCtx = renderer.contextInterface()->rhiContext();
    QSSG_ASSERT(rhiCtx->rhi()->isRecordingFrame(), return);
    QRhiCommandBuffer *cb = rhiCtx->commandBuffer();

    const auto *layerData = QSSGLayerRenderData::getCurrent(renderer);
    QSSG_ASSERT(layerData, return);

    QSSG_CHECK(reflectionMapManager);
    if (!reflectionPassObjects.isEmpty() || !reflectionProbes.isEmpty()) {
        cb->debugMarkBegin(QByteArrayLiteral("Quick3D reflection map"));
        Q_TRACE_SCOPE(QSSG_renderPass, QStringLiteral("Quick3D reflection map"));
        Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DRenderPass);
        rhiRenderReflectionMap(rhiCtx.get(),
                               this,
                               *layerData,
                               &ps,
                               *reflectionMapManager,
                               reflectionProbes,
                               reflectionPassObjects,
                               renderer);

        cb->debugMarkEnd();
        Q_QUICK3D_PROFILE_END_WITH_STRING(QQuick3DProfiler::Quick3DRenderPass, 0, QByteArrayLiteral("reflection_map"));
    }
}

void ReflectionMapPass::resetForFrame()
{
    ps = {};
    reflectionProbes.clear();
    reflectionPassObjects.clear();
}

// ZPrePass
void ZPrePassPass::renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data)
{
    using namespace RenderHelpers;

    // INPUT: Item2Ds + depth write + depth prepass

    // DEPENDECY: none

    // OUTPUT: Depth buffer attchment for current target

    // NOTE: Could we make the depth pass more complete and just do a blit here?
    //
    // 1. If we have a depth map, just do a blit and then update with the rest
    // 2. If we don't have a depth map (and/or SSAO) consider using a lower lod level.

    // CONDITION: Input + globally enabled or ?

    QSSG_ASSERT(!data.renderedCameras.isEmpty(), return);
    QSSGRenderCamera *camera = data.renderedCameras[0];

    const auto &rhiCtx = renderer.contextInterface()->rhiContext();
    QSSG_ASSERT(rhiCtx->rhi()->isRecordingFrame(), return);
    QRhiCommandBuffer *cb = rhiCtx->commandBuffer();
    ps = data.getPipelineState();

    renderedDepthWriteObjects = data.getSortedRenderedDepthWriteObjects(*camera);
    renderedOpaqueDepthPrepassObjects = data.getSortedrenderedOpaqueDepthPrepassObjects(*camera);

    cb->debugMarkBegin(QByteArrayLiteral("Quick3D prepare Z prepass"));
    Q_TRACE_SCOPE(QSSG_renderPass, QStringLiteral("Quick3D prepare Z prepass"));
    Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DRenderPass);
    active = rhiPrepareDepthPass(rhiCtx.get(), this, ps, rhiCtx->mainRenderPassDescriptor(), data,
                                         renderedDepthWriteObjects, renderedOpaqueDepthPrepassObjects,
                                         rhiCtx->mainPassSampleCount(), data.layer.viewCount);
    data.setZPrePassPrepResult(active);
    cb->debugMarkEnd();
    Q_QUICK3D_PROFILE_END_WITH_STRING(QQuick3DProfiler::Quick3DRenderPass, 0, QByteArrayLiteral("prepare_z_prepass"));
}

void ZPrePassPass::renderPass(QSSGRenderer &renderer)
{
    using namespace RenderHelpers;

    const auto &rhiCtx = renderer.contextInterface()->rhiContext();
    QSSG_ASSERT(rhiCtx->rhi()->isRecordingFrame(), return);

    bool needsSetViewport = true;
    QRhiCommandBuffer *cb = rhiCtx->commandBuffer();

    if (active) {
        Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DRenderPass);
        Q_TRACE_SCOPE(QSSG_renderPass, QStringLiteral("Quick3D render Z prepass"));
        cb->debugMarkBegin(QByteArrayLiteral("Quick3D render Z prepass"));
        rhiRenderDepthPass(rhiCtx.get(), ps, renderedDepthWriteObjects, renderedOpaqueDepthPrepassObjects, &needsSetViewport);
        cb->debugMarkEnd();
        Q_QUICK3D_PROFILE_END_WITH_STRING(QQuick3DProfiler::Quick3DRenderPass, 0, QByteArrayLiteral("render_z_prepass"));
    }
}

void ZPrePassPass::resetForFrame()
{
    renderedDepthWriteObjects.clear();
    renderedOpaqueDepthPrepassObjects.clear();
    ps = {};
    active = false;
}

// SSAO PASS
void SSAOMapPass::renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data)
{
    using namespace RenderHelpers;

    // Assumption for now is that all passes are keept alive and only reset once a frame is done.
    // I.e., holding data like this should be safe (If that's no longer the case we need to do ref counting
    // for shared data).

    const auto &rhiCtx = renderer.contextInterface()->rhiContext();
    QSSG_ASSERT(rhiCtx->rhi()->isRecordingFrame(), return);

    rhiAoTexture = data.getRenderResult(QSSGFrameData::RenderResult::AoTexture);
    rhiDepthTexture = data.getRenderResult(QSSGFrameData::RenderResult::DepthTexture);
    QSSG_ASSERT_X(!data.renderedCameras.isEmpty(), "Preparing AO pass failed, missing camera", return);
    camera = data.renderedCameras[0];
    QSSG_ASSERT_X((rhiDepthTexture && rhiDepthTexture->isValid()), "Preparing AO pass failed, missing equired texture(s)", return);

    const auto &shaderCache = renderer.contextInterface()->shaderCache();
    ssaoShaderPipeline = shaderCache->getBuiltInRhiShaders().getRhiSsaoShader(data.layer.viewCount);
    aoSettings = { data.layer.aoStrength, data.layer.aoDistance, data.layer.aoSoftness, data.layer.aoBias, data.layer.aoSamplerate, data.layer.aoDither };

    ps = data.getPipelineState();
    const auto &layerPrepResult = data.layerPrepResult;
    const bool ready = rhiAoTexture && rhiPrepareAoTexture(rhiCtx.get(), layerPrepResult.textureDimensions(), rhiAoTexture, data.layer.viewCount);

    if (Q_UNLIKELY(!ready))
        rhiAoTexture = nullptr;
}

void SSAOMapPass::renderPass(QSSGRenderer &renderer)
{
    using namespace RenderHelpers;

    // INPUT: Camera + depth map

    // DEPENDECY: Depth map (zprepass)

    // OUTPUT: AO Texture

    // NOTE:

    // CONDITION: SSAO enabled
    QSSG_ASSERT(camera && rhiDepthTexture && rhiDepthTexture->isValid(), return);

    const auto &rhiCtx = renderer.contextInterface()->rhiContext();
    QSSG_ASSERT(rhiCtx->rhi()->isRecordingFrame(), return);

    QRhiCommandBuffer *cb = rhiCtx->commandBuffer();
    cb->debugMarkBegin(QByteArrayLiteral("Quick3D SSAO map"));
    Q_TRACE_SCOPE(QSSG_renderPass, QStringLiteral("Quick3D SSAO map"));
    Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DRenderPass);

    if (Q_LIKELY(rhiAoTexture && rhiAoTexture->isValid())) {
        rhiRenderAoTexture(rhiCtx.get(),
                           this,
                           renderer,
                           *ssaoShaderPipeline,
                           ps,
                           aoSettings,
                           *rhiAoTexture,
                           *rhiDepthTexture,
                           *camera);
    }

    cb->debugMarkEnd();
    Q_QUICK3D_PROFILE_END_WITH_STRING(QQuick3DProfiler::Quick3DRenderPass, 0, QByteArrayLiteral("ssao_map"));
}

void SSAOMapPass::resetForFrame()
{
    rhiDepthTexture = nullptr;
    rhiAoTexture = nullptr;
    camera = nullptr;
    ps = {};
    aoSettings = {};
}

// DEPTH TEXTURE PASS
void DepthMapPass::renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data)
{
    using namespace RenderHelpers;

    QSSG_ASSERT(!data.renderedCameras.isEmpty(), return);
    QSSGRenderCamera *camera = data.renderedCameras[0];

    const auto &rhiCtx = renderer.contextInterface()->rhiContext();
    QSSG_ASSERT(rhiCtx->rhi()->isRecordingFrame(), return);
    const auto &layerPrepResult = data.layerPrepResult;
    bool ready = false;
    ps = data.getPipelineState();

    if (m_multisampling) {
        ps.samples = rhiCtx->mainPassSampleCount();
        rhiDepthTexture = data.getRenderResult(QSSGFrameData::RenderResult::DepthTextureMS);
    } else {
        ps.samples = 1;
        rhiDepthTexture = data.getRenderResult(QSSGFrameData::RenderResult::DepthTexture);
    }

    if (Q_LIKELY(rhiDepthTexture && rhiPrepareDepthTexture(rhiCtx.get(), layerPrepResult.textureDimensions(), rhiDepthTexture, data.layer.viewCount, ps.samples))) {
        sortedOpaqueObjects = data.getSortedOpaqueRenderableObjects(*camera);
        sortedTransparentObjects = data.getSortedTransparentRenderableObjects(*camera);
        // the depth texture is always non-MSAA, but is a 2D array with multiview
        ready = rhiPrepareDepthPass(rhiCtx.get(), this, ps, rhiDepthTexture->rpDesc, data,
                                    sortedOpaqueObjects, sortedTransparentObjects,
                                    ps.samples, data.layer.viewCount);
    }

    if (Q_UNLIKELY(!ready))
        rhiDepthTexture = nullptr;
}

void DepthMapPass::renderPass(QSSGRenderer &renderer)
{
    using namespace RenderHelpers;

    // INPUT: sorted objects (opaque + transparent) (maybe...)

    // DEPENDECY: If this is only used for the AO case, that dictates if this should be done or not.

    // OUTPUT: Texture

    // NOTE: Why are we prepping opaque + transparent object if we're not using them? And why are we staying compatible with 5.15?
    //       Only used for AO? Merge into the AO pass?

    // NOTES:
    //
    // 1: If requested, use this and blit it in the z-pre pass.
    // 2. Why are we handling the transparent objects in the render prep (only)?

    // CONDITION:

    const auto &rhiCtx = renderer.contextInterface()->rhiContext();
    QSSG_ASSERT(rhiCtx->rhi()->isRecordingFrame(), return);
    QRhiCommandBuffer *cb = rhiCtx->commandBuffer();
    cb->debugMarkBegin(QByteArrayLiteral("Quick3D depth texture"));

    if (Q_LIKELY(rhiDepthTexture && rhiDepthTexture->isValid())) {
        bool needsSetViewport = true;
        cb->beginPass(rhiDepthTexture->rt, Qt::transparent, { 1.0f, 0 }, nullptr, rhiCtx->commonPassFlags());
        QSSGRHICTX_STAT(rhiCtx, beginRenderPass(rhiDepthTexture->rt));
        Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DRenderPass);
        // NB! We do not pass sortedTransparentObjects in the 4th
        // argument to stay compatible with the 5.15 code base,
        // which also does not include semi-transparent objects in
        // the depth texture. In addition, capturing after the
        // opaque pass, not including transparent objects, is part
        // of the contract for screen reading custom materials,
        // both for depth and color.
        rhiRenderDepthPass(rhiCtx.get(), ps, sortedOpaqueObjects, {}, &needsSetViewport);
        cb->endPass();
        QSSGRHICTX_STAT(rhiCtx, endRenderPass());
        Q_QUICK3D_PROFILE_END_WITH_STRING(QQuick3DProfiler::Quick3DRenderPass, 0, QByteArrayLiteral("depth_texture"));
    }

    cb->debugMarkEnd();
}

void DepthMapPass::resetForFrame()
{
    rhiDepthTexture = nullptr;
    sortedOpaqueObjects.clear();
    sortedTransparentObjects.clear();
    ps = {};
}

// SCREEN TEXTURE PASS

void ScreenMapPass::renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data)
{
    using namespace RenderHelpers;

    QSSG_ASSERT(!data.renderedCameras.isEmpty(), return);
    QSSGRenderCamera *camera = data.renderedCameras[0];

    const auto &rhiCtx = renderer.contextInterface()->rhiContext();
    QSSG_ASSERT(rhiCtx->rhi()->isRecordingFrame(), return);
    rhiScreenTexture = data.getRenderResult(QSSGFrameData::RenderResult::ScreenTexture);
    auto &layer = data.layer;
    const auto &layerPrepResult = data.layerPrepResult;
    wantsMips = layerPrepResult.flags.requiresMipmapsForScreenTexture();
    sortedOpaqueObjects = data.getSortedOpaqueRenderableObjects(*camera);
    ps = data.getPipelineState();
    ps.samples = 1; // screen texture is always non-MSAA
    ps.viewCount = data.layer.viewCount; // but is a 2D texture array when multiview

    if (layer.background == QSSGRenderLayer::Background::Color)
        clearColor = QColor::fromRgbF(layer.clearColor.x(), layer.clearColor.y(), layer.clearColor.z());

    if (rhiCtx->rhi()->isFeatureSupported(QRhi::TexelFetch)) {
        if (layer.background == QSSGRenderLayer::Background::SkyBoxCubeMap && layer.skyBoxCubeMap) {
            if (!skyboxCubeMapPass)
                skyboxCubeMapPass = SkyboxCubeMapPass();

            skyboxCubeMapPass->skipTonemapping = true;
            skyboxCubeMapPass->renderPrep(renderer, data);

            // The pass expects to output to the main render target, but we have
            // our own texture here, possibly with a differing sample count, so
            // override the relevant settings once renderPrep() is done.
            skyboxCubeMapPass->ps.samples = ps.samples;

            skyboxPass = std::nullopt;
        } else if (layer.background == QSSGRenderLayer::Background::SkyBox && layer.lightProbe) {
            if (!skyboxPass)
                skyboxPass = SkyboxPass();

            skyboxPass->skipTonemapping = true;
            skyboxPass->renderPrep(renderer, data);

            skyboxPass->ps.samples = ps.samples;

            skyboxCubeMapPass = std::nullopt;
        }
    }

    bool ready = false;
    if (Q_LIKELY(rhiScreenTexture && rhiPrepareScreenTexture(rhiCtx.get(), layerPrepResult.textureDimensions(), wantsMips, rhiScreenTexture, layer.viewCount))) {
        ready = true;
        if (skyboxCubeMapPass)
            skyboxCubeMapPass->rpDesc = rhiScreenTexture->rpDesc;
        if (skyboxPass)
            skyboxPass->rpDesc = rhiScreenTexture->rpDesc;
        // NB: not compatible with disabling LayerEnableDepthTest
        // because there are effectively no "opaque" objects then.
        // Disable Tonemapping for all materials in the screen pass texture
        shaderFeatures = data.getShaderFeatures();
        shaderFeatures.disableTonemapping();
        const auto &sortedOpaqueObjects = data.getSortedOpaqueRenderableObjects(*camera);
        for (const auto &handle : sortedOpaqueObjects) {
            // Reflection cube maps are not available at this point, make sure they are turned off.
            bool recRef = handle.obj->renderableFlags.receivesReflections();
            handle.obj->renderableFlags.setReceivesReflections(false);
            rhiPrepareRenderable(rhiCtx.get(), this, data, *handle.obj, rhiScreenTexture->rpDesc, &ps, shaderFeatures, 1, data.layer.viewCount);
            handle.obj->renderableFlags.setReceivesReflections(recRef);
        }
    }

    if (Q_UNLIKELY(!ready))
        rhiScreenTexture = nullptr;
}

void ScreenMapPass::renderPass(QSSGRenderer &renderer)
{
    using namespace RenderHelpers;

    // INPUT: Sorted opaque objects + depth objects

    // DEPENDECY: Depth pass (if enabled)

    // OUTPUT: Texture (screen texture).

    // NOTE: Used for refrection and effects (?)

    // CONDITION:

    const auto &rhiCtx = renderer.contextInterface()->rhiContext();
    QSSG_ASSERT(rhiCtx->rhi()->isRecordingFrame(), return);
    QRhiCommandBuffer *cb = rhiCtx->commandBuffer();

    cb->debugMarkBegin(QByteArrayLiteral("Quick3D screen texture"));

    if (Q_LIKELY(rhiScreenTexture && rhiScreenTexture->isValid())) {
        cb->beginPass(rhiScreenTexture->rt, clearColor, { 1.0f, 0 }, nullptr, rhiCtx->commonPassFlags());
        QSSGRHICTX_STAT(rhiCtx, beginRenderPass(rhiScreenTexture->rt));
        Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DRenderPass);

        bool needsSetViewport = true;
        for (const auto &handle : std::as_const(sortedOpaqueObjects))
            rhiRenderRenderable(rhiCtx.get(), ps, *handle.obj, &needsSetViewport);

        if (skyboxCubeMapPass)
            skyboxCubeMapPass->renderPass(renderer);
        else if (skyboxPass)
            skyboxPass->renderPass(renderer);

        QRhiResourceUpdateBatch *rub = nullptr;
        if (wantsMips) {
            rub = rhiCtx->rhi()->nextResourceUpdateBatch();
            rub->generateMips(rhiScreenTexture->texture);
        }
        cb->endPass(rub);
        QSSGRHICTX_STAT(rhiCtx, endRenderPass());
        Q_QUICK3D_PROFILE_END_WITH_STRING(QQuick3DProfiler::Quick3DRenderPass, 0, QByteArrayLiteral("screen_texture"));
    }

    cb->debugMarkEnd();
}

void ScreenMapPass::resetForFrame()
{
    rhiScreenTexture = nullptr;
    if (skyboxPass)
        skyboxPass->resetForFrame();
    if (skyboxCubeMapPass)
        skyboxCubeMapPass->resetForFrame();
    ps = {};
    wantsMips = false;
    clearColor = Qt::transparent;
    shaderFeatures = {};
    sortedOpaqueObjects.clear();
}

void ScreenReflectionPass::renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data)
{
    QSSG_ASSERT(!data.renderedCameras.isEmpty(), return);
    QSSGRenderCamera *camera = data.renderedCameras[0];

    const auto &rhiCtx = renderer.contextInterface()->rhiContext();
    QSSG_ASSERT(rhiCtx->rhi()->isRecordingFrame(), return);
    rhiScreenTexture = data.getRenderResult(QSSGFrameData::RenderResult::ScreenTexture);
    QSSG_ASSERT_X(rhiScreenTexture && rhiScreenTexture->isValid(), "Invalid screen texture!", return);

    const auto &layer = data.layer;
    const auto shaderFeatures = data.getShaderFeatures();
    const bool layerEnableDepthTest = layer.layerFlags.testFlag(QSSGRenderLayer::LayerFlag::EnableDepthTest);

    QRhiRenderPassDescriptor *mainRpDesc = rhiCtx->mainRenderPassDescriptor();
    const int samples = rhiCtx->mainPassSampleCount();
    const int viewCount = data.layer.viewCount;

    // NOTE: We're piggybacking on the screen map pass for now, but we could do better.
    ps = data.getPipelineState();
    const bool depthTestEnabled = (data.screenMapPass.ps.flags.testFlag(QSSGRhiGraphicsPipelineState::Flag::DepthTestEnabled));
    ps.flags.setFlag(QSSGRhiGraphicsPipelineState::Flag::DepthTestEnabled, depthTestEnabled);
    const bool depthWriteEnabled = (data.screenMapPass.ps.flags.testFlag(QSSGRhiGraphicsPipelineState::Flag::DepthWriteEnabled));
    ps.flags.setFlag(QSSGRhiGraphicsPipelineState::Flag::DepthWriteEnabled, depthWriteEnabled);
    sortedScreenTextureObjects = data.getSortedScreenTextureRenderableObjects(*camera);
    for (const auto &handle : std::as_const(sortedScreenTextureObjects)) {
        QSSGRenderableObject *theObject = handle.obj;
        const auto depthWriteMode = theObject->depthWriteMode;
        ps.flags.setFlag(QSSGRhiGraphicsPipelineState::Flag::BlendEnabled, theObject->renderableFlags.hasTransparency());
        const bool curDepthWriteEnabled = !(depthWriteMode == QSSGDepthDrawMode::Never || depthWriteMode == QSSGDepthDrawMode::OpaquePrePass
                                     || data.isZPrePassActive() || !layerEnableDepthTest);
        ps.flags.setFlag(QSSGRhiGraphicsPipelineState::Flag::DepthWriteEnabled, curDepthWriteEnabled);
        RenderHelpers::rhiPrepareRenderable(rhiCtx.get(), this, data, *theObject, mainRpDesc, &ps, shaderFeatures, samples, viewCount);
    }
}

void ScreenReflectionPass::renderPass(QSSGRenderer &renderer)
{
    if (QSSG_GUARD(rhiScreenTexture && rhiScreenTexture->isValid())) {
        const auto &rhiCtx = renderer.contextInterface()->rhiContext();
        QSSG_ASSERT(rhiCtx->rhi()->isRecordingFrame(), return);
        QRhiCommandBuffer *cb = rhiCtx->commandBuffer();

        // 3. Screen texture depended objects
        cb->debugMarkBegin(QByteArrayLiteral("Quick3D render screen texture dependent"));
        Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DRenderPass);
        Q_TRACE(QSSG_renderPass_entry, QStringLiteral("Quick3D render screen texture dependent"));
        bool needsSetViewport = true;
        for (const auto &handle : std::as_const(sortedScreenTextureObjects)) {
            QSSGRenderableObject *theObject = handle.obj;
            RenderHelpers::rhiRenderRenderable(rhiCtx.get(), ps, *theObject, &needsSetViewport);
        }
        cb->debugMarkEnd();
        Q_QUICK3D_PROFILE_END_WITH_STRING(QQuick3DProfiler::Quick3DRenderPass, 0, QByteArrayLiteral("screen_texture_dependent"));
        Q_TRACE(QSSG_renderPass_exit);
    }
}

void ScreenReflectionPass::resetForFrame()
{
    sortedScreenTextureObjects.clear();
    rhiScreenTexture = nullptr;
    ps = {};
}

void OpaquePass::prep(const QSSGRenderContextInterface &ctx,
                      QSSGLayerRenderData &data,
                      QSSGPassKey passKey,
                      QSSGRhiGraphicsPipelineState &ps,
                      QSSGShaderFeatures shaderFeatures,
                      QRhiRenderPassDescriptor *rpDesc,
                      const QSSGRenderableObjectList &sortedOpaqueObjects)
{
    const auto &rhiCtx = ctx.rhiContext();
    QSSG_ASSERT(rpDesc && rhiCtx->rhi()->isRecordingFrame(), return);

    const auto &layer = data.layer;
    const bool layerEnableDepthTest = layer.layerFlags.testFlag(QSSGRenderLayer::LayerFlag::EnableDepthTest);

    for (const auto &handle : std::as_const(sortedOpaqueObjects)) {
        QSSGRenderableObject *theObject = handle.obj;
        const auto depthWriteMode = theObject->depthWriteMode;
        const bool curDepthWriteEnabled = !(depthWriteMode == QSSGDepthDrawMode::Never ||
                                            depthWriteMode == QSSGDepthDrawMode::OpaquePrePass ||
                                            data.isZPrePassActive() || !layerEnableDepthTest);
        ps.flags.setFlag(QSSGRhiGraphicsPipelineState::Flag::DepthWriteEnabled, curDepthWriteEnabled);
        RenderHelpers::rhiPrepareRenderable(rhiCtx.get(), passKey, data, *theObject, rpDesc, &ps, shaderFeatures, ps.samples, ps.viewCount);
    }
}

void OpaquePass::render(const QSSGRenderContextInterface &ctx,
                        const QSSGRhiGraphicsPipelineState &ps,
                        const QSSGRenderableObjectList &sortedOpaqueObjects)
{
    const auto &rhiCtx = ctx.rhiContext();
    QSSG_ASSERT(rhiCtx->rhi()->isRecordingFrame(), return);
    bool needsSetViewport = true;
    for (const auto &handle : std::as_const(sortedOpaqueObjects)) {
        QSSGRenderableObject *theObject = handle.obj;
        RenderHelpers::rhiRenderRenderable(rhiCtx.get(), ps, *theObject, &needsSetViewport);
    }
}

void OpaquePass::renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data)
{
    auto *ctx = renderer.contextInterface();
    const auto &rhiCtx = ctx->rhiContext();
    QSSG_ASSERT(rhiCtx->rhi()->isRecordingFrame(), return);
    QSSG_ASSERT(!data.renderedCameras.isEmpty() && data.renderedCameraData.has_value() , return);
    QSSGRenderCamera *camera = data.renderedCameras[0];

    ps = data.getPipelineState();
    ps.samples = rhiCtx->mainPassSampleCount();
    ps.viewCount = data.layer.viewCount;
    ps.depthFunc = QRhiGraphicsPipeline::LessOrEqual;
    ps.flags.setFlag(QSSGRhiGraphicsPipelineState::Flag::BlendEnabled, false);

    // opaque objects (or, this list is empty when LayerEnableDepthTest is disabled)
    sortedOpaqueObjects = data.getSortedOpaqueRenderableObjects(*camera);
    shaderFeatures = data.getShaderFeatures();

    QRhiRenderPassDescriptor *mainRpDesc = rhiCtx->mainRenderPassDescriptor();
    prep(*ctx, data, this, ps, shaderFeatures, mainRpDesc, sortedOpaqueObjects);
}

void OpaquePass::renderPass(QSSGRenderer &renderer)
{
    auto *ctx = renderer.contextInterface();
    const auto &rhiCtx = ctx->rhiContext();
    QSSG_ASSERT(rhiCtx->rhi()->isRecordingFrame(), return);
    QRhiCommandBuffer *cb = rhiCtx->commandBuffer();

    cb->debugMarkBegin(QByteArrayLiteral("Quick3D render opaque"));
    Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DRenderPass);
    Q_TRACE(QSSG_renderPass_entry, QStringLiteral("Quick3D render opaque"));
    render(*ctx, ps, sortedOpaqueObjects);
    cb->debugMarkEnd();
    Q_QUICK3D_PROFILE_END_WITH_STRING(QQuick3DProfiler::Quick3DRenderPass, 0, QByteArrayLiteral("opaque_pass"));
    Q_TRACE(QSSG_renderPass_exit);
}

void OpaquePass::resetForFrame()
{
    sortedOpaqueObjects.clear();
    ps = {};
    shaderFeatures = {};
}

void TransparentPass::prep(const QSSGRenderContextInterface &ctx,
                           QSSGLayerRenderData &data,
                           QSSGPassKey passKey,
                           QSSGRhiGraphicsPipelineState &ps,
                           QSSGShaderFeatures shaderFeatures,
                           QRhiRenderPassDescriptor *rpDesc,
                           const QSSGRenderableObjectList &sortedTransparentObjects,
                           bool oit)
{
    const auto &rhiCtx = ctx.rhiContext();
    QSSG_ASSERT(rpDesc && rhiCtx->rhi()->isRecordingFrame(), return);

    const bool zPrePassActive = data.isZPrePassActive();
    for (const auto &handle : std::as_const(sortedTransparentObjects)) {
        QSSGRenderableObject *theObject = handle.obj;
        const auto depthWriteMode = theObject->depthWriteMode;
        const bool curDepthWriteEnabled = (depthWriteMode == QSSGDepthDrawMode::Always && !zPrePassActive);
        ps.flags.setFlag(QSSGRhiGraphicsPipelineState::Flag::DepthWriteEnabled, curDepthWriteEnabled);
        if (!(theObject->renderableFlags.isCompletelyTransparent())) {
            RenderHelpers::rhiPrepareRenderable(rhiCtx.get(), passKey, data, *theObject, rpDesc, &ps, shaderFeatures,
                                                ps.samples, ps.viewCount, nullptr, nullptr, QSSGRenderTextureCubeFaceNone, nullptr, oit);
        }
    }
}

void TransparentPass::render(const QSSGRenderContextInterface &ctx,
                             const QSSGRhiGraphicsPipelineState &ps,
                             const QSSGRenderableObjectList &sortedTransparentObjects)
{
    const auto &rhiCtx = ctx.rhiContext();
    QSSG_ASSERT(rhiCtx->rhi()->isRecordingFrame(), return);
    // If scissorRect is set, Item2Ds will be drawn by a workaround of modifying
    // viewport, not using actual 3D scissor test.
    // It means non-opaque objects may be affected by this viewport setting.
    bool needsSetViewport = true;
    for (const auto &handle : std::as_const(sortedTransparentObjects)) {
        QSSGRenderableObject *theObject = handle.obj;
        if (!theObject->renderableFlags.isCompletelyTransparent())
            RenderHelpers::rhiRenderRenderable(rhiCtx.get(), ps, *theObject, &needsSetViewport);
    }
}

void TransparentPass::renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data)
{
    auto *ctx = renderer.contextInterface();
    const auto &rhiCtx = ctx->rhiContext();

    QSSG_ASSERT(!data.renderedCameras.isEmpty() && data.renderedCameraData.has_value() , return);
    QSSGRenderCamera *camera = data.renderedCameras[0];

    QRhiRenderPassDescriptor *mainRpDesc = rhiCtx->mainRenderPassDescriptor();

    ps = data.getPipelineState();
    ps.samples = rhiCtx->mainPassSampleCount();
    ps.viewCount = data.layer.viewCount;

    // transparent objects (or, without LayerEnableDepthTest, all objects)
    ps.flags.setFlag(QSSGRhiGraphicsPipelineState::Flag::BlendEnabled, true);
    ps.flags.setFlag(QSSGRhiGraphicsPipelineState::Flag::DepthWriteEnabled, false);

    shaderFeatures = data.getShaderFeatures();
    sortedTransparentObjects = data.getSortedTransparentRenderableObjects(*camera);

    prep(*ctx, data, this, ps, shaderFeatures, mainRpDesc, sortedTransparentObjects);
}

void TransparentPass::renderPass(QSSGRenderer &renderer)
{
    auto *ctx = renderer.contextInterface();
    const auto &rhiCtx = ctx->rhiContext();
    QSSG_ASSERT(rhiCtx->rhi()->isRecordingFrame(), return);
    QRhiCommandBuffer *cb = rhiCtx->commandBuffer();

    cb->debugMarkBegin(QByteArrayLiteral("Quick3D render alpha"));
    Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DRenderPass);
    Q_TRACE(QSSG_renderPass_entry, QStringLiteral("Quick3D render alpha"));
    render(*ctx, ps, sortedTransparentObjects);
    cb->debugMarkEnd();
    Q_QUICK3D_PROFILE_END_WITH_STRING(QQuick3DProfiler::Quick3DRenderPass, 0, QByteArrayLiteral("transparent_pass"));
    Q_TRACE(QSSG_renderPass_exit);
}

void TransparentPass::resetForFrame()
{
    sortedTransparentObjects.clear();
    ps = {};
    shaderFeatures = {};
}

void SkyboxPass::renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data)
{
    if (!skipPrep) {
        const auto &rhiCtx = renderer.contextInterface()->rhiContext();
        QSSG_ASSERT(rhiCtx->rhi()->isRecordingFrame(), return);
        QSSG_ASSERT(!data.renderedCameras.isEmpty(), return);
        QSSG_ASSERT(data.renderedCameras.count() == data.layer.viewCount, return);
        layer = &data.layer;
        QSSG_ASSERT(layer, return);

        rpDesc = rhiCtx->mainRenderPassDescriptor();
        ps = data.getPipelineState();
        ps.samples = rhiCtx->mainPassSampleCount();
        ps.viewCount = data.layer.viewCount;
        ps.polygonMode = QRhiGraphicsPipeline::Fill;

        // When there are effects, then it is up to the last pass of the
        // last effect to perform tonemapping, neither the skybox nor the
        // main render pass should alter the colors then.
        skipTonemapping = layer->firstEffect != nullptr;

        RenderHelpers::rhiPrepareSkyBox(rhiCtx.get(), this, *layer, data.renderedCameras, renderer);
        skipPrep = true;
    }
}

void SkyboxPass::renderPass(QSSGRenderer &renderer)
{
    const auto &rhiCtx = renderer.contextInterface()->rhiContext();
    QSSG_ASSERT(rhiCtx->rhi()->isRecordingFrame(), return);
    QSSG_ASSERT(layer, return);

    QRhiShaderResourceBindings *srb = layer->skyBoxSrb;
    QSSG_ASSERT(srb, return);

    Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DRenderPass);
    Q_TRACE_SCOPE(QSSG_renderPass, QStringLiteral("Quick3D render skybox"));

    // Note: We get the shader here, as the screen map pass might modify the state of
    // the tonemap mode.

    QSSGRenderLayer::TonemapMode tonemapMode = skipTonemapping && (layer->tonemapMode != QSSGRenderLayer::TonemapMode::Custom) ?  QSSGRenderLayer::TonemapMode::None : layer->tonemapMode;
    const auto &shaderCache = renderer.contextInterface()->shaderCache();
    auto shaderPipeline = shaderCache->getBuiltInRhiShaders().getRhiSkyBoxShader(tonemapMode, layer->skyBoxIsRgbe8, layer->viewCount);
    QSSG_CHECK(shaderPipeline);
    QSSGRhiGraphicsPipelineStatePrivate::setShaderPipeline(ps, shaderPipeline.get());
    renderer.rhiQuadRenderer()->recordRenderQuad(rhiCtx.get(), &ps, srb, rpDesc, { QSSGRhiQuadRenderer::DepthTest | QSSGRhiQuadRenderer::RenderBehind });
    Q_QUICK3D_PROFILE_END_WITH_STRING(QQuick3DProfiler::Quick3DRenderPass, 0, QByteArrayLiteral("skybox_map"));
}

void SkyboxPass::resetForFrame()
{
    ps = {};
    layer = nullptr;
    skipPrep = false;
}

void SkyboxCubeMapPass::renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data)
{
    const auto &rhiCtx = renderer.contextInterface()->rhiContext();
    QSSG_ASSERT(rhiCtx->rhi()->isRecordingFrame(), return);
    QSSG_ASSERT(!data.renderedCameras.isEmpty(), return);
    QSSG_ASSERT(data.renderedCameras.count() == data.layer.viewCount, return);
    layer = &data.layer;
    QSSG_ASSERT(layer, return);

    rpDesc = rhiCtx->mainRenderPassDescriptor();
    ps = data.getPipelineState();
    ps.samples = rhiCtx->mainPassSampleCount();
    ps.viewCount = data.layer.viewCount;
    ps.polygonMode = QRhiGraphicsPipeline::Fill;

    const auto &shaderCache = renderer.contextInterface()->shaderCache();
    skyBoxCubeShader = shaderCache->getBuiltInRhiShaders().getRhiSkyBoxCubeShader(data.layer.viewCount);

    RenderHelpers::rhiPrepareSkyBox(rhiCtx.get(), this, *layer, data.renderedCameras, renderer);
}

void SkyboxCubeMapPass::renderPass(QSSGRenderer &renderer)
{
    const auto &rhiCtx = renderer.contextInterface()->rhiContext();
    QSSG_ASSERT(rhiCtx->rhi()->isRecordingFrame(), return);
    QSSG_ASSERT(layer && skyBoxCubeShader, return);

    QRhiShaderResourceBindings *srb = layer->skyBoxSrb;
    QSSG_ASSERT(srb, return);

    Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DRenderPass);
    Q_TRACE_SCOPE(QSSG_renderPass, QStringLiteral("Quick3D render skybox"));

    QSSGRhiGraphicsPipelineStatePrivate::setShaderPipeline(ps, skyBoxCubeShader.get());
    renderer.rhiCubeRenderer()->recordRenderCube(rhiCtx.get(), &ps, srb, rpDesc, { QSSGRhiQuadRenderer::DepthTest | QSSGRhiQuadRenderer::RenderBehind });
    Q_QUICK3D_PROFILE_END_WITH_STRING(QQuick3DProfiler::Quick3DRenderPass, 0, QByteArrayLiteral("skybox_cube"));
}

void SkyboxCubeMapPass::resetForFrame()
{
    ps = {};
    layer = nullptr;
}

void Item2DPass::renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data)
{
    const auto &rhiCtx = renderer.contextInterface()->rhiContext();
    QSSG_ASSERT(rhiCtx->rhi()->isRecordingFrame(), return);
    const auto &layer = data.layer;

    ps = data.getPipelineState();

    // objects rendered by Qt Quick 2D
    ps.flags.setFlag(QSSGRhiGraphicsPipelineState::Flag::BlendEnabled, false);

    item2Ds = data.getRenderableItem2Ds();
    // NOTE: This marks the start of the 2D sub-scene rendering as it might result in
    // a nested 3D scene to be rendered and if we don't save the state here, we can
    // end up with a mismatched state in the QtQuick3D renderer.
    // See the end of this function for the corresponding end call (endSubLayerRender()).
    renderer.beginSubLayerRender(data);
    for (const auto &item2D: std::as_const(item2Ds)) {
        // Set the projection matrix
        if (!item2D->m_renderer)
            continue;
        if (item2D->m_renderer && item2D->m_renderer->currentRhi() != renderer.contextInterface()->rhiContext()->rhi()) {
            static bool contextWarningShown = false;
            if (!contextWarningShown) {
                qWarning () << "Scene with embedded 2D content can only be rendered in one window.";
                contextWarningShown = true;
            }
            continue;
        }

        auto layerPrepResult = data.layerPrepResult;

        QRhiRenderTarget *renderTarget = rhiCtx->renderTarget();
        item2D->m_renderer->setDevicePixelRatio(renderTarget->devicePixelRatio());
        const QRect deviceRect(QPoint(0, 0), renderTarget->pixelSize());
        const int viewCount = data.layer.viewCount;
        QSSG_ASSERT(item2D->mvps.count() == viewCount, return);
        if (layer.scissorRect.isValid()) {
            QRect effScissor = layer.scissorRect & layerPrepResult.viewport.toRect();
            QMatrix4x4 correctionMat = correctMVPForScissor(layerPrepResult.viewport,
                                                            effScissor,
                                                            rhiCtx->rhi()->isYUpInNDC());
            for (int viewIndex = 0; viewIndex < viewCount; ++viewIndex) {
                const QMatrix4x4 projectionMatrix = correctionMat * item2D->mvps[viewIndex];
                item2D->m_renderer->setProjectionMatrix(projectionMatrix, viewIndex);
            }
            item2D->m_renderer->setViewportRect(effScissor);
        } else {
            for (int viewIndex = 0; viewIndex < viewCount; ++viewIndex)
                item2D->m_renderer->setProjectionMatrix(item2D->mvps[viewIndex], viewIndex);
            item2D->m_renderer->setViewportRect(RenderHelpers::correctViewportCoordinates(layerPrepResult.viewport, deviceRect));
        }
        item2D->m_renderer->setDeviceRect(deviceRect);
        QRhiRenderPassDescriptor *oldRp = nullptr;
        if (item2D->m_rp) {
            // Changing render target, and so incompatible renderpass
            // descriptors should be uncommon, but possible.
            if (!item2D->m_rp->isCompatible(rhiCtx->mainRenderPassDescriptor()))
                std::swap(item2D->m_rp, oldRp);
        }
        if (!item2D->m_rp) {
            // Do not pass our object to the Qt Quick scenegraph. It may
            // hold on to it, leading to lifetime and ownership issues.
            // Rather, create a dedicated, compatible object.
            item2D->m_rp = rhiCtx->mainRenderPassDescriptor()->newCompatibleRenderPassDescriptor();
            QSSG_CHECK(item2D->m_rp);
        }
        QSGRenderTarget sgRt(renderTarget, item2D->m_rp, rhiCtx->commandBuffer());
        sgRt.multiViewCount = data.layer.viewCount;
        item2D->m_renderer->setRenderTarget(sgRt);
        delete oldRp;
        item2D->m_renderer->prepareSceneInline();
    }
    renderer.endSubLayerRender(data);
}

void Item2DPass::renderPass(QSSGRenderer &renderer)
{
    QSSG_ASSERT(!item2Ds.isEmpty(), return);

    const auto &rhiCtx = renderer.contextInterface()->rhiContext();
    QSSG_ASSERT(rhiCtx->rhi()->isRecordingFrame(), return);
    QRhiCommandBuffer *cb = rhiCtx->commandBuffer();

    cb->debugMarkBegin(QByteArrayLiteral("Quick3D render 2D sub-scene"));
    Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DRenderPass);
    Q_TRACE_SCOPE(QSSG_renderPass, QStringLiteral("Quick3D render 2D sub-scene"));
    QSSGLayerRenderData *data = QSSGLayerRenderData::getCurrent(renderer);
    renderer.beginSubLayerRender(*data);
    for (const auto &item : std::as_const(item2Ds)) {
        QSSGRenderItem2D *item2D = static_cast<QSSGRenderItem2D *>(item);
        if (item2D->m_renderer && item2D->m_renderer->currentRhi() == renderer.contextInterface()->rhiContext()->rhi())
            item2D->m_renderer->renderSceneInline();
    }
    renderer.endSubLayerRender(*data);
    cb->debugMarkEnd();
    Q_QUICK3D_PROFILE_END_WITH_STRING(QQuick3DProfiler::Quick3DRenderPass, 0, QByteArrayLiteral("2D_sub_scene"));
}

void Item2DPass::resetForFrame()
{
    item2Ds.clear();
    ps = {};
}

void InfiniteGridPass::renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data)
{
    const auto &rhiCtx = renderer.contextInterface()->rhiContext();
    QSSG_ASSERT(rhiCtx->rhi()->isRecordingFrame(), return);
    QSSG_ASSERT(!data.renderedCameras.isEmpty(), return);
    QSSG_ASSERT(data.renderedCameras.count() == data.layer.viewCount, return);
    layer = &data.layer;
    QSSG_ASSERT(layer, return);

    const auto &shaderCache = renderer.contextInterface()->shaderCache();
    gridShader = shaderCache->getBuiltInRhiShaders().getRhiGridShader(data.layer.viewCount);

    ps = data.getPipelineState();
    ps.samples = rhiCtx->mainPassSampleCount();
    ps.viewCount = data.layer.viewCount;
    ps.flags.setFlag(QSSGRhiGraphicsPipelineState::Flag::BlendEnabled, true);

    RenderHelpers::rhiPrepareGrid(rhiCtx.get(), this, *layer, data.renderedCameras, renderer);
}

void InfiniteGridPass::renderPass(QSSGRenderer &renderer)
{
    const auto &rhiCtx = renderer.contextInterface()->rhiContext();
    QSSG_ASSERT(gridShader && rhiCtx->rhi()->isRecordingFrame(), return);
    QRhiCommandBuffer *cb = rhiCtx->commandBuffer();

    cb->debugMarkBegin(QByteArrayLiteral("Quick3D render grid"));
    Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DRenderPass);
    Q_TRACE_SCOPE(QSSG_renderPass, QStringLiteral("Quick3D render grid"));
    QSSGRhiGraphicsPipelineStatePrivate::setShaderPipeline(ps, gridShader.get());
    QRhiShaderResourceBindings *srb = layer->gridSrb;
    QRhiRenderPassDescriptor *rpDesc = rhiCtx->mainRenderPassDescriptor();
    renderer.rhiQuadRenderer()->recordRenderQuad(rhiCtx.get(), &ps, srb, rpDesc, { QSSGRhiQuadRenderer::DepthTest });
    Q_QUICK3D_PROFILE_END_WITH_STRING(QQuick3DProfiler::Quick3DRenderPass, 0, QByteArrayLiteral("render_grid"));
}

void InfiniteGridPass::resetForFrame()
{
    ps = {};
    layer = nullptr;
}

void DebugDrawPass::renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data)
{
    const auto &rhiCtx = renderer.contextInterface()->rhiContext();
    QSSG_ASSERT(rhiCtx->rhi()->isRecordingFrame(), return);
    QSSGRhiContextPrivate *rhiCtxD = QSSGRhiContextPrivate::get(rhiCtx.get());
    QSSG_ASSERT(!data.renderedCameras.isEmpty(), return);
    QSSG_ASSERT(data.renderedCameras.count() == data.layer.viewCount, return);

    const auto &shaderCache = renderer.contextInterface()->shaderCache();
    debugObjectShader = shaderCache->getBuiltInRhiShaders().getRhiDebugObjectShader();
    ps = data.getPipelineState();

    // debug objects
    const auto &debugDraw = renderer.contextInterface()->debugDrawSystem();
    if (debugDraw && debugDraw->hasContent()) {
        QRhi *rhi = rhiCtx->rhi();
        QRhiResourceUpdateBatch *rub = rhi->nextResourceUpdateBatch();
        debugDraw->prepareGeometry(rhiCtx.get(), rub);
        QSSGRhiDrawCallData &dcd = rhiCtxD->drawCallData({ this, nullptr, nullptr, 0 });
        if (!dcd.ubuf) {
            dcd.ubuf = rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64 * data.renderedCameras.count());
            dcd.ubuf->create();
        }
        char *ubufData = dcd.ubuf->beginFullDynamicBufferUpdateForCurrentFrame();
        for (qsizetype viewIdx = 0; viewIdx < data.renderedCameras.count(); ++viewIdx) {
            QMatrix4x4 viewProjection(Qt::Uninitialized);
            data.renderedCameras[viewIdx]->calculateViewProjectionMatrix(viewProjection);
            viewProjection = rhi->clipSpaceCorrMatrix() * viewProjection;
            memcpy(ubufData, viewProjection.constData() + viewIdx * 64, 64);
        }
        dcd.ubuf->endFullDynamicBufferUpdateForCurrentFrame();

        QSSGRhiShaderResourceBindingList bindings;
        bindings.addUniformBuffer(0, QRhiShaderResourceBinding::VertexStage, dcd.ubuf);
        dcd.srb = rhiCtxD->srb(bindings);

        rhiCtx->commandBuffer()->resourceUpdate(rub);
    }
}

void DebugDrawPass::renderPass(QSSGRenderer &renderer)
{
    const auto &rhiCtx = renderer.contextInterface()->rhiContext();
    QSSG_ASSERT(debugObjectShader && rhiCtx->rhi()->isRecordingFrame(), return);
    QRhiCommandBuffer *cb = rhiCtx->commandBuffer();
    QSSGRhiContextPrivate *rhiCtxD = QSSGRhiContextPrivate::get(rhiCtx.get());

    const auto &debugDraw = renderer.contextInterface()->debugDrawSystem();
    if (debugDraw && debugDraw->hasContent()) {
        cb->debugMarkBegin(QByteArrayLiteral("Quick 3D debug objects"));
        Q_TRACE_SCOPE(QSSG_renderPass, QStringLiteral("Quick 3D debug objects"));
        Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DRenderPass);
        QSSGRhiGraphicsPipelineStatePrivate::setShaderPipeline(ps, debugObjectShader.get());
        QSSGRhiDrawCallData &dcd = rhiCtxD->drawCallData({ this, nullptr, nullptr, 0 });
        QRhiShaderResourceBindings *srb = dcd.srb;
        QRhiRenderPassDescriptor *rpDesc = rhiCtx->mainRenderPassDescriptor();
        debugDraw->recordRenderDebugObjects(rhiCtx.get(), &ps, srb, rpDesc);
        cb->debugMarkEnd();
        Q_QUICK3D_PROFILE_END_WITH_STRING(QQuick3DProfiler::Quick3DRenderPass, 0, QByteArrayLiteral("debug_objects"));
    }
}

void DebugDrawPass::resetForFrame()
{
    ps = {};
}

void UserPass::renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data)
{
    Q_UNUSED(renderer);
    auto &frameData = data.getFrameData();
    for (const auto &p : std::as_const(extensions)) {
        p->prepareRender(frameData);
        if (p->mode() == QSSGRenderExtension::RenderMode::Standalone)
            p->render(frameData);
    }
}

void UserPass::renderPass(QSSGRenderer &renderer)
{
    auto *data = QSSGLayerRenderData::getCurrent(renderer);
    QSSG_ASSERT(data, return);
    auto &frameData = data->getFrameData();
    for (const auto &p : std::as_const(extensions)) {
        if (p->mode() == QSSGRenderExtension::RenderMode::Main)
            p->render(frameData);
    }
}

void UserPass::resetForFrame()
{
    for (const auto &p : std::as_const(extensions))
        p->resetForFrame();

    // TODO: We should track if we need to update this list.
    extensions.clear();
}

void OITRenderPass::renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data)
{
    auto *ctx = renderer.contextInterface();
    const auto &rhiCtx = ctx->rhiContext();
    auto *rhi = rhiCtx->rhi();

    QSSG_ASSERT(!data.renderedCameras.isEmpty() && data.renderedCameraData.has_value() , return);
    QSSGRenderCamera *camera = data.renderedCameras[0];

    ps = data.getPipelineState();
    ps.samples = rhiCtx->mainPassSampleCount();
    ps.viewCount = rhiCtx->mainPassViewCount();

    ps.flags.setFlag(QSSGRhiGraphicsPipelineState::Flag::BlendEnabled, true);
    ps.flags.setFlag(QSSGRhiGraphicsPipelineState::Flag::DepthWriteEnabled, false);

    shaderFeatures = data.getShaderFeatures();
    sortedTransparentObjects = data.getSortedTransparentRenderableObjects(*camera);

    if (method == QSSGRenderLayer::OITMethod::WeightedBlended) {
        ps.colorAttachmentCount = 2;

        rhiAccumTexture = data.getRenderResult(QSSGFrameData::RenderResult::AccumTexture);
        rhiRevealageTexture = data.getRenderResult(QSSGFrameData::RenderResult::RevealageTexture);
        if (ps.samples > 1)
            rhiDepthTexture = data.getRenderResult(QSSGFrameData::RenderResult::DepthTextureMS);
        else
            rhiDepthTexture = data.getRenderResult(QSSGFrameData::RenderResult::DepthTexture);
        if (!rhiDepthTexture->isValid())
            return;
        auto &oitrt = data.getOitRenderContext();
        if (!oitrt.oitRenderTarget || oitrt.oitRenderTarget->pixelSize() != data.layerPrepResult.textureDimensions()
            || rhiDepthTexture->texture != oitrt.oitRenderTarget->description().depthTexture()
            || ps.samples != oitrt.oitRenderTarget->sampleCount()) {
            if (oitrt.oitRenderTarget) {
                rhiAccumTexture->texture->destroy();
                rhiRevealageTexture->texture->destroy();
                oitrt.oitRenderTarget->destroy();
                oitrt.renderPassDescriptor->destroy();
                oitrt.oitRenderTarget = nullptr;
            }
            const QRhiTexture::Flags textureFlags = QRhiTexture::RenderTarget;
            if (ps.viewCount >= 2) {
                rhiAccumTexture->texture = rhi->newTextureArray(QRhiTexture::RGBA16F, ps.viewCount, data.layerPrepResult.textureDimensions(), ps.samples, textureFlags);
                rhiRevealageTexture->texture = rhi->newTextureArray(QRhiTexture::R16F, ps.viewCount, data.layerPrepResult.textureDimensions(), ps.samples, textureFlags);
            } else {
                rhiAccumTexture->texture = rhi->newTexture(QRhiTexture::RGBA16F, data.layerPrepResult.textureDimensions(), ps.samples, textureFlags);
                rhiRevealageTexture->texture = rhi->newTexture(QRhiTexture::R16F, data.layerPrepResult.textureDimensions(), ps.samples, textureFlags);
            }
            rhiAccumTexture->texture->create();
            rhiRevealageTexture->texture->create();

            QRhiTextureRenderTargetDescription desc;
            desc.setColorAttachments({{rhiAccumTexture->texture}, {rhiRevealageTexture->texture}});
            desc.setDepthTexture(rhiDepthTexture->texture);

            if (oitrt.oitRenderTarget == nullptr) {
                oitrt.oitRenderTarget = rhi->newTextureRenderTarget(desc, QRhiTextureRenderTarget::PreserveDepthStencilContents);
                oitrt.renderPassDescriptor = oitrt.oitRenderTarget->newCompatibleRenderPassDescriptor();
                oitrt.oitRenderTarget->setRenderPassDescriptor(oitrt.renderPassDescriptor);
                oitrt.oitRenderTarget->create();

                renderTarget = oitrt.oitRenderTarget;
            }
        }
        QSSGRhiContextPrivate *rhiCtxD = QSSGRhiContextPrivate::get(rhiCtx.get());
        const auto &shaderCache = renderer.contextInterface()->shaderCache();
        clearPipeline = shaderCache->getBuiltInRhiShaders().getRhiClearMRTShader();

        QSSGRhiShaderResourceBindingList bindings;
        QVector4D clearData[2];
        clearData[0] = QVector4D(0.0, 0.0, 0.0, 0.0);
        clearData[1] = QVector4D(1.0, 1.0, 1.0, 1.0);

        QSSGRhiDrawCallData &dcd(rhiCtxD->drawCallData({ this, nullptr, nullptr, 0 }));
        QRhiBuffer *&ubuf = dcd.ubuf;
        const int ubufSize = sizeof(clearData);
        if (!ubuf) {
            ubuf = rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, ubufSize);
            ubuf->create();
        }

        QRhiResourceUpdateBatch *rub = rhi->nextResourceUpdateBatch();
        rub->updateDynamicBuffer(ubuf, 0, ubufSize, &clearData);
        renderer.rhiQuadRenderer()->prepareQuad(rhiCtx.get(), rub);

        bindings.addUniformBuffer(0, QRhiShaderResourceBinding::FragmentStage, ubuf);

        clearSrb = rhiCtxD->srb(bindings);

        ps.targetBlend[0].srcAlpha = QRhiGraphicsPipeline::One;
        ps.targetBlend[0].srcColor = QRhiGraphicsPipeline::One;
        ps.targetBlend[0].dstAlpha = QRhiGraphicsPipeline::One;
        ps.targetBlend[0].dstColor = QRhiGraphicsPipeline::One;
        ps.targetBlend[1].srcAlpha = QRhiGraphicsPipeline::Zero;
        ps.targetBlend[1].srcColor = QRhiGraphicsPipeline::Zero;
        ps.targetBlend[1].dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;
        ps.targetBlend[1].dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;

        TransparentPass::prep(*ctx, data, this, ps, shaderFeatures, oitrt.renderPassDescriptor, sortedTransparentObjects, true);
    }
}

void OITRenderPass::renderPass(QSSGRenderer &renderer)
{
    auto *ctx = renderer.contextInterface();
    const auto &rhiCtx = ctx->rhiContext();
    QSSG_ASSERT(rhiCtx->rhi()->isRecordingFrame(), return);
    QRhiCommandBuffer *cb = rhiCtx->commandBuffer();

    if (method == QSSGRenderLayer::OITMethod::WeightedBlended) {
        if (Q_LIKELY(renderTarget)) {
            cb->beginPass(renderTarget, Qt::black, {});

            QRhiShaderResourceBindings *srb = clearSrb;
            QSSG_ASSERT(srb, return);
            ps.flags.setFlag(QSSGRhiGraphicsPipelineState::Flag::BlendEnabled, false);
            QSSGRhiGraphicsPipelineStatePrivate::setShaderPipeline(ps, clearPipeline.get());
            renderer.rhiQuadRenderer()->recordRenderQuad(rhiCtx.get(), &ps, srb, renderTarget->renderPassDescriptor(), {});
            ps.flags.setFlag(QSSGRhiGraphicsPipelineState::Flag::BlendEnabled, true);

            cb->debugMarkBegin(QByteArrayLiteral("Quick3D render order-independent alpha"));
            Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DRenderPass);
            Q_TRACE(QSSG_renderPass_entry, QStringLiteral("Quick3D render order-independent alpha"));
            ps.flags.setFlag(QSSGRhiGraphicsPipelineState::Flag::DepthTestEnabled, true);
            ps.flags.setFlag(QSSGRhiGraphicsPipelineState::Flag::DepthWriteEnabled, false);
            TransparentPass::render(*ctx, ps, sortedTransparentObjects);
            cb->debugMarkEnd();
            Q_QUICK3D_PROFILE_END_WITH_STRING(QQuick3DProfiler::Quick3DRenderPass, 0, QByteArrayLiteral("transparent_order_independent_pass"));
            Q_TRACE(QSSG_renderPass_exit);

            cb->endPass();
        }
    }
}

QSSGRenderPass::Type OITRenderPass::passType() const
{
    if (method == QSSGRenderLayer::OITMethod::WeightedBlended)
        return Type::Standalone;
    return Type::Main;
}


void OITRenderPass::resetForFrame()
{
    sortedTransparentObjects.clear();
    ps = {};
    shaderFeatures = {};
    rhiAccumTexture = nullptr;
    rhiRevealageTexture = nullptr;
    rhiDepthTexture = nullptr;
}

void OITCompositePass::renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data)
{
    using namespace RenderHelpers;

    QSSG_ASSERT(!data.renderedCameras.isEmpty(), return);

    const auto &rhiCtx = renderer.contextInterface()->rhiContext();
    QSSG_ASSERT(rhiCtx->rhi()->isRecordingFrame(), return);
    const auto &shaderCache = renderer.contextInterface()->shaderCache();

    ps = data.getPipelineState();
    ps.samples = rhiCtx->mainPassSampleCount();
    ps.viewCount = rhiCtx->mainPassViewCount();

    if (method == QSSGRenderLayer::OITMethod::WeightedBlended) {
        rhiAccumTexture = data.getRenderResult(QSSGFrameData::RenderResult::AccumTexture);
        rhiRevealageTexture = data.getRenderResult(QSSGFrameData::RenderResult::RevealageTexture);
        compositeShaderPipeline = shaderCache->getBuiltInRhiShaders().getRhiOitCompositeShader(method, ps.samples > 1 ? true : false);
    }
}

void OITCompositePass::renderPass(QSSGRenderer &renderer)
{
    using namespace RenderHelpers;

    const auto &rhiCtx = renderer.contextInterface()->rhiContext();
    QSSG_ASSERT(rhiCtx->rhi()->isRecordingFrame(), return);
    QRhiCommandBuffer *cb = rhiCtx->commandBuffer();

    QSSGRhiContextPrivate *rhiCtxD = QSSGRhiContextPrivate::get(rhiCtx.get());

    if (!rhiAccumTexture->texture || !rhiRevealageTexture->texture)
        return;

    if (method == QSSGRenderLayer::OITMethod::WeightedBlended) {
        QSSGRhiShaderResourceBindingList bindings;

        QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Nearest,
                                                 QRhiSampler::Nearest,
                                                 QRhiSampler::None,
                                                 QRhiSampler::ClampToEdge,
                                                 QRhiSampler::ClampToEdge,
                                                 QRhiSampler::ClampToEdge });
        bindings.addTexture(1, QRhiShaderResourceBinding::FragmentStage, rhiAccumTexture->texture, sampler);
        bindings.addTexture(2, QRhiShaderResourceBinding::FragmentStage, rhiRevealageTexture->texture, sampler);

        compositeSrb = rhiCtxD->srb(bindings);

        QRhiShaderResourceBindings *srb = compositeSrb;
        QSSG_ASSERT(srb, return);

        cb->debugMarkBegin(QByteArrayLiteral("Quick3D revealage"));
        QSSGRhiGraphicsPipelineStatePrivate::setShaderPipeline(ps, compositeShaderPipeline.get());
        ps.flags.setFlag(QSSGRhiGraphicsPipelineState::Flag::BlendEnabled, true);
        renderer.rhiQuadRenderer()->recordRenderQuad(rhiCtx.get(), &ps, srb, rhiCtx->mainRenderPassDescriptor(),
                                                     { QSSGRhiQuadRenderer::UvCoords | QSSGRhiQuadRenderer::DepthTest});
        Q_QUICK3D_PROFILE_END_WITH_STRING(QQuick3DProfiler::Quick3DRenderPass, 0, QByteArrayLiteral("revealage"));
        cb->debugMarkEnd();
    }
}

void OITCompositePass::resetForFrame()
{
    ps = {};
    shaderFeatures = {};
    rhiAccumTexture = nullptr;
    rhiRevealageTexture = nullptr;
}

QT_END_NAMESPACE
