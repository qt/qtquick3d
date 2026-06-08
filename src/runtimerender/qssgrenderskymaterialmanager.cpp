// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include <QtQuick3DRuntimeRender/private/qssgrenderskymaterialmanager_p.h>

#include <QtQuick3DRuntimeRender/private/qssgrenderskymaterial_p.h>
#include <QtQuick3DRuntimeRender/private/qssglayerrenderdata_p.h>
#include <QtQuick3DUtils/private/qssgassert_p.h>

#include "qssgrendercontextcore.h"
#include "qssgrendershadercache_p.h"
#include "qssgrhicontext_p.h"

#include <QtCore/qmath.h>

#include <cmath>

using namespace Qt::StringLiterals;

QT_BEGIN_NAMESPACE

namespace {

static constexpr QRhiTexture::Format cTextureFormat = QRhiTexture::RGBA16F;

static int nearestPowerOfTwo(int v)
{
    if (v <= 1)
        return 1;
    int upper = qNextPowerOfTwo(v);
    int lower = upper >> 1;
    return (v - lower < upper - v) ? lower : upper;
}

static QVarLengthArray<QMatrix4x4, 6> skyIblEnvironmentMapViews(QRhi *rhi)
{
    auto lookAt = [](const QVector3D &eye, const QVector3D &center, const QVector3D &up) {
        QMatrix4x4 viewMatrix;
        viewMatrix.lookAt(eye, center, up);
        return viewMatrix;
    };

    QVarLengthArray<QMatrix4x4, 6> views;
    views.append(lookAt(QVector3D(0.0f, 0.0f, 0.0f), QVector3D(1.0, 0.0, 0.0), QVector3D(0.0f, -1.0f, 0.0f)));
    views.append(lookAt(QVector3D(0.0f, 0.0f, 0.0f), QVector3D(-1.0, 0.0, 0.0), QVector3D(0.0f, -1.0f, 0.0f)));
    if (rhi->isYUpInFramebuffer()) {
        views.append(lookAt(QVector3D(0.0f, 0.0f, 0.0f), QVector3D(0.0, 1.0, 0.0), QVector3D(0.0f, 0.0f, 1.0f)));
        views.append(lookAt(QVector3D(0.0f, 0.0f, 0.0f), QVector3D(0.0, -1.0, 0.0), QVector3D(0.0f, 0.0f, -1.0f)));
    } else {
        views.append(lookAt(QVector3D(0.0f, 0.0f, 0.0f), QVector3D(0.0, -1.0, 0.0), QVector3D(0.0f, 0.0f, -1.0f)));
        views.append(lookAt(QVector3D(0.0f, 0.0f, 0.0f), QVector3D(0.0, 1.0, 0.0), QVector3D(0.0f, 0.0f, 1.0f)));
    }
    views.append(lookAt(QVector3D(0.0f, 0.0f, 0.0f), QVector3D(0.0, 0.0, 1.0), QVector3D(0.0f, -1.0f, 0.0f)));
    views.append(lookAt(QVector3D(0.0f, 0.0f, 0.0f), QVector3D(0.0, 0.0, -1.0), QVector3D(0.0f, -1.0f, 0.0f)));
    return views;
}

// Creates 6 face render targets for a texture, always at mip level 0.
// preserveColorContents controls whether the render target loads existing
// content before drawing (additive accumulation) or clears it (first slice).
// Using level=0 unconditionally is intentional: on Android GLES some drivers
// silently ignore the level parameter to glFramebufferTextureLayer for level>0,
// so the accumulator is allocated as separate per-mip non-mipmapped textures
// and each is always attached at level 0.
static bool skyIblCreateFaceTargets(QRhi *rhi,
                                    QRhiTexture *texture,
                                    const QByteArray &namePrefix,
                                    QSSGSkyIblFaceTargets *outTargets,
                                    bool preserveColorContents = false)
{
    Q_ASSERT(outTargets);
    const QRhiTextureRenderTarget::Flags rtFlags = preserveColorContents
            ? QRhiTextureRenderTarget::Flags(QRhiTextureRenderTarget::PreserveColorContents)
            : QRhiTextureRenderTarget::Flags();
    for (const auto face : QSSGRenderTextureCubeFaces) {
        QRhiColorAttachment att(texture);
        att.setLayer(quint8(face));
        // No setLevel() call — always attaches at mip level 0. Each per-mip
        // accumulator texture has only one mip level, so this is correct.
        QRhiTextureRenderTargetDescription rtDesc;
        rtDesc.setColorAttachments({ att });
        auto renderTarget = rhi->newTextureRenderTarget(rtDesc, rtFlags);
        renderTarget->setName(namePrefix + "/"_ba + QSSGBaseTypeHelpers::displayName(face));
        renderTarget->setDescription(rtDesc);
        if (!outTargets->renderPassDesc)
            outTargets->renderPassDesc = renderTarget->newCompatibleRenderPassDescriptor();
        renderTarget->setRenderPassDescriptor(outTargets->renderPassDesc);
        if (!renderTarget->create()) {
            qWarning("Failed to build sky IBL env map render target");
            return false;
        }
        outTargets->renderTargets << renderTarget;
    }
    return true;
}

static bool skyIblCreatePrefilterTargets(QRhi *rhi,
                                         QRhiTexture *texture,
                                         const QSize &environmentMapSize,
                                         const QByteArray &namePrefix,
                                         QSSGSkyIblPrefilterTargets *outTargets,
                                         bool preserveColorContents = false)
{
    Q_ASSERT(outTargets);
    const bool hasMips = texture->flags().testFlag(QRhiTexture::MipMapped);
    outTargets->mipmapCount = hasMips ? qMin(rhi->mipLevelsForSize(environmentMapSize), 6) : 1;
    outTargets->mipLevelSizes.resize(outTargets->mipmapCount);
    outTargets->mipRenderTargetsMap.resize(outTargets->mipmapCount);

    const QRhiTextureRenderTarget::Flags rtFlags = preserveColorContents
            ? QRhiTextureRenderTarget::Flags(QRhiTextureRenderTarget::PreserveColorContents)
            : QRhiTextureRenderTarget::Flags();

    auto cleanup = [outTargets](QRhiTextureRenderTarget *failed, const QSSGSkyIblCubeFaceRenderTargets &partial) {
        delete failed;
        for (auto *rt : partial)
            delete rt;
        for (const QSSGSkyIblCubeFaceRenderTargets &rts : std::as_const(outTargets->mipRenderTargetsMap)) {
            for (auto *rt : rts)
                delete rt;
        }
        delete outTargets->renderPassDesc;
        outTargets->renderPassDesc = nullptr;
        outTargets->mipRenderTargetsMap.clear();
        outTargets->mipLevelSizes.clear();
        outTargets->mipmapCount = 0;
    };

    for (int mipLevel = 0; mipLevel < outTargets->mipmapCount; ++mipLevel) {
        QSSGSkyIblCubeFaceRenderTargets renderTargets;
        for (const auto face : QSSGRenderTextureCubeFaces) {
            QRhiColorAttachment att(texture);
            att.setLayer(quint8(face));
            att.setLevel(mipLevel);
            QRhiTextureRenderTargetDescription rtDesc;
            rtDesc.setColorAttachments({ att });
            auto renderTarget = rhi->newTextureRenderTarget(rtDesc, rtFlags);
            renderTarget->setName(namePrefix + QByteArrayLiteral("/m") + QByteArray::number(mipLevel)
                                  + QByteArrayLiteral("/") + QSSGBaseTypeHelpers::displayName(face));
            renderTarget->setDescription(rtDesc);
            if (!outTargets->renderPassDesc)
                outTargets->renderPassDesc = renderTarget->newCompatibleRenderPassDescriptor();
            renderTarget->setRenderPassDescriptor(outTargets->renderPassDesc);
            if (!renderTarget->create()) {
                qWarning("Failed to build sky IBL prefilter env map render target");
                cleanup(renderTarget, renderTargets);
                return false;
            }
            renderTargets << renderTarget;
        }
        const QSize levelSize(environmentMapSize.width() * std::pow(0.5, mipLevel),
                              environmentMapSize.height() * std::pow(0.5, mipLevel));
        outTargets->mipLevelSizes[mipLevel] = levelSize;
        outTargets->mipRenderTargetsMap[mipLevel] = renderTargets;
    }
    return true;
}

static void skyIblInitializeUnrenderedMips(QRhi *rhi,
                                           QRhiCommandBuffer *cb,
                                           QSSGRhiContext *context,
                                           QRhiTexture *texture,
                                           int firstMip,
                                           int mipCountExclusive,
                                           const QByteArray &debugObjectName)
{
    if (firstMip >= mipCountExclusive)
        return;
    QRhiRenderPassDescriptor *rpDesc = nullptr;
    for (int mipLevel = firstMip; mipLevel < mipCountExclusive; ++mipLevel) {
        for (const auto face : QSSGRenderTextureCubeFaces) {
            QRhiColorAttachment att(texture);
            att.setLayer(quint8(face));
            att.setLevel(mipLevel);
            QRhiTextureRenderTargetDescription rtDesc;
            rtDesc.setColorAttachments({ att });
            auto *rt = rhi->newTextureRenderTarget(rtDesc);
            rt->setName(debugObjectName + "/init/m"_ba + QByteArray::number(mipLevel) + "/"_ba
                        + QSSGBaseTypeHelpers::displayName(face));
            rt->setDescription(rtDesc);
            if (!rpDesc)
                rpDesc = rt->newCompatibleRenderPassDescriptor();
            rt->setRenderPassDescriptor(rpDesc);
            if (!rt->create()) {
                qWarning("Failed to create sky IBL init render target");
                delete rt;
                continue;
            }
            rt->deleteLater();
            cb->beginPass(rt, QColor(0, 0, 0, 1), { 1.0f, 0 }, nullptr, context->commonPassFlags());
            cb->endPass();
        }
    }
    if (rpDesc)
        rpDesc->deleteLater();
}

static const float skyIblCubeVerts[] = {
    -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,
    -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, -1.0f,
    -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f,

    -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f,
    1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,

    -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f,
    1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f,

    1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,
    1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f,

    -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,
    1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,

    0.0f,  1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  1.0f,

    1.0f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  0.0f,

    1.0f,  0.0f,  1.0f,  1.0f,  0.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

    1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,

    1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,  1.0f,  1.0f,  0.0f,

    0.0f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,  1.0f,  0.0f,
};

static bool ensureDynamicUBuf(QRhi *rhi, QRhiBuffer *&dst, int size, const char *errorMessage)
{
    if (dst)
        return true;
    dst = rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, size);
    if (!dst->create()) {
        qWarning("%s", errorMessage);
        delete dst;
        dst = nullptr;
        return false;
    }
    return true;
}

