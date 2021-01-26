/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
** Copyright (C) 2021 The Qt Company Ltd.
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
****************************************************************************/

#ifndef QSSG_RENDER_CUSTOM_MATERIAL_RENDER_CONTEXT_H
#define QSSG_RENDER_CUSTOM_MATERIAL_RENDER_CONTEXT_H

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

#include <QtGui/QMatrix4x4>
#include <QtGui/QMatrix3x3>
#include <QtQuick3DRuntimeRender/private/qssgrendershaderkeys_p.h>

QT_BEGIN_NAMESPACE

struct QSSGLayerRenderData;
struct QSSGRenderSubset;
struct QSSGRenderCustomMaterial;
struct QSSGRenderableImage;

struct QSSGCustomMaterialRenderContext
{
    // The lights and camera will not change per layer,
    // so that information can be set once for all the shaders.
    const QSSGRenderLayer &layer;
    const QSSGLayerRenderData &layerData;
    const QVector<QSSGRenderLight *> &lights;
    const QSSGRenderCamera &camera;

    // Per-object information.
    const QSSGRenderModel &model;
    const QSSGRenderSubset &subset;
    const QMatrix4x4 &modelViewProjection;
    const QMatrix4x4 &modelMatrix; ///< model to world transformation
    const QMatrix3x3 &normalMatrix;
    const QSSGRenderCustomMaterial &material;
    const QSSGRef<QSSGRenderTexture2D> depthTexture;
    const QSSGRef<QSSGRenderTexture2D> aoTexture;
    QSSGShaderDefaultMaterialKey materialKey;
    QSSGRenderableImage *firstImage;
    float opacity;

    QSSGCustomMaterialRenderContext(const QSSGRenderLayer &inLayer,
                                      const QSSGLayerRenderData &inData,
                                      const QVector<QSSGRenderLight *> &inLights,
                                      const QSSGRenderCamera &inCamera,
                                      const QSSGRenderModel &inModel,
                                      const QSSGRenderSubset &inSubset,
                                      const QMatrix4x4 &inMvp,
                                      const QMatrix4x4 &inWorld,
                                      const QMatrix3x3 &inNormal,
                                      const QSSGRenderCustomMaterial &inMaterial,
                                      const QSSGRef<QSSGRenderTexture2D> &inDepthTex,
                                      const QSSGRef<QSSGRenderTexture2D> &inAoTex,
                                      QSSGShaderDefaultMaterialKey inMaterialKey,
                                      QSSGRenderableImage *inFirstImage = nullptr,
                                      float inOpacity = 1.0);
    ~QSSGCustomMaterialRenderContext();
};

QT_END_NAMESPACE

#endif
