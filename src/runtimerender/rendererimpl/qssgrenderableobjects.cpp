/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qssgrenderableobjects_p.h"

#include <QtQuick3DRuntimeRender/private/qssgrenderer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrhicustommaterialsystem_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderlight_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderdefaultmaterialshadergenerator_p.h>

QT_BEGIN_NAMESPACE
struct QSSGRenderableImage;
struct QSSGSubsetRenderable;

QSSGSubsetRenderable::QSSGSubsetRenderable(QSSGRenderableObjectFlags inFlags,
                                           const QVector3D &inWorldCenterPt,
                                           const QSSGRef<QSSGRenderer> &gen,
                                           QSSGRenderSubset &inSubset,
                                           const QSSGModelContext &inModelContext,
                                           float inOpacity,
                                           const QSSGRenderGraphObject &mat,
                                           QSSGRenderableImage *inFirstImage,
                                           QSSGShaderDefaultMaterialKey inShaderKey,
                                           const QSSGDataView<QMatrix4x4> &inBoneGlobals,
                                           const QSSGDataView<QMatrix3x3> &inBoneNormals,
                                           const QSSGShaderLightList &inLights,
                                           const QSSGDataView<float> &inMorphWeights)
    : QSSGRenderableObject(inFlags, inWorldCenterPt, inModelContext.model.globalTransform, inSubset.bounds, inModelContext.model.m_depthBias)
    , generator(gen)
    , modelContext(inModelContext)
    , subset(inSubset)
    , opacity(inOpacity)
    , material(mat)
    , firstImage(inFirstImage)
    , shaderDescription(inShaderKey)
    , boneGlobals(inBoneGlobals)
    , boneNormals(inBoneNormals)
    , lights(inLights)
    , morphWeights(inMorphWeights)
{
    if (mat.type == QSSGRenderGraphObject::Type::CustomMaterial) {
        renderableFlags.setCustomMaterialMeshSubset(true);
        depthWriteMode = static_cast<const QSSGRenderCustomMaterial *>(&mat)->m_depthDrawMode;
    } else {
        renderableFlags.setDefaultMaterialMeshSubset(true);
        depthWriteMode = static_cast<const QSSGRenderDefaultMaterial *>(&mat)->depthDrawMode;
    }
}

QSSGParticlesRenderable::QSSGParticlesRenderable(QSSGRenderableObjectFlags inFlags,
                                                 const QVector3D &inWorldCenterPt,
                                                 const QSSGRef<QSSGRenderer> &gen,
                                                 const QSSGRenderParticles &inParticles,
                                                 QSSGRenderableImage *inFirstImage,
                                                 QSSGRenderableImage *inColorTable,
                                                 const QSSGShaderLightList &inLights,
                                                 float inOpacity)
    : QSSGRenderableObject(inFlags, inWorldCenterPt, inParticles.globalTransform, inParticles.m_particleBuffer.bounds(), inParticles.m_depthBias)
    , generator(gen)
    , particles(inParticles)
    , firstImage(inFirstImage)
    , colorTable(inColorTable)
    , lights(inLights)
    , opacity(inOpacity)
{
    renderableFlags.setParticlesRenderable(true);
}

QT_END_NAMESPACE