static bool ensureSrb(QRhi *rhi, QRhiShaderResourceBindings *&dst, std::initializer_list<QRhiShaderResourceBinding> bindings, const char *errorMessage)
{
    if (dst)
        return true;
    dst = rhi->newShaderResourceBindings();
    dst->setBindings(bindings);
    if (!dst->create()) {
        qWarning("%s", errorMessage);
        delete dst;
        dst = nullptr;
        return false;
    }
    return true;
}

struct PrefilterPipelineConfig
{
    QSSGRhiShaderPipeline *shader = nullptr;
    QRhiShaderResourceBindings *srb = nullptr;
    QRhiRenderPassDescriptor *rpd = nullptr;
    bool additiveBlend = false;
};

static QRhiGraphicsPipeline *createPrefilterPipeline(QRhi *rhi,
                                                     const QRhiVertexInputLayout &inputLayout,
                                                     const PrefilterPipelineConfig &cfg,
                                                     const char *errorMessage)
{
    auto *pipeline = rhi->newGraphicsPipeline();
    pipeline->setCullMode(QRhiGraphicsPipeline::Front);
    pipeline->setFrontFace(QRhiGraphicsPipeline::CCW);
    pipeline->setDepthOp(QRhiGraphicsPipeline::LessOrEqual);
    pipeline->setShaderStages({ *cfg.shader->vertexStage(), *cfg.shader->fragmentStage() });
    pipeline->setVertexInputLayout(inputLayout);
    pipeline->setShaderResourceBindings(cfg.srb);
    pipeline->setRenderPassDescriptor(cfg.rpd);
    pipeline->setFlags(QRhiGraphicsPipeline::UsesScissor);
    if (cfg.additiveBlend) {
        QRhiGraphicsPipeline::TargetBlend addBlend;
        addBlend.enable = true;
        addBlend.srcColor = QRhiGraphicsPipeline::One;
        addBlend.dstColor = QRhiGraphicsPipeline::One;
        addBlend.opColor = QRhiGraphicsPipeline::Add;
        addBlend.srcAlpha = QRhiGraphicsPipeline::One;
        addBlend.dstAlpha = QRhiGraphicsPipeline::One;
        addBlend.opAlpha = QRhiGraphicsPipeline::Add;
        pipeline->setTargetBlends({ addBlend });
    }
    if (!pipeline->create()) {
        qWarning("%s", errorMessage);
        delete pipeline;
        return nullptr;
    }
    return pipeline;
}

static void drawCubeFace(QRhiCommandBuffer *cb,
                         QSSGRhiContext *ctx,
                         QRhiTextureRenderTarget *rt,
                         QSize viewport,
                         QRhiGraphicsPipeline *pipeline,
                         QRhiShaderResourceBindings *srb,
                         const QRhiCommandBuffer::VertexInput &vbufBinding,
                         const QVector<QPair<int, quint32>> &dynamicOffsets,
                         const QByteArray &profilerLabel,
                         const QByteArray &passDebugLabel,
                         const QColor &clearColor = QColor(0, 0, 0, 1))
{
    cb->beginPass(rt, clearColor, { 1.0f, 0 }, nullptr, ctx->commonPassFlags());
    QSSGRHICTX_STAT(ctx, beginRenderPass(rt));
    Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DRenderPass);
    cb->setViewport(QRhiViewport(0, 0, viewport.width(), viewport.height()));
    cb->setScissor(QRhiScissor(0, 0, viewport.width(), viewport.height()));
    cb->setGraphicsPipeline(pipeline);
    cb->setShaderResources(srb, dynamicOffsets.size(), dynamicOffsets.constData());
    cb->setVertexInput(0, 1, &vbufBinding);
    Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DRenderCall);
    cb->draw(36);
    QSSGRHICTX_STAT(ctx, draw(36, 1));
    Q_QUICK3D_PROFILE_END_WITH_STRING(QQuick3DProfiler::Quick3DRenderCall, 36llu | (1llu << 32), profilerLabel);
    cb->endPass();
    QSSGRHICTX_STAT(ctx, endRenderPass());
    Q_QUICK3D_PROFILE_END_WITH_STRING(QQuick3DProfiler::Quick3DRenderPass, 0, passDebugLabel);
}

} // namespace

