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
#include <ssg/qssgrenderer.h>

QT_BEGIN_NAMESPACE

class QSSGShaderCache;
class QSSGProgramGenerator;
class QSSGShaderLibraryManager;
class QSSGBufferManager;
struct QSSGRenderNode;
struct QSSGRenderItem2D;
struct QSSGRenderRay;
struct QSSGSubsetRenderable;
struct QSSGShaderDefaultMaterialKeyProperties;
struct QSSGShaderFeatures;

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

    static QSSGRenderPickResult syncPick(const QSSGRenderContextInterface &ctx,
                                         const QSSGRenderLayer &layer,
                                         const QSSGRenderRay &ray,
                                         QSSGRenderNode *target = nullptr);

    // Setting this true enables picking for all the models, regardless of
    // the models pickable property.
    static bool isGlobalPickingEnabled(const QSSGRenderer &renderer) { return renderer.m_globalPickingEnabled; }
    static void setGlobalPickingEnabled(QSSGRenderer &renderer, bool isEnabled);

    static void setRenderContextInterface(QSSGRenderer &renderer, QSSGRenderContextInterface *ctx);
};

QT_END_NAMESPACE

#endif // QSSG_RENDERER_P_H
