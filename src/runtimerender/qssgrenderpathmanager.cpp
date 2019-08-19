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

#include "qssgrenderpathmanager_p.h"

#include <QtQuick3DRender/private/qssgrendervertexbuffer_p.h>
#include <QtQuick3DRender/private/qssgrenderinputassembler_p.h>
#include <QtQuick3DRender/private/qssgrendercontext_p.h>
#include <QtQuick3DRender/private/qssgrendervertexbuffer_p.h>
#include <QtQuick3DRender/private/qssgrendershaderprogram_p.h>
#include <QtQuick3DRender/private/qssgrendershaderprogram_p.h>
#include <QtQuick3DRender/private/qssgrenderpathrender_p.h>
#include <QtQuick3DRender/private/qssgrenderpathspecification_p.h>

#include <QtQuick3DRuntimeRender/private/qssgrenderpath_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercontextcore_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercodegenerator_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderdynamicobjectsystem_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercamera_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderpathrendercontext_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercodegeneratorv2_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderdefaultmaterialshadergenerator_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercustommaterial_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercustommaterialsystem_p.h>
#include <QtQuick3DRuntimeRender/private/qssgvertexpipelineimpl_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendersubpath_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderpathmath_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderinputstreamfactory_p.h>

#include <QtQuick3DAssetImport/private/qssgpathutilities_p.h>

QT_BEGIN_NAMESPACE

typedef QSSGPathUtilities::QSSGPathBuffer TImportPathBuffer;
using namespace path;

struct QSSGPathShaderMapKey
{
    QByteArray m_name;
    QSSGShaderDefaultMaterialKey m_materialKey;
    uint m_hashCode;
    QSSGPathShaderMapKey(const QByteArray &inName, QSSGShaderDefaultMaterialKey inKey)
        : m_name(inName), m_materialKey(inKey)
    {
        m_hashCode = qHash(m_name) ^ m_materialKey.hash();
    }
    bool operator==(const QSSGPathShaderMapKey &inKey) const
    {
        return m_hashCode == inKey.m_hashCode && m_name == inKey.m_name && m_materialKey == inKey.m_materialKey;
    }
};

uint qHash(const QSSGPathShaderMapKey &inKey)
{
    return inKey.m_hashCode;
}

namespace {

struct QSSGPathSubPathBuffer
{
    QAtomicInt ref;
    QVector<QSSGPathAnchorPoint> m_sourceData;
    QSSGPathDirtyFlags m_flags;
    QSSGRenderSubPath &m_subPath;
    bool m_closed;

    QSSGPathSubPathBuffer(QSSGRenderSubPath &inSubPath) : m_subPath(inSubPath), m_closed(false) {}
};

struct QSSGImportPathWrapper
{
    QAtomicInt ref;
    QSSGPathUtilities::QSSGPathBuffer *m_path;

    QSSGImportPathWrapper(QSSGPathUtilities::QSSGPathBuffer &inPath) : m_path(&inPath) {}

    ~QSSGImportPathWrapper() { delete m_path; }
};

typedef QSSGRef<QSSGImportPathWrapper> TPathBufferPtr;

struct QSSGPathBuffer
{
    QAtomicInt ref;
    QVector<QSSGRef<QSSGPathSubPathBuffer>> m_subPaths;
    TPathBufferPtr m_pathBuffer;

    QSSGRef<QSSGRenderVertexBuffer> m_patchData;
    QSSGRef<QSSGRenderInputAssembler> m_inputAssembler;
    QSSGRef<QSSGRenderPathRender> m_pathRender;

    QVector2D m_beginTaperData;
    QVector2D m_endTaperData;
    quint32 m_numVertexes{ 0 };
    QSSGRenderPath::PathType m_pathType{ QSSGRenderPath::PathType::Geometry };
    float m_width{ 0.0f };
    float m_cpuError{ 0.0f };
    QSSGBounds3 m_bounds = QSSGBounds3::empty();
    QSSGOption<QSSGTaperInformation> m_beginTaper;
    QSSGOption<QSSGTaperInformation> m_endTaper;
    QString m_sourcePath;

    // Cached data for geometry paths

    QSSGPathDirtyFlags m_flags;

    void clearGeometryPathData()
    {
        m_patchData = nullptr;
        m_inputAssembler = nullptr;
    }

    void clearPaintedPathData() { m_pathRender = nullptr; }

    QSSGPathUtilities::QSSGPathBuffer getPathData(QSSGPathUtilities::QSSGPathBufferBuilder &inSpec)
    {
        if (m_subPaths.size()) {
            inSpec.clear();
            for (int idx = 0, end = m_subPaths.size(); idx < end; ++idx) {
                const QSSGPathSubPathBuffer &theSubPathBuffer(*m_subPaths[idx]);
                for (int equationIdx = 0, equationEnd = theSubPathBuffer.m_sourceData.size(); equationIdx < equationEnd;
                     ++equationIdx) {
                    const QSSGPathAnchorPoint &thePoint = theSubPathBuffer.m_sourceData[equationIdx];
                    if (equationIdx == 0) {
                        inSpec.moveTo(thePoint.position);
                    } else {
                        const QSSGPathAnchorPoint &thePrevPoint = theSubPathBuffer.m_sourceData[equationIdx - 1];
                        QVector2D c1 = QSSGPathManagerInterface::getControlPointFromAngleDistance(thePrevPoint.position,
                                                                                                    thePrevPoint.outgoingAngle,
                                                                                                    thePrevPoint.outgoingDistance);
                        QVector2D c2 = QSSGPathManagerInterface::getControlPointFromAngleDistance(thePoint.position,
                                                                                                    thePoint.incomingAngle,
                                                                                                    thePoint.incomingDistance);
                        QVector2D p2 = thePoint.position;
                        inSpec.cubicCurveTo(c1, c2, p2);
                    }
                }
                if (theSubPathBuffer.m_closed)
                    inSpec.close();
            }
            return inSpec.getPathBuffer();
        } else if (m_pathBuffer)
            return *m_pathBuffer->m_path;
        return QSSGPathUtilities::QSSGPathBuffer();
    }

    void setPathType(QSSGRenderPath::PathType inPathType)
    {
        if (inPathType != m_pathType) {
            switch (m_pathType) {
            case QSSGRenderPath::PathType::Geometry:
                clearGeometryPathData();
                break;
            case QSSGRenderPath::PathType::Painted:
                clearPaintedPathData();
                break;
            default:
                Q_UNREACHABLE();
                // No further processing for unexpected path type
                return;
            }
            m_flags |= QSSGPathDirtyFlagValue::PathType;
        }
        m_pathType = inPathType;
    }

    static QSSGOption<QSSGTaperInformation> toTaperInfo(QSSGRenderPath::Capping capping, float capOffset, float capOpacity, float capWidth)
    {
        if (capping == QSSGRenderPath::Capping::None)
            return QSSGEmpty();

        return QSSGTaperInformation(capOffset, capOpacity, capWidth);
    }

    void setBeginTaperInfo(QSSGRenderPath::Capping capping, float capOffset, float capOpacity, float capWidth)
    {
        QSSGOption<QSSGTaperInformation> newBeginInfo = toTaperInfo(capping, capOffset, capOpacity, capWidth);
        if (newBeginInfo != m_beginTaper) {
            m_beginTaper = newBeginInfo;
            m_flags |= QSSGPathDirtyFlagValue::BeginTaper;
        }
    }

    void setEndTaperInfo(QSSGRenderPath::Capping capping, float capOffset, float capOpacity, float capWidth)
    {
        QSSGOption<QSSGTaperInformation> newEndInfo = toTaperInfo(capping, capOffset, capOpacity, capWidth);
        if (newEndInfo != m_endTaper) {
            m_endTaper = newEndInfo;
            m_flags |= QSSGPathDirtyFlagValue::EndTaper;
        }
    }

    void setWidth(float inWidth)
    {
        if (!qFuzzyCompare(inWidth, m_width)) {
            m_width = inWidth;
            m_flags |= QSSGPathDirtyFlagValue::Width;
        }
    }

    void setCPUError(float inError)
    {
        if (!qFuzzyCompare(inError, m_cpuError)) {
            m_cpuError = inError;
            m_flags |= QSSGPathDirtyFlagValue::CPUError;
        }
    }
};

struct QSSGPathGeneratedShader
{
    QSSGRef<QSSGRenderShaderProgram> m_shader;
    QSSGRenderCachedShaderProperty<float> m_width;
    QSSGRenderCachedShaderProperty<float> m_innerTessAmount;
    QSSGRenderCachedShaderProperty<float> m_edgeTessAmount;
    QSSGRenderCachedShaderProperty<QVector2D> m_beginTaperData;
    QSSGRenderCachedShaderProperty<QVector2D> m_endTaperData;
    QSSGRenderCachedShaderProperty<QMatrix4x4> m_wireframeViewMatrix;

    QSSGPathGeneratedShader(const QSSGRef<QSSGRenderShaderProgram> &sh)
        : m_shader(sh)
        , m_width("pathWidth", sh)
        , m_innerTessAmount("tessInnerLevel", sh)
        , m_edgeTessAmount("tessEdgeLevel", sh)
        , m_beginTaperData("beginTaperInfo", sh)
        , m_endTaperData("endTaperInfo", sh)
        , m_wireframeViewMatrix("viewport_matrix", sh)
    {
    }
};

struct QSSGPathVertexPipeline : public QSSGVertexPipelineImpl
{

    QSSGPathVertexPipeline(const QSSGRef<QSSGShaderProgramGeneratorInterface> &inProgGenerator,
                             const QSSGRef<QSSGMaterialShaderGeneratorInterface> &inMaterialGenerator,
                             bool inWireframe)
        : QSSGVertexPipelineImpl(inMaterialGenerator, inProgGenerator, inWireframe)
    {
    }

    void assignTessEvalVarying(const QByteArray &inVarName, const QByteArray &inVarValueExpr)
    {
        QByteArray ext;
        if (programGenerator()->getEnabledStages() & QSSGShaderGeneratorStage::Geometry)
            ext = "TE";
        tessEval() << "\t" << inVarName << ext << " = " << inVarValueExpr << ";"
                   << "\n";
    }

    void assignOutput(const QByteArray &inVarName, const QByteArray &inVarValueExpr) override
    {
        assignTessEvalVarying(inVarName, inVarValueExpr);
    }

