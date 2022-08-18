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

#include <QtQuick3DUtils/private/qssgrenderbasetypes_p.h>

#define QSSG_RENDER_MINIMUM_RENDER_OPACITY .01f

QT_BEGIN_NAMESPACE

struct QSSGRenderableObject;
class QSSGLightmapper;

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

struct QSSGLayerRenderPreparationResult
{
    QSSGRenderEffect *lastEffect = nullptr;
    QSSGLayerRenderPreparationResultFlags flags;
    QRectF viewport;
    QRectF scissor;
    QSSGRenderLayer *layer = nullptr;

    QSSGLayerRenderPreparationResult() = default;
    QSSGLayerRenderPreparationResult(const QRectF &inViewport, const QRectF &inScissor, QSSGRenderLayer &inLayer);

    bool isLayerVisible() const;
    QSize textureDimensions() const;
    QSSGCameraGlobalCalculationResult setupCameraForRender(QSSGRenderCamera &inCamera);
};

struct QSSGRenderableNodeEntry
{
    QSSGRenderNode *node = nullptr;
    QSSGShaderLightList lights;
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

struct QSSGRhiRenderableTexture
{
    QRhiTexture *texture = nullptr;
    QRhiRenderBuffer *depthStencil = nullptr;
    QRhiRenderPassDescriptor *rpDesc = nullptr;
    QRhiTextureRenderTarget *rt = nullptr;
    bool isValid() const { return texture && rpDesc && rt; }
    void resetRenderTarget() {
        delete rt;
        rt = nullptr;
        delete rpDesc;
        rpDesc = nullptr;
    }
    void reset() {
        resetRenderTarget();
        delete texture;
        delete depthStencil;
        *this = QSSGRhiRenderableTexture();
    }
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
    QAtomicInt ref;

    enum Enum {
        MAX_AA_LEVELS = 8,
        MAX_TEMPORAL_AA_LEVELS = 2,
    };

    QSSGLayerRenderData(QSSGRenderLayer &inLayer, const QSSGRef<QSSGRenderer> &inRenderer);
    ~QSSGLayerRenderData();

    typedef QVector<QSSGModelContext *> TModelContextPtrList;
    typedef QVector<QSSGRenderableObjectHandle> TRenderableObjectList;

    void rhiPrepare();
    void rhiRender();

    QSSGShaderDefaultMaterialKey generateLightingKey(QSSGRenderDefaultMaterial::MaterialLighting inLightingType,
                                                     const QSSGShaderLightList &lights, bool receivesShadows = true);

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
                                                                         const QSSGShaderLightList &lights,
                                                                         QSSGLayerRenderPreparationResultFlags &ioFlags);

    QSSGDefaultMaterialPreparationResult prepareCustomMaterialForRender(QSSGRenderCustomMaterial &inMaterial,
                                                                        QSSGRenderableObjectFlags &inExistingFlags,
                                                                        float inOpacity, bool alreadyDirty,
                                                                        const QSSGShaderLightList &lights,
                                                                        QSSGLayerRenderPreparationResultFlags &ioFlags);

    // Updates lights with model receivesShadows. Do not pass globalLights.
    bool prepareModelForRender(const QSSGRenderModel &inModel,
                               const QMatrix4x4 &inViewProjection,
                               const QSSGOption<QSSGClippingFrustum> &inClipFrustum,
                               QSSGShaderLightList &lights,
                               QSSGLayerRenderPreparationResultFlags &ioFlags);
    bool prepareParticlesForRender(const QSSGRenderParticles &inParticles,
                                   const QSSGOption<QSSGClippingFrustum> &inClipFrustum,
                                   QSSGShaderLightList &lights);

    // Helper function used during PrepareForRender and PrepareAndRender
    bool prepareRenderablesForRender(const QMatrix4x4 &inViewProjection,
                                     const QSSGOption<QSSGClippingFrustum> &inClipFrustum,
                                     QSSGLayerRenderPreparationResultFlags &ioFlags);

    void prepareResourceLoaders();

    void prepareForRender();
    // Helper function used during prepareForRender
    void prepareReflectionProbesForRender();

    QVector3D getCameraDirection();
    // Per-frame cache of renderable objects post-sort.
    const TRenderableObjectList &getSortedOpaqueRenderableObjects();
    // If layer depth test is false, this may also contain opaque objects.
    const TRenderableObjectList &getSortedTransparentRenderableObjects();
    const TRenderableObjectList &getSortedScreenTextureRenderableObjects();
    const QVector<QSSGBakedLightingModel> &getSortedBakedLightingModels();
    const QVector<QSSGRenderableNodeEntry> &getRenderableItem2Ds();

    void resetForFrame();

    void maybeBakeLightmap();

    QSSGRenderLayer &layer;
    QSSGRef<QSSGRenderer> renderer;
    // List of nodes we can render, not all may be active.  Found by doing a depth-first
    // search through m_FirstChild if length is zero.

    // renderableNodes have all lights, but properties configured for specific node
    QVector<QSSGRenderableNodeEntry> renderableNodes;
    QVector<QSSGRenderCamera *> cameras;
    QVector<QSSGRenderLight *> lights;
    QVector<QSSGRenderReflectionProbe *> reflectionProbes;
    QVector<QSSGRenderableNodeEntry> renderableItem2Ds;
    QVector<QSSGRenderableNodeEntry> renderedItem2Ds;

    // Results of prepare for render.

    QSSGRenderCamera *camera = nullptr;
    QSSGShaderLightList globalLights; // Contains all lights except ones with a scope set
    TRenderableObjectList opaqueObjects;
    TRenderableObjectList transparentObjects;
    TRenderableObjectList screenTextureObjects;
    QVector<QSSGBakedLightingModel> bakedLightingModels;
    // Sorted lists of the rendered objects.  There may be other transforms applied so
    // it is simplest to duplicate the lists.
    TRenderableObjectList renderedOpaqueObjects;
    TRenderableObjectList renderedTransparentObjects;
    TRenderableObjectList renderedScreenTextureObjects;
    TRenderableObjectList renderedOpaqueDepthPrepassObjects;
    TRenderableObjectList renderedDepthWriteObjects;
    QVector<QSSGBakedLightingModel> renderedBakedLightingModels;

    QSSGOption<QSSGClippingFrustum> clippingFrustum;
    QSSGOption<QSSGLayerRenderPreparationResult> layerPrepResult;
    QSSGOption<QVector3D> cameraDirection;

    TModelContextPtrList modelContexts;

    QSSGShaderFeatures features;
    bool tooManyLightsWarningShown = false;
    bool tooManyShadowLightsWarningShown = false;
    bool particlesNotSupportedWarningShown = false;

    QSSGRenderShadowMap *shadowMapManager = nullptr;
    QSSGRenderReflectionMap *reflectionMapManager = nullptr;

    // RHI resources
    QSSGRhiRenderableTexture rhiDepthTexture;
    QSSGRhiRenderableTexture rhiAoTexture;
    QSSGRhiRenderableTexture rhiScreenTexture;

private:
    bool m_globalZPrePassActive = false;
    QSSGLightmapper *m_lightmapper = nullptr;
};

QT_END_NAMESPACE

#endif // QSSG_LAYER_RENDER_DATA_H

