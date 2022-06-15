// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QSSG_RHI_PARTICLES_H
#define QSSG_RHI_PARTICLES_H

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

#include <QtQuick3DRuntimeRender/private/qtquick3druntimerenderglobal_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderableobjects_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderparticles_p.h>
#include <QtGui/QMatrix4x4>
#include <QtGui/QMatrix3x3>

QT_BEGIN_NAMESPACE

class QSSGLayerRenderData;
struct QSSGRenderableImage;
struct QSSGRenderLayer;
struct QSSGRenderLight;
struct QSSGRenderCamera;
struct QSSGReflectionMapEntry;
class QRhiTexture;

class QSSGParticleRenderer
{
public:
    static void updateUniformsForParticles(QSSGRef<QSSGRhiShaderPipeline> &shaderPipeline,
                                         QSSGRhiContext *rhiCtx,
                                         char *ubufData,
                                         QSSGParticlesRenderable &renderable,
                                         QSSGRenderCamera &inCamera);
    static void updateUniformsForParticleModel(QSSGRef<QSSGRhiShaderPipeline> &shaderPipeline,
                                               char *ubufData,
                                               const QSSGRenderModel *model, quint32 offset);

    static void rhiPrepareRenderable(QSSGRef<QSSGRhiShaderPipeline> &shaderPipeline,
                                     QSSGRhiContext *rhiCtx,
                                     QSSGRhiGraphicsPipelineState *ps,
                                     QSSGParticlesRenderable &renderable,
                                     QSSGLayerRenderData &inData,
                                     QRhiRenderPassDescriptor *renderPassDescriptor,
                                     int samples,
                                     QSSGRenderCamera *camera = nullptr,
                                     int cubeFace = -1,
                                     QSSGReflectionMapEntry *entry = nullptr);
    static void rhiRenderRenderable(QSSGRhiContext *rhiCtx,
                                    QSSGParticlesRenderable &renderable,
                                    QSSGLayerRenderData &inData,
                                    bool *needsSetViewport,
                                    int cubeFace,
                                    QSSGRhiGraphicsPipelineState *state);
    static void prepareParticlesForModel(QSSGRef<QSSGRhiShaderPipeline> &shaderPipeline,
                                         QSSGRhiContext *rhiCtx,
                                         QSSGRhiShaderResourceBindingList &bindings,
                                         const QSSGRenderModel *model);
};

QT_END_NAMESPACE

#endif