    void initializeTessShaders()
    {
        QSSGShaderStageGeneratorInterface &theTessControl(tessControl());
        QSSGShaderStageGeneratorInterface &theTessEval(tessEval());

        // first setup tessellation control shader
        theTessControl.addUniform("tessEdgeLevel", "float");
        theTessControl.addUniform("tessInnerLevel", "float");

        theTessControl.addInclude("tessellationPath.glsllib");

        theTessControl.append("void main() {\n");
        theTessControl.append("\tgl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;");
        theTessControl.append("\ttessShader( tessEdgeLevel, tessInnerLevel );\n");

        bool hasGeometryShader = programGenerator()->getStage(QSSGShaderGeneratorStage::Geometry) != nullptr;

        // second setup tessellation control shader
        QByteArray outExt("");
        if (hasGeometryShader)
            outExt = "TE";

        theTessEval.addInclude("tessellationPath.glsllib");
        theTessEval.addUniform("normal_matrix", "mat3");
        theTessEval.addUniform("model_view_projection", "mat4");
        theTessEval.addUniform("pathWidth", "float");
        theTessEval.addUniform("material_diffuse", "vec4");
        addInterpolationParameter("varTexCoord0", "vec2");
        addInterpolationParameter("varTessOpacity", "float");

        theTessEval.append("void main() {\n");
        theTessEval.append("\tSTessShaderResult shaderResult = tessShader( pathWidth );\n");
        theTessEval.append("\tvec3 pos = shaderResult.m_Position;\n");
        assignTessEvalVarying("varTessOpacity", "shaderResult.m_Opacity");
        assignTessEvalVarying("varTexCoord0", "shaderResult.m_TexCoord.xy");
        if (hasGeometryShader)
            theTessEval << "\tvec2 varTexCoord0 = shaderResult.m_TexCoord.xy;\n";

        theTessEval << "\tvec3 object_normal = vec3(0.0, 0.0, 1.0);\n";
        theTessEval << "\tvec3 world_normal = normal_matrix * object_normal;\n";
        theTessEval << "\tvec3 tangent = vec3( shaderResult.m_Tangent, 0.0 );\n";
        theTessEval << "\tvec3 binormal = vec3( shaderResult.m_Binormal, 0.0 );\n";

        // These are necessary for texture generation.
        theTessEval << "\tvec3 uTransform;"
                    << "\n";
        theTessEval << "\tvec3 vTransform;"
                    << "\n";

        if (m_displacementImage) {
            materialGenerator()->generateImageUVCoordinates(*this, m_displacementIdx, 0, *m_displacementImage);
            theTessEval.addUniform("displaceAmount", "float");
            theTessEval.addUniform("model_matrix", "mat4");
            theTessEval.addInclude("defaultMaterialFileDisplacementTexture.glsllib");
            QSSGDefaultMaterialShaderGeneratorInterface::ImageVariableNames theVarNames = materialGenerator()->getImageVariableNames(
                    m_displacementIdx);

            theTessEval.addUniform(theVarNames.m_imageSampler, "sampler2D");
            QSSGDefaultMaterialShaderGeneratorInterface::ImageVariableNames theNames = materialGenerator()->getImageVariableNames(
                    m_displacementIdx);
            theTessEval << "\tpos = defaultMaterialFileDisplacementTexture( " << theNames.m_imageSampler
                        << ", displaceAmount, " << theNames.m_imageFragCoords << outExt << ", vec3( 0.0, 0.0, 1.0 )"
                        << ", pos.xyz );"
                        << "\n";
        }
    }
    void finalizeTessControlShader() {}

    void finalizeTessEvaluationShader()
    {
        // ### Investigate whether the outExp should be used
        //        QString outExt("");
        //        if (programGenerator()->getEnabledStages() & ShaderGeneratorStages::Geometry)
        //            outExt = "TE";

        QSSGShaderStageGeneratorInterface &tessEvalShader(*programGenerator()->getStage(QSSGShaderGeneratorStage::TessEval));
        tessEvalShader.append("\tgl_Position = model_view_projection * vec4( pos, 1.0 );\n");
    }

    void beginVertexGeneration(quint32 displacementImageIdx, QSSGRenderableImage *displacementImage) override
    {
        setupDisplacement(displacementImageIdx, displacementImage);

        QSSGShaderGeneratorStageFlags theStages(QSSGShaderProgramGeneratorInterface::defaultFlags());
        theStages |= QSSGShaderGeneratorStage::TessControl;
        theStages |= QSSGShaderGeneratorStage::TessEval;
        if (m_wireframe) {
            theStages |= QSSGShaderGeneratorStage::Geometry;
        }
        programGenerator()->beginProgram(theStages);
        initializeTessShaders();
        if (m_wireframe) {
            initializeWireframeGeometryShader();
        }
        // Open up each stage.
        QSSGShaderStageGeneratorInterface &vertexShader(vertex());

        vertexShader.addIncoming("attr_pos", "vec4");

        // useless vert shader because real work is done in TES.
        vertexShader << "void main()\n"
                        "{\n";
        vertexShader << "\tgl_Position = attr_pos;\n"; // if tessellation is enabled pass down
        // object coordinates;
        vertexShader << "}\n";
    }

    void beginFragmentGeneration() override
    {
        fragment().addUniform("material_diffuse", "vec4");
        fragment() << "void main()"
                   << "\n"
                   << "{"
                   << "\n";
        // We do not pass object opacity through the pipeline.
        fragment() << "\tfloat object_opacity = varTessOpacity * material_diffuse.a;"
                   << "\n";
    }
    void doGenerateUVCoords(quint32) override
    {
        // these are always generated regardless
    }

    // fragment shader expects varying vertex normal
    // lighting in vertex pipeline expects world_normal
    void doGenerateWorldNormal() override { assignTessEvalVarying("varNormal", "world_normal"); }
    void doGenerateObjectNormal() override { assignTessEvalVarying("varObjectNormal", "object_normal"); }
    void doGenerateWorldPosition() override
    {
        tessEval().addUniform("model_matrix", "mat4");
        tessEval() << "\tvec3 local_model_world_position = vec3((model_matrix * vec4(pos, 1.0)).xyz);\n";
    }
    void doGenerateVarTangentAndBinormal() override
    {
        tessEval().addUniform("normal_matrix", "mat3");
        assignOutput("varTangent", "normal_matrix * tangent");
        assignOutput("varBinormal", "normal_matrix * binormal");
    }

    void doGenerateVertexColor() override
    {
        vertex().addIncoming("attr_color", "vec3");
        vertex() << "\tvarColor = attr_color;"
                 << "\n";
    }

    void endVertexGeneration(bool) override
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
    }

    void endFragmentGeneration(bool) override { fragment().append("}"); }

    void addInterpolationParameter(const QByteArray &inName, const QByteArray &inType) override
    {
        m_interpolationParameters.insert(inName, inType);
        fragment().addIncoming(inName, inType);
        if (hasTessellation()) {
            QByteArray nameBuilder;
            nameBuilder = inName;
            if (programGenerator()->getEnabledStages() & QSSGShaderGeneratorStage::Geometry)
                nameBuilder.append("TE");

            tessEval().addOutgoing(nameBuilder, inType);
        }
    }

    QSSGShaderStageGeneratorInterface &activeStage() override { return tessEval(); }
};

struct QSSGPathXYGeneratedShader
{
    QSSGRef<QSSGRenderShaderProgram> m_shader;
    QSSGRenderCachedShaderProperty<QVector4D> m_rectDimensions;
    QSSGRenderCachedShaderProperty<QMatrix4x4> m_modelMatrix;
    QSSGRenderCachedShaderProperty<QVector3D> m_cameraPosition;
    QSSGRenderCachedShaderProperty<QVector2D> m_cameraProperties;

    QSSGPathXYGeneratedShader(const QSSGRef<QSSGRenderShaderProgram> &sh)
        : m_shader(sh)
        , m_rectDimensions("uni_rect_dimensions", sh)
        , m_modelMatrix("model_matrix", sh)
        , m_cameraPosition("camera_position", sh)
        , m_cameraProperties("camera_properties", sh)
    {
    }
    virtual ~QSSGPathXYGeneratedShader() = default;
};

// Helper implements the vertex pipeline for mesh subsets when bound to the default material.
// Should be completely possible to use for custom materials with a bit of refactoring.
struct QSSGXYRectVertexPipeline : public QSSGVertexPipelineImpl
{

    QSSGXYRectVertexPipeline(QSSGRef<QSSGShaderProgramGeneratorInterface> inProgGenerator,
                               QSSGRef<QSSGMaterialShaderGeneratorInterface> inMaterialGenerator)
        : QSSGVertexPipelineImpl(inMaterialGenerator, inProgGenerator, false)
    {
    }

    void beginVertexGeneration(quint32 displacementImageIdx, QSSGRenderableImage *displacementImage) override
    {
        m_displacementIdx = displacementImageIdx;
        m_displacementImage = displacementImage;

        QSSGShaderGeneratorStageFlags theStages(QSSGShaderProgramGeneratorInterface::defaultFlags());
        programGenerator()->beginProgram(theStages);
        // Open up each stage.
        QSSGShaderStageGeneratorInterface &vertexShader(vertex());
        vertexShader.addIncoming("attr_pos", "vec2");
        vertexShader.addUniform("uni_rect_dimensions", "vec4");

        vertexShader << "void main()"
                     << "\n"
                     << "{"
                     << "\n";
        vertexShader << "\tvec3 uTransform;"
                     << "\n";
        vertexShader << "\tvec3 vTransform;"
                     << "\n";

        vertexShader.addUniform("model_view_projection", "mat4");
        vertexShader << "\tfloat posX = mix( uni_rect_dimensions.x, uni_rect_dimensions.z, attr_pos.x );"
                     << "\n";
        vertexShader << "\tfloat posY = mix( uni_rect_dimensions.y, uni_rect_dimensions.w, attr_pos.y );"
                     << "\n";
        vertexShader << "\tvec3  pos = vec3(posX, posY, 0.0 );"
                     << "\n";
        vertexShader.append("\tgl_Position = model_view_projection * vec4(pos, 1.0);");
    }

