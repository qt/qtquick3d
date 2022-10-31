// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qssgrenderpass_p.h"
#include "qssgrhiquadrenderer_p.h"
#include "qssglayerrenderdata_p.h"
#include "qssgrendercontextcore_p.h"
#include "qssgdebugdrawsystem_p.h"

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
// SHADOW PASS

void ShadowMapPass::renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data)
{
    Q_UNUSED(renderer)
    using namespace RenderHelpers;

    camera = data.camera;
    QSSG_ASSERT(camera, return);

    const auto &renderedDepthWriteObjects = data.getSortedRenderedDepthWriteObjects();
    const auto &renderedOpaqueDepthPrepassObjects = data.getSortedrenderedOpaqueDepthPrepassObjects();

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
        ps.depthTestEnable = true;
        ps.depthWriteEnable = true;
        // Try reducing self-shadowing and artifacts.
        ps.depthBias = 2;
        ps.slopeScaledDepthBias = 1.5f;

        const auto &sortedOpaqueObjects = data.getSortedOpaqueRenderableObjects();
        const auto &sortedTransparentObjects = data.getSortedTransparentRenderableObjects();
        const auto [casting, receiving] = calculateSortedObjectBounds(sortedOpaqueObjects,
                                                                      sortedTransparentObjects);
        castingObjectsBox = casting;
        receivingObjectsBox = receiving;
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
                           globalLights, // scoped lights are not relevant here
                           shadowPassObjects,
                           renderer,
                           castingObjectsBox,
                           receivingObjectsBox);

        cb->debugMarkEnd();
        Q_QUICK3D_PROFILE_END_WITH_STRING(QQuick3DProfiler::Quick3DRenderPass, 0, QByteArrayLiteral("shadow_map"));
    }
}

void ShadowMapPass::release()
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

    ps = data.getPipelineState();
    ps.depthTestEnable = true;
    ps.depthWriteEnable = true;
    ps.blendEnable = true;

    reflectionProbes = data.reflectionProbes;
    reflectionMapManager = data.requestReflectionMapManager();

    const auto &sortedOpaqueObjects = data.getSortedOpaqueRenderableObjects();
    const auto &sortedTransparentObjects = data.getSortedTransparentRenderableObjects();

    QSSG_ASSERT(reflectionPassObjects.isEmpty(), reflectionPassObjects.clear());

    // NOTE: We should consider keeping track of the reflection casting objects to avoid
    // filtering this list on each prep.
    for (const auto &handles : { &sortedOpaqueObjects, &sortedTransparentObjects }) {
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

    // TODO: Workaroud as we using the layer data in rhiRenderReflectionMap() consider
    // if we can extract the data we need for rendering in the prep step...
    QSSG_ASSERT(renderer.getLayerGlobalRenderProperties().layer.renderData, return);
    const auto &data = *renderer.getLayerGlobalRenderProperties().layer.renderData;

    QSSG_CHECK(reflectionMapManager);
    if (!reflectionPassObjects.isEmpty() || !reflectionProbes.isEmpty()) {
        cb->debugMarkBegin(QByteArrayLiteral("Quick3D reflection map"));
        Q_TRACE_SCOPE(QSSG_renderPass, QStringLiteral("Quick3D reflection map"));
        Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DRenderPass);
        rhiRenderReflectionMap(rhiCtx.get(),
                               this,
                               data,
                               &ps,
                               *reflectionMapManager,
                               reflectionProbes,
                               reflectionPassObjects,
                               renderer);

        cb->debugMarkEnd();
        Q_QUICK3D_PROFILE_END_WITH_STRING(QQuick3DProfiler::Quick3DRenderPass, 0, QByteArrayLiteral("reflection_map"));
    }
}

