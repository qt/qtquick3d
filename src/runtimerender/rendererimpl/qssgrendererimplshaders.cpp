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

#if 0
static inline void addVertexDepth(QSSGShaderVertexCodeGenerator &vertexShader)
{
    // near plane, far plane
    vertexShader.addInclude("viewProperties.glsllib");
    vertexShader.addVarying("vertex_depth", "float");
    // the w coordinate is the unormalized distance to the object from the camera
    // We want the normalized distance, with 0 representing the far plane and 1 representing
    // the near plane, of the object in the vertex depth variable.

    vertexShader << "    vertex_depth = calculateVertexDepth( cameraProperties, gl_Position );"
                 << "\n";
}
#endif

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

        tessCtrlShader.append("    ctWorldPos[0] = varWorldPos[0];");
        tessCtrlShader.append("    ctWorldPos[1] = varWorldPos[1];");
        tessCtrlShader.append("    ctWorldPos[2] = varWorldPos[2];");

        if (tessMode == TessellationModeValues::Phong || tessMode == TessellationModeValues::NPatch) {
            tessCtrlShader.append("    ctNorm[0] = varObjectNormal[0];");
            tessCtrlShader.append("    ctNorm[1] = varObjectNormal[1];");
            tessCtrlShader.append("    ctNorm[2] = varObjectNormal[2];");
        }
        if (tessMode == TessellationModeValues::NPatch) {
            tessCtrlShader.append("    ctTangent[0] = varTangent[0];");
            tessCtrlShader.append("    ctTangent[1] = varTangent[1];");
            tessCtrlShader.append("    ctTangent[2] = varTangent[2];");
        }

        tessCtrlShader.append("    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;");
        tessCtrlShader.append("    tessShader( tessLevelOuter, tessLevelInner);\n");
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
            tessEvalShader.append("    ctNorm[0] = varObjectNormalTC[0];");
            tessEvalShader.append("    ctNorm[1] = varObjectNormalTC[1];");
            tessEvalShader.append("    ctNorm[2] = varObjectNormalTC[2];");

            tessEvalShader.append("    ctTangent[0] = varTangentTC[0];");
            tessEvalShader.append("    ctTangent[1] = varTangentTC[1];");
            tessEvalShader.append("    ctTangent[2] = varTangentTC[2];");
        }

        tessEvalShader.append("    vec4 pos = tessShader( );\n");
    }

    void finalizeTessControlShader()
    {
        QSSGShaderStageGeneratorInterface &tessCtrlShader(*programGenerator()->getStage(QSSGShaderGeneratorStage::TessControl));
        // add varyings we must pass through
        typedef TStrTableStrMap::const_iterator TParamIter;
        for (TParamIter iter = m_interpolationParameters.begin(), end = m_interpolationParameters.end(); iter != end; ++iter) {
            tessCtrlShader << "    " << iter.key() << "TC[gl_InvocationID] = " << iter.key() << "[gl_InvocationID];\n";
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
                tessEvalShader << "    " << iter.key() << outExt << " = gl_TessCoord.z * " << iter.key() << "TC[0] + ";
                tessEvalShader << "gl_TessCoord.x * " << iter.key() << "TC[1] + ";
                tessEvalShader << "gl_TessCoord.y * " << iter.key() << "TC[2];\n";
            }

            // transform the normal
            if (m_generationFlags & GenerationFlag::WorldNormal)
                tessEvalShader << "\n    varNormal" << outExt << " = normalize(normalMatrix * teNorm);\n";
            // transform the tangent
            if (m_generationFlags & GenerationFlag::TangentBinormal) {
                tessEvalShader << "\n    varTangent" << outExt << " = normalize(normalMatrix * teTangent);\n";
                // transform the binormal
                tessEvalShader << "\n    varBinormal" << outExt << " = normalize(normalMatrix * teBinormal);\n";
            }
        } else {
            for (TParamIter iter = m_interpolationParameters.begin(), end = m_interpolationParameters.end(); iter != end; ++iter) {
                tessEvalShader << "    " << iter.key() << outExt << " = gl_TessCoord.x * " << iter.key() << "TC[0] + ";
                tessEvalShader << "gl_TessCoord.y * " << iter.key() << "TC[1] + ";
                tessEvalShader << "gl_TessCoord.z * " << iter.key() << "TC[2];\n";
            }

            // displacement mapping makes only sense with linear tessellation
            if (tessMode == TessellationModeValues::Linear && m_displacementImage) {
                QSSGDefaultMaterialShaderGeneratorInterface::ImageVariableNames
                        theNames = renderer.contextInterface()->defaultMaterialShaderGenerator()->getImageVariableNames(m_displacementIdx);
                tessEvalShader << "    pos.xyz = defaultMaterialFileDisplacementTexture( " << theNames.m_imageSampler
                               << ", displaceAmount, " << theNames.m_imageFragCoords << outExt;
                tessEvalShader << ", varObjectNormal" << outExt << ", pos.xyz );"
                               << "\n";
                tessEvalShader << "    varWorldPos" << outExt << "= (modelMatrix * pos).xyz;"
                               << "\n";
                tessEvalShader << "    varViewVector" << outExt << "= normalize(cameraPosition - "
                               << "varWorldPos" << outExt << ");"
                               << "\n";
            }

            // transform the normal
            tessEvalShader << "\n    varNormal" << outExt << " = normalize(normalMatrix * varObjectNormal" << outExt << ");\n";
        }

        tessEvalShader.append("    gl_Position = modelViewProjection * pos;\n");
    }

    void beginVertexGeneration(const QSSGShaderDefaultMaterialKey &inKey, quint32 displacementImageIdx, QSSGRenderableImage *displacementImage) override
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
        vertexShader << "    vec3 uTransform;"
                     << "\n";
        vertexShader << "    vec3 vTransform;"
                     << "\n";

        if (displacementImage) {
            generateUVCoords(inKey);
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

                vertexShader << "    vec3 displacedPos = defaultMaterialFileDisplacementTexture( " << theVarNames.m_imageSampler
                             << ", displaceAmount, " << theVarNames.m_imageFragCoords << ", attr_norm, attr_pos );"
                             << "\n";
                addInterpolationParameter("varWorldPos", "vec3");
                vertexShader.append("    vec3 local_model_world_position = (modelMatrix * "
                                    "vec4(displacedPos, 1.0)).xyz;");
                assignOutput("varWorldPos", "local_model_world_position");
            }
        }
        // for tessellation we pass on the position in object coordinates
        // Also note that gl_Position is written in the tess eval shader
        if (hasTessellation())
            vertexShader.append("    gl_Position = vec4(attr_pos, 1.0);");
        else {
            vertexShader.addUniform("modelViewProjection", "mat4");
            if (displacementImage)
                vertexShader.append("    gl_Position = modelViewProjection * vec4(displacedPos, 1.0);");
            else
                vertexShader.append("    gl_Position = modelViewProjection * vec4(attr_pos, 1.0);");
        }

        if (hasTessellation()) {
            generateWorldPosition();
            generateWorldNormal(inKey);
            generateObjectNormal();
            generateVarTangentAndBinormal(inKey);
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
        fragment() << "    float objectOpacity = material_properties.a;"
                   << "\n";
    }

    void assignOutput(const QByteArray &inVarName, const QByteArray &inVarValue) override
    {
        vertex() << "    " << inVarName << " = " << inVarValue << ";\n";
    }
    void doGenerateUVCoords(quint32 inUVSet = 0) override
    {
        Q_ASSERT(inUVSet == 0 || inUVSet == 1);

        if (inUVSet == 0) {
            vertex().addIncoming("attr_uv0", "vec2");
            vertex() << "    varTexCoord0 = attr_uv0;\n";
        } else if (inUVSet == 1) {
            vertex().addIncoming("attr_uv1", "vec2");
            vertex() << "    varTexCoord1 = attr_uv1;\n";
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
            vertexGenerator.append("    vec3 world_normal = normalize(normalMatrix * attr_norm).xyz;");
            vertexGenerator.append("    varNormal = world_normal;");
        }
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

    void doGenerateVarTangent() override
    {
        vertex().addIncoming("attr_textan", "vec3");

        bool hasNPatchTessellation = tessMode == TessellationModeValues::NPatch;
        if (!hasNPatchTessellation)
            vertex() << "    varTangent = normalMatrix * attr_textan;\n";
        else
            vertex() << "    varTangent = attr_textan;\n";
    }

    void doGenerateVarBinormal() override
    {
        vertex().addIncoming("attr_binormal", "vec3");

        bool hasNPatchTessellation = tessMode == TessellationModeValues::NPatch;
        if (!hasNPatchTessellation)
            vertex() << "    varBinormal = normalMatrix * attr_binormal;\n";
        else
            vertex() << "    varBinormal = attr_binormal;\n";
    }

    void doGenerateVertexColor(const QSSGShaderDefaultMaterialKey &inKey) override
    {
        const bool meshHasColors = renderer.defaultMaterialShaderKeyProperties().m_vertexAttributes.getBitValue(
                QSSGShaderKeyVertexAttribute::Color, inKey);
        if (meshHasColors)
            vertex().addIncoming("attr_color", "vec4");
        else
            vertex().append("    vec4 attr_color = vec4(0.0, 0.0, 0.0, 1.0);");
        vertex().append("    varColor = attr_color;");
    }

    bool hasAttributeInKey(QSSGShaderKeyVertexAttribute::VertexAttributeBits inAttr, const QSSGShaderDefaultMaterialKey &inKey) override
    {
        return renderer.defaultMaterialShaderKeyProperties().m_vertexAttributes.getBitValue(inAttr, inKey);
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

// --------------  Special cases for shadows  -------------------

QSSGRef<QSSGRenderableDepthPrepassShader> QSSGRendererImpl::getParaboloidDepthShader(TessellationModeValues inTessMode)
{
    if (m_contextInterface->renderContext()->supportsTessellation()
            && inTessMode != TessellationModeValues::NoTessellation) {
        if (inTessMode == TessellationModeValues::Linear)
            return getParaboloidDepthTessLinearShader();
        if (inTessMode == TessellationModeValues::Phong)
            return getParaboloidDepthTessPhongShader();
        if (inTessMode == TessellationModeValues::NPatch)
            return getParaboloidDepthTessNPatchShader();
    }

    return getParaboloidDepthNoTessShader();
}

QSSGRef<QSSGRenderableDepthPrepassShader> QSSGRendererImpl::getParaboloidDepthNoTessShader()
{
    QSSGRef<QSSGRenderableDepthPrepassShader> &theDepthShader = m_paraboloidDepthShader;

    if (theDepthShader.isNull()) {
        QByteArray name = "paraboloid depth shader";

        QSSGRef<QSSGShaderCache> theCache = m_contextInterface->shaderCache();
        QSSGRef<QSSGRenderShaderProgram> depthShaderProgram = theCache->getProgram(name, ShaderFeatureSetList());
        if (!depthShaderProgram) {
            getProgramGenerator()->beginProgram();
            QSSGShaderStageGeneratorInterface &vertexShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Vertex));
            QSSGShaderStageGeneratorInterface &fragmentShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Fragment));
            QSSGShaderProgramGeneratorInterface::outputParaboloidDepthVertex(vertexShader);
            QSSGShaderProgramGeneratorInterface::outputParaboloidDepthFragment(fragmentShader);
            depthShaderProgram = getProgramGenerator()->compileGeneratedShader(name, QSSGShaderCacheProgramFlags(), ShaderFeatureSetList());
        }

        if (depthShaderProgram) {
            theDepthShader = QSSGRef<QSSGRenderableDepthPrepassShader>(
                    new QSSGRenderableDepthPrepassShader(depthShaderProgram, context()));
        } else {
            theDepthShader = QSSGRef<QSSGRenderableDepthPrepassShader>();
        }
    }

    return theDepthShader;
}

QSSGRef<QSSGRenderableDepthPrepassShader> QSSGRendererImpl::getParaboloidDepthTessLinearShader()
{
    QSSGRef<QSSGRenderableDepthPrepassShader> &theDepthShader = m_paraboloidDepthTessLinearShader;

    if (theDepthShader.isNull()) {
        QByteArray name = "paraboloid depth tess linear shader";

        QSSGRef<QSSGShaderCache> theCache = m_contextInterface->shaderCache();
        QSSGRef<QSSGRenderShaderProgram> depthShaderProgram = theCache->getProgram(name, ShaderFeatureSetList());
        if (!depthShaderProgram) {
            getProgramGenerator()->beginProgram(
                    QSSGShaderGeneratorStageFlags(QSSGShaderGeneratorStage::Vertex | QSSGShaderGeneratorStage::TessControl
                                               | QSSGShaderGeneratorStage::TessEval | QSSGShaderGeneratorStage::Fragment));
            QSSGShaderStageGeneratorInterface &vertexShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Vertex));
            QSSGShaderStageGeneratorInterface &tessCtrlShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::TessControl));
            QSSGShaderStageGeneratorInterface &tessEvalShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::TessEval));
            QSSGShaderStageGeneratorInterface &fragmentShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Fragment));

            vertexShader.addIncoming("attr_pos", "vec3");
            // vertexShader.AddOutgoing("world_pos", "vec4");
            vertexShader.addUniform("modelViewProjection", "mat4");

            vertexShader.append("void main() {");
            vertexShader.append("    gl_Position = vec4(attr_pos, 1.0);");
            // vertexShader.Append("    world_pos = attr_pos;");
            vertexShader.append("}");

            tessCtrlShader.addInclude("tessellationLinear.glsllib");
            tessCtrlShader.addUniform("tessLevelInner", "float");
            tessCtrlShader.addUniform("tessLevelOuter", "float");
            // tessCtrlShader.AddOutgoing( "outUVTC", "vec2" );
            // tessCtrlShader.AddOutgoing( "outNormalTC", "vec3" );
            tessCtrlShader.append("void main() {\n");
            // tessCtrlShader.Append("    ctWorldPos[0] = outWorldPos[0];");
            // tessCtrlShader.Append("    ctWorldPos[1] = outWorldPos[1];");
            // tessCtrlShader.Append("    ctWorldPos[2] = outWorldPos[2];");
            tessCtrlShader.append("    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;");
            tessCtrlShader.append("    tessShader( tessLevelOuter, tessLevelInner);\n");
            tessCtrlShader.append("}");

            tessEvalShader.addInclude("tessellationLinear.glsllib");
            tessEvalShader.addUniform("modelViewProjection", "mat4");
            tessEvalShader.addOutgoing("world_pos", "vec4");
            tessEvalShader.append("void main() {");
            tessEvalShader.append("    vec4 pos = tessShader( );\n");
            QSSGShaderProgramGeneratorInterface::outputParaboloidDepthTessEval(tessEvalShader);
            tessEvalShader.append("}");

            QSSGShaderProgramGeneratorInterface::outputParaboloidDepthFragment(fragmentShader);
            depthShaderProgram = getProgramGenerator()->compileGeneratedShader(name, QSSGShaderCacheProgramFlags(), ShaderFeatureSetList());
        }

        if (depthShaderProgram) {
            theDepthShader = QSSGRef<QSSGRenderableDepthPrepassShader>(
                    new QSSGRenderableDepthPrepassShader(depthShaderProgram, context()));
        } else {
            theDepthShader = QSSGRef<QSSGRenderableDepthPrepassShader>();
        }
    }

    return theDepthShader;
}

