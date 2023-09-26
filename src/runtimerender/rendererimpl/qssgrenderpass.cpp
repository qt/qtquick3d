// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qssgrenderpass_p.h"
#include "qssgrhiquadrenderer_p.h"
#include "qssglayerrenderdata_p.h"
#include "qssgrendercontextcore_p.h"
#include "qssgdebugdrawsystem_p.h"
#include "extensionapi/qssgrenderextensions_p.h"

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

    cb->debugMarkBegin(QByteArrayLiteral("Quick3D prepare Z prepass"));
    Q_TRACE_SCOPE(QSSG_renderPass, QStringLiteral("Quick3D prepare Z prepass"));
    Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DRenderPass);
    active = rhiPrepareDepthPass(rhiCtx.get(), this, ps, rhiCtx->mainRenderPassDescriptor(), data,
                                         renderedDepthWriteObjects, renderedOpaqueDepthPrepassObjects,
                                         rhiCtx->mainPassSampleCount());
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

void ZPrePassPass::release()
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
    camera = data.camera;
    QSSG_ASSERT_X((camera && rhiDepthTexture && rhiDepthTexture->isValid()), "Preparing AO pass failed, missing camera or required texture(s)", return);

    ssaoShaderPipeline = data.renderer->getRhiSsaoShader();
    ambientOcclusion = { data.layer.aoStrength, data.layer.aoDistance, data.layer.aoSoftness, data.layer.aoBias, data.layer.aoSamplerate, data.layer.aoDither };

    ps = data.getPipelineState();
    const auto &layerPrepResult = data.layerPrepResult;
    const bool ready = rhiAoTexture && rhiPrepareAoTexture(rhiCtx.get(), layerPrepResult.textureDimensions(), rhiAoTexture);

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
                           ambientOcclusion,
                           *rhiAoTexture,
                           *rhiDepthTexture,
                           *camera);
    }

    cb->debugMarkEnd();
    Q_QUICK3D_PROFILE_END_WITH_STRING(QQuick3DProfiler::Quick3DRenderPass, 0, QByteArrayLiteral("ssao_map"));
}