void ReflectionMapPass::release()
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

    // CONDITION: Input + globally enabled or ?

    const auto &rhiCtx = renderer.contextInterface()->rhiContext();
    QSSG_ASSERT(rhiCtx->rhi()->isRecordingFrame(), return);
    QRhiCommandBuffer *cb = rhiCtx->commandBuffer();
    ps = data.getPipelineState();

    renderedDepthWriteObjects = data.getSortedRenderedDepthWriteObjects();
    renderedOpaqueDepthPrepassObjects = data.getSortedrenderedOpaqueDepthPrepassObjects();

    const auto &layer = data.layer;
    const bool hasItem2Ds = data.renderableItem2Ds.isEmpty();
    bool zPrePass = layer.layerFlags.testFlag(QSSGRenderLayer::LayerFlag::EnableDepthPrePass)
            && layer.layerFlags.testFlag(QSSGRenderLayer::LayerFlag::EnableDepthTest)
            && (!renderedDepthWriteObjects.isEmpty() || hasItem2Ds);
    if (zPrePass || !renderedOpaqueDepthPrepassObjects.isEmpty()) {
        cb->debugMarkBegin(QByteArrayLiteral("Quick3D prepare Z prepass"));
        Q_TRACE_SCOPE(QSSG_renderPass, QStringLiteral("Quick3D prepare Z prepass"));
        Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DRenderPass);
        if (!zPrePass) {
            state = { State::Forced };
            rhiPrepareDepthPass(rhiCtx.get(), this, ps, rhiCtx->mainRenderPassDescriptor(), data,
                                {}, renderedOpaqueDepthPrepassObjects,
                                QSSGRhiDrawCallDataKey::ZPrePass,
                                rhiCtx->mainPassSampleCount());
        } else if (rhiPrepareDepthPass(rhiCtx.get(), this, ps, rhiCtx->mainRenderPassDescriptor(), data,
                                    renderedDepthWriteObjects, renderedOpaqueDepthPrepassObjects,
                                    QSSGRhiDrawCallDataKey::ZPrePass,
                                    rhiCtx->mainPassSampleCount())) {
                state = { State::Active };
        }
        cb->debugMarkEnd();
        Q_QUICK3D_PROFILE_END_WITH_STRING(QQuick3DProfiler::Quick3DRenderPass, 0, QByteArrayLiteral("prepare_z_prepass"));
    }
}

void ZPrePassPass::renderPass(QSSGRenderer &renderer)
{
    using namespace RenderHelpers;

    const auto &rhiCtx = renderer.contextInterface()->rhiContext();
    QSSG_ASSERT(rhiCtx->rhi()->isRecordingFrame(), return);

    bool needsSetViewport = true;
    QRhiCommandBuffer *cb = rhiCtx->commandBuffer();

    if (state == State::Active) {
        Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DRenderPass);
        Q_TRACE_SCOPE(QSSG_renderPass, QStringLiteral("Quick3D render Z prepass"));
        cb->debugMarkBegin(QByteArrayLiteral("Quick3D render Z prepass"));
        rhiRenderDepthPass(rhiCtx.get(), ps, renderedDepthWriteObjects, renderedOpaqueDepthPrepassObjects, &needsSetViewport);
        cb->debugMarkEnd();
        Q_QUICK3D_PROFILE_END_WITH_STRING(QQuick3DProfiler::Quick3DRenderPass, 0, QByteArrayLiteral("render_z_prepass"));
    } else if (state == State::Forced) {
        Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DRenderPass);
        Q_TRACE_SCOPE(QSSG_renderPass, QStringLiteral("Quick3D render Z forced prepass"));
        cb->debugMarkBegin(QByteArrayLiteral("Quick3D render Z forced prepass"));
        rhiRenderDepthPass(rhiCtx.get(), ps, {}, renderedOpaqueDepthPrepassObjects, &needsSetViewport);
        cb->debugMarkEnd();
        Q_QUICK3D_PROFILE_END_WITH_STRING(QQuick3DProfiler::Quick3DRenderPass, 0, QByteArrayLiteral("render_z_prepass_forced"));
    }
}

void ZPrePassPass::release()
{
    renderedDepthWriteObjects.clear();
    renderedOpaqueDepthPrepassObjects.clear();
    ps = {};
    state = { State::Disabled };
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

    rhiDepthTexture = &data.depthMapPass.rhiDepthTexture;
    camera = data.camera;
    QSSG_ASSERT_X((camera && rhiDepthTexture->isValid()), "Preparing AO pass failed, missing camera or depth texture", return);

    ssaoShaderPipeline = data.renderer->getRhiSsaoShader();
    ambientOcclusion = { data.layer.aoStrength, data.layer.aoDistance, data.layer.aoSoftness, data.layer.aoBias, data.layer.aoSamplerate, data.layer.aoDither };

    ps = data.getPipelineState();
    const auto &layerPrepResult = data.layerPrepResult;
    const bool ready = rhiPrepareAoTexture(rhiCtx.get(), layerPrepResult->textureDimensions(), &rhiAoTexture);

    if (Q_UNLIKELY(!ready))
        rhiAoTexture.reset();
}

void SSAOMapPass::renderPass(QSSGRenderer &renderer)
{
    using namespace RenderHelpers;

    // INPUT: Camera + depth map

    // DEPENDECY: Depth map (zprepass)

    // OUTPUT: AO Texture

    // NOTE:

    // CONDITION: SSAO enabled
    QSSG_ASSERT(camera, return);
    QSSG_ASSERT(rhiDepthTexture && rhiDepthTexture->isValid(), return);

    const auto &rhiCtx = renderer.contextInterface()->rhiContext();
    QSSG_ASSERT(rhiCtx->rhi()->isRecordingFrame(), return);

    QRhiCommandBuffer *cb = rhiCtx->commandBuffer();
    cb->debugMarkBegin(QByteArrayLiteral("Quick3D SSAO map"));
    Q_TRACE_SCOPE(QSSG_renderPass, QStringLiteral("Quick3D SSAO map"));
    Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DRenderPass);

    if (Q_LIKELY(rhiAoTexture.isValid()))
        rhiRenderAoTexture(rhiCtx.get(),
                           this,
                           renderer,
                           *ssaoShaderPipeline,
                           ps,
                           ambientOcclusion,
                           rhiAoTexture,
                           *rhiDepthTexture,
                           *camera);

    cb->debugMarkEnd();
    Q_QUICK3D_PROFILE_END_WITH_STRING(QQuick3DProfiler::Quick3DRenderPass, 0, QByteArrayLiteral("ssao_map"));
}