QSSGRef<QSSGRenderableDepthPrepassShader> QSSGRendererImpl::getParaboloidDepthTessPhongShader()
{
    QSSGRef<QSSGRenderableDepthPrepassShader> &theDepthShader = m_paraboloidDepthTessPhongShader;

    if (theDepthShader.isNull()) {
        QByteArray name = "paraboloid depth tess phong shader";

        QSSGRef<QSSGShaderCache> theCache = m_contextInterface->shaderCache();
        QSSGRef<QSSGRenderShaderProgram> depthShaderProgram = theCache->getProgram(name, ShaderFeatureSetList());
        if (!depthShaderProgram) {
            getProgramGenerator()->beginProgram(
                    QSSGShaderGeneratorStageFlags(QSSGShaderGeneratorStage::Vertex | QSSGShaderGeneratorStage::TessControl
                                               | QSSGShaderGeneratorStage::TessEval | QSSGShaderGeneratorStage::Fragment));
            QSSGShaderStageGeneratorInterface &vertexShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Vertex));
            QSSGShaderStageGeneratorInterface &tessCtrlShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::TessControl));
            QSSGShaderStageGeneratorInterface &tessEvalShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::TessEval));
            QSSGShaderStageGeneratorInterface &fragmentShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Fragment));

            vertexShader.addIncoming("attr_pos", "vec3");
            // vertexShader.AddOutgoing("world_pos", "vec4");
            vertexShader.addUniform("modelViewProjection", "mat4");

            vertexShader.append("void main() {");
            vertexShader.append("    gl_Position = vec4(attr_pos, 1.0);");
            // vertexShader.Append("    world_pos = attr_pos;");
            vertexShader.append("}");

            tessCtrlShader.addInclude("tessellationPhong.glsllib");
            tessCtrlShader.addUniform("tessLevelInner", "float");
            tessCtrlShader.addUniform("tessLevelOuter", "float");
            // tessCtrlShader.AddOutgoing( "outUVTC", "vec2" );
            // tessCtrlShader.AddOutgoing( "outNormalTC", "vec3" );
            tessCtrlShader.append("void main() {\n");
            // tessCtrlShader.Append("    ctWorldPos[0] = outWorldPos[0];");
            // tessCtrlShader.Append("    ctWorldPos[1] = outWorldPos[1];");
            // tessCtrlShader.Append("    ctWorldPos[2] = outWorldPos[2];");
            tessCtrlShader.append("    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;");
            tessCtrlShader.append("    tessShader( tessLevelOuter, tessLevelInner);\n");
            tessCtrlShader.append("}");

            tessEvalShader.addInclude("tessellationPhong.glsllib");
            tessEvalShader.addUniform("modelViewProjection", "mat4");
            tessEvalShader.addOutgoing("world_pos", "vec4");
            tessEvalShader.append("void main() {");
            tessEvalShader.append("    vec4 pos = tessShader( );\n");
            QSSGShaderProgramGeneratorInterface::outputParaboloidDepthTessEval(tessEvalShader);
            tessEvalShader.append("}");

            QSSGShaderProgramGeneratorInterface::outputParaboloidDepthFragment(fragmentShader);
            depthShaderProgram = getProgramGenerator()->compileGeneratedShader(name, QSSGShaderCacheProgramFlags(), ShaderFeatureSetList());
        }

        if (depthShaderProgram) {
            theDepthShader = QSSGRef<QSSGRenderableDepthPrepassShader>(
                    new QSSGRenderableDepthPrepassShader(depthShaderProgram, context()));
        } else {
            theDepthShader = QSSGRef<QSSGRenderableDepthPrepassShader>();
        }
    }

    return theDepthShader;
}

QSSGRef<QSSGRenderableDepthPrepassShader> QSSGRendererImpl::getParaboloidDepthTessNPatchShader()
{
    QSSGRef<QSSGRenderableDepthPrepassShader> &theDepthShader = m_paraboloidDepthTessNPatchShader;

    if (theDepthShader.isNull()) {
        QByteArray name = "paraboloid depth tess NPatch shader";

        QSSGRef<QSSGShaderCache> theCache = m_contextInterface->shaderCache();
        QSSGRef<QSSGRenderShaderProgram> depthShaderProgram = theCache->getProgram(name, ShaderFeatureSetList());
        if (!depthShaderProgram) {
            getProgramGenerator()->beginProgram(
                    QSSGShaderGeneratorStageFlags(QSSGShaderGeneratorStage::Vertex | QSSGShaderGeneratorStage::TessControl
                                               | QSSGShaderGeneratorStage::TessEval | QSSGShaderGeneratorStage::Fragment));
            QSSGShaderStageGeneratorInterface &vertexShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Vertex));
            QSSGShaderStageGeneratorInterface &tessCtrlShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::TessControl));
            QSSGShaderStageGeneratorInterface &tessEvalShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::TessEval));
            QSSGShaderStageGeneratorInterface &fragmentShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Fragment));

            vertexShader.addIncoming("attr_pos", "vec3");
            // vertexShader.AddOutgoing("world_pos", "vec4");
            vertexShader.addUniform("modelViewProjection", "mat4");

            vertexShader.append("void main() {");
            vertexShader.append("    gl_Position = vec4(attr_pos, 1.0);");
            // vertexShader.Append("    world_pos = attr_pos;");
            vertexShader.append("}");

            tessCtrlShader.addInclude("tessellationNPatch.glsllib");
            tessCtrlShader.addUniform("tessLevelInner", "float");
            tessCtrlShader.addUniform("tessLevelOuter", "float");
            // tessCtrlShader.AddOutgoing( "outUVTC", "vec2" );
            // tessCtrlShader.AddOutgoing( "outNormalTC", "vec3" );
            tessCtrlShader.append("void main() {\n");
            // tessCtrlShader.Append("    ctWorldPos[0] = outWorldPos[0];");
            // tessCtrlShader.Append("    ctWorldPos[1] = outWorldPos[1];");
            // tessCtrlShader.Append("    ctWorldPos[2] = outWorldPos[2];");
            tessCtrlShader.append("    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;");
            tessCtrlShader.append("    tessShader( tessLevelOuter, tessLevelInner);\n");
            tessCtrlShader.append("}");

            tessEvalShader.addInclude("tessellationNPatch.glsllib");
            tessEvalShader.addUniform("modelViewProjection", "mat4");
            tessEvalShader.addOutgoing("world_pos", "vec4");
            tessEvalShader.append("void main() {");
            tessEvalShader.append("    vec4 pos = tessShader( );\n");
            QSSGShaderProgramGeneratorInterface::outputParaboloidDepthTessEval(tessEvalShader);
            tessEvalShader.append("}");

            QSSGShaderProgramGeneratorInterface::outputParaboloidDepthFragment(fragmentShader);
            depthShaderProgram = getProgramGenerator()->compileGeneratedShader(name, QSSGShaderCacheProgramFlags(), ShaderFeatureSetList());
        }

        if (depthShaderProgram) {
            theDepthShader = QSSGRef<QSSGRenderableDepthPrepassShader>(
                    new QSSGRenderableDepthPrepassShader(depthShaderProgram, context()));
        } else {
            theDepthShader = QSSGRef<QSSGRenderableDepthPrepassShader>();
        }
    }

    return theDepthShader;
}

QSSGRef<QSSGRenderableDepthPrepassShader> QSSGRendererImpl::getCubeShadowDepthShader(TessellationModeValues inTessMode)
{
    if (m_contextInterface->renderContext()->supportsTessellation()
            && inTessMode != TessellationModeValues::NoTessellation) {
        if (inTessMode == TessellationModeValues::Linear)
            return getCubeDepthTessLinearShader();
        if (inTessMode == TessellationModeValues::Phong)
            return getCubeDepthTessPhongShader();
        if (inTessMode == TessellationModeValues::NPatch)
            return getCubeDepthTessNPatchShader();
    }

    return getCubeDepthNoTessShader();
}

QSSGRef<QSSGRenderableDepthPrepassShader> QSSGRendererImpl::getCubeDepthNoTessShader()
{
    QSSGRef<QSSGRenderableDepthPrepassShader> &theDepthShader = m_cubemapDepthShader;

    if (theDepthShader.isNull()) {
        QByteArray name = "cubemap face depth shader";

        QSSGRef<QSSGShaderCache> theCache = m_contextInterface->shaderCache();
        QSSGRef<QSSGRenderShaderProgram> depthShaderProgram = theCache->getProgram(name, ShaderFeatureSetList());

        if (!depthShaderProgram) {
            // GetProgramGenerator()->BeginProgram(
            // TShaderGeneratorStageFlags(ShaderGeneratorStages::Vertex |
            // ShaderGeneratorStages::Fragment | ShaderGeneratorStages::Geometry) );
            getProgramGenerator()->beginProgram();
            QSSGShaderStageGeneratorInterface &vertexShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Vertex));
            QSSGShaderStageGeneratorInterface &fragmentShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Fragment));
            // IShaderStageGenerator& geometryShader( *GetProgramGenerator()->GetStage(
            // ShaderGeneratorStages::Geometry ) );

            QSSGShaderProgramGeneratorInterface::outputCubeFaceDepthVertex(vertexShader);
            // IShaderProgramGenerator::OutputCubeFaceDepthGeometry( geometryShader );
            QSSGShaderProgramGeneratorInterface::outputCubeFaceDepthFragment(fragmentShader);
            depthShaderProgram = getProgramGenerator()->compileGeneratedShader(name, QSSGShaderCacheProgramFlags(), ShaderFeatureSetList());
        } else if (theCache->isShaderCachePersistenceEnabled()) {
            // we load from shader cache set default shader stages
            getProgramGenerator()->beginProgram();
            depthShaderProgram = getProgramGenerator()->compileGeneratedShader(name, QSSGShaderCacheProgramFlags(), ShaderFeatureSetList());
        }

        if (depthShaderProgram) {
            theDepthShader = QSSGRef<QSSGRenderableDepthPrepassShader>(
                    new QSSGRenderableDepthPrepassShader(depthShaderProgram, context()));
        } else {
            theDepthShader = QSSGRef<QSSGRenderableDepthPrepassShader>();
        }
    }

    return theDepthShader;
}

QSSGRef<QSSGRenderableDepthPrepassShader> QSSGRendererImpl::getCubeDepthTessLinearShader()
{
    QSSGRef<QSSGRenderableDepthPrepassShader> &theDepthShader = m_cubemapDepthTessLinearShader;

    if (theDepthShader.isNull()) {
        QByteArray name = "cubemap face depth linear tess shader";

        QSSGRef<QSSGShaderCache> theCache = m_contextInterface->shaderCache();
        QSSGRef<QSSGRenderShaderProgram> depthShaderProgram = theCache->getProgram(name, ShaderFeatureSetList());

        if (!depthShaderProgram) {
            // GetProgramGenerator()->BeginProgram(
            // TShaderGeneratorStageFlags(ShaderGeneratorStages::Vertex |
            // ShaderGeneratorStages::Fragment | ShaderGeneratorStages::Geometry) );
            getProgramGenerator()->beginProgram(
                    QSSGShaderGeneratorStageFlags(QSSGShaderGeneratorStage::Vertex | QSSGShaderGeneratorStage::TessControl
                                               | QSSGShaderGeneratorStage::TessEval | QSSGShaderGeneratorStage::Fragment));
            QSSGShaderStageGeneratorInterface &vertexShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Vertex));
            QSSGShaderStageGeneratorInterface &fragmentShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Fragment));
            // IShaderStageGenerator& geometryShader( *GetProgramGenerator()->GetStage(
            // ShaderGeneratorStages::Geometry ) );
            QSSGShaderStageGeneratorInterface &tessCtrlShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::TessControl));
            QSSGShaderStageGeneratorInterface &tessEvalShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::TessEval));

            vertexShader.addIncoming("attr_pos", "vec3");
            vertexShader.append("void main() {");
            vertexShader.append("    gl_Position = vec4(attr_pos, 1.0);");
            vertexShader.append("}");

            // IShaderProgramGenerator::OutputCubeFaceDepthGeometry( geometryShader );
            QSSGShaderProgramGeneratorInterface::outputCubeFaceDepthFragment(fragmentShader);

            tessCtrlShader.addInclude("tessellationLinear.glsllib");
            tessCtrlShader.addUniform("tessLevelInner", "float");
            tessCtrlShader.addUniform("tessLevelOuter", "float");
            tessCtrlShader.append("void main() {\n");
            tessCtrlShader.append("    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;");
            tessCtrlShader.append("    tessShader( tessLevelOuter, tessLevelInner);\n");
            tessCtrlShader.append("}");

            tessEvalShader.addInclude("tessellationLinear.glsllib");
            tessEvalShader.addUniform("modelViewProjection", "mat4");
            tessEvalShader.addUniform("modelMatrix", "mat4");
            tessEvalShader.addOutgoing("world_pos", "vec4");
            tessEvalShader.append("void main() {");
            tessEvalShader.append("    vec4 pos = tessShader( );\n");
            tessEvalShader.append("    world_pos = modelMatrix * pos;");
            tessEvalShader.append("    world_pos /= world_pos.w;");
            tessEvalShader.append("    gl_Position = modelViewProjection * pos;");
            tessEvalShader.append("}");

            depthShaderProgram = getProgramGenerator()->compileGeneratedShader(name, QSSGShaderCacheProgramFlags(), ShaderFeatureSetList());
        }

        if (depthShaderProgram) {
            theDepthShader = QSSGRef<QSSGRenderableDepthPrepassShader>(
                    new QSSGRenderableDepthPrepassShader(depthShaderProgram, context()));
        } else {
            theDepthShader = QSSGRef<QSSGRenderableDepthPrepassShader>();
        }
    }

    return theDepthShader;
}