// Per-frame derived state passed between phase methods
struct QSSGRenderSkyMaterialManager::FrameState
{
    // computeFrameState
    QSize environmentMapSize;
    int totalSamples = 0;
    bool enableIBL = false;
    bool needCreateEnv = false;
    bool inProgressTimeSlice = false;
    bool envContentDirty = false;
    bool deferEnvRefresh = false;
    bool needRenderEnv = false;

    // ensureTextures
    bool prefilteredJustCreated = false;
    int prefilterTotalMipCount = 0;
    int prefilterSpecularMipCount = 0;
    int prefilterRoughnessDenom = 1;
    float resolution = 0.0f;

    // deriveCycleState
    bool multiFrame = false;
    bool prefilterIsConverged = false;
    bool runPrefilterSlice = false;
    int perFrameBudget = 0;
    int sliceSamplesThisFrame = 0;
    int sliceSampleStart = 0;
    int sliceSampleEnd = 0;
    bool sliceCompletesCycle = false;
    bool writePrefilteredCubeThisFrame = false;
    bool runIrradiancePass = false;
    bool haveConvergedResultEntering = false;
    bool isFirstSlice = false;

    // ensureSharedResources
    QRhiVertexInputLayout inputLayout;
    QMatrix4x4 mvp;
    QVarLengthArray<QMatrix4x4, 6> views;
    int ubufElementSize = 0;
    QRhiCommandBuffer::VertexInput vbufBinding { nullptr, 0 };
};

QSSGRenderSkyMaterialManager::QSSGRenderSkyMaterialManager(const QSSGRenderContextInterface &inContext)
    : m_context(inContext)
{
}

QSSGRenderSkyMaterialManager::~QSSGRenderSkyMaterialManager()
{
    releaseCachedResources();
}

void QSSGRenderSkyMaterialManager::clearPrefilterCache()
{
    auto releasePrefilterTargets = [](QSSGSkyIblPrefilterTargets &t) {
        for (const QSSGSkyIblCubeFaceRenderTargets &renderTargets : std::as_const(t.mipRenderTargetsMap)) {
            for (QRhiTextureRenderTarget *renderTarget : renderTargets)
                delete renderTarget;
        }
        delete t.renderPassDesc;
        t.renderPassDesc = nullptr;
        t.mipRenderTargetsMap.clear();
        t.mipLevelSizes.clear();
        t.mipmapCount = 0;
    };

    auto releaseFaceTargets = [](QSSGSkyIblFaceTargets &t) {
        for (QRhiTextureRenderTarget *rt : t.renderTargets)
            delete rt;
        delete t.renderPassDesc;
        t.renderPassDesc = nullptr;
        t.renderTargets.clear();
    };

    auto safeDelete = [](auto *&res) {
        delete res;
        res = nullptr;
    };

    safeDelete(m_cache.vertexBuffer);
    safeDelete(m_cache.uBuf);
    safeDelete(m_cache.uBufSlice);
    safeDelete(m_cache.uBufNormalize);
    safeDelete(m_cache.uBufIrradiance);

    releaseFaceTargets(m_cache.envFaceTargets);
    releasePrefilterTargets(m_cache.prefilterTargets);

    // Release per-mip accumulator face targets (preserve and clear variants)
    for (QSSGSkyIblFaceTargets &t : m_cache.accumPreserveFaceTargets)
        releaseFaceTargets(t);
    m_cache.accumPreserveFaceTargets.clear();

    for (QSSGSkyIblFaceTargets &t : m_cache.accumClearFaceTargets)
        releaseFaceTargets(t);
    m_cache.accumClearFaceTargets.clear();

    // Release per-mip normalize SRBs
    for (QRhiShaderResourceBindings *srb : std::as_const(m_cache.normalizeSrbs))
        delete srb;
    m_cache.normalizeSrbs.clear();

    safeDelete(m_cache.sliceSrb);
    safeDelete(m_cache.irradianceSrb);

    safeDelete(m_cache.envMapPipeline);
    safeDelete(m_cache.slicePipeline);
    safeDelete(m_cache.normalizeCubePipeline);
    safeDelete(m_cache.irradiancePipeline);

    m_cache.environmentMapSize = { };
    m_cache.enableIBL = false;
    m_cache.prefilterTotalMipCount = 0;
    m_cache.envCubeMap = nullptr;
    m_cache.prefilteredCubeMap = nullptr;
    m_cache.prefilterAccumulators.clear();
    m_cache.envShaderPipeline = nullptr;
}

void QSSGRenderSkyMaterialManager::releaseCachedResources()
{
    clearPrefilterCache();
    if (auto rhiCtx = m_context.rhiContext().get(); QSSG_GUARD(rhiCtx && rhiCtx->isValid())) {
        QSSGRhiContextPrivate *rhiCtxD = QSSGRhiContextPrivate::get(rhiCtx);
        if (m_envCubeMap)
            rhiCtxD->releaseTexture(m_envCubeMap);
        if (m_prefilteredCubeMap)
            rhiCtxD->releaseTexture(m_prefilteredCubeMap);
        for (QRhiTexture *t : std::as_const(m_prefilterAccumulators))
            rhiCtxD->releaseTexture(t);
    }
    m_envCubeMap = nullptr;
    m_prefilteredCubeMap = nullptr;
    m_prefilterAccumulators.clear();
    m_skyIblTexture = { };
    m_cubeMapSize = { };
    m_prefilteredMipCount = 0;
    m_accumulatedSamples = 0;
    m_accumIblSampleCount = 0;
    m_haveConvergedResult = false;
    m_envTailMipsInitialized = false;
    m_prefilteredTailMipsInitialized = false;
    m_finalizeIblPending = false;
}

QSSGRenderImageTexture QSSGRenderSkyMaterialManager::resolve(QSSGRenderSkyMaterial *settings)
{
    const auto &rhiCtx = m_context.rhiContext();
    if (!QSSG_GUARD(rhiCtx && rhiCtx->isValid() && rhiCtx->rhi()->isRecordingFrame()))
        return { };

    if (!ensureEnvironmentMap(settings)) {
        return { };
    }

    const bool pendingAccumulation = settings->enableIBL && m_accumulatedSamples < settings->iblSampleCount;
    settings->wantsMoreFrames = pendingAccumulation || settings->isDirty || m_finalizeIblPending;

    return m_skyIblTexture;
}