    void outputParaboloidDepthShaders()
    {
        QSSGShaderGeneratorStageFlags theStages(QSSGShaderProgramGeneratorInterface::defaultFlags());
        programGenerator()->beginProgram(theStages);
        QSSGShaderStageGeneratorInterface &vertexShader(vertex());
        vertexShader.addIncoming("attr_pos", "vec2");
        vertexShader.addUniform("uni_rect_dimensions", "vec4");
        vertexShader.addUniform("model_view_projection", "mat4");
        vertexShader << "void main()"
                     << "\n"
                     << "{"
                     << "\n";
        vertexShader << "\tfloat posX = mix( uni_rect_dimensions.x, uni_rect_dimensions.z, attr_pos.x );"
                     << "\n";
        vertexShader << "\tfloat posY = mix( uni_rect_dimensions.y, uni_rect_dimensions.w, attr_pos.y );"
                     << "\n";
        vertexShader << "\tvec3 pos = vec3(posX, posY, 0.0 );"
                     << "\n";
        QSSGShaderProgramGeneratorInterface::outputParaboloidDepthTessEval(vertexShader);
        vertexShader << "}"
                     << "\n";

        QSSGShaderProgramGeneratorInterface::outputParaboloidDepthFragment(fragment());
    }

    void outputCubeFaceDepthShaders()
    {
        QSSGShaderGeneratorStageFlags theStages(QSSGShaderProgramGeneratorInterface::defaultFlags());
        programGenerator()->beginProgram(theStages);
        QSSGShaderStageGeneratorInterface &vertexShader(vertex());
        QSSGShaderStageGeneratorInterface &fragmentShader(fragment());
        vertexShader.addIncoming("attr_pos", "vec2");
        vertexShader.addUniform("uni_rect_dimensions", "vec4");
        vertexShader.addUniform("model_matrix", "mat4");
        vertexShader.addUniform("model_view_projection", "mat4");

        vertexShader.addOutgoing("world_pos", "vec4");
        vertexShader.append("void main() {");
        vertexShader.append("   float posX = mix( uni_rect_dimensions.x, uni_rect_dimensions.z, attr_pos.x );");
        vertexShader.append("   float posY = mix( uni_rect_dimensions.y, uni_rect_dimensions.w, attr_pos.y );");
        vertexShader.append("   world_pos = model_matrix * vec4( posX, posY, 0.0, 1.0 );");
        vertexShader.append("   world_pos /= world_pos.w;");
        vertexShader.append("	gl_Position = model_view_projection * vec4( posX, posY, 0.0, 1.0 );");
        vertexShader.append("}");

        fragmentShader.addUniform("camera_position", "vec3");
        fragmentShader.addUniform("camera_properties", "vec2");

        beginFragmentGeneration();
        fragmentShader.append("\tfloat dist = 0.5 * length( world_pos.xyz - camera_position );"); // Why?
        fragmentShader.append("\tdist = (dist - camera_properties.x) / (camera_properties.y - camera_properties.x);");
        fragmentShader.append("\tfragOutput = vec4(dist);");
        fragmentShader.append("}");
    }

    void beginFragmentGeneration() override
    {
        fragment().addUniform("material_diffuse", "vec4");
        fragment() << "void main()"
                   << "\n"
                   << "{"
                   << "\n";
        // We do not pass object opacity through the pipeline.
        fragment() << "\tfloat object_opacity = material_diffuse.a;"
                   << "\n";
    }

    void assignOutput(const QByteArray &inVarName, const QByteArray &inVarValue) override
    {
        vertex() << "\t" << inVarName << " = " << inVarValue << ";\n";
    }
    void doGenerateUVCoords(quint32) override
    {
        vertex() << "\tvarTexCoord0 = attr_pos;"
                 << "\n";
    }

    // fragment shader expects varying vertex normal
    // lighting in vertex pipeline expects world_normal
    void doGenerateWorldNormal() override
    {
        QSSGShaderStageGeneratorInterface &vertexGenerator(vertex());
        vertexGenerator.addUniform("normal_matrix", "mat3");
        vertexGenerator.append("\tvec3 world_normal = normalize(normal_matrix * vec3( 0.0, 0.0, 1.0) ).xyz;");
        vertexGenerator.append("\tvarNormal = world_normal;");
    }

    void doGenerateObjectNormal() override
    {
        addInterpolationParameter("varObjectNormal", "vec3");
        vertex().append("\tvarObjectNormal = vec3(0.0, 0.0, 1.0 );");
    }

    void doGenerateWorldPosition() override
    {
        vertex().append("\tvec3 local_model_world_position = (model_matrix * vec4(pos, 1.0)).xyz;");
        assignOutput("varWorldPos", "local_model_world_position");
    }

    void doGenerateVarTangentAndBinormal() override
    {
        vertex().addIncoming("attr_textan", "vec3");
        vertex().addIncoming("attr_binormal", "vec3");
        vertex() << "\tvarTangent = normal_matrix * vec3(1.0, 0.0, 0.0);"
                 << "\n"
                 << "\tvarBinormal = normal_matrix * vec3(0.0, 1.0, 0.0);"
                 << "\n";
    }

    void doGenerateVertexColor() override
    {
        vertex().addIncoming("attr_color", "vec3");
        vertex() << "\tvarColor = attr_color;"
                 << "\n";
    }

    void endVertexGeneration(bool) override { vertex().append("}"); }

    void endFragmentGeneration(bool) override { fragment().append("}"); }

    void addInterpolationParameter(const QByteArray &inName, const QByteArray &inType) override
    {
        m_interpolationParameters.insert(inName, inType);
        vertex().addOutgoing(inName, inType);
        fragment().addIncoming(inName, inType);
    }

    QSSGShaderStageGeneratorInterface &activeStage() override { return vertex(); }
};

struct QSSGPathManager : public QSSGPathManagerInterface
{
    typedef QHash<QSSGRenderPath *, QSSGRef<QSSGPathBuffer>> TPathBufferHash;
    typedef QHash<QSSGRenderSubPath *, QSSGRef<QSSGPathSubPathBuffer>> TPathSubPathBufferHash;
    typedef QHash<QSSGPathShaderMapKey, QSSGPathGeneratedShader *> TShaderMap;
    typedef QHash<QSSGPathShaderMapKey, QSSGPathXYGeneratedShader *> TPaintedShaderMap;
    typedef QHash<QString, TPathBufferPtr> TStringPathBufferMap;

    QSSGRenderContextInterface *m_context;
    QString m_idBuilder;
    TPathSubPathBufferHash m_subPathBuffers;
    TPathBufferHash m_buffers;
    QVector<QSSGResultCubic> m_subdivResult;
    QVector<float> m_keyPointVec;
    QVector<QVector4D> m_patchBuffer;
    TShaderMap m_pathGeometryShaders;
    TPaintedShaderMap m_pathPaintedShaders;
    TStringPathBufferMap m_sourcePathBufferMap;
    QMutex m_pathBufferMutex;

    QScopedPointer<QSSGPathGeneratedShader> m_depthShader;
    QScopedPointer<QSSGPathGeneratedShader> m_depthDisplacementShader;

    QScopedPointer<QSSGPathXYGeneratedShader> m_paintedDepthShader;
    QScopedPointer<QSSGPathXYGeneratedShader> m_paintedShadowShader;
    QScopedPointer<QSSGPathXYGeneratedShader> m_paintedCubeShadowShader;
    QSSGRef<QSSGRenderInputAssembler> m_paintedRectInputAssembler;
    QSSGRef<QSSGRenderVertexBuffer> m_paintedRectVertexBuffer;
    QSSGRef<QSSGRenderIndexBuffer> m_paintedRectIndexBuffer;

    QVector<QSSGRef<QSSGRenderDepthStencilState>> m_depthStencilStates;

    QSSGRef<QSSGRenderPathSpecification> m_pathSpecification;
    QScopedPointer<QSSGPathUtilities::QSSGPathBufferBuilder> m_pathBuilder;

    QSSGPathManager(QSSGRenderContextInterface *ctx) : m_context(ctx) {}

    virtual ~QSSGPathManager() {
        m_paintedRectInputAssembler = nullptr;
        qDeleteAll(m_pathGeometryShaders);
        qDeleteAll(m_pathPaintedShaders);
    }

    // Called during binary load which is heavily threaded.
    void setPathSubPathData(const QSSGRenderSubPath &inPath, QSSGDataView<QSSGPathAnchorPoint> inPathCubicCurves) override
    {
        QMutexLocker locker(&m_pathBufferMutex);
        auto inserter = m_subPathBuffers.find(const_cast<QSSGRenderSubPath *>(&inPath));
        if (inserter == m_subPathBuffers.end()) {
            inserter = m_subPathBuffers.insert(const_cast<QSSGRenderSubPath *>(&inPath),
                                               QSSGRef<QSSGPathSubPathBuffer>(new QSSGPathSubPathBuffer(
                                                       const_cast<QSSGRenderSubPath &>(inPath))));
        }

        QSSGRef<QSSGPathSubPathBuffer> theBuffer = inserter.value();
        theBuffer->m_sourceData.clear();
        for (int i = 0; i < inPathCubicCurves.size(); ++i)
            theBuffer->m_sourceData.append(inPathCubicCurves[i]);
        theBuffer->m_flags |= QSSGPathDirtyFlagValue::SourceData;
    }

    QSSGRef<QSSGPathBuffer> getPathBufferObject(const QSSGRenderPath &inPath)
    {
        TPathBufferHash::iterator inserter = m_buffers.find(const_cast<QSSGRenderPath *>(&inPath));
        if (inserter == m_buffers.end())
            inserter = m_buffers.insert(const_cast<QSSGRenderPath *>(&inPath), QSSGRef<QSSGPathBuffer>(new QSSGPathBuffer()));

        return inserter.value();
    }

    QSSGRef<QSSGPathSubPathBuffer> getPathBufferObject(const QSSGRenderSubPath &inSubPath)
    {
        TPathSubPathBufferHash::iterator iter = m_subPathBuffers.find(const_cast<QSSGRenderSubPath *>(&inSubPath));
        if (iter != m_subPathBuffers.end())
            return iter.value();
        return nullptr;
    }

    QSSGDataRef<QSSGPathAnchorPoint> getPathSubPathBuffer(const QSSGRenderSubPath &inPath) override
    {
        QSSGRef<QSSGPathSubPathBuffer> theBuffer = getPathBufferObject(inPath);
        if (theBuffer)
            return toDataRef(theBuffer->m_sourceData.data(), quint32(theBuffer->m_sourceData.size()));
        return QSSGDataRef<QSSGPathAnchorPoint>();
    }