void SSAOMapPass::release()
{
    rhiDepthTexture = nullptr;
    rhiAoTexture = nullptr;
    camera = nullptr;
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
    rhiDepthTexture = data.getRenderResult(QSSGFrameData::RenderResult::DepthTexture);
    if (Q_LIKELY(rhiDepthTexture && rhiPrepareDepthTexture(rhiCtx.get(), layerPrepResult.textureDimensions(), rhiDepthTexture))) {
        sortedOpaqueObjects = data.getSortedOpaqueRenderableObjects();
        sortedTransparentObjects = data.getSortedTransparentRenderableObjects();
        ready = rhiPrepareDepthPass(rhiCtx.get(), this, ps, rhiDepthTexture->rpDesc, data,
                                    sortedOpaqueObjects, sortedTransparentObjects,
                                    1);
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

    // CONDITION:

    const auto &rhiCtx = renderer.contextInterface()->rhiContext();
    QSSG_ASSERT(rhiCtx->rhi()->isRecordingFrame(), return);
    QRhiCommandBuffer *cb = rhiCtx->commandBuffer();
    cb->debugMarkBegin(QByteArrayLiteral("Quick3D depth texture"));

    if (Q_LIKELY(rhiDepthTexture && rhiDepthTexture->isValid())) {
        bool needsSetViewport = true;
        cb->beginPass(rhiDepthTexture->rt, Qt::transparent, { 1.0f, 0 }, nullptr, QSSGRhiContext::commonPassFlags());
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

void DepthMapPass::release()
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

    const auto &rhiCtx = renderer.contextInterface()->rhiContext();
    QSSG_ASSERT(rhiCtx->rhi()->isRecordingFrame(), return);
    rhiScreenTexture = data.getRenderResult(QSSGFrameData::RenderResult::ScreenTexture);
    auto &layer = data.layer;
    const auto &layerPrepResult = data.layerPrepResult;
    wantsMips = layerPrepResult.flags.requiresMipmapsForScreenTexture();
    sortedOpaqueObjects = data.getSortedOpaqueRenderableObjects();
    ps = data.getPipelineState();

    if (layer.background == QSSGRenderLayer::Background::Color)
        clearColor = QColor::fromRgbF(layer.clearColor.x(), layer.clearColor.y(), layer.clearColor.z());

    if (rhiCtx->rhi()->isFeatureSupported(QRhi::TexelFetch)) {
        if (layer.background == QSSGRenderLayer::Background::SkyBoxCubeMap && layer.skyBoxCubeMap) {
            skyboxPass = &data.skyboxCubeMapPass;
            data.skyboxCubeMapPass.renderPrep(renderer, data);
        } else if (layer.background == QSSGRenderLayer::Background::SkyBox && layer.lightProbe) {
            skyboxPass = &data.skyboxPass;
            const bool tonemappingEnabled = data.skyboxPass.skipTonemapping;
            data.skyboxPass.skipTonemapping = true;
            data.skyboxPass.renderPrep(renderer, data);
            data.skyboxPass.skipTonemapping = tonemappingEnabled;
        }
    }

    bool ready = false;
    if (Q_LIKELY(rhiScreenTexture && rhiPrepareScreenTexture(rhiCtx.get(), layerPrepResult.textureDimensions(), wantsMips, rhiScreenTexture))) {
        ready = true;
        // NB: not compatible with disabling LayerEnableDepthTest
        // because there are effectively no "opaque" objects then.
        // Disable Tonemapping for all materials in the screen pass texture
        shaderFeatures = data.getShaderFeatures();
        shaderFeatures.disableTonemapping();
        const auto &sortedOpaqueObjects = data.getSortedOpaqueRenderableObjects();
        for (const auto &handle : sortedOpaqueObjects)
            rhiPrepareRenderable(rhiCtx.get(), this, data, *handle.obj, rhiScreenTexture->rpDesc, &ps, shaderFeatures, 1);
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
        cb->beginPass(rhiScreenTexture->rt, clearColor, { 1.0f, 0 }, nullptr, QSSGRhiContext::commonPassFlags());
        QSSGRHICTX_STAT(rhiCtx, beginRenderPass(rhiScreenTexture->rt));
        Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DRenderPass);

        bool needsSetViewport = true;
        for (const auto &handle : std::as_const(sortedOpaqueObjects))
            rhiRenderRenderable(rhiCtx.get(), ps, *handle.obj, &needsSetViewport);

        if (skyboxPass)
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

void ScreenMapPass::release()
{
    rhiScreenTexture = nullptr;
    if (skyboxPass) {
        // NOTE: The screen map pass is prepped and rendered before we render the skybox to the main render target,
        // i.e., we are not interfering with the skybox pass' state. Just make sure sure to leave the skybox pass
        // in a good state now that we're done with it.
        skyboxPass->release();
        skyboxPass = nullptr;
    }
    ps = {};
    wantsMips = false;
    clearColor = Qt::transparent;
    shaderFeatures = {};
    sortedOpaqueObjects.clear();
}

void ScreenReflectionPass::renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data)
{
    const auto &rhiCtx = renderer.contextInterface()->rhiContext();
    QSSG_ASSERT(rhiCtx->rhi()->isRecordingFrame(), return);
    rhiScreenTexture = data.getRenderResult(QSSGFrameData::RenderResult::ScreenTexture);
    QSSG_ASSERT_X(rhiScreenTexture && rhiScreenTexture->isValid(), "Invalid screen texture!", return);

    const auto &layer = data.layer;
    const auto shaderFeatures = data.getShaderFeatures();
    const bool layerEnableDepthTest = layer.layerFlags.testFlag(QSSGRenderLayer::LayerFlag::EnableDepthTest);

    QRhiRenderPassDescriptor *mainRpDesc = rhiCtx->mainRenderPassDescriptor();
    const int samples = rhiCtx->mainPassSampleCount();

    // NOTE: We're piggybacking on the screen map pass for now, but we could do better.
    ps = data.getPipelineState();
    ps.depthTestEnable = data.screenMapPass.ps.depthTestEnable;
    ps.depthWriteEnable = data.screenMapPass.ps.depthWriteEnable;
    sortedScreenTextureObjects = data.getSortedScreenTextureRenderableObjects();
    for (const auto &handle : std::as_const(sortedScreenTextureObjects)) {
        QSSGRenderableObject *theObject = handle.obj;
        const auto depthWriteMode = theObject->depthWriteMode;
        ps.blendEnable = theObject->renderableFlags.hasTransparency();
        ps.depthWriteEnable = !(depthWriteMode == QSSGDepthDrawMode::Never || depthWriteMode == QSSGDepthDrawMode::OpaquePrePass
                                 || data.isZPrePassActive() || !layerEnableDepthTest);
        RenderHelpers::rhiPrepareRenderable(rhiCtx.get(), this, data, *theObject, mainRpDesc, &ps, shaderFeatures, samples);
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

void ScreenReflectionPass::release()
{
    sortedScreenTextureObjects.clear();
    rhiScreenTexture = nullptr;
    ps = {};
}

void OpaquePass::renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data)
{
    const auto &rhiCtx = renderer.contextInterface()->rhiContext();
    QSSG_ASSERT(rhiCtx->rhi()->isRecordingFrame(), return);
    auto camera = data.camera;
    const auto &cameraData = data.cameraData;
    QSSG_ASSERT(camera && cameraData.has_value() , return);

    ps = data.getPipelineState();

    { // Frustum culling
        const auto &clippingFrustum = data.cameraData->clippingFrustum;
        const auto &opaqueObjects = data.getSortedOpaqueRenderableObjects();
        if (clippingFrustum.has_value())
            QSSGLayerRenderData::frustumCulling(clippingFrustum.value(), opaqueObjects, sortedOpaqueObjects);
        else
            sortedOpaqueObjects = opaqueObjects;
    }

    shaderFeatures = data.getShaderFeatures();

    const auto &layer = data.layer;
    const bool layerEnableDepthTest = layer.layerFlags.testFlag(QSSGRenderLayer::LayerFlag::EnableDepthTest);

    ps.depthFunc = QRhiGraphicsPipeline::LessOrEqual;
    ps.blendEnable = false;

    QRhiRenderPassDescriptor *mainRpDesc = rhiCtx->mainRenderPassDescriptor();
    const int samples = rhiCtx->mainPassSampleCount();

    // opaque objects (or, this list is empty when LayerEnableDepthTest is disabled)
    for (const auto &handle : std::as_const(sortedOpaqueObjects)) {
        QSSGRenderableObject *theObject = handle.obj;
        const auto depthWriteMode = theObject->depthWriteMode;
        ps.depthWriteEnable = !(depthWriteMode == QSSGDepthDrawMode::Never ||
                                depthWriteMode == QSSGDepthDrawMode::OpaquePrePass ||
                                data.isZPrePassActive() || !layerEnableDepthTest);
        RenderHelpers::rhiPrepareRenderable(rhiCtx.get(), this, data, *theObject, mainRpDesc, &ps, shaderFeatures, samples);
    }
}

void OpaquePass::renderPass(QSSGRenderer &renderer)
{
    const auto &rhiCtx = renderer.contextInterface()->rhiContext();
    QSSG_ASSERT(rhiCtx->rhi()->isRecordingFrame(), return);
    QRhiCommandBuffer *cb = rhiCtx->commandBuffer();

    bool needsSetViewport = true;

    cb->debugMarkBegin(QByteArrayLiteral("Quick3D render opaque"));
    Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DRenderPass);
    Q_TRACE(QSSG_renderPass_entry, QStringLiteral("Quick3D render opaque"));
    for (const auto &handle : std::as_const(sortedOpaqueObjects)) {
        QSSGRenderableObject *theObject = handle.obj;
        RenderHelpers::rhiRenderRenderable(rhiCtx.get(), ps, *theObject, &needsSetViewport);
    }
    cb->debugMarkEnd();
    Q_QUICK3D_PROFILE_END_WITH_STRING(QQuick3DProfiler::Quick3DRenderPass, 0, QByteArrayLiteral("opaque_pass"));
    Q_TRACE(QSSG_renderPass_exit);
}

void OpaquePass::release()
{
    sortedOpaqueObjects.clear();
    ps = {};
    shaderFeatures = {};
}

void TransparentPass::renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data)
{
    const auto &rhiCtx = renderer.contextInterface()->rhiContext();
    QSSG_ASSERT(rhiCtx->rhi()->isRecordingFrame(), return);
    auto camera = data.camera;
    const auto &cameraData = data.cameraData;
    QSSG_ASSERT(camera && cameraData.has_value(), return);

    QRhiRenderPassDescriptor *mainRpDesc = rhiCtx->mainRenderPassDescriptor();
    const int samples = rhiCtx->mainPassSampleCount();

    ps = data.getPipelineState();

    // transparent objects (or, without LayerEnableDepthTest, all objects)
    ps.blendEnable = true;
    ps.depthWriteEnable = false;

    shaderFeatures = data.getShaderFeatures();

    { // Frustum culling
        const auto &clippingFrustum = data.cameraData->clippingFrustum;
        const auto &transparentObject = data.getSortedTransparentRenderableObjects();
        if (clippingFrustum.has_value())
            QSSGLayerRenderData::frustumCulling(clippingFrustum.value(), transparentObject, sortedTransparentObjects);
        else
            sortedTransparentObjects = transparentObject;
    }

    const bool zPrePassActive = data.isZPrePassActive();
    for (const auto &handle : std::as_const(sortedTransparentObjects)) {
        QSSGRenderableObject *theObject = handle.obj;
        const auto depthWriteMode = theObject->depthWriteMode;
        ps.depthWriteEnable = (depthWriteMode == QSSGDepthDrawMode::Always && !zPrePassActive);
        if (!(theObject->renderableFlags.isCompletelyTransparent()))
            RenderHelpers::rhiPrepareRenderable(rhiCtx.get(), this, data, *theObject, mainRpDesc, &ps, shaderFeatures, samples);
    }
}

void TransparentPass::renderPass(QSSGRenderer &renderer)
{
    const auto &rhiCtx = renderer.contextInterface()->rhiContext();
    QSSG_ASSERT(rhiCtx->rhi()->isRecordingFrame(), return);
    QRhiCommandBuffer *cb = rhiCtx->commandBuffer();

    cb->debugMarkBegin(QByteArrayLiteral("Quick3D render alpha"));
    Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DRenderPass);
    Q_TRACE(QSSG_renderPass_entry, QStringLiteral("Quick3D render alpha"));
    // If scissorRect is set, Item2Ds will be drawn by a workaround of modifying
    // viewport, not using actual 3D scissor test.
    // It means non-opaque objects may be affected by this viewport setting.
    bool needsSetViewport = true;
    for (const auto &handle : std::as_const(sortedTransparentObjects)) {
        QSSGRenderableObject *theObject = handle.obj;
        if (!theObject->renderableFlags.isCompletelyTransparent())
            RenderHelpers::rhiRenderRenderable(rhiCtx.get(), ps, *theObject, &needsSetViewport);
    }
    cb->debugMarkEnd();
    Q_QUICK3D_PROFILE_END_WITH_STRING(QQuick3DProfiler::Quick3DRenderPass, 0, QByteArrayLiteral("transparent_pass"));
    Q_TRACE(QSSG_renderPass_exit);
}

