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
#include <QtQuick3DRuntimeRender/private/qssgshadermaterialadapter_p.h>

QT_BEGIN_NAMESPACE

QSSGMaterialVertexPipeline::QSSGMaterialVertexPipeline(const QSSGRef<QSSGProgramGenerator> &programGen,
                                                       const QSSGShaderDefaultMaterialKeyProperties &materialProperties,
                                                       QSSGShaderMaterialAdapter *materialAdapter,
                                                       QSSGDataView<QMatrix4x4> boneGlobals,
                                                       QSSGDataView<QMatrix3x3> boneNormals)
    : m_programGenerator(programGen)
    , defaultMaterialShaderKeyProperties(materialProperties)
    , materialAdapter(materialAdapter)
    , boneGlobals(boneGlobals)
    , boneNormals(boneNormals)
    , hasCustomShadedMain(false)
    , skipCustomFragmentSnippet(false)
{
    m_hasSkinning = boneGlobals.size() > 0;
}

static inline void insertProcessorArgs(QByteArray &snippet, const char *argKey, const char* (*argListFunc)())
{
    const int argKeyLen = int(strlen(argKey));
    const int argKeyPos = snippet.indexOf(argKey);
    if (argKeyPos >= 0)
        snippet = snippet.left(argKeyPos) + argListFunc() + snippet.mid(argKeyPos + argKeyLen);
}

static inline void insertDirectionalLightProcessorArgs(QByteArray &snippet)
{
    insertProcessorArgs(snippet, "/*%QT_ARGS_DIRECTIONAL_LIGHT%*/", QSSGMaterialShaderGenerator::directionalLightProcessorArgumentList);
}

static inline void insertPointLightProcessorArgs(QByteArray &snippet)
{
    insertProcessorArgs(snippet, "/*%QT_ARGS_POINT_LIGHT%*/", QSSGMaterialShaderGenerator::pointLightProcessorArgumentList);
}

static inline void insertSpotLightProcessorArgs(QByteArray &snippet)
{
    insertProcessorArgs(snippet, "/*%QT_ARGS_SPOT_LIGHT%*/", QSSGMaterialShaderGenerator::spotLightProcessorArgumentList);
}

static inline void insertAmbientLightProcessorArgs(QByteArray &snippet)
{
    insertProcessorArgs(snippet, "/*%QT_ARGS_AMBIENT_LIGHT%*/", QSSGMaterialShaderGenerator::ambientLightProcessorArgumentList);
}

static inline void insertSpecularLightProcessorArgs(QByteArray &snippet)
{
    insertProcessorArgs(snippet, "/*%QT_ARGS_SPECULAR_LIGHT%*/", QSSGMaterialShaderGenerator::specularLightProcessorArgumentList);
}

static inline void insertFragmentMainArgs(QByteArray &snippet)
{
    insertProcessorArgs(snippet, "/*%QT_ARGS_MAIN%*/", QSSGMaterialShaderGenerator::shadedFragmentMainArgumentList);
}

static inline void insertVertexMainArgs(QByteArray &snippet)
{
    insertProcessorArgs(snippet, "/*%QT_ARGS_MAIN%*/", QSSGMaterialShaderGenerator::vertexMainArgumentList);
}

