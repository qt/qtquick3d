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

#include <QtQuick3DRuntimeRender/private/qssgrendererimpl_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderlight_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercontextcore_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercache_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderdynamicobjectsystem_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercodegeneratorv2_p.h>
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
    QSSGRendererImpl &renderer;
    QSSGSubsetRenderable &renderable;
    TessellationModeValues tessMode;

    QSSGSubsetMaterialVertexPipeline(QSSGRendererImpl &inRenderer, QSSGSubsetRenderable &inRenderable, bool inWireframeRequested)
        : QSSGVertexPipelineImpl(inRenderer.contextInterface()->defaultMaterialShaderGenerator(),
                                   inRenderer.contextInterface()->shaderProgramGenerator(),
                                   false)
        , renderer(inRenderer)
        , renderable(inRenderable)
        , tessMode(TessellationModeValues::NoTessellation)
    {
        if (inRenderer.context()->supportsTessellation())
            tessMode = inRenderable.tessellationMode;

        if (inRenderer.context()->supportsGeometryStage() && tessMode != TessellationModeValues::NoTessellation)
            m_wireframe = inWireframeRequested;
    }

    void initializeTessControlShader()
    {
        if (tessMode == TessellationModeValues::NoTessellation
                || programGenerator()->getStage(QSSGShaderGeneratorStage::TessControl) == nullptr) {
            return;
        }

        QSSGShaderStageGeneratorInterface &tessCtrlShader(*programGenerator()->getStage(QSSGShaderGeneratorStage::TessControl));

        tessCtrlShader.addUniform("tessLevelInner", "float");
        tessCtrlShader.addUniform("tessLevelOuter", "float");

        setupTessIncludes(QSSGShaderGeneratorStage::TessControl, tessMode);

        tessCtrlShader.append("void main() {\n");

        tessCtrlShader.append("\tctWorldPos[0] = varWorldPos[0];");
        tessCtrlShader.append("\tctWorldPos[1] = varWorldPos[1];");
        tessCtrlShader.append("\tctWorldPos[2] = varWorldPos[2];");

        if (tessMode == TessellationModeValues::Phong || tessMode == TessellationModeValues::NPatch) {
            tessCtrlShader.append("\tctNorm[0] = varObjectNormal[0];");
            tessCtrlShader.append("\tctNorm[1] = varObjectNormal[1];");
            tessCtrlShader.append("\tctNorm[2] = varObjectNormal[2];");
        }
        if (tessMode == TessellationModeValues::NPatch) {
            tessCtrlShader.append("\tctTangent[0] = varTangent[0];");
            tessCtrlShader.append("\tctTangent[1] = varTangent[1];");
            tessCtrlShader.append("\tctTangent[2] = varTangent[2];");
        }

        tessCtrlShader.append("\tgl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;");
        tessCtrlShader.append("\ttessShader( tessLevelOuter, tessLevelInner);\n");
    }
    void initializeTessEvaluationShader()
    {
        if (tessMode == TessellationModeValues::NoTessellation
                || programGenerator()->getStage(QSSGShaderGeneratorStage::TessEval) == nullptr) {
            return;
        }

        QSSGShaderStageGeneratorInterface &tessEvalShader(*programGenerator()->getStage(QSSGShaderGeneratorStage::TessEval));

        setupTessIncludes(QSSGShaderGeneratorStage::TessEval, tessMode);

        if (tessMode == TessellationModeValues::Linear)
            renderer.contextInterface()->defaultMaterialShaderGenerator()->addDisplacementImageUniforms(tessEvalShader,
                                                                                                          m_displacementIdx,
                                                                                                          m_displacementImage);

        tessEvalShader.addUniform("modelViewProjection", "mat4");
        tessEvalShader.addUniform("normalMatrix", "mat3");

        tessEvalShader.append("void main() {");

        if (tessMode == TessellationModeValues::NPatch) {
            tessEvalShader.append("\tctNorm[0] = varObjectNormalTC[0];");
            tessEvalShader.append("\tctNorm[1] = varObjectNormalTC[1];");
            tessEvalShader.append("\tctNorm[2] = varObjectNormalTC[2];");

            tessEvalShader.append("\tctTangent[0] = varTangentTC[0];");
            tessEvalShader.append("\tctTangent[1] = varTangentTC[1];");
            tessEvalShader.append("\tctTangent[2] = varTangentTC[2];");
        }

        tessEvalShader.append("\tvec4 pos = tessShader( );\n");
    }

    void finalizeTessControlShader()
    {
        QSSGShaderStageGeneratorInterface &tessCtrlShader(*programGenerator()->getStage(QSSGShaderGeneratorStage::TessControl));
        // add varyings we must pass through
        typedef TStrTableStrMap::const_iterator TParamIter;
        for (TParamIter iter = m_interpolationParameters.begin(), end = m_interpolationParameters.end(); iter != end; ++iter) {
            tessCtrlShader << "\t" << iter.key() << "TC[gl_InvocationID] = " << iter.key() << "[gl_InvocationID];\n";
        }
    }

    void finalizeTessEvaluationShader()
    {
        QSSGShaderStageGeneratorInterface &tessEvalShader(*programGenerator()->getStage(QSSGShaderGeneratorStage::TessEval));

        QByteArray outExt;
        if (programGenerator()->getEnabledStages() & QSSGShaderGeneratorStage::Geometry)
            outExt = "TE";

        // add varyings we must pass through
        typedef TStrTableStrMap::const_iterator TParamIter;
        if (tessMode == TessellationModeValues::NPatch) {
            for (TParamIter iter = m_interpolationParameters.begin(), end = m_interpolationParameters.end(); iter != end; ++iter) {
                tessEvalShader << "\t" << iter.key() << outExt << " = gl_TessCoord.z * " << iter.key() << "TC[0] + ";
                tessEvalShader << "gl_TessCoord.x * " << iter.key() << "TC[1] + ";
                tessEvalShader << "gl_TessCoord.y * " << iter.key() << "TC[2];\n";
            }

            // transform the normal
            if (m_generationFlags & GenerationFlag::WorldNormal)
                tessEvalShader << "\n\tvarNormal" << outExt << " = normalize(normalMatrix * teNorm);\n";
            // transform the tangent
            if (m_generationFlags & GenerationFlag::TangentBinormal) {
                tessEvalShader << "\n\tvarTangent" << outExt << " = normalize(normalMatrix * teTangent);\n";
                // transform the binormal
                tessEvalShader << "\n\tvarBinormal" << outExt << " = normalize(normalMatrix * teBinormal);\n";
            }
        } else {
            for (TParamIter iter = m_interpolationParameters.begin(), end = m_interpolationParameters.end(); iter != end; ++iter) {
                tessEvalShader << "\t" << iter.key() << outExt << " = gl_TessCoord.x * " << iter.key() << "TC[0] + ";
                tessEvalShader << "gl_TessCoord.y * " << iter.key() << "TC[1] + ";
                tessEvalShader << "gl_TessCoord.z * " << iter.key() << "TC[2];\n";
            }

            // displacement mapping makes only sense with linear tessellation
            if (tessMode == TessellationModeValues::Linear && m_displacementImage) {
                QSSGDefaultMaterialShaderGeneratorInterface::ImageVariableNames
                        theNames = renderer.contextInterface()->defaultMaterialShaderGenerator()->getImageVariableNames(m_displacementIdx);
                tessEvalShader << "\tpos.xyz = defaultMaterialFileDisplacementTexture( " << theNames.m_imageSampler
                               << ", displaceAmount, " << theNames.m_imageFragCoords << outExt;
                tessEvalShader << ", varObjectNormal" << outExt << ", pos.xyz );"
                               << "\n";
                tessEvalShader << "\tvarWorldPos" << outExt << "= (modelMatrix * pos).xyz;"
                               << "\n";
                tessEvalShader << "\tvarViewVector" << outExt << "= normalize(cameraPosition - "
                               << "varWorldPos" << outExt << ");"
                               << "\n";
            }

            // transform the normal
            tessEvalShader << "\n\tvarNormal" << outExt << " = normalize(normalMatrix * varObjectNormal" << outExt << ");\n";
        }

        tessEvalShader.append("\tgl_Position = modelViewProjection * pos;\n");
    }

    void beginVertexGeneration(quint32 displacementImageIdx, QSSGRenderableImage *displacementImage) override
    {
        m_displacementIdx = displacementImageIdx;
        m_displacementImage = displacementImage;

        QSSGShaderGeneratorStageFlags theStages(QSSGShaderProgramGeneratorInterface::defaultFlags());
        if (tessMode != TessellationModeValues::NoTessellation) {
            theStages |= QSSGShaderGeneratorStage::TessControl;
            theStages |= QSSGShaderGeneratorStage::TessEval;
        }
        if (m_wireframe) {
            theStages |= QSSGShaderGeneratorStage::Geometry;
        }
        programGenerator()->beginProgram(theStages);
        if (tessMode != TessellationModeValues::NoTessellation) {
            initializeTessControlShader();
            initializeTessEvaluationShader();
        }
        if (m_wireframe) {
            initializeWireframeGeometryShader();
        }
        // Open up each stage.
        QSSGShaderStageGeneratorInterface &vertexShader(vertex());
        vertexShader.addIncoming("attr_pos", "vec3");
        vertexShader << "void main()"
                     << "\n"
                     << "{"
                     << "\n";
        vertexShader << "\tvec3 uTransform;"
                     << "\n";
        vertexShader << "\tvec3 vTransform;"
                     << "\n";

        if (displacementImage) {
            generateUVCoords();
            materialGenerator()->generateImageUVCoordinates(*this, displacementImageIdx, 0, *displacementImage);
            if (!hasTessellation()) {
                vertexShader.addUniform("displaceAmount", "float");
                // we create the world position setup here
                // because it will be replaced with the displaced position
                setCode(GenerationFlag::WorldPosition);
                vertexShader.addUniform("modelMatrix", "mat4");

                vertexShader.addInclude("defaultMaterialFileDisplacementTexture.glsllib");
                QSSGDefaultMaterialShaderGeneratorInterface::ImageVariableNames theVarNames = materialGenerator()->getImageVariableNames(
                        displacementImageIdx);

                vertexShader.addUniform(theVarNames.m_imageSampler, "sampler2D");

                vertexShader << "\tvec3 displacedPos = defaultMaterialFileDisplacementTexture( " << theVarNames.m_imageSampler
                             << ", displaceAmount, " << theVarNames.m_imageFragCoords << ", attr_norm, attr_pos );"
                             << "\n";
                addInterpolationParameter("varWorldPos", "vec3");
                vertexShader.append("\tvec3 local_model_world_position = (modelMatrix * "
                                    "vec4(displacedPos, 1.0)).xyz;");
                assignOutput("varWorldPos", "local_model_world_position");
            }
        }
        // for tessellation we pass on the position in object coordinates
        // Also note that gl_Position is written in the tess eval shader
        if (hasTessellation())
            vertexShader.append("\tgl_Position = vec4(attr_pos, 1.0);");
        else {
            vertexShader.addUniform("modelViewProjection", "mat4");
            if (displacementImage)
                vertexShader.append("\tgl_Position = modelViewProjection * vec4(displacedPos, 1.0);");
            else
                vertexShader.append("\tgl_Position = modelViewProjection * vec4(attr_pos, 1.0);");
        }

        if (hasTessellation()) {
            generateWorldPosition();
            generateWorldNormal();
            generateObjectNormal();
            generateVarTangentAndBinormal();
        }
    }

    void beginFragmentGeneration() override
    {
        fragment().addUniform("material_properties", "vec4");
        fragment() << "void main()"
                   << "\n"
                   << "{"
                   << "\n";
        // We do not pass object opacity through the pipeline.
        fragment() << "\tfloat objectOpacity = material_properties.a;"
                   << "\n";
    }

    void assignOutput(const QByteArray &inVarName, const QByteArray &inVarValue) override
    {
        vertex() << "\t" << inVarName << " = " << inVarValue << ";\n";
    }
    void doGenerateUVCoords(quint32 inUVSet = 0) override
    {
        Q_ASSERT(inUVSet == 0 || inUVSet == 1);

        if (inUVSet == 0) {
            vertex().addIncoming("attr_uv0", "vec2");
            vertex() << "\tvarTexCoord0 = attr_uv0;"
                     << "\n";
        } else if (inUVSet == 1) {
            vertex().addIncoming("attr_uv1", "vec2");
            vertex() << "\tvarTexCoord1 = attr_uv1;"
                     << "\n";
        }
    }

    // fragment shader expects varying vertex normal
    // lighting in vertex pipeline expects world_normal
    void doGenerateWorldNormal() override
    {
        QSSGShaderStageGeneratorInterface &vertexGenerator(vertex());
        vertexGenerator.addIncoming("attr_norm", "vec3");
        vertexGenerator.addUniform("normalMatrix", "mat3");
        if (hasTessellation() == false) {
            vertexGenerator.append("\tvec3 world_normal = normalize(normalMatrix * attr_norm).xyz;");
            vertexGenerator.append("\tvarNormal = world_normal;");
        }
    }
    void doGenerateObjectNormal() override
    {
        addInterpolationParameter("varObjectNormal", "vec3");
        vertex().append("\tvarObjectNormal = attr_norm;");
    }
    void doGenerateWorldPosition() override
    {
        vertex().append("\tvec3 local_model_world_position = (modelMatrix * vec4(attr_pos, 1.0)).xyz;");
        assignOutput("varWorldPos", "local_model_world_position");
    }

    void doGenerateVarTangentAndBinormal() override
    {
        vertex().addIncoming("attr_textan", "vec3");
        vertex().addIncoming("attr_binormal", "vec3");

        bool hasNPatchTessellation = tessMode == TessellationModeValues::NPatch;

        if (!hasNPatchTessellation) {
            vertex() << "\tvarTangent = normalMatrix * attr_textan;"
                     << "\n"
                     << "\tvarBinormal = normalMatrix * attr_binormal;"
                     << "\n";
        } else {
            vertex() << "\tvarTangent = attr_textan;"
                     << "\n"
                     << "\tvarBinormal = attr_binormal;"
                     << "\n";
        }
    }

    void doGenerateVertexColor() override
    {
        vertex().addIncoming("attr_color", "vec3");
        vertex().append("\tvarColor = attr_color;");
    }

    void endVertexGeneration(bool customShader) override
    {

        if (hasTessellation()) {
            // finalize tess control shader
            finalizeTessControlShader();
            // finalize tess evaluation shader
            finalizeTessEvaluationShader();

            tessControl().append("}");
            tessEval().append("}");
        }
        if (m_wireframe) {
            // finalize geometry shader
            finalizeWireframeGeometryShader();
            geometry().append("}");
        }
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
        if (hasTessellation()) {
            QByteArray nameBuilder(inName);
            nameBuilder.append("TC");
            tessControl().addOutgoing(nameBuilder, inType);

            nameBuilder = inName;
            if (programGenerator()->getEnabledStages() & QSSGShaderGeneratorStage::Geometry) {
                nameBuilder.append("TE");
                geometry().addOutgoing(inName, inType);
            }
            tessEval().addOutgoing(nameBuilder, inType);
        }
    }

    QSSGShaderStageGeneratorInterface &activeStage() override { return vertex(); }
};