bool QSSGRenderSkyMaterialManager::ensureEnvironmentMap(QSSGRenderSkyMaterial *inSky)
{
    const auto &context = m_context.rhiContext();
    if (!context->rhi()->isTextureFormatSupported(cTextureFormat)) {
        static bool warningPrinted = false;
        if (Q_UNLIKELY(!warningPrinted)) {
            qWarning() << "SkyMaterial not supported due to missing RGBA16F texture format support.";
            warningPrinted = true;
        }
        return false;
    }

    auto *cb = context->commandBuffer();

    FrameState fs;
    if (!computeFrameState(inSky, fs))
        return false;

    if (!ensureTextures(fs))
        return false;

    deriveCycleState(inSky, fs);

    QSSGRhiShaderPipelinePtr shaderPipeline;
    if (fs.needRenderEnv) {
        shaderPipeline = inSky->ensurePipeline(m_context);
        if (!shaderPipeline) {
            return false;
        }
    }
    QSSGRhiShaderPipeline *envShaderPipelineKey = fs.needRenderEnv ? shaderPipeline.get() : m_cache.envShaderPipeline;

    validateAndUpdateCacheKey(fs, envShaderPipelineKey);

    if (!ensureSharedResources(fs, cb))
        return false;

    auto *rub = context->rhi()->nextResourceUpdateBatch();
    for (const auto face : QSSGRenderTextureCubeFaces) {
        rub->updateDynamicBuffer(m_cache.uBuf, quint8(face) * fs.ubufElementSize, 64, fs.mvp.constData());
        rub->updateDynamicBuffer(m_cache.uBuf, quint8(face) * fs.ubufElementSize + 64, 64, fs.views[quint8(face)].constData());
    }

    if (fs.runIrradiancePass) {
        struct IrradianceData
        {
            float roughness;
            float resolution;
            float lodBias;
            int sampleCount;
            int distribution;
        } irradianceData;
        irradianceData.roughness = 0.0f;
        irradianceData.resolution = fs.resolution;
        irradianceData.lodBias = 0.0f;
        irradianceData.distribution = 0;
        irradianceData.sampleCount = qMax(int(fs.resolution / 4.0f), 1);
        rub->updateDynamicBuffer(m_cache.uBufIrradiance, 0, sizeof(IrradianceData), &irradianceData);
    }

    if (fs.needRenderEnv) {
        if (!renderEnvironmentCube(inSky, fs, shaderPipeline, cb, rub))
            return false;
        rub = nullptr;
    }

    if (fs.enableIBL && fs.needRenderEnv)
        runEnvironmentMipChain(fs, cb);

    if (!rub)
        rub = context->rhi()->nextResourceUpdateBatch();

    cb->debugMarkBegin("Sky IBL Pre-filtered Environment Cubemap Generation");
    if (!runPrefilterCycle(inSky, fs, cb, rub))
        return false;
    if (rub) {
        cb->resourceUpdate(rub);
        rub = nullptr;
    }
    cb->debugMarkEnd();

    m_prefilteredMipCount = m_cache.prefilterTargets.mipmapCount;
    initializeTailMips(fs, cb);

    m_skyIblTexture.m_texture = m_prefilteredCubeMap;
    m_skyIblTexture.m_mipmapCount = m_prefilteredMipCount;
    m_skyIblTexture.m_flags.setLinear(true);
    m_skyIblTexture.m_flags.setRgbe8(false);
    return true;
}

bool QSSGRenderSkyMaterialManager::computeFrameState(QSSGRenderSkyMaterial *inSky, FrameState &fs)
{
    fs.enableIBL = inSky->enableIBL;
    fs.totalSamples = qBound(1, inSky->iblSampleCount, 1024);

    const int radianceMapSize = qBound(8, nearestPowerOfTwo(inSky->radianceMapSize), 2048);
    fs.environmentMapSize = QSize(radianceMapSize, radianceMapSize);

    const bool envWantsMips = fs.enableIBL;
    const bool envHasMips = m_envCubeMap && m_envCubeMap->flags().testFlag(QRhiTexture::MipMapped);
    fs.needCreateEnv = !m_envCubeMap || m_cubeMapSize != fs.environmentMapSize || envHasMips != envWantsMips;

    fs.inProgressTimeSlice = fs.enableIBL && m_accumulatedSamples > 0 && m_accumulatedSamples < m_accumIblSampleCount;
    fs.envContentDirty = inSky->isDirty && !fs.needCreateEnv;
    fs.deferEnvRefresh = fs.envContentDirty && (fs.inProgressTimeSlice || m_finalizeIblPending);
    fs.needRenderEnv = fs.needCreateEnv || (fs.envContentDirty && !fs.deferEnvRefresh);
    return true;
}

bool QSSGRenderSkyMaterialManager::ensureTextures(FrameState &fs)
{
    const auto &context = m_context.rhiContext();
    auto *rhi = context->rhi();
    QSSGRhiContextPrivate *rhiCtxD = QSSGRhiContextPrivate::get(context.get());

    // --- Environment cube ---
    if (fs.needCreateEnv) {
        if (m_envCubeMap) {
            rhiCtxD->releaseTexture(m_envCubeMap);
            m_envCubeMap = nullptr;
        }
        // The prefiltered cube derives from m_envCubeMap and must be invalidated alongside.
        if (m_prefilteredCubeMap) {
            rhiCtxD->releaseTexture(m_prefilteredCubeMap);
            m_prefilteredCubeMap = nullptr;
            m_prefilteredMipCount = 0;
        }
        m_envTailMipsInitialized = false;
        m_prefilteredTailMipsInitialized = false;

        QRhiTexture::Flags envFlags = QRhiTexture::RenderTarget | QRhiTexture::CubeMap;
        if (fs.enableIBL)
            envFlags |= QRhiTexture::MipMapped | QRhiTexture::UsedWithGenerateMips;
        m_envCubeMap = rhi->newTexture(cTextureFormat, fs.environmentMapSize, 1, envFlags);
        if (!m_envCubeMap->create()) {
            qWarning("Failed to create Sky IBL environment cube map");
            delete m_envCubeMap;
            m_envCubeMap = nullptr;
            return false;
        }
        m_envCubeMap->setName("SkyMaterialLightProbe procEnvCube"_ba);
        rhiCtxD->registerTexture(m_envCubeMap);
        m_cubeMapSize = fs.environmentMapSize;
    }

    if (!m_prefilteredCubeMap) {
        const QRhiTexture::Flags pfFlags = QRhiTexture::RenderTarget | QRhiTexture::CubeMap | QRhiTexture::MipMapped;
        m_prefilteredCubeMap = rhi->newTexture(cTextureFormat, fs.environmentMapSize, 1, pfFlags);
        if (!m_prefilteredCubeMap->create()) {
            qWarning("Failed to create Sky IBL pre-filtered environment cube map");
            delete m_prefilteredCubeMap;
            m_prefilteredCubeMap = nullptr;
            return false;
        }
        m_prefilteredCubeMap->setName("SkyMaterialLightProbe"_ba);
        rhiCtxD->registerTexture(m_prefilteredCubeMap);
        fs.prefilteredJustCreated = true;
        m_haveConvergedResult = false;
        m_prefilteredTailMipsInitialized = false;
    }

    fs.prefilterTotalMipCount = m_prefilteredCubeMap->flags().testFlag(QRhiTexture::MipMapped)
            ? qMin(rhi->mipLevelsForSize(fs.environmentMapSize), 6)
            : 1;
    fs.prefilterSpecularMipCount = fs.enableIBL ? fs.prefilterTotalMipCount - 1 : 1;
    fs.prefilterRoughnessDenom = qMax(fs.prefilterSpecularMipCount - 1, 1);
    fs.resolution = float(fs.environmentMapSize.width());

    // Determine whether the existing per-mip accumulator set is still valid.
    // We need one texture per specular mip level, each sized for that mip.
    const bool needCreateAccum = m_prefilterAccumulators.isEmpty() || m_prefilterAccumulators.size() != fs.prefilterSpecularMipCount
            || (!m_prefilterAccumulators.isEmpty() && m_prefilterAccumulators[0]->pixelSize() != fs.environmentMapSize);

    if (needCreateAccum) {
        for (QRhiTexture *t : std::as_const(m_prefilterAccumulators))
            rhiCtxD->releaseTexture(t);
        m_prefilterAccumulators.clear();
        m_accumulatedSamples = 0;

        // Allocate one non-mipmapped 2D array texture per specular mip level.
        // Each texture is sized for its mip level and only has mip 0.
        //
        // Rationale: on Android GLES (Adreno, Mali) glFramebufferTextureLayer
        // with level > 0 is sometimes silently broken. The driver accepts the
        // FBO as complete but writes to the wrong location or ignores the level
        // entirely. By giving each mip level its own texture and always attaching
        // at level 0, we avoid the broken code path entirely.
        bool ok = true;
        for (int mip = 0; mip < fs.prefilterSpecularMipCount; ++mip) {
            const QSize mipSize(qMax(1, fs.environmentMapSize.width() >> mip), qMax(1, fs.environmentMapSize.height() >> mip));
            // No MipMapped flag — single level only, always attached at level 0.
            auto *t = rhi->newTextureArray(cTextureFormat, 6, mipSize, 1, QRhiTexture::RenderTarget);
            if (!t->create()) {
                qWarning("Failed to create Sky IBL prefilter accumulator mip %d", mip);
                delete t;
                ok = false;
                break;
            }
            t->setName("SkyMaterialLightProbe procEnvPfAccum m"_ba + QByteArray::number(mip));
            rhiCtxD->registerTexture(t);
            m_prefilterAccumulators.append(t);
        }

        if (!ok) {
            for (QRhiTexture *t : std::as_const(m_prefilterAccumulators))
                rhiCtxD->releaseTexture(t);
            m_prefilterAccumulators.clear();
            // Accumulator unavailable — fall back to single-frame prefilter.
        } else {
            // No tail-mip initialization needed: each texture has exactly one
            // mip level (level 0), so there are no unwritten tail mips.
        }
    }

    return true;
}

