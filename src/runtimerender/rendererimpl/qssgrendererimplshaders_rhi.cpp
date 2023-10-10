// Copyright (C) 2008-2012 NVIDIA Corporation.
// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qssgrendererimplshaders_p.h"

#include <QtQuick3DRuntimeRender/private/qssgrenderer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderlight_p.h>
#include "../qssgrendercontextcore.h"
#include <QtQuick3DRuntimeRender/private/qssgrendershadercache_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershaderlibrarymanager_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercodegenerator_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderdefaultmaterialshadergenerator_p.h>
#include <QtQuick3DRuntimeRender/private/qssgvertexpipelineimpl_p.h>

// this file contains the getXxxxShader implementations suitable for the QRhi-based rendering path

QT_BEGIN_NAMESPACE

QSSGRhiShaderPipelinePtr QSSGBuiltInRhiShaderCache::getBuiltinRhiShader(const QByteArray &name,
                                                                        QSSGRhiShaderPipelinePtr &storage)
{
    QSSGRhiShaderPipelinePtr &result = storage;
    if (!result) {
        // loadBuiltin must always return a valid QSSGRhiShaderPipeline.
        // There will just be no stages if loading fails.
        result = m_shaderCache.loadBuiltinForRhi(name);
    }
    return result;
}

QSSGRhiShaderPipelinePtr QSSGBuiltInRhiShaderCache::getRhiCubemapShadowBlurXShader()
{
    return getBuiltinRhiShader(QByteArrayLiteral("cubeshadowblurx"), m_cubemapShadowBlurXRhiShader);
}

QSSGRhiShaderPipelinePtr QSSGBuiltInRhiShaderCache::getRhiCubemapShadowBlurYShader()
{
    return getBuiltinRhiShader(QByteArrayLiteral("cubeshadowblury"), m_cubemapShadowBlurYRhiShader);
}

QSSGRhiShaderPipelinePtr QSSGBuiltInRhiShaderCache::getRhiGridShader()
{
    return getBuiltinRhiShader(QByteArrayLiteral("grid"), m_gridShader);
}

QSSGRhiShaderPipelinePtr QSSGBuiltInRhiShaderCache::getRhiOrthographicShadowBlurXShader()
{
    return getBuiltinRhiShader(QByteArrayLiteral("orthoshadowblurx"), m_orthographicShadowBlurXRhiShader);
}

QSSGRhiShaderPipelinePtr QSSGBuiltInRhiShaderCache::getRhiOrthographicShadowBlurYShader()
{
    return getBuiltinRhiShader(QByteArrayLiteral("orthoshadowblury"), m_orthographicShadowBlurYRhiShader);
}

QSSGRhiShaderPipelinePtr QSSGBuiltInRhiShaderCache::getRhiSsaoShader()
{
    return getBuiltinRhiShader(QByteArrayLiteral("ssao"), m_ssaoRhiShader);
}

QSSGRhiShaderPipelinePtr QSSGBuiltInRhiShaderCache::getRhiSkyBoxCubeShader()
{
    return getBuiltinRhiShader(QByteArrayLiteral("skyboxcube"), m_skyBoxCubeRhiShader);
}

static inline constexpr size_t getSkyboxIndex(QSSGRenderLayer::TonemapMode tonemapMode, bool isRGBE)
{
    switch (tonemapMode) {
    case QSSGRenderLayer::TonemapMode::None:
        return 0 + (size_t(isRGBE) * QSSGRenderLayer::TonemapModeCount);
    case QSSGRenderLayer::TonemapMode::Linear:
        return 1 + (size_t(isRGBE) * QSSGRenderLayer::TonemapModeCount);
    case QSSGRenderLayer::TonemapMode::Aces:
        return 2 + (size_t(isRGBE) * QSSGRenderLayer::TonemapModeCount);
    case QSSGRenderLayer::TonemapMode::HejlDawson:
        return 3 + (size_t(isRGBE) * QSSGRenderLayer::TonemapModeCount);
    case QSSGRenderLayer::TonemapMode::Filmic:
        return 4 + (size_t(isRGBE) * QSSGRenderLayer::TonemapModeCount);
    }

    Q_UNREACHABLE_RETURN(0);
}

