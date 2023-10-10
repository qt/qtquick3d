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
#include <QtQuick3DRuntimeRender/private/qssgrenderpickresult_p.h>

QT_BEGIN_NAMESPACE

class QSSGRhiQuadRenderer;
class QSSGRhiCubeRenderer;
class QSSGBufferManager;
class QSSGShaderCache;
class QSSGProgramGenerator;
class QSSGShaderLibraryManager;
struct QSSGRenderItem2D;
struct QSSGReflectionMapEntry;
struct QSSGRenderRay;

// Steps needed to render:
// 1.  beginFrame - Reset the per-frame allocator
// 2.  Make sure state is set (viewport, windowDimensions, dpr, sceneColor, scissorRect)
// 3.  prepareLayerForRenderer - process the layer scene (prepare the scene for rendering)
// 4.  rhiPrepare - start rendering necessary sub-scenes and prepare resources
// 5.  rhiRender - render the layer scene
// 6.  endFrame

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRenderer
{
    Q_DISABLE_COPY(QSSGRenderer)
    using PickResultList = QVarLengthArray<QSSGRenderPickResult, 20>; // Lets assume most items are filtered out already
public:
    QSSGRenderer();
    ~QSSGRenderer();

    // Returns true if this layer or a sibling was dirty.
    bool prepareLayerForRender(QSSGRenderLayer &inLayer);

    void rhiPrepare(QSSGRenderLayer &inLayer);
    void rhiRender(QSSGRenderLayer &inLayer);

    // Clients need to call this every frame in order for various subsystems to release
    // temporary per-frame allocated objects.
    void beginFrame(QSSGRenderLayer *layer, bool allowRecursion = true);

    // When allowRecursion is true, the cleanup is only done when all
    // beginFrames got their corresponding endFrame. This is indicated by the
    // return value (false if nothing's been done due to pending "frames")
    bool endFrame(QSSGRenderLayer *layer, bool allowRecursion = true);

    // Get the number of times EndFrame has been called
    quint32 frameCount() const { return m_frameCount; }

    void setViewport(QRect inViewport) { m_viewport = inViewport; }
    QRect viewport() const { return m_viewport; }

    void setDpr(float dpr) { m_dpr = dpr; }
    float dpr() const { return m_dpr; }

    void setScissorRect(QRect inScissorRect) { m_scissorRect = inScissorRect; }
    QRect scissorRect() const { return m_scissorRect; }

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

    static QSSGRhiShaderPipelinePtr generateRhiShaderPipelineImpl(QSSGSubsetRenderable &renderable, QSSGShaderLibraryManager &shaderLibraryManager,
                                                                  QSSGShaderCache &shaderCache,
                                                                  QSSGProgramGenerator &shaderProgramGenerator,
                                                                  const QSSGShaderDefaultMaterialKeyProperties &shaderKeyProperties,
                                                                  const QSSGShaderFeatures &featureSet,
                                                                  QByteArray &shaderString);

    QSSGRhiShaderPipelinePtr getShaderPipelineForDefaultMaterial(QSSGSubsetRenderable &inRenderable,
                                                                 const QSSGShaderFeatures &inFeatureSet);

    QSSGRenderContextInterface *contextInterface() const { return m_contextInterface; }

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

    void cleanupResources(QList<QSSGRenderGraphObject*> &resources);
    void cleanupResources(QSet<QSSGRenderGraphObject*> &resources);

private:
    friend class QQuick3DSceneRenderer;
    friend class QSSGRenderContextInterface;
    friend class QSSGLayerRenderData;
    friend class QQuick3DWindowAttachment;

    QSSGLayerRenderData *getOrCreateLayerRenderData(QSSGRenderLayer &layer);
    void beginLayerRender(QSSGLayerRenderData &inLayer);
    void endLayerRender();

    void addMaterialDirtyClear(QSSGRenderGraphObject *material);

    void setRenderContextInterface(QSSGRenderContextInterface *ctx);

    void cleanupUnreferencedBuffers(QSSGRenderLayer *inLayer);
    void resetResourceCounters(QSSGRenderLayer *inLayer);
    void releaseCachedResources();
    QSSGRhiShaderPipelinePtr generateRhiShaderPipeline(QSSGSubsetRenderable &inRenderable, const QSSGShaderFeatures &inFeatureSet);

    QSSGRenderContextInterface *m_contextInterface = nullptr; //  We're own by the context interface

    bool m_globalPickingEnabled = false;

    // Temporary information stored only when rendering a particular layer.
    QSSGLayerRenderData *m_currentLayer = nullptr;
    QByteArray m_generatedShaderString;

    QSet<QSSGRenderGraphObject *> m_materialClearDirty;

    QSSGRhiQuadRenderer *m_rhiQuadRenderer = nullptr;
    QSSGRhiCubeRenderer *m_rhiCubeRenderer = nullptr;

    quint32 m_activeFrameRef = 0;
    quint32 m_frameCount = 0;

    // Viewport that this render context should use
    QRect m_viewport;
    float m_dpr = 1.0;
    QRect m_scissorRect;
    QColor m_sceneColor;
};

QT_END_NAMESPACE

#endif