void QSSGRenderSkyMaterialManager::deriveCycleState(QSSGRenderSkyMaterial *inSky, FrameState &fs)
{
    // Finalize frame: all slicing is complete, run normalize + irradiance now.
    if (m_finalizeIblPending && !fs.needRenderEnv && !fs.prefilteredJustCreated && m_accumIblSampleCount == fs.totalSamples) {
        m_finalizeIblPending = false;
        m_haveConvergedResult = true;
        fs.runPrefilterSlice = false;
        fs.writePrefilteredCubeThisFrame = true;
        fs.runIrradiancePass = fs.enableIBL;
        fs.sliceSamplesThisFrame = 0;
        fs.sliceSampleStart = m_accumulatedSamples;
        fs.sliceSampleEnd = m_accumulatedSamples;
        fs.isFirstSlice = false;
        fs.sliceCompletesCycle = false;
        fs.haveConvergedResultEntering = false; // was not converged before this frame
        fs.multiFrame = false;
        return;
    }

    fs.multiFrame = fs.enableIBL && inSky->iblSamplesPerFrame > 0 && inSky->iblSamplesPerFrame < fs.totalSamples;

    // Restart the accumulator if anything stale: env content changed, prefiltered cube was
    // re-created, or the sample-count target changed.
    if (!m_prefilterAccumulators.isEmpty()) {
        const bool resetAccumulation = fs.needRenderEnv || fs.prefilteredJustCreated || m_accumIblSampleCount != fs.totalSamples;
        if (resetAccumulation) {
            m_accumulatedSamples = 0;
            // Any finalize pending from the previous cycle is now stale — the accumulators
            // have been reset so running normalize would write garbage to the prefiltered cube.
            m_finalizeIblPending = false;
        }
        m_accumIblSampleCount = fs.totalSamples;
    }

    fs.prefilterIsConverged = !m_prefilterAccumulators.isEmpty() && m_accumulatedSamples >= fs.totalSamples;
    fs.runPrefilterSlice = !m_prefilterAccumulators.isEmpty() && !fs.prefilterIsConverged;

    // perFrameBudget = samples to integrate this frame.
    //   Single-frame (iblSamplesPerFrame <= 0) or !enableIBL: all remaining samples.
    //   Multi-frame:                                          the requested budget.
    fs.perFrameBudget = (inSky->iblSamplesPerFrame > 0 && fs.enableIBL) ? inSky->iblSamplesPerFrame
                                                                        : (fs.totalSamples - m_accumulatedSamples);
    fs.sliceSamplesThisFrame = fs.runPrefilterSlice ? qMin(fs.perFrameBudget, fs.totalSamples - m_accumulatedSamples) : 0;
    fs.sliceCompletesCycle = fs.runPrefilterSlice && (m_accumulatedSamples + fs.sliceSamplesThisFrame >= fs.totalSamples);

    fs.sliceSampleStart = m_accumulatedSamples;
    fs.sliceSampleEnd = fs.runPrefilterSlice ? qMin(m_accumulatedSamples + fs.perFrameBudget, fs.totalSamples) : 0;
    fs.isFirstSlice = m_accumulatedSamples == 0;
    fs.haveConvergedResultEntering = m_haveConvergedResult;

    if (inSky->iblRenderFrames >= 1) {
        // iblRenderFrames >= 1: normalize and irradiance are deferred to a dedicated frame
        // after all slicing completes, so the last slice frame never pays both costs.
        // m_finalizeIblPending is set in runPrefilterCycle (not here) so it only fires
        // once the accumulators actually contain data. Setting it here would trigger a
        // spurious finalize when the prefilter was blocked by canPrefilter==false (e.g.
        // skyRenderFrames >= 1 on the env-render frame), normalising empty accumulators.
        fs.writePrefilteredCubeThisFrame = false;
        fs.runIrradiancePass = false;
    } else {
        // iblRenderFrames == 0: normalize runs every frame so the user sees progressive
        // convergence while slicing, then fully on the cycle-completion frame.
        fs.writePrefilteredCubeThisFrame = fs.runPrefilterSlice && (fs.sliceCompletesCycle || !m_haveConvergedResult);
        fs.runIrradiancePass = fs.enableIBL && fs.writePrefilteredCubeThisFrame;
    }
}

void QSSGRenderSkyMaterialManager::validateAndUpdateCacheKey(const FrameState &fs, QSSGRhiShaderPipeline *envShaderPipelineKey)
{
    const bool cacheValid = m_cache.environmentMapSize == fs.environmentMapSize && m_cache.enableIBL == fs.enableIBL
            && m_cache.prefilterTotalMipCount == fs.prefilterTotalMipCount && m_cache.envCubeMap == m_envCubeMap
            && m_cache.prefilteredCubeMap == m_prefilteredCubeMap && m_cache.prefilterAccumulators == m_prefilterAccumulators
            && m_cache.envShaderPipeline == envShaderPipelineKey;
    if (cacheValid)
        return;
    clearPrefilterCache();
    m_cache.environmentMapSize = fs.environmentMapSize;
    m_cache.enableIBL = fs.enableIBL;
    m_cache.prefilterTotalMipCount = fs.prefilterTotalMipCount;
    m_cache.envCubeMap = m_envCubeMap;
    m_cache.prefilteredCubeMap = m_prefilteredCubeMap;
    m_cache.prefilterAccumulators = m_prefilterAccumulators;
    m_cache.envShaderPipeline = envShaderPipelineKey;
}

