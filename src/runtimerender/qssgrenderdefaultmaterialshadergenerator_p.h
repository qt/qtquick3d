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
#include <QtQuick3DRuntimeRender/private/qssgrenderableimage_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershaderkeys_p.h>

QT_BEGIN_NAMESPACE

struct QSSGRenderableImage;
struct QSSGShaderDefaultMaterialKeyProperties;
struct QSSGShaderDefaultMaterialKey;
struct QSSGLayerGlobalRenderProperties;
struct QSSGMaterialVertexPipeline;

struct Q_QUICK3DRUNTIMERENDER_EXPORT QSSGMaterialShaderGenerator
{
    struct LightVariableNames
    {
        QByteArray lightColor;
        QByteArray lightSpecularColor;
        QByteArray lightAttenuation;
        QByteArray lightConstantAttenuation;
        QByteArray lightLinearAttenuation;
        QByteArray lightQuadraticAttenuation;
        QByteArray normalizedDirection;
        QByteArray lightDirection;
        QByteArray lightPos;
        QByteArray lightConeAngle;
        QByteArray lightInnerConeAngle;
        QByteArray relativeDistance;
        QByteArray relativeDirection;
        QByteArray spotAngle;
    };

    struct ShadowVariableNames
    {
        QByteArray shadowMapStem;
        QByteArray shadowCubeStem;
        QByteArray shadowMatrixStem;
        QByteArray shadowCoordStem;
        QByteArray shadowControlStem;
    };

    ~QSSGMaterialShaderGenerator() = default;

    static const char* getSamplerName(QSSGRenderableImage::Type type);

    static QSSGRef<QSSGRhiShaderPipeline> generateMaterialRhiShader(const QByteArray &inShaderKeyPrefix,
                                                                    QSSGMaterialVertexPipeline &vertexGenerator,
                                                                    const QSSGShaderDefaultMaterialKey &key,
                                                                    QSSGShaderDefaultMaterialKeyProperties &inProperties,
                                                                    const ShaderFeatureSetList &inFeatureSet,
                                                                    const QSSGRenderGraphObject &inMaterial,
                                                                    const QSSGShaderLightList &inLights,
                                                                    QSSGRenderableImage *inFirstImage, const QSSGRef<QSSGShaderLibraryManager> &shaderLibraryManager,
                                                                    const QSSGRef<QSSGShaderCache> &theCache);

    static void setRhiMaterialProperties(const QSSGRenderContextInterface &,
                                         QSSGRef<QSSGRhiShaderPipeline> &shaders,
                                         char *ubufData,
                                         QSSGRhiGraphicsPipelineState *inPipelineState,
                                         const QSSGRenderGraphObject &inMaterial,
                                         const QSSGShaderDefaultMaterialKey &inKey,
                                         QSSGShaderDefaultMaterialKeyProperties &inProperties,
                                         QSSGRenderCamera &inCamera,
                                         const QMatrix4x4 &inModelViewProjection,
                                         const QMatrix3x3 &inNormalMatrix,
                                         const QMatrix4x4 &inGlobalTransform,
                                         const QMatrix4x4 &clipSpaceCorrMatrix,
                                         const QSSGDataView<QMatrix4x4> &inBoneGlobals,
                                         const QSSGDataView<QMatrix3x3> &inBoneNormals,
                                         QSSGRenderableImage *inFirstImage,
                                         float inOpacity,
                                         const QSSGLayerGlobalRenderProperties &inRenderProperties,
                                         const QSSGShaderLightList &inLights,
                                         bool receivesShadows,
                                         const QVector2D *shadowDepthAdjust);

    static const char *directionalLightProcessorArgumentList();
    static const char *pointLightProcessorArgumentList();
    static const char *spotLightProcessorArgumentList();
    static const char *ambientLightProcessorArgumentList();
    static const char *specularLightProcessorArgumentList();
    static const char *shadedFragmentMainArgumentList();
    static const char *vertexMainArgumentList();

private:
    QSSGMaterialShaderGenerator() = delete;
    Q_DISABLE_COPY(QSSGMaterialShaderGenerator)
};

QT_END_NAMESPACE
#endif
