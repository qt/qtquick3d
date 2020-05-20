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

#include <QtQuick3DRuntimeRender/private/qssgrenderer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderlight_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercontextcore_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercache_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershaderlibrarymanager_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercodegenerator_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderdefaultmaterialshadergenerator_p.h>
#include <QtQuick3DRuntimeRender/private/qssgvertexpipelineimpl_p.h>

// This adds support for the depth buffers in the shader so we can do depth
// texture-based effects.
#define QSSG_RENDER_SUPPORT_DEPTH_TEXTURE 1

QT_BEGIN_NAMESPACE

// Helper implements the vertex pipeline for mesh subsets when bound to the default material.
// Should be completely possible to use for custom materials with a bit of refactoring.
struct QSSGSubsetMaterialVertexPipeline : public QSSGVertexPipelineImpl
{
    QSSGRenderer &renderer;
    QSSGSubsetRenderable &renderable;

    QSSGSubsetMaterialVertexPipeline(QSSGRenderer &inRenderer, QSSGSubsetRenderable &inRenderable)
        : QSSGVertexPipelineImpl(inRenderer.contextInterface()->defaultMaterialShaderGenerator(),
                                   inRenderer.contextInterface()->shaderProgramGenerator())
        , renderer(inRenderer)
        , renderable(inRenderable)
    {
    }

    void beginVertexGeneration() override
    {
        QSSGShaderGeneratorStageFlags theStages(QSSGShaderProgramGeneratorInterface::defaultFlags());
        programGenerator()->beginProgram(theStages);

        // Open up each stage.
        QSSGStageGeneratorBase &vertexShader(vertex());
        vertexShader.addIncoming("attr_pos", "vec3");
        vertexShader << "void main()"
                     << "\n"
                     << "{"
                     << "\n";
        vertexShader << "    vec3 uTransform;"
                     << "\n";
        vertexShader << "    vec3 vTransform;"
                     << "\n";

        // for tessellation we pass on the position in object coordinates
        // Also note that gl_Position is written in the tess eval shader
        vertexShader.addUniform("modelViewProjection", "mat4");
        vertexShader.append("    gl_Position = modelViewProjection * vec4(attr_pos, 1.0);");
    }

    void beginFragmentGeneration() override
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

    void assignOutput(const QByteArray &inVarName, const QByteArray &inVarValue) override
    {
        vertex() << "    " << inVarName << " = " << inVarValue << ";\n";
    }
    void doGenerateUVCoords(quint32 inUVSet, const QSSGShaderDefaultMaterialKey &inKey) override
    {
        Q_ASSERT(inUVSet == 0 || inUVSet == 1);

        if (inUVSet == 0) {
            const bool meshHasTexCoord0 = renderer.defaultMaterialShaderKeyProperties().m_vertexAttributes.getBitValue(
                        QSSGShaderKeyVertexAttribute::TexCoord0, inKey);
            if (meshHasTexCoord0)
                vertex().addIncoming("attr_uv0", "vec2");
            else
                vertex().append("    vec2 attr_uv0 = vec2(0.0);");
            vertex() << "    varTexCoord0 = attr_uv0;"
                     << "\n";
        } else if (inUVSet == 1) {
            const bool meshHasTexCoord1 = renderer.defaultMaterialShaderKeyProperties().m_vertexAttributes.getBitValue(
                        QSSGShaderKeyVertexAttribute::TexCoord1, inKey);
            if (meshHasTexCoord1)
                vertex().addIncoming("attr_uv1", "vec2");
            else
                vertex().append("    vec2 attr_uv1 = vec2(0.0);");
            vertex() << "    varTexCoord1 = attr_uv1;"
                     << "\n";
        }
    }

    // fragment shader expects varying vertex normal
    // lighting in vertex pipeline expects world_normal
    void doGenerateWorldNormal(const QSSGShaderDefaultMaterialKey &inKey) override
    {
        const bool meshHasNormals = renderer.defaultMaterialShaderKeyProperties().m_vertexAttributes.getBitValue(
                    QSSGShaderKeyVertexAttribute::Normal, inKey);

        QSSGStageGeneratorBase &vertexGenerator(vertex());
        if (meshHasNormals)
            vertexGenerator.addIncoming("attr_norm", "vec3");
        else
            vertexGenerator.append("    vec3 attr_norm = vec3(0.0);");
        vertexGenerator.addUniform("normalMatrix", "mat3");
        vertexGenerator.append("    vec3 world_normal = normalize(normalMatrix * attr_norm).xyz;");
        vertexGenerator.append("    varNormal = world_normal;");
    }
    void doGenerateObjectNormal() override
    {
        addInterpolationParameter("varObjectNormal", "vec3");
        vertex().append("    varObjectNormal = attr_norm;");
    }
    void doGenerateWorldPosition() override
    {
        vertex().append("    vec3 local_model_world_position = (modelMatrix * vec4(attr_pos, 1.0)).xyz;");
        assignOutput("varWorldPos", "local_model_world_position");
    }

