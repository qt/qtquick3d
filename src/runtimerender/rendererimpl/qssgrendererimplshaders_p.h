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
    // The shader refs are non-null if we have attempted to generate the
    // shader. This does not mean we were successul, however.
    // RHI
    QSSGRhiShaderPipelinePtr m_cubemapShadowBlurXRhiShader;
    QSSGRhiShaderPipelinePtr m_cubemapShadowBlurYRhiShader;
    QSSGRhiShaderPipelinePtr m_gridShader;
    QSSGRhiShaderPipelinePtr m_orthographicShadowBlurXRhiShader;
    QSSGRhiShaderPipelinePtr m_orthographicShadowBlurYRhiShader;
    QSSGRhiShaderPipelinePtr m_ssaoRhiShader;
    QSSGRhiShaderPipelinePtr m_skyBoxRhiShader[QSSGRenderLayer::TonemapModeCount * 2 /* rgbe+hdr */];
    QSSGRhiShaderPipelinePtr m_skyBoxCubeRhiShader;
    QSSGRhiShaderPipelinePtr m_supersampleResolveRhiShader;
    QSSGRhiShaderPipelinePtr m_progressiveAARhiShader;
    QSSGRhiShaderPipelinePtr m_texturedQuadRhiShader;
    QSSGRhiShaderPipelinePtr m_simpleQuadRhiShader;
    QSSGRhiShaderPipelinePtr m_lightmapUVRasterShader;
    QSSGRhiShaderPipelinePtr m_lightmapUVRasterShader_uv;
    QSSGRhiShaderPipelinePtr m_lightmapUVRasterShader_uv_tangent;
    QSSGRhiShaderPipelinePtr m_lightmapDilateShader;
    QSSGRhiShaderPipelinePtr m_debugObjectShader;

    QSSGRhiShaderPipelinePtr m_reflectionprobePreFilterShader;
    QSSGRhiShaderPipelinePtr m_environmentmapPreFilterShader[2];
    QSSGRhiShaderPipelinePtr m_environmentmapShader;

    QSSGRhiShaderPipelinePtr m_particlesNoLightingSimpleRhiShader;
    QSSGRhiShaderPipelinePtr m_particlesNoLightingMappedRhiShader;
    QSSGRhiShaderPipelinePtr m_particlesNoLightingAnimatedRhiShader;
    QSSGRhiShaderPipelinePtr m_particlesVLightingSimpleRhiShader;
    QSSGRhiShaderPipelinePtr m_particlesVLightingMappedRhiShader;
    QSSGRhiShaderPipelinePtr m_particlesVLightingAnimatedRhiShader;
    QSSGRhiShaderPipelinePtr m_lineParticlesRhiShader;
    QSSGRhiShaderPipelinePtr m_lineParticlesMappedRhiShader;
    QSSGRhiShaderPipelinePtr m_lineParticlesAnimatedRhiShader;
    QSSGRhiShaderPipelinePtr m_lineParticlesVLightRhiShader;
    QSSGRhiShaderPipelinePtr m_lineParticlesMappedVLightRhiShader;
    QSSGRhiShaderPipelinePtr m_lineParticlesAnimatedVLightRhiShader;

    QSSGRhiShaderPipelinePtr getBuiltinRhiShader(const QByteArray &name, QSSGRhiShaderPipelinePtr &storage);

    QSSGShaderCache &m_shaderCache; // We're owned by the shadercache
    explicit QSSGBuiltInRhiShaderCache(QSSGShaderCache &shaderCache)
        : m_shaderCache(shaderCache) {}

    friend class QSSGShaderCache;

public:
    enum class LightmapUVRasterizationShaderMode {
        Default,
        Uv,
        UvTangent
    };

        // shader implementations, RHI
    QSSGRhiShaderPipelinePtr getRhiCubemapShadowBlurXShader();
    QSSGRhiShaderPipelinePtr getRhiCubemapShadowBlurYShader();
    QSSGRhiShaderPipelinePtr getRhiGridShader();
    QSSGRhiShaderPipelinePtr getRhiOrthographicShadowBlurXShader();
    QSSGRhiShaderPipelinePtr getRhiOrthographicShadowBlurYShader();
    QSSGRhiShaderPipelinePtr getRhiSsaoShader();
    QSSGRhiShaderPipelinePtr getRhiSkyBoxCubeShader();
    QSSGRhiShaderPipelinePtr getRhiSkyBoxShader(QSSGRenderLayer::TonemapMode tonemapMode, bool isRGBE);
    QSSGRhiShaderPipelinePtr getRhiSupersampleResolveShader();
    QSSGRhiShaderPipelinePtr getRhiProgressiveAAShader();
    QSSGRhiShaderPipelinePtr getRhiTexturedQuadShader();
    QSSGRhiShaderPipelinePtr getRhiParticleShader(QSSGRenderParticles::FeatureLevel featureLevel);
    QSSGRhiShaderPipelinePtr getRhiSimpleQuadShader();
    QSSGRhiShaderPipelinePtr getRhiLightmapUVRasterizationShader(LightmapUVRasterizationShaderMode mode);
    QSSGRhiShaderPipelinePtr getRhiLightmapDilateShader();
    QSSGRhiShaderPipelinePtr getRhiDebugObjectShader();
    QSSGRhiShaderPipelinePtr getRhiReflectionprobePreFilterShader();
    QSSGRhiShaderPipelinePtr getRhienvironmentmapPreFilterShader(bool isRGBE);
    QSSGRhiShaderPipelinePtr getRhiEnvironmentmapShader();
};

QT_END_NAMESPACE

#endif // QSSGRENDERERIMPLSHADERS_P_H