QSSGRef<QSSGRenderableDepthPrepassShader> QSSGRendererImpl::getCubeDepthTessPhongShader()
{
    QSSGRef<QSSGRenderableDepthPrepassShader> &theDepthShader = m_cubemapDepthTessPhongShader;

    if (theDepthShader.isNull()) {
        QByteArray name = "cubemap face depth phong tess shader";

        QSSGRef<QSSGShaderCache> theCache = m_contextInterface->shaderCache();
        QSSGRef<QSSGRenderShaderProgram> depthShaderProgram = theCache->getProgram(name, ShaderFeatureSetList());

        if (!depthShaderProgram) {
            // GetProgramGenerator()->BeginProgram(
            // TShaderGeneratorStageFlags(ShaderGeneratorStages::Vertex |
            // ShaderGeneratorStages::Fragment | ShaderGeneratorStages::Geometry) );
            getProgramGenerator()->beginProgram(
                    QSSGShaderGeneratorStageFlags(QSSGShaderGeneratorStage::Vertex | QSSGShaderGeneratorStage::TessControl
                                               | QSSGShaderGeneratorStage::TessEval | QSSGShaderGeneratorStage::Fragment));
            QSSGShaderStageGeneratorInterface &vertexShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Vertex));
            QSSGShaderStageGeneratorInterface &fragmentShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Fragment));
            // IShaderStageGenerator& geometryShader( *GetProgramGenerator()->GetStage(
            // ShaderGeneratorStages::Geometry ) );
            QSSGShaderStageGeneratorInterface &tessCtrlShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::TessControl));
            QSSGShaderStageGeneratorInterface &tessEvalShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::TessEval));

            vertexShader.addIncoming("attr_pos", "vec3");
            vertexShader.addIncoming("attr_norm", "vec3");
            vertexShader.addOutgoing("outNormal", "vec3");
            vertexShader.append("void main() {");
            vertexShader.append("    gl_Position = vec4(attr_pos, 1.0);");
            vertexShader.append("    outNormal = attr_norm;");
            vertexShader.append("}");

            // IShaderProgramGenerator::OutputCubeFaceDepthGeometry( geometryShader );
            QSSGShaderProgramGeneratorInterface::outputCubeFaceDepthFragment(fragmentShader);

            tessCtrlShader.addInclude("tessellationPhong.glsllib");
            tessCtrlShader.addUniform("tessLevelInner", "float");
            tessCtrlShader.addUniform("tessLevelOuter", "float");
            tessCtrlShader.append("void main() {\n");
            tessCtrlShader.append("    ctNorm[0] = outNormal[0];");
            tessCtrlShader.append("    ctNorm[1] = outNormal[1];");
            tessCtrlShader.append("    ctNorm[2] = outNormal[2];");
            tessCtrlShader.append("    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;");
            tessCtrlShader.append("    tessShader( tessLevelOuter, tessLevelInner);\n");
            tessCtrlShader.append("}");

            tessEvalShader.addInclude("tessellationPhong.glsllib");
            tessEvalShader.addUniform("modelViewProjection", "mat4");
            tessEvalShader.addUniform("modelMatrix", "mat4");
            tessEvalShader.addOutgoing("world_pos", "vec4");
            tessEvalShader.append("void main() {");
            tessEvalShader.append("    vec4 pos = tessShader( );\n");
            tessEvalShader.append("    world_pos = modelMatrix * pos;");
            tessEvalShader.append("    world_pos /= world_pos.w;");
            tessEvalShader.append("    gl_Position = modelViewProjection * pos;");
            tessEvalShader.append("}");

            depthShaderProgram = getProgramGenerator()->compileGeneratedShader(name, QSSGShaderCacheProgramFlags(), ShaderFeatureSetList());
        }

        if (depthShaderProgram) {
            theDepthShader = QSSGRef<QSSGRenderableDepthPrepassShader>(
                    new QSSGRenderableDepthPrepassShader(depthShaderProgram, context()));
        } else {
            theDepthShader = QSSGRef<QSSGRenderableDepthPrepassShader>();
        }
    }

    return theDepthShader;
}

QSSGRef<QSSGRenderableDepthPrepassShader> QSSGRendererImpl::getCubeDepthTessNPatchShader()
{
    QSSGRef<QSSGRenderableDepthPrepassShader> &theDepthShader = m_cubemapDepthTessNPatchShader;

    if (theDepthShader.isNull()) {
        QByteArray name = "cubemap face depth npatch tess shader";

        QSSGRef<QSSGShaderCache> theCache = m_contextInterface->shaderCache();
        QSSGRef<QSSGRenderShaderProgram> depthShaderProgram = theCache->getProgram(name, ShaderFeatureSetList());

        if (!depthShaderProgram) {
            // GetProgramGenerator()->BeginProgram(
            // TShaderGeneratorStageFlags(ShaderGeneratorStages::Vertex |
            // ShaderGeneratorStages::Fragment | ShaderGeneratorStages::Geometry) );
            getProgramGenerator()->beginProgram(
                    QSSGShaderGeneratorStageFlags(QSSGShaderGeneratorStage::Vertex | QSSGShaderGeneratorStage::TessControl
                                               | QSSGShaderGeneratorStage::TessEval | QSSGShaderGeneratorStage::Fragment));
            QSSGShaderStageGeneratorInterface &vertexShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Vertex));
            QSSGShaderStageGeneratorInterface &fragmentShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Fragment));
            // IShaderStageGenerator& geometryShader( *GetProgramGenerator()->GetStage(
            // ShaderGeneratorStages::Geometry ) );
            QSSGShaderStageGeneratorInterface &tessCtrlShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::TessControl));
            QSSGShaderStageGeneratorInterface &tessEvalShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::TessEval));

            vertexShader.addIncoming("attr_pos", "vec3");
            vertexShader.addIncoming("attr_norm", "vec3");
            vertexShader.addOutgoing("outNormal", "vec3");
            vertexShader.append("void main() {");
            vertexShader.append("    gl_Position = vec4(attr_pos, 1.0);");
            vertexShader.append("    outNormal = attr_norm;");
            vertexShader.append("}");

            // IShaderProgramGenerator::OutputCubeFaceDepthGeometry( geometryShader );
            QSSGShaderProgramGeneratorInterface::outputCubeFaceDepthFragment(fragmentShader);

            tessCtrlShader.addOutgoing("outNormalTC", "vec3");
            tessCtrlShader.addInclude("tessellationNPatch.glsllib");
            tessCtrlShader.addUniform("tessLevelInner", "float");
            tessCtrlShader.addUniform("tessLevelOuter", "float");
            tessCtrlShader.append("void main() {\n");
            tessCtrlShader.append("    ctNorm[0] = outNormal[0];");
            tessCtrlShader.append("    ctNorm[1] = outNormal[1];");
            tessCtrlShader.append("    ctNorm[2] = outNormal[2];");
            tessCtrlShader.append("    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;");
            tessCtrlShader.append("    tessShader( tessLevelOuter, tessLevelInner);\n");
            tessCtrlShader.append("    outNormalTC[gl_InvocationID] = outNormal[gl_InvocationID];\n");
            tessCtrlShader.append("}");

            tessEvalShader.addInclude("tessellationNPatch.glsllib");
            tessEvalShader.addUniform("modelViewProjection", "mat4");
            tessEvalShader.addUniform("modelMatrix", "mat4");
            tessEvalShader.addOutgoing("world_pos", "vec4");
            tessEvalShader.append("void main() {");
            tessEvalShader.append("    ctNorm[0] = outNormalTC[0];");
            tessEvalShader.append("    ctNorm[1] = outNormalTC[1];");
            tessEvalShader.append("    ctNorm[2] = outNormalTC[2];");
            tessEvalShader.append("    vec4 pos = tessShader( );\n");
            tessEvalShader.append("    world_pos = modelMatrix * pos;");
            tessEvalShader.append("    world_pos /= world_pos.w;");
            tessEvalShader.append("    gl_Position = modelViewProjection * pos;");
            tessEvalShader.append("}");

            depthShaderProgram = getProgramGenerator()->compileGeneratedShader(name, QSSGShaderCacheProgramFlags(), ShaderFeatureSetList());
        }

        if (depthShaderProgram) {
            theDepthShader = QSSGRef<QSSGRenderableDepthPrepassShader>(
                    new QSSGRenderableDepthPrepassShader(depthShaderProgram, context()));
        } else {
            theDepthShader = QSSGRef<QSSGRenderableDepthPrepassShader>();
        }
    }

    return theDepthShader;
}

QSSGRef<QSSGRenderableDepthPrepassShader> QSSGRendererImpl::getOrthographicDepthShader(TessellationModeValues inTessMode)
{
    if (m_contextInterface->renderContext()->supportsTessellation()
            && inTessMode != TessellationModeValues::NoTessellation) {
        if (inTessMode == TessellationModeValues::Linear)
            return getOrthographicDepthTessLinearShader();
        if (inTessMode == TessellationModeValues::Phong)
            return getOrthographicDepthTessPhongShader();
        if (inTessMode == TessellationModeValues::NPatch)
            return getOrthographicDepthTessNPatchShader();
    }

    return getOrthographicDepthNoTessShader();
}

QSSGRef<QSSGRenderableDepthPrepassShader> QSSGRendererImpl::getOrthographicDepthNoTessShader()
{
    QSSGRef<QSSGRenderableDepthPrepassShader> &theDepthShader = m_orthographicDepthShader;

    if (theDepthShader.isNull()) {
        QByteArray name = "orthographic depth shader";

        QSSGRef<QSSGShaderCache> theCache = m_contextInterface->shaderCache();
        QSSGRef<QSSGRenderShaderProgram> depthShaderProgram = theCache->getProgram(name, ShaderFeatureSetList());
        if (!depthShaderProgram) {
            getProgramGenerator()->beginProgram();
            QSSGShaderStageGeneratorInterface &vertexShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Vertex));
            QSSGShaderStageGeneratorInterface &fragmentShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Fragment));
            vertexShader.addIncoming("attr_pos", "vec3");
            vertexShader.addUniform("modelViewProjection", "mat4");
            vertexShader.addOutgoing("outDepth", "vec3");
            vertexShader.append("void main() {");
            vertexShader.append("   gl_Position = modelViewProjection * vec4( attr_pos, 1.0 );");
            vertexShader.append("   outDepth.x = gl_Position.z / gl_Position.w;");
            vertexShader.append("}");
            fragmentShader.append("void main() {");
            fragmentShader.append("    float depth = (outDepth.x + 1.0) * 0.5;");
            fragmentShader.append("    fragOutput = vec4(depth);");
            fragmentShader.append("}");

            depthShaderProgram = getProgramGenerator()->compileGeneratedShader(name, QSSGShaderCacheProgramFlags(), ShaderFeatureSetList());
        }

        if (depthShaderProgram) {
            theDepthShader = QSSGRef<QSSGRenderableDepthPrepassShader>(
                    new QSSGRenderableDepthPrepassShader(depthShaderProgram, context()));
        } else {
            theDepthShader = QSSGRef<QSSGRenderableDepthPrepassShader>();
        }
    }

    return theDepthShader;
}

QSSGRef<QSSGRenderableDepthPrepassShader> QSSGRendererImpl::getOrthographicDepthTessLinearShader()
{
    QSSGRef<QSSGRenderableDepthPrepassShader> &theDepthShader = m_orthographicDepthTessLinearShader;

    if (theDepthShader.isNull()) {
        QByteArray name = "orthographic depth tess linear shader";

        QSSGRef<QSSGShaderCache> theCache = m_contextInterface->shaderCache();
        QSSGRef<QSSGRenderShaderProgram> depthShaderProgram = theCache->getProgram(name, ShaderFeatureSetList());
        if (!depthShaderProgram) {
            getProgramGenerator()->beginProgram(
                    QSSGShaderGeneratorStageFlags(QSSGShaderGeneratorStage::Vertex | QSSGShaderGeneratorStage::TessControl
                                               | QSSGShaderGeneratorStage::TessEval | QSSGShaderGeneratorStage::Fragment));
            QSSGShaderStageGeneratorInterface &vertexShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Vertex));
            QSSGShaderStageGeneratorInterface &tessCtrlShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::TessControl));
            QSSGShaderStageGeneratorInterface &tessEvalShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::TessEval));
            QSSGShaderStageGeneratorInterface &fragmentShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Fragment));

            vertexShader.addIncoming("attr_pos", "vec3");
            vertexShader.addUniform("modelViewProjection", "mat4");

            vertexShader.append("void main() {");
            vertexShader.append("    gl_Position = vec4(attr_pos, 1.0);");
            vertexShader.append("}");
            fragmentShader.append("void main() {");
            fragmentShader.append("    float depth = (outDepth.x + 1.0) * 0.5;");
            fragmentShader.append("    fragOutput = vec4(depth);");
            fragmentShader.append("}");

            tessCtrlShader.addInclude("tessellationLinear.glsllib");
            tessCtrlShader.addUniform("tessLevelInner", "float");
            tessCtrlShader.addUniform("tessLevelOuter", "float");
            tessCtrlShader.append("void main() {\n");
            tessCtrlShader.append("    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;");
            tessCtrlShader.append("    tessShader( tessLevelOuter, tessLevelInner);\n");
            tessCtrlShader.append("}");

            tessEvalShader.addInclude("tessellationLinear.glsllib");
            tessEvalShader.addUniform("modelViewProjection", "mat4");
            tessEvalShader.addOutgoing("outDepth", "vec3");
            tessEvalShader.append("void main() {");
            tessEvalShader.append("    vec4 pos = tessShader( );\n");
            tessEvalShader.append("    gl_Position = modelViewProjection * pos;");
            tessEvalShader.append("    outDepth.x = gl_Position.z / gl_Position.w;");
            tessEvalShader.append("}");

            depthShaderProgram = getProgramGenerator()->compileGeneratedShader(name, QSSGShaderCacheProgramFlags(), ShaderFeatureSetList());
        }

        if (depthShaderProgram) {
            theDepthShader = QSSGRef<QSSGRenderableDepthPrepassShader>(
                    new QSSGRenderableDepthPrepassShader(depthShaderProgram, context()));
        } else {
            theDepthShader = QSSGRef<QSSGRenderableDepthPrepassShader>();
        }
    }

    return theDepthShader;
}

QSSGRef<QSSGRenderableDepthPrepassShader> QSSGRendererImpl::getOrthographicDepthTessPhongShader()
{
    QSSGRef<QSSGRenderableDepthPrepassShader> &theDepthShader = m_orthographicDepthTessPhongShader;

    if (theDepthShader.isNull()) {
        QByteArray name = "orthographic depth tess phong shader";

        QSSGRef<QSSGShaderCache> theCache = m_contextInterface->shaderCache();
        QSSGRef<QSSGRenderShaderProgram> depthShaderProgram = theCache->getProgram(name, ShaderFeatureSetList());
        if (!depthShaderProgram) {
            getProgramGenerator()->beginProgram(
                    QSSGShaderGeneratorStageFlags(QSSGShaderGeneratorStage::Vertex | QSSGShaderGeneratorStage::TessControl
                                               | QSSGShaderGeneratorStage::TessEval | QSSGShaderGeneratorStage::Fragment));
            QSSGShaderStageGeneratorInterface &vertexShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Vertex));
            QSSGShaderStageGeneratorInterface &tessCtrlShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::TessControl));
            QSSGShaderStageGeneratorInterface &tessEvalShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::TessEval));
            QSSGShaderStageGeneratorInterface &fragmentShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Fragment));

            vertexShader.addIncoming("attr_pos", "vec3");
            vertexShader.addIncoming("attr_norm", "vec3");
            vertexShader.addOutgoing("outNormal", "vec3");
            vertexShader.addUniform("modelViewProjection", "mat4");

            vertexShader.append("void main() {");
            vertexShader.append("    gl_Position = vec4(attr_pos, 1.0);");
            vertexShader.append("    outNormal = attr_norm;");
            vertexShader.append("}");
            fragmentShader.append("void main() {");
            fragmentShader.append("    float depth = (outDepth.x + 1.0) * 0.5;");
            fragmentShader.append("    fragOutput = vec4(depth);");
            fragmentShader.append("}");

            tessCtrlShader.addInclude("tessellationPhong.glsllib");
            tessCtrlShader.addUniform("tessLevelInner", "float");
            tessCtrlShader.addUniform("tessLevelOuter", "float");
            tessCtrlShader.append("void main() {\n");
            tessCtrlShader.append("    ctNorm[0] = outNormal[0];");
            tessCtrlShader.append("    ctNorm[1] = outNormal[1];");
            tessCtrlShader.append("    ctNorm[2] = outNormal[2];");
            tessCtrlShader.append("    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;");
            tessCtrlShader.append("    tessShader( tessLevelOuter, tessLevelInner);\n");
            tessCtrlShader.append("}");

            tessEvalShader.addInclude("tessellationPhong.glsllib");
            tessEvalShader.addUniform("modelViewProjection", "mat4");
            tessEvalShader.addOutgoing("outDepth", "vec3");
            tessEvalShader.append("void main() {");
            tessEvalShader.append("    vec4 pos = tessShader( );\n");
            tessEvalShader.append("    gl_Position = modelViewProjection * pos;");
            tessEvalShader.append("    outDepth.x = gl_Position.z / gl_Position.w;");
            tessEvalShader.append("}");

            depthShaderProgram = getProgramGenerator()->compileGeneratedShader(name, QSSGShaderCacheProgramFlags(), ShaderFeatureSetList());
        }

        if (depthShaderProgram) {
            theDepthShader = QSSGRef<QSSGRenderableDepthPrepassShader>(
                    new QSSGRenderableDepthPrepassShader(depthShaderProgram, context()));
        } else {
            theDepthShader = QSSGRef<QSSGRenderableDepthPrepassShader>();
        }
    }

    return theDepthShader;
}