void TransparentPass::release()
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
        auto camera = data.camera;
        QSSG_ASSERT(camera, return);
        layer = &data.layer;
        QSSG_ASSERT(layer, return);

        ps = data.getPipelineState();
        ps.polygonMode = QRhiGraphicsPipeline::Fill;

        // When there are effects, then it is up to the last pass of the
        // last effect to perform tonemapping, neither the skybox nor the
        // main render pass should alter the colors then.
        skipTonemapping = layer->firstEffect != nullptr;

        RenderHelpers::rhiPrepareSkyBox(rhiCtx.get(), this, *layer, *camera, renderer);
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
    QSSGRenderLayer::TonemapMode tonemapMode = skipTonemapping ? QSSGRenderLayer::TonemapMode::None : layer->tonemapMode;

    auto shaderPipeline = renderer.getRhiSkyBoxShader(tonemapMode, layer->skyBoxIsRgbe8);
    QSSG_CHECK(shaderPipeline);
    ps.shaderPipeline = shaderPipeline.get();
    QRhiRenderPassDescriptor *rpDesc = rhiCtx->mainRenderPassDescriptor();
    ps.samples = rhiCtx->mainPassSampleCount();
    renderer.rhiQuadRenderer()->recordRenderQuad(rhiCtx.get(), &ps, srb, rpDesc, { QSSGRhiQuadRenderer::DepthTest | QSSGRhiQuadRenderer::RenderBehind });
    Q_QUICK3D_PROFILE_END_WITH_STRING(QQuick3DProfiler::Quick3DRenderPass, 0, QByteArrayLiteral("skybox_map"));
}

