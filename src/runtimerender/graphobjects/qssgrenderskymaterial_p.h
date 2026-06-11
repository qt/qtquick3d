// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QSSG_RENDER_SKY_MATERIAL_H
#define QSSG_RENDER_SKY_MATERIAL_H

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

#include <QtQuick3DRuntimeRender/private/qssgrendergraphobject_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderhelpers_p.h>
#include <QtQuick3DRuntimeRender/private/qssgshadermaterialadapter_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercache_p.h>

QT_BEGIN_NAMESPACE

class QSSGRhiEffectSystem;

struct Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRenderSkyMaterial : public QSSGRenderGraphObject
{
    // Background source: Cubemap samples the IBL radiance cube; ScreenSpace* evaluate the
    // sky shader directly on screen at the given scale. See the SkyMaterial QML docs.
    enum class SkyboxMode { Cubemap, ScreenSpaceFull, ScreenSpaceHalf, ScreenSpaceQuarter };

    int radianceMapSize = 512;
    int iblSampleCount = 16;
    int iblSamplesPerFrame = 0;
    int iblRenderFrames = 0;
    SkyboxMode skyboxMode = SkyboxMode::Cubemap;
    bool wantsMoreFrames = false;
    bool enableIBL = false;

    QSSGRenderSkyMaterial();
    ~QSSGRenderSkyMaterial();

    QList<QSSGBaseTypeProperty> propertyUniforms;

    // Cube-face pipeline used to render the procedural sky into the IBL environment cube.
    QSSGRhiShaderPipelinePtr iblPassPipeline;
    // Screen-space pipeline used to evaluate the sky shader directly into the visible
    // background (fullscreen quad), decoupled from the IBL cube.
    QSSGRhiShaderPipelinePtr backgroundPipeline;

    QSSGRhiShaderPipelinePtr ensurePipeline(const QSSGRenderContextInterface &sgContext);
    QSSGRhiShaderPipelinePtr ensureBackgroundPipeline(const QSSGRenderContextInterface &sgContext,
                                                      const QSSGShaderFeatures &tonemapFeatures,
                                                      quint32 tonemapKey,
                                                      int viewCount);

    // returns uniform stride per face
    quint32 updateUniforms(const QSSGRenderContextInterface &sgContext, const QMatrix4x4 &mvp, const QVarLengthArray<QMatrix4x4, 6> views);
    void updateBackgroundUniforms(const QSSGRenderContextInterface &sgContext,
                                  const QVarLengthArray<QMatrix4x4, 2> &inverseProjections,
                                  const QVarLengthArray<QMatrix4x4, 2> &viewRotations,
                                  float adjustY,
                                  float exposure);

    QByteArray fragmentShaderSource;

    QByteArray shaderPathKey = "sky material --";

    bool isDirty = true;
    bool isFragmentShaderDirty = true;
    bool isBackgroundShaderDirty = true;

    QSSGRhiShaderResourceBindingList bindings;
    QSSGRhiShaderResourceBindingList backgroundBindings;

private:
    QSSGRhiShaderPipelinePtr buildPipeline(const QSSGRenderContextInterface &sgContext,
                                           QByteArray vertexShader,
                                           const QSSGShaderCustomMaterialAdapter::StringPairList &vertexViewDependentUniforms,
                                           const QSSGShaderCustomMaterialAdapter::StringPairList &vertexUniforms,
                                           const QByteArray &fragmentMainSnippet,
                                           const QSSGShaderCustomMaterialAdapter::StringPairList &fragmentUniforms,
                                           const QSSGShaderFeatures &features,
                                           int viewCount,
                                           const QByteArray &cacheKeyTag);

    quint32 m_backgroundTonemapKey = quint32(-1);
    int m_backgroundViewCount = 0;
};

QT_END_NAMESPACE

#endif // QSSG_RENDER_SKY_MATERIAL_H