void QSSGMaterialVertexPipeline::beginVertexGeneration(const QSSGShaderDefaultMaterialKey &inKey,
                                                       const ShaderFeatureSetList &inFeatureSet,
                                                       const QSSGRef<QSSGShaderLibraryManager> &shaderLibraryManager)
{
    QSSGShaderGeneratorStageFlags theStages(QSSGProgramGenerator::defaultFlags());
    programGenerator()->beginProgram(theStages);

    QSSGStageGeneratorBase &vertexShader(vertex());

    const bool meshHasNormals = defaultMaterialShaderKeyProperties.m_vertexAttributes.getBitValue(
                QSSGShaderKeyVertexAttribute::Normal, inKey);
    const bool meshHasTexCoord0 = defaultMaterialShaderKeyProperties.m_vertexAttributes.getBitValue(
                QSSGShaderKeyVertexAttribute::TexCoord0, inKey);
    const bool meshHasTexCoord1 = defaultMaterialShaderKeyProperties.m_vertexAttributes.getBitValue(
                QSSGShaderKeyVertexAttribute::TexCoord1, inKey);
    const bool meshHasTangents = defaultMaterialShaderKeyProperties.m_vertexAttributes.getBitValue(
                QSSGShaderKeyVertexAttribute::Tangent, inKey);
    const bool meshHasBinormals = defaultMaterialShaderKeyProperties.m_vertexAttributes.getBitValue(
                QSSGShaderKeyVertexAttribute::Binormal, inKey);
    const bool meshHasColors = defaultMaterialShaderKeyProperties.m_vertexAttributes.getBitValue(
                QSSGShaderKeyVertexAttribute::Color, inKey);
    const bool meshHasJointsAndWeights = defaultMaterialShaderKeyProperties.m_vertexAttributes.getBitValue(
                QSSGShaderKeyVertexAttribute::JointAndWeight, inKey);
    const bool overridesPosition = defaultMaterialShaderKeyProperties.m_overridesPosition.getValue(inKey);
    const bool usesProjectionMatrix = defaultMaterialShaderKeyProperties.m_usesProjectionMatrix.getValue(inKey);
    const bool usesInvProjectionMatrix = defaultMaterialShaderKeyProperties.m_usesInverseProjectionMatrix.getValue(inKey);
    const bool usesPointsTopology = defaultMaterialShaderKeyProperties.m_usesPointsTopology.getValue(inKey);
    const bool usesFloatJointIndices = defaultMaterialShaderKeyProperties.m_usesFloatJointIndices.getValue(inKey);

    vertexShader.addIncoming("attr_pos", "vec3");

    if (m_hasSkinning && meshHasJointsAndWeights) {
        vertexShader.addInclude("skinanim.glsllib");
        if (usesFloatJointIndices)
            vertexShader.addIncoming("attr_joints", "vec4");
        else
            vertexShader.addIncoming("attr_joints", "ivec4");
        vertexShader.addIncoming("attr_weights", "vec4");

        vertexShader.addUniformArray("qt_boneTransforms", "mat4", boneGlobals.mSize);
        vertexShader.addUniformArray("qt_boneNormalTransforms", "mat3", boneNormals.mSize);
    }

    const bool hasCustomVertexShader = materialAdapter->hasCustomShaderSnippet(QSSGShaderCache::ShaderType::Vertex);
    if (hasCustomVertexShader) {
        QByteArray snippet = materialAdapter->customShaderSnippet(QSSGShaderCache::ShaderType::Vertex,
                                                                  shaderLibraryManager);
        if (materialAdapter->hasCustomShaderFunction(QSSGShaderCache::ShaderType::Vertex,
                                                     QByteArrayLiteral("qt_customMain"),
                                                     shaderLibraryManager))
        {
            insertVertexMainArgs(snippet);

            if (m_hasSkinning) {
                vertexShader.addInclude("skinanim.glsllib");
                vertexShader.addUniformArray("qt_boneTransforms", "mat4", boneGlobals.mSize);
                vertexShader.addUniformArray("qt_boneNormalTransforms", "mat3", boneNormals.mSize);
            }

            if (!materialAdapter->isUnshaded()) {
                hasCustomShadedMain = true;
            }
        }
        vertexShader << snippet;
    }

    vertexShader << "void main()"
                 << "\n"
                 << "{"
                 << "\n";
    // These local variables will be used for whole the pipeline
    // instead of each attributes since it is more convenient
    // for adding new routines.
    vertexShader.append("    vec4 qt_vertPosition = vec4(attr_pos, 1.0);");
    vertexShader.append("    vec3 qt_vertNormal = vec3(0.0);");
    vertexShader.append("    vec3 qt_vertTangent = vec3(0.0);");
    vertexShader.append("    vec3 qt_vertBinormal = vec3(0.0);");
    vertexShader.append("    vec2 qt_vertUV0 = vec2(0.0);");
    vertexShader.append("    vec2 qt_vertUV1 = vec2(0.0);");
    vertexShader.append("    ivec4 qt_vertJoints = ivec4(0);");
    vertexShader.append("    vec4 qt_vertWeights = vec4(0.0);");
    vertexShader.append("    vec4 qt_vertColor = vec4(1.0);"); // must be 1,1,1,1 to not alter when multiplying with it

    vertexShader.addUniform("qt_modelViewProjection", "mat4");

    skipCustomFragmentSnippet = false;
    for (const auto &feature : inFeatureSet) {
        if (feature.name == QSSGShaderDefines::asString(QSSGShaderDefines::DepthPass))
            skipCustomFragmentSnippet = feature.enabled;
    }

    if (hasCustomVertexShader) { // this is both for unshaded and shaded
        vertexShader.addUniform("qt_viewProjectionMatrix", "mat4");
        vertexShader.addUniform("qt_modelMatrix", "mat4");
        vertexShader.addUniform("qt_viewMatrix", "mat4");
        vertexShader.addUniform("qt_normalMatrix", "mat3");
        vertexShader.addUniform("qt_cameraPosition", "vec3");
        vertexShader.addUniform("qt_cameraDirection", "vec3");
        vertexShader.addUniform("qt_cameraProperties", "vec2");
        if (usesProjectionMatrix)
            vertexShader.addUniform("qt_projectionMatrix", "mat4");
        if (usesInvProjectionMatrix)
            vertexShader.addUniform("qt_inverseProjectionMatrix", "mat4");
    }

    if (meshHasNormals) {
        vertexShader.append("    qt_vertNormal = attr_norm;");
        vertexShader.addIncoming("attr_norm", "vec3");
    }
    if (meshHasTexCoord0) {
        vertexShader.append("    qt_vertUV0 = attr_uv0;");
        vertexShader.addIncoming("attr_uv0", "vec2");
    }
    if (meshHasTexCoord1) {
        vertexShader.append("    qt_vertUV1 = attr_uv1;");
        vertexShader.addIncoming("attr_uv1", "vec2");
    }
    if (meshHasTangents) {
        vertexShader.append("    qt_vertTangent = attr_textan;");
        vertexShader.addIncoming("attr_textan", "vec3");
    }
    if (meshHasBinormals) {
        vertexShader.append("    qt_vertBinormal = attr_binormal;");
        vertexShader.addIncoming("attr_binormal", "vec3");
    }
    if (meshHasColors) {
        vertexShader.append("    qt_vertColor = attr_color;");
        vertexShader.addIncoming("attr_color", "vec4");
    }
    if (meshHasJointsAndWeights) {
        if (usesFloatJointIndices) {
            vertexShader.addIncoming("attr_joints", "vec4");
            vertexShader.append("    qt_vertJoints = ivec4(attr_joints);");
        } else {
            vertexShader.addIncoming("attr_joints", "ivec4");
            vertexShader.append("    qt_vertJoints = attr_joints;");
        }
        vertexShader.addIncoming("attr_weights", "vec4");
        vertexShader.append("    qt_vertWeights = attr_weights;");
    }


    if (!materialAdapter->isUnshaded() || !hasCustomVertexShader) {
        vertexShader << "    vec3 qt_uTransform;\n";
        vertexShader << "    vec3 qt_vTransform;\n";
        if (hasCustomShadedMain)
            vertexShader.append("    qt_customMain(qt_vertPosition.xyz, qt_vertNormal, qt_vertUV0, qt_vertUV1, qt_vertTangent, qt_vertBinormal, qt_vertJoints, qt_vertWeights, qt_vertColor);");

        if (m_hasSkinning) {
            vertexShader.append("    if (qt_vertWeights != vec4(0.0))");
            vertexShader.append("        qt_vertPosition = qt_getSkinMatrix(qt_vertJoints, qt_vertWeights) * qt_vertPosition;");
        }

        if (!hasCustomShadedMain || !overridesPosition)
            vertexShader.append("    gl_Position = qt_modelViewProjection * qt_vertPosition;");
    }

    if (usesPointsTopology && !hasCustomVertexShader) {
        vertexShader.addUniform("qt_materialPointSize", "float");
        vertexShader.append("    gl_PointSize = qt_materialPointSize;");
    } // with a custom vertex shader it is up to it to set gl_PointSize (aka POINT_SIZE)
}