void SkyboxPass::release()
{
    ps = {};
    layer = nullptr;
    skipPrep = false;
}

void SkyboxCubeMapPass::renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data)
{
    const auto &rhiCtx = renderer.contextInterface()->rhiContext();
    QSSG_ASSERT(rhiCtx->rhi()->isRecordingFrame(), return);
    auto camera = data.camera;
    QSSG_ASSERT(camera, return);
    layer = &data.layer;
    QSSG_ASSERT(layer, return);

    ps = data.getPipelineState();
    ps.polygonMode = QRhiGraphicsPipeline::Fill;

    RenderHelpers::rhiPrepareSkyBox(rhiCtx.get(), this, *layer, *camera, renderer);
}

void SkyboxCubeMapPass::renderPass(QSSGRenderer &renderer)
{
    const auto &rhiCtx = renderer.contextInterface()->rhiContext();
    QSSG_ASSERT(rhiCtx->rhi()->isRecordingFrame(), return);
    QSSG_ASSERT(layer, return);

    QRhiShaderResourceBindings *srb = layer->skyBoxSrb;
    QSSG_ASSERT(srb, return);

    Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DRenderPass);
    Q_TRACE_SCOPE(QSSG_renderPass, QStringLiteral("Quick3D render skybox"));
    auto shaderPipeline = renderer.getRhiSkyBoxCubeShader();
    QSSG_CHECK(shaderPipeline);
    ps.shaderPipeline = shaderPipeline.get();
    QRhiRenderPassDescriptor *rpDesc = rhiCtx->mainRenderPassDescriptor();
    renderer.rhiCubeRenderer()->recordRenderCube(rhiCtx.get(), &ps, srb, rpDesc, { QSSGRhiQuadRenderer::DepthTest | QSSGRhiQuadRenderer::RenderBehind });
    Q_QUICK3D_PROFILE_END_WITH_STRING(QQuick3DProfiler::Quick3DRenderPass, 0, QByteArrayLiteral("skybox_cube"));
}

