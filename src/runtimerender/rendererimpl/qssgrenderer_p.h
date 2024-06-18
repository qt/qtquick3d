// Copyright (C) 2008-2012 NVIDIA Corporation.
// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QSSG_RENDERER_P_H
#define QSSG_RENDERER_P_H

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

#include <private/qssgrenderpickresult_p.h>
#include <private/qssgrhicontext_p.h>
#include <private/qssgrhiquadrenderer_p.h>

QT_BEGIN_NAMESPACE

class QSSGShaderCache;
class QSSGProgramGenerator;
class QSSGShaderLibraryManager;
class QSSGBufferManager;
class QSSGLayerRenderData;
class QSSGRenderContextInterface;
struct QSSGRenderNode;
struct QSSGRenderItem2D;
struct QSSGRenderRay;
struct QSSGSubsetRenderable;
struct QSSGShaderDefaultMaterialKeyProperties;
struct QSSGShaderFeatures;

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRenderer
{
    Q_DISABLE_COPY(QSSGRenderer)
public:
    QSSGRenderer();
    ~QSSGRenderer();

    // Returns true if this layer or a sibling was dirty.
    bool prepareLayerForRender(QSSGRenderLayer &inLayer);

    void rhiPrepare(QSSGRenderLayer &inLayer);
    void rhiRender(QSSGRenderLayer &inLayer);

    // Clients need to call this every frame in order for various subsystems to release
    // temporary per-frame allocated objects.
    void beginFrame(QSSGRenderLayer &layer, bool allowRecursion = true);

    // When allowRecursion is true, the cleanup is only done when all
    // beginFrames got their corresponding endFrame. This is indicated by the
    // return value (false if nothing's been done due to pending "frames")
    bool endFrame(QSSGRenderLayer &layer, bool allowRecursion = true);

    // Get the number of times EndFrame has been called
    [[nodiscard]] constexpr quint32 frameCount() const { return m_frameCount; }

    void setViewport(QRect inViewport) { m_viewport = inViewport; }
    QRect viewport() const { return m_viewport; }

    void setDpr(float dpr) { m_dpr = dpr; }
    float dpr() const { return m_dpr; }

    void setScissorRect(QRect inScissorRect) { m_scissorRect = inScissorRect; }
    QRect scissorRect() const { return m_scissorRect; }

    const std::unique_ptr<QSSGRhiQuadRenderer> &rhiQuadRenderer() const;
    const std::unique_ptr<QSSGRhiCubeRenderer> &rhiCubeRenderer() const;

    QSSGRenderContextInterface *contextInterface() const { return m_contextInterface; }

protected:
    void cleanupResources(QList<QSSGRenderGraphObject*> &resources);
    void cleanupResources(QSet<QSSGRenderGraphObject*> &resources);

private:
    friend class QSSGRendererPrivate;
    friend class QSSGLayerRenderData;
    friend class QSSGRenderContextInterface;
    friend class QQuick3DSceneRenderer;
    friend class QQuick3DWindowAttachment;
    friend class QSSGCleanupObject;

    QSSGLayerRenderData *getOrCreateLayerRenderData(QSSGRenderLayer &layer);
    void beginLayerRender(QSSGLayerRenderData &inLayer);
    void endLayerRender();
    void addMaterialDirtyClear(QSSGRenderGraphObject *material);
    void cleanupUnreferencedBuffers(QSSGRenderLayer *inLayer);
    void resetResourceCounters(QSSGRenderLayer *inLayer);
    void releaseCachedResources();

    QSSGRenderContextInterface *m_contextInterface = nullptr; //  We're own by the context interface

    bool m_globalPickingEnabled = false;

    // Temporary information stored only when rendering a particular layer.
    QSSGLayerRenderData *m_currentLayer = nullptr;
    QByteArray m_generatedShaderString;

    QSet<QSSGRenderGraphObject *> m_materialClearDirty;

    mutable std::unique_ptr<QSSGRhiQuadRenderer> m_rhiQuadRenderer;
    mutable std::unique_ptr<QSSGRhiCubeRenderer> m_rhiCubeRenderer;

    quint32 m_activeFrameRef = 0;
    quint32 m_frameCount = 0;

    // Viewport that this render context should use
    QRect m_viewport;
    float m_dpr = 1.0;
    QRect m_scissorRect;
};

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRendererPrivate
{
    QSSGRendererPrivate() = default;
public:
    using PickResultList = QVarLengthArray<QSSGRenderPickResult, 20>; // Lets assume most items are filtered out already

    static QSSGRhiShaderPipelinePtr generateRhiShaderPipelineImpl(QSSGSubsetRenderable &renderable,
                                                                  QSSGShaderLibraryManager &shaderLibraryManager,
                                                                  QSSGShaderCache &shaderCache,
                                                                  QSSGProgramGenerator &shaderProgramGenerator,
                                                                  const QSSGShaderDefaultMaterialKeyProperties &shaderKeyProperties,
                                                                  const QSSGShaderFeatures &featureSet,
                                                                  QByteArray &shaderString);
    static QSSGRhiShaderPipelinePtr generateRhiShaderPipeline(QSSGRenderer &renderer,
                                                              QSSGSubsetRenderable &inRenderable,
                                                              const QSSGShaderFeatures &inFeatureSet);

    static QSSGRhiShaderPipelinePtr getShaderPipelineForDefaultMaterial(QSSGRenderer &renderer,
                                                                        QSSGSubsetRenderable &inRenderable,
                                                                        const QSSGShaderFeatures &inFeatureSet);

    static void getLayerHitObjectList(const QSSGRenderLayer &layer,
                                      QSSGBufferManager &bufferManager,
                                      const QSSGRenderRay &ray,
                                      bool inPickEverything,
                                      PickResultList &outIntersectionResult);
    static void intersectRayWithSubsetRenderable(QSSGBufferManager &bufferManager,
                                                 const QSSGRenderRay &inRay,
                                                 const QSSGRenderNode &node,
                                                 PickResultList &outIntersectionResultList);
    static void intersectRayWithItem2D(const QSSGRenderRay &inRay,
                                       const QSSGRenderItem2D &item2D,
                                       PickResultList &outIntersectionResultList);

    static PickResultList syncPickAll(const QSSGRenderContextInterface &ctx,
                                      const QSSGRenderLayer &layer,
                                      const QSSGRenderRay &ray);

    static PickResultList syncPick(const QSSGRenderContextInterface &ctx,
                                   const QSSGRenderLayer &layer,
                                   const QSSGRenderRay &ray,
                                   QSSGRenderNode *target = nullptr);

    static PickResultList syncPickSubset(const QSSGRenderLayer &layer,
                                         QSSGBufferManager &bufferManager,
                                         const QSSGRenderRay &ray,
                                         QVarLengthArray<QSSGRenderNode *> subset);

    // Setting this true enables picking for all the models, regardless of
    // the models pickable property.
    static bool isGlobalPickingEnabled(const QSSGRenderer &renderer) { return renderer.m_globalPickingEnabled; }
    static void setGlobalPickingEnabled(QSSGRenderer &renderer, bool isEnabled);

    static void setRenderContextInterface(QSSGRenderer &renderer, QSSGRenderContextInterface *ctx);
};

QT_END_NAMESPACE

#endif // QSSG_RENDERER_P_H