    QSSGDataRef<QSSGPathAnchorPoint> resizePathSubPathBuffer(const QSSGRenderSubPath &inPath, quint32 inNumAnchors) override
    {
        QSSGRef<QSSGPathSubPathBuffer> theBuffer = getPathBufferObject(inPath);
        if (theBuffer == nullptr)
            setPathSubPathData(inPath, QSSGDataView<QSSGPathAnchorPoint>());
        theBuffer = getPathBufferObject(inPath);
        Q_ASSERT(inNumAnchors > INT_MAX);
        theBuffer->m_sourceData.resize(int(inNumAnchors));
        theBuffer->m_flags |= QSSGPathDirtyFlagValue::SourceData;
        return toDataRef(theBuffer->m_sourceData.data(), quint32(theBuffer->m_sourceData.size()));
    }

    // This needs to be done using roots of the first derivative.
    QSSGBounds3 getBounds(const QSSGRenderPath &inPath) override
    {
        QSSGBounds3 retval(QSSGBounds3::empty());

        QSSGRef<QSSGPathBuffer> thePathBuffer = getPathBufferObject(inPath);
        if (thePathBuffer) {
            QSSGPathDirtyFlags geomDirtyFlags(QSSGPathDirtyFlagValue::SourceData | QSSGPathDirtyFlagValue::BeginTaper
                                                | QSSGPathDirtyFlagValue::EndTaper | QSSGPathDirtyFlagValue::Width
                                                | QSSGPathDirtyFlagValue::CPUError);

            if ((((quint32)thePathBuffer->m_flags) & (quint32)geomDirtyFlags) == 0) {
                return thePathBuffer->m_bounds;
            }
        }

        for (QSSGRenderSubPath *theSubPath = inPath.m_firstSubPath; theSubPath; theSubPath = theSubPath->m_nextSubPath) {
            QSSGRef<QSSGPathSubPathBuffer> theBuffer = getPathBufferObject(*theSubPath);
            if (!theBuffer)
                continue;

            int numAnchors = theBuffer->m_sourceData.size();
            for (int idx = 0, end = numAnchors; idx < end; ++idx) {
                const QSSGPathAnchorPoint &thePoint(theBuffer->m_sourceData[idx]);
                QVector2D position(thePoint.position);
                retval.include(QVector3D(position.x(), position.y(), 0.0f));
                if (idx) {
                    QVector2D incoming(QSSGPathManagerInterface::getControlPointFromAngleDistance(thePoint.position,
                                                                                                    thePoint.incomingAngle,
                                                                                                    thePoint.incomingDistance));
                    retval.include(QVector3D(incoming.x(), incoming.y(), 0.0f));
                }

                if (idx < (numAnchors - 1)) {
                    QVector2D outgoing(QSSGPathManagerInterface::getControlPointFromAngleDistance(thePoint.position,
                                                                                                    thePoint.outgoingAngle,
                                                                                                    thePoint.outgoingDistance));
                    retval.include(QVector3D(outgoing.x(), outgoing.y(), 0.0f));
                }
            }
        }

        return retval;
    }

    // find a point that will join these two curves *if* they are not first derivative continuous
    static QSSGOption<QVector2D> getAdjoiningPoint(QVector2D prevC2, QVector2D point, QVector2D C1, float pathWidth)
    {
        QVector2D incomingDxDy = (point - prevC2);
        QVector2D outgoingDxDy = (C1 - point);
        incomingDxDy.normalize();
        outgoingDxDy.normalize();
        float determinant = (incomingDxDy.x() * outgoingDxDy.y()) - (incomingDxDy.y() * outgoingDxDy.x());
        if (std::fabs(determinant) > .001f) {
            float mult = determinant > 0.0f ? 1.0f : -1.0f;
            QVector2D incomingNormal(incomingDxDy.y(), -incomingDxDy.x());
            QVector2D outgoingNormal(outgoingDxDy.y(), -outgoingDxDy.x());

            QVector2D leftEdge = point + mult * incomingNormal * pathWidth;
            QVector2D rightEdge = point + mult * outgoingNormal * pathWidth;

            return (leftEdge + rightEdge) / 2.0f;
        }
        return QSSGEmpty();
    }

    QSSGOption<QPair<quint32, float>> findBreakEquation(float inTaperStart)
    {
        float lengthTotal = 0;
        for (int idx = 0, end = m_subdivResult.size(); idx < end; ++idx) {
            if (lengthTotal + m_subdivResult[idx].m_length > inTaperStart) {
                float breakTValue = (inTaperStart - lengthTotal) / m_subdivResult[idx].m_length;
                QVector<QSSGResultCubic>::iterator breakIter = m_subdivResult.begin() + idx;
                QSSGCubicBezierCurve theCurve(breakIter->m_p1, breakIter->m_c1, breakIter->m_c2, breakIter->m_p2);
                QPair<QSSGCubicBezierCurve, QSSGCubicBezierCurve> subdivCurve = theCurve.splitCubicBezierCurve(breakTValue);
                float originalBreakT = breakIter->m_tStart + (breakIter->m_tStop - breakIter->m_tStart) * breakTValue;
                // Update the existing item to point to the second equation
                breakIter->m_p1 = subdivCurve.second.m_points[0];
                breakIter->m_c1 = subdivCurve.second.m_points[1];
                breakIter->m_c2 = subdivCurve.second.m_points[2];
                breakIter->m_p2 = subdivCurve.second.m_points[3];
                float originalLength = breakIter->m_length;
                float originalStart = breakIter->m_tStart;
                breakIter->m_length *= (1.0f - breakTValue);
                breakIter->m_tStart = originalBreakT;
                QSSGResultCubic newCubic(subdivCurve.first.m_points[0],
                                           subdivCurve.first.m_points[1],
                                           subdivCurve.first.m_points[2],
                                           subdivCurve.first.m_points[3],
                                           breakIter->m_equationIndex,
                                           originalStart,
                                           originalBreakT,
                                           originalLength * breakTValue);

                m_subdivResult.insert(breakIter, newCubic);
                return QPair<quint32, float>(quint32(idx), breakTValue);
            }
            lengthTotal += m_subdivResult[idx].m_length;
        }
        return QSSGEmpty();
    }