void SkyboxCubeMapPass::release()
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
            QRect effScissor = layer.scissorRect & layerPrepResult.viewport.toRect();
            QMatrix4x4 correctionMat = correctMVPForScissor(layerPrepResult.viewport,
                                                            effScissor,
                                                            rhiCtx->rhi()->isYUpInNDC());
            item2D->m_renderer->setProjectionMatrix(correctionMat * item2D->MVP);
            item2D->m_renderer->setViewportRect(effScissor);
        } else {
            item2D->m_renderer->setProjectionMatrix(item2D->MVP);
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
        item2D->m_renderer->setRenderTarget({ renderTarget, item2D->m_rp, rhiCtx->commandBuffer() });
        delete oldRp;
        item2D->m_renderer->prepareSceneInline();
    }
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
    for (const auto &item : std::as_const(item2Ds)) {
        QSSGRenderItem2D *item2D = static_cast<QSSGRenderItem2D *>(item);
        if (item2D->m_renderer && item2D->m_renderer->currentRhi() == renderer.contextInterface()->rhi())
            item2D->m_renderer->renderSceneInline();
    }
    cb->debugMarkEnd();
    Q_QUICK3D_PROFILE_END_WITH_STRING(QQuick3DProfiler::Quick3DRenderPass, 0, QByteArrayLiteral("2D_sub_scene"));
}

void Item2DPass::release()
{
    item2Ds.clear();
    ps = {};
}

void InfiniteGridPass::renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data)
{
    const auto &rhiCtx = renderer.contextInterface()->rhiContext();
    QSSG_ASSERT(rhiCtx->rhi()->isRecordingFrame(), return);
    auto camera = data.camera;
    QSSG_ASSERT(camera, return);
    layer = &data.layer;
    QSSG_ASSERT(layer, return);

    ps = data.getPipelineState();
    ps.blendEnable = true;
    RenderHelpers::rhiPrepareGrid(rhiCtx.get(), this, *layer, *camera, renderer);
}