QSSGRef<QSSGRenderableDepthPrepassShader> QSSGRendererImpl::getOrthographicDepthTessNPatchShader()
{
    QSSGRef<QSSGRenderableDepthPrepassShader> &theDepthShader = m_orthographicDepthTessNPatchShader;

    if (theDepthShader.isNull()) {
        QByteArray name = "orthographic depth tess npatch shader";

        QSSGRef<QSSGShaderCache> theCache = m_contextInterface->shaderCache();
        QSSGRef<QSSGRenderShaderProgram> depthShaderProgram = theCache->getProgram(name, ShaderFeatureSetList());
        if (!depthShaderProgram) {
            getProgramGenerator()->beginProgram(
                    QSSGShaderGeneratorStageFlags(QSSGShaderGeneratorStage::Vertex | QSSGShaderGeneratorStage::TessControl
                                               | QSSGShaderGeneratorStage::TessEval | QSSGShaderGeneratorStage::Fragment));
            QSSGShaderStageGeneratorInterface &vertexShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Vertex));
            QSSGShaderStageGeneratorInterface &tessCtrlShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::TessControl));
            QSSGShaderStageGeneratorInterface &tessEvalShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::TessEval));
            QSSGShaderStageGeneratorInterface &fragmentShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Fragment));

            vertexShader.addIncoming("attr_pos", "vec3");
            vertexShader.addIncoming("attr_norm", "vec3");
            vertexShader.addOutgoing("outNormal", "vec3");
            vertexShader.addUniform("modelViewProjection", "mat4");
            fragmentShader.addUniform("modelViewProjection", "mat4");
            fragmentShader.addUniform("cameraProperties", "vec2");
            fragmentShader.addUniform("cameraPosition", "vec3");
            fragmentShader.addUniform("cameraDirection", "vec3");
            fragmentShader.addInclude("depthpass.glsllib");

            vertexShader.append("void main() {");
            vertexShader.append("    gl_Position = vec4(attr_pos, 1.0);");
            vertexShader.append("    outNormal = attr_norm;");
            vertexShader.append("}");
            fragmentShader.append("void main() {");
            // fragmentShader.Append("    fragOutput = vec4(0.0, 0.0, 0.0, 0.0);");
            fragmentShader.append("    float depth = (outDepth.x - cameraProperties.x) / "
                                  "(cameraProperties.y - cameraProperties.x);");
            fragmentShader.append("    fragOutput = vec4(depth);");
            fragmentShader.append("}");

            tessCtrlShader.addInclude("tessellationNPatch.glsllib");
            tessCtrlShader.addUniform("tessLevelInner", "float");
            tessCtrlShader.addUniform("tessLevelOuter", "float");
            tessCtrlShader.addOutgoing("outNormalTC", "vec3");
            tessCtrlShader.append("void main() {\n");
            tessCtrlShader.append("    ctNorm[0] = outNormal[0];");
            tessCtrlShader.append("    ctNorm[1] = outNormal[1];");
            tessCtrlShader.append("    ctNorm[2] = outNormal[2];");
            tessCtrlShader.append("    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;");
            tessCtrlShader.append("    tessShader( tessLevelOuter, tessLevelInner);\n");
            tessCtrlShader.append("}");

            tessEvalShader.addInclude("tessellationNPatch.glsllib");
            tessEvalShader.addUniform("modelViewProjection", "mat4");
            tessEvalShader.addUniform("modelMatrix", "mat4");
            tessEvalShader.addOutgoing("outDepth", "vec3");
            tessEvalShader.append("void main() {");
            tessEvalShader.append("    vec4 pos = tessShader( );\n");
            tessEvalShader.append("    gl_Position = modelViewProjection * pos;");
            tessEvalShader.append("    outDepth.x = gl_Position.z / gl_Position.w;");
            tessEvalShader.append("}");

            depthShaderProgram = getProgramGenerator()->compileGeneratedShader(name, QSSGShaderCacheProgramFlags(), ShaderFeatureSetList());
        }

        if (depthShaderProgram) {
            theDepthShader = QSSGRef<QSSGRenderableDepthPrepassShader>(
                    new QSSGRenderableDepthPrepassShader(depthShaderProgram, context()));
        } else {
            theDepthShader = QSSGRef<QSSGRenderableDepthPrepassShader>();
        }
    }

    return theDepthShader;
}

// ---------------------------------

const QSSGRef<QSSGRenderableDepthPrepassShader> &QSSGRendererImpl::getDepthPrepassShader(bool inDisplaced)
{
    QSSGRef<QSSGRenderableDepthPrepassShader> &theDepthPrePassShader = (!inDisplaced)
            ? m_depthPrepassShader
            : m_depthPrepassShaderDisplaced;

    if (theDepthPrePassShader.isNull()) {
        // check if we do displacement mapping
        QByteArray name = "depth prepass shader";
        if (inDisplaced)
            name.append(" displacement");

        QSSGRef<QSSGShaderCache> theCache = m_contextInterface->shaderCache();
        QSSGRef<QSSGRenderShaderProgram> depthShaderProgram = theCache->getProgram(name, ShaderFeatureSetList());
        if (!depthShaderProgram) {
            getProgramGenerator()->beginProgram();
            QSSGShaderStageGeneratorInterface &vertexShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Vertex));
            QSSGShaderStageGeneratorInterface &fragmentShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Fragment));
            vertexShader.addIncoming("attr_pos", "vec3");
            vertexShader.addUniform("modelViewProjection", "mat4");

            vertexShader.append("void main() {");

            if (inDisplaced) {
                contextInterface()->defaultMaterialShaderGenerator()->addDisplacementMappingForDepthPass(vertexShader);
            } else {
                vertexShader.append("    gl_Position = modelViewProjection * vec4(attr_pos, 1.0);");
            }
            vertexShader.append("}");
            fragmentShader.append("void main() {");
            fragmentShader.append("    fragOutput = vec4(0.0, 0.0, 0.0, 0.0);");
            fragmentShader.append("}");

            depthShaderProgram = getProgramGenerator()->compileGeneratedShader(name, QSSGShaderCacheProgramFlags(), ShaderFeatureSetList());
        } else if (theCache->isShaderCachePersistenceEnabled()) {
            // we load from shader cache set default shader stages
            getProgramGenerator()->beginProgram();
            depthShaderProgram = getProgramGenerator()->compileGeneratedShader(name, QSSGShaderCacheProgramFlags(), ShaderFeatureSetList());
        }

        if (depthShaderProgram) {
            theDepthPrePassShader = QSSGRef<QSSGRenderableDepthPrepassShader>(
                    new QSSGRenderableDepthPrepassShader(depthShaderProgram, context()));
        } else {
            theDepthPrePassShader = QSSGRef<QSSGRenderableDepthPrepassShader>();
        }
    }
    return theDepthPrePassShader;
}

const QSSGRef<QSSGRenderableDepthPrepassShader> &QSSGRendererImpl::getDepthTessPrepassShader(TessellationModeValues inTessMode, bool inDisplaced)
{
    if (m_contextInterface->renderContext()->supportsTessellation()
            && inTessMode != TessellationModeValues::NoTessellation) {
        if (inTessMode == TessellationModeValues::Linear)
            return getDepthTessLinearPrepassShader(inDisplaced);
        if (inTessMode == TessellationModeValues::Phong)
            return getDepthTessPhongPrepassShader();
        if (inTessMode == TessellationModeValues::NPatch)
            return getDepthTessNPatchPrepassShader();
    }

    return getDepthPrepassShader(inDisplaced);
}

const QSSGRef<QSSGRenderableDepthPrepassShader> &QSSGRendererImpl::getDepthTessLinearPrepassShader(bool inDisplaced)
{
    QSSGRef<QSSGRenderableDepthPrepassShader> &theDepthPrePassShader = (!inDisplaced)
            ? m_depthTessLinearPrepassShader
            : m_depthTessLinearPrepassShaderDisplaced;

    if (theDepthPrePassShader.isNull()) {
        // check if we do displacement mapping
        QByteArray name = "depth tess linear prepass shader";
        if (inDisplaced)
            name.append(" displacement");

        QSSGRef<QSSGShaderCache> theCache = m_contextInterface->shaderCache();
        QSSGRef<QSSGRenderShaderProgram> depthShaderProgram = theCache->getProgram(name, ShaderFeatureSetList());
        if (!depthShaderProgram) {
            getProgramGenerator()->beginProgram(
                    QSSGShaderGeneratorStageFlags(QSSGShaderGeneratorStage::Vertex | QSSGShaderGeneratorStage::TessControl
                                               | QSSGShaderGeneratorStage::TessEval | QSSGShaderGeneratorStage::Fragment));
            QSSGShaderStageGeneratorInterface &vertexShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Vertex));
            QSSGShaderStageGeneratorInterface &tessCtrlShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::TessControl));
            QSSGShaderStageGeneratorInterface &tessEvalShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::TessEval));
            QSSGShaderStageGeneratorInterface &fragmentShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Fragment));
            vertexShader.addIncoming("attr_pos", "vec3");
            if (inDisplaced) {
                vertexShader.addIncoming("attr_uv0", "vec2");
                vertexShader.addIncoming("attr_norm", "vec3");

                vertexShader.addUniform("displacementMap_rot", "vec4");
                vertexShader.addUniform("displacementMap_offset", "vec3");

                vertexShader.addOutgoing("outNormal", "vec3");
                vertexShader.addOutgoing("outUV", "vec2");
            }
            vertexShader.addOutgoing("outWorldPos", "vec3");
            vertexShader.addUniform("modelViewProjection", "mat4");
            vertexShader.addUniform("modelMatrix", "mat4");
            vertexShader.append("void main() {");
            vertexShader.append("    gl_Position = vec4(attr_pos, 1.0);");
            if (inDisplaced) {
                vertexShader.append("    outNormal = attr_norm;");
                vertexShader.append("    vec3 uTransform = vec3( displacementMap_rot.x, "
                                    "displacementMap_rot.y, displacementMap_offset.x );");
                vertexShader.append("    vec3 vTransform = vec3( displacementMap_rot.z, "
                                    "displacementMap_rot.w, displacementMap_offset.y );");
                vertexShader.addInclude("defaultMaterialLighting.glsllib"); // getTransformedUVCoords is in the
                // lighting code addition.
                vertexShader << "    vec2 uv_coords = attr_uv0;"
                             << "\n";
                vertexShader << "    outUV = getTransformedUVCoords( vec3( uv_coords, 1.0), "
                                "uTransform, vTransform );\n";
            }
            vertexShader.append("    outWorldPos = (modelMatrix * vec4(attr_pos, 1.0)).xyz;");
            vertexShader.append("}");
            fragmentShader.append("void main() {");
            fragmentShader.append("    fragOutput = vec4(0.0, 0.0, 0.0, 0.0);");
            fragmentShader.append("}");

            tessCtrlShader.addInclude("tessellationLinear.glsllib");
            tessCtrlShader.addUniform("tessLevelInner", "float");
            tessCtrlShader.addUniform("tessLevelOuter", "float");
            tessCtrlShader.addOutgoing("outUVTC", "vec2");
            tessCtrlShader.addOutgoing("outNormalTC", "vec3");
            tessCtrlShader.append("void main() {\n");
            tessCtrlShader.append("    ctWorldPos[0] = outWorldPos[0];");
            tessCtrlShader.append("    ctWorldPos[1] = outWorldPos[1];");
            tessCtrlShader.append("    ctWorldPos[2] = outWorldPos[2];");
            tessCtrlShader.append("    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;");
            tessCtrlShader.append("    tessShader( tessLevelOuter, tessLevelInner);\n");

            if (inDisplaced) {
                tessCtrlShader.append("    outUVTC[gl_InvocationID] = outUV[gl_InvocationID];");
                tessCtrlShader.append("    outNormalTC[gl_InvocationID] = outNormal[gl_InvocationID];");
            }

            tessCtrlShader.append("}");

            tessEvalShader.addInclude("tessellationLinear.glsllib");
            tessEvalShader.addUniform("modelViewProjection", "mat4");
            if (inDisplaced) {
                tessEvalShader.addUniform("displacementSampler", "sampler2D");
                tessEvalShader.addUniform("displaceAmount", "float");
                tessEvalShader.addInclude("defaultMaterialFileDisplacementTexture.glsllib");
            }
            tessEvalShader.addOutgoing("outUV", "vec2");
            tessEvalShader.addOutgoing("outNormal", "vec3");
            tessEvalShader.append("void main() {");
            tessEvalShader.append("    vec4 pos = tessShader( );\n");

            if (inDisplaced) {
                tessEvalShader << "    outUV = gl_TessCoord.x * outUVTC[0] + gl_TessCoord.y * "
                                  "outUVTC[1] + gl_TessCoord.z * outUVTC[2];"
                               << "\n";
                tessEvalShader << "    outNormal = gl_TessCoord.x * outNormalTC[0] + gl_TessCoord.y * "
                                  "outNormalTC[1] + gl_TessCoord.z * outNormalTC[2];"
                               << "\n";
                tessEvalShader << "    vec3 displacedPos = defaultMaterialFileDisplacementTexture( "
                                  "displacementSampler , displaceAmount, outUV , outNormal, pos.xyz );"
                               << "\n";
                tessEvalShader.append("    gl_Position = modelViewProjection * vec4(displacedPos, 1.0);");
            } else
                tessEvalShader.append("    gl_Position = modelViewProjection * pos;");

            tessEvalShader.append("}");
            QSSGShaderCacheProgramFlags theFlags(ShaderCacheProgramFlagValues::TessellationEnabled);

            depthShaderProgram = getProgramGenerator()->compileGeneratedShader(name, theFlags, ShaderFeatureSetList());
        } else if (theCache->isShaderCachePersistenceEnabled()) {
            // we load from shader cache set default shader stages
            getProgramGenerator()->beginProgram(
                    QSSGShaderGeneratorStageFlags(QSSGShaderGeneratorStage::Vertex | QSSGShaderGeneratorStage::TessControl
                                               | QSSGShaderGeneratorStage::TessEval | QSSGShaderGeneratorStage::Fragment));
            QSSGShaderCacheProgramFlags theFlags(ShaderCacheProgramFlagValues::TessellationEnabled);

            depthShaderProgram = getProgramGenerator()->compileGeneratedShader(name, theFlags, ShaderFeatureSetList());
        }

        if (depthShaderProgram) {
            theDepthPrePassShader = QSSGRef<QSSGRenderableDepthPrepassShader>(
                    new QSSGRenderableDepthPrepassShader(depthShaderProgram, context()));
        } else {
            theDepthPrePassShader = QSSGRef<QSSGRenderableDepthPrepassShader>();
        }
    }
    return theDepthPrePassShader;
}

