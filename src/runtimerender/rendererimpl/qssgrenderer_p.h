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

namespace RenderHelpers
{

std::pair<QSSGBoxPoints, QSSGBoxPoints> calculateSortedObjectBounds(const QVector<QSSGRenderableObjectHandle> &sortedOpaqueObjects,
                                                                    const QVector<QSSGRenderableObjectHandle> &sortedTransparentObjects);

void rhiRenderShadowMap(QSSGRhiContext *rhiCtx, QSSGPassKey passKey,
                        QSSGRhiGraphicsPipelineState &ps,
                        QSSGRenderShadowMap &shadowMapManager,
                        const QSSGRenderCamera &camera,
                        const QSSGShaderLightList &globalLights,
                        const QVector<QSSGRenderableObjectHandle> &sortedOpaqueObjects,
                        QSSGRenderer &renderer,
                        const QSSGBoxPoints &castingObjectsBox,
                        const QSSGBoxPoints &receivingObjectsBox);

void rhiRenderReflectionMap(QSSGRhiContext *rhiCtx,
                            QSSGPassKey passKey,
                            const QSSGLayerRenderData &inData, QSSGRhiGraphicsPipelineState *ps,
                            QSSGRenderReflectionMap &reflectionMapManager,
                            const QVector<QSSGRenderReflectionProbe *> &reflectionProbes,
                            const QVector<QSSGRenderableObjectHandle> &reflectionPassObjects,
                            QSSGRenderer &renderer);

bool rhiPrepareDepthPass(QSSGRhiContext *rhiCtx, QSSGPassKey passKey,
                         const QSSGRhiGraphicsPipelineState &basePipelineState,
                         QRhiRenderPassDescriptor *rpDesc,
                         QSSGLayerRenderData &inData,
                         const QVector<QSSGRenderableObjectHandle> &sortedOpaqueObjects,
                         const QVector<QSSGRenderableObjectHandle> &sortedTransparentObjects,
                         int samples);

void rhiRenderDepthPass(QSSGRhiContext *rhiCtx, const QSSGRhiGraphicsPipelineState &ps,
                        const QVector<QSSGRenderableObjectHandle> &sortedOpaqueObjects,
                        const QVector<QSSGRenderableObjectHandle> &sortedTransparentObjects,
                        bool *needsSetViewport);

bool rhiPrepareAoTexture(QSSGRhiContext *rhiCtx, const QSize &size, QSSGRhiRenderableTexture *renderableTex);

void rhiRenderAoTexture(QSSGRhiContext *rhiCtx, QSSGPassKey passKey, QSSGRenderer &renderer, QSSGRhiShaderPipeline &shaderPipeline,
                        QSSGRhiGraphicsPipelineState &ps, const SSAOMapPass::AmbientOcclusion &ao, const QSSGRhiRenderableTexture &rhiAoTexture, const QSSGRhiRenderableTexture &rhiDepthTexture,
                        const QSSGRenderCamera &camera);

bool rhiPrepareScreenTexture(QSSGRhiContext *rhiCtx, const QSize &size, bool wantsMips, QSSGRhiRenderableTexture *renderableTex);

void rhiPrepareGrid(QSSGRhiContext *rhiCtx, QSSGPassKey passKey, QSSGRenderLayer &layer,
                    QSSGRenderCamera &inCamera, QSSGRenderer &renderer);


void rhiPrepareSkyBox(QSSGRhiContext *rhiCtx, QSSGPassKey passKey,
                      QSSGRenderLayer &layer,
                      QSSGRenderCamera &inCamera,
                      QSSGRenderer &renderer);

void rhiPrepareSkyBoxForReflectionMap(QSSGRhiContext *rhiCtx, QSSGPassKey passKey,
                                      QSSGRenderLayer &layer,
                                      QSSGRenderCamera &inCamera,
                                      QSSGRenderer &renderer,
                                      QSSGReflectionMapEntry *entry,
                                      QSSGRenderTextureCubeFace cubeFace);

Q_QUICK3DRUNTIMERENDER_EXPORT void rhiPrepareRenderable(QSSGRhiContext *rhiCtx, QSSGPassKey passKey,
                                                       const QSSGLayerRenderData &inData,
                                                       QSSGRenderableObject &inObject,
                                                       QRhiRenderPassDescriptor *renderPassDescriptor,
                                                       QSSGRhiGraphicsPipelineState *ps,
                                                       QSSGShaderFeatures featureSet,
                                                       int samples,
                                                       QSSGRenderCamera *inCamera = nullptr,
                                                       QMatrix4x4 *alteredModelViewProjection = nullptr,
                                                       QSSGRenderTextureCubeFace cubeFace = QSSGRenderTextureCubeFaceNone,
                                                       QSSGReflectionMapEntry *entry = nullptr);

Q_QUICK3DRUNTIMERENDER_EXPORT void rhiRenderRenderable(QSSGRhiContext *rhiCtx,
                                                       const QSSGRhiGraphicsPipelineState &state,
                                                       QSSGRenderableObject &object,
                                                       bool *needsSetViewport,
                                                       QSSGRenderTextureCubeFace cubeFace = QSSGRenderTextureCubeFaceNone);

bool rhiPrepareDepthTexture(QSSGRhiContext *rhiCtx, const QSize &size, QSSGRhiRenderableTexture *renderableTex);

inline QRect correctViewportCoordinates(const QRectF &layerViewport, const QRect &deviceRect)
{
    const int y = deviceRect.bottom() - layerViewport.bottom() + 1;
    return QRect(layerViewport.x(), y, layerViewport.width(), layerViewport.height());
}
}

QT_END_NAMESPACE

#endif
