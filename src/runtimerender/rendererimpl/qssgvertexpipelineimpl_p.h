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

#ifndef QSSG_VERTEX_PIPELINE_IMPL_H
#define QSSG_VERTEX_PIPELINE_IMPL_H

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

#include <QtQuick3DRuntimeRender/private/qssgrenderdefaultmaterialshadergenerator_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershaderkeys_p.h>

#include <QtCore/QSharedPointer>

QT_BEGIN_NAMESPACE

struct QSSGMaterialVertexPipeline
{
    enum class GenerationFlag
    {
        UVCoords = 1,
        EnvMapReflection = 1 << 1,
        ViewVector = 1 << 2,
        WorldNormal = 1 << 3,
        ObjectNormal = 1 << 4,
        WorldPosition = 1 << 5,
        TangentBinormal = 1 << 6,
        UVCoords1 = 1 << 7,
        VertexColor = 1 << 8,
        PerspDivDepth = 1 << 9,
        PerspDivWorldPos = 1 << 10
    };

    typedef QHash<QByteArray, QByteArray> TStrTableStrMap;
    typedef TStrTableStrMap::const_iterator TParamIter;
    typedef QFlags<GenerationFlag> GenerationFlags;

    QSSGRef<QSSGProgramGenerator> m_programGenerator;
    QString m_tempString;

    GenerationFlags m_generationFlags;
    bool m_hasSkinning;
    TStrTableStrMap m_interpolationParameters;
    QList<QByteArray> m_addedFunctions;

    const QSSGShaderDefaultMaterialKeyProperties &defaultMaterialShaderKeyProperties;
    QSSGShaderMaterialAdapter *materialAdapter;
    QSSGDataView<QMatrix4x4> boneGlobals;
    QSSGDataView<QMatrix3x3> boneNormals;
    bool useFloatJointIndices;
    bool hasCustomShadedMain;
    bool skipCustomFragmentSnippet;

    QSSGMaterialVertexPipeline(const QSSGRef<QSSGProgramGenerator> &inProgram,
                               const QSSGShaderDefaultMaterialKeyProperties &materialProperties,
                               QSSGShaderMaterialAdapter *materialAdapter,
                               QSSGDataView<QMatrix4x4> boneGlobals,
                               QSSGDataView<QMatrix3x3> boneNormals);

    ~QSSGMaterialVertexPipeline() = default;

    // Trues true if the code was *not* set.
    bool setCode(GenerationFlag inCode)
    {
        if (m_generationFlags & inCode)
            return true;
        m_generationFlags |= inCode;
        return false;
    }
    bool hasCode(GenerationFlag inCode) { return (m_generationFlags & inCode); }
    const QSSGRef<QSSGProgramGenerator> &programGenerator() const { return m_programGenerator; }

    QSSGStageGeneratorBase &vertex()
    {
        return *programGenerator()->getStage(QSSGShaderGeneratorStage::Vertex);
    }
    QSSGStageGeneratorBase &fragment()
    {
        return *programGenerator()->getStage(QSSGShaderGeneratorStage::Fragment);
    }