const QSSGRef<QSSGRenderableDepthPrepassShader> &QSSGRendererImpl::getDepthTessPhongPrepassShader()
{
    if (m_depthTessPhongPrepassShader.isNull()) {
        QSSGRef<QSSGShaderCache> theCache = m_contextInterface->shaderCache();
        QByteArray name = "depth tess phong prepass shader";
        QSSGRef<QSSGRenderShaderProgram> depthShaderProgram = theCache->getProgram(name, ShaderFeatureSetList());
        if (!depthShaderProgram) {
            getProgramGenerator()->beginProgram(
                    QSSGShaderGeneratorStageFlags(QSSGShaderGeneratorStage::Vertex | QSSGShaderGeneratorStage::TessControl
                                               | QSSGShaderGeneratorStage::TessEval | QSSGShaderGeneratorStage::Fragment));
            QSSGShaderStageGeneratorInterface &vertexShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Vertex));
            QSSGShaderStageGeneratorInterface &tessCtrlShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::TessControl));
            QSSGShaderStageGeneratorInterface &tessEvalShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::TessEval));
            QSSGShaderStageGeneratorInterface &fragmentShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Fragment));
            vertexShader.addIncoming("attr_pos", "vec3");
            vertexShader.addIncoming("attr_norm", "vec3");
            vertexShader.addOutgoing("outNormal", "vec3");
            vertexShader.addOutgoing("outWorldPos", "vec3");
            vertexShader.addUniform("modelViewProjection", "mat4");
            vertexShader.addUniform("modelMatrix", "mat4");
            vertexShader.append("void main() {");
            vertexShader.append("    gl_Position = vec4(attr_pos, 1.0);");
            vertexShader.append("    outWorldPos = (modelMatrix * vec4(attr_pos, 1.0)).xyz;");
            vertexShader.append("    outNormal = attr_norm;");
            vertexShader.append("}");
            fragmentShader.append("void main() {");
            fragmentShader.append("    fragOutput = vec4(0.0, 0.0, 0.0, 0.0);");
            fragmentShader.append("}");

            tessCtrlShader.addInclude("tessellationPhong.glsllib");
            tessCtrlShader.addUniform("tessLevelInner", "float");
            tessCtrlShader.addUniform("tessLevelOuter", "float");
            tessCtrlShader.append("void main() {\n");
            tessCtrlShader.append("    ctWorldPos[0] = outWorldPos[0];");
            tessCtrlShader.append("    ctWorldPos[1] = outWorldPos[1];");
            tessCtrlShader.append("    ctWorldPos[2] = outWorldPos[2];");
            tessCtrlShader.append("    ctNorm[0] = outNormal[0];");
            tessCtrlShader.append("    ctNorm[1] = outNormal[1];");
            tessCtrlShader.append("    ctNorm[2] = outNormal[2];");
            tessCtrlShader.append("    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;");
            tessCtrlShader.append("    tessShader( tessLevelOuter, tessLevelInner);\n");
            tessCtrlShader.append("}");

            tessEvalShader.addInclude("tessellationPhong.glsllib");
            tessEvalShader.addUniform("modelViewProjection", "mat4");
            tessEvalShader.append("void main() {");
            tessEvalShader.append("    vec4 pos = tessShader( );\n");
            tessEvalShader.append("    gl_Position = modelViewProjection * pos;\n");
            tessEvalShader.append("}");
            QSSGShaderCacheProgramFlags theFlags(ShaderCacheProgramFlagValues::TessellationEnabled);

            depthShaderProgram = getProgramGenerator()->compileGeneratedShader(name, theFlags, ShaderFeatureSetList());
        } else if (theCache->isShaderCachePersistenceEnabled()) {
            // we load from shader cache set default shader stages
            getProgramGenerator()->beginProgram(
                    QSSGShaderGeneratorStageFlags(QSSGShaderGeneratorStage::Vertex | QSSGShaderGeneratorStage::TessControl
                                               | QSSGShaderGeneratorStage::TessEval | QSSGShaderGeneratorStage::Fragment));
            QSSGShaderCacheProgramFlags theFlags(ShaderCacheProgramFlagValues::TessellationEnabled);

            depthShaderProgram = getProgramGenerator()->compileGeneratedShader(name, theFlags, ShaderFeatureSetList());
        }

        if (depthShaderProgram) {
            m_depthTessPhongPrepassShader = QSSGRef<QSSGRenderableDepthPrepassShader>(
                    new QSSGRenderableDepthPrepassShader(depthShaderProgram, context()));
        } else {
            m_depthTessPhongPrepassShader = QSSGRef<QSSGRenderableDepthPrepassShader>();
        }
    }
    return m_depthTessPhongPrepassShader;
}

const QSSGRef<QSSGRenderableDepthPrepassShader> &QSSGRendererImpl::getDepthTessNPatchPrepassShader()
{
    if (m_depthTessNPatchPrepassShader.isNull()) {
        QSSGRef<QSSGShaderCache> theCache = m_contextInterface->shaderCache();
        QByteArray name = "depth tess npatch prepass shader";
        QSSGRef<QSSGRenderShaderProgram> depthShaderProgram = theCache->getProgram(name, ShaderFeatureSetList());
        if (!depthShaderProgram) {
            getProgramGenerator()->beginProgram(
                    QSSGShaderGeneratorStageFlags(QSSGShaderGeneratorStage::Vertex | QSSGShaderGeneratorStage::TessControl
                                               | QSSGShaderGeneratorStage::TessEval | QSSGShaderGeneratorStage::Fragment));
            QSSGShaderStageGeneratorInterface &vertexShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Vertex));
            QSSGShaderStageGeneratorInterface &tessCtrlShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::TessControl));
            QSSGShaderStageGeneratorInterface &tessEvalShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::TessEval));
            QSSGShaderStageGeneratorInterface &fragmentShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Fragment));
            vertexShader.addIncoming("attr_pos", "vec3");
            vertexShader.addIncoming("attr_norm", "vec3");
            vertexShader.addOutgoing("outNormal", "vec3");
            vertexShader.addOutgoing("outWorldPos", "vec3");
            vertexShader.addUniform("modelViewProjection", "mat4");
            vertexShader.addUniform("modelMatrix", "mat4");
            vertexShader.append("void main() {");
            vertexShader.append("    gl_Position = vec4(attr_pos, 1.0);");
            vertexShader.append("    outWorldPos = (modelMatrix * vec4(attr_pos, 1.0)).xyz;");
            vertexShader.append("    outNormal = attr_norm;");
            vertexShader.append("}");
            fragmentShader.append("void main() {");
            fragmentShader.append("    fragOutput = vec4(0.0, 0.0, 0.0, 0.0);");
            fragmentShader.append("}");

            tessCtrlShader.addOutgoing("outNormalTC", "vec3");
            tessCtrlShader.addInclude("tessellationNPatch.glsllib");
            tessCtrlShader.addUniform("tessLevelInner", "float");
            tessCtrlShader.addUniform("tessLevelOuter", "float");
            tessCtrlShader.append("void main() {\n");
            tessCtrlShader.append("    ctWorldPos[0] = outWorldPos[0];");
            tessCtrlShader.append("    ctWorldPos[1] = outWorldPos[1];");
            tessCtrlShader.append("    ctWorldPos[2] = outWorldPos[2];");
            tessCtrlShader.append("    ctNorm[0] = outNormal[0];");
            tessCtrlShader.append("    ctNorm[1] = outNormal[1];");
            tessCtrlShader.append("    ctNorm[2] = outNormal[2];");
            tessCtrlShader.append("    ctTangent[0] = outNormal[0];"); // we don't care for the tangent
            tessCtrlShader.append("    ctTangent[1] = outNormal[1];");
            tessCtrlShader.append("    ctTangent[2] = outNormal[2];");
            tessCtrlShader.append("    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;");
            tessCtrlShader.append("    tessShader( tessLevelOuter, tessLevelInner);\n");
            tessCtrlShader.append("    outNormalTC[gl_InvocationID] = outNormal[gl_InvocationID];\n");
            tessCtrlShader.append("}");

            tessEvalShader.addInclude("tessellationNPatch.glsllib");
            tessEvalShader.addUniform("modelViewProjection", "mat4");
            tessEvalShader.append("void main() {");
            tessEvalShader.append("    ctNorm[0] = outNormalTC[0];");
            tessEvalShader.append("    ctNorm[1] = outNormalTC[1];");
            tessEvalShader.append("    ctNorm[2] = outNormalTC[2];");
            tessEvalShader.append("    ctTangent[0] = outNormalTC[0];"); // we don't care for the tangent
            tessEvalShader.append("    ctTangent[1] = outNormalTC[1];");
            tessEvalShader.append("    ctTangent[2] = outNormalTC[2];");
            tessEvalShader.append("    vec4 pos = tessShader( );\n");
            tessEvalShader.append("    gl_Position = modelViewProjection * pos;\n");
            tessEvalShader.append("}");
            QSSGShaderCacheProgramFlags theFlags(ShaderCacheProgramFlagValues::TessellationEnabled);

            depthShaderProgram = getProgramGenerator()->compileGeneratedShader(name, theFlags, ShaderFeatureSetList());
        } else if (theCache->isShaderCachePersistenceEnabled()) {
            // we load from shader cache set default shader stages
            getProgramGenerator()->beginProgram(
                    QSSGShaderGeneratorStageFlags(QSSGShaderGeneratorStage::Vertex | QSSGShaderGeneratorStage::TessControl
                                               | QSSGShaderGeneratorStage::TessEval | QSSGShaderGeneratorStage::Fragment));
            QSSGShaderCacheProgramFlags theFlags(ShaderCacheProgramFlagValues::TessellationEnabled);

            depthShaderProgram = getProgramGenerator()->compileGeneratedShader(name, theFlags, ShaderFeatureSetList());
        }

        if (depthShaderProgram) {
            m_depthTessNPatchPrepassShader = QSSGRef<QSSGRenderableDepthPrepassShader>(
                    new QSSGRenderableDepthPrepassShader(depthShaderProgram, context()));
        } else {
            m_depthTessNPatchPrepassShader = QSSGRef<QSSGRenderableDepthPrepassShader>();
        }
    }
    return m_depthTessNPatchPrepassShader;
}

QSSGRef<QSSGSkyBoxShader> QSSGRendererImpl::getSkyBoxShader()
{
    if (!m_skyBoxShader) {
        QSSGRef<QSSGShaderCache> theCache = m_contextInterface->shaderCache();
        QByteArray name = "fullscreen skybox shader";
        QSSGRef<QSSGRenderShaderProgram> skyBoxShaderProgram = theCache->getProgram(name, ShaderFeatureSetList());
        if (!skyBoxShaderProgram) {
            QSSGRef<QSSGShaderProgramGeneratorInterface> theGenerator(getProgramGenerator());
            theGenerator->beginProgram();
            QSSGShaderStageGeneratorInterface &vertexGenerator(*theGenerator->getStage(QSSGShaderGeneratorStage::Vertex));
            QSSGShaderStageGeneratorInterface &fragmentGenerator(*theGenerator->getStage(QSSGShaderGeneratorStage::Fragment));

            vertexGenerator.addIncoming("attr_pos", "vec3");

            vertexGenerator.addOutgoing("eye_direction", "vec3");

            vertexGenerator.addUniform("viewMatrix", "mat4");
            vertexGenerator.addUniform("projection", "mat4");

            vertexGenerator.append("void main() {");
            vertexGenerator.append("    gl_Position = vec4(attr_pos, 1.0);");
            vertexGenerator.append("    mat4 inverseProjection = inverse(projection);");
            vertexGenerator.append("    vec3 unprojected = (inverseProjection * gl_Position).xyz;");
            vertexGenerator.append("    eye_direction = normalize(mat3(viewMatrix) * unprojected);");
            vertexGenerator.append("}");

            fragmentGenerator.addInclude("customMaterial.glsllib"); // Needed for PI, PI_TWO

            fragmentGenerator.addUniform("skybox_image", "sampler2D");
            fragmentGenerator.addUniform("output_color", "vec3");

            fragmentGenerator.append("void main() {");

            // Ideally, we would just reuse getProbeSampleUV like this, but that leads to issues
            // with incorrect texture gradients because we're drawing on a quad and not a sphere.
            // See explanation below.
            // fragmentGenerator.addInclude("sampleProbe.glsllib");
            // fragmentGenerator.append("    gl_FragColor = texture2D(skybox_image, getProbeSampleUV(eye, vec4(1.0, 0.0, 0.0, 1.0), vec2(0,0)));");

            // nlerp direction vector, not entirely correct, but simple/efficient
            fragmentGenerator.append("    vec3 eye = normalize(eye_direction);");

            // Equirectangular textures project longitude and latitude to the xy plane
            fragmentGenerator.append("    float longitude = atan(eye.x, eye.z) / PI_TWO + 0.5;");
            fragmentGenerator.append("    float latitude = asin(eye.y) / PI + 0.5;");
            fragmentGenerator.append("    vec2 uv = vec2(longitude, latitude);");

            // Because of the non-standard projection, the texture lookup for normal texture
            // filtering is messed up.
            // TODO: Alternatively, we could check if it's possible to disable some of the texture
            // filtering just for the skybox part.
            fragmentGenerator.append("    vec4 color = textureLod(skybox_image, uv, 0.0);");
            fragmentGenerator.append("    vec3 rgbeColor = color.rgb * pow(2.0, color.a * 255.0 - 128.0);");
            fragmentGenerator.append("    vec3 tonemappedColor = rgbeColor.rgb / (rgbeColor.rgb + vec3(1.0));");
            fragmentGenerator.append("    vec3 gammaCorrectedColor = pow( tonemappedColor, vec3( 1.0 / 2.2 ));");
            fragmentGenerator.append("    gl_FragColor = vec4(gammaCorrectedColor, 1.0);");
            fragmentGenerator.append("}");

            // No flags enabled
            skyBoxShaderProgram = theGenerator->compileGeneratedShader(name, QSSGShaderCacheProgramFlags(), ShaderFeatureSetList());
        }

        if (skyBoxShaderProgram) {
            m_skyBoxShader = QSSGRef<QSSGSkyBoxShader>(
                        new QSSGSkyBoxShader(skyBoxShaderProgram, context()));
        } else {
            m_skyBoxShader = QSSGRef<QSSGSkyBoxShader>();
        }
    }
    return m_skyBoxShader;
}

