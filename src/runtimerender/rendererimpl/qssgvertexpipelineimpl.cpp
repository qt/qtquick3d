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

#include "qssgvertexpipelineimpl_p.h"

#include <QtQuick3DRuntimeRender/private/qssgrenderer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderlight_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercontextcore_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercache_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershaderlibrarymanager_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercodegenerator_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderdefaultmaterialshadergenerator_p.h>

// This adds support for the depth buffers in the shader so we can do depth
// texture-based effects.
#define QSSG_RENDER_SUPPORT_DEPTH_TEXTURE 1

QT_BEGIN_NAMESPACE

QSSGSubsetMaterialVertexPipeline::QSSGSubsetMaterialVertexPipeline(const QSSGRef<QSSGProgramGenerator> &inProgram,
                                                                   const QSSGShaderDefaultMaterialKeyProperties &materialProperties,
                                                                   QSSGSubsetRenderable &inRenderable)
    : QSSGVertexPipelineBase(inProgram)
    , defaultMaterialShaderKeyProperties(materialProperties)
    , renderable(inRenderable)
{
    m_hasSkinning = (inRenderable.boneGlobals.size() > 0);
}

void QSSGSubsetMaterialVertexPipeline::beginVertexGeneration()
{
    QSSGShaderGeneratorStageFlags theStages(QSSGProgramGenerator::defaultFlags());
    programGenerator()->beginProgram(theStages);

    // Open up each stage.
    QSSGStageGeneratorBase &vertexShader(vertex());
    vertexShader.addIncoming("attr_pos", "vec3");
    if (m_hasSkinning) {
        vertexShader.addIncoming("attr_joints", "uvec4");
        vertexShader.addIncoming("attr_weights", "vec4");
        vertexShader.addUniformArray("boneTransforms", "mat4", renderable.boneGlobals.mSize);
        vertexShader.addUniformArray("boneNormalTransforms", "mat3", renderable.boneNormals.mSize);

        vertexShader << "mat4 getSkinMatrix()"
                     << "\n"
                     << "{"
                     << "\n";
        // If some formats needs these weights to be normalized
        // it should be applied here
        vertexShader << "    return boneTransforms[attr_joints.x] * attr_weights.x"
                     << "\n"
                     << "       + boneTransforms[attr_joints.y] * attr_weights.y"
                     << "\n"
                     << "       + boneTransforms[attr_joints.z] * attr_weights.z"
                     << "\n"
                     << "       + boneTransforms[attr_joints.w] * attr_weights.w;"
                     << "\n"
                     << "}"
                     << "\n";
        vertexShader << "mat3 getSkinNormalMatrix()"
                     << "\n"
                     << "{"
                     << "\n";
        vertexShader << "    return boneNormalTransforms[attr_joints.x] * attr_weights.x"
                     << "\n"
                     << "       + boneNormalTransforms[attr_joints.y] * attr_weights.y"
                     << "\n"
                     << "       + boneNormalTransforms[attr_joints.z] * attr_weights.z"
                     << "\n"
                     << "       + boneNormalTransforms[attr_joints.w] * attr_weights.w;"
                     << "\n"
                     << "}"
                     << "\n";
    }
    vertexShader << "void main()"
                 << "\n"
                 << "{"
                 << "\n";
    vertexShader << "    vec3 uTransform;"
                 << "\n";
    vertexShader << "    vec3 vTransform;"
                 << "\n";

    vertexShader.addUniform("modelViewProjection", "mat4");
    if (m_hasSkinning) {
        vertexShader.append("    vec4 skinnedPos;");
        vertexShader.append("    if (attr_weights != vec4(0.0))");
        vertexShader.append("        skinnedPos = getSkinMatrix() * vec4(attr_pos, 1.0);");
        vertexShader.append("    else");
        vertexShader.append("        skinnedPos = vec4(attr_pos, 1.0);");
        vertexShader.append("    gl_Position = modelViewProjection * skinnedPos;");
    } else {
        vertexShader.append("    gl_Position = modelViewProjection * vec4(attr_pos, 1.0);");
    }
}

void QSSGSubsetMaterialVertexPipeline::beginFragmentGeneration()
{
    fragment().addUniform("material_properties", "vec4");
    fragment() << "void main()"
               << "\n"
               << "{"
               << "\n";
    // We do not pass object opacity through the pipeline.
    fragment() << "    float objectOpacity = material_properties.a;"
               << "\n";
}

void QSSGSubsetMaterialVertexPipeline::assignOutput(const QByteArray &inVarName, const QByteArray &inVarValue)
{
    vertex() << "    " << inVarName << " = " << inVarValue << ";\n";
}

void QSSGSubsetMaterialVertexPipeline::doGenerateUVCoords(quint32 inUVSet, const QSSGShaderDefaultMaterialKey &inKey)
{
    Q_ASSERT(inUVSet == 0 || inUVSet == 1);

    if (inUVSet == 0) {
        const bool meshHasTexCoord0 = defaultMaterialShaderKeyProperties.m_vertexAttributes.getBitValue(
                    QSSGShaderKeyVertexAttribute::TexCoord0, inKey);
        if (meshHasTexCoord0)
            vertex().addIncoming("attr_uv0", "vec2");
        else
            vertex().append("    vec2 attr_uv0 = vec2(0.0);");
        vertex() << "    varTexCoord0 = attr_uv0;"
                 << "\n";
    } else if (inUVSet == 1) {
        const bool meshHasTexCoord1 = defaultMaterialShaderKeyProperties.m_vertexAttributes.getBitValue(
                    QSSGShaderKeyVertexAttribute::TexCoord1, inKey);
        if (meshHasTexCoord1)
            vertex().addIncoming("attr_uv1", "vec2");
        else
            vertex().append("    vec2 attr_uv1 = vec2(0.0);");
        vertex() << "    varTexCoord1 = attr_uv1;"
                 << "\n";
    }
}

