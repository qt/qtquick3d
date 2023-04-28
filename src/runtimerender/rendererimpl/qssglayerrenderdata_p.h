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
#include <QtQuick3DRuntimeRender/private/qssgrenderclippingfrustum_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadowmap_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendereffect_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderresourceloader_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderreflectionmap_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercamera_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrhicontext_p.h>

#include <QtQuick3DUtils/private/qssgrenderbasetypes_p.h>

#include <optional>

#include "qssgrenderpass_p.h"

#define QSSG_RENDER_MINIMUM_RENDER_OPACITY .01f

QT_BEGIN_NAMESPACE

struct QSSGRenderableObject;

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
    RequiresMipmapsForScreenTexture = 1 << 6
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
};

struct QSSGCameraData
{
    QSSGCameraData() = default;
    QSSGCameraData(const QVector3D &dir, const QVector3D &pos)
        : direction(dir)
        , position(pos)
    {}
    QSSGCameraData(const QSSGRenderCamera &camera)
        : QSSGCameraData(camera.getScalingCorrectDirection(), camera.getGlobalPos())
    {}
    QVector3D direction;
    QVector3D position;
};

struct QSSGLayerRenderPreparationResult
{
    QSSGLayerRenderPreparationResultFlags flags;
    QRectF viewport;
    QSSGRenderLayer *layer = nullptr;

    QSSGLayerRenderPreparationResult() = default;
    QSSGLayerRenderPreparationResult(const QRectF &inViewport, QSSGRenderLayer &inLayer);

    bool isLayerVisible() const;
    QSize textureDimensions() const;
    QSSGCameraGlobalCalculationResult setupCameraForRender(QSSGRenderCamera &inCamera);
};

struct QSSGRenderableNodeEntry
{
    QSSGRenderNode *node = nullptr;
    mutable QSSGRenderMesh *mesh = nullptr;
    mutable QSSGShaderLightListView lights;
    QSSGRenderableNodeEntry() = default;
    QSSGRenderableNodeEntry(QSSGRenderNode &inNode) : node(&inNode) {}
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

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGLayerRenderData
{
public:
    enum Enum {
        MAX_AA_LEVELS = 8,
        MAX_TEMPORAL_AA_LEVELS = 2,
    };

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
                                QSSGShaderDefaultMaterialKey &key,
                                QSSGRenderer *renderer);

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

    // Helper functions used during PrepareForRender and PrepareAndRender
    // Updates lights with model receivesShadows. Do not pass globalLights.
    bool prepareModelForRender(const RenderableNodeEntries &renderableModels,
                               const QMatrix4x4 &inViewProjection,
                               QSSGLayerRenderPreparationResultFlags &ioFlags,
                               const QSSGCameraData &cameraData,
                               float lodThreshold = 0.0f);
    bool prepareParticlesForRender(const RenderableNodeEntries &renderableParticles, const QSSGCameraData &cameraData);
    static bool prepareItem2DsForRender(const QSSGRenderContextInterface &ctxIfc, const RenderableItem2DEntries &renderableItem2Ds,
                                        const QMatrix4x4 &inViewProjection);

    void prepareResourceLoaders();

    void prepareForRender();
    // Helper function used during prepareForRender
    void prepareReflectionProbesForRender();

    static qsizetype frustumCulling(const QSSGClippingFrustum &clipFrustum, const QSSGRenderableObjectList &renderables, QSSGRenderableObjectList &visibleRenderables);
    [[nodiscard]] static qsizetype frustumCullingInline(const QSSGClippingFrustum &clipFrustum, QSSGRenderableObjectList &renderables);

    [[nodiscard]] QSSGCameraData getCameraDirectionAndPosition();
    // Per-frame cache of renderable objects post-sort (for the MAIN rendering camera, i.e., don't use these lists for rendering from a different camera).
    const QSSGRenderableObjectList &getSortedOpaqueRenderableObjects();
    // If layer depth test is false, this may also contain opaque objects.
    const QSSGRenderableObjectList &getSortedTransparentRenderableObjects();
    const QSSGRenderableObjectList &getSortedScreenTextureRenderableObjects();
    const QVector<QSSGBakedLightingModel> &getSortedBakedLightingModels();
    const RenderableItem2DEntries &getRenderableItem2Ds();
    const QSSGRenderableObjectList &getSortedRenderedDepthWriteObjects();
    const QSSGRenderableObjectList &getSortedrenderedOpaqueDepthPrepassObjects();