void SSAOMapPass::release()
{
    rhiDepthTexture = nullptr;
    camera = nullptr;
    rhiAoTexture.reset();
    ps = {};
    ao = AmbientOcclusion();
}

// DEPTH TEXTURE PASS
void DepthMapPass::renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data)
{
    using namespace RenderHelpers;

    const auto &rhiCtx = renderer.contextInterface()->rhiContext();
    QSSG_ASSERT(rhiCtx->rhi()->isRecordingFrame(), return);
    const auto &layerPrepResult = data.layerPrepResult;
    bool ready = false;
    ps = data.getPipelineState();
    if (Q_LIKELY(rhiPrepareDepthTexture(rhiCtx.get(), layerPrepResult->textureDimensions(), &rhiDepthTexture))) {
        sortedOpaqueObjects = data.getSortedOpaqueRenderableObjects();
        sortedTransparentObjects = data.getSortedTransparentRenderableObjects();
        ready = rhiPrepareDepthPass(rhiCtx.get(), this, ps, rhiDepthTexture.rpDesc, data,
                                    sortedOpaqueObjects, sortedTransparentObjects,
                                    QSSGRhiDrawCallDataKey::DepthTexture,
                                    1);
    }

    if (Q_UNLIKELY(!ready))
        rhiDepthTexture.reset();
}