    bool prepareGeometryPathForRender(const QSSGRenderPath &inPath, QSSGPathBuffer &inPathBuffer)
    {

        m_subdivResult.clear();
        m_keyPointVec.clear();
        const QSSGRenderPath &thePath(inPath);

        inPathBuffer.setBeginTaperInfo(thePath.m_beginCapping, thePath.m_beginCapOffset, thePath.m_beginCapOpacity, thePath.m_beginCapWidth);
        inPathBuffer.setEndTaperInfo(thePath.m_endCapping, thePath.m_endCapOffset, thePath.m_endCapOpacity, thePath.m_endCapWidth);
        inPathBuffer.setWidth(inPath.m_width);
        inPathBuffer.setCPUError(inPath.m_linearError);

        QSSGPathDirtyFlags geomDirtyFlags(QSSGPathDirtyFlagValue::SourceData | QSSGPathDirtyFlagValue::BeginTaper | QSSGPathDirtyFlagValue::EndTaper
                                            | QSSGPathDirtyFlagValue::Width | QSSGPathDirtyFlagValue::CPUError);

        bool retval = false;
        if (!inPathBuffer.m_patchData || (((quint32)inPathBuffer.m_flags) & (quint32)geomDirtyFlags) != 0) {
            QSSGPathUtilities::QSSGPathBuffer thePathData = inPathBuffer.getPathData(*m_pathBuilder);

            int dataIdx = 0;
            QVector2D prevPoint(0, 0);
            int equationIdx = 0;
            for (int commandIdx = 0, commandEnd = thePathData.commands.size(); commandIdx < commandEnd; ++commandIdx) {
                switch (thePathData.commands[commandIdx]) {
                case QSSGPathUtilities::PathCommand::MoveTo:
                    prevPoint = QVector2D(thePathData.data[dataIdx], thePathData.data[dataIdx + 1]);
                    dataIdx += 2;
                    break;
                case QSSGPathUtilities::PathCommand::CubicCurveTo: {
                    QVector2D c1(thePathData.data[dataIdx], thePathData.data[dataIdx + 1]);
                    dataIdx += 2;
                    QVector2D c2(thePathData.data[dataIdx], thePathData.data[dataIdx + 1]);
                    dataIdx += 2;
                    QVector2D p2(thePathData.data[dataIdx], thePathData.data[dataIdx + 1]);
                    dataIdx += 2;
                    outerAdaptiveSubdivideBezierCurve(m_subdivResult,
                                                      m_keyPointVec,
                                                      QSSGCubicBezierCurve(prevPoint, c1, c2, p2),
                                                      qMax(inPath.m_linearError, 1.0f),
                                                      quint32(equationIdx));
                    ++equationIdx;
                    prevPoint = p2;
                } break;
                case QSSGPathUtilities::PathCommand::Close:
                    break;

                default:
                    Q_ASSERT(false);
                    break;
                }
            }

            float theLocalWidth = inPath.m_width / 2.0f;

            QVector2D theBeginTaperData(theLocalWidth, thePath.globalOpacity);
            QVector2D theEndTaperData(theLocalWidth, thePath.globalOpacity);

            float pathLength = 0.0f;
            for (int idx = 0, end = m_subdivResult.size(); idx < end; ++idx)
                pathLength += m_subdivResult.at(idx).m_length;

            if (thePath.m_beginCapping == QSSGRenderPath::Capping::Taper || thePath.m_endCapping == QSSGRenderPath::Capping::Taper) {
                float maxTaperStart = pathLength / 2.0f;
                if (thePath.m_beginCapping == QSSGRenderPath::Capping::Taper) {
                    // Can't start more than halfway across the path.
                    float taperStart = qMin(thePath.m_beginCapOffset, maxTaperStart);
                    float endTaperWidth = thePath.m_beginCapWidth;
                    float endTaperOpacity = thePath.globalOpacity * thePath.m_beginCapOpacity;
                    theBeginTaperData = QVector2D(endTaperWidth, endTaperOpacity);
                    // Find where we need to break the current equations.
                    QSSGOption<QPair<quint32, float>> breakEquationAndT(findBreakEquation(taperStart));
                    if (breakEquationAndT.hasValue()) {
                        quint32 breakEquation = breakEquationAndT->first;

                        float lengthTotal = 0;
                        for (int idx = 0, end = int(breakEquation); idx <= end; ++idx) {
                            QSSGResultCubic &theCubic = m_subdivResult[idx];
                            theCubic.m_mode = QSSGResultCubic::BeginTaper;

                            theCubic.m_taperMultiplier[0] = lengthTotal / taperStart;
                            lengthTotal += theCubic.m_length;
                            theCubic.m_taperMultiplier[1] = lengthTotal / taperStart;
                        }
                    }
                }
                if (thePath.m_endCapping == QSSGRenderPath::Capping::Taper) {
                    float taperStart = qMin(thePath.m_endCapOffset, maxTaperStart);
                    float endTaperWidth = thePath.m_endCapWidth;
                    float endTaperOpacity = thePath.globalOpacity * thePath.m_endCapOpacity;
                    theEndTaperData = QVector2D(endTaperWidth, endTaperOpacity);
                    // Invert taper start so that the forward search works.
                    QSSGOption<QPair<quint32, float>> breakEquationAndT(findBreakEquation(pathLength - taperStart));

                    if (breakEquationAndT.hasValue()) {
                        quint32 breakEquation = breakEquationAndT->first;
                        ++breakEquation;

                        float lengthTotal = 0;
                        for (int idx = int(breakEquation), end = m_subdivResult.size(); idx < end; ++idx) {
                            QSSGResultCubic &theCubic = m_subdivResult[idx];
                            theCubic.m_mode = QSSGResultCubic::EndTaper;

                            theCubic.m_taperMultiplier[0] = 1.0f - (lengthTotal / taperStart);
                            lengthTotal += theCubic.m_length;
                            theCubic.m_taperMultiplier[1] = 1.0f - (lengthTotal / taperStart);
                        }
                    }
                }
            }

            const QSSGRef<QSSGRenderContext> &theRenderContext(m_context->renderContext());
            // Create quads out of each point.
            if (m_subdivResult.empty())
                return false;

            // Generate patches.
            m_patchBuffer.clear();
            float pathWidth = thePath.m_width / 2.0f;
            // texture coords
            float texCoordU = 0.0;

            for (int idx = 0, end = m_subdivResult.size(); idx < end; ++idx) {
                // create patches
                QSSGResultCubic thePoint(m_subdivResult[idx]);

                m_patchBuffer.push_back(createVec4(thePoint.m_p1, thePoint.m_c1));
                m_patchBuffer.push_back(createVec4(thePoint.m_c2, thePoint.m_p2));

                // Now we need to take care of cases where the control points of the adjoining
                // SubPaths
                // do not line up; i.e. there is a discontinuity of the 1st derivative
                // The simplest way to do this is to move the edge vertex to a halfway point
                // between a line bisecting the two control lines
                QVector2D incomingAdjoining(thePoint.m_p1);
                QVector2D outgoingAdjoining(thePoint.m_p2);
                if (idx) {
                    QSSGResultCubic previousCurve = m_subdivResult[idx - 1];
                    if (previousCurve.m_equationIndex != thePoint.m_equationIndex) {
                        float anchorWidth = thePoint.getP1Width(pathWidth, theBeginTaperData.x(), theEndTaperData.x());
                        QSSGOption<QVector2D> adjoining = getAdjoiningPoint(previousCurve.m_c2, thePoint.m_p1, thePoint.m_c1, anchorWidth);
                        if (adjoining.hasValue())
                            incomingAdjoining = *adjoining;
                    }
                }
                if (idx < (end - 1)) {
                    QSSGResultCubic nextCurve = m_subdivResult[idx + 1];
                    if (nextCurve.m_equationIndex != thePoint.m_equationIndex) {
                        float anchorWidth = thePoint.getP2Width(pathWidth, theBeginTaperData.x(), theEndTaperData.x());
                        QSSGOption<QVector2D> adjoining = getAdjoiningPoint(thePoint.m_c2, thePoint.m_p2, nextCurve.m_c1, anchorWidth);
                        if (adjoining.hasValue())
                            outgoingAdjoining = *adjoining;
                    }
                }
                m_patchBuffer.push_back(createVec4(incomingAdjoining, outgoingAdjoining));

                QVector4D taperData(0.0f, 0.0f, 0.0f, 0.0f);
                taperData.setX(thePoint.m_taperMultiplier.x());
                taperData.setY(thePoint.m_taperMultiplier.y());
                // Note we could put a *lot* more data into this thing.
                taperData.setZ(float(thePoint.m_mode));
                m_patchBuffer.push_back(taperData);

                // texture coord generation
                // note we only generate u here. v is generated in the tess shader
                // u coord for P1 and C1
                QVector2D udata(texCoordU, texCoordU + (thePoint.m_length / pathLength));
                texCoordU = udata.y();
                m_patchBuffer.push_back(QVector4D(udata.x(), udata.y(), 0.0, 0.0));
            }

            // buffer size is 3.0*4.0*bufSize
            quint32 bufSize = quint32(m_patchBuffer.size()) * sizeof(QVector4D);
            quint32 stride = sizeof(QVector4D);

            if ((!inPathBuffer.m_patchData) || inPathBuffer.m_patchData->size() < bufSize) {
                inPathBuffer.m_patchData = new QSSGRenderVertexBuffer(theRenderContext, QSSGRenderBufferUsageType::Dynamic,
                                                                        stride,
                                                                        toByteView(m_patchBuffer));
                inPathBuffer.m_numVertexes = quint32(m_patchBuffer.size());
                inPathBuffer.m_inputAssembler = nullptr;
            } else {
                Q_ASSERT(inPathBuffer.m_patchData->size() >= bufSize);
                inPathBuffer.m_patchData->updateBuffer(toByteView(m_patchBuffer));
            }

            if (!inPathBuffer.m_inputAssembler) {
                QSSGRenderVertexBufferEntry theEntries[] = {
                    QSSGRenderVertexBufferEntry("attr_pos", QSSGRenderComponentType::Float32, 4),
                };

                QSSGRenderDrawMode primType = QSSGRenderDrawMode::Patches;

                QSSGRef<QSSGRenderAttribLayout> theLayout = theRenderContext->createAttributeLayout(toDataView(theEntries, 1));
                // How many vertices the TCS shader has access to in order to produce its output
                // array of vertices.
                const quint32 inputPatchVertexCount = 5;
                inPathBuffer.m_inputAssembler = theRenderContext->createInputAssembler(theLayout,
                                                                                       toDataView(inPathBuffer.m_patchData),
                                                                                       nullptr,
                                                                                       toDataView(stride),
                                                                                       toDataView(quint32(0)),
                                                                                       primType,
                                                                                       inputPatchVertexCount);
            }
            inPathBuffer.m_beginTaperData = theBeginTaperData;
            inPathBuffer.m_endTaperData = theEndTaperData;

            // cache bounds
            QSSGBounds3 bounds = getBounds(inPath);
            inPathBuffer.m_bounds.minimum = bounds.minimum;
            inPathBuffer.m_bounds.maximum = bounds.maximum;
        }

        return retval;
    }

    QSSGRef<QSSGMaterialShaderGeneratorInterface> getMaterialShaderGenertator(QSSGPathRenderContext &inRenderContext) const
    {
        const bool isDefaultMaterial = (inRenderContext.material.type == QSSGRenderGraphObject::Type::DefaultMaterial);
        if (isDefaultMaterial)
            return m_context->defaultMaterialShaderGenerator();

        return m_context->customMaterialShaderGenerator();
    }

    QByteArray getMaterialNameForKey(QSSGPathRenderContext &inRenderContext)
    {
        bool isDefaultMaterial = (inRenderContext.material.type == QSSGRenderGraphObject::Type::DefaultMaterial);

        if (!isDefaultMaterial) {
            QSSGRef<QSSGMaterialSystem> theMaterialSystem(m_context->customMaterialSystem());
            const QSSGRenderCustomMaterial &theCustomMaterial(
                    reinterpret_cast<const QSSGRenderCustomMaterial &>(inRenderContext.material));

            return theMaterialSystem->getShaderName(theCustomMaterial);
        }

        return QByteArray();
    }

    bool preparePaintedPathForRender(const QSSGRenderPath &inPath, QSSGPathBuffer &inPathBuffer)
    {
        const QSSGRef<QSSGRenderContext> &theContext(this->m_context->renderContext());
        if (!inPathBuffer.m_pathRender || (inPathBuffer.m_flags & QSSGPathDirtyFlagValue::SourceData)) {
            if (!inPathBuffer.m_pathRender) {
                inPathBuffer.m_pathRender = theContext->createPathRender();
            }

            if (inPathBuffer.m_pathRender == nullptr || m_pathSpecification == nullptr) {
                //	Q_ASSERT( false );
                return false;
            }

            m_pathSpecification->reset();
            QSSGPathUtilities::QSSGPathBuffer thePathData = inPathBuffer.getPathData(*m_pathBuilder);

            int dataIdx = 0;
            for (int commandIdx = 0, commandEnd = thePathData.commands.size(); commandIdx < commandEnd; ++commandIdx) {

                switch (thePathData.commands[commandIdx]) {
                case QSSGPathUtilities::PathCommand::MoveTo:
                    m_pathSpecification->moveTo(QVector2D(thePathData.data[dataIdx], thePathData.data[dataIdx + 1]));
                    dataIdx += 2;
                    break;
                case QSSGPathUtilities::PathCommand::CubicCurveTo: {
                    QVector2D c1(thePathData.data[dataIdx], thePathData.data[dataIdx + 1]);
                    dataIdx += 2;
                    QVector2D c2(thePathData.data[dataIdx], thePathData.data[dataIdx + 1]);
                    dataIdx += 2;
                    QVector2D p2(thePathData.data[dataIdx], thePathData.data[dataIdx + 1]);
                    dataIdx += 2;
                    m_pathSpecification->cubicCurveTo(c1, c2, p2);
                } break;
                case QSSGPathUtilities::PathCommand::Close:
                    m_pathSpecification->closePath();
                    break;
                default:
                    Q_ASSERT(false);
                    break;
                }
            }

            inPathBuffer.m_pathRender->setPathSpecification(m_pathSpecification);

            // cache bounds
            QSSGBounds3 bounds = getBounds(inPath);
            inPathBuffer.m_bounds.minimum = bounds.minimum;
            inPathBuffer.m_bounds.maximum = bounds.maximum;

            return true;
        }

        return false;
    }