bool QSSGRenderSkyMaterialManager::ensureSharedResources(FrameState &fs, QRhiCommandBuffer *cb)
{
    const auto &context = m_context.rhiContext();
    auto *rhi = context->rhi();

    fs.inputLayout.setBindings({ { 3 * sizeof(float) } });
    fs.inputLayout.setAttributes({ { 0, 0, QRhiVertexInputAttribute::Float3, 0 } });

    fs.mvp = rhi->clipSpaceCorrMatrix();
    fs.mvp.perspective(90.0f, 1.0f, 0.1f, 10.0f);
    fs.views = skyIblEnvironmentMapViews(rhi);

    fs.ubufElementSize = rhi->ubufAligned(128);

    if (!m_cache.vertexBuffer) {
        m_cache.vertexBuffer = rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(skyIblCubeVerts));
        if (!m_cache.vertexBuffer->create()) {
            qWarning("Failed to create sky IBL vertex buffer");
            delete m_cache.vertexBuffer;
            m_cache.vertexBuffer = nullptr;
            return false;
        }
        auto *initRub = rhi->nextResourceUpdateBatch();
        initRub->uploadStaticBuffer(m_cache.vertexBuffer, skyIblCubeVerts);
        cb->resourceUpdate(initRub);
    }
    if (!ensureDynamicUBuf(rhi, m_cache.uBuf, fs.ubufElementSize * 6, "Failed to create sky IBL view uniform buffer"))
        return false;
    fs.vbufBinding = QRhiCommandBuffer::VertexInput(m_cache.vertexBuffer, 0);

    if (!m_cache.prefilterTargets.renderPassDesc) {
        if (!skyIblCreatePrefilterTargets(rhi, m_prefilteredCubeMap, fs.environmentMapSize, "SkyMaterialLightProbe procEnvPf"_ba, &m_cache.prefilterTargets)) {
            return false;
        }
    }

    if (!ensureDynamicUBuf(rhi, m_cache.uBufIrradiance, rhi->ubufAligned(20), "Failed to create sky IBL irradiance uniform buffer"))
        return false;

    const QSSGRhiSamplerDescription samplerNoMipDesc { QRhiSampler::Linear,      QRhiSampler::Linear,
                                                       QRhiSampler::None,        QRhiSampler::ClampToEdge,
                                                       QRhiSampler::ClampToEdge, QRhiSampler::Repeat };
    QRhiSampler *envMapCubeNoMipSampler = context->sampler(samplerNoMipDesc);

    if (!ensureSrb(rhi,
                   m_cache.irradianceSrb,
                   { QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(0, QRhiShaderResourceBinding::VertexStage, m_cache.uBuf, 128),
                     QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(2, QRhiShaderResourceBinding::FragmentStage, m_cache.uBufIrradiance, 20),
                     QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, m_envCubeMap, envMapCubeNoMipSampler) },
                   "Failed to create sky IBL irradiance SRB"))
        return false;

    if (!m_cache.irradiancePipeline) {
        const auto &shader = m_context.shaderCache()->getBuiltInRhiShaders().getRhienvironmentmapPreFilterShader(false);
        m_cache.irradiancePipeline = createPrefilterPipeline(rhi,
                                                             fs.inputLayout,
                                                             { shader.get(),
                                                               m_cache.irradianceSrb,
                                                               m_cache.prefilterTargets.renderPassDesc,
                                                               false },
                                                             "Failed to create sky IBL realtime irradiance pipeline "
                                                             "state");
        if (!m_cache.irradiancePipeline)
            return false;
    }
    return true;
}

bool QSSGRenderSkyMaterialManager::renderEnvironmentCube(QSSGRenderSkyMaterial *inSky,
                                                         const FrameState &fs,
                                                         const QSSGRhiShaderPipelinePtr &shaderPipeline,
                                                         QRhiCommandBuffer *cb,
                                                         QRhiResourceUpdateBatch *rub)
{
    const auto &context = m_context.rhiContext();
    auto *rhi = context->rhi();
    QSSGRhiContextPrivate *rhiCtxD = QSSGRhiContextPrivate::get(context.get());

    const quint32 skyElementSize = inSky->updateUniforms(m_context, fs.mvp, fs.views);

    if (!m_cache.envFaceTargets.renderPassDesc) {
        if (!skyIblCreateFaceTargets(rhi, m_envCubeMap, "SkyMaterialLightProbe procEnvCube"_ba, &m_cache.envFaceTargets))
            return false;
    }

    QRhiShaderResourceBindings *envSrb = rhiCtxD->srb(inSky->bindings);
    Q_ASSERT(envSrb);

    if (!m_cache.envMapPipeline) {
        m_cache.envMapPipeline = createPrefilterPipeline(rhi,
                                                         fs.inputLayout,
                                                         { shaderPipeline.get(), envSrb, m_cache.envFaceTargets.renderPassDesc, false },
                                                         "Failed to create sky IBL env map pipeline state");
        if (!m_cache.envMapPipeline)
            return false;
    }

    cb->resourceUpdate(rub);

    cb->debugMarkBegin("Sky IBL Procedural Environment Cubemap Generation");
    for (const auto face : QSSGRenderTextureCubeFaces) {
        const QVector<QPair<int, quint32>> offsets = { { 0, quint32(skyElementSize * quint8(face)) } };
        drawCubeFace(cb,
                     context.get(),
                     m_cache.envFaceTargets.renderTargets[quint8(face)],
                     fs.environmentMapSize,
                     m_cache.envMapPipeline,
                     envSrb,
                     fs.vbufBinding,
                     offsets,
                     QByteArrayLiteral("sky_ibl_procedural_environment_map"),
                     QSSG_RENDERPASS_NAME("sky_ibl_procedural_environment_map", 0, face));
    }
    cb->debugMarkEnd();

    inSky->isDirty = false;
    return true;
}

void QSSGRenderSkyMaterialManager::runEnvironmentMipChain(const FrameState &fs, QRhiCommandBuffer *cb)
{
    const auto &context = m_context.rhiContext();
    auto *rhi = context->rhi();

    if (!m_envTailMipsInitialized) {
        const int envFullMipCount = rhi->mipLevelsForSize(fs.environmentMapSize);
        skyIblInitializeUnrenderedMips(rhi, cb, context.get(), m_envCubeMap, 1, envFullMipCount, "SkyMaterialLightProbe procEnvCube"_ba);
        m_envTailMipsInitialized = true;
    }

    auto *rubMip = rhi->nextResourceUpdateBatch();
    rubMip->generateMips(m_envCubeMap);
    cb->resourceUpdate(rubMip);
}

