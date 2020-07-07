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

        Q_ASSERT(inUVSet == 0 || inUVSet == 1);

        if (inUVSet == 0)
            addInterpolationParameter("varTexCoord0", "vec2");
        else if (inUVSet == 1)
            addInterpolationParameter("varTexCoord1", "vec2");

        doGenerateUVCoords(inUVSet, inKey);
    }
    void generateEnvMapReflection(const QSSGShaderDefaultMaterialKey &inKey)
    {
        if (setCode(GenerationFlag::EnvMapReflection))
            return;

        generateWorldPosition();
        generateWorldNormal(inKey);
        QSSGStageGeneratorBase &activeGenerator(activeStage());
        activeGenerator.addInclude("viewProperties.glsllib");
        addInterpolationParameter("var_object_to_camera", "vec3");

        activeGenerator.append("    var_object_to_camera = normalize( local_model_world_position - qt_cameraPosition );");

        // World normal cannot be relied upon in the vertex shader because of bump maps.
        fragment().append("    vec3 environment_map_reflection = reflect( "
                          "normalize(var_object_to_camera), qt_world_normal.xyz );");
        fragment().append("    environment_map_reflection *= vec3( 0.5, 0.5, 0 );");
        fragment().append("    environment_map_reflection += vec3( 0.5, 0.5, 1.0 );");
    }
    void generateViewVector()
    {
        if (setCode(GenerationFlag::ViewVector))
            return;
        generateWorldPosition();
        QSSGStageGeneratorBase &activeGenerator(activeStage());
        activeGenerator.addInclude("viewProperties.glsllib");
        addInterpolationParameter("varViewVector", "vec3");

        activeGenerator.append("    vec3 qt_local_view_vector = normalize(qt_cameraPosition - local_model_world_position);");
        assignOutput("varViewVector", "qt_local_view_vector");
        fragment() << "    vec3 qt_view_vector = normalize(varViewVector);\n";
    }

    // fragment shader expects varying vertex normal
    // lighting in vertex pipeline expects qt_world_normal

    // qt_world_normal in both vert and frag shader
    void generateWorldNormal(const QSSGShaderDefaultMaterialKey &inKey)
    {
        if (setCode(GenerationFlag::WorldNormal))
            return;

        if (hasAttributeInKey(QSSGShaderKeyVertexAttribute::Normal, inKey)) {
            addInterpolationParameter("varNormal", "vec3");
            doGenerateWorldNormal(inKey);
        } else {
            fragment().append("    vec3 varNormal = cross(dFdx(varWorldPos), dFdy(varWorldPos));");
        }
        fragment().append("    vec3 qt_world_normal = normalize( varNormal );");
    }

    // object_normal in both vert and frag shader
    void generateObjectNormal()
    {
        if (setCode(GenerationFlag::ObjectNormal))
            return;
        doGenerateObjectNormal();
        fragment().append("    vec3 object_normal = normalize(varObjectNormal);");
    }

    // model_world_position in both vert and frag shader
    void generateWorldPosition()
    {
        if (setCode(GenerationFlag::WorldPosition))
            return;

        activeStage().addUniform("qt_modelMatrix", "mat4");
        addInterpolationParameter("varWorldPos", "vec3");
        doGenerateWorldPosition();

        assignOutput("varWorldPos", "local_model_world_position");
    }
    void generateVarTangentAndBinormal(const QSSGShaderDefaultMaterialKey &inKey)
    {
        if (setCode(GenerationFlag::TangentBinormal))
            return;

        // I assumes that there is no mesh having only binormal without tangent
        // since it is an abnormal case
        if (hasAttributeInKey(QSSGShaderKeyVertexAttribute::Binormal, inKey))
            addInterpolationParameter("varBinormal", "vec3");
        if (hasAttributeInKey(QSSGShaderKeyVertexAttribute::Tangent, inKey)) {
            addInterpolationParameter("varTangent", "vec3");
            doGenerateVarTangentAndBinormal(inKey);
            fragment() << "    vec3 qt_tangent = normalize(varTangent);\n"
                       << "    vec3 qt_binormal = normalize(varBinormal);\n";
        } else {
            fragment() << "    vec3 qt_tangent = vec3(0.0);\n"
                       << "    vec3 qt_binormal = vec3(0.0);\n";
        }
    }
    void generateVertexColor(const QSSGShaderDefaultMaterialKey &inKey)
    {
        if (setCode(GenerationFlag::VertexColor))
            return;
        addInterpolationParameter("varColor", "vec4");
        doGenerateVertexColor(inKey);
        fragment().append("    vec4 qt_vertColor = varColor;");
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
    void beginVertexGeneration();
    // The fragment shader expects a floating point constant, qt_objectOpacity to be defined
    // post this method.
    void beginFragmentGeneration();
    // Output variables may be mangled in some circumstances so the shader generation system
    // needs an abstraction
    // mechanism around this.
    void assignOutput(const QByteArray &inVarName, const QByteArray &inVarValueExpr);

    // responsible for closing all vertex and fragment generation
    void endVertexGeneration();
    void endFragmentGeneration();

    QSSGStageGeneratorBase &activeStage();
    void addInterpolationParameter(const QByteArray &inParamName, const QByteArray &inParamType);

    void doGenerateUVCoords(quint32 inUVSet, const QSSGShaderDefaultMaterialKey &inKey);
    void doGenerateWorldNormal(const QSSGShaderDefaultMaterialKey &inKey);
    void doGenerateObjectNormal();
    void doGenerateWorldPosition();
    void doGenerateVarTangentAndBinormal(const QSSGShaderDefaultMaterialKey &inKey);
    void doGenerateVertexColor(const QSSGShaderDefaultMaterialKey &inKey);
    bool hasAttributeInKey(QSSGShaderKeyVertexAttribute::VertexAttributeBits inAttr, const QSSGShaderDefaultMaterialKey &inKey);

    void addCustomMaterialBuiltins(const QSSGShaderDefaultMaterialKey &inKey);
};

QT_END_NAMESPACE

#endif