    bool prepareForRender(const QSSGRenderPath &inPath) override
    {
        QSSGRef<QSSGPathBuffer> thePathBuffer = getPathBufferObject(inPath);
        if (!thePathBuffer) {
            return false;
        }
        const QSSGRef<QSSGRenderContext> &theContext(this->m_context->renderContext());
        if (!m_pathSpecification)
            m_pathSpecification = theContext->createPathSpecification();
        if (!m_pathSpecification)
            return false;
        if (!m_pathBuilder)
            m_pathBuilder.reset(new QSSGPathUtilities::QSSGPathBufferBuilder());

        thePathBuffer->setPathType(inPath.m_pathType);
        bool retval = false;
        if (inPath.m_pathBuffer.isEmpty()) {
            thePathBuffer->m_pathBuffer = nullptr;
            // Ensure the SubPath list is identical and clear, percolating any dirty flags up to the
            // path buffer.
            int subPathIdx = 0;
            for (const QSSGRenderSubPath *theSubPath = inPath.m_firstSubPath; theSubPath;
                 theSubPath = theSubPath->m_nextSubPath, ++subPathIdx) {
                QSSGRef<QSSGPathSubPathBuffer> theSubPathBuffer = getPathBufferObject(*theSubPath);
                if (theSubPathBuffer == nullptr)
                    continue;
                thePathBuffer->m_flags |= theSubPathBuffer->m_flags;

                if (theSubPathBuffer->m_closed != theSubPath->m_closed) {
                    thePathBuffer->m_flags |= QSSGPathDirtyFlagValue::SourceData;
                    theSubPathBuffer->m_closed = theSubPath->m_closed;
                }

                if (thePathBuffer->m_subPaths.size() <= subPathIdx || thePathBuffer->m_subPaths[subPathIdx] != theSubPathBuffer) {
                    thePathBuffer->m_flags |= QSSGPathDirtyFlagValue::SourceData;
                    if (thePathBuffer->m_subPaths.size() <= subPathIdx)
                        thePathBuffer->m_subPaths.push_back(theSubPathBuffer);
                    else
                        thePathBuffer->m_subPaths[subPathIdx] = theSubPathBuffer;
                }

                theSubPathBuffer->m_flags = QSSGPathDirtyFlags();
            }

            if (subPathIdx != thePathBuffer->m_subPaths.size()) {
                thePathBuffer->m_subPaths.resize(subPathIdx);
                thePathBuffer->m_flags |= QSSGPathDirtyFlagValue::SourceData;
            }
        } else {
            thePathBuffer->m_subPaths.clear();
            TStringPathBufferMap::iterator inserter = m_sourcePathBufferMap.find(inPath.m_pathBuffer);
            //            QPair<TStringPathBufferMap::iterator, bool> inserter =
            //                    m_SourcePathBufferMap.insert(inPath.m_PathBuffer, TPathBufferPtr());
            if (inserter == m_sourcePathBufferMap.end()) {
                QSharedPointer<QIODevice> theStream = m_context->inputStreamFactory()->getStreamForFile(inPath.m_pathBuffer);
                if (theStream) {
                    QSSGPathUtilities::QSSGPathBuffer *theNewBuffer = QSSGPathUtilities::QSSGPathBuffer::load(*theStream);
                    if (theNewBuffer)
                        inserter = m_sourcePathBufferMap.insert(inPath.m_pathBuffer,
                                                                QSSGRef<QSSGImportPathWrapper>(
                                                                        new QSSGImportPathWrapper(*theNewBuffer)));
                }
            }
            if (thePathBuffer->m_pathBuffer != inserter.value()) {
                thePathBuffer->m_pathBuffer = inserter.value();
                thePathBuffer->m_flags |= QSSGPathDirtyFlagValue::SourceData;
            }
        }

        if (inPath.m_pathType == QSSGRenderPath::PathType::Geometry)
            retval = prepareGeometryPathForRender(inPath, *thePathBuffer);
        else
            retval = preparePaintedPathForRender(inPath, *thePathBuffer);
        thePathBuffer->m_flags = QSSGPathDirtyFlags();
        return retval;
    }

    void setMaterialProperties(const QSSGRef<QSSGRenderShaderProgram> &inShader,
                               QSSGPathRenderContext &inRenderContext,
                               QSSGLayerGlobalRenderProperties &inRenderProperties)
    {
        const QSSGRef<QSSGMaterialShaderGeneratorInterface> &theMaterialGenerator = getMaterialShaderGenertator(inRenderContext);
        const QSSGRef<QSSGRenderContext> &theRenderContext(m_context->renderContext());
        theRenderContext->setActiveShader(inShader);

        theMaterialGenerator->setMaterialProperties(inShader,
                                                    inRenderContext.material,
                                                    inRenderContext.cameraVec,
                                                    inRenderContext.mvp,
                                                    inRenderContext.normalMatrix,
                                                    inRenderContext.path.globalTransform,
                                                    inRenderContext.firstImage,
                                                    inRenderContext.opacity,
                                                    inRenderProperties);
    }

    void doRenderGeometryPath(QSSGPathGeneratedShader *inShader,
                              QSSGPathRenderContext &inRenderContext,
                              QSSGLayerGlobalRenderProperties &inRenderProperties,
                              const QSSGRef<QSSGPathBuffer> &inPathBuffer)
    {
        if (inPathBuffer->m_inputAssembler == nullptr)
            return;

        setMaterialProperties(inShader->m_shader, inRenderContext, inRenderProperties);
        const QSSGRef<QSSGRenderContext> &theRenderContext(m_context->renderContext());

        inShader->m_beginTaperData.set(inPathBuffer->m_beginTaperData);
        inShader->m_endTaperData.set(inPathBuffer->m_endTaperData);
        if (inRenderContext.enableWireframe) {
            // we need the viewport matrix
            QRect theViewport(theRenderContext->viewport());
            QMatrix4x4 vpMatrix = { (float)theViewport.width() / 2.0f,
                                    0.0,
                                    0.0,
                                    0.0,
                                    0.0,
                                    (float)theViewport.height() / 2.0f,
                                    0.0,
                                    0.0,
                                    0.0,
                                    0.0,
                                    1.0,
                                    0.0,
                                    (float)theViewport.width() / 2.0f + (float)theViewport.x(),
                                    (float)theViewport.height() / 2.0f + (float)theViewport.y(),
                                    0.0,
                                    1.0 };
            inShader->m_wireframeViewMatrix.set(vpMatrix);
        }

        float tessEdgeValue = qMin(64.0f, qMax(1.0f, inRenderContext.path.m_edgeTessAmount));
        float tessInnerValue = qMin(64.0f, qMax(1.0f, inRenderContext.path.m_innerTessAmount));
        inShader->m_edgeTessAmount.set(tessEdgeValue);
        inShader->m_innerTessAmount.set(tessInnerValue);
        inShader->m_width.set(inRenderContext.path.m_width / 2.0f);
        theRenderContext->setInputAssembler(inPathBuffer->m_inputAssembler);
        theRenderContext->setCullingEnabled(false);
        QSSGRenderDrawMode primType = QSSGRenderDrawMode::Patches;
        theRenderContext->draw(primType, (quint32)inPathBuffer->m_numVertexes, 0);
    }

    QSSGRef<QSSGRenderDepthStencilState> getDepthStencilState()
    {
        const QSSGRef<QSSGRenderContext> &theRenderContext(m_context->renderContext());
        QSSGRenderBoolOp theDepthFunction = theRenderContext->depthFunction();
        bool isDepthEnabled = theRenderContext->isDepthTestEnabled();
        bool isStencilEnabled = theRenderContext->isStencilTestEnabled();
        bool isDepthWriteEnabled = theRenderContext->isDepthWriteEnabled();
        for (int idx = 0, end = m_depthStencilStates.size(); idx < end; ++idx) {
            const QSSGRef<QSSGRenderDepthStencilState> &theState = m_depthStencilStates.at(idx);
            if (theState->depthFunction() == theDepthFunction && theState->depthEnabled() == isDepthEnabled
                && theState->depthMask() == isDepthWriteEnabled)
                return theState;
        }
        QSSGRenderStencilFunction theArg(QSSGRenderBoolOp::NotEqual, 0, 0xFF);
        QSSGRenderStencilOperation theOpArg(QSSGRenderStencilOp::Keep, QSSGRenderStencilOp::Keep, QSSGRenderStencilOp::Zero);
        m_depthStencilStates.push_back(
                new QSSGRenderDepthStencilState(theRenderContext, isDepthEnabled, isDepthWriteEnabled,
                                                      theDepthFunction, isStencilEnabled, theArg, theArg, theOpArg, theOpArg));
        return m_depthStencilStates.back();
    }

    static void doSetCorrectiveScale(const QMatrix4x4 &mvp, QMatrix4x4 &outScale, QSSGBounds3 pathBounds)
    {
        // Compute the projected locations for the paraboloid and regular projection
        // and thereby set the appropriate scaling factor.
        QVector3D points[4];
        QVector3D projReg[4], projParab[4];
        points[0] = pathBounds.minimum;
        points[1] = QVector3D(pathBounds.maximum.x(), pathBounds.minimum.y(), pathBounds.minimum.z());
        points[2] = pathBounds.maximum;
        points[3] = QVector3D(pathBounds.minimum.x(), pathBounds.maximum.y(), pathBounds.maximum.z());

        // Do the two different projections.
        for (int i = 0; i < 4; ++i) {
            QVector4D tmp;
            tmp = mat44::transform(mvp, QVector4D(points[i], 1.0f));
            tmp /= tmp.w();
            QVector3D tmp3d(tmp.x(), tmp.y(), tmp.z());
            projReg[i] = tmp3d;
            projParab[i] = tmp3d.normalized();
            projParab[i] /= projParab[i].z() + 1.0f;
        }

        QSSGBounds3 boundsA, boundsB;
        for (int i = 0; i < 4; ++i) {
            boundsA.include(projReg[i]);
            boundsB.include(projParab[i]);
        }
        float xscale = (boundsB.maximum.x() - boundsB.minimum.x()) / (boundsA.maximum.x() - boundsA.minimum.x());
        float yscale = (boundsB.maximum.y() - boundsB.minimum.y()) / (boundsA.maximum.y() - boundsA.minimum.y());
        float zscale = vec3::magnitudeSquared(boundsB.maximum - boundsB.minimum)
                / vec3::magnitudeSquared(boundsA.maximum - boundsA.minimum);
        // The default minimum here is just a stupid figure that looks good on our content because
        // we'd
        // been using it for a little while before.  Just for demo.
        xscale = qMin<float>(0.5333333f, qMin<float>(xscale, yscale));
        yscale = qMin<float>(0.5333333f, qMin<float>(xscale, yscale));
        outScale.scale(xscale, yscale, zscale);
    }

