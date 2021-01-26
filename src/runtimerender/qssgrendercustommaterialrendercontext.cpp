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


#include "qssgrendercustommaterialrendercontext_p.h"

#include <QtQuick3DRender/private/qssgrendertexture2d_p.h>

QT_BEGIN_NAMESPACE

QSSGCustomMaterialRenderContext::QSSGCustomMaterialRenderContext(const QSSGRenderLayer &inLayer,
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
                                                                     QSSGRenderableImage *inFirstImage,
                                                                     float inOpacity)
    : layer(inLayer)
    , layerData(inData)
    , lights(inLights)
    , camera(inCamera)
    , model(inModel)
    , subset(inSubset)
    , modelViewProjection(inMvp)
    , modelMatrix(inWorld)
    , normalMatrix(inNormal)
    , material(inMaterial)
    , depthTexture(inDepthTex)
    , aoTexture(inAoTex)
    , materialKey(inMaterialKey)
    , firstImage(inFirstImage)
    , opacity(inOpacity)
{
}

QSSGCustomMaterialRenderContext::~QSSGCustomMaterialRenderContext() = default;

QT_END_NAMESPACE
