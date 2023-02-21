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

#include <QtQuick3DRuntimeRender/private/qssgrenderableobjects_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendermesh_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendermodel_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderdefaultmaterial_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderlayer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderray_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercamera_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercache_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderclippingfrustum_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershaderkeys_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercache_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderdefaultmaterialshadergenerator_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderbuffermanager_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderpickresult_p.h>
#include <QtQuick3DRuntimeRender/private/qssgshadermapkey_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderpass_p.h>

#include <QtQuick3DUtils/private/qssgbounds3_p.h>
#include <QtQuick3DUtils/private/qssgdataref_p.h>

QT_BEGIN_NAMESPACE

class QSSGRhiQuadRenderer;
class QSSGRhiCubeRenderer;
struct QSSGRenderItem2D;
struct QSSGReflectionMapEntry;

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRenderer
{
    using PickResultList = QVarLengthArray<QSSGRenderPickResult, 20>; // Lets assume most items are filtered out already

public:
    QAtomicInt ref;
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
                               const QSSGRef<QSSGBufferManager> &bufferManager,
                               const QSSGRenderRay &ray);

    QSSGRenderPickResult syncPick(const QSSGRenderLayer &layer,
                                  const QSSGRef<QSSGBufferManager> &bufferManager,
                                  const QSSGRenderRay &ray,
                                  QSSGRenderNode *target = nullptr);

    // Setting this true enables picking for all the models, regardless of
    // the models pickable property.
    bool isGlobalPickingEnabled() const;
    void setGlobalPickingEnabled(bool isEnabled);

    QSSGRhiQuadRenderer *rhiQuadRenderer();
    QSSGRhiCubeRenderer *rhiCubeRenderer();

    // Callback during the layer render process.
    void beginLayerRender(QSSGLayerRenderData &inLayer);
    void endLayerRender();
    void addMaterialDirtyClear(QSSGRenderGraphObject *material);

    static QSSGRef<QSSGRhiShaderPipeline> generateRhiShaderPipelineImpl(QSSGSubsetRenderable &renderable, const QSSGRef<QSSGShaderLibraryManager> &shaderLibraryManager,
                                                                        const QSSGRef<QSSGShaderCache> &shaderCache,
                                                                        const QSSGRef<QSSGProgramGenerator> &shaderProgramGenerator,
                                                                        QSSGShaderDefaultMaterialKeyProperties &shaderKeyProperties,
                                                                        const QSSGShaderFeatures &featureSet,
                                                                        QByteArray &shaderString);

    QSSGRef<QSSGRhiShaderPipeline> getShaderPipelineForDefaultMaterial(QSSGSubsetRenderable &inRenderable,
                                                                       const QSSGShaderFeatures &inFeatureSet);

    QSSGLayerGlobalRenderProperties getLayerGlobalRenderProperties();

    QSSGRenderContextInterface *contextInterface() { return m_contextInterface; }

    // Returns true if the renderer expects new frame to be rendered
    // Happens when progressive AA is enabled
    bool rendererRequestsFrames() const;

    enum class LightmapUVRasterizationShaderMode {
        Default,
        Uv,
        UvTangent
    };

    // shader implementations, RHI, implemented in qssgrendererimplshaders_rhi.cpp
    QSSGRef<QSSGRhiShaderPipeline> getRhiCubemapShadowBlurXShader();
    QSSGRef<QSSGRhiShaderPipeline> getRhiCubemapShadowBlurYShader();
    QSSGRef<QSSGRhiShaderPipeline> getRhiGridShader();
    QSSGRef<QSSGRhiShaderPipeline> getRhiOrthographicShadowBlurXShader();
    QSSGRef<QSSGRhiShaderPipeline> getRhiOrthographicShadowBlurYShader();
    QSSGRef<QSSGRhiShaderPipeline> getRhiSsaoShader();
    QSSGRef<QSSGRhiShaderPipeline> getRhiSkyBoxCubeShader();
    QSSGRef<QSSGRhiShaderPipeline> getRhiSkyBoxShader(QSSGRenderLayer::TonemapMode tonemapMode, bool isRGBE);
    QSSGRef<QSSGRhiShaderPipeline> getRhiSupersampleResolveShader();
    QSSGRef<QSSGRhiShaderPipeline> getRhiProgressiveAAShader();
    QSSGRef<QSSGRhiShaderPipeline> getRhiTexturedQuadShader();
    QSSGRef<QSSGRhiShaderPipeline> getRhiParticleShader(QSSGRenderParticles::FeatureLevel featureLevel);
    QSSGRef<QSSGRhiShaderPipeline> getRhiSimpleQuadShader();
    QSSGRef<QSSGRhiShaderPipeline> getRhiLightmapUVRasterizationShader(LightmapUVRasterizationShaderMode mode);
    QSSGRef<QSSGRhiShaderPipeline> getRhiLightmapDilateShader();
    QSSGRef<QSSGRhiShaderPipeline> getRhiDebugObjectShader();

    static void setTonemapFeatures(QSSGShaderFeatures &features, QSSGRenderLayer::TonemapMode tonemapMode);