    void doRenderPaintedPath(QSSGPathXYGeneratedShader *inShader,
                             QSSGPathRenderContext &inRenderContext,
                             QSSGLayerGlobalRenderProperties &inRenderProperties,
                             const QSSGRef<QSSGPathBuffer> &inPathBuffer,
                             bool isParaboloidPass = false)
    {
        if (!inPathBuffer->m_pathRender)
            return;
        const QSSGRef<QSSGRenderContext> &theRenderContext(m_context->renderContext());
        if (!m_paintedRectInputAssembler) {
            const QVector2D vertexes[] = {
                QVector2D(0.0, 0.0),
                QVector2D(1.0, 0.0),
                QVector2D(1.0, 1.0),
                QVector2D(0.0, 1.0),
            };

            const quint8 indexes[] = {
                0, 1, 2, 2, 3, 0,
            };

            quint32 stride = sizeof(QVector2D);

            QSSGRenderVertexBufferEntry theBufferEntries[] = {
                QSSGRenderVertexBufferEntry("attr_pos", QSSGRenderComponentType::Float32, 2, 0)
            };

            m_paintedRectVertexBuffer = new QSSGRenderVertexBuffer(theRenderContext, QSSGRenderBufferUsageType::Static,
                                                                     sizeof(QVector2D),
                                                                     toByteView(vertexes, 4));
            m_paintedRectIndexBuffer = new QSSGRenderIndexBuffer(theRenderContext,
                                                                   QSSGRenderBufferUsageType::Static,
                                                                   QSSGRenderComponentType::UnsignedInteger8,
                                                                   toByteView(indexes, 6));
            QSSGRef<QSSGRenderAttribLayout> theAttribLayout = theRenderContext->createAttributeLayout(
                    toDataView(theBufferEntries, 1));
            m_paintedRectInputAssembler = theRenderContext->createInputAssembler(theAttribLayout,
                                                                                 toDataView(m_paintedRectVertexBuffer),
                                                                                 m_paintedRectIndexBuffer,
                                                                                 toDataView(stride),
                                                                                 toDataView(quint32(0)),
                                                                                 QSSGRenderDrawMode::Triangles);
        }

        // our current render target needs stencil
        Q_ASSERT(theRenderContext->stencilBits() > 0);

        theRenderContext->setDepthStencilState(getDepthStencilState());

        // http://developer.download.nvidia.com/assets/gamedev/files/Mixing_Path_Rendering_and_3D.pdf
        theRenderContext->setPathStencilDepthOffset(-.05f, -1.0f);

        // Stencil out the geometry.
        QMatrix4x4 pathMdlView;
        // Why is this happening?  Well, it's because the painted-on path rendering is always
        // a flat splatted 2D object.  This is bad because a paraboloid projection demands a very
        // different
        // non-linear space into which we must draw.  Path Rendering does not allow this sort of
        // spatial
        // warping internally, and all we end up passing in as a simple perspective projection.
        // So for the fix, I'm scaling the actual "object" size so that it fits into the correctly
        // projected
        // polygon inside the paraboloid depth pass.  Obviously, this scaling factor is wrong, and
        // not generic
        // enough to cover cases like polygons covering a large spread of the FOV and so on.  It's
        // really
        // just a filthy awful, morally deplorable HACK.  But it's basically the quickest fix at
        // hand.
        // This is also about the only possible approach that *could* work short of rendering the
        // paths in
        // a render-to-texture pass and splatting that texture on a sufficiently tessellated quad.
        // Unless
        // there's a way to program NVPR's internal projection scheme, that is.
        // Geometry-based paths will work out better, I think, because they're actually creating
        // geometry.
        // This is essentially a 2D painting process inside a quad where the actual rendered region
        // isn't
        // exactly where NVPR thinks it should be because they're not projecting points the same
        // way.
        if (isParaboloidPass) {
            doSetCorrectiveScale(inRenderContext.mvp, pathMdlView, inPathBuffer->m_pathRender->getPathObjectStrokeBox());
        }

        bool isStencilEnabled = theRenderContext->isStencilTestEnabled();
        theRenderContext->setStencilTestEnabled(true);
        theRenderContext->setPathProjectionMatrix(inRenderContext.mvp);
        theRenderContext->setPathModelViewMatrix(pathMdlView);

        if (inRenderContext.isStroke) {
            inPathBuffer->m_pathRender->setStrokeWidth(inRenderContext.path.m_width);
            inPathBuffer->m_pathRender->stencilStroke();
        } else
            inPathBuffer->m_pathRender->stencilFill();

        // The stencil buffer will dictate whether this object renders or not.  So we need to ignore
        // the depth test result.
        QSSGRenderBoolOp theDepthFunc = theRenderContext->depthFunction();
        theRenderContext->setDepthFunction(QSSGRenderBoolOp::AlwaysTrue);
        // Now render the path; this resets the stencil buffer.
        setMaterialProperties(inShader->m_shader, inRenderContext, inRenderProperties);
        QSSGBounds3 rectBounds = inPathBuffer->m_pathRender->getPathObjectStrokeBox();
        if (isParaboloidPass) {
            rectBounds.scale(1.570796326795f);
        } // PKC : More of the same ugly hack.
        inShader->m_rectDimensions.set(
                QVector4D(rectBounds.minimum.x(), rectBounds.minimum.y(), rectBounds.maximum.x(), rectBounds.maximum.y()));
        theRenderContext->setInputAssembler(m_paintedRectInputAssembler);
        theRenderContext->setCullingEnabled(false);
        // Render exactly two triangles
        theRenderContext->draw(QSSGRenderDrawMode::Triangles, 6, 0);
        theRenderContext->setStencilTestEnabled(isStencilEnabled);
        theRenderContext->setDepthFunction(theDepthFunc);
    }

    void renderDepthPrepass(QSSGPathRenderContext &inRenderContext,
                            QSSGLayerGlobalRenderProperties inRenderProperties,
                            TShaderFeatureSet inFeatureSet) override
    {
        QSSGRef<QSSGPathBuffer> thePathBuffer = getPathBufferObject(inRenderContext.path);
        if (!thePathBuffer)
            return;

        if (thePathBuffer->m_pathType == QSSGRenderPath::PathType::Geometry) {
            quint32 displacementIdx = 0;
            quint32 imageIdx = 0;
            QSSGRenderableImage *displacementImage = nullptr;

            for (QSSGRenderableImage *theImage = inRenderContext.firstImage; theImage != nullptr && displacementImage == nullptr;
                 theImage = theImage->m_nextImage, ++imageIdx) {
                if (theImage->m_mapType == QSSGImageMapTypes::Displacement) {
                    displacementIdx = imageIdx;
                    displacementImage = theImage;
                }
            }

            QScopedPointer<QSSGPathGeneratedShader> &theDesiredDepthShader = displacementImage == nullptr ? m_depthShader : m_depthDisplacementShader;

            if (!theDesiredDepthShader) {
                QSSGRef<QSSGDefaultMaterialShaderGeneratorInterface> theMaterialGenerator(
                        m_context->defaultMaterialShaderGenerator());
                QSSGPathVertexPipeline thePipeline(m_context->shaderProgramGenerator(), theMaterialGenerator, false);
                thePipeline.beginVertexGeneration(displacementIdx, displacementImage);
                thePipeline.beginFragmentGeneration();
                thePipeline.fragment().append("\tfragOutput = vec4(1.0, 1.0, 1.0, 1.0);");
                thePipeline.endVertexGeneration(false);
                thePipeline.endFragmentGeneration(false);
                const char *shaderName = "path depth";
                if (displacementImage)
                    shaderName = "path depth displacement";

                QSSGShaderCacheProgramFlags theFlags;
                const QSSGRef<QSSGRenderShaderProgram> &theProgram = thePipeline.programGenerator()->compileGeneratedShader(shaderName, theFlags, inFeatureSet);
                if (theProgram)
                    theDesiredDepthShader.reset(new QSSGPathGeneratedShader(theProgram));
            }
            if (theDesiredDepthShader)
                doRenderGeometryPath(theDesiredDepthShader.get(), inRenderContext, inRenderProperties, thePathBuffer);
        } else {
            // painted path, go stroke route for now.
            if (!m_paintedDepthShader) {
                QSSGRef<QSSGDefaultMaterialShaderGeneratorInterface> theMaterialGenerator(
                        m_context->defaultMaterialShaderGenerator());
                QSSGXYRectVertexPipeline thePipeline(m_context->shaderProgramGenerator(), theMaterialGenerator);
                thePipeline.beginVertexGeneration(0, nullptr);
                thePipeline.beginFragmentGeneration();
                thePipeline.fragment().append("\tfragOutput = vec4(1.0, 1.0, 1.0, 1.0);");
                thePipeline.endVertexGeneration(false);
                thePipeline.endFragmentGeneration(false);
                QSSGShaderCacheProgramFlags theFlags;
                const QSSGRef<QSSGRenderShaderProgram> &theProgram = thePipeline.programGenerator()->compileGeneratedShader("path painted depth",
                                                                                                                                theFlags,
                                                                                                                                inFeatureSet);
                if (theProgram)
                    m_paintedDepthShader.reset(new QSSGPathXYGeneratedShader(theProgram));
            }
            if (m_paintedDepthShader)
                doRenderPaintedPath(m_paintedDepthShader.get(), inRenderContext, inRenderProperties, thePathBuffer);
        }
    }

