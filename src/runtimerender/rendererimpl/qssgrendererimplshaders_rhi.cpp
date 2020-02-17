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

#include <QtQuick3DRuntimeRender/private/qssgrendererimpl_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderlight_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercontextcore_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercache_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderdynamicobjectsystem_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercodegeneratorv2_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderdefaultmaterialshadergenerator_p.h>
#include <QtQuick3DRuntimeRender/private/qssgvertexpipelineimpl_p.h>

// this file contains the getXxxxShader implementations suitable for the QRhi-based rendering path

QT_BEGIN_NAMESPACE

QSSGRef<QSSGRhiShaderStagesWithResources> QSSGRendererImpl::getRhiShader(const QByteArray &name,
                                                                         QSSGRef<QSSGRhiShaderStagesWithResources> &storage)
{
    QSSGRef<QSSGRhiShaderStagesWithResources> &result = storage;
    if (result.isNull()) {
        // loadBuiltin must always return a valid QSSGRhiShaderStages.
        // There will just be no stages if loading fails.
        result = QSSGRhiShaderStagesWithResources::fromShaderStages(getProgramGenerator()->loadBuiltinRhiShader(name),
                                                                    name);
    }
    return result;
}

QSSGRef<QSSGRhiShaderStagesWithResources> QSSGRendererImpl::getRhiCubemapShadowDepthShader()
{
    return getRhiShader(QByteArrayLiteral("cubeshadowdepth"), m_cubemapShadowDepthRhiShader);
}

QSSGRef<QSSGRhiShaderStagesWithResources> QSSGRendererImpl::getRhiOrthographicShadowDepthShader()
{
    return getRhiShader(QByteArrayLiteral("orthoshadowdepth"), m_orthographicShadowDepthRhiShader);
}

QSSGRef<QSSGRhiShaderStagesWithResources> QSSGRendererImpl::getRhiCubemapShadowBlurXShader()
{
    return getRhiShader(QByteArrayLiteral("cubeshadowblurx"), m_cubemapShadowBlurXRhiShader);
}

QSSGRef<QSSGRhiShaderStagesWithResources> QSSGRendererImpl::getRhiCubemapShadowBlurYShader()
{
    return getRhiShader(QByteArrayLiteral("cubeshadowblury"), m_cubemapShadowBlurYRhiShader);
}

QSSGRef<QSSGRhiShaderStagesWithResources> QSSGRendererImpl::getRhiOrthographicShadowBlurXShader()
{
    return getRhiShader(QByteArrayLiteral("orthoshadowblurx"), m_orthographicShadowBlurXRhiShader);
}

QSSGRef<QSSGRhiShaderStagesWithResources> QSSGRendererImpl::getRhiOrthographicShadowBlurYShader()
{
    return getRhiShader(QByteArrayLiteral("orthoshadowblury"), m_orthographicShadowBlurYRhiShader);
}

QSSGRef<QSSGRhiShaderStagesWithResources> QSSGRendererImpl::getRhiDepthPrepassShader()
{
    return getRhiShader(QByteArrayLiteral("depthprepass"), m_depthPrepassRhiShader);
}

QSSGRef<QSSGRhiShaderStagesWithResources> QSSGRendererImpl::getRhiSsaoShader()
{
    return getRhiShader(QByteArrayLiteral("ssao"), m_ssaoRhiShader);
}

QSSGRef<QSSGRhiShaderStagesWithResources> QSSGRendererImpl::getRhiSkyBoxShader()
{
    return getRhiShader(QByteArrayLiteral("skybox"), m_skyBoxRhiShader);
}

QSSGRef<QSSGRhiShaderStagesWithResources> QSSGRendererImpl::getRhiSupersampleResolveShader()
{
    return getRhiShader(QByteArrayLiteral("ssaaresolve"), m_supersampleResolveRhiShader);
}

QSSGRef<QSSGRhiShaderStagesWithResources> QSSGRendererImpl::getRhiFlippedSupersampleResolveShader()
{
    return getRhiShader(QByteArrayLiteral("ssaaresolve_flip"), m_flippedSupersampleResolveRhiShader);
}

QT_END_NAMESPACE
