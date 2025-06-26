// Copyright (C) 2008-2012 NVIDIA Corporation.
// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QSSG_LAYER_RENDER_DATA_H
#define QSSG_LAYER_RENDER_DATA_H


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

#include <QtQuick3DRuntimeRender/private/qssgrenderitem2d_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercache_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderableobjects_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadowmap_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendereffect_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderresourceloader_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderreflectionmap_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercamera_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrhicontext_p.h>
#include <QtQuick3DRuntimeRender/private/qssgperframeallocator_p.h>
#include <QtQuick3DRuntimeRender/private/qssgshadermapkey_p.h>
#include <QtQuick3DRuntimeRender/private/qssglightmapbaker_p.h>
#include <ssg/qssgrenderextensions.h>

#include <QtQuick3DUtils/private/qssgrenderbasetypes_p.h>

#include <optional>
#include <unordered_map>

#include "qssgrenderpass_p.h"
#include "qssgrenderdata_p.h"

QT_BEGIN_NAMESPACE

class QSSGRenderableObject;

class QSGRenderer;

enum class QSSGLayerRenderPreparationResultFlag
{
    // Was the data in this layer dirty (meaning re-render to texture, possibly)
    WasLayerDataDirty = 1 << 0,

    // Was the data in this layer dirty *or* this layer *or* any effect dirty.
    WasDirty = 1 << 1,

    RequiresDepthTexture = 1 << 2,

    // SSAO should be done in a separate pass
    // Note that having an AO pass necessitates a DepthTexture so this flag should
    // never be set without the RequiresDepthTexture flag as well.
    RequiresSsaoPass = 1 << 3,

    // if some light cause shadow
    // we need a separate per light shadow map pass
    RequiresShadowMapPass = 1 << 4,

    RequiresScreenTexture = 1 << 5,

    // set together with RequiresScreenTexture when SCREEN_MIP_TEXTURE is used
    RequiresMipmapsForScreenTexture = 1 << 6,

    // Set when material has custom blend mode(not SourceOver)
    MaterialHasCustomBlendMode = 1 << 7,

    // Set when multisampled depth texture is required
    RequiresDepthTextureMS = 1 << 8
};

struct QSSGLayerRenderPreparationResultFlags : public QFlags<QSSGLayerRenderPreparationResultFlag>
{
    bool wasLayerDataDirty() const
    {
        return this->operator&(QSSGLayerRenderPreparationResultFlag::WasLayerDataDirty);
    }
    void setLayerDataDirty(bool inValue)
    {
        setFlag(QSSGLayerRenderPreparationResultFlag::WasLayerDataDirty, inValue);
    }

    bool wasDirty() const { return this->operator&(QSSGLayerRenderPreparationResultFlag::WasDirty); }
    void setWasDirty(bool inValue) { setFlag(QSSGLayerRenderPreparationResultFlag::WasDirty, inValue); }

    bool requiresDepthTexture() const
    {
        return this->operator&(QSSGLayerRenderPreparationResultFlag::RequiresDepthTexture);
    }
    void setRequiresDepthTexture(bool inValue)
    {
        setFlag(QSSGLayerRenderPreparationResultFlag::RequiresDepthTexture, inValue);
    }

    bool requiresDepthTextureMS() const
    {
        return this->operator&(QSSGLayerRenderPreparationResultFlag::RequiresDepthTextureMS);
    }
    void setRequiresDepthTextureMS(bool inValue)
    {
        setFlag(QSSGLayerRenderPreparationResultFlag::RequiresDepthTextureMS, inValue);
    }

    bool requiresSsaoPass() const { return this->operator&(QSSGLayerRenderPreparationResultFlag::RequiresSsaoPass); }
    void setRequiresSsaoPass(bool inValue)
    {
        setFlag(QSSGLayerRenderPreparationResultFlag::RequiresSsaoPass, inValue);
    }

    bool requiresShadowMapPass() const
    {
        return this->operator&(QSSGLayerRenderPreparationResultFlag::RequiresShadowMapPass);
    }
    void setRequiresShadowMapPass(bool inValue)
    {
        setFlag(QSSGLayerRenderPreparationResultFlag::RequiresShadowMapPass, inValue);
    }