bool QSSGRenderSkyMaterialManager::runPrefilterCycle(QSSGRenderSkyMaterial *inSky,
                                                     const FrameState &fs,
                                                     QRhiCommandBuffer *cb,
                                                     QRhiResourceUpdateBatch *&rub)
{
    Q_UNUSED(inSky);
    if (!fs.runPrefilterSlice && !fs.writePrefilteredCubeThisFrame) {
        cb->resourceUpdate(rub);
        rub = nullptr;
        return true;
    }

    const auto &context = m_context.rhiContext();
    auto *rhi = context->rhi();

    // Ensure per-mip face targets for the accumulator.
    //
    // Each specular mip level has its own non-mipmapped 2D array texture
    // (m_prefilterAccumulators[mip]), and we need two sets of face render
    // targets for it: one that preserves existing content (for slices 2..N,
    // additive blend) and one that clears (for the first slice).
    //
    // We always attach at level 0 because each accumulator texture has only
    // one mip level. This sidesteps the Android GLES driver bug where
    // glFramebufferTextureLayer ignores level > 0.
    if (m_cache.accumPreserveFaceTargets.size() != fs.prefilterSpecularMipCount) {
        // Release any stale targets (size mismatch after cache invalidation)
        for (QSSGSkyIblFaceTargets &t : m_cache.accumPreserveFaceTargets) {
            for (QRhiTextureRenderTarget *rt : t.renderTargets)
                delete rt;
            delete t.renderPassDesc;
        }
        m_cache.accumPreserveFaceTargets.clear();
        m_cache.accumPreserveFaceTargets.resize(fs.prefilterSpecularMipCount);

        for (int mip = 0; mip < fs.prefilterSpecularMipCount; ++mip) {
            if (!skyIblCreateFaceTargets(rhi,
                                         m_prefilterAccumulators[mip],
                                         "SkyMaterialLightProbe procEnvPfAccum/m"_ba + QByteArray::number(mip),
                                         &m_cache.accumPreserveFaceTargets[mip],
                                         true)) {
                return false;
            }
        }
    }

    if (m_cache.accumClearFaceTargets.size() != fs.prefilterSpecularMipCount) {
        for (QSSGSkyIblFaceTargets &t : m_cache.accumClearFaceTargets) {
            for (QRhiTextureRenderTarget *rt : t.renderTargets)
                delete rt;
            delete t.renderPassDesc;
        }
        m_cache.accumClearFaceTargets.clear();
        m_cache.accumClearFaceTargets.resize(fs.prefilterSpecularMipCount);

        for (int mip = 0; mip < fs.prefilterSpecularMipCount; ++mip) {
            if (!skyIblCreateFaceTargets(rhi,
                                         m_prefilterAccumulators[mip],
                                         "SkyMaterialLightProbe procEnvPfAccumClear/m"_ba + QByteArray::number(mip),
                                         &m_cache.accumClearFaceTargets[mip],
                                         false)) {
                return false;
            }
        }
    }

    QSSGSkyIblPrefilterTargets &prefilterTargets = m_cache.prefilterTargets;

    constexpr int uBufSliceSize = 32;
    constexpr int uBufNormalizeSize = 16;
    const int uBufSliceElementSize = rhi->ubufAligned(uBufSliceSize);
    const int uBufNormalizeElementSize = rhi->ubufAligned(uBufNormalizeSize);
    const int uBufNormalizeEntryCount = qMax(fs.prefilterSpecularMipCount, 1) * 6;

    if (!ensureDynamicUBuf(rhi, m_cache.uBufSlice, uBufSliceElementSize * qMax(fs.prefilterSpecularMipCount, 1), "Failed to create sky IBL slice uniform buffer"))
        return false;
    if (!ensureDynamicUBuf(rhi, m_cache.uBufNormalize, uBufNormalizeElementSize * uBufNormalizeEntryCount, "Failed to create sky IBL normalize uniform buffer"))
        return false;

    const QSSGRhiSamplerDescription mipSamplerDesc { QRhiSampler::Linear,      QRhiSampler::Linear,
                                                     QRhiSampler::Linear,      QRhiSampler::ClampToEdge,
                                                     QRhiSampler::ClampToEdge, QRhiSampler::Repeat };
    const QSSGRhiSamplerDescription noMipLinearSamplerDesc { QRhiSampler::Linear,      QRhiSampler::Linear,
                                                             QRhiSampler::None,        QRhiSampler::ClampToEdge,
                                                             QRhiSampler::ClampToEdge, QRhiSampler::Repeat };
    const QSSGRhiSamplerDescription nearestSamplerDesc { QRhiSampler::Nearest,     QRhiSampler::Nearest,
                                                         QRhiSampler::None,        QRhiSampler::ClampToEdge,
                                                         QRhiSampler::ClampToEdge, QRhiSampler::Repeat };
    QRhiSampler *envMapCubeSampler = context->sampler(fs.enableIBL ? mipSamplerDesc : noMipLinearSamplerDesc);
    QRhiSampler *accumReadSampler = context->sampler(nearestSamplerDesc);

    if (!ensureSrb(rhi,
                   m_cache.sliceSrb,
                   { QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(0, QRhiShaderResourceBinding::VertexStage, m_cache.uBuf, 128),
                     QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(2, QRhiShaderResourceBinding::FragmentStage, m_cache.uBufSlice, uBufSliceSize),
                     QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, m_envCubeMap, envMapCubeSampler) },
                   "Failed to create sky IBL slice SRB"))
        return false;

    // Create one normalize SRB per specular mip level, each binding the
    // corresponding per-mip accumulator texture. Since the accumulator textures
    // are separate non-mipmapped arrays (one per mip), we always read from
    // level 0 of whichever texture is bound — the normalize shader passes 0
    // as the texelFetch lod (mipLevel in the UBO is set to 0 for all entries).
    if (m_cache.normalizeSrbs.size() != fs.prefilterSpecularMipCount) {
        for (QRhiShaderResourceBindings *srb : std::as_const(m_cache.normalizeSrbs))
            delete srb;
        m_cache.normalizeSrbs.clear();
        m_cache.normalizeSrbs.resize(fs.prefilterSpecularMipCount, nullptr);
    }
    for (int mip = 0; mip < fs.prefilterSpecularMipCount; ++mip) {
        if (!ensureSrb(rhi,
                       m_cache.normalizeSrbs[mip],
                       { QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(0, QRhiShaderResourceBinding::VertexStage, m_cache.uBuf, 128),
                         QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(2, QRhiShaderResourceBinding::FragmentStage, m_cache.uBufNormalize, uBufNormalizeSize),
                         QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, m_prefilterAccumulators[mip], accumReadSampler) },
                       "Failed to create sky IBL normalize SRB"))
            return false;
    }

    if (!m_cache.slicePipeline) {
        const auto &shader = m_context.shaderCache()->getBuiltInRhiShaders().getRhiSkyIblPreFilterShader();
        m_cache.slicePipeline = createPrefilterPipeline(rhi,
                                                        fs.inputLayout,
                                                        { shader.get(),
                                                          m_cache.sliceSrb,
                                                          m_cache.accumPreserveFaceTargets[0].renderPassDesc,
                                                          true },
                                                        "Failed to create sky IBL slice pipeline state");
        if (!m_cache.slicePipeline)
            return false;
    }
    if (!m_cache.normalizeCubePipeline) {
        // All normalize SRBs share the same layout; create the pipeline from SRB 0.
        const auto &shader = m_context.shaderCache()->getBuiltInRhiShaders().getRhiSkyIblPreFilterNormalizeShader();
        m_cache.normalizeCubePipeline = createPrefilterPipeline(rhi,
                                                                fs.inputLayout,
                                                                { shader.get(),
                                                                  m_cache.normalizeSrbs[0],
                                                                  prefilterTargets.renderPassDesc,
                                                                  false },
                                                                "Failed to create sky IBL normalize-to-cube pipeline "
                                                                "state");
        if (!m_cache.normalizeCubePipeline)
            return false;
    }

    for (int mipLevel = 0; mipLevel < fs.prefilterSpecularMipCount; ++mipLevel) {
        struct SliceData
        {
            float roughness;
            float resolution;
            quint32 sampleStart;
            quint32 sampleEnd;
            quint32 totalSampleCount;
            quint32 _pad0;
            quint32 _pad1;
            quint32 _pad2;
        } sliceData;
        sliceData.roughness = float(mipLevel) / float(fs.prefilterRoughnessDenom);
        sliceData.resolution = fs.resolution;
        sliceData.sampleStart = quint32(fs.sliceSampleStart);
        sliceData.sampleEnd = quint32(fs.sliceSampleEnd);
        sliceData.totalSampleCount = quint32(fs.totalSamples);
        sliceData._pad0 = sliceData._pad1 = sliceData._pad2 = 0;
        rub->updateDynamicBuffer(m_cache.uBufSlice, mipLevel * uBufSliceElementSize, sizeof(SliceData), &sliceData);

        for (const auto face : QSSGRenderTextureCubeFaces) {
            struct NormalizeData
            {
                qint32 faceIndex;
                qint32 _pad0 = 0;
                qint32 _pad1 = 0;
                qint32 _pad2 = 0;
            } normalizeData;
            normalizeData.faceIndex = quint8(face);
            const int entryIndex = mipLevel * 6 + quint8(face);
            rub->updateDynamicBuffer(m_cache.uBufNormalize, entryIndex * uBufNormalizeElementSize, sizeof(NormalizeData), &normalizeData);
        }
    }

    cb->resourceUpdate(rub);
    rub = nullptr;

    // Slice accumulation pass.
    //
    // On the first slice we use the clear-variant face targets (no PreserveColorContents)
    // so the render pass clears the accumulator before the additive draw. This replaces
    // the old pattern of a separate empty clear pass followed by a preserve pass, which
    // was fragile on tile-based GPUs (the load in the preserve pass could see stale tile
    // data from a prior clear pass that stored to the same surface).
    //
    // On subsequent slices we use the preserve-variant targets so the additive blend
    // accumulates on top of the existing content.
    if (fs.runPrefilterSlice) {
        for (int mipLevel = 0; mipLevel < fs.prefilterSpecularMipCount; ++mipLevel) {
            QSSGSkyIblFaceTargets &sliceTargets = fs.isFirstSlice ? m_cache.accumClearFaceTargets[mipLevel]
                                                                  : m_cache.accumPreserveFaceTargets[mipLevel];
            const QSize mipSize(qMax(1, fs.environmentMapSize.width() >> mipLevel),
                                qMax(1, fs.environmentMapSize.height() >> mipLevel));

            for (const auto face : QSSGRenderTextureCubeFaces) {
                const QVector<QPair<int, quint32>> offsets = { { 0, quint32(fs.ubufElementSize * quint8(face)) },
                                                               { 2, quint32(uBufSliceElementSize * mipLevel) } };
                drawCubeFace(cb,
                             context.get(),
                             sliceTargets.renderTargets[quint8(face)],
                             mipSize,
                             m_cache.slicePipeline,
                             m_cache.sliceSrb,
                             fs.vbufBinding,
                             offsets,
                             QByteArrayLiteral("sky_ibl_prefilter_slice"),
                             QSSG_RENDERPASS_NAME("sky_ibl_prefilter_slice", mipLevel, face),
                             QColor(0, 0, 0, 0));
            }
        }
        m_accumulatedSamples = fs.sliceSampleEnd;
        if (fs.sliceCompletesCycle) {
            if (inSky->iblRenderFrames >= 1) {
                // Defer normalize+irradiance to a dedicated finalize frame.
                // Set the flag here (after the slice actually ran) so the accumulators
                // are guaranteed to contain data when the finalize fires.
                m_finalizeIblPending = true;
            } else {
                // iblRenderFrames==0: normalize runs inline, mark converged now.
                m_haveConvergedResult = true;
            }
        }
    }

    // Normalize accumulated samples into the prefiltered cube.
    // For iblRenderFrames==0: runs every frame until convergence (progressive).
    // For iblRenderFrames>=1: only runs in the dedicated finalize frame.
    if (fs.writePrefilteredCubeThisFrame) {
        for (int mipLevel = 0; mipLevel < fs.prefilterSpecularMipCount; ++mipLevel) {
            for (const auto face : QSSGRenderTextureCubeFaces) {
                const int normalizeEntryIndex = mipLevel * 6 + quint8(face);
                const QVector<QPair<int, quint32>> offsets = { { 0, quint32(fs.ubufElementSize * quint8(face)) },
                                                               { 2, quint32(uBufNormalizeElementSize * normalizeEntryIndex) } };
                drawCubeFace(cb,
                             context.get(),
                             prefilterTargets.mipRenderTargetsMap[mipLevel][quint8(face)],
                             prefilterTargets.mipLevelSizes[mipLevel],
                             m_cache.normalizeCubePipeline,
                             m_cache.normalizeSrbs[mipLevel],
                             fs.vbufBinding,
                             offsets,
                             QByteArrayLiteral("sky_ibl_prefilter_normalize"),
                             QSSG_RENDERPASS_NAME("sky_ibl_prefilter_normalize", mipLevel, face));
            }
        }
    }

    if (fs.runIrradiancePass) {
        const int irradianceMip = prefilterTargets.mipmapCount - 1;
        for (const auto face : QSSGRenderTextureCubeFaces) {
            const QVector<QPair<int, quint32>> offsets = { { 0, quint32(fs.ubufElementSize * quint8(face)) }, { 2, 0u } };
            drawCubeFace(cb,
                         context.get(),
                         prefilterTargets.mipRenderTargetsMap[irradianceMip][quint8(face)],
                         prefilterTargets.mipLevelSizes[irradianceMip],
                         m_cache.irradiancePipeline,
                         m_cache.irradianceSrb,
                         fs.vbufBinding,
                         offsets,
                         QByteArrayLiteral("sky_ibl_irradiance"),
                         QSSG_RENDERPASS_NAME("sky_ibl_irradiance", irradianceMip, face));
        }
    }

    return true;
}