void DepthMapPass::renderPass(QSSGRenderer &renderer)
{
    using namespace RenderHelpers;

    // INPUT: sorted objects (opaque + transparent) (maybe...)

    // DEPENDECY: If this is only used for the AO case, that dictates if this should be done or not.

    // OUTPUT: Texture

    // NOTE: Why are we prepping opaque + transparent object if we're not using them? And why are we staying compatible with 5.15?
    //       Only used for AO? Merge into the AO pass?

    // CONDITION:

    const auto &rhiCtx = renderer.contextInterface()->rhiContext();
    QSSG_ASSERT(rhiCtx->rhi()->isRecordingFrame(), return);
    QRhiCommandBuffer *cb = rhiCtx->commandBuffer();
    cb->debugMarkBegin(QByteArrayLiteral("Quick3D depth texture"));

    if (Q_LIKELY(rhiDepthTexture.isValid())) {
        bool needsSetViewport = true;
        cb->beginPass(rhiDepthTexture.rt, Qt::transparent, { 1.0f, 0 }, nullptr, QSSGRhiContext::commonPassFlags());
        QSSGRHICTX_STAT(rhiCtx, beginRenderPass(rhiDepthTexture.rt));
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

void DepthMapPass::release()
{
    rhiDepthTexture.reset();
    sortedOpaqueObjects.clear();
    sortedTransparentObjects.clear();
    ps = {};
}

// SCREEN TEXTURE PASS

void ScreenMapPass::renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data)
{
    using namespace RenderHelpers;

    const auto &rhiCtx = renderer.contextInterface()->rhiContext();
    QSSG_ASSERT(rhiCtx->rhi()->isRecordingFrame(), return);
    auto camera = data.camera;
    QSSG_ASSERT(camera, return);
    auto &layer = data.layer;
    const auto &layerPrepResult = data.layerPrepResult;
    wantsMips = layerPrepResult->flags.requiresMipmapsForScreenTexture();
    sortedOpaqueObjects = data.getSortedOpaqueRenderableObjects();
    const auto &renderedOpaqueDepthPrepassObjects = data.getSortedrenderedOpaqueDepthPrepassObjects();
    const auto &renderedDepthWriteObjects = data.getSortedRenderedDepthWriteObjects();
    ps = data.getPipelineState();

    if (layer.background == QSSGRenderLayer::Background::Color)
        clearColor = QColor::fromRgbF(layer.clearColor.x(), layer.clearColor.y(), layer.clearColor.z());

    if ((layer.background == QSSGRenderLayer::Background::SkyBox && layer.lightProbe)
            || (layer.background == QSSGRenderLayer::Background::SkyBoxCubeMap && layer.skyBoxCubeMap))
    {
        if (!data.plainSkyBoxPrepared) {
            data.plainSkyBoxPrepared = true;
            rhiPrepareSkyBox(rhiCtx.get(), this, layer, *camera, renderer);
        }
    }

    const bool layerEnableDepthTest = layer.layerFlags.testFlag(QSSGRenderLayer::LayerFlag::EnableDepthTest);
    const bool depthTestEnableDefault = layerEnableDepthTest && (!sortedOpaqueObjects.isEmpty() || !renderedOpaqueDepthPrepassObjects.isEmpty() || !renderedDepthWriteObjects.isEmpty());
    // enable depth write for opaque objects when there was no Z prepass
    const bool depthWriteEnableDefault = depthTestEnableDefault && (!layer.layerFlags.testFlag(QSSGRenderLayer::LayerFlag::EnableDepthPrePass) || (data.zPrePassPass.state != ZPrePassPass::State::Active));

    ps.depthTestEnable = depthTestEnableDefault;
    ps.depthWriteEnable = depthWriteEnableDefault;

    bool ready = false;
    if (Q_LIKELY(rhiPrepareScreenTexture(rhiCtx.get(), layerPrepResult->textureDimensions(), wantsMips, &rhiScreenTexture))) {
        ready = true;
        // NB: not compatible with disabling LayerEnableDepthTest
        // because there are effectively no "opaque" objects then.
        // Disable Tonemapping for all materials in the screen pass texture
        shaderFeatures = data.getShaderFeatures();
        shaderFeatures.disableTonemapping();
        const auto &sortedOpaqueObjects = data.getSortedOpaqueRenderableObjects();
        for (const auto &handle : sortedOpaqueObjects)
            rhiPrepareRenderable(rhiCtx.get(), this, data, *handle.obj, rhiScreenTexture.rpDesc, &ps, shaderFeatures, 1);
    }

    if (Q_UNLIKELY(!ready))
        rhiScreenTexture.reset();
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

    if (Q_LIKELY(rhiScreenTexture.isValid())) {
        const auto &layer = renderer.getLayerGlobalRenderProperties().layer;
        cb->beginPass(rhiScreenTexture.rt, clearColor, { 1.0f, 0 }, nullptr, QSSGRhiContext::commonPassFlags());
        QSSGRHICTX_STAT(rhiCtx, beginRenderPass(rhiScreenTexture.rt));
        Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DRenderPass);

        if (layer.background == QSSGRenderLayer::Background::SkyBox
            && rhiCtx->rhi()->isFeatureSupported(QRhi::TexelFetch) && layer.skyBoxSrb) {
            // This is offscreen, so rendered untonemapped
            auto shaderPipeline = renderer.getRhiSkyBoxShader(QSSGRenderLayer::TonemapMode::None, layer.skyBoxIsRgbe8);
            QSSG_CHECK(shaderPipeline);
            ps.shaderPipeline = shaderPipeline.get();
            QRhiShaderResourceBindings *srb = layer.skyBoxSrb;
            QRhiRenderPassDescriptor *rpDesc = rhiCtx->mainRenderPassDescriptor();
            renderer.rhiQuadRenderer()->recordRenderQuad(rhiCtx.get(), &ps, srb, rpDesc, {});
        } else if (layer.background == QSSGRenderLayer::Background::SkyBoxCubeMap
                   && rhiCtx->rhi()->isFeatureSupported(QRhi::TexelFetch) && layer.skyBoxSrb) {
            auto shaderPipeline = renderer.getRhiSkyBoxCubeShader();
            QSSG_CHECK(shaderPipeline);
            ps.shaderPipeline = shaderPipeline.get();
            QRhiShaderResourceBindings *srb = layer.skyBoxSrb;
            QRhiRenderPassDescriptor *rpDesc = rhiCtx->mainRenderPassDescriptor();
            renderer.rhiCubeRenderer()->recordRenderCube(rhiCtx.get(), &ps, srb, rpDesc, {});
        }
        bool needsSetViewport = true;
        for (const auto &handle : std::as_const(sortedOpaqueObjects))
            rhiRenderRenderable(rhiCtx.get(), ps, *handle.obj, &needsSetViewport);

        QRhiResourceUpdateBatch *rub = nullptr;
        if (wantsMips) {
            rub = rhiCtx->rhi()->nextResourceUpdateBatch();
            rub->generateMips(rhiScreenTexture.texture);
        }
        cb->endPass(rub);
        QSSGRHICTX_STAT(rhiCtx, endRenderPass());
        Q_QUICK3D_PROFILE_END_WITH_STRING(QQuick3DProfiler::Quick3DRenderPass, 0, QByteArrayLiteral("screen_texture"));
    }

    cb->debugMarkEnd();
}

void ScreenMapPass::release()
{
    rhiScreenTexture.reset();
    ps = {};
    wantsMips = false;
    clearColor = Qt::transparent;
    shaderFeatures = {};
    sortedOpaqueObjects.clear();
}