    /**
     * @brief Generates UV coordinates in shader code
     *
     * @param[in] inUVSet index of UV data set
     *
     * @return no return
     */
    void generateUVCoords(quint32 inUVSet, const QSSGShaderDefaultMaterialKey &inKey)
    {
        if (inUVSet == 0 && setCode(GenerationFlag::UVCoords))
            return;
        if (inUVSet == 1 && setCode(GenerationFlag::UVCoords1))
            return;

        const bool meshHasUV0 = hasAttributeInKey(QSSGShaderKeyVertexAttribute::TexCoord0, inKey);
        const bool meshHasUV1 = hasAttributeInKey(QSSGShaderKeyVertexAttribute::TexCoord1, inKey);

        Q_ASSERT(inUVSet == 0 || inUVSet == 1);

        if (inUVSet == 0) {
            if (hasCustomShadedMain || meshHasUV0) {
                addInterpolationParameter("qt_varTexCoord0", "vec2");
                vertex() << "    qt_varTexCoord0 = qt_vertUV0;\n";
            } else {
                vertex() << "    vec2 qt_varTexCoord0 = vec2(0.0);\n";
                fragment() << "    vec2 qt_varTexCoord0 = vec2(0.0);\n";
            }
        } else if (inUVSet == 1) {
            if (hasCustomShadedMain || meshHasUV1) {
                addInterpolationParameter("qt_varTexCoord1", "vec2");
                vertex() << "    qt_varTexCoord1 = qt_vertUV1;\n";
            } else {
                vertex() << "    vec2 qt_varTexCoord1 = vec2(0.0);\n";
                fragment() << "    vec2 qt_varTexCoord1 = vec2(0.0);\n";
            }
        }
    }
    void generateEnvMapReflection(const QSSGShaderDefaultMaterialKey &inKey)
    {
        if (setCode(GenerationFlag::EnvMapReflection))
            return;

        generateWorldPosition();
        generateWorldNormal(inKey);
        QSSGStageGeneratorBase &activeGenerator(activeStage());
        activeGenerator.addInclude("viewProperties.glsllib");
        addInterpolationParameter("qt_var_object_to_camera", "vec3");

        activeGenerator.append("    qt_var_object_to_camera = normalize( qt_local_model_world_position - qt_cameraPosition );");

        // World normal cannot be relied upon in the vertex shader because of bump maps.
        fragment().append("    vec3 environment_map_reflection = reflect( "
                          "normalize(qt_var_object_to_camera), qt_world_normal.xyz );");
        fragment().append("    environment_map_reflection *= vec3( 0.5, 0.5, 0 );");
        fragment().append("    environment_map_reflection += vec3( 0.5, 0.5, 1.0 );");
    }
    void generateViewVector()
    {
        if (setCode(GenerationFlag::ViewVector))
            return;
        generateWorldPosition();
        activeStage().addUniform("qt_cameraPosition", "vec3");

        fragment() << "    vec3 qt_view_vector = normalize(qt_cameraPosition - qt_varWorldPos);\n";
    }

    // fragment shader expects varying vertex normal
    // lighting in vertex pipeline expects qt_world_normal

    // qt_world_normal in both vert and frag shader
    void generateWorldNormal(const QSSGShaderDefaultMaterialKey &inKey)
    {
        if (setCode(GenerationFlag::WorldNormal))
            return;

        const bool meshHasNormal = hasAttributeInKey(QSSGShaderKeyVertexAttribute::Normal, inKey);

        if (hasCustomShadedMain || meshHasNormal) {
            addInterpolationParameter("qt_varNormal", "vec3");
            doGenerateWorldNormal();
        } else {
            generateWorldPosition();
            fragment().append("    vec3 qt_varNormal = cross(dFdx(qt_varWorldPos), dFdy(qt_varWorldPos));");
        }
        fragment().append("    vec3 qt_world_normal = normalize(qt_varNormal);");
    }

    void generateObjectNormal()
    {
        if (setCode(GenerationFlag::ObjectNormal))
            return;

        addInterpolationParameter("qt_varObjectNormal", "vec3");
        vertex().append("    qt_varObjectNormal = qt_vertNormal;");
        fragment().append("    vec3 object_normal = normalize(qt_varObjectNormal);");
    }

    void generateWorldPosition()
    {
        if (setCode(GenerationFlag::WorldPosition))
            return;

        activeStage().addUniform("qt_modelMatrix", "mat4");
        addInterpolationParameter("qt_varWorldPos", "vec3");
        vertex().append("    vec3 qt_local_model_world_position = (qt_modelMatrix * qt_vertPosition).xyz;");

        assignOutput("qt_varWorldPos", "qt_local_model_world_position");
    }

    void generateDepth()
    {
        if (setCode(GenerationFlag::PerspDivDepth))
            return;

        addInterpolationParameter("qt_varDepth", "float");
        vertex().append("    qt_varDepth = gl_Position.z / gl_Position.w;");
    }

    void generateShadowWorldPosition()
    {
        if (setCode(GenerationFlag::PerspDivWorldPos))
            return;

        activeStage().addUniform("qt_modelMatrix", "mat4");
        addInterpolationParameter("qt_varShadowWorldPos", "vec3");

        vertex().append("    vec4 qt_shadow_world_tmp = qt_modelMatrix * qt_vertPosition;");
        vertex().append("    qt_varShadowWorldPos = qt_shadow_world_tmp.xyz / qt_shadow_world_tmp.w;");
    }