void QSSGRenderSkyMaterialManager::initializeTailMips(const FrameState &fs, QRhiCommandBuffer *cb)
{
    if (!m_prefilteredCubeMap->flags().testFlag(QRhiTexture::MipMapped) || m_prefilteredTailMipsInitialized)
        return;
    const auto &context = m_context.rhiContext();
    auto *rhi = context->rhi();
    const int prefilteredFullMipCount = rhi->mipLevelsForSize(fs.environmentMapSize);
    // Mips actually written this lifetime depend on enableIBL:
    //   * enableIBL=true:  prefilter loop writes mips [0, prefilterMipCount), irradiance writes
    //                      the last (prefilterTargets.mipmapCount-1). So mips [0, prefilterTargets.mipmapCount)
    //                      are covered → init starts at prefilterTargets.mipmapCount.
    //   * enableIBL=false: prefilter loop writes only mip 0, irradiance is skipped → init starts at 1.
    // The prefiltered cube is allocated mip-mapped in both modes for downstream sampler
    // completeness; we just need to clear the tail.
    const int firstUnwrittenMip = fs.enableIBL ? m_cache.prefilterTargets.mipmapCount : 1;
    skyIblInitializeUnrenderedMips(rhi, cb, context.get(), m_prefilteredCubeMap, firstUnwrittenMip, prefilteredFullMipCount, "SkyMaterialLightProbe"_ba);
    m_prefilteredTailMipsInitialized = true;
}

QT_END_NAMESPACE