    bool requiresScreenTexture() const
    {
        return this->operator&(QSSGLayerRenderPreparationResultFlag::RequiresScreenTexture);
    }
    void setRequiresScreenTexture(bool inValue)
    {
        setFlag(QSSGLayerRenderPreparationResultFlag::RequiresScreenTexture, inValue);
    }

    bool requiresMipmapsForScreenTexture() const
    {
        return this->operator&(QSSGLayerRenderPreparationResultFlag::RequiresMipmapsForScreenTexture);
    }
    void setRequiresMipmapsForScreenTexture(bool inValue)
    {
        setFlag(QSSGLayerRenderPreparationResultFlag::RequiresMipmapsForScreenTexture, inValue);
    }

    bool hasCustomBlendMode() const
    {
        return this->operator&(QSSGLayerRenderPreparationResultFlag::MaterialHasCustomBlendMode);
    }
    void setHasCustomBlendMode(bool inValue)
    {
        setFlag(QSSGLayerRenderPreparationResultFlag::MaterialHasCustomBlendMode, inValue);
    }
};

struct QSSGLayerRenderPreparationResult
{
    QSSGLayerRenderPreparationResultFlags flags;
    QRectF viewport;
    QSSGRenderLayer *layer = nullptr;

    QSSGLayerRenderPreparationResult() = default;
    QSSGLayerRenderPreparationResult(const QRectF &inViewport, QSSGRenderLayer &inLayer);

    bool isNull() const { return !layer; }
    bool isLayerVisible() const;
    QSize textureDimensions() const;
};

struct QSSGDefaultMaterialPreparationResult
{
    QSSGRenderableImage *firstImage;
    float opacity;
    QSSGRenderableObjectFlags renderableFlags;
    QSSGShaderDefaultMaterialKey materialKey;
    bool dirty;

    explicit QSSGDefaultMaterialPreparationResult(QSSGShaderDefaultMaterialKey inMaterialKey);
};

struct QSSGBakedLightingModel
{
    QSSGBakedLightingModel(const QSSGRenderModel *model, const QVector<QSSGRenderableObjectHandle> &renderables)
        : model(model),
          renderables(renderables)
    { }

    const QSSGRenderModel *model;
    QVector<QSSGRenderableObjectHandle> renderables;
};