    void generateVarTangentAndBinormal(const QSSGShaderDefaultMaterialKey &inKey)
    {
        if (setCode(GenerationFlag::TangentBinormal))
            return;

        const bool meshHasTangent = hasAttributeInKey(QSSGShaderKeyVertexAttribute::Tangent, inKey);
        const bool meshHasBinormal = hasAttributeInKey(QSSGShaderKeyVertexAttribute::Binormal, inKey);

        // I assume that there is no mesh having only binormal without tangent
        // since it is an abnormal case
        if (hasCustomShadedMain || meshHasTangent) {
            addInterpolationParameter("qt_varTangent", "vec3");
            doGenerateVarTangent();
            fragment() << "    vec3 qt_tangent = normalize(qt_varTangent);\n";

            if (hasCustomShadedMain || meshHasBinormal) {
                addInterpolationParameter("qt_varBinormal", "vec3");
                doGenerateVarBinormal();
                fragment() << "    vec3 qt_binormal = normalize(qt_varBinormal);\n";
            } else {
                fragment() << "    vec3 qt_binormal = vec3(0.0);\n";
            }
        } else {
            fragment() << "    vec3 qt_tangent = vec3(0.0);\n"
                       << "    vec3 qt_binormal = vec3(0.0);\n";
        }
    }
    void generateVertexColor(const QSSGShaderDefaultMaterialKey &inKey)
    {
        if (setCode(GenerationFlag::VertexColor))
            return;

        const bool meshHasColor = hasAttributeInKey(QSSGShaderKeyVertexAttribute::Color, inKey);
        const bool usesVarColor = defaultMaterialShaderKeyProperties.m_usesVarColor.getValue(inKey);
        if (hasCustomShadedMain || usesVarColor || meshHasColor) {
            addInterpolationParameter("qt_varColor", "vec4");
            vertex().append("    qt_varColor = qt_vertColor;");
            fragment().append("    vec4 qt_vertColor = qt_varColor;\n");
        } else {
            fragment().append("    vec4 qt_vertColor = vec4(1.0);\n"); // must be 1,1,1,1 to not alter when multiplying with it
        }
    }

    void addIncoming(const QByteArray &name, const QByteArray &type) { activeStage().addIncoming(name, type); }

    void addOutgoing(const QByteArray &name, const QByteArray &type) { addInterpolationParameter(name, type); }

    void addUniform(const QByteArray &name, const QByteArray &type) { activeStage().addUniform(name, type); }

    void addUniformArray(const QByteArray &name, const QByteArray &type, quint32 size) { activeStage().addUniformArray(name, type, size); }

    void addInclude(const QByteArray &name) { activeStage().addInclude(name); }

    void addFunction(const QByteArray &functionName)
    {
        if (!m_addedFunctions.contains(functionName)) {
            m_addedFunctions.push_back(functionName);
            QByteArray includeName = "func" + functionName + ".glsllib";
            addInclude(includeName);
        }
    }

    void addConstantBuffer(const QByteArray &name, const QByteArray &layout)
    {
        activeStage().addConstantBuffer(name, layout);
    }

    void addConstantBufferParam(const QByteArray &cbName, const QByteArray &paramName, const QByteArray &type)
    {
        activeStage().addConstantBufferParam(cbName, paramName, type);
    }

    QSSGStageGeneratorBase &operator<<(const QByteArray &data)
    {
        activeStage() << data;
        return activeStage();
    }

    void append(const QByteArray &data) { activeStage().append(data); }

    QSSGShaderGeneratorStage stage() const
    {
        return const_cast<QSSGMaterialVertexPipeline *>(this)->activeStage().stage();
    }

    // Responsible for beginning all vertex and fragment generation (void main() { etc).
    void beginVertexGeneration(const QSSGShaderDefaultMaterialKey &inKey,
                               const ShaderFeatureSetList &inFeatureSet,
                               const QSSGRef<QSSGShaderLibraryManager> &shaderLibraryManager);
    // The fragment shader expects a floating point constant, qt_objectOpacity to be defined
    // post this method.
    void beginFragmentGeneration(const QSSGRef<QSSGShaderLibraryManager> &shaderLibraryManager);
    // Output variables may be mangled in some circumstances so the shader generation system
    // needs an abstraction
    // mechanism around this.
    void assignOutput(const QByteArray &inVarName, const QByteArray &inVarValueExpr);

    // responsible for closing all vertex and fragment generation
    void endVertexGeneration();
    void endFragmentGeneration();

    QSSGStageGeneratorBase &activeStage();
    void addInterpolationParameter(const QByteArray &inParamName, const QByteArray &inParamType);

    void doGenerateWorldNormal();
    void doGenerateVarTangent();
    void doGenerateVarBinormal();
    bool hasAttributeInKey(QSSGShaderKeyVertexAttribute::VertexAttributeBits inAttr, const QSSGShaderDefaultMaterialKey &inKey);
};

QT_END_NAMESPACE

#endif