    void doGenerateVarTangentAndBinormal(const QSSGShaderDefaultMaterialKey &inKey) override
    {
        const bool meshHasTangents = renderer.defaultMaterialShaderKeyProperties().m_vertexAttributes.getBitValue(
                    QSSGShaderKeyVertexAttribute::Tangent, inKey);
        const bool meshHasBinormals = renderer.defaultMaterialShaderKeyProperties().m_vertexAttributes.getBitValue(
                    QSSGShaderKeyVertexAttribute::Binormal, inKey);

        if (meshHasTangents)
            vertex().addIncoming("attr_textan", "vec3");
        else
            vertex() << "    vec3 attr_textan = vec3(0.0);\n";

        if (meshHasBinormals)
            vertex().addIncoming("attr_binormal", "vec3");
        else
            vertex() << "    vec3 attr_binormal = vec3(0.0);\n";

        vertex() << "    varTangent = normalMatrix * attr_textan;"
                 << "\n"
                 << "    varBinormal = normalMatrix * attr_binormal;"
                 << "\n";
    }

    void doGenerateVertexColor(const QSSGShaderDefaultMaterialKey &inKey) override
    {
        const bool meshHasColors = renderer.defaultMaterialShaderKeyProperties().m_vertexAttributes.getBitValue(
                    QSSGShaderKeyVertexAttribute::Color, inKey);
        if (meshHasColors)
            vertex().addIncoming("attr_color", "vec3");
        else
            vertex().append("    vec3 attr_color = vec3(0.0);");
        vertex().append("    varColor = attr_color;");
    }

    void endVertexGeneration(bool customShader) override
    {
        if (!customShader)
            vertex().append("}");
    }

    void endFragmentGeneration(bool customShader) override
    {
        if (!customShader)
            fragment().append("}");
    }

    void addInterpolationParameter(const QByteArray &inName, const QByteArray &inType) override
    {
        m_interpolationParameters.insert(inName, inType);
        vertex().addOutgoing(inName, inType);
        fragment().addIncoming(inName, inType);
    }

    QSSGStageGeneratorBase &activeStage() override { return vertex(); }
};

static QByteArray logPrefix() { return QByteArrayLiteral("mesh subset pipeline-- "); }

QSSGRef<QSSGRhiShaderStages> QSSGRenderer::generateRhiShaderStages(QSSGSubsetRenderable &inRenderable,
                                                                       const ShaderFeatureSetList &inFeatureSet)
{
    // build a string that allows us to print out the shader we are generating to the log.
    // This is time consuming but I feel like it doesn't happen all that often and is very
    // useful to users
    // looking at the log file.
    m_generatedShaderString = logPrefix();

    QSSGShaderDefaultMaterialKey theKey(inRenderable.shaderDescription);
    theKey.toString(m_generatedShaderString, m_defaultMaterialShaderKeyProperties);

    const QSSGRef<QSSGShaderCache> &theCache = m_contextInterface->shaderCache();
    const QSSGRef<QSSGRhiShaderStages> &cachedShaders = theCache->getRhiShaderStages(m_generatedShaderString, inFeatureSet);
    if (cachedShaders)
        return cachedShaders;

    QSSGSubsetMaterialVertexPipeline pipeline(*this, inRenderable);

    return m_contextInterface->defaultMaterialShaderGenerator()->generateRhiShaderStages(inRenderable.material,
                                                                                         inRenderable.shaderDescription,
                                                                                         pipeline,
                                                                                         inFeatureSet,
                                                                                         m_currentLayer->globalLights,
                                                                                         inRenderable.firstImage,
                                                                                         inRenderable.renderableFlags.hasTransparency(),
                                                                                         logPrefix());
}

QT_END_NAMESPACE