// MAIN PASS
void MainPass::renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data)
{
    using namespace RenderHelpers;

    // INPUT: Everything available

    // DEPENDECY: Result from previous passes

    // OUTPUT: Screen

    // NOTE: (Does not write to the depth buffer as that's done in the ZPrePass).

    // CONDITION: Always

    const auto &rhiCtx = renderer.contextInterface()->rhiContext();
    QSSG_ASSERT(rhiCtx->rhi()->isRecordingFrame(), return);
    auto camera = data.camera;
    QSSG_ASSERT(camera, return);

    ps = data.getPipelineState();
    ps.depthFunc = QRhiGraphicsPipeline::LessOrEqual;
    ps.blendEnable = false;

    auto &layer = data.layer;
    shaderFeatures = data.getShaderFeatures();

    if ((layer.background == QSSGRenderLayer::Background::SkyBox && layer.lightProbe)
            || (layer.background == QSSGRenderLayer::Background::SkyBoxCubeMap && layer.skyBoxCubeMap))
    {
        if (!data.plainSkyBoxPrepared) {
            data.plainSkyBoxPrepared = true;
            rhiPrepareSkyBox(rhiCtx.get(), this, layer, *camera, renderer);
        }
    }

    {
        const auto &clippingFrustum = data.clippingFrustum;
        const auto &opaqueObjects = data.getSortedOpaqueRenderableObjects();
        const auto &transparentObject = data.getSortedTransparentRenderableObjects();
        if (clippingFrustum.has_value()) {
            QSSGLayerRenderData::frustumCulling(clippingFrustum.value(), opaqueObjects, sortedOpaqueObjects);
            QSSGLayerRenderData::frustumCulling(clippingFrustum.value(), transparentObject, sortedTransparentObjects);
        } else {
            sortedOpaqueObjects = opaqueObjects;
            sortedTransparentObjects = transparentObject;
        }
    }
    const bool layerEnableDepthTest = layer.layerFlags.testFlag(QSSGRenderLayer::LayerFlag::EnableDepthTest);
    const auto &renderedOpaqueDepthPrepassObjects = data.getSortedrenderedOpaqueDepthPrepassObjects();
    const auto &renderedDepthWriteObjects = data.getSortedRenderedDepthWriteObjects();
    const bool depthTestEnableDefault = layerEnableDepthTest && (!sortedOpaqueObjects.isEmpty() || !renderedOpaqueDepthPrepassObjects.isEmpty() || !renderedDepthWriteObjects.isEmpty());;
    const bool depthWriteEnableDefault = depthTestEnableDefault && (!layer.layerFlags.testFlag(QSSGRenderLayer::LayerFlag::EnableDepthPrePass) || (data.zPrePassPass.state != ZPrePassPass::State::Active));;

    ps.depthTestEnable = depthTestEnableDefault;
    // enable depth write for opaque objects when there was no Z prepass
    ps.depthWriteEnable = depthWriteEnableDefault;

    // Enable Wireframe mode
    ps.polygonMode = layer.wireframeMode ? QRhiGraphicsPipeline::Line : QRhiGraphicsPipeline::Fill;

    // make the buffer copies and other stuff we put on the command buffer in
    // here show up within a named section in tools like RenderDoc when running
    // with QSG_RHI_PROFILE=1 (which enables debug markers)
    QRhiCommandBuffer *cb = rhiCtx->commandBuffer();
    cb->debugMarkBegin(QByteArrayLiteral("Quick3D prepare renderables"));
    Q_TRACE_SCOPE(QSSG_renderPass, QStringLiteral("Quick3D prepare renderables"));
    Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DRenderPass);

    QRhiRenderPassDescriptor *mainRpDesc = rhiCtx->mainRenderPassDescriptor();
    const int samples = rhiCtx->mainPassSampleCount();

    // opaque objects (or, this list is empty when LayerEnableDepthTest is disabled)
    for (const auto &handle : std::as_const(sortedOpaqueObjects)) {
        QSSGRenderableObject *theObject = handle.obj;
        const auto depthWriteMode = theObject->depthWriteMode;
        ps.depthWriteEnable = !(depthWriteMode == QSSGDepthDrawMode::Never ||
                                depthWriteMode == QSSGDepthDrawMode::OpaquePrePass ||
                                (data.zPrePassPass.state == ZPrePassPass::State::Active) || !layerEnableDepthTest);
        rhiPrepareRenderable(rhiCtx.get(), this, data, *theObject, mainRpDesc, &ps, shaderFeatures, samples);
    }

    // objects that requires the screen texture
    ps.depthTestEnable = depthTestEnableDefault;
    ps.depthWriteEnable = depthWriteEnableDefault;
    sortedScreenTextureObjects = data.getSortedScreenTextureRenderableObjects();
    for (const auto &handle : std::as_const(sortedScreenTextureObjects)) {
        QSSGRenderableObject *theObject = handle.obj;
        const auto depthWriteMode = theObject->depthWriteMode;
        ps.blendEnable = theObject->renderableFlags.hasTransparency();
        ps.depthWriteEnable = !(depthWriteMode == QSSGDepthDrawMode::Never || depthWriteMode == QSSGDepthDrawMode::OpaquePrePass
                                 || (data.zPrePassPass.state == ZPrePassPass::State::Active) || !layerEnableDepthTest);
        rhiPrepareRenderable(rhiCtx.get(), this, data, *theObject, mainRpDesc, &ps, shaderFeatures, samples);
    }

    // objects rendered by Qt Quick 2D
    ps.depthTestEnable = depthTestEnableDefault;
    ps.depthWriteEnable = depthWriteEnableDefault;
    ps.blendEnable = false;

    item2Ds = data.getRenderableItem2Ds();
    for (const auto &item2D: std::as_const(item2Ds)) {
        // Set the projection matrix
        if (!item2D->m_renderer)
            continue;
        if (item2D->m_renderer && item2D->m_renderer->currentRhi() != renderer.contextInterface()->rhi()) {
            static bool contextWarningShown = false;
            if (!contextWarningShown) {
                qWarning () << "Scene with embedded 2D content can only be rendered in one window.";
                contextWarningShown = true;
            }
            continue;
        }

        auto layerPrepResult = data.layerPrepResult;

        const auto &renderTarget = rhiCtx->renderTarget();
        item2D->m_renderer->setDevicePixelRatio(renderTarget->devicePixelRatio());
        const QRect deviceRect(QPoint(0, 0), renderTarget->pixelSize());
        if (layer.scissorRect.isValid()) {
            QRect effScissor = layer.scissorRect & layerPrepResult->viewport.toRect();
            QMatrix4x4 correctionMat = correctMVPForScissor(layerPrepResult->viewport,
                                                            effScissor,
                                                            rhiCtx->rhi()->isYUpInNDC());
            item2D->m_renderer->setProjectionMatrix(correctionMat * item2D->MVP);
            item2D->m_renderer->setViewportRect(effScissor);
        } else {
            item2D->m_renderer->setProjectionMatrix(item2D->MVP);
            item2D->m_renderer->setViewportRect(correctViewportCoordinates(layerPrepResult->viewport, deviceRect));
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
        item2D->m_renderer->setRenderTarget({ renderTarget, item2D->m_rp, rhiCtx->commandBuffer() });
        delete oldRp;
        item2D->m_renderer->prepareSceneInline();
    }

    // transparent objects (or, without LayerEnableDepthTest, all objects)
    ps.blendEnable = true;
    ps.depthWriteEnable = false;

    for (const auto &handle : std::as_const(sortedTransparentObjects)) {
        QSSGRenderableObject *theObject = handle.obj;
        const auto depthWriteMode = theObject->depthWriteMode;
        ps.depthWriteEnable = (depthWriteMode == QSSGDepthDrawMode::Always && (data.zPrePassPass.state != ZPrePassPass::State::Active));
        if (!(theObject->renderableFlags.isCompletelyTransparent()))
            rhiPrepareRenderable(rhiCtx.get(), this, data, *theObject, mainRpDesc, &ps, shaderFeatures, samples);
    }


    if (layer.gridEnabled)
        rhiPrepareGrid(rhiCtx.get(), layer, *camera, renderer);

    // debug objects
    const auto &debugDraw = renderer.contextInterface()->debugDrawSystem();
    if (debugDraw && debugDraw->hasContent()) {
        QRhiResourceUpdateBatch *rub = rhiCtx->rhi()->nextResourceUpdateBatch();
        debugDraw->prepareGeometry(rhiCtx.get(), rub);
        QSSGRhiDrawCallData &dcd = rhiCtx->drawCallData({ &layer, nullptr, nullptr, 0, QSSGRhiDrawCallDataKey::DebugObjects });
        if (!dcd.ubuf) {
            dcd.ubuf = rhiCtx->rhi()->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64);
            dcd.ubuf->create();
        }
        char *ubufData = dcd.ubuf->beginFullDynamicBufferUpdateForCurrentFrame();
        QMatrix4x4 viewProjection(Qt::Uninitialized);
        camera->calculateViewProjectionMatrix(viewProjection);
        viewProjection = rhiCtx->rhi()->clipSpaceCorrMatrix() * viewProjection;
        memcpy(ubufData, viewProjection.constData(), 64);
        dcd.ubuf->endFullDynamicBufferUpdateForCurrentFrame();

        QSSGRhiShaderResourceBindingList bindings;
        bindings.addUniformBuffer(0, QRhiShaderResourceBinding::VertexStage, dcd.ubuf);
        dcd.srb = rhiCtx->srb(bindings);

        rhiCtx->commandBuffer()->resourceUpdate(rub);
    }

    cb->debugMarkEnd();
    Q_QUICK3D_PROFILE_END_WITH_STRING(QQuick3DProfiler::Quick3DRenderPass, 0, QByteArrayLiteral("prepare_renderables"));
}