struct QSSGOITRenderContext
{
    QRhiTextureRenderTarget *oitRenderTarget = nullptr;
    QRhiRenderPassDescriptor *renderPassDescriptor = nullptr;
    void reset()
    {
        delete oitRenderTarget;
        delete renderPassDescriptor;
        oitRenderTarget = nullptr;
        renderPassDescriptor = nullptr;
    }
};

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGLayerRenderData
{
public:
    enum Enum {
        MAX_AA_LEVELS = 8,
        MAX_TEMPORAL_AA_LEVELS = 2,
    };

    using InstanceTransforms = QSSGGlobalRenderNodeData::InstanceTransforms;
    using ModelViewProjections = QSSGRenderModelData::ModelViewProjections;

    using QSSGModelsView = QSSGDataView<QSSGRenderModel *>;
    using QSSGParticlesView = QSSGDataView<QSSGRenderParticles *>;
    using QSSGItem2DsView = QSSGDataView<QSSGRenderItem2D *>;
    using QSSGCamerasView = QSSGDataView<QSSGRenderCamera *>;
    using QSSGLightsView = QSSGDataView<QSSGRenderLight *>;
    using QSSGReflectionProbesView = QSSGDataView<QSSGRenderReflectionProbe *>;
    using QSSGNonCategorizedView = QSSGDataView<QSSGRenderNode *>;

    using RenderableFilter = std::function<bool(QSSGModelContext *)>;

    QSSGLayerRenderData(QSSGRenderLayer &inLayer, QSSGRenderer &inRenderer);
    ~QSSGLayerRenderData();

    typedef QVector<QSSGModelContext *> TModelContextPtrList;
    using RenderableNodeEntries = QVector<QSSGRenderableNodeEntry>;
    using RenderableItem2DEntries = QVector<QSSGRenderItem2D *>;

    QSSGShaderDefaultMaterialKey generateLightingKey(QSSGRenderDefaultMaterial::MaterialLighting inLightingType,
                                                     const QSSGShaderLightListView &lights, bool receivesShadows = true);

    void prepareImageForRender(QSSGRenderImage &inImage,
                               QSSGRenderableImage::Type inMapType,
                               QSSGRenderableImage *&ioFirstImage,
                               QSSGRenderableImage *&ioNextImage,
                               QSSGRenderableObjectFlags &ioFlags,
                               QSSGShaderDefaultMaterialKey &ioGeneratedShaderKey,
                               quint32 inImageIndex, QSSGRenderDefaultMaterial *inMaterial = nullptr);

    void setVertexInputPresence(const QSSGRenderableObjectFlags &renderableFlags,
                                QSSGShaderDefaultMaterialKey &key);

    static void prepareModelBoneTextures(const QSSGRenderContextInterface &contextInterface,
                                         const RenderableNodeEntries &renderableModels);

    // Helper functions used during PrepareForRender and PrepareAndRender
    // Updates lights with model receivesShadows. Do not pass globalLights.
    bool prepareModelsForRender(QSSGRenderContextInterface &ctx,
                                const RenderableNodeEntries &renderableModels,
                                QSSGLayerRenderPreparationResultFlags &ioFlags,
                                const QSSGRenderCameraList &allCameras,
                                const QSSGRenderCameraDataList &allCameraData,
                                TModelContextPtrList &modelContexts,
                                QSSGRenderableObjectList &opaqueObjects,
                                QSSGRenderableObjectList &transparentObjects,
                                QSSGRenderableObjectList &screenTextureObjects,
                                float lodThreshold = 0.0f);
    bool prepareParticlesForRender(const RenderableNodeEntries &renderableParticles, const QSSGRenderCameraData &cameraData, QSSGLayerRenderPreparationResultFlags &ioFlags);
    bool prepareItem2DsForRender(const QSSGRenderContextInterface &ctxIfc,
                                 const QSSGItem2DsView &renderableItem2Ds);

    void prepareResourceLoaders();

    void prepareForRender();
    // Helper function used during prepareForRender
    void prepareReflectionProbesForRender();

    static qsizetype frustumCulling(const QSSGClippingFrustum &clipFrustum, const QSSGRenderableObjectList &renderables, QSSGRenderableObjectList &visibleRenderables);
    [[nodiscard]] static qsizetype frustumCullingInline(const QSSGClippingFrustum &clipFrustum, QSSGRenderableObjectList &renderables);


    // Per-frame cache of renderable objects post-sort (for the MAIN rendering camera, i.e., don't use these lists for rendering from a different camera).
    const QSSGRenderableObjectList &getSortedOpaqueRenderableObjects(const QSSGRenderCamera &camera, size_t index = 0);
    // If layer depth test is false, this may also contain opaque objects.
    const QSSGRenderableObjectList &getSortedTransparentRenderableObjects(const QSSGRenderCamera &camera, size_t index = 0);
    const QSSGRenderableObjectList &getSortedScreenTextureRenderableObjects(const QSSGRenderCamera &camera, size_t index = 0);
    const QVector<QSSGBakedLightingModel> &getSortedBakedLightingModels();
    const RenderableItem2DEntries &getRenderableItem2Ds();
    const QSSGRenderableObjectList &getSortedRenderedDepthWriteObjects(const QSSGRenderCamera &camera, size_t index = 0);
    const QSSGRenderableObjectList &getSortedrenderedOpaqueDepthPrepassObjects(const QSSGRenderCamera &camera, size_t index = 0);

    void resetForFrame();

    QSSGFrameData &getFrameData();

    ShadowMapPass shadowMapPass;
    ReflectionMapPass reflectionMapPass;
    ZPrePassPass zPrePassPass;
    SSAOMapPass ssaoMapPass;
    DepthMapPass depthMapPass;
    DepthMapPass depthMapPassMS;
    ScreenMapPass screenMapPass;
    ScreenReflectionPass reflectionPass;
    Item2DPass item2DPass;
    SkyboxPass skyboxPass;
    SkyboxCubeMapPass skyboxCubeMapPass;
    static constexpr size_t USERPASSES = 2; // See QSSGRenderLayer::RenderExtensionMode::Count
    UserPass userPasses[USERPASSES];
    OpaquePass opaquePass;
    TransparentPass transparentPass;
    OITRenderPass oitRenderPass;
    OITCompositePass oitCompositePass;
    InfiniteGridPass infiniteGridPass;
    DebugDrawPass debugDrawPass;

    // Built-in passes
    QVarLengthArray<QSSGRenderPass *, 16> activePasses;

    QSSGRenderLayer &layer;
    QSSGRenderer *renderer = nullptr;
    // List of nodes we can render, not all may be active.  Found by doing a depth-first
    // search through m_FirstChild if length is zero.

    using LayerNodes = std::vector<QSSGRenderNode *>;
    QSSGGlobalRenderNodeData::LayerNodeView layerNodes;
    LayerNodes layerNodesCategorized;

    // renderableNodes have all lights, but properties configured for specific node
    RenderableNodeEntries renderableModels;
    RenderableNodeEntries renderableParticles;

    // Views into the collected nodes (unsorted)
    QSSGModelsView modelsView;
    QSSGParticlesView particlesView;
    QSSGItem2DsView item2DsView;
    QSSGCamerasView camerasView;
    QSSGLightsView lightsView;
    QSSGReflectionProbesView reflectionProbesView;
    QSSGNonCategorizedView nonCategorizedView;

    // Results of prepare for render.
    QSSGRenderCameraList renderedCameras; // multiple items with multiview, one otherwise (or zero if no cameras at all)
    QSSGShaderLightList globalLights; // All non-scoped lights

    QVector<QSSGBakedLightingModel> bakedLightingModels;
    // Sorted lists of the rendered objects.  There may be other transforms applied so
    // it is simplest to duplicate the lists.
    QVector<QSSGBakedLightingModel> renderedBakedLightingModels;
    RenderableItem2DEntries renderedItem2Ds;
    // Temporary look-up map for Item2D data (for use in prepareItem2DsForRender()).
    QSSGRenderer::Item2DDataMap item2DDataMap;
    QPointer<QSGRenderContext> item2DRenderContext;

    QSSGLayerRenderPreparationResult layerPrepResult;
    std::optional<QSSGRenderCameraDataList> renderedCameraData;

    TModelContextPtrList modelContexts;


    bool tooManyLightsWarningShown = false;
    bool tooManyShadowLightsWarningShown = false;
    bool oitWarningUnsupportedShown = false;
    bool oitWarningInvalidBlendModeShown = false;
    bool orderIndependentTransparencyEnabled = false;

    std::unique_ptr<QSSGLightmapBaker> lightmapBaker = nullptr;

    QSSGShaderFeatures getShaderFeatures() const { return features; }
    QSSGRhiGraphicsPipelineState getPipelineState() const { return ps; }

    void initializeLightmapBaking(QSSGLightmapBaker::Context &ctx);
    void maybeProcessLightmapBaking();

    [[nodiscard]] QSSGRenderGraphObject *getCamera(QSSGCameraId id) const;
    [[nodiscard]] QSSGRenderCamera *activeCamera() const { return !renderedCameras.isEmpty() ? renderedCameras[0] : nullptr; }

    [[nodiscard]] QSSGRenderCameraData getCameraRenderData(const QSSGRenderCamera *camera);
    [[nodiscard]] QSSGRenderCameraData getCameraRenderData(const QSSGRenderCamera *camera) const;

    void setLightmapTexture(const QSSGModelContext &modelContext, QRhiTexture *lightmapTexture);
    [[nodiscard]] QRhiTexture *getLightmapTexture(const QSSGModelContext &modelContext) const;

    void setBonemapTexture(const QSSGModelContext &modelContext, QRhiTexture *bonemapTexture);
    [[nodiscard]] QRhiTexture *getBonemapTexture(const QSSGModelContext &modelContext) const;

    [[nodiscard]] QSSGRenderContextInterface *contextInterface() const;
    // Note: temp. API to report the state of the z-prepass step
    [[nodiscard]] bool isZPrePassActive() const { return zPrePassActive; }
    void setZPrePassPrepResult(bool res) { zPrePassActive = res; }

    // Exposed as const, as we often need to use this to look-up values from a specific key.
    [[nodiscard]] const QSSGShaderDefaultMaterialKeyProperties &getDefaultMaterialPropertyTable() const
    {
        return defaultMaterialShaderKeyProperties;
    }

    struct GlobalRenderProperties
    {
        bool isYUpInFramebuffer = true;
        bool isYUpInNDC = true;
        bool isClipDepthZeroToOne = true;
    };

    [[nodiscard]] static GlobalRenderProperties globalRenderProperties(const QSSGRenderContextInterface &ctx);

    // Temp. API. Ideally there shouldn't be a reason for anyone to hold onto these,
    // but we follow the existing pattern for now.
    const QSSGRenderShadowMapPtr &requestShadowMapManager();
    const QSSGRenderReflectionMapPtr &requestReflectionMapManager();
    const QSSGRenderShadowMapPtr &getShadowMapManager() const { return shadowMapManager; }
    const QSSGRenderReflectionMapPtr &getReflectionMapManager() const { return reflectionMapManager; }

    QSSGOITRenderContext &getOitRenderContext() { return oitRenderContext; }

    static bool prepareInstancing(QSSGRhiContext *rhiCtx,
                                  QSSGSubsetRenderable *renderable,
                                  const QVector3D &cameraDirection,
                                  const QVector3D &cameraPosition,
                                  float minThreshold,
                                  float maxThreshold);

    [[nodiscard]] QSSGRhiRenderableTexture *getRenderResult(QSSGFrameData::RenderResult id) { return &renderResults[size_t(id)]; }
    [[nodiscard]] const QSSGRhiRenderableTexture *getRenderResult(QSSGFrameData::RenderResult id) const { return &renderResults[size_t(id)]; }
    [[nodiscard]] static inline const std::unique_ptr<QSSGPerFrameAllocator> &perFrameAllocator(QSSGRenderContextInterface &ctx);
    [[nodiscard]] static inline QSSGLayerRenderData *getCurrent(const QSSGRenderer &renderer) { return renderer.m_currentLayer; }
    void saveRenderState(const QSSGRenderer &renderer);
    void restoreRenderState(QSSGRenderer &renderer);

    static void setTonemapFeatures(QSSGShaderFeatures &features, QSSGRenderLayer::TonemapMode tonemapMode)
    {
        features.set(QSSGShaderFeatures::Feature::LinearTonemapping,
                     tonemapMode == QSSGRenderLayer::TonemapMode::Linear);
        features.set(QSSGShaderFeatures::Feature::AcesTonemapping,
                     tonemapMode == QSSGRenderLayer::TonemapMode::Aces);
        features.set(QSSGShaderFeatures::Feature::HejlDawsonTonemapping,
                     tonemapMode == QSSGRenderLayer::TonemapMode::HejlDawson);
        features.set(QSSGShaderFeatures::Feature::FilmicTonemapping,
                     tonemapMode == QSSGRenderLayer::TonemapMode::Filmic);
        features.set(QSSGShaderFeatures::Feature::ForceIblExposure,
                     tonemapMode == QSSGRenderLayer::TonemapMode::Custom);
    }

    QSSGPrepContextId getOrCreateExtensionContext(const QSSGRenderExtension &ext,
                                                  QSSGRenderCamera *camera = nullptr,
                                                  quint32 slot = 0);

    // Model API
    QSSGRenderablesId createRenderables(QSSGPrepContextId prepId, const QList<QSSGNodeId> &nodes, QSSGRenderHelpers::CreateFlags createFlags);
    void setGlobalTransform(QSSGRenderablesId renderablesId, const QSSGRenderModel &model, const QMatrix4x4 &mvp);
    QMatrix4x4 getGlobalTransform(QSSGPrepContextId prepId, const QSSGRenderModel &model);
    void setGlobalOpacity(QSSGRenderablesId renderablesId, const QSSGRenderModel &model, float opacity);
    float getGlobalOpacity(QSSGPrepContextId prepId, const QSSGRenderModel &model);
    [[nodiscard]] QMatrix4x4 getModelMvps(QSSGPrepContextId prepId, const QSSGRenderModel &model) const;
    void setModelMaterials(QSSGRenderablesId renderablesId, const QSSGRenderModel &model, const QList<QSSGResourceId> &materials);
    void setModelMaterials(const QSSGRenderablesId renderablesId, const QList<QSSGResourceId> &materials);
    [[nodiscard]] QSSGPrepResultId prepareModelsForRender(QSSGRenderContextInterface &contextInterface,
                                                               QSSGPrepContextId prepId,
                                                               QSSGRenderablesId renderablesId,
                                                               float lodThreshold);

    // Convenience wrappers for getting values from the node, model store.
    [[nodiscard]] QMatrix4x4 getGlobalTransform(QSSGRenderNodeHandle h, QMatrix4x4 defaultValue) const;
    [[nodiscard]] QMatrix4x4 getGlobalTransform(QSSGRenderNodeHandle h) const;
    [[nodiscard]] QMatrix4x4 getGlobalTransform(const QSSGRenderNode &node) const;

    [[nodiscard]] QMatrix3x3 getNormalMatrix(QSSGRenderModelHandle h) const;
    [[nodiscard]] QMatrix3x3 getNormalMatrix(const QSSGRenderModel &model) const;

    [[nodiscard]] ModelViewProjections getModelMvps(QSSGRenderModelHandle h) const;
    [[nodiscard]] ModelViewProjections getModelMvps(const QSSGRenderModel &model) const;

    [[nodiscard]] InstanceTransforms getInstanceTransforms(QSSGRenderNodeHandle h) const;
    [[nodiscard]] InstanceTransforms getInstanceTransforms(const QSSGRenderNode &node) const;

    [[nodiscard]] float getGlobalOpacity(QSSGRenderNodeHandle h, float defaultValue = 1.0f) const;
    [[nodiscard]] float getGlobalOpacity(const QSSGRenderNode &node) const;

    //
    void prepareRenderables(QSSGRenderContextInterface &ctx,
                            QSSGPrepResultId prepId,
                            QRhiRenderPassDescriptor *renderPassDescriptor,
                            const QSSGRhiGraphicsPipelineState &ps,
                            QSSGRenderablesFilters filter);
    void renderRenderables(QSSGRenderContextInterface &ctx,
                           QSSGPrepResultId prepId);

    static bool calculateGlobalVariables(QSSGRenderNode &node,
                                         std::vector<QMatrix4x4> &globalTransforms,
                                         std::vector<float> &globalOpacities);

    QSSGRenderCameraData getCameraDataImpl(const QSSGRenderCamera *camera) const;

private:
    friend class QSSGRenderer;
    friend class QSSGRendererPrivate;
    friend class QSSGFrameData;
    friend class QSSGModelHelpers;
    friend class QSSGRenderHelpers;

    class ExtensionContext
    {
    public:
        explicit ExtensionContext() = default;
        explicit ExtensionContext(const QSSGRenderExtension &ownerExt, QSSGRenderCamera *cam, size_t idx, quint32 slot)
            : owner(&ownerExt), camera(cam), ps{}, filter{0}, index(idx), slot(slot)
        { }
        const QSSGRenderExtension *owner = nullptr;
        QSSGRenderCamera *camera = nullptr;
        QSSGRhiGraphicsPipelineState ps[3] {};
        QSSGRenderablesFilters filter { 0 };
        size_t index = 0; // index into the model store
        quint32 slot = 0;
    };

    std::vector<ExtensionContext> extContexts { ExtensionContext{ /* 0 - Always available */ } };
    std::vector<RenderableNodeEntries> renderableModelStore { RenderableNodeEntries{ /* 0 - Always available */ } };
    std::vector<TModelContextPtrList> modelContextStore { TModelContextPtrList{ /* 0 - Always available */ }};
    std::vector<QSSGRenderableObjectList> renderableObjectStore { QSSGRenderableObjectList{ /* 0 - Always available */ }};
    std::vector<QSSGRenderableObjectList> opaqueObjectStore { QSSGRenderableObjectList{ /* 0 - Always available */ }};
    std::vector<QSSGRenderableObjectList> transparentObjectStore { QSSGRenderableObjectList{ /* 0 - Always available */ }};
    std::vector<QSSGRenderableObjectList> screenTextureObjectStore { QSSGRenderableObjectList{ /* 0 - Always available */ }};

    std::shared_ptr<QSSGGlobalRenderNodeData> nodeData;
    std::unique_ptr<QSSGRenderModelData> modelData;

    // Soreted cache (per camera and extension)
    using PerCameraCache = std::unordered_map<const QSSGRenderCamera *, QSSGRenderableObjectList>;
    std::vector<PerCameraCache> sortedOpaqueObjectCache { PerCameraCache{ /* 0 - Always available */ } };
    std::vector<PerCameraCache> sortedTransparentObjectCache { PerCameraCache{ /* 0 - Always available */ } };
    std::vector<PerCameraCache> sortedScreenTextureObjectCache { PerCameraCache{ /* 0 - Always available */ } };
    std::vector<PerCameraCache> sortedOpaqueDepthPrepassCache { PerCameraCache{ /* 0 - Always available */ } };
    std::vector<PerCameraCache> sortedDepthWriteCache { PerCameraCache{ /* 0 - Always available */ } };

    [[nodiscard]] const QSSGRenderCameraDataList &getCachedCameraDatas();
    void ensureCachedCameraDatas();
    void updateSortedDepthObjectsListImp(const QSSGRenderCamera &camera, size_t index);


    QSSGDefaultMaterialPreparationResult prepareDefaultMaterialForRender(QSSGRenderDefaultMaterial &inMaterial,
                                                                         QSSGRenderableObjectFlags &inExistingFlags,
                                                                         float inOpacity,
                                                                         const QSSGShaderLightListView &lights,
                                                                         QSSGLayerRenderPreparationResultFlags &ioFlags);

    QSSGDefaultMaterialPreparationResult prepareCustomMaterialForRender(QSSGRenderCustomMaterial &inMaterial,
                                                                        QSSGRenderableObjectFlags &inExistingFlags,
                                                                        float inOpacity, bool alreadyDirty,
                                                                        const QSSGShaderLightListView &lights,
                                                                        QSSGLayerRenderPreparationResultFlags &ioFlags);

    static void prepareModelMaterials(RenderableNodeEntries &renderableModels, bool cullUnrenderables);
    static void prepareModelMaterials(const RenderableNodeEntries::ConstIterator &begin,
                                      const RenderableNodeEntries::ConstIterator &end);

    // Persistent data
    QHash<QSSGShaderMapKey, QSSGRhiShaderPipelinePtr> shaderMap;

    // Cached buffer.
    QByteArray generatedShaderString;

    // Saved render state (for sublayers)
    struct SavedRenderState
    {
        QRect viewport;
        QRect scissorRect;
        float dpr = 1.0;
    };

    std::optional<SavedRenderState> savedRenderState;

    // Note: Re-used to avoid expensive initialization.
    // - Should be revisit, as we can do better.
    QSSGShaderDefaultMaterialKeyProperties defaultMaterialShaderKeyProperties;
    QSSGFrameData frameData;
    QSSGRhiGraphicsPipelineState ps; // Base pipleline state
    QSSGShaderFeatures features; // Base feature set
    quint32 version = 0;
    bool particlesEnabled = true;
    bool hasDepthWriteObjects = false;
    bool zPrePassActive = false;
    // NOTE: For the time being we need to keep track of extensions modifying the renderables
    // because then we need to reset the lists.
    // FIXME: This should be revisited, as we can do better (hence the verbose name).
    bool renderablesModifiedByExtension = false;
    enum class DepthPrepassObject : quint8
    {
        None = 0x0,
        ScreenTexture = 0x1,
        Transparent = 0x2,
        Opaque = 0x4
    };
    using DepthPrepassObjectStateT = std::underlying_type_t<DepthPrepassObject>;
    DepthPrepassObjectStateT depthPrepassObjectsState { DepthPrepassObjectStateT(DepthPrepassObject::None) };
    QSSGRenderShadowMapPtr shadowMapManager;
    QSSGRenderReflectionMapPtr reflectionMapManager;
    QHash<const QSSGModelContext *, QRhiTexture *> lightmapTextures;
    QHash<const QSSGModelContext *, QRhiTexture *> bonemapTextures;
    QSSGRhiRenderableTexture renderResults[6] {};
    QSSGOITRenderContext oitRenderContext;
};

QT_END_NAMESPACE

#endif // QSSG_LAYER_RENDER_DATA_H