    void resetForFrame();

    void maybeBakeLightmap();

    ShadowMapPass shadowMapPass;
    ReflectionMapPass reflectionMapPass;
    ZPrePassPass zPrePassPass;
    SSAOMapPass ssaoMapPass;
    DepthMapPass depthMapPass;
    ScreenMapPass screenMapPass;
    MainPass mainPass;

    // Built-in passes
    QVarLengthArray<QSSGRenderPass *, 8> activePasses;

    QSSGRenderLayer &layer;
    QSSGRenderer *renderer = nullptr;
    // List of nodes we can render, not all may be active.  Found by doing a depth-first
    // search through m_FirstChild if length is zero.

    // renderableNodes have all lights, but properties configured for specific node
    RenderableNodeEntries renderableModels;
    RenderableNodeEntries renderableParticles;
    QVector<QSSGRenderItem2D *> renderableItem2Ds;
    QVector<QSSGRenderCamera *> cameras;
    QVector<QSSGRenderLight *> lights;
    QVector<QSSGRenderReflectionProbe *> reflectionProbes;

    // Results of prepare for render.
    QSSGRenderCamera *camera = nullptr;
    QSSGShaderLightList globalLights; // All non-scoped lights
    QSSGRenderableObjectList opaqueObjects;
    QSSGRenderableObjectList transparentObjects;
    QSSGRenderableObjectList screenTextureObjects;
    QVector<QSSGBakedLightingModel> bakedLightingModels;
    // Sorted lists of the rendered objects.  There may be other transforms applied so
    // it is simplest to duplicate the lists.
    QSSGRenderableObjectList renderedOpaqueObjects;
    QSSGRenderableObjectList renderedTransparentObjects;
    QSSGRenderableObjectList renderedScreenTextureObjects;
    QSSGRenderableObjectList renderedOpaqueDepthPrepassObjects;
    QSSGRenderableObjectList renderedDepthWriteObjects;
    QVector<QSSGBakedLightingModel> renderedBakedLightingModels;
    RenderableItem2DEntries renderedItem2Ds;

    std::optional<QSSGClippingFrustum> clippingFrustum;
    std::optional<QSSGLayerRenderPreparationResult> layerPrepResult;
    std::optional<QSSGCameraData> cameraData;

    TModelContextPtrList modelContexts;


    bool tooManyLightsWarningShown = false;
    bool tooManyShadowLightsWarningShown = false;
    bool particlesNotSupportedWarningShown = false;

    QSSGLightmapper *m_lightmapper = nullptr;

    QSSGShaderFeatures getShaderFeatures() const { return features; }
    QSSGRhiGraphicsPipelineState getPipelineState() const { return ps; }

    bool interactiveLightmapBakingRequested = false;
    QSSGLightmapper::Callback lightmapBakingOutputCallback;

    bool plainSkyBoxPrepared = false;

    // Temp. API. Ideally there shouldn't be a reason for anyone to hold onto these,
    // but we follow the existing pattern for now.
    const QSSGRenderShadowMapPtr &requestShadowMapManager();
    const QSSGRenderReflectionMapPtr &requestReflectionMapManager();
    const QSSGRenderShadowMapPtr &getShadowMapManager() const { return shadowMapManager; }
    const QSSGRenderReflectionMapPtr &getReflectionMapManager() const { return reflectionMapManager; }

private:
    void updateSortedDepthObjectsListImp();
    QSSGRhiGraphicsPipelineState ps; // Base pipleline state
    QSSGShaderFeatures features; // Base feature set
    QSSGRenderShadowMapPtr shadowMapManager;
    QSSGRenderReflectionMapPtr reflectionMapManager;
};

QT_END_NAMESPACE

#endif // QSSG_LAYER_RENDER_DATA_H

