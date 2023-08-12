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
                                                                  const QSSGShaderDefaultMaterialKeyProperties &shaderKeyProperties,
                                                                  const QSSGShaderFeatures &featureSet,
                                                                  QByteArray &shaderString);

    QSSGRhiShaderPipelinePtr getShaderPipelineForDefaultMaterial(QSSGSubsetRenderable &inRenderable,
                                                                 const QSSGShaderFeatures &inFeatureSet);

    QSSGLayerGlobalRenderProperties getLayerGlobalRenderProperties();

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

private:
    friend class QSSGRenderContextInterface;
    friend class QSSGLayerRenderData;

    void setRenderContextInterface(QSSGRenderContextInterface *ctx);

    void releaseCachedResources();
    QSSGRhiShaderPipelinePtr generateRhiShaderPipeline(QSSGSubsetRenderable &inRenderable, const QSSGShaderFeatures &inFeatureSet);

    QSSGRenderContextInterface *m_contextInterface = nullptr; //  We're own by the context interface



    bool m_globalPickingEnabled = false;

    // Temporary information stored only when rendering a particular layer.
    QSSGLayerRenderData *m_currentLayer = nullptr;
    QMatrix4x4 m_viewProjection;
    QByteArray m_generatedShaderString;

    QSet<QSSGRenderGraphObject *> m_materialClearDirty;

    QSSGRhiQuadRenderer *m_rhiQuadRenderer = nullptr;
    QSSGRhiCubeRenderer *m_rhiCubeRenderer = nullptr;

    QHash<QSSGShaderMapKey, QSSGRhiShaderPipelinePtr> m_shaderMap;
};

QT_END_NAMESPACE

#endif