void QSSGMaterialVertexPipeline::beginFragmentGeneration(const QSSGRef<QSSGShaderLibraryManager> &shaderLibraryManager)
{
    fragment().addUniform("qt_material_properties", "vec4");

    if (!skipCustomFragmentSnippet && materialAdapter->hasCustomShaderSnippet(QSSGShaderCache::ShaderType::Fragment)) {
        QByteArray snippet = materialAdapter->customShaderSnippet(QSSGShaderCache::ShaderType::Fragment,
                                                                  shaderLibraryManager);
        if (!materialAdapter->isUnshaded()) {
            insertAmbientLightProcessorArgs(snippet);
            insertSpecularLightProcessorArgs(snippet);
            insertSpotLightProcessorArgs(snippet);
            insertPointLightProcessorArgs(snippet);
            insertDirectionalLightProcessorArgs(snippet);
            insertFragmentMainArgs(snippet);
        }
        fragment() << snippet;
    }

    fragment() << "void main()"
               << "\n"
               << "{"
               << "\n";

    if (!materialAdapter->isUnshaded() || !materialAdapter->hasCustomShaderSnippet(QSSGShaderCache::ShaderType::Fragment))
        fragment() << "    float qt_objectOpacity = qt_material_properties.a;\n";
}

void QSSGMaterialVertexPipeline::assignOutput(const QByteArray &inVarName, const QByteArray &inVarValue)
{
    vertex() << "    " << inVarName << " = " << inVarValue << ";\n";
}