protected:
    static void getLayerHitObjectList(const QSSGRenderLayer &layer,
                                      const QSSGRef<QSSGBufferManager> &bufferManager,
                                      const QSSGRenderRay &ray,
                                      bool inPickEverything,
                                      PickResultList &outIntersectionResult);
    static void intersectRayWithSubsetRenderable(const QSSGRef<QSSGBufferManager> &bufferManager,
                                                 const QSSGRenderRay &inRay,
                                                 const QSSGRenderNode &node,
                                                 PickResultList &outIntersectionResultList);
    static void intersectRayWithItem2D(const QSSGRenderRay &inRay, const QSSGRenderItem2D &item2D, PickResultList &outIntersectionResultList);

private:
    friend class QSSGRenderContextInterface;
    void releaseCachedResources();
    QSSGRef<QSSGRhiShaderPipeline> getBuiltinRhiShader(const QByteArray &name,
                                                       QSSGRef<QSSGRhiShaderPipeline> &storage);
    QSSGRef<QSSGRhiShaderPipeline> generateRhiShaderPipeline(QSSGSubsetRenderable &inRenderable, const QSSGShaderFeatures &inFeatureSet);

    QSSGRenderContextInterface *m_contextInterface = nullptr; //  We're own by the context interface

    // The shader refs are non-null if we have attempted to generate the
    // shader. This does not mean we were successul, however.

    // RHI
    QSSGRef<QSSGRhiShaderPipeline> m_cubemapShadowBlurXRhiShader;
    QSSGRef<QSSGRhiShaderPipeline> m_cubemapShadowBlurYRhiShader;
    QSSGRef<QSSGRhiShaderPipeline> m_gridShader;
    QSSGRef<QSSGRhiShaderPipeline> m_orthographicShadowBlurXRhiShader;
    QSSGRef<QSSGRhiShaderPipeline> m_orthographicShadowBlurYRhiShader;
    QSSGRef<QSSGRhiShaderPipeline> m_ssaoRhiShader;
    QSSGRef<QSSGRhiShaderPipeline> m_skyBoxRhiShader;
    QSSGRef<QSSGRhiShaderPipeline> m_skyBoxCubeRhiShader;
    QSSGRef<QSSGRhiShaderPipeline> m_supersampleResolveRhiShader;
    QSSGRef<QSSGRhiShaderPipeline> m_progressiveAARhiShader;
    QSSGRef<QSSGRhiShaderPipeline> m_texturedQuadRhiShader;
    QSSGRef<QSSGRhiShaderPipeline> m_simpleQuadRhiShader;
    QSSGRef<QSSGRhiShaderPipeline> m_lightmapUVRasterShader;
    QSSGRef<QSSGRhiShaderPipeline> m_lightmapUVRasterShader_uv;
    QSSGRef<QSSGRhiShaderPipeline> m_lightmapUVRasterShader_uv_tangent;
    QSSGRef<QSSGRhiShaderPipeline> m_lightmapDilateShader;
    QSSGRef<QSSGRhiShaderPipeline> m_debugObjectShader;

    QSSGRef<QSSGRhiShaderPipeline> m_particlesNoLightingSimpleRhiShader;
    QSSGRef<QSSGRhiShaderPipeline> m_particlesNoLightingMappedRhiShader;
    QSSGRef<QSSGRhiShaderPipeline> m_particlesNoLightingAnimatedRhiShader;
    QSSGRef<QSSGRhiShaderPipeline> m_particlesVLightingSimpleRhiShader;
    QSSGRef<QSSGRhiShaderPipeline> m_particlesVLightingMappedRhiShader;
    QSSGRef<QSSGRhiShaderPipeline> m_particlesVLightingAnimatedRhiShader;
    QSSGRef<QSSGRhiShaderPipeline> m_lineParticlesRhiShader;
    QSSGRef<QSSGRhiShaderPipeline> m_lineParticlesMappedRhiShader;
    QSSGRef<QSSGRhiShaderPipeline> m_lineParticlesAnimatedRhiShader;
    QSSGRef<QSSGRhiShaderPipeline> m_lineParticlesVLightRhiShader;
    QSSGRef<QSSGRhiShaderPipeline> m_lineParticlesMappedVLightRhiShader;
    QSSGRef<QSSGRhiShaderPipeline> m_lineParticlesAnimatedVLightRhiShader;

    bool m_globalPickingEnabled = false;

    // Temporary information stored only when rendering a particular layer.
    QSSGLayerRenderData *m_currentLayer = nullptr;
    QMatrix4x4 m_viewProjection;
    QByteArray m_generatedShaderString;

    bool m_progressiveAARenderRequest = false;
    QSSGShaderDefaultMaterialKeyProperties m_defaultMaterialShaderKeyProperties;

    QSet<QSSGRenderGraphObject *> m_materialClearDirty;

    QSSGRhiQuadRenderer *m_rhiQuadRenderer = nullptr;
    QSSGRhiCubeRenderer *m_rhiCubeRenderer = nullptr;

    QHash<QSSGShaderMapKey, QSSGRef<QSSGRhiShaderPipeline>> m_shaderMap;

    // Skybox shader state
    QSSGRenderLayer::TonemapMode m_skyboxTonemapMode = QSSGRenderLayer::TonemapMode::None;
    bool m_isSkyboxRGBE = false;
};

