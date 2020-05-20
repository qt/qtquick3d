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

#include <QtCore/QSharedPointer>

QT_BEGIN_NAMESPACE
// Baseclass for the vertex pipelines to be sure we have consistent implementations.
struct QSSGVertexPipelineImpl : public QSSGDefaultMaterialVertexPipelineInterface
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

    typedef TStrTableStrMap::const_iterator TParamIter;
    typedef QFlags<GenerationFlag> GenerationFlags;

    QSSGRef<QSSGMaterialShaderGeneratorInterface> m_materialGenerator;
    QSSGRef<QSSGShaderProgramGeneratorInterface> m_programGenerator;
    QString m_tempString;

    GenerationFlags m_generationFlags;
    TStrTableStrMap m_interpolationParameters;
    QList<QByteArray> m_addedFunctions;

    QSSGVertexPipelineImpl(const QSSGRef<QSSGMaterialShaderGeneratorInterface> &inMaterial,
                           const QSSGRef<QSSGShaderProgramGeneratorInterface> &inProgram)

        : m_materialGenerator(inMaterial)
        , m_programGenerator(inProgram)
    {
    }

    // Trues true if the code was *not* set.
    bool setCode(GenerationFlag inCode)
    {
        if (m_generationFlags & inCode)
            return true;
        m_generationFlags |= inCode;
        return false;
    }
    bool hasCode(GenerationFlag inCode) { return (m_generationFlags & inCode); }
    QSSGRef<QSSGShaderProgramGeneratorInterface> programGenerator() { return m_programGenerator; }

    QSSGStageGeneratorBase &vertex()
    {
        return *programGenerator()->getStage(QSSGShaderGeneratorStage::Vertex);
    }
    QSSGStageGeneratorBase &geometry()
    {
        return *programGenerator()->getStage(QSSGShaderGeneratorStage::Geometry);
    }
    QSSGStageGeneratorBase &fragment()
    {
        return *programGenerator()->getStage(QSSGShaderGeneratorStage::Fragment);
    }
    QSSGRef<QSSGMaterialShaderGeneratorInterface> materialGenerator() { return m_materialGenerator; }

    bool hasGeometryStage() const { return m_programGenerator->getEnabledStages() & QSSGShaderGeneratorStage::Geometry; }

    void generateUVCoords(quint32 inUVSet, const QSSGShaderDefaultMaterialKey &inKey) override
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
    void generateEnvMapReflection(const QSSGShaderDefaultMaterialKey &inKey) override
    {
        if (setCode(GenerationFlag::EnvMapReflection))
            return;

        generateWorldPosition();
        generateWorldNormal(inKey);
        QSSGStageGeneratorBase &activeGenerator(activeStage());
        activeGenerator.addInclude("viewProperties.glsllib");
        addInterpolationParameter("var_object_to_camera", "vec3");

        activeGenerator.append("    var_object_to_camera = normalize( local_model_world_position "
                               "- cameraPosition );");

        // World normal cannot be relied upon in the vertex shader because of bump maps.
        fragment().append("    vec3 environment_map_reflection = reflect( "
                          "normalize(var_object_to_camera), world_normal.xyz );");
        fragment().append("    environment_map_reflection *= vec3( 0.5, 0.5, 0 );");
        fragment().append("    environment_map_reflection += vec3( 0.5, 0.5, 1.0 );");
    }
    void generateViewVector() override
    {
        if (setCode(GenerationFlag::ViewVector))
            return;
        generateWorldPosition();
        QSSGStageGeneratorBase &activeGenerator(activeStage());
        activeGenerator.addInclude("viewProperties.glsllib");
        addInterpolationParameter("varViewVector", "vec3");

        activeGenerator.append("    vec3 local_view_vector = normalize(cameraPosition - "
                               "local_model_world_position);");
        assignOutput("varViewVector", "local_view_vector");
        fragment() << "    vec3 view_vector = normalize(varViewVector);\n";
    }

    // fragment shader expects varying vertex normal
    // lighting in vertex pipeline expects world_normal
    void generateWorldNormal(const QSSGShaderDefaultMaterialKey &inKey) override
    {
        if (setCode(GenerationFlag::WorldNormal))
            return;
        addInterpolationParameter("varNormal", "vec3");
        doGenerateWorldNormal(inKey);
        fragment().append("    vec3 world_normal = normalize( varNormal );");
    }
    void generateObjectNormal() override
    {
        if (setCode(GenerationFlag::ObjectNormal))
            return;
        doGenerateObjectNormal();
        fragment().append("    vec3 object_normal = normalize(varObjectNormal);");
    }
    void generateWorldPosition() override
    {
        if (setCode(GenerationFlag::WorldPosition))
            return;

        activeStage().addUniform("modelMatrix", "mat4");
        addInterpolationParameter("varWorldPos", "vec3");
        doGenerateWorldPosition();

        assignOutput("varWorldPos", "local_model_world_position");
    }
    void generateVarTangentAndBinormal(const QSSGShaderDefaultMaterialKey &inKey) override
    {
        if (setCode(GenerationFlag::TangentBinormal))
            return;
        addInterpolationParameter("varTangent", "vec3");
        addInterpolationParameter("varBinormal", "vec3");
        doGenerateVarTangentAndBinormal(inKey);
        fragment() << "    vec3 tangent = normalize(varTangent);\n"
                   << "    vec3 binormal = normalize(varBinormal);\n";
    }
    void generateVertexColor(const QSSGShaderDefaultMaterialKey &inKey) override
    {
        if (setCode(GenerationFlag::VertexColor))
            return;
        addInterpolationParameter("varColor", "vec3");
        doGenerateVertexColor(inKey);
        fragment().append("    vec3 vertColor = varColor;");
    }

    void addIncoming(const QByteArray &name, const QByteArray &type) override { activeStage().addIncoming(name, type); }

    void addOutgoing(const QByteArray &name, const QByteArray &type) override { addInterpolationParameter(name, type); }

    void addUniform(const QByteArray &name, const QByteArray &type) override { activeStage().addUniform(name, type); }

    void addInclude(const QByteArray &name) override { activeStage().addInclude(name); }

    void addFunction(const QByteArray &functionName) override
    {
        if (!m_addedFunctions.contains(functionName)) {
            m_addedFunctions.push_back(functionName);
            QByteArray includeName = "func" + functionName + ".glsllib";
            addInclude(includeName);
        }
    }

    void addConstantBuffer(const QByteArray &name, const QByteArray &layout) override
    {
        activeStage().addConstantBuffer(name, layout);
    }
    void addConstantBufferParam(const QByteArray &cbName, const QByteArray &paramName, const QByteArray &type) override
    {
        activeStage().addConstantBufferParam(cbName, paramName, type);
    }

    QSSGStageGeneratorBase &operator<<(const QByteArray &data) override
    {
        activeStage() << data;
        return *this;
    }

    void append(const QByteArray &data) override { activeStage().append(data); }

    QSSGShaderGeneratorStage stage() const override
    {
        return const_cast<QSSGVertexPipelineImpl *>(this)->activeStage().stage();
    }

    void beginVertexGeneration() override = 0;
    void assignOutput(const QByteArray &inVarName, const QByteArray &inVarValueExpr) override = 0;
    void endVertexGeneration(bool customShader) override = 0;

    void beginFragmentGeneration() override = 0;
    void endFragmentGeneration(bool customShader) override = 0;

    virtual QSSGStageGeneratorBase &activeStage() = 0;
    virtual void addInterpolationParameter(const QByteArray &inParamName, const QByteArray &inParamType) = 0;

    virtual void doGenerateUVCoords(quint32 inUVSet, const QSSGShaderDefaultMaterialKey &inKey) = 0;
    virtual void doGenerateWorldNormal(const QSSGShaderDefaultMaterialKey &inKey) = 0;
    virtual void doGenerateObjectNormal() = 0;
    virtual void doGenerateWorldPosition() = 0;
    virtual void doGenerateVarTangentAndBinormal(const QSSGShaderDefaultMaterialKey &inKey) = 0;
    virtual void doGenerateVertexColor(const QSSGShaderDefaultMaterialKey &inKey) = 0;
};
QT_END_NAMESPACE

#endif