QSSGRef<QSSGDefaultAoPassShader> QSSGRendererImpl::getDefaultAoPassShader(const ShaderFeatureSetList &inFeatureSet)
{
    if (m_defaultAoPassShader.isNull()) {
        QSSGRef<QSSGShaderCache> theCache = m_contextInterface->shaderCache();
        QByteArray name = "fullscreen AO pass shader";
        QSSGRef<QSSGRenderShaderProgram> aoPassShaderProgram = theCache->getProgram(name, ShaderFeatureSetList());
        if (!aoPassShaderProgram) {
            getProgramGenerator()->beginProgram();
            QSSGShaderStageGeneratorInterface &theVertexGenerator(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Vertex));
            QSSGShaderStageGeneratorInterface &theFragmentGenerator(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Fragment));
            theVertexGenerator.addIncoming("attr_pos", "vec3");
            theVertexGenerator.addIncoming("attr_uv", "vec2");
            theVertexGenerator.addOutgoing("uv_coords", "vec2");
            theVertexGenerator.append("void main() {");
            theVertexGenerator.append("    gl_Position = vec4(attr_pos.xy, 0.5, 1.0 );");
            theVertexGenerator.append("    uv_coords = attr_uv;");
            theVertexGenerator.append("}");

            // fragmentGenerator.AddInclude( "SSAOCustomMaterial.glsllib" );
            theFragmentGenerator.addInclude("viewProperties.glsllib");
            theFragmentGenerator.addInclude("screenSpaceAO.glsllib");
            if (m_context->renderContextType() == QSSGRenderContextType::GLES2) {
                theFragmentGenerator << "    uniform vec4 aoProperties;"
                                     << "\n"
                                     << "    uniform vec4 aoProperties2;"
                                     << "\n"
                                     << "    uniform vec4 shadowProperties;"
                                     << "\n"
                                     << "    uniform vec4 aoScreenConst;"
                                     << "\n"
                                     << "    uniform vec4 uvToEyeConst;"
                                     << "\n";
            } else {
                theFragmentGenerator << "layout (std140) uniform aoShadow { "
                                     << "\n"
                                     << "    vec4 aoProperties;"
                                     << "\n"
                                     << "    vec4 aoProperties2;"
                                     << "\n"
                                     << "    vec4 shadowProperties;"
                                     << "\n"
                                     << "    vec4 aoScreenConst;"
                                     << "\n"
                                     << "    vec4 uvToEyeConst;"
                                     << "\n"
                                     << "};"
                                     << "\n";
            }
            theFragmentGenerator.addUniform("cameraDirection", "vec3");
            theFragmentGenerator.addUniform("depthTexture", "sampler2D");
            theFragmentGenerator.append("void main() {");
            theFragmentGenerator << "    float aoFactor;"
                                 << "\n";
            theFragmentGenerator << "    vec3 screenNorm;"
                                 << "\n";

            // We're taking multiple depth samples and getting the derivatives at each of them
            // to get a more
            // accurate view space normal vector.  When we do only one, we tend to get bizarre
            // values at the edges
            // surrounding objects, and this also ends up giving us weird AO values.
            // If we had a proper screen-space normal map, that would also do the trick.
            if (m_context->renderContextType() == QSSGRenderContextType::GLES2) {
                theFragmentGenerator.addUniform("depthTextureSize", "vec2");
                theFragmentGenerator.append("    ivec2 iCoords = ivec2( gl_FragCoord.xy );");
                theFragmentGenerator.append("    float depth = getDepthValue( "
                                            "texture2D(depthTexture, vec2(iCoords)"
                                            " / depthTextureSize), cameraProperties );");
                theFragmentGenerator.append("    depth = depthValueToLinearDistance( depth, cameraProperties );");
                theFragmentGenerator.append("    depth = (depth - cameraProperties.x) / "
                                            "(cameraProperties.y - cameraProperties.x);");
                theFragmentGenerator.append("    float depth2 = getDepthValue( "
                                            "texture2D(depthTexture, vec2(iCoords+ivec2(1))"
                                            " / depthTextureSize), cameraProperties );");
                theFragmentGenerator.append("    depth2 = depthValueToLinearDistance( depth, cameraProperties );");
                theFragmentGenerator.append("    float depth3 = getDepthValue( "
                                            "texture2D(depthTexture, vec2(iCoords-ivec2(1))"
                                            " / depthTextureSize), cameraProperties );");
            } else {
                theFragmentGenerator.append("    ivec2 iCoords = ivec2( gl_FragCoord.xy );");
                theFragmentGenerator.append("    float depth = getDepthValue( "
                                            "texelFetch(depthTexture, iCoords, 0), "
                                            "cameraProperties );");
                theFragmentGenerator.append("    depth = depthValueToLinearDistance( depth, cameraProperties );");
                theFragmentGenerator.append("    depth = (depth - cameraProperties.x) / "
                                            "(cameraProperties.y - cameraProperties.x);");
                theFragmentGenerator.append("    float depth2 = getDepthValue( "
                                            "texelFetch(depthTexture, iCoords+ivec2(1), 0), "
                                            "cameraProperties );");
                theFragmentGenerator.append("    depth2 = depthValueToLinearDistance( depth, cameraProperties );");
                theFragmentGenerator.append("    float depth3 = getDepthValue( "
                                            "texelFetch(depthTexture, iCoords-ivec2(1), 0), "
                                            "cameraProperties );");
            }
            theFragmentGenerator.append("    depth3 = depthValueToLinearDistance( depth, cameraProperties );");
            theFragmentGenerator.append("    vec3 tanU = vec3(10, 0, dFdx(depth));");
            theFragmentGenerator.append("    vec3 tanV = vec3(0, 10, dFdy(depth));");
            theFragmentGenerator.append("    screenNorm = normalize(cross(tanU, tanV));");
            theFragmentGenerator.append("    tanU = vec3(10, 0, dFdx(depth2));");
            theFragmentGenerator.append("    tanV = vec3(0, 10, dFdy(depth2));");
            theFragmentGenerator.append("    screenNorm += normalize(cross(tanU, tanV));");
            theFragmentGenerator.append("    tanU = vec3(10, 0, dFdx(depth3));");
            theFragmentGenerator.append("    tanV = vec3(0, 10, dFdy(depth3));");
            theFragmentGenerator.append("    screenNorm += normalize(cross(tanU, tanV));");
            theFragmentGenerator.append("    screenNorm = -normalize(screenNorm);");

            theFragmentGenerator.append("    aoFactor = \
                                        SSambientOcclusion( depthTexture, screenNorm, aoProperties, aoProperties2, \
                                                            cameraProperties, aoScreenConst, uvToEyeConst );");

            theFragmentGenerator.append("    gl_FragColor = vec4(aoFactor, aoFactor, aoFactor, 1.0);");

            theFragmentGenerator.append("}");
            aoPassShaderProgram = getProgramGenerator()->compileGeneratedShader(name, QSSGShaderCacheProgramFlags(), inFeatureSet);
        }

        if (aoPassShaderProgram) {
            m_defaultAoPassShader = QSSGRef<QSSGDefaultAoPassShader>(
                    new QSSGDefaultAoPassShader(aoPassShaderProgram, context()));
        } else {
            m_defaultAoPassShader = QSSGRef<QSSGDefaultAoPassShader>();
        }
    }
    return m_defaultAoPassShader;
}

#ifdef QT_QUICK3D_DEBUG_SHADOWS
QSSGRef<QSSGDefaultAoPassShader> QSSGRendererImpl::getDebugDepthShader(ShaderFeatureSetList inFeatureSet)
{
    if (m_debugDepthShader.isNull()) {
        QSSGRef<QSSGShaderCache> theCache = m_contextInterface->shaderCache();
        QByteArray name = "depth display shader";
        QSSGRef<QSSGRenderShaderProgram> depthShaderProgram = theCache->getProgram(name, ShaderFeatureSetList());
        if (!depthShaderProgram) {
            getProgramGenerator()->beginProgram();
            QSSGShaderStageGeneratorInterface &theVertexGenerator(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Vertex));
            QSSGShaderStageGeneratorInterface &theFragmentGenerator(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Fragment));
            theVertexGenerator.addIncoming("attr_pos", "vec3");
            theVertexGenerator.addIncoming("attr_uv", "vec2");
            theVertexGenerator.addOutgoing("uv_coords", "vec2");
            theVertexGenerator.append("void main() {");
            theVertexGenerator.append("\tgl_Position = vec4(attr_pos.xy, 0.5, 1.0);");
            theVertexGenerator.append("\tuv_coords = attr_uv;");
            theVertexGenerator.append("}");

            theFragmentGenerator.addUniform("depthTexture", "sampler2D");
            theFragmentGenerator.append("void main() {");
            theFragmentGenerator.append("\tivec2 iCoords = ivec2(gl_FragCoord.xy);");
            theFragmentGenerator.append("\tfloat depSample = texelFetch(depthTexture, iCoords, 0).x;");
            theFragmentGenerator.append("\tif (depSample <= 0) discard;");
            theFragmentGenerator.append("\tgl_FragColor = vec4(depSample, depSample, depSample, 1.0);");
            theFragmentGenerator.append("\treturn;");
            theFragmentGenerator.append("}");
        }

        depthShaderProgram = getProgramGenerator()->compileGeneratedShader(name, QSSGShaderCacheProgramFlags(), inFeatureSet);

        if (depthShaderProgram) {
            m_debugDepthShader = QSSGRef<QSSGDefaultAoPassShader>(new QSSGDefaultAoPassShader(depthShaderProgram, context()));
        } else {
            m_debugDepthShader = QSSGRef<QSSGDefaultAoPassShader>();
        }
    }
    return m_debugDepthShader;
}

QSSGRef<QSSGDefaultAoPassShader> QSSGRendererImpl::getDebugCubeDepthShader(ShaderFeatureSetList inFeatureSet)
{
    if (!m_debugCubemapDepthShader) {
        QSSGRef<QSSGShaderCache> theCache = m_contextInterface->shaderCache();
        QByteArray name = "cube depth display shader";
        QSSGRef<QSSGRenderShaderProgram> cubeShaderProgram = theCache->getProgram(name, ShaderFeatureSetList());
        if (!cubeShaderProgram) {
            getProgramGenerator()->beginProgram();
            QSSGShaderStageGeneratorInterface &theVertexGenerator(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Vertex));
            QSSGShaderStageGeneratorInterface &theFragmentGenerator(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Fragment));
            theVertexGenerator.addIncoming("attr_pos", "vec3");
            theVertexGenerator.addIncoming("attr_uv", "vec2");
            theVertexGenerator.addOutgoing("sample_dir", "vec3");
            theVertexGenerator.append("void main() {");
            theVertexGenerator.append("\tgl_Position = vec4(attr_pos.xy, 0.5, 1.0);");
            theVertexGenerator.append("\tsample_dir = vec3(4.0 * (attr_uv.x - 0.5), -1.0, 4.0 * (attr_uv.y - 0.5));");
            theVertexGenerator.append("}");
            theFragmentGenerator.addUniform("depthCube", "samplerCube");
            theFragmentGenerator.append("void main() {");
            theFragmentGenerator.append("\tfloat smpDepth = texture(depthCube, sample_dir).x;");
            theFragmentGenerator.append("\tif (smpDepth <= 0) discard;");
            theFragmentGenerator.append("\tgl_FragColor = vec4(smpDepth, smpDepth, smpDepth, 1.0);");
            theFragmentGenerator.append("}");
        }

        cubeShaderProgram = getProgramGenerator()->compileGeneratedShader(name, QSSGShaderCacheProgramFlags(), inFeatureSet);

        if (cubeShaderProgram) {
            m_debugCubemapDepthShader = QSSGRef<QSSGDefaultAoPassShader>(
                    new QSSGDefaultAoPassShader(cubeShaderProgram, context()));
        } else {
            m_debugCubemapDepthShader = QSSGRef<QSSGDefaultAoPassShader>();
        }
    }
    return m_debugCubemapDepthShader;
}
#endif

QSSGRef<QSSGRenderShaderProgram> QSSGRendererImpl::getTextAtlasEntryShader()
{
    getProgramGenerator()->beginProgram();

    QSSGShaderStageGeneratorInterface &vertexGenerator(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Vertex));
    QSSGShaderStageGeneratorInterface &fragmentGenerator(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Fragment));

    vertexGenerator.addIncoming("attr_pos", "vec3");
    vertexGenerator.addIncoming("attr_uv", "vec2");
    vertexGenerator.addUniform("modelViewProjection", "mat4");
    vertexGenerator.addOutgoing("uv_coords", "vec2");
    vertexGenerator.append("void main() {");

    vertexGenerator.append("    gl_Position = modelViewProjection * vec4(attr_pos, 1.0);");
    vertexGenerator.append("    uv_coords = attr_uv;");
    vertexGenerator.append("}");

    fragmentGenerator.addUniform("text_image", "sampler2D");
    fragmentGenerator.append("void main() {");
    fragmentGenerator.append("    float alpha = texture2D( text_image, uv_coords ).a;");
    fragmentGenerator.append("    fragOutput = vec4(alpha, alpha, alpha, alpha);");
    fragmentGenerator.append("}");

    return getProgramGenerator()->compileGeneratedShader("texture atlas entry shader", QSSGShaderCacheProgramFlags(), ShaderFeatureSetList());
}

QSSGRef<QSSGFlippedQuadShader> QSSGRendererImpl::getFlippedQuadShader()
{
    if (m_flippedQuadShader)
        return m_flippedQuadShader;

    QSSGRef<QSSGShaderCache> theCache = m_contextInterface->shaderCache();
    QByteArray name = "flipped quad shader";
    QSSGRef<QSSGRenderShaderProgram> shader = theCache->getProgram(name, ShaderFeatureSetList());
    if (!shader) {
        getProgramGenerator()->beginProgram();

        QSSGShaderStageGeneratorInterface &vertexGenerator(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Vertex));
        QSSGShaderStageGeneratorInterface &fragmentGenerator(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Fragment));

        vertexGenerator.addIncoming("attr_pos", "vec3");
        vertexGenerator.addIncoming("attr_uv", "vec2");
        // xy of text dimensions are scaling factors, zw are offset factors.
        vertexGenerator.addUniform("layer_dimensions", "vec2");
        vertexGenerator.addUniform("modelViewProjection", "mat4");
        vertexGenerator.addOutgoing("uv_coords", "vec2");
        vertexGenerator.append("void main() {");
        vertexGenerator << "    vec3 layerPos = vec3(attr_pos.x * layer_dimensions.x / 2.0"
                        << ", attr_pos.y * layer_dimensions.y / 2.0"
                        << ", attr_pos.z);"
                        << "\n";

        vertexGenerator.append("    gl_Position = modelViewProjection * vec4(layerPos, 1.0);");
        vertexGenerator.append("    uv_coords = vec2(attr_uv.x, 1.0 - attr_uv.y);");
        vertexGenerator.append("}");

        fragmentGenerator.addUniform("layer_image", "sampler2D");
        fragmentGenerator.addUniform("opacity", "float");
        fragmentGenerator.append("void main() {");
        fragmentGenerator.append("    vec2 theCoords = uv_coords;\n");
        fragmentGenerator.append("    vec4 theLayerTexture = texture2D( layer_image, theCoords );\n");
        fragmentGenerator.append("    fragOutput = theLayerTexture * opacity;\n");
        fragmentGenerator.append("}");
        shader = getProgramGenerator()->compileGeneratedShader(name, QSSGShaderCacheProgramFlags(),
                                                               ShaderFeatureSetList());
    }
    QSSGRef<QSSGFlippedQuadShader> retval;
    if (shader)
        retval = QSSGRef<QSSGFlippedQuadShader>(new QSSGFlippedQuadShader(shader));
    m_flippedQuadShader = retval;
    return m_flippedQuadShader;
}