namespace RenderHelpers
{

std::pair<QSSGBoxPoints, QSSGBoxPoints> calculateSortedObjectBounds(const QVector<QSSGRenderableObjectHandle> &sortedOpaqueObjects,
                                                                    const QVector<QSSGRenderableObjectHandle> &sortedTransparentObjects);

void rhiRenderShadowMap(QSSGRhiContext *rhiCtx, QSSGPassKey passKey,
                        QSSGRhiGraphicsPipelineState &ps,
                        const QSSGRef<QSSGRenderShadowMap> &shadowMapManager,
                        const QSSGRenderCamera &camera,
                        const QSSGShaderLightList &globalLights,
                        const QVector<QSSGRenderableObjectHandle> &sortedOpaqueObjects,
                        const QSSGRef<QSSGRenderer> &renderer,
                        const QSSGBoxPoints &castingObjectsBox,
                        const QSSGBoxPoints &receivingObjectsBox);

void rhiRenderReflectionMap(QSSGRhiContext *rhiCtx,
                            QSSGPassKey passKey,
                            const QSSGLayerRenderData &inData, QSSGRhiGraphicsPipelineState *ps,
                            const QSSGRef<QSSGRenderReflectionMap> &reflectionMapManager,
                            const QVector<QSSGRenderReflectionProbe *> &reflectionProbes,
                            const QVector<QSSGRenderableObjectHandle> &reflectionPassObjects,
                            const QSSGRef<QSSGRenderer> &renderer);

bool rhiPrepareDepthPass(QSSGRhiContext *rhiCtx, QSSGPassKey passKey,
                         const QSSGRhiGraphicsPipelineState &basePipelineState,
                         QRhiRenderPassDescriptor *rpDesc,
                         QSSGLayerRenderData &inData,
                         const QVector<QSSGRenderableObjectHandle> &sortedOpaqueObjects,
                         const QVector<QSSGRenderableObjectHandle> &sortedTransparentObjects,
                         QSSGRhiDrawCallDataKey::Selector ubufSel,
                         int samples);

void rhiRenderDepthPass(QSSGRhiContext *rhiCtx, const QSSGRhiGraphicsPipelineState &ps,
                        const QVector<QSSGRenderableObjectHandle> &sortedOpaqueObjects,
                        const QVector<QSSGRenderableObjectHandle> &sortedTransparentObjects,
                        bool *needsSetViewport);

bool rhiPrepareAoTexture(QSSGRhiContext *rhiCtx, const QSize &size, QSSGRhiRenderableTexture *renderableTex);

void rhiRenderAoTexture(QSSGRhiContext *rhiCtx, QSSGPassKey passKey, const QSSGRef<QSSGRenderer> &renderer, const QSSGRef<QSSGRhiShaderPipeline> &shaderPipeline,
                        QSSGRhiGraphicsPipelineState &ps, const SSAOMapPass::AmbientOcclusion &ao, const QSSGRhiRenderableTexture &rhiAoTexture, const QSSGRhiRenderableTexture &rhiDepthTexture,
                        const QSSGRenderCamera &camera);

bool rhiPrepareScreenTexture(QSSGRhiContext *rhiCtx, const QSize &size, bool wantsMips, QSSGRhiRenderableTexture *renderableTex);

void rhiPrepareGrid(QSSGRhiContext *rhiCtx, QSSGRenderLayer &layer,
                    QSSGRenderCamera &inCamera, const QSSGRef<QSSGRenderer> &renderer);


void rhiPrepareSkyBox(QSSGRhiContext *rhiCtx, QSSGPassKey passKey,
                      QSSGRenderLayer &layer,
                      QSSGRenderCamera &inCamera,
                      const QSSGRef<QSSGRenderer> &renderer);

void rhiPrepareSkyBoxForReflectionMap(QSSGRhiContext *rhiCtx, QSSGPassKey passKey,
                                      QSSGRenderLayer &layer,
                                      QSSGRenderCamera &inCamera,
                                      const QSSGRef<QSSGRenderer> &renderer,
                                      QSSGReflectionMapEntry *entry,
                                      int cubeFace);

void rhiPrepareRenderable(QSSGRhiContext *rhiCtx, QSSGPassKey passKey,
                          const QSSGLayerRenderData &inData,
                          QSSGRenderableObject &inObject,
                          QRhiRenderPassDescriptor *renderPassDescriptor,
                          QSSGRhiGraphicsPipelineState *ps,
                          QSSGShaderFeatures featureSet,
                          int samples,
                          QSSGRenderCamera *inCamera = nullptr,
                          QMatrix4x4 *alteredModelViewProjection = nullptr,
                          int cubeFace = -1,
                          QSSGReflectionMapEntry *entry = nullptr);

void rhiRenderRenderable(QSSGRhiContext *rhiCtx,
                         const QSSGRhiGraphicsPipelineState &state,
                         QSSGRenderableObject &object,
                         bool *needsSetViewport,
                         int cubeFace = -1);

bool rhiPrepareDepthTexture(QSSGRhiContext *rhiCtx, const QSize &size, QSSGRhiRenderableTexture *renderableTex);

inline QRect correctViewportCoordinates(const QRectF &layerViewport, const QRect &deviceRect)
{
    const int y = deviceRect.bottom() - layerViewport.bottom() + 1;
    return QRect(layerViewport.x(), y, layerViewport.width(), layerViewport.height());
}
}

QT_END_NAMESPACE

#endif