void MainPass::renderPass(QSSGRenderer &renderer)
{
    using namespace RenderHelpers;

    const auto &rhiCtx = renderer.contextInterface()->rhiContext();
    QSSG_ASSERT(rhiCtx->rhi()->isRecordingFrame(), return);
    QRhiCommandBuffer *cb = rhiCtx->commandBuffer();

    bool needsSetViewport = true;

    // 1. Render opaque objects
    cb->debugMarkBegin(QByteArrayLiteral("Quick3D render opaque"));
    Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DRenderPass);
    Q_TRACE(QSSG_renderPass_entry, QStringLiteral("Quick3D render opaque"));
    for (const auto &handle : std::as_const(sortedOpaqueObjects)) {
        QSSGRenderableObject *theObject = handle.obj;
        rhiRenderRenderable(rhiCtx.get(), ps, *theObject, &needsSetViewport);
    }
    cb->debugMarkEnd();
    Q_QUICK3D_PROFILE_END_WITH_STRING(QQuick3DProfiler::Quick3DRenderPass, 0, QByteArrayLiteral("opaque_pass"));
    Q_TRACE(QSSG_renderPass_exit);

    // 2. Render sky box (opt)

    const auto &layer = renderer.getLayerGlobalRenderProperties().layer;
    auto polygonMode = ps.polygonMode;
    ps.polygonMode = QRhiGraphicsPipeline::Fill;
    if (layer.background == QSSGRenderLayer::Background::SkyBoxCubeMap
        && rhiCtx->rhi()->isFeatureSupported(QRhi::TexelFetch)
        && layer.skyBoxSrb)
    {
        Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DRenderPass);
        Q_TRACE_SCOPE(QSSG_renderPass, QStringLiteral("Quick3D render skybox"));
        auto shaderPipeline = renderer.getRhiSkyBoxCubeShader();
        QSSG_CHECK(shaderPipeline);
        ps.shaderPipeline = shaderPipeline.get();
        QRhiShaderResourceBindings *srb = layer.skyBoxSrb;
        QRhiRenderPassDescriptor *rpDesc = rhiCtx->mainRenderPassDescriptor();
        renderer.rhiCubeRenderer()->recordRenderCube(rhiCtx.get(), &ps, srb, rpDesc, { QSSGRhiQuadRenderer::DepthTest | QSSGRhiQuadRenderer::RenderBehind });
        Q_QUICK3D_PROFILE_END_WITH_STRING(QQuick3DProfiler::Quick3DRenderPass, 0, QByteArrayLiteral("skybox_cube"));

    } else if (layer.background == QSSGRenderLayer::Background::SkyBox
               && rhiCtx->rhi()->isFeatureSupported(QRhi::TexelFetch)
               && layer.skyBoxSrb)
    {
        Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DRenderPass);
        Q_TRACE_SCOPE(QSSG_renderPass, QStringLiteral("Quick3D render skybox"));
        QSSGRenderLayer::TonemapMode tonemapMode = layer.tonemapMode;
        // When there are effects, then it is up to the last pass of the
        // last effect to perform tonemapping, neither the skybox nor the
        // main render pass should alter the colors then.
        if (layer.firstEffect)
            tonemapMode = QSSGRenderLayer::TonemapMode::None;

        auto shaderPipeline = renderer.getRhiSkyBoxShader(tonemapMode, layer.skyBoxIsRgbe8);
        QSSG_CHECK(shaderPipeline);
        ps.shaderPipeline = shaderPipeline.get();
        QRhiShaderResourceBindings *srb = layer.skyBoxSrb;
        QRhiRenderPassDescriptor *rpDesc = rhiCtx->mainRenderPassDescriptor();
        renderer.rhiQuadRenderer()->recordRenderQuad(rhiCtx.get(), &ps, srb, rpDesc, { QSSGRhiQuadRenderer::DepthTest | QSSGRhiQuadRenderer::RenderBehind });
        Q_QUICK3D_PROFILE_END_WITH_STRING(QQuick3DProfiler::Quick3DRenderPass, 0, QByteArrayLiteral("skybox_map"));
    }
    ps.polygonMode = polygonMode;

    // 3. Screen texture depended objects
    cb->debugMarkBegin(QByteArrayLiteral("Quick3D render screen texture dependent"));
    Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DRenderPass);
    Q_TRACE(QSSG_renderPass_entry, QStringLiteral("Quick3D render screen texture dependent"));
    for (const auto &handle : std::as_const(sortedScreenTextureObjects)) {
        QSSGRenderableObject *theObject = handle.obj;
        rhiRenderRenderable(rhiCtx.get(), ps, *theObject, &needsSetViewport);
    }
    cb->debugMarkEnd();
    Q_QUICK3D_PROFILE_END_WITH_STRING(QQuick3DProfiler::Quick3DRenderPass, 0, QByteArrayLiteral("screen_texture_dependent"));
    Q_TRACE(QSSG_renderPass_exit);

    // 4. Item2Ds
    if (!item2Ds.isEmpty()) {
        cb->debugMarkBegin(QByteArrayLiteral("Quick3D render 2D sub-scene"));
        Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DRenderPass);
        Q_TRACE_SCOPE(QSSG_renderPass, QStringLiteral("Quick3D render 2D sub-scene"));
        for (const auto &item : std::as_const(item2Ds)) {
            QSSGRenderItem2D *item2D = static_cast<QSSGRenderItem2D *>(item);
            if (item2D->m_renderer && item2D->m_renderer->currentRhi() == renderer.contextInterface()->rhi())
                item2D->m_renderer->renderSceneInline();
        }
        cb->debugMarkEnd();
        Q_QUICK3D_PROFILE_END_WITH_STRING(QQuick3DProfiler::Quick3DRenderPass, 0, QByteArrayLiteral("2D_sub_scene"));
    }

    // 5. Non-opaque objects
    cb->debugMarkBegin(QByteArrayLiteral("Quick3D render alpha"));
    Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DRenderPass);
    Q_TRACE(QSSG_renderPass_entry, QStringLiteral("Quick3D render alpha"));
    // If scissorRect is set, Item2Ds will be drawn by a workaround of modifying
    // viewport, not using actual 3D scissor test.
    // It means non-opaque objects may be affected by this viewport setting.
    needsSetViewport = true;
    for (const auto &handle : std::as_const(sortedTransparentObjects)) {
        QSSGRenderableObject *theObject = handle.obj;
        if (!theObject->renderableFlags.isCompletelyTransparent())
            rhiRenderRenderable(rhiCtx.get(), ps, *theObject, &needsSetViewport);
    }
    cb->debugMarkEnd();
    Q_QUICK3D_PROFILE_END_WITH_STRING(QQuick3DProfiler::Quick3DRenderPass, 0, QByteArrayLiteral("transparent_pass"));
    Q_TRACE(QSSG_renderPass_exit);

    // 6. Infinite grid
    if (layer.gridEnabled) {
        cb->debugMarkBegin(QByteArrayLiteral("Quick3D render grid"));
        Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DRenderPass);
        Q_TRACE_SCOPE(QSSG_renderPass, QStringLiteral("Quick3D render grid"));
        const auto &shaderPipeline = renderer.getRhiGridShader();
        Q_ASSERT(shaderPipeline);
        ps.shaderPipeline = shaderPipeline.get();
        QRhiShaderResourceBindings *srb = layer.gridSrb;
        QRhiRenderPassDescriptor *rpDesc = rhiCtx->mainRenderPassDescriptor();
        renderer.rhiQuadRenderer()->recordRenderQuad(rhiCtx.get(), &ps, srb, rpDesc, { QSSGRhiQuadRenderer::DepthTest });
        Q_QUICK3D_PROFILE_END_WITH_STRING(QQuick3DProfiler::Quick3DRenderPass, 0, QByteArrayLiteral("render_grid"));
    }

    // 7. Debug Draw content
    const auto &debugDraw = renderer.contextInterface()->debugDrawSystem();
    if (debugDraw && debugDraw->hasContent()) {
        cb->debugMarkBegin(QByteArrayLiteral("Quick 3D debug objects"));
        Q_TRACE_SCOPE(QSSG_renderPass, QStringLiteral("Quick 3D debug objects"));
        Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DRenderPass);
        auto shaderPipeline = renderer.getRhiDebugObjectShader();
        QSSG_CHECK(shaderPipeline);
        ps.shaderPipeline = shaderPipeline.get();
        QSSGRhiDrawCallData &dcd = rhiCtx->drawCallData({ &layer, nullptr, nullptr, 0, QSSGRhiDrawCallDataKey::DebugObjects });
        QRhiShaderResourceBindings *srb = dcd.srb;
        QRhiRenderPassDescriptor *rpDesc = rhiCtx->mainRenderPassDescriptor();
        debugDraw->recordRenderDebugObjects(rhiCtx.get(), &ps, srb, rpDesc);
        cb->debugMarkEnd();
        Q_QUICK3D_PROFILE_END_WITH_STRING(QQuick3DProfiler::Quick3DRenderPass, 0, QByteArrayLiteral("debug_objects"));
    }
}

void MainPass::release()
{
    ps = {};
    shaderFeatures = {};
    sortedOpaqueObjects.clear();
    sortedTransparentObjects.clear();
    sortedScreenTextureObjects.clear();
    item2Ds.clear();
}

QT_END_NAMESPACE
