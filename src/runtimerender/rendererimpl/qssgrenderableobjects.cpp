// Copyright (C) 2008-2012 NVIDIA Corporation.
// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qssgrenderableobjects_p.h"

#include <QtQuick3DRuntimeRender/private/qssgrenderer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrhicustommaterialsystem_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderlight_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderdefaultmaterialshadergenerator_p.h>

QT_BEGIN_NAMESPACE
struct QSSGRenderableImage;
struct QSSGSubsetRenderable;

QSSGSubsetRenderable::QSSGSubsetRenderable(Type type,
                                           QSSGRenderableObjectFlags inFlags,
                                           const QVector3D &inWorldCenterPt,
                                           const QSSGRef<QSSGRenderer> &gen,
                                           const QSSGRenderSubset &inSubset,
                                           const QSSGModelContext &inModelContext,
                                           float inOpacity,
                                           const QSSGRenderGraphObject &mat,
                                           QSSGRenderableImage *inFirstImage,
                                           QSSGShaderDefaultMaterialKey inShaderKey,
                                           const QSSGShaderLightList &inLights)
    : QSSGRenderableObject(type,
                           inFlags,
                           inWorldCenterPt,
                           inModelContext.model.globalTransform,
                           inSubset.bounds,
                           inModelContext.model.m_depthBiasSq)
    , generator(gen)
    , modelContext(inModelContext)
    , subset(inSubset)
    , opacity(inOpacity)
    , material(mat)
    , firstImage(inFirstImage)
    , shaderDescription(inShaderKey)
    , lights(inLights)
{
    if (mat.type == QSSGRenderGraphObject::Type::CustomMaterial)
        depthWriteMode = static_cast<const QSSGRenderCustomMaterial *>(&mat)->m_depthDrawMode;
    else
        depthWriteMode = static_cast<const QSSGRenderDefaultMaterial *>(&mat)->depthDrawMode;
}

QSSGParticlesRenderable::QSSGParticlesRenderable(QSSGRenderableObjectFlags inFlags,
                                                 const QVector3D &inWorldCenterPt,
                                                 const QSSGRef<QSSGRenderer> &gen,
                                                 const QSSGRenderParticles &inParticles,
                                                 QSSGRenderableImage *inFirstImage,
                                                 QSSGRenderableImage *inColorTable,
                                                 const QSSGShaderLightList &inLights,
                                                 float inOpacity)
    : QSSGRenderableObject(Type::Particles,
                           inFlags,
                           inWorldCenterPt,
                           inParticles.globalTransform,
                           QSSGBounds3(),
                           inParticles.m_depthBiasSq)
    , generator(gen)
    , particles(inParticles)
    , firstImage(inFirstImage)
    , colorTable(inColorTable)
    , lights(inLights)
    , opacity(inOpacity)
{
    // Bounds are in global space for particles
    globalBounds = inParticles.m_particleBuffer.bounds();
}

QT_END_NAMESPACE