QSSGRhiShaderPipelinePtr QSSGBuiltInRhiShaderCache::getRhiSkyBoxShader(QSSGRenderLayer::TonemapMode tonemapMode, bool isRGBE)
{
    // Skybox shader is special and has multiple possible shaders so we have to do
    // a bit of manual work here (mapping resolved in getSkyboxIndex()).
    static constexpr char variant[][23] { "skybox_hdr_none",
                                          "skybox_hdr_linear",
                                          "skybox_hdr_aces",
                                          "skybox_hdr_hejldawson",
                                          "skybox_hdr_filmic",
                                          "skybox_rgbe_none",
                                          "skybox_rgbe_linear",
                                          "skybox_rgbe_aces",
                                          "skybox_rgbe_hejldawson",
                                          "skybox_rgbe_filmic" };

    const size_t skyboxIndex = getSkyboxIndex(tonemapMode, isRGBE);
    return getBuiltinRhiShader(QByteArray::fromRawData(variant[skyboxIndex], std::char_traits<char>::length(variant[skyboxIndex])), m_skyBoxRhiShader[skyboxIndex]);
}

QSSGRhiShaderPipelinePtr QSSGBuiltInRhiShaderCache::getRhiSupersampleResolveShader()
{
    return getBuiltinRhiShader(QByteArrayLiteral("ssaaresolve"), m_supersampleResolveRhiShader);
}

QSSGRhiShaderPipelinePtr QSSGBuiltInRhiShaderCache::getRhiProgressiveAAShader()
{
    return getBuiltinRhiShader(QByteArrayLiteral("progressiveaa"), m_progressiveAARhiShader);
}

QSSGRhiShaderPipelinePtr QSSGBuiltInRhiShaderCache::getRhiTexturedQuadShader()
{
    return getBuiltinRhiShader(QByteArrayLiteral("texturedquad"), m_texturedQuadRhiShader);
}

QSSGRhiShaderPipelinePtr QSSGBuiltInRhiShaderCache::getRhiParticleShader(QSSGRenderParticles::FeatureLevel featureLevel)
{
    switch (featureLevel) {
    case QSSGRenderParticles::FeatureLevel::Simple:
        return getBuiltinRhiShader(QByteArrayLiteral("particlesnolightsimple"), m_particlesNoLightingSimpleRhiShader);
        break;
    case QSSGRenderParticles::FeatureLevel::Mapped:
        return getBuiltinRhiShader(QByteArrayLiteral("particlesnolightmapped"), m_particlesNoLightingMappedRhiShader);
        break;
    case QSSGRenderParticles::FeatureLevel::Animated:
        return getBuiltinRhiShader(QByteArrayLiteral("particlesnolightanimated"), m_particlesNoLightingAnimatedRhiShader);
        break;
    case QSSGRenderParticles::FeatureLevel::SimpleVLight:
        return getBuiltinRhiShader(QByteArrayLiteral("particlesvlightsimple"), m_particlesVLightingSimpleRhiShader);
        break;
    case QSSGRenderParticles::FeatureLevel::MappedVLight:
        return getBuiltinRhiShader(QByteArrayLiteral("particlesvlightmapped"), m_particlesVLightingMappedRhiShader);
        break;
    case QSSGRenderParticles::FeatureLevel::AnimatedVLight:
        return getBuiltinRhiShader(QByteArrayLiteral("particlesvlightanimated"), m_particlesVLightingAnimatedRhiShader);
        break;
    case QSSGRenderParticles::FeatureLevel::Line:
        return getBuiltinRhiShader(QByteArrayLiteral("lineparticles"), m_lineParticlesRhiShader);
        break;
    case QSSGRenderParticles::FeatureLevel::LineMapped:
        return getBuiltinRhiShader(QByteArrayLiteral("lineparticlesmapped"), m_lineParticlesMappedRhiShader);
        break;
    case QSSGRenderParticles::FeatureLevel::LineAnimated:
        return getBuiltinRhiShader(QByteArrayLiteral("lineparticlesanimated"), m_lineParticlesAnimatedRhiShader);
        break;
    case QSSGRenderParticles::FeatureLevel::LineVLight:
        return getBuiltinRhiShader(QByteArrayLiteral("lineparticlesvlightsimple"), m_lineParticlesVLightRhiShader);
        break;
    case QSSGRenderParticles::FeatureLevel::LineMappedVLight:
        return getBuiltinRhiShader(QByteArrayLiteral("lineparticlesvlightmapped"), m_lineParticlesMappedVLightRhiShader);
        break;
    case QSSGRenderParticles::FeatureLevel::LineAnimatedVLight:
        return getBuiltinRhiShader(QByteArrayLiteral("lineparticlesvlightanimated"), m_lineParticlesAnimatedVLightRhiShader);
        break;
    }
    return getBuiltinRhiShader(QByteArrayLiteral("particlesnolightanimated"), m_particlesNoLightingAnimatedRhiShader);
}

