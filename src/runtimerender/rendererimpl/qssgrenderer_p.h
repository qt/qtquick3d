// Copyright (C) 2008-2012 NVIDIA Corporation.
// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QSSG_RENDERER_H
#define QSSG_RENDERER_H

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

#include <QtQuick3DRuntimeRender/private/qssgrenderlayer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderdefaultmaterialshadergenerator_p.h> // TODO:
#include <QtQuick3DRuntimeRender/private/qssgrenderpickresult_p.h>
#include <QtQuick3DRuntimeRender/private/qssgshadermapkey_p.h>

QT_BEGIN_NAMESPACE

class QSSGRhiQuadRenderer;
class QSSGRhiCubeRenderer;
class QSSGBufferManager;
class QSSGShaderCache;
struct QSSGRenderItem2D;
struct QSSGReflectionMapEntry;
struct QSSGRenderRay;

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRenderer
{
    Q_DISABLE_COPY(QSSGRenderer)
    using PickResultList = QVarLengthArray<QSSGRenderPickResult, 20>; // Lets assume most items are filtered out already
public:
    QSSGRenderer();
    ~QSSGRenderer();

    QSSGShaderDefaultMaterialKeyProperties &defaultMaterialShaderKeyProperties()
    {
        return m_defaultMaterialShaderKeyProperties;
    }

    void setRenderContextInterface(QSSGRenderContextInterface *ctx);

    // Returns true if this layer or a sibling was dirty.
    bool prepareLayerForRender(QSSGRenderLayer &inLayer);

    void rhiPrepare(QSSGRenderLayer &inLayer);
    void rhiRender(QSSGRenderLayer &inLayer);

    void cleanupResources(QList<QSSGRenderGraphObject*> &resources);
    void cleanupResources(QSet<QSSGRenderGraphObject*> &resources);

    QSSGLayerRenderData *getOrCreateLayerRenderData(QSSGRenderLayer &layer);

    // The QSSGRenderContextInterface calls these, clients should not.
    void beginFrame(QSSGRenderLayer *layer);
    void endFrame(QSSGRenderLayer *layer);

    PickResultList syncPickAll(const QSSGRenderLayer &layer,
                               QSSGBufferManager &bufferManager,
                               const QSSGRenderRay &ray);

    QSSGRenderPickResult syncPick(const QSSGRenderLayer &layer,
                                  QSSGBufferManager &bufferManager,
                                  const QSSGRenderRay &ray,
                                  QSSGRenderNode *target = nullptr);

    // Setting this true enables picking for all the models, regardless of
    // the models pickable property.
    bool isGlobalPickingEnabled() const;
    void setGlobalPickingEnabled(bool isEnabled);

    QSSGRhiQuadRenderer *rhiQuadRenderer();
    QSSGRhiCubeRenderer *rhiCubeRenderer();

    void beginLayerRender(QSSGLayerRenderData &inLayer);
    void endLayerRender();
    void addMaterialDirtyClear(QSSGRenderGraphObject *material);

    static QSSGRhiShaderPipelinePtr generateRhiShaderPipelineImpl(QSSGSubsetRenderable &renderable, QSSGShaderLibraryManager &shaderLibraryManager,
                                                                  QSSGShaderCache &shaderCache,
                                                                  QSSGProgramGenerator &shaderProgramGenerator,
                                                                  QSSGShaderDefaultMaterialKeyProperties &shaderKeyProperties,
                                                                  const QSSGShaderFeatures &featureSet,
                                                                  QByteArray &shaderString);

    QSSGRhiShaderPipelinePtr getShaderPipelineForDefaultMaterial(QSSGSubsetRenderable &inRenderable,
                                                                 const QSSGShaderFeatures &inFeatureSet);

    QSSGLayerGlobalRenderProperties getLayerGlobalRenderProperties();

    QSSGRenderContextInterface *contextInterface() const { return m_contextInterface; }

    enum class LightmapUVRasterizationShaderMode {
        Default,
        Uv,
        UvTangent
    };

    // shader implementations, RHI, implemented in qssgrendererimplshaders_rhi.cpp
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

    static void setTonemapFeatures(QSSGShaderFeatures &features, QSSGRenderLayer::TonemapMode tonemapMode);

protected:
    static void getLayerHitObjectList(const QSSGRenderLayer &layer,
                                      QSSGBufferManager &bufferManager,
                                      const QSSGRenderRay &ray,
                                      bool inPickEverything,
                                      PickResultList &outIntersectionResult);
    static void intersectRayWithSubsetRenderable(QSSGBufferManager &bufferManager,
                                                 const QSSGRenderRay &inRay,
                                                 const QSSGRenderNode &node,
                                                 PickResultList &outIntersectionResultList);
    static void intersectRayWithItem2D(const QSSGRenderRay &inRay, const QSSGRenderItem2D &item2D, PickResultList &outIntersectionResultList);

private:
    friend class QSSGRenderContextInterface;
    friend class QSSGLayerRenderData;

    void releaseCachedResources();
    QSSGRhiShaderPipelinePtr getBuiltinRhiShader(const QByteArray &name,
                                                 QSSGRhiShaderPipelinePtr &storage);
    QSSGRhiShaderPipelinePtr generateRhiShaderPipeline(QSSGSubsetRenderable &inRenderable, const QSSGShaderFeatures &inFeatureSet);

    QSSGRenderContextInterface *m_contextInterface = nullptr; //  We're own by the context interface

    // The shader refs are non-null if we have attempted to generate the
    // shader. This does not mean we were successul, however.

    // RHI
    QSSGRhiShaderPipelinePtr m_cubemapShadowBlurXRhiShader;
    QSSGRhiShaderPipelinePtr m_cubemapShadowBlurYRhiShader;
    QSSGRhiShaderPipelinePtr m_gridShader;
    QSSGRhiShaderPipelinePtr m_orthographicShadowBlurXRhiShader;
    QSSGRhiShaderPipelinePtr m_orthographicShadowBlurYRhiShader;
    QSSGRhiShaderPipelinePtr m_ssaoRhiShader;
    QSSGRhiShaderPipelinePtr m_skyBoxRhiShader;
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

    bool m_globalPickingEnabled = false;

    // Temporary information stored only when rendering a particular layer.
    QSSGLayerRenderData *m_currentLayer = nullptr;
    QMatrix4x4 m_viewProjection;
    QByteArray m_generatedShaderString;

    QSSGShaderDefaultMaterialKeyProperties m_defaultMaterialShaderKeyProperties;

    QSet<QSSGRenderGraphObject *> m_materialClearDirty;

    QSSGRhiQuadRenderer *m_rhiQuadRenderer = nullptr;
    QSSGRhiCubeRenderer *m_rhiCubeRenderer = nullptr;

    QHash<QSSGShaderMapKey, QSSGRhiShaderPipelinePtr> m_shaderMap;

    // Skybox shader state
    QSSGRenderLayer::TonemapMode m_skyboxTonemapMode = QSSGRenderLayer::TonemapMode::None;
    bool m_isSkyboxRGBE = false;
};

QT_END_NAMESPACE

#endif
