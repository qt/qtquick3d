/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
******************************************************************************/

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

struct QSSGLayerRenderData;
struct QSSGRenderableImage;
struct QSSGRenderLayer;
struct QSSGRenderLight;
struct QSSGRenderCamera;
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
                                     int samples);
    static void rhiRenderRenderable(QSSGRhiContext *rhiCtx,
                                    QSSGParticlesRenderable &renderable,
                                    QSSGLayerRenderData &inData,
                                    bool *needsSetViewport);
    static void prepareParticlesForModel(QSSGRef<QSSGRhiShaderPipeline> &shaderPipeline,
                                         QSSGRhiContext *rhiCtx,
                                         QSSGRhiShaderResourceBindingList &bindings,
                                         const QSSGRenderModel *model);
};

QT_END_NAMESPACE

#endif