QSSGRhiShaderPipelinePtr QSSGBuiltInRhiShaderCache::getRhiSimpleQuadShader()
{
    return getBuiltinRhiShader(QByteArrayLiteral("simplequad"), m_simpleQuadRhiShader);
}

QSSGRhiShaderPipelinePtr QSSGBuiltInRhiShaderCache::getRhiLightmapUVRasterizationShader(LightmapUVRasterizationShaderMode mode)
{
    switch (mode) {
    case LightmapUVRasterizationShaderMode::Uv:
        return getBuiltinRhiShader(QByteArrayLiteral("lightmapuvraster_uv"), m_lightmapUVRasterShader_uv);
    case LightmapUVRasterizationShaderMode::UvTangent:
        return getBuiltinRhiShader(QByteArrayLiteral("lightmapuvraster_uv_tangent"), m_lightmapUVRasterShader_uv_tangent);
    case LightmapUVRasterizationShaderMode::Default:
        return getBuiltinRhiShader(QByteArrayLiteral("lightmapuvraster"), m_lightmapUVRasterShader);
    }

    Q_UNREACHABLE_RETURN(getBuiltinRhiShader(QByteArrayLiteral("lightmapuvraster"), m_lightmapUVRasterShader));
}

QSSGRhiShaderPipelinePtr QSSGBuiltInRhiShaderCache::getRhiLightmapDilateShader()
{
    return getBuiltinRhiShader(QByteArrayLiteral("lightmapdilate"), m_lightmapDilateShader);
}

QSSGRhiShaderPipelinePtr QSSGBuiltInRhiShaderCache::getRhiDebugObjectShader()
{
    return getBuiltinRhiShader(QByteArrayLiteral("debugobject"), m_debugObjectShader);
}

QSSGRhiShaderPipelinePtr QSSGBuiltInRhiShaderCache::getRhiReflectionprobePreFilterShader()
{
    return getBuiltinRhiShader(QByteArrayLiteral("reflectionprobeprefilter"), m_reflectionprobePreFilterShader);
}

QSSGRhiShaderPipelinePtr QSSGBuiltInRhiShaderCache::getRhienvironmentmapPreFilterShader(bool isRGBE)
{
    static constexpr char variant[][29] { "environmentmapprefilter", "environmentmapprefilter_rgbe" };
    const quint8 idx = quint8(isRGBE);
    return getBuiltinRhiShader(QByteArray::fromRawData(variant[idx], std::char_traits<char>::length(variant[idx])), m_environmentmapPreFilterShader[idx]);
}

QSSGRhiShaderPipelinePtr QSSGBuiltInRhiShaderCache::getRhiEnvironmentmapShader()
{
    return getBuiltinRhiShader(QByteArrayLiteral("environmentmap"), m_environmentmapShader);
}

QT_END_NAMESPACE