static QByteArray logPrefix() { return QByteArrayLiteral("mesh subset pipeline-- "); }

QSSGRef<QSSGRenderShaderProgram> QSSGRendererImpl::generateShader(QSSGSubsetRenderable &inRenderable,
                                                                  const ShaderFeatureSetList &inFeatureSet)
{
    // build a string that allows us to print out the shader we are generating to the log.
    // This is time consuming but I feel like it doesn't happen all that often and is very
    // useful to users
    // looking at the log file.
    m_generatedShaderString = logPrefix();

    QSSGShaderDefaultMaterialKey theKey(inRenderable.shaderDescription);
    theKey.toString(m_generatedShaderString, m_defaultMaterialShaderKeyProperties);
    QSSGRef<QSSGShaderCache> theCache = m_contextInterface->shaderCache();
    const QSSGRef<QSSGRenderShaderProgram> &cachedProgram = theCache->getProgram(m_generatedShaderString, inFeatureSet);
    if (cachedProgram)
        return cachedProgram;

    QSSGSubsetMaterialVertexPipeline pipeline(*this,
                                                inRenderable,
                                                m_defaultMaterialShaderKeyProperties.m_wireframeMode.getValue(theKey));
    return m_contextInterface->defaultMaterialShaderGenerator()->generateShader(inRenderable.material,
                                                                               inRenderable.shaderDescription,
                                                                               pipeline,
                                                                               inFeatureSet,
                                                                               m_currentLayer->globalLights,
                                                                               inRenderable.firstImage,
                                                                               inRenderable.renderableFlags.hasTransparency(),
                                                                               logPrefix());
}

QSSGRef<QSSGRhiShaderStages> QSSGRendererImpl::generateRhiShaderStages(QSSGSubsetRenderable &inRenderable,
                                                                       const ShaderFeatureSetList &inFeatureSet)
{
    // build a string that allows us to print out the shader we are generating to the log.
    // This is time consuming but I feel like it doesn't happen all that often and is very
    // useful to users
    // looking at the log file.
    m_generatedShaderString = logPrefix();

    QSSGShaderDefaultMaterialKey theKey(inRenderable.shaderDescription);
    theKey.toString(m_generatedShaderString, m_defaultMaterialShaderKeyProperties);

    QSSGRef<QSSGShaderCache> theCache = m_contextInterface->shaderCache();
    const QSSGRef<QSSGRhiShaderStages> &cachedShaders = theCache->getRhiShaderStages(m_generatedShaderString, inFeatureSet);
    if (cachedShaders)
        return cachedShaders;

    QSSGSubsetMaterialVertexPipeline pipeline(*this,
                                              inRenderable,
                                              m_defaultMaterialShaderKeyProperties.m_wireframeMode.getValue(theKey));

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