void QSSGSubsetMaterialVertexPipeline::doGenerateWorldNormal(const QSSGShaderDefaultMaterialKey &inKey)
{
    const bool meshHasNormals = defaultMaterialShaderKeyProperties.m_vertexAttributes.getBitValue(
                QSSGShaderKeyVertexAttribute::Normal, inKey);

    QSSGStageGeneratorBase &vertexGenerator(vertex());
    if (meshHasNormals)
        vertexGenerator.addIncoming("attr_norm", "vec3");
    else
        vertexGenerator.append("    vec3 attr_norm = vec3(0.0);");
    vertexGenerator.addUniform("normalMatrix", "mat3");
    if (!m_hasSkinning) {
        vertexGenerator.append("    vec3 world_normal = normalize(normalMatrix * attr_norm).xyz;");
    } else {
        vertexGenerator.append("    vec3 skinned_norm = attr_norm;");
        vertexGenerator.append("    if (attr_weights != vec4(0.0))");
        vertexGenerator.append("        skinned_norm = getSkinNormalMatrix() * attr_norm;");
        vertexGenerator.append("    vec3 world_normal = normalize(normalMatrix * skinned_norm).xyz;");
    }
    vertexGenerator.append("    varNormal = world_normal;");
}

void QSSGSubsetMaterialVertexPipeline::doGenerateObjectNormal()
{
    addInterpolationParameter("varObjectNormal", "vec3");
    vertex().append("    varObjectNormal = attr_norm;");
}

void QSSGSubsetMaterialVertexPipeline::doGenerateWorldPosition()
{
    if (!m_hasSkinning) {
        vertex().append("    vec3 local_model_world_position = (modelMatrix * vec4(attr_pos, 1.0)).xyz;");
    } else {
        vertex().append("    vec3 local_model_world_position = (modelMatrix * skinnedPos).xyz;");
    }
}

void QSSGSubsetMaterialVertexPipeline::doGenerateVarTangentAndBinormal(const QSSGShaderDefaultMaterialKey &inKey)
{
    const bool meshHasTangents = defaultMaterialShaderKeyProperties.m_vertexAttributes.getBitValue(
                QSSGShaderKeyVertexAttribute::Tangent, inKey);
    const bool meshHasBinormals = defaultMaterialShaderKeyProperties.m_vertexAttributes.getBitValue(
                QSSGShaderKeyVertexAttribute::Binormal, inKey);

    if (meshHasTangents)
        vertex().addIncoming("attr_textan", "vec3");
    else
        vertex() << "    vec3 attr_textan = vec3(0.0);\n";

    if (meshHasBinormals)
        vertex().addIncoming("attr_binormal", "vec3");
    else
        vertex() << "    vec3 attr_binormal = vec3(0.0);\n";

    if (!m_hasSkinning) {
        vertex() << "    varTangent = (modelMatrix * vec4(attr_textan, 0.0)).xyz;"
                 << "\n"
                 << "    varBinormal = (modelMatrix * vec4(attr_binormal, 0.0)).xyz;"
                 << "\n";
    } else {
        vertex() << "    vec4 skinnedTangent = vec4(attr_textan, 0.0);"
                 << "\n"
                 << "    vec4 skinnedBinorm = vec4(attr_binormal, 0.0);"
                 << "\n"
                 << "    if (attr_weights != vec4(0.0)) {"
                 << "\n"
                 << "       skinnedTangent = getSkinMatrix() * skinnedTangent;"
                 << "\n"
                 << "       skinnedBinorm = getSkinMatrix() * skinnedBinorm;"
                 << "\n"
                 << "    }"
                 << "\n"
                 << "    varTangent = (modelMatrix * skinnedTangent).xyz;"
                 << "\n"
                 << "    varBinormal = (modelMatrix * skinnedBinorm).xyz;"
                 << "\n";
    }
}

void QSSGSubsetMaterialVertexPipeline::doGenerateVertexColor(const QSSGShaderDefaultMaterialKey &inKey)
{
    const bool meshHasColors = defaultMaterialShaderKeyProperties.m_vertexAttributes.getBitValue(
                QSSGShaderKeyVertexAttribute::Color, inKey);
    if (meshHasColors)
        vertex().addIncoming("attr_color", "vec4");
    else
        vertex().append("    vec4 attr_color = vec4(0.0, 0.0, 0.0, 1.0);");
    vertex().append("    varColor = attr_color;");
}

bool QSSGSubsetMaterialVertexPipeline::hasAttributeInKey(QSSGShaderKeyVertexAttribute::VertexAttributeBits inAttr, const QSSGShaderDefaultMaterialKey &inKey)
{
    return defaultMaterialShaderKeyProperties.m_vertexAttributes.getBitValue(inAttr, inKey);
}

void QSSGSubsetMaterialVertexPipeline::endVertexGeneration(bool customShader)
{
    if (!customShader)
        vertex().append("}");
}

void QSSGSubsetMaterialVertexPipeline::endFragmentGeneration(bool customShader)
{
    if (!customShader)
        fragment().append("}");
}

void QSSGSubsetMaterialVertexPipeline::addInterpolationParameter(const QByteArray &inName, const QByteArray &inType)
{
    m_interpolationParameters.insert(inName, inType);
    vertex().addOutgoing(inName, inType);
    fragment().addIncoming(inName, inType);
}

QSSGStageGeneratorBase &QSSGSubsetMaterialVertexPipeline::activeStage() { return vertex(); }

QT_END_NAMESPACE