QSSGRef<QSSGLayerProgAABlendShader> QSSGRendererImpl::getLayerProgAABlendShader()
{
    if (m_layerProgAAShader)
        return m_layerProgAAShader;

    QSSGRef<QSSGShaderCache> theCache = m_contextInterface->shaderCache();
    QByteArray name = "layer progressiveAA blend shader";
    QSSGRef<QSSGRenderShaderProgram> shader = theCache->getProgram(name, ShaderFeatureSetList());
    if (!shader) {
        getProgramGenerator()->beginProgram();

        QSSGShaderStageGeneratorInterface &vertexGenerator(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Vertex));
        QSSGShaderStageGeneratorInterface &fragmentGenerator(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Fragment));
        vertexGenerator.addIncoming("attr_pos", "vec3");
        vertexGenerator.addIncoming("attr_uv", "vec2");
        vertexGenerator.addOutgoing("uv_coords", "vec2");
        vertexGenerator.append("void main() {");
        vertexGenerator.append("    gl_Position = vec4(attr_pos, 1.0 );");
        vertexGenerator.append("    uv_coords = attr_uv;");
        vertexGenerator.append("}");
        fragmentGenerator.addUniform("accumulator", "sampler2D");
        fragmentGenerator.addUniform("last_frame", "sampler2D");
        fragmentGenerator.addUniform("blend_factors", "vec2");
        fragmentGenerator.append("void main() {");
        fragmentGenerator.append("    vec4 accum = texture2D( accumulator, uv_coords );");
        fragmentGenerator.append("    vec4 lastFrame = texture2D( last_frame, uv_coords );");
        fragmentGenerator.append("    gl_FragColor = accum*blend_factors.y + lastFrame*blend_factors.x;");
        fragmentGenerator.append("}");
        shader = getProgramGenerator()->compileGeneratedShader(name, QSSGShaderCacheProgramFlags(),
                                                               ShaderFeatureSetList());
    }
    QSSGRef<QSSGLayerProgAABlendShader> retval;
    if (shader)
        retval = QSSGRef<QSSGLayerProgAABlendShader>(new QSSGLayerProgAABlendShader(shader));
    m_layerProgAAShader = retval;
    return m_layerProgAAShader;
}

QSSGRef<QSSGLayerLastFrameBlendShader> QSSGRendererImpl::getLayerLastFrameBlendShader()
{
    if (m_layerLastFrameBlendShader)
        return m_layerLastFrameBlendShader;

    QSSGRef<QSSGShaderCache> theCache = m_contextInterface->shaderCache();
    QByteArray name = "layer last frame blend shader";
    QSSGRef<QSSGRenderShaderProgram> shader = theCache->getProgram(name, ShaderFeatureSetList());
    if (!shader) {
        getProgramGenerator()->beginProgram();

        QSSGShaderStageGeneratorInterface &vertexGenerator(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Vertex));
        QSSGShaderStageGeneratorInterface &fragmentGenerator(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Fragment));
        vertexGenerator.addIncoming("attr_pos", "vec3");
        vertexGenerator.addIncoming("attr_uv", "vec2");
        vertexGenerator.addOutgoing("uv_coords", "vec2");
        vertexGenerator.append("void main() {");
        vertexGenerator.append("    gl_Position = vec4(attr_pos, 1.0);");
        vertexGenerator.append("    uv_coords = attr_uv;");
        vertexGenerator.append("}");
        fragmentGenerator.addUniform("last_frame", "sampler2D");
        fragmentGenerator.addUniform("blend_factor", "float");
        fragmentGenerator.append("void main() {");
        fragmentGenerator.append("    vec4 lastFrame = texture2D(last_frame, uv_coords);");
        fragmentGenerator.append("    gl_FragColor = vec4(lastFrame.rgb*blend_factor, blend_factor);");
        fragmentGenerator.append("}");
        QSSGRef<QSSGRenderShaderProgram>
                theShader = getProgramGenerator()->compileGeneratedShader(name,
                                                                          QSSGShaderCacheProgramFlags(),
                                                                          ShaderFeatureSetList());
    }
    QSSGRef<QSSGLayerLastFrameBlendShader> retval;
    if (shader)
        retval = QSSGRef<QSSGLayerLastFrameBlendShader>(new QSSGLayerLastFrameBlendShader(shader));
    m_layerLastFrameBlendShader = retval;
    return m_layerLastFrameBlendShader;
}

QSSGRef<QSSGCompositShader> QSSGRendererImpl::getCompositShader()
{
    if (m_compositShader)
        return m_compositShader;

    QSSGRef<QSSGShaderCache> theCache = m_contextInterface->shaderCache();
    QByteArray name = "composit shader";
    QSSGRef<QSSGRenderShaderProgram> shader = theCache->getProgram(name, ShaderFeatureSetList());
    if (!shader) {
        getProgramGenerator()->beginProgram();

        QSSGShaderStageGeneratorInterface &vertexGenerator(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Vertex));
        QSSGShaderStageGeneratorInterface &fragmentGenerator(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Fragment));
        vertexGenerator.addIncoming("attr_pos", "vec3");
        vertexGenerator.addIncoming("attr_uv", "vec2");
        vertexGenerator.addOutgoing("uv_coords", "vec2");
        vertexGenerator.append("void main() {");
        vertexGenerator.append("\tgl_Position = vec4(attr_pos, 1.0);");
        vertexGenerator.append("\tuv_coords = attr_uv;");
        vertexGenerator.append("}");
        fragmentGenerator.addUniform("last_frame", "sampler2D");
        fragmentGenerator.append("void main() {");
        fragmentGenerator.append("\tgl_FragColor = texture2D(last_frame, uv_coords);");
        fragmentGenerator.append("}");
        shader = getProgramGenerator()->compileGeneratedShader(name, QSSGShaderCacheProgramFlags(),
                                                               ShaderFeatureSetList());
    }
    QSSGRef<QSSGCompositShader> retval;
    if (shader)
        retval = QSSGRef<QSSGCompositShader>(new QSSGCompositShader(shader));
    m_compositShader = retval;
    return m_compositShader;
}

QSSGRef<QSSGShadowmapPreblurShader> QSSGRendererImpl::getCubeShadowBlurXShader()
{
    if (m_cubeShadowBlurXShader)
        return m_cubeShadowBlurXShader;

    QSSGShaderPreprocessorFeature noFragOutputFeature("NO_FRAG_OUTPUT", true);
    ShaderFeatureSetList features;
    features.push_back(noFragOutputFeature);
    QSSGRef<QSSGShaderCache> theCache = m_contextInterface->shaderCache();
    QByteArray name = "cubemap shadow blur X shader";
    QSSGRef<QSSGRenderShaderProgram> shader = theCache->getProgram(name, features);
    if (!shader) {
        getProgramGenerator()->beginProgram();

        QSSGShaderStageGeneratorInterface &vertexGenerator(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Vertex));
        QSSGShaderStageGeneratorInterface &fragmentGenerator(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Fragment));
        vertexGenerator.addIncoming("attr_pos", "vec3");
        // vertexGenerator.AddIncoming("attr_uv", "vec2");
        vertexGenerator.addOutgoing("uv_coords", "vec2");
        vertexGenerator.append("void main() {");
        vertexGenerator.append("    gl_Position = vec4(attr_pos, 1.0 );");
        vertexGenerator.append("    uv_coords.xy = attr_pos.xy;");
        vertexGenerator.append("}");

        // This with the ShadowBlurYShader design for a 2-pass 5x5 (sigma=1.0)
        // Weights computed using -- http://dev.theomader.com/gaussian-kernel-calculator/
        fragmentGenerator.addUniform("cameraProperties", "vec2");
        fragmentGenerator.addUniform("depthCube", "samplerCube");
        // fragmentGenerator.AddUniform("depthSrc", "sampler2D");
        fragmentGenerator.append("layout(location = 0) out vec4 frag0;");
        fragmentGenerator.append("layout(location = 1) out vec4 frag1;");
        fragmentGenerator.append("layout(location = 2) out vec4 frag2;");
        fragmentGenerator.append("layout(location = 3) out vec4 frag3;");
        fragmentGenerator.append("layout(location = 4) out vec4 frag4;");
        fragmentGenerator.append("layout(location = 5) out vec4 frag5;");
        fragmentGenerator.append("void main() {");
        fragmentGenerator.append("    float ofsScale = cameraProperties.x / 2500.0;");
        fragmentGenerator.append("    vec3 dir0 = vec3(1.0, -uv_coords.y, -uv_coords.x);");
        fragmentGenerator.append("    vec3 dir1 = vec3(-1.0, -uv_coords.y, uv_coords.x);");
        fragmentGenerator.append("    vec3 dir2 = vec3(uv_coords.x, 1.0, uv_coords.y);");
        fragmentGenerator.append("    vec3 dir3 = vec3(uv_coords.x, -1.0, -uv_coords.y);");
        fragmentGenerator.append("    vec3 dir4 = vec3(uv_coords.x, -uv_coords.y, 1.0);");
        fragmentGenerator.append("    vec3 dir5 = vec3(-uv_coords.x, -uv_coords.y, -1.0);");
        fragmentGenerator.append("    float depth0;");
        fragmentGenerator.append("    float depth1;");
        fragmentGenerator.append("    float depth2;");
        fragmentGenerator.append("    float outDepth;");
        fragmentGenerator.append("    depth0 = texture(depthCube, dir0).x;");
        fragmentGenerator.append("    depth1 = texture(depthCube, dir0 + vec3(0.0, 0.0, -ofsScale)).x;");
        fragmentGenerator.append("    depth1 += texture(depthCube, dir0 + vec3(0.0, 0.0, ofsScale)).x;");
        fragmentGenerator.append("    depth2 = texture(depthCube, dir0 + vec3(0.0, 0.0, -2.0*ofsScale)).x;");
        fragmentGenerator.append("    depth2 += texture(depthCube, dir0 + vec3(0.0, 0.0, 2.0*ofsScale)).x;");
        fragmentGenerator.append("    outDepth = 0.38774 * depth0 + 0.24477 * depth1 + 0.06136 * depth2;");
        fragmentGenerator.append("    frag0 = vec4(outDepth);");

        fragmentGenerator.append("    depth0 = texture(depthCube, dir1).x;");
        fragmentGenerator.append("    depth1 = texture(depthCube, dir1 + vec3(0.0, 0.0, -ofsScale)).x;");
        fragmentGenerator.append("    depth1 += texture(depthCube, dir1 + vec3(0.0, 0.0, ofsScale)).x;");
        fragmentGenerator.append("    depth2 = texture(depthCube, dir1 + vec3(0.0, 0.0, -2.0*ofsScale)).x;");
        fragmentGenerator.append("    depth2 += texture(depthCube, dir1 + vec3(0.0, 0.0, 2.0*ofsScale)).x;");
        fragmentGenerator.append("    outDepth = 0.38774 * depth0 + 0.24477 * depth1 + 0.06136 * depth2;");
        fragmentGenerator.append("    frag1 = vec4(outDepth);");

        fragmentGenerator.append("    depth0 = texture(depthCube, dir2).x;");
        fragmentGenerator.append("    depth1 = texture(depthCube, dir2 + vec3(-ofsScale, 0.0, 0.0)).x;");
        fragmentGenerator.append("    depth1 += texture(depthCube, dir2 + vec3(ofsScale, 0.0, 0.0)).x;");
        fragmentGenerator.append("    depth2 = texture(depthCube, dir2 + vec3(-2.0*ofsScale, 0.0, 0.0)).x;");
        fragmentGenerator.append("    depth2 += texture(depthCube, dir2 + vec3(2.0*ofsScale, 0.0, 0.0)).x;");
        fragmentGenerator.append("    outDepth = 0.38774 * depth0 + 0.24477 * depth1 + 0.06136 * depth2;");
        fragmentGenerator.append("    frag2 = vec4(outDepth);");

        fragmentGenerator.append("    depth0 = texture(depthCube, dir3).x;");
        fragmentGenerator.append("    depth1 = texture(depthCube, dir3 + vec3(-ofsScale, 0.0, 0.0)).x;");
        fragmentGenerator.append("    depth1 += texture(depthCube, dir3 + vec3(ofsScale, 0.0, 0.0)).x;");
        fragmentGenerator.append("    depth2 = texture(depthCube, dir3 + vec3(-2.0*ofsScale, 0.0, 0.0)).x;");
        fragmentGenerator.append("    depth2 += texture(depthCube, dir3 + vec3(2.0*ofsScale, 0.0, 0.0)).x;");
        fragmentGenerator.append("    outDepth = 0.38774 * depth0 + 0.24477 * depth1 + 0.06136 * depth2;");
        fragmentGenerator.append("    frag3 = vec4(outDepth);");

        fragmentGenerator.append("    depth0 = texture(depthCube, dir4).x;");
        fragmentGenerator.append("    depth1 = texture(depthCube, dir4 + vec3(-ofsScale, 0.0, 0.0)).x;");
        fragmentGenerator.append("    depth1 += texture(depthCube, dir4 + vec3(ofsScale, 0.0, 0.0)).x;");
        fragmentGenerator.append("    depth2 = texture(depthCube, dir4 + vec3(-2.0*ofsScale, 0.0, 0.0)).x;");
        fragmentGenerator.append("    depth2 += texture(depthCube, dir4 + vec3(2.0*ofsScale, 0.0, 0.0)).x;");
        fragmentGenerator.append("    outDepth = 0.38774 * depth0 + 0.24477 * depth1 + 0.06136 * depth2;");
        fragmentGenerator.append("    frag4 = vec4(outDepth);");

        fragmentGenerator.append("    depth0 = texture(depthCube, dir5).x;");
        fragmentGenerator.append("    depth1 = texture(depthCube, dir5 + vec3(-ofsScale, 0.0, 0.0)).x;");
        fragmentGenerator.append("    depth1 += texture(depthCube, dir5 + vec3(ofsScale, 0.0, 0.0)).x;");
        fragmentGenerator.append("    depth2 = texture(depthCube, dir5 + vec3(-2.0*ofsScale, 0.0, 0.0)).x;");
        fragmentGenerator.append("    depth2 += texture(depthCube, dir5 + vec3(2.0*ofsScale, 0.0, 0.0)).x;");
        fragmentGenerator.append("    outDepth = 0.38774 * depth0 + 0.24477 * depth1 + 0.06136 * depth2;");
        fragmentGenerator.append("    frag5 = vec4(outDepth);");

        fragmentGenerator.append("}");

        shader = getProgramGenerator()->compileGeneratedShader(name, QSSGShaderCacheProgramFlags(),
                                                               features);
    }
    QSSGRef<QSSGShadowmapPreblurShader> retval;
    if (shader)
        retval = QSSGRef<QSSGShadowmapPreblurShader>(new QSSGShadowmapPreblurShader(shader));
    m_cubeShadowBlurXShader = retval;
    return m_cubeShadowBlurXShader;
}

