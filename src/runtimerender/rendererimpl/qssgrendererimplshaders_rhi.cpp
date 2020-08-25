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

QSSGRef<QSSGRhiShaderStagesWithResources> QSSGRenderer::getRhiShader(const QByteArray &name,
                                                                         QSSGRef<QSSGRhiShaderStagesWithResources> &storage)
{
    QSSGRef<QSSGRhiShaderStagesWithResources> &result = storage;
    if (result.isNull()) {
        // loadBuiltin must always return a valid QSSGRhiShaderStages.
        // There will just be no stages if loading fails.
        result = QSSGRhiShaderStagesWithResources::fromShaderStages(m_contextInterface->shaderCache()->loadBuiltinForRhi(name));
    }
    return result;
}

QSSGRef<QSSGRhiShaderStagesWithResources> QSSGRenderer::getRhiCubemapShadowBlurXShader()
{
    return getRhiShader(QByteArrayLiteral("cubeshadowblurx"), m_cubemapShadowBlurXRhiShader);
}

QSSGRef<QSSGRhiShaderStagesWithResources> QSSGRenderer::getRhiCubemapShadowBlurYShader()
{
    return getRhiShader(QByteArrayLiteral("cubeshadowblury"), m_cubemapShadowBlurYRhiShader);
}

QSSGRef<QSSGRhiShaderStagesWithResources> QSSGRenderer::getRhiOrthographicShadowBlurXShader()
{
    return getRhiShader(QByteArrayLiteral("orthoshadowblurx"), m_orthographicShadowBlurXRhiShader);
}

QSSGRef<QSSGRhiShaderStagesWithResources> QSSGRenderer::getRhiOrthographicShadowBlurYShader()
{
    return getRhiShader(QByteArrayLiteral("orthoshadowblury"), m_orthographicShadowBlurYRhiShader);
}

QSSGRef<QSSGRhiShaderStagesWithResources> QSSGRenderer::getRhiSsaoShader()
{
    return getRhiShader(QByteArrayLiteral("ssao"), m_ssaoRhiShader);
}

QSSGRef<QSSGRhiShaderStagesWithResources> QSSGRenderer::getRhiSkyBoxShader(QSSGRenderLayer::TonemapMode tonemapMode, bool isRGBE)
{
    // Skybox shader is special and has multiple possible shaders so we have to do
    // a bit of manual work here.

    QSSGRef<QSSGRhiShaderStagesWithResources> &result = m_skyBoxRhiShader;
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
        case QSSGRenderLayer::TonemapMode::HejlRichard:
            name.append(QByteArrayLiteral("_hejlrichard"));
            break;
        case QSSGRenderLayer::TonemapMode::Filmic:
            name.append(QByteArrayLiteral("_filmic"));
            break;
        case QSSGRenderLayer::TonemapMode::Linear:
        default:
            name.append(QByteArrayLiteral("_linear"));
        }

        result = QSSGRhiShaderStagesWithResources::fromShaderStages(m_contextInterface->shaderCache()->loadBuiltinForRhi(name));
    }
    return result;
}

QSSGRef<QSSGRhiShaderStagesWithResources> QSSGRenderer::getRhiSupersampleResolveShader()
{
    return getRhiShader(QByteArrayLiteral("ssaaresolve"), m_supersampleResolveRhiShader);
}

QSSGRef<QSSGRhiShaderStagesWithResources> QSSGRenderer::getRhiProgressiveAAShader()
{
    return getRhiShader(QByteArrayLiteral("progressiveaa"), m_progressiveAARhiShader);
}

QSSGRef<QSSGRhiShaderStagesWithResources> QSSGRenderer::getRhiTexturedQuadShader()
{
    return getRhiShader(QByteArrayLiteral("texturedquad"), m_texturedQuadRhiShader);
}

QT_END_NAMESPACE
