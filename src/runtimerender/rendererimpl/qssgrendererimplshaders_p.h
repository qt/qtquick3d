// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QSSGRENDERERIMPLSHADERS_P_H
#define QSSGRENDERERIMPLSHADERS_P_H

#include <QtCore/qbytearray.h>

#include <QtQuick3DRuntimeRender/private/qtquick3druntimerenderglobal_p.h>

#include <QtQuick3DRuntimeRender/private/qssgrhicontext_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderlayer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderparticles_p.h>

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

QT_BEGIN_NAMESPACE

class QSSGShaderCache;

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGBuiltInRhiShaderCache
{
public:
    explicit QSSGBuiltInRhiShaderCache(QSSGShaderCache &shaderCache)
        : m_shaderCache(shaderCache) {}

    void releaseCachedResources();

    enum class LightmapUVRasterizationShaderMode {
        Default,
        Uv,
        UvTangent
    };

    QSSGRhiShaderPipelinePtr getRhiCubemapShadowBlurXShader();
    QSSGRhiShaderPipelinePtr getRhiCubemapShadowBlurYShader();
    QSSGRhiShaderPipelinePtr getRhiGridShader(int viewCount);
    QSSGRhiShaderPipelinePtr getRhiOrthographicShadowBlurXShader();
    QSSGRhiShaderPipelinePtr getRhiOrthographicShadowBlurYShader();
    QSSGRhiShaderPipelinePtr getRhiSsaoShader(int viewCount);
    QSSGRhiShaderPipelinePtr getRhiSkyBoxCubeShader(int viewCount);
    QSSGRhiShaderPipelinePtr getRhiSkyBoxShader(QSSGRenderLayer::TonemapMode tonemapMode, bool isRGBE, int viewCount);
    QSSGRhiShaderPipelinePtr getRhiSupersampleResolveShader(int viewCount);
    QSSGRhiShaderPipelinePtr getRhiProgressiveAAShader();
    QSSGRhiShaderPipelinePtr getRhiParticleShader(QSSGRenderParticles::FeatureLevel featureLevel, int viewCount);
    QSSGRhiShaderPipelinePtr getRhiSimpleQuadShader(int viewCount);
    QSSGRhiShaderPipelinePtr getRhiLightmapUVRasterizationShader(LightmapUVRasterizationShaderMode mode);
    QSSGRhiShaderPipelinePtr getRhiLightmapDilateShader();
    QSSGRhiShaderPipelinePtr getRhiDebugObjectShader();
    QSSGRhiShaderPipelinePtr getRhiReflectionprobePreFilterShader();
    QSSGRhiShaderPipelinePtr getRhienvironmentmapPreFilterShader(bool isRGBE);
    QSSGRhiShaderPipelinePtr getRhiEnvironmentmapShader();

private:
    QSSGShaderCache &m_shaderCache; // We're owned by the shadercache

    struct BuiltinShader {
        // The shader refs are non-null if we have attempted to generate the
        // shader. This does not mean we were successul, however.
        QSSGRhiShaderPipelinePtr shaderPipeline;
        int viewCount = 1;
    };

    QSSGRhiShaderPipelinePtr getBuiltinRhiShader(const QByteArray &name,
                                                BuiltinShader &storage,
                                                int viewCount = 1);

    struct {
        BuiltinShader cubemapShadowBlurXRhiShader;
        BuiltinShader cubemapShadowBlurYRhiShader;
        BuiltinShader gridShader;
        BuiltinShader orthographicShadowBlurXRhiShader;
        BuiltinShader orthographicShadowBlurYRhiShader;
        BuiltinShader ssaoRhiShader;
        BuiltinShader skyBoxRhiShader[QSSGRenderLayer::TonemapModeCount * 2 /* rgbe+hdr */];
        BuiltinShader skyBoxCubeRhiShader;
        BuiltinShader supersampleResolveRhiShader;
        BuiltinShader progressiveAARhiShader;
        BuiltinShader texturedQuadRhiShader;
        BuiltinShader simpleQuadRhiShader;
        BuiltinShader lightmapUVRasterShader;
        BuiltinShader lightmapUVRasterShader_uv;
        BuiltinShader lightmapUVRasterShader_uv_tangent;
        BuiltinShader lightmapDilateShader;
        BuiltinShader debugObjectShader;

        BuiltinShader reflectionprobePreFilterShader;
        BuiltinShader environmentmapPreFilterShader[2];
        BuiltinShader environmentmapShader;

        BuiltinShader particlesNoLightingSimpleRhiShader;
        BuiltinShader particlesNoLightingMappedRhiShader;
        BuiltinShader particlesNoLightingAnimatedRhiShader;
        BuiltinShader particlesVLightingSimpleRhiShader;
        BuiltinShader particlesVLightingMappedRhiShader;
        BuiltinShader particlesVLightingAnimatedRhiShader;
        BuiltinShader lineParticlesRhiShader;
        BuiltinShader lineParticlesMappedRhiShader;
        BuiltinShader lineParticlesAnimatedRhiShader;
        BuiltinShader lineParticlesVLightRhiShader;
        BuiltinShader lineParticlesMappedVLightRhiShader;
        BuiltinShader lineParticlesAnimatedVLightRhiShader;
    } m_cache;
};

QT_END_NAMESPACE

#endif // QSSGRENDERERIMPLSHADERS_P_H
