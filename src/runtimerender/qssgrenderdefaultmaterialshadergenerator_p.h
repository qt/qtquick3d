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

#ifndef QSSG_RENDER_DEFAULT_MATERIAL_SHADER_GENERATOR_H
#define QSSG_RENDER_DEFAULT_MATERIAL_SHADER_GENERATOR_H

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
#include <QtQuick3DRuntimeRender/private/qssgrendershadowmap_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderlight_p.h>

QT_BEGIN_NAMESPACE

struct QSSGRenderableImage;

struct Q_QUICK3DRUNTIMERENDER_EXPORT QSSGMaterialShaderGenerator final : public QSSGMaterialShaderGeneratorInterface
{
    const QSSGRenderDefaultMaterial *m_currentMaterial;

    QSSGRef<QSSGRenderShadowMap> m_shadowMapManager;
    bool m_lightsAsSeparateUniforms = false;

    QHash<quint32, ImageVariableNames> m_imageVariableNames;

    QByteArray m_lightColor;
    QByteArray m_lightSpecularColor;
    QByteArray m_lightAttenuation;
    QByteArray m_lightConstantAttenuation;
    QByteArray m_lightLinearAttenuation;
    QByteArray m_lightQuadraticAttenuation;
    QByteArray m_normalizedDirection;
    QByteArray m_lightDirection;
    QByteArray m_lightPos;
    QByteArray m_lightUp;
    QByteArray m_lightRt;
    QByteArray m_lightConeAngle;
    QByteArray m_lightInnerConeAngle;
    QByteArray m_relativeDistance;
    QByteArray m_relativeDirection;
    QByteArray m_spotAngle;

    QByteArray m_shadowMapStem;
    QByteArray m_shadowCubeStem;
    QByteArray m_shadowMatrixStem;
    QByteArray m_shadowCoordStem;
    QByteArray m_shadowControlStem;

    ~QSSGMaterialShaderGenerator() = default;

    const QSSGRef<QSSGProgramGenerator> &programGenerator();
    QSSGVertexPipelineBase &vertexGenerator();
    QSSGStageGeneratorBase &fragmentGenerator();
    const QSSGRenderDefaultMaterial *material();
    bool hasTransparency();

    void setupImageVariableNames(quint32 imageIdx);

    QByteArray textureCoordVariableName(quint32 uvSet) const;

    ImageVariableNames getImageVariableNames(quint32 inIdx) override;

    void addLocalVariable(QSSGStageGeneratorBase &inGenerator, const QByteArray &inName, const QByteArray &inType);

    QByteArray uvTransform(const QByteArray& imageRotations, const QByteArray& imageOffsets) const;

    void generateImageUVCoordinates(QSSGVertexPipelineBase &vertexShader, QSSGRenderableImage &image, quint32 idx, quint32 uvSet = 0) override;

    void generateImageUVSampler(quint32 idx, quint32 uvSet = 0);

    void outputSpecularEquation(QSSGRenderDefaultMaterial::MaterialSpecularModel inSpecularModel,
                                QSSGStageGeneratorBase &fragmentShader,
                                const QByteArray &inLightDir,
                                const QByteArray &inLightSpecColor);

    void outputDiffuseAreaLighting(QSSGStageGeneratorBase &infragmentShader, const QByteArray &inPos, const QByteArray &inLightPrefix);

    void outputSpecularAreaLighting(QSSGStageGeneratorBase &infragmentShader,
                                    const QByteArray &inPos,
                                    const QByteArray &inView,
                                    const QByteArray &inLightSpecColor);

    void addTranslucencyIrradiance(QSSGStageGeneratorBase &infragmentShader,
                                   QSSGRenderableImage *image,
                                   bool areaLight);

    void setupShadowMapVariableNames(size_t lightIdx);

    void addShadowMapContribution(QSSGStageGeneratorBase &inLightShader, quint32 lightIndex, QSSGRenderLight::Type inType);

    void maybeAddMaterialFresnel(QSSGStageGeneratorBase &fragmentShader, QSSGDataView<quint32> inKey, bool &fragmentHasSpecularAmount, bool hasMetalness);
    void setupLightVariableNames(qint32 lightIdx, QSSGRenderLight &inLight);

    void generateShadowMapOcclusion(quint32 lightIdx, bool inShadowEnabled, QSSGRenderLight::Type inType);

    void addSpecularAmount(QSSGStageGeneratorBase &fragmentShader, bool &fragmentHasSpecularAmount, bool reapply = false);

    void generateFragmentShader(QSSGShaderDefaultMaterialKey &inKey);

    QSSGRef<QSSGRhiShaderStages> generateMaterialRhiShader(const QByteArray &inShaderPrefix);

    QSSGRef<QSSGRhiShaderStages> generateRhiShaderStages(const QSSGRenderContextInterface &,
                                                         const QSSGRenderGraphObject &inMaterial,
                                                         QSSGShaderDefaultMaterialKey inShaderDescription,
                                                         QSSGVertexPipelineBase &inVertexPipeline,
                                                         const ShaderFeatureSetList &inFeatureSet,
                                                         const QVector<QSSGRenderLight *> &inLights,
                                                         QSSGRenderableImage *inFirstImage,
                                                         bool inHasTransparency,
                                                         const QByteArray &inVertexPipelineName,
                                                         const QByteArray & = QByteArray()) override;

    void setRhiImageShaderVariables(const QSSGRef<QSSGRhiShaderStagesWithResources> &inShader, QSSGRenderableImage &inImage, quint32 idx);

    void setRhiMaterialProperties(const QSSGRenderContextInterface &,
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
                                  bool receivesShadows) override;
};

QT_END_NAMESPACE
#endif