    void renderShadowMapPass(QSSGPathRenderContext &inRenderContext,
                             QSSGLayerGlobalRenderProperties inRenderProperties,
                             TShaderFeatureSet inFeatureSet) override
    {
        QSSGRef<QSSGPathBuffer> thePathBuffer = getPathBufferObject(inRenderContext.path);
        if (!thePathBuffer)
            return;

        if (inRenderContext.material.type != QSSGRenderGraphObject::Type::DefaultMaterial)
            return;

        if (thePathBuffer->m_pathType == QSSGRenderPath::PathType::Painted) {
            // painted path, go stroke route for now.
            if (!m_paintedShadowShader) {
                QSSGRef<QSSGDefaultMaterialShaderGeneratorInterface> theMaterialGenerator(
                        m_context->defaultMaterialShaderGenerator());
                QSSGXYRectVertexPipeline thePipeline(m_context->shaderProgramGenerator(), theMaterialGenerator);
                thePipeline.outputParaboloidDepthShaders();
                QSSGShaderCacheProgramFlags theFlags;
                const QSSGRef<QSSGRenderShaderProgram> &theProgram = thePipeline.programGenerator()->compileGeneratedShader("path painted paraboloid depth",
                                                                                                                                theFlags,
                                                                                                                                inFeatureSet);
                if (theProgram) {
                    m_paintedShadowShader.reset(new QSSGPathXYGeneratedShader(theProgram));
                }
            }
            if (m_paintedShadowShader) {
                // Setup the shader paraboloid information.
                const QSSGRef<QSSGRenderContext> &theRenderContext(m_context->renderContext());
                theRenderContext->setActiveShader(m_paintedShadowShader->m_shader);

                doRenderPaintedPath(m_paintedShadowShader.get(), inRenderContext, inRenderProperties, thePathBuffer, true);
            }
        } else {
            // Until we've also got a proper path render path for this, we'll call the old-fashioned
            // stuff.
            renderDepthPrepass(inRenderContext, inRenderProperties, inFeatureSet);
            // Q_ASSERT( false );
        }
    }

    void renderCubeFaceShadowPass(QSSGPathRenderContext &inRenderContext,
                                  QSSGLayerGlobalRenderProperties inRenderProperties,
                                  TShaderFeatureSet inFeatureSet) override
    {
        QSSGRef<QSSGPathBuffer> thePathBuffer = getPathBufferObject(inRenderContext.path);
        if (!thePathBuffer)
            return;

        if (inRenderContext.material.type != QSSGRenderGraphObject::Type::DefaultMaterial)
            return;

        if (thePathBuffer->m_pathType == QSSGRenderPath::PathType::Painted) {
            if (!m_paintedCubeShadowShader) {
                QSSGRef<QSSGDefaultMaterialShaderGeneratorInterface> theMaterialGenerator(
                        m_context->defaultMaterialShaderGenerator());
                QSSGXYRectVertexPipeline thePipeline(m_context->shaderProgramGenerator(), theMaterialGenerator);
                thePipeline.outputCubeFaceDepthShaders();
                QSSGShaderCacheProgramFlags theFlags;
                const QSSGRef<QSSGRenderShaderProgram> &theProgram = thePipeline.programGenerator()->compileGeneratedShader("path painted cube face depth",
                                                                                                                                theFlags,
                                                                                                                                inFeatureSet);
                if (theProgram) {
                    m_paintedCubeShadowShader.reset(new QSSGPathXYGeneratedShader(theProgram));
                }
            }
            if (m_paintedCubeShadowShader) {
                // Setup the shader information.
                const QSSGRef<QSSGRenderContext> &theRenderContext(m_context->renderContext());
                theRenderContext->setActiveShader(m_paintedCubeShadowShader->m_shader);

                m_paintedCubeShadowShader->m_cameraPosition.set(inRenderContext.camera.getGlobalPos());
                m_paintedCubeShadowShader->m_cameraProperties.set(QVector2D(1.0f, inRenderContext.camera.clipFar));
                m_paintedCubeShadowShader->m_modelMatrix.set(inRenderContext.modelMatrix);

                doRenderPaintedPath(m_paintedCubeShadowShader.get(), inRenderContext, inRenderProperties, thePathBuffer, false);
            }
        } else {
            // Until we've also got a proper path render path for this, we'll call the old-fashioned
            // stuff.
            renderDepthPrepass(inRenderContext, inRenderProperties, inFeatureSet);
        }
    }

    void renderPath(QSSGPathRenderContext &inRenderContext,
                    QSSGLayerGlobalRenderProperties inRenderProperties,
                    TShaderFeatureSet inFeatureSet) override
    {
        QSSGRef<QSSGPathBuffer> thePathBuffer = getPathBufferObject(inRenderContext.path);
        if (!thePathBuffer) {
            return;
        }

        bool isDefaultMaterial = (inRenderContext.material.type == QSSGRenderGraphObject::Type::DefaultMaterial);

        if (thePathBuffer->m_pathType == QSSGRenderPath::PathType::Geometry) {
            const QSSGRef<QSSGMaterialShaderGeneratorInterface> &theMaterialGenerator = getMaterialShaderGenertator(inRenderContext);

            // we need a more evolved key her for custom materials
            // the same key can still need a different shader
            QSSGPathShaderMapKey sPathkey = QSSGPathShaderMapKey(getMaterialNameForKey(inRenderContext),
                                                                     inRenderContext.materialKey);
            TShaderMap::iterator inserter = m_pathGeometryShaders.find(sPathkey);
            // QPair<TShaderMap::iterator, bool> inserter = m_PathGeometryShaders.insert(sPathkey, QSSGRef<SPathGeneratedShader>(nullptr));
            if (inserter == m_pathGeometryShaders.end()) {
                QSSGPathVertexPipeline thePipeline(m_context->shaderProgramGenerator(),
                                                     theMaterialGenerator,
                                                     m_context->wireframeMode());

                QSSGRef<QSSGRenderShaderProgram> theProgram = nullptr;

                if (isDefaultMaterial) {
                    theProgram = theMaterialGenerator->generateShader(inRenderContext.material,
                                                                      inRenderContext.materialKey,
                                                                      thePipeline,
                                                                      inFeatureSet,
                                                                      inRenderProperties.lights,
                                                                      inRenderContext.firstImage,
                                                                      inRenderContext.opacity < 1.0f,
                                                                      "path geometry pipeline-- ");
                } else {
                    QSSGRef<QSSGMaterialSystem> theMaterialSystem(m_context->customMaterialSystem());
                    const QSSGRenderCustomMaterial &theCustomMaterial(
                            reinterpret_cast<const QSSGRenderCustomMaterial &>(inRenderContext.material));

                    theProgram = theMaterialGenerator
                                         ->generateShader(inRenderContext.material,
                                                          inRenderContext.materialKey,
                                                          thePipeline,
                                                          inFeatureSet,
                                                          inRenderProperties.lights,
                                                          inRenderContext.firstImage,
                                                          inRenderContext.opacity < 1.0f,
                                                          "path geometry pipeline-- ",
                                                          theMaterialSystem->getShaderName(theCustomMaterial));
                }

                if (theProgram)
                    inserter = m_pathGeometryShaders.insert(sPathkey, new QSSGPathGeneratedShader(theProgram));
            }
            if (inserter == m_pathGeometryShaders.end())
                return;

            doRenderGeometryPath(inserter.value(), inRenderContext, inRenderProperties, thePathBuffer);
        } else {
            const QSSGRef<QSSGMaterialShaderGeneratorInterface> &theMaterialGenerator = getMaterialShaderGenertator(inRenderContext);

            // we need a more evolved key her for custom materials
            // the same key can still need a different shader
            QSSGPathShaderMapKey sPathkey = QSSGPathShaderMapKey(getMaterialNameForKey(inRenderContext),
                                                                     inRenderContext.materialKey);
            TPaintedShaderMap::iterator inserter = m_pathPaintedShaders.find(sPathkey);

            if (inserter == m_pathPaintedShaders.end()) {
                QSSGXYRectVertexPipeline thePipeline(m_context->shaderProgramGenerator(), theMaterialGenerator);

                QSSGRef<QSSGRenderShaderProgram> theProgram = nullptr;

                if (isDefaultMaterial) {
                    theProgram = theMaterialGenerator->generateShader(inRenderContext.material,
                                                                      inRenderContext.materialKey,
                                                                      thePipeline,
                                                                      inFeatureSet,
                                                                      inRenderProperties.lights,
                                                                      inRenderContext.firstImage,
                                                                      inRenderContext.opacity < 1.0f,
                                                                      "path painted pipeline-- ");
                } else {
                    const QSSGRef<QSSGMaterialSystem> &theMaterialSystem(m_context->customMaterialSystem());
                    const QSSGRenderCustomMaterial &theCustomMaterial(
                            reinterpret_cast<const QSSGRenderCustomMaterial &>(inRenderContext.material));

                    theProgram = theMaterialGenerator
                                         ->generateShader(inRenderContext.material,
                                                          inRenderContext.materialKey,
                                                          thePipeline,
                                                          inFeatureSet,
                                                          inRenderProperties.lights,
                                                          inRenderContext.firstImage,
                                                          inRenderContext.opacity < 1.0f,
                                                          "path painted pipeline-- ",
                                                          theMaterialSystem->getShaderName(theCustomMaterial));
                }

                if (theProgram)
                    inserter = m_pathPaintedShaders.insert(sPathkey, new QSSGPathXYGeneratedShader(theProgram));
            }
            if (inserter == m_pathPaintedShaders.end())
                return;

            doRenderPaintedPath(inserter.value(), inRenderContext, inRenderProperties, thePathBuffer);
        }
    }
};
}

QSSGPathManagerInterface::~QSSGPathManagerInterface() = default;

QVector2D QSSGPathManagerInterface::getControlPointFromAngleDistance(QVector2D inPosition, float inIncomingAngle, float inIncomingDistance)
{
    if (inIncomingDistance == 0.0f)
        return inPosition;
    const float angleRad = degToRad(inIncomingAngle);
    const float angleSin = std::sin(angleRad);
    const float angleCos = std::cos(angleRad);
    const QVector2D relativeAngles = QVector2D(angleCos * inIncomingDistance, angleSin * inIncomingDistance);
    return inPosition + relativeAngles;
}

QVector2D QSSGPathManagerInterface::getAngleDistanceFromControlPoint(QVector2D inPosition, QVector2D inControlPoint)
{
    const QVector2D relative = inControlPoint - inPosition;
    const float angleRad = std::atan2(relative.y(), relative.x());
    const float distance = vec2::magnitude(relative);
    return QVector2D(radToDeg(angleRad), distance);
}

QSSGRef<QSSGPathManagerInterface> QSSGPathManagerInterface::createPathManager(QSSGRenderContextInterface *ctx)
{
    return QSSGRef<QSSGPathManager>(new QSSGPathManager(ctx));
}

QT_END_NAMESPACE