void InfiniteGridPass::renderPass(QSSGRenderer &renderer)
{
    const auto &rhiCtx = renderer.contextInterface()->rhiContext();
    QSSG_ASSERT(rhiCtx->rhi()->isRecordingFrame(), return);
    QRhiCommandBuffer *cb = rhiCtx->commandBuffer();

    cb->debugMarkBegin(QByteArrayLiteral("Quick3D render grid"));
    Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DRenderPass);
    Q_TRACE_SCOPE(QSSG_renderPass, QStringLiteral("Quick3D render grid"));
    const auto &shaderPipeline = renderer.getRhiGridShader();
    Q_ASSERT(shaderPipeline);
    ps.shaderPipeline = shaderPipeline.get();
    QRhiShaderResourceBindings *srb = layer->gridSrb;
    QRhiRenderPassDescriptor *rpDesc = rhiCtx->mainRenderPassDescriptor();
    renderer.rhiQuadRenderer()->recordRenderQuad(rhiCtx.get(), &ps, srb, rpDesc, { QSSGRhiQuadRenderer::DepthTest });
    Q_QUICK3D_PROFILE_END_WITH_STRING(QQuick3DProfiler::Quick3DRenderPass, 0, QByteArrayLiteral("render_grid"));
}

void InfiniteGridPass::release()
{
    ps = {};
    layer = nullptr;
}

void DebugDrawPass::renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data)
{
    const auto &rhiCtx = renderer.contextInterface()->rhiContext();
    QSSG_ASSERT(rhiCtx->rhi()->isRecordingFrame(), return);
    auto camera = data.camera;
    QSSG_ASSERT(camera, return);

    ps = data.getPipelineState();

    // debug objects
    const auto &debugDraw = renderer.contextInterface()->debugDrawSystem();
    if (debugDraw && debugDraw->hasContent()) {
        QRhiResourceUpdateBatch *rub = rhiCtx->rhi()->nextResourceUpdateBatch();
        debugDraw->prepareGeometry(rhiCtx.get(), rub);
        QSSGRhiDrawCallData &dcd = rhiCtx->drawCallData({ this, nullptr, nullptr, 0 });
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
}

void DebugDrawPass::renderPass(QSSGRenderer &renderer)
{
    const auto &rhiCtx = renderer.contextInterface()->rhiContext();
    QSSG_ASSERT(rhiCtx->rhi()->isRecordingFrame(), return);
    QRhiCommandBuffer *cb = rhiCtx->commandBuffer();

    const auto &debugDraw = renderer.contextInterface()->debugDrawSystem();
    if (debugDraw && debugDraw->hasContent()) {
        cb->debugMarkBegin(QByteArrayLiteral("Quick 3D debug objects"));
        Q_TRACE_SCOPE(QSSG_renderPass, QStringLiteral("Quick 3D debug objects"));
        Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DRenderPass);
        auto shaderPipeline = renderer.getRhiDebugObjectShader();
        QSSG_CHECK(shaderPipeline);
        ps.shaderPipeline = shaderPipeline.get();
        QSSGRhiDrawCallData &dcd = rhiCtx->drawCallData({ this, nullptr, nullptr, 0 });
        QRhiShaderResourceBindings *srb = dcd.srb;
        QRhiRenderPassDescriptor *rpDesc = rhiCtx->mainRenderPassDescriptor();
        debugDraw->recordRenderDebugObjects(rhiCtx.get(), &ps, srb, rpDesc);
        cb->debugMarkEnd();
        Q_QUICK3D_PROFILE_END_WITH_STRING(QQuick3DProfiler::Quick3DRenderPass, 0, QByteArrayLiteral("debug_objects"));
    }
}

void DebugDrawPass::release()
{
    ps = {};
}

void UserPass::renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data)
{
    auto &frameData = data.getFrameData();
    for (const auto &p : std::as_const(extensions)) {
        p->prepareRender(renderer, frameData);
        if (p->type() == QSSGRenderExtension::Type::Standalone)
            p->render(renderer);
    }
}

void UserPass::renderPass(QSSGRenderer &renderer)
{
    for (const auto &p : std::as_const(extensions)) {
        if (p->type() == QSSGRenderExtension::Type::Main)
            p->render(renderer);
    }
}

void UserPass::release()
{
    for (const auto &p : std::as_const(extensions))
        p->release();

    // TODO: We should track if we need to update this list.
    extensions.clear();
}

QT_END_NAMESPACE