QSSGRef<QSSGShadowmapPreblurShader> QSSGRendererImpl::getCubeShadowBlurYShader()
{
    if (m_cubeShadowBlurYShader)
        return m_cubeShadowBlurYShader;

    QSSGShaderPreprocessorFeature noFragOutputFeature("NO_FRAG_OUTPUT", true);
    ShaderFeatureSetList features;
    features.push_back(noFragOutputFeature);
    QSSGRef<QSSGShaderCache> theCache = m_contextInterface->shaderCache();
    QByteArray name = "cubemap shadow blur Y shader";
    QSSGRef<QSSGRenderShaderProgram> shader = theCache->getProgram(name, features);
    if (!shader) {
        getProgramGenerator()->beginProgram();

        QSSGShaderStageGeneratorInterface &vertexGenerator(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Vertex));
        QSSGShaderStageGeneratorInterface &fragmentGenerator(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Fragment));
        vertexGenerator.addIncoming("attr_pos", "vec3");
        // vertexGenerator.AddIncoming("attr_uv", "vec2");
        vertexGenerator.addOutgoing("uv_coords", "vec2");
        vertexGenerator.append("void main() {");
        vertexGenerator.append("    gl_Position = vec4(attr_pos, 1.0 );");
        vertexGenerator.append("    uv_coords.xy = attr_pos.xy;");
        vertexGenerator.append("}");

        // This with the ShadowBlurXShader design for a 2-pass 5x5 (sigma=1.0)
        // Weights computed using -- http://dev.theomader.com/gaussian-kernel-calculator/
        fragmentGenerator.addUniform("cameraProperties", "vec2");
        fragmentGenerator.addUniform("depthCube", "samplerCube");
        // fragmentGenerator.AddUniform("depthSrc", "sampler2D");
        fragmentGenerator.append("layout(location = 0) out vec4 frag0;");
        fragmentGenerator.append("layout(location = 1) out vec4 frag1;");
        fragmentGenerator.append("layout(location = 2) out vec4 frag2;");
        fragmentGenerator.append("layout(location = 3) out vec4 frag3;");
        fragmentGenerator.append("layout(location = 4) out vec4 frag4;");
        fragmentGenerator.append("layout(location = 5) out vec4 frag5;");
        fragmentGenerator.append("void main() {");
        fragmentGenerator.append("    float ofsScale = cameraProperties.x / 2500.0;");
        fragmentGenerator.append("    vec3 dir0 = vec3(1.0, -uv_coords.y, -uv_coords.x);");
        fragmentGenerator.append("    vec3 dir1 = vec3(-1.0, -uv_coords.y, uv_coords.x);");
        fragmentGenerator.append("    vec3 dir2 = vec3(uv_coords.x, 1.0, uv_coords.y);");
        fragmentGenerator.append("    vec3 dir3 = vec3(uv_coords.x, -1.0, -uv_coords.y);");
        fragmentGenerator.append("    vec3 dir4 = vec3(uv_coords.x, -uv_coords.y, 1.0);");
        fragmentGenerator.append("    vec3 dir5 = vec3(-uv_coords.x, -uv_coords.y, -1.0);");
        fragmentGenerator.append("    float depth0;");
        fragmentGenerator.append("    float depth1;");
        fragmentGenerator.append("    float depth2;");
        fragmentGenerator.append("    float outDepth;");
        fragmentGenerator.append("    depth0 = texture(depthCube, dir0).x;");
        fragmentGenerator.append("    depth1 = texture(depthCube, dir0 + vec3(0.0, -ofsScale, 0.0)).x;");
        fragmentGenerator.append("    depth1 += texture(depthCube, dir0 + vec3(0.0, ofsScale, 0.0)).x;");
        fragmentGenerator.append("    depth2 = texture(depthCube, dir0 + vec3(0.0, -2.0*ofsScale, 0.0)).x;");
        fragmentGenerator.append("    depth2 += texture(depthCube, dir0 + vec3(0.0, 2.0*ofsScale, 0.0)).x;");
        fragmentGenerator.append("    outDepth = 0.38774 * depth0 + 0.24477 * depth1 + 0.06136 * depth2;");
        fragmentGenerator.append("    frag0 = vec4(outDepth);");

        fragmentGenerator.append("    depth0 = texture(depthCube, dir1).x;");
        fragmentGenerator.append("    depth1 = texture(depthCube, dir1 + vec3(0.0, -ofsScale, 0.0)).x;");
        fragmentGenerator.append("    depth1 += texture(depthCube, dir1 + vec3(0.0, ofsScale, 0.0)).x;");
        fragmentGenerator.append("    depth2 = texture(depthCube, dir1 + vec3(0.0, -2.0*ofsScale, 0.0)).x;");
        fragmentGenerator.append("    depth2 += texture(depthCube, dir1 + vec3(0.0, 2.0*ofsScale, 0.0)).x;");
        fragmentGenerator.append("    outDepth = 0.38774 * depth0 + 0.24477 * depth1 + 0.06136 * depth2;");
        fragmentGenerator.append("    frag1 = vec4(outDepth);");

        fragmentGenerator.append("    depth0 = texture(depthCube, dir2).x;");
        fragmentGenerator.append("    depth1 = texture(depthCube, dir2 + vec3(0.0, 0.0, -ofsScale)).x;");
        fragmentGenerator.append("    depth1 += texture(depthCube, dir2 + vec3(0.0, 0.0, ofsScale)).x;");
        fragmentGenerator.append("    depth2 = texture(depthCube, dir2 + vec3(0.0, 0.0, -2.0*ofsScale)).x;");
        fragmentGenerator.append("    depth2 += texture(depthCube, dir2 + vec3(0.0, 0.0, 2.0*ofsScale)).x;");
        fragmentGenerator.append("    outDepth = 0.38774 * depth0 + 0.24477 * depth1 + 0.06136 * depth2;");
        fragmentGenerator.append("    frag2 = vec4(outDepth);");

        fragmentGenerator.append("    depth0 = texture(depthCube, dir3).x;");
        fragmentGenerator.append("    depth1 = texture(depthCube, dir3 + vec3(0.0, 0.0, -ofsScale)).x;");
        fragmentGenerator.append("    depth1 += texture(depthCube, dir3 + vec3(0.0, 0.0, ofsScale)).x;");
        fragmentGenerator.append("    depth2 = texture(depthCube, dir3 + vec3(0.0, 0.0, -2.0*ofsScale)).x;");
        fragmentGenerator.append("    depth2 += texture(depthCube, dir3 + vec3(0.0, 0.0, 2.0*ofsScale)).x;");
        fragmentGenerator.append("    outDepth = 0.38774 * depth0 + 0.24477 * depth1 + 0.06136 * depth2;");
        fragmentGenerator.append("    frag3 = vec4(outDepth);");

        fragmentGenerator.append("    depth0 = texture(depthCube, dir4).x;");
        fragmentGenerator.append("    depth1 = texture(depthCube, dir4 + vec3(0.0, -ofsScale, 0.0)).x;");
        fragmentGenerator.append("    depth1 += texture(depthCube, dir4 + vec3(0.0, ofsScale, 0.0)).x;");
        fragmentGenerator.append("    depth2 = texture(depthCube, dir4 + vec3(0.0, -2.0*ofsScale, 0.0)).x;");
        fragmentGenerator.append("    depth2 += texture(depthCube, dir4 + vec3(0.0, 2.0*ofsScale, 0.0)).x;");
        fragmentGenerator.append("    outDepth = 0.38774 * depth0 + 0.24477 * depth1 + 0.06136 * depth2;");
        fragmentGenerator.append("    frag4 = vec4(outDepth);");

        fragmentGenerator.append("    depth0 = texture(depthCube, dir5).x;");
        fragmentGenerator.append("    depth1 = texture(depthCube, dir5 + vec3(0.0, -ofsScale, 0.0)).x;");
        fragmentGenerator.append("    depth1 += texture(depthCube, dir5 + vec3(0.0, ofsScale, 0.0)).x;");
        fragmentGenerator.append("    depth2 = texture(depthCube, dir5 + vec3(0.0, -2.0*ofsScale, 0.0)).x;");
        fragmentGenerator.append("    depth2 += texture(depthCube, dir5 + vec3(0.0, 2.0*ofsScale, 0.0)).x;");
        fragmentGenerator.append("    outDepth = 0.38774 * depth0 + 0.24477 * depth1 + 0.06136 * depth2;");
        fragmentGenerator.append("    frag5 = vec4(outDepth);");

        fragmentGenerator.append("}");

        shader = getProgramGenerator()->compileGeneratedShader(name, QSSGShaderCacheProgramFlags(),
                                                               features);
    }

    QSSGRef<QSSGShadowmapPreblurShader> retval;
    if (shader)
        retval = QSSGRef<QSSGShadowmapPreblurShader>(new QSSGShadowmapPreblurShader(shader));
    m_cubeShadowBlurYShader = retval;
    return m_cubeShadowBlurYShader;
}

QSSGRef<QSSGShadowmapPreblurShader> QSSGRendererImpl::getOrthoShadowBlurXShader()
{
    if (m_orthoShadowBlurXShader)
        return m_orthoShadowBlurXShader;

    QSSGRef<QSSGShaderCache> theCache = m_contextInterface->shaderCache();
    QByteArray name = "shadow map blur X shader";
    QSSGRef<QSSGRenderShaderProgram> shader = theCache->getProgram(name, ShaderFeatureSetList());
    if (!shader) {
        getProgramGenerator()->beginProgram();

        QSSGShaderStageGeneratorInterface &vertexGenerator(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Vertex));
        QSSGShaderStageGeneratorInterface &fragmentGenerator(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Fragment));
        vertexGenerator.addIncoming("attr_pos", "vec3");
        vertexGenerator.addIncoming("attr_uv", "vec2");
        vertexGenerator.addOutgoing("uv_coords", "vec2");
        vertexGenerator.append("void main() {");
        vertexGenerator.append("    gl_Position = vec4(attr_pos, 1.0 );");
        vertexGenerator.append("    uv_coords.xy = attr_uv.xy;");
        vertexGenerator.append("}");

        fragmentGenerator.addUniform("cameraProperties", "vec2");
        fragmentGenerator.addUniform("depthSrc", "sampler2D");
        fragmentGenerator.append("void main() {");
        fragmentGenerator.append("    vec2 ofsScale = vec2( cameraProperties.x / 7680.0, 0.0 );");
        fragmentGenerator.append("    float depth0 = texture(depthSrc, uv_coords).x;");
        fragmentGenerator.append("    float depth1 = texture(depthSrc, uv_coords + ofsScale).x;");
        fragmentGenerator.append("    depth1 += texture(depthSrc, uv_coords - ofsScale).x;");
        fragmentGenerator.append("    float depth2 = texture(depthSrc, uv_coords + 2.0 * ofsScale).x;");
        fragmentGenerator.append("    depth2 += texture(depthSrc, uv_coords - 2.0 * ofsScale).x;");
        fragmentGenerator.append("    float outDepth = 0.38774 * depth0 + 0.24477 * depth1 + 0.06136 * depth2;");
        fragmentGenerator.append("    fragOutput = vec4(outDepth);");
        fragmentGenerator.append("}");

        shader = getProgramGenerator()->compileGeneratedShader(name, QSSGShaderCacheProgramFlags(),
                                                               ShaderFeatureSetList());
    }
    QSSGRef<QSSGShadowmapPreblurShader> retval;
    if (shader)
        retval = QSSGRef<QSSGShadowmapPreblurShader>(new QSSGShadowmapPreblurShader(shader));
    m_orthoShadowBlurXShader = retval;
    return m_orthoShadowBlurXShader;
}

QSSGRef<QSSGShadowmapPreblurShader> QSSGRendererImpl::getOrthoShadowBlurYShader()
{
    if (m_orthoShadowBlurYShader)
        return m_orthoShadowBlurYShader;

    QSSGRef<QSSGShaderCache> theCache = m_contextInterface->shaderCache();
    QByteArray name = "shadow map blur Y shader";
    QSSGRef<QSSGRenderShaderProgram> shader = theCache->getProgram(name, ShaderFeatureSetList());
    if (!shader) {
        getProgramGenerator()->beginProgram();

        QSSGShaderStageGeneratorInterface &vertexGenerator(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Vertex));
        QSSGShaderStageGeneratorInterface &fragmentGenerator(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Fragment));
        vertexGenerator.addIncoming("attr_pos", "vec3");
        vertexGenerator.addIncoming("attr_uv", "vec2");
        vertexGenerator.addOutgoing("uv_coords", "vec2");
        vertexGenerator.append("void main() {");
        vertexGenerator.append("    gl_Position = vec4(attr_pos, 1.0 );");
        vertexGenerator.append("    uv_coords.xy = attr_uv.xy;");
        vertexGenerator.append("}");

        fragmentGenerator.addUniform("cameraProperties", "vec2");
        fragmentGenerator.addUniform("depthSrc", "sampler2D");
        fragmentGenerator.append("void main() {");
        fragmentGenerator.append("    vec2 ofsScale = vec2( 0.0, cameraProperties.x / 7680.0 );");
        fragmentGenerator.append("    float depth0 = texture(depthSrc, uv_coords).x;");
        fragmentGenerator.append("    float depth1 = texture(depthSrc, uv_coords + ofsScale).x;");
        fragmentGenerator.append("    depth1 += texture(depthSrc, uv_coords - ofsScale).x;");
        fragmentGenerator.append("    float depth2 = texture(depthSrc, uv_coords + 2.0 * ofsScale).x;");
        fragmentGenerator.append("    depth2 += texture(depthSrc, uv_coords - 2.0 * ofsScale).x;");
        fragmentGenerator.append("    float outDepth = 0.38774 * depth0 + 0.24477 * depth1 + 0.06136 * depth2;");
        fragmentGenerator.append("    fragOutput = vec4(outDepth);");
        fragmentGenerator.append("}");

        shader = getProgramGenerator()->compileGeneratedShader(name, QSSGShaderCacheProgramFlags(),
                                                               ShaderFeatureSetList());
    }
    QSSGRef<QSSGShadowmapPreblurShader> retval;
    if (shader)
        retval = QSSGRef<QSSGShadowmapPreblurShader>(new QSSGShadowmapPreblurShader(shader));
    m_orthoShadowBlurYShader = retval;
    return m_orthoShadowBlurYShader;
}

QT_END_NAMESPACE
