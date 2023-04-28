// Copyright (C) 2008-2012 NVIDIA Corporation.
// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtQuick3DRuntimeRender/private/qssgrenderer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderlight_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercontextcore_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercache_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershaderlibrarymanager_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercodegenerator_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderdefaultmaterialshadergenerator_p.h>
#include <QtQuick3DRuntimeRender/private/qssgvertexpipelineimpl_p.h>

// this file contains the getXxxxShader implementations suitable for the QRhi-based rendering path

QT_BEGIN_NAMESPACE

QSSGRhiShaderPipelinePtr QSSGRenderer::getBuiltinRhiShader(const QByteArray &name,
                                                           QSSGRhiShaderPipelinePtr &storage)
{
    QSSGRhiShaderPipelinePtr &result = storage;
    if (!result) {
        // loadBuiltin must always return a valid QSSGRhiShaderPipeline.
        // There will just be no stages if loading fails.
        result = m_contextInterface->shaderCache()->loadBuiltinForRhi(name);
    }
    return result;
}

QSSGRhiShaderPipelinePtr QSSGRenderer::getRhiCubemapShadowBlurXShader()
{
    return getBuiltinRhiShader(QByteArrayLiteral("cubeshadowblurx"), m_cubemapShadowBlurXRhiShader);
}

QSSGRhiShaderPipelinePtr QSSGRenderer::getRhiCubemapShadowBlurYShader()
{
    return getBuiltinRhiShader(QByteArrayLiteral("cubeshadowblury"), m_cubemapShadowBlurYRhiShader);
}

QSSGRhiShaderPipelinePtr QSSGRenderer::getRhiGridShader()
{
    return getBuiltinRhiShader(QByteArrayLiteral("grid"), m_gridShader);
}

QSSGRhiShaderPipelinePtr QSSGRenderer::getRhiOrthographicShadowBlurXShader()
{
    return getBuiltinRhiShader(QByteArrayLiteral("orthoshadowblurx"), m_orthographicShadowBlurXRhiShader);
}

QSSGRhiShaderPipelinePtr QSSGRenderer::getRhiOrthographicShadowBlurYShader()
{
    return getBuiltinRhiShader(QByteArrayLiteral("orthoshadowblury"), m_orthographicShadowBlurYRhiShader);
}

QSSGRhiShaderPipelinePtr QSSGRenderer::getRhiSsaoShader()
{
    return getBuiltinRhiShader(QByteArrayLiteral("ssao"), m_ssaoRhiShader);
}

QSSGRhiShaderPipelinePtr QSSGRenderer::getRhiSkyBoxCubeShader()
{
    return getBuiltinRhiShader(QByteArrayLiteral("skyboxcube"), m_skyBoxCubeRhiShader);
}

QSSGRhiShaderPipelinePtr QSSGRenderer::getRhiSkyBoxShader(QSSGRenderLayer::TonemapMode tonemapMode, bool isRGBE)
{
    // Skybox shader is special and has multiple possible shaders so we have to do
    // a bit of manual work here.

    QSSGRhiShaderPipelinePtr &result = m_skyBoxRhiShader;
    if (!result || tonemapMode != m_skyboxTonemapMode || isRGBE != m_isSkyboxRGBE) {
        QByteArray name = QByteArrayLiteral("skybox");
        if (isRGBE)
            name.append(QByteArrayLiteral("_rgbe"));
        else
            name.append(QByteArrayLiteral("_hdr"));

        switch (tonemapMode) {
        case QSSGRenderLayer::TonemapMode::None:
            name.append(QByteArrayLiteral("_none"));
            break;
        case QSSGRenderLayer::TonemapMode::Aces:
            name.append(QByteArrayLiteral("_aces"));
            break;
        case QSSGRenderLayer::TonemapMode::HejlDawson:
            name.append(QByteArrayLiteral("_hejldawson"));
            break;
        case QSSGRenderLayer::TonemapMode::Filmic:
            name.append(QByteArrayLiteral("_filmic"));
            break;
        case QSSGRenderLayer::TonemapMode::Linear:
        default:
            name.append(QByteArrayLiteral("_linear"));
        }

        result = m_contextInterface->shaderCache()->loadBuiltinForRhi(name);
        m_skyboxTonemapMode = tonemapMode;
        m_isSkyboxRGBE = isRGBE;
    }
    return result;
}

QSSGRhiShaderPipelinePtr QSSGRenderer::getRhiSupersampleResolveShader()
{
    return getBuiltinRhiShader(QByteArrayLiteral("ssaaresolve"), m_supersampleResolveRhiShader);
}

QSSGRhiShaderPipelinePtr QSSGRenderer::getRhiProgressiveAAShader()
{
    return getBuiltinRhiShader(QByteArrayLiteral("progressiveaa"), m_progressiveAARhiShader);
}

QSSGRhiShaderPipelinePtr QSSGRenderer::getRhiTexturedQuadShader()
{
    return getBuiltinRhiShader(QByteArrayLiteral("texturedquad"), m_texturedQuadRhiShader);
}

QSSGRhiShaderPipelinePtr QSSGRenderer::getRhiParticleShader(QSSGRenderParticles::FeatureLevel featureLevel)
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

QSSGRhiShaderPipelinePtr QSSGRenderer::getRhiSimpleQuadShader()
{
    return getBuiltinRhiShader(QByteArrayLiteral("simplequad"), m_simpleQuadRhiShader);
}

QSSGRhiShaderPipelinePtr QSSGRenderer::getRhiLightmapUVRasterizationShader(LightmapUVRasterizationShaderMode mode)
{
    switch (mode) {
    case LightmapUVRasterizationShaderMode::Uv:
        return getBuiltinRhiShader(QByteArrayLiteral("lightmapuvraster_uv"), m_lightmapUVRasterShader_uv);
    case LightmapUVRasterizationShaderMode::UvTangent:
        return getBuiltinRhiShader(QByteArrayLiteral("lightmapuvraster_uv_tangent"), m_lightmapUVRasterShader_uv_tangent);
    default:
        break;
    }
    return getBuiltinRhiShader(QByteArrayLiteral("lightmapuvraster"), m_lightmapUVRasterShader);
}

QSSGRhiShaderPipelinePtr QSSGRenderer::getRhiLightmapDilateShader()
{
    return getBuiltinRhiShader(QByteArrayLiteral("lightmapdilate"), m_lightmapDilateShader);
}

QSSGRhiShaderPipelinePtr QSSGRenderer::getRhiDebugObjectShader()
{
    return getBuiltinRhiShader(QByteArrayLiteral("debugobject"), m_debugObjectShader);
}

QT_END_NAMESPACE