void QSSGMaterialVertexPipeline::doGenerateWorldNormal()
{
    QSSGStageGeneratorBase &vertexGenerator(vertex());
    vertexGenerator.addUniform("qt_normalMatrix", "mat3");
    if (m_hasSkinning) {
        vertexGenerator.append("    if (qt_vertWeights != vec4(0.0))");
        vertexGenerator.append("        qt_vertNormal = qt_getSkinNormalMatrix(qt_vertJoints, qt_vertWeights) * qt_vertNormal;");
    }
    vertexGenerator.append("    vec3 qt_world_normal = normalize(qt_normalMatrix * qt_vertNormal);");
    vertexGenerator.append("    qt_varNormal = qt_world_normal;");
}

void QSSGMaterialVertexPipeline::doGenerateVarTangent()
{
    if (m_hasSkinning) {
        vertex() << "    if (qt_vertWeights != vec4(0.0)) {\n"
                 << "       qt_vertTangent = (qt_getSkinMatrix(qt_vertJoints, qt_vertWeights) * vec4(qt_vertTangent, 0.0)).xyz;\n"
                 << "    }\n";

    }
    vertex() << "    qt_varTangent = (qt_modelMatrix * vec4(qt_vertTangent, 0.0)).xyz;\n";
}

void QSSGMaterialVertexPipeline::doGenerateVarBinormal()
{
    if (m_hasSkinning) {
        vertex() << "    if (qt_vertWeights != vec4(0.0)) {\n"
                 << "       qt_vertBinormal = (qt_getSkinMatrix(qt_vertJoints, qt_vertWeights) * vec4(qt_vertBinormal, 0.0)).xyz;\n"
                 << "    }\n";
    }
    vertex() << "    qt_varBinormal = (qt_modelMatrix * vec4(qt_vertBinormal, 0.0)).xyz;\n";
}

bool QSSGMaterialVertexPipeline::hasAttributeInKey(QSSGShaderKeyVertexAttribute::VertexAttributeBits inAttr,
                                                   const QSSGShaderDefaultMaterialKey &inKey)
{
    return defaultMaterialShaderKeyProperties.m_vertexAttributes.getBitValue(inAttr, inKey);
}

void QSSGMaterialVertexPipeline::endVertexGeneration()
{
    if (materialAdapter->isUnshaded() && materialAdapter->hasCustomShaderSnippet(QSSGShaderCache::ShaderType::Vertex))
        vertex() << "    qt_customMain(qt_vertPosition.xyz, qt_vertNormal, qt_vertUV0, qt_vertUV1, qt_vertTangent, qt_vertBinormal, qt_vertJoints, qt_vertWeights, qt_vertColor);\n";

    vertex().append("}");
}

void QSSGMaterialVertexPipeline::endFragmentGeneration()
{
    if (!skipCustomFragmentSnippet && materialAdapter->isUnshaded() && materialAdapter->hasCustomShaderSnippet(QSSGShaderCache::ShaderType::Fragment))
        fragment() << "    qt_customMain();\n";

    fragment().append("}");
}

void QSSGMaterialVertexPipeline::addInterpolationParameter(const QByteArray &inName, const QByteArray &inType)
{
    m_interpolationParameters.insert(inName, inType);
    vertex().addOutgoing(inName, inType);
    fragment().addIncoming(inName, inType);
}

QSSGStageGeneratorBase &QSSGMaterialVertexPipeline::activeStage()
{
    return vertex();
}

QT_END_NAMESPACE
