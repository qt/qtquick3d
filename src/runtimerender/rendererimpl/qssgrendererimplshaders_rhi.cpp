/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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

QSSGRef<QSSGRhiShaderPipeline> QSSGRenderer::getBuiltinRhiShader(const QByteArray &name,
                                                                 QSSGRef<QSSGRhiShaderPipeline> &storage)
{
    QSSGRef<QSSGRhiShaderPipeline> &result = storage;
    if (result.isNull()) {
        // loadBuiltin must always return a valid QSSGRhiShaderPipeline.
        // There will just be no stages if loading fails.
        result = m_contextInterface->shaderCache()->loadBuiltinForRhi(name);
    }
    return result;
}

QSSGRef<QSSGRhiShaderPipeline> QSSGRenderer::getRhiCubemapShadowBlurXShader()
{
    return getBuiltinRhiShader(QByteArrayLiteral("cubeshadowblurx"), m_cubemapShadowBlurXRhiShader);
}

QSSGRef<QSSGRhiShaderPipeline> QSSGRenderer::getRhiCubemapShadowBlurYShader()
{
    return getBuiltinRhiShader(QByteArrayLiteral("cubeshadowblury"), m_cubemapShadowBlurYRhiShader);
}

QSSGRef<QSSGRhiShaderPipeline> QSSGRenderer::getRhiOrthographicShadowBlurXShader()
{
    return getBuiltinRhiShader(QByteArrayLiteral("orthoshadowblurx"), m_orthographicShadowBlurXRhiShader);
}

QSSGRef<QSSGRhiShaderPipeline> QSSGRenderer::getRhiOrthographicShadowBlurYShader()
{
    return getBuiltinRhiShader(QByteArrayLiteral("orthoshadowblury"), m_orthographicShadowBlurYRhiShader);
}

QSSGRef<QSSGRhiShaderPipeline> QSSGRenderer::getRhiSsaoShader()
{
    return getBuiltinRhiShader(QByteArrayLiteral("ssao"), m_ssaoRhiShader);
}

QSSGRef<QSSGRhiShaderPipeline> QSSGRenderer::getRhiSkyBoxShader(QSSGRenderLayer::TonemapMode tonemapMode, bool isRGBE)
{
    // Skybox shader is special and has multiple possible shaders so we have to do
    // a bit of manual work here.

    QSSGRef<QSSGRhiShaderPipeline> &result = m_skyBoxRhiShader;
    if (result.isNull() || tonemapMode != m_skyboxTonemapMode || isRGBE != m_isSkyboxRGBE) {
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
    }
    return result;
}

QSSGRef<QSSGRhiShaderPipeline> QSSGRenderer::getRhiSupersampleResolveShader()
{
    return getBuiltinRhiShader(QByteArrayLiteral("ssaaresolve"), m_supersampleResolveRhiShader);
}

QSSGRef<QSSGRhiShaderPipeline> QSSGRenderer::getRhiProgressiveAAShader()
{
    return getBuiltinRhiShader(QByteArrayLiteral("progressiveaa"), m_progressiveAARhiShader);
}

QSSGRef<QSSGRhiShaderPipeline> QSSGRenderer::getRhiTexturedQuadShader()
{
    return getBuiltinRhiShader(QByteArrayLiteral("texturedquad"), m_texturedQuadRhiShader);
}

QSSGRef<QSSGRhiShaderPipeline> QSSGRenderer::getRhiParticleShader(QSSGRenderParticles::FeatureLevel featureLevel)
{
    switch (featureLevel) {
    case QSSGRenderParticles::FeatureLevel::Simple:
        return getBuiltinRhiShader(QByteArrayLiteral("particlesnolightsimple"), m_particlesNoLightingSimpleRhiShader);
        break;
    case QSSGRenderParticles::FeatureLevel::Mapped:
        return getBuiltinRhiShader(QByteArrayLiteral("particlesnolightmapped"), m_particlesNoLightingMappedRhiShader);
        break;
    case QSSGRenderParticles::FeatureLevel::Animated:
    default:
        return getBuiltinRhiShader(QByteArrayLiteral("particlesnolightanimated"), m_particlesNoLightingAnimatedRhiShader);
        break;
    }
}

QT_END_NAMESPACE
