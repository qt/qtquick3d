// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QSSGRENDERSKYMATERIALMANAGER_H
#define QSSGRENDERSKYMATERIALMANAGER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtQuick3DRuntimeRender/private/qtquick3druntimerenderglobal_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderimagetexture_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderskymaterial_p.h>

#include <QtCore/qhash.h>
#include <QtCore/qlist.h>
#include <QtCore/qmap.h>
#include <QtCore/qsize.h>
#include <QtCore/qvarlengtharray.h>

#include <rhi/qrhi.h>

#include <memory>

QT_BEGIN_NAMESPACE

class QSSGRenderContextInterface;
class QSSGRhiShaderPipeline;

using QSSGSkyIblCubeFaceRenderTargets = QVarLengthArray<QRhiTextureRenderTarget *, 6>;

struct QSSGSkyIblFaceTargets
{
    QSSGSkyIblCubeFaceRenderTargets renderTargets;
    QRhiRenderPassDescriptor *renderPassDesc = nullptr;
};

struct QSSGSkyIblPrefilterTargets
{
    int mipmapCount = 0;
    QVarLengthArray<QSize, 6> mipLevelSizes;
    QVarLengthArray<QSSGSkyIblCubeFaceRenderTargets, 6> mipRenderTargetsMap;
    QRhiRenderPassDescriptor *renderPassDesc = nullptr;
};

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRenderSkyMaterialManager
{
    Q_DISABLE_COPY(QSSGRenderSkyMaterialManager)
public:
    explicit QSSGRenderSkyMaterialManager(const QSSGRenderContextInterface &inContext);
    ~QSSGRenderSkyMaterialManager();

    void releaseCachedResources();

    QSSGRenderImageTexture resolve(QSSGRenderSkyMaterial *settings);

private:
    bool ensureEnvironmentMap(QSSGRenderSkyMaterial *settings);

    struct FrameState;

    bool computeFrameState(QSSGRenderSkyMaterial *inSky, FrameState &fs);
    bool ensureTextures(FrameState &fs);
    void deriveCycleState(QSSGRenderSkyMaterial *inSky, FrameState &fs);
    void validateAndUpdateCacheKey(const FrameState &fs, QSSGRhiShaderPipeline *envShaderPipelineKey);
    bool ensureSharedResources(FrameState &fs, QRhiCommandBuffer *cb);
    bool renderEnvironmentCube(QSSGRenderSkyMaterial *inSky,
                               const FrameState &fs,
                               const QSSGRhiShaderPipelinePtr &shaderPipeline,
                               QRhiCommandBuffer *cb,
                               QRhiResourceUpdateBatch *rub);
    void runEnvironmentMipChain(const FrameState &fs, QRhiCommandBuffer *cb);
    bool runPrefilterCycle(QSSGRenderSkyMaterial *inSky, const FrameState &fs, QRhiCommandBuffer *cb, QRhiResourceUpdateBatch *&rub);
    void initializeTailMips(const FrameState &fs, QRhiCommandBuffer *cb);

    const QSSGRenderContextInterface &m_context;

    QSSGRenderImageTexture m_skyIblTexture;

    // The procedural cubemap is shared between IBL modes;
    // m_prefilteredCubeMap is only allocated when IBL is enabled.
    QRhiTexture *m_envCubeMap = nullptr;
    QRhiTexture *m_prefilteredCubeMap = nullptr;
    QSize m_cubeMapSize;
    int m_prefilteredMipCount = 0;

    // One non-mipmapped GL_TEXTURE_2D_ARRAY per specular mip level.
    // Storing them separately (rather than as one mipmapped array) works
    // around an Android GLES driver bug where glFramebufferTextureLayer
    // silently ignores level > 0, causing all slice writes to land on
    // mip 0 and corrupting the prefiltered result. Each texture here has
    // exactly one mip level and is always attached at level 0.
    QList<QRhiTexture *> m_prefilterAccumulators;

    int m_accumulatedSamples = 0;
    int m_accumIblSampleCount = 0;
    bool m_haveConvergedResult = false;
    bool m_envTailMipsInitialized = false;
    bool m_prefilteredTailMipsInitialized = false;

    struct PrefilterResourceCache
    {
        // Invalidation keys
        QSize environmentMapSize;
        bool enableIBL = false;
        int prefilterTotalMipCount = 0;
        QRhiTexture *envCubeMap = nullptr;
        QRhiTexture *prefilteredCubeMap = nullptr;
        // One entry per specular mip level, mirroring m_prefilterAccumulators.
        QList<QRhiTexture *> prefilterAccumulators;
        QSSGRhiShaderPipeline *envShaderPipeline = nullptr;

        // Cached resources.
        QRhiBuffer *vertexBuffer = nullptr;
        QRhiBuffer *uBuf = nullptr;
        QRhiBuffer *uBufSlice = nullptr;
        QRhiBuffer *uBufNormalize = nullptr;
        QRhiBuffer *uBufIrradiance = nullptr;

        QSSGSkyIblFaceTargets envFaceTargets;
        QSSGSkyIblPrefilterTargets prefilterTargets;

        // Per-mip face targets for the accumulator (one entry per specular mip level).
        // Preserve variant: loads existing content so additive slices accumulate.
        // Clear variant:    clears before drawing, used on the first slice.
        QList<QSSGSkyIblFaceTargets> accumPreserveFaceTargets;
        QList<QSSGSkyIblFaceTargets> accumClearFaceTargets;

        QRhiShaderResourceBindings *sliceSrb = nullptr;
        // One SRB per specular mip level, each binding the corresponding
        // per-mip accumulator texture so the normalize pass reads the right data.
        QVarLengthArray<QRhiShaderResourceBindings *, 6> normalizeSrbs;
        QRhiShaderResourceBindings *irradianceSrb = nullptr;

        QRhiGraphicsPipeline *envMapPipeline = nullptr;
        QRhiGraphicsPipeline *slicePipeline = nullptr;
        QRhiGraphicsPipeline *normalizeCubePipeline = nullptr;
        QRhiGraphicsPipeline *irradiancePipeline = nullptr;
    };

    PrefilterResourceCache m_cache;

    void clearPrefilterCache();
};

using QSSGRenderSkyMaterialManagerPtr = std::shared_ptr<QSSGRenderSkyMaterialManager>;

QT_END_NAMESPACE

#endif
