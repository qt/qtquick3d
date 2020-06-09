/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#ifndef QSSG_RENDER_CUSTOM_MATERIAL_SHADER_GENERATOR_H
#define QSSG_RENDER_CUSTOM_MATERIAL_SHADER_GENERATOR_H

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

#include <QtQuick3DRuntimeRender/private/qssgrendermaterialshadergenerator_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercodegenerator_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderableimage_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderlight_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershaderkeys_p.h>

QT_BEGIN_NAMESPACE

class QSSGRenderShadowMap;
struct QSSGRenderableImage;
struct QSSGRenderCustomMaterial;
struct QSSGShaderDefaultMaterialKey;
struct QSSGVertexPipelineBase;

struct Q_QUICK3DRUNTIMERENDER_EXPORT QSSGCustomMaterialShaderGenerator
{
    QAtomicInt ref;
    struct ImageVariableNames
    {
        QByteArray imageSampler;
        QByteArray imageFragCoords;
        QByteArray imageFragCoordsTemp;
        QByteArray imageOffsets;
        QByteArray imageRotations;
        QByteArray imageSamplerSize;
    };

    QSSGCustomMaterialShaderGenerator() = default;

    bool m_hasTransparency = false;
    QSSGShaderDefaultMaterialKey *m_currentKey = nullptr;
    QSSGVertexPipelineBase *m_currentPipeline = nullptr;
    ShaderFeatureSetList m_currentFeatureSet;
    QSSGShaderLightList m_lights;
    QSSGRenderableImage *m_firstImage = nullptr;
    QSSGShaderDefaultMaterialKeyProperties m_defaultMaterialShaderKeyProperties;

    const QSSGRenderCustomMaterial *m_currentMaterial = nullptr;

    QByteArray m_imageSampler;
    QByteArray m_imageFragCoords;
    QByteArray m_imageRotScale;
    QByteArray m_imageOffset;

    QSSGShaderDefaultMaterialKey &key() { return *m_currentKey; }
    const QSSGRef<QSSGProgramGenerator> &programGenerator() const;
    QSSGVertexPipelineBase &vertexGenerator() const;
    QSSGStageGeneratorBase &fragmentGenerator();
    const QSSGRenderCustomMaterial &material();
    bool hasTransparency() const;

    quint32 convertTextureTypeValue(QSSGRenderableImage::Type inType);

    ImageVariableNames getImageVariableNames(uint imageIdx);

    bool generateVertexShader(const QSSGRenderContextInterface &renderContext, QSSGShaderDefaultMaterialKey &, const QByteArray &inShaderPathName);

    void setRhiLightBufferData(QSSGLightSourceShader *lightData, QSSGRenderLight *light, float clipFar, int shadowIdx);

    void setRhiMaterialProperties(const QSSGRenderContextInterface &renderContext,
                                  QSSGRef<QSSGRhiShaderStagesWithResources> &shaders,
                                  QSSGRhiGraphicsPipelineState *inPipelineState,
                                  const QSSGRenderGraphObject &inMaterial,
                                  const QVector2D &inCameraVec,
                                  const QMatrix4x4 &inModelViewProjection,
                                  const QMatrix3x3 &inNormalMatrix,
                                  const QMatrix4x4 &inGlobalTransform,
                                  const QMatrix4x4 &clipSpaceCorrMatrix,
                                  const QSSGDataView<QMatrix4x4> &inBones,
                                  QSSGRenderableImage *inFirstImage,
                                  float inOpacity,
                                  const QSSGLayerGlobalRenderProperties &inRenderProperties,
                                  const QSSGShaderLightList &inLights,
                                  bool receivesShadows = true);

    void generateLightmapIndirectFunc(QSSGStageGeneratorBase &inFragmentShader, QSSGRenderImage *pEmissiveLightmap);

    void generateLightmapRadiosityFunc(QSSGStageGeneratorBase &inFragmentShader, QSSGRenderImage *pRadiosityLightmap);

    void generateLightmapShadowFunc(QSSGStageGeneratorBase &inFragmentShader, QSSGRenderImage *pBakedShadowMap);

    void generateLightmapIndirectSetupCode(QSSGStageGeneratorBase &inFragmentShader,
                                           QSSGRenderableImage *pIndirectLightmap,
                                           QSSGRenderableImage *pRadiosityLightmap);

    void generateLightmapShadowCode(QSSGStageGeneratorBase &inFragmentShader, QSSGRenderableImage *pBakedShadowMap);

    void applyEmissiveMask(QSSGStageGeneratorBase &inFragmentShader, QSSGRenderImage *pEmissiveMaskMap);

    void registerNonSnippetUnconditionalUniforms(QSSGStageGeneratorBase &fs);

    bool generateFragmentShader(const QSSGRenderContextInterface &renderContext, QSSGShaderDefaultMaterialKey &inKey,
                                const QByteArray &inShaderPathName,
                                bool hasCustomVertShader);

    QSSGRef<QSSGRhiShaderStages> generateCustomMaterialRhiShader(const QSSGRenderContextInterface &renderContext, const QByteArray &inShaderPrefix,
                                                                 const QByteArray &inCustomMaterialName);

    QSSGRef<QSSGRhiShaderStages> generateRhiShaderStages(const QSSGRenderContextInterface &renderContext,
                                                         const QSSGRenderGraphObject &inMaterial,
                                                         QSSGShaderDefaultMaterialKey inShaderDescription,
                                                         QSSGVertexPipelineBase &inVertexPipeline,
                                                         const ShaderFeatureSetList &inFeatureSet,
                                                         const QSSGShaderLightList &inLights,
                                                         QSSGRenderableImage *inFirstImage,
                                                         bool inHasTransparency,
                                                         const QByteArray &inShaderPrefix,
                                                         const QByteArray &inCustomMaterialName = QByteArray());
private:
    Q_DISABLE_COPY(QSSGCustomMaterialShaderGenerator)
};

QT_END_NAMESPACE

#endif
