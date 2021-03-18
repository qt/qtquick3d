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

#include "qssgrendercustommaterialsystem_p.h"

#include <QtQuick3DUtils/private/qssgutils_p.h>

#include <QtQuick3DRender/private/qssgrendercontext_p.h>
#include <QtQuick3DRender/private/qssgrendershaderprogram_p.h>

#include <QtQuick3DRuntimeRender/private/qssgrendercustommaterialrendercontext_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercontextcore_p.h>
//#include <QtQuick3DRuntimeRender/private/qssgrendercustommaterial_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderdynamicobjectsystemcommands_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderbuffermanager_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderresourcemanager_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendermesh_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercamera_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderlight_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderlayer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderdynamicobjectsystemutil_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderableimage_p.h>
#include <QtQuick3DRuntimeRender/private/qssgvertexpipelineimpl_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendererimpllayerrenderdata_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendermodel_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderprefiltertexture_p.h>
#include <QtQuick3DRuntimeRender/private/qssgruntimerenderlogging_p.h>

QT_BEGIN_NAMESPACE

QSSGCustomMaterialVertexPipeline::QSSGCustomMaterialVertexPipeline(QSSGRenderContextInterface *inContext,
                                                                   TessellationModeValues inTessMode)
    : QSSGVertexPipelineImpl(inContext->customMaterialShaderGenerator(), inContext->shaderProgramGenerator(), false)
    , m_context(inContext)
    , m_tessMode(TessellationModeValues::NoTessellation)
{
    if (m_context->renderContext()->supportsTessellation()) {
        m_tessMode = inTessMode;
    }

    if (m_context->renderContext()->supportsGeometryStage() && m_tessMode != TessellationModeValues::NoTessellation) {
        m_wireframe = inContext->wireframeMode();
    }
}

void QSSGCustomMaterialVertexPipeline::initializeTessControlShader()
{
    if (m_tessMode == TessellationModeValues::NoTessellation
            || programGenerator()->getStage(QSSGShaderGeneratorStage::TessControl) == nullptr) {
        return;
    }

    QSSGShaderStageGeneratorInterface &tessCtrlShader(*programGenerator()->getStage(QSSGShaderGeneratorStage::TessControl));

    tessCtrlShader.addUniform("tessLevelInner", "float");
    tessCtrlShader.addUniform("tessLevelOuter", "float");

    setupTessIncludes(QSSGShaderGeneratorStage::TessControl, m_tessMode);

    tessCtrlShader.append("void main() {\n");

    tessCtrlShader.append("\tctWorldPos[0] = varWorldPos[0];");
    tessCtrlShader.append("\tctWorldPos[1] = varWorldPos[1];");
    tessCtrlShader.append("\tctWorldPos[2] = varWorldPos[2];");

    if (m_tessMode == TessellationModeValues::Phong || m_tessMode == TessellationModeValues::NPatch) {
        tessCtrlShader.append("\tctNorm[0] = varObjectNormal[0];");
        tessCtrlShader.append("\tctNorm[1] = varObjectNormal[1];");
        tessCtrlShader.append("\tctNorm[2] = varObjectNormal[2];");
    }
    if (m_tessMode == TessellationModeValues::NPatch) {
        tessCtrlShader.append("\tctTangent[0] = varObjTangent[0];");
        tessCtrlShader.append("\tctTangent[1] = varObjTangent[1];");
        tessCtrlShader.append("\tctTangent[2] = varObjTangent[2];");
    }

    tessCtrlShader.append("\tgl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;");
    tessCtrlShader.append("\ttessShader( tessLevelOuter, tessLevelInner);\n");
}

void QSSGCustomMaterialVertexPipeline::initializeTessEvaluationShader()
{
    if (m_tessMode == TessellationModeValues::NoTessellation
            || programGenerator()->getStage(QSSGShaderGeneratorStage::TessEval) == nullptr) {
        return;
    }

    QSSGShaderStageGeneratorInterface &tessEvalShader(*programGenerator()->getStage(QSSGShaderGeneratorStage::TessEval));

    tessEvalShader.addUniform("modelViewProjection", "mat4");
    tessEvalShader.addUniform("normalMatrix", "mat3");

    setupTessIncludes(QSSGShaderGeneratorStage::TessEval, m_tessMode);

    if (m_tessMode == TessellationModeValues::Linear && m_displacementImage) {
        tessEvalShader.addInclude("defaultMaterialFileDisplacementTexture.glsllib");
        tessEvalShader.addUniform("modelMatrix", "mat4");
        tessEvalShader.addUniform("displace_tiling", "vec3");
        tessEvalShader.addUniform("displaceAmount", "float");
        tessEvalShader.addUniform(m_displacementImage->m_image.m_imageShaderName.toUtf8(), "sampler2D");
    }

    tessEvalShader.append("void main() {");

    if (m_tessMode == TessellationModeValues::NPatch) {
        tessEvalShader.append("\tctNorm[0] = varObjectNormalTC[0];");
        tessEvalShader.append("\tctNorm[1] = varObjectNormalTC[1];");
        tessEvalShader.append("\tctNorm[2] = varObjectNormalTC[2];");

        tessEvalShader.append("\tctTangent[0] = varTangentTC[0];");
        tessEvalShader.append("\tctTangent[1] = varTangentTC[1];");
        tessEvalShader.append("\tctTangent[2] = varTangentTC[2];");
    }

    tessEvalShader.append("\tvec4 pos = tessShader( );\n");
}

void QSSGCustomMaterialVertexPipeline::finalizeTessControlShader()
{
    QSSGShaderStageGeneratorInterface &tessCtrlShader(*programGenerator()->getStage(QSSGShaderGeneratorStage::TessControl));
    // add varyings we must pass through
    typedef TStrTableStrMap::const_iterator TParamIter;
    for (TParamIter iter = m_interpolationParameters.begin(), end = m_interpolationParameters.end(); iter != end; ++iter) {
        tessCtrlShader << "\t" << iter.key() << "TC[gl_InvocationID] = " << iter.key() << "[gl_InvocationID];\n";
    }
}

void QSSGCustomMaterialVertexPipeline::finalizeTessEvaluationShader()
{
    QSSGShaderStageGeneratorInterface &tessEvalShader(*programGenerator()->getStage(QSSGShaderGeneratorStage::TessEval));

    QByteArray outExt;
    if (programGenerator()->getEnabledStages() & QSSGShaderGeneratorStage::Geometry)
        outExt = "TE";

    // add varyings we must pass through
    typedef TStrTableStrMap::const_iterator TParamIter;
    if (m_tessMode == TessellationModeValues::NPatch) {
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
        if (m_tessMode == TessellationModeValues::Linear && m_displacementImage) {
            tessEvalShader << "\ttexture_coordinate_info tmp = textureCoordinateInfo( varTexCoord0" << outExt
                           << ", varTangent" << outExt << ", varBinormal" << outExt << " );"
                           << "\n";
            tessEvalShader << "\ttmp = transformCoordinate( rotationTranslationScale( vec3( "
                              "0.000000, 0.000000, 0.000000 ), vec3( 0.000000, 0.000000, "
                              "0.000000 ), displace_tiling ), tmp);"
                           << "\n";

            tessEvalShader << "\tpos.xyz = defaultMaterialFileDisplacementTexture( "
                           << m_displacementImage->m_image.m_imageShaderName.toUtf8() << ", displaceAmount, "
                           << "tmp.position.xy";
            tessEvalShader << ", varObjectNormal" << outExt << ", pos.xyz );"
                           << "\n";
            tessEvalShader << "\tvarWorldPos" << outExt << "= (modelMatrix * pos).xyz;"
                           << "\n";
        }

        // transform the normal
        tessEvalShader << "\n\tvarNormal" << outExt << " = normalize(normalMatrix * varObjectNormal" << outExt << ");\n";
    }

    tessEvalShader.append("\tgl_Position = modelViewProjection * pos;\n");
}

// Responsible for beginning all vertex and fragment generation (void main() { etc).
void QSSGCustomMaterialVertexPipeline::beginVertexGeneration(const QSSGShaderDefaultMaterialKey &inKey, quint32 displacementImageIdx, QSSGRenderableImage *displacementImage)
{
    m_displacementIdx = displacementImageIdx;
    m_displacementImage = displacementImage;

    QSSGShaderGeneratorStageFlags theStages(QSSGShaderProgramGeneratorInterface::defaultFlags());

    if (m_tessMode != TessellationModeValues::NoTessellation) {
        theStages |= QSSGShaderGeneratorStage::TessControl;
        theStages |= QSSGShaderGeneratorStage::TessEval;
    }
    if (m_wireframe) {
        theStages |= QSSGShaderGeneratorStage::Geometry;
    }

    programGenerator()->beginProgram(theStages);

    if (m_tessMode != TessellationModeValues::NoTessellation) {
        initializeTessControlShader();
        initializeTessEvaluationShader();
    }
    if (m_wireframe) {
        initializeWireframeGeometryShader();
    }

    QSSGShaderStageGeneratorInterface &vertexShader(vertex());

    // thinks we need
    vertexShader.addInclude("viewProperties.glsllib");
    vertexShader.addInclude("customMaterial.glsllib");

    vertexShader.addIncoming("attr_pos", "vec3");
    vertexShader << "void main()"
                 << "\n"
                 << "{"
                 << "\n";

    if (displacementImage) {
        generateUVCoords(inKey, 0);
        if (!hasTessellation()) {
            vertexShader.addUniform("displaceAmount", "float");
            vertexShader.addUniform("displace_tiling", "vec3");
            // we create the world position setup here
            // because it will be replaced with the displaced position
            setCode(GenerationFlag::WorldPosition);
            vertexShader.addUniform("modelMatrix", "mat4");

            vertexShader.addInclude("defaultMaterialFileDisplacementTexture.glsllib");
            vertexShader.addUniform(displacementImage->m_image.m_imageShaderName.toUtf8(), "sampler2D");

            vertexShader << "\ttexture_coordinate_info tmp = textureCoordinateInfo( texCoord0, "
                            "varTangent, varBinormal );"
                         << "\n";
            vertexShader << "\ttmp = transformCoordinate( rotationTranslationScale( vec3( "
                            "0.000000, 0.000000, 0.000000 ), vec3( 0.000000, 0.000000, "
                            "0.000000 ), displace_tiling ), tmp);"
                         << "\n";

            vertexShader << "\tvec3 displacedPos = defaultMaterialFileDisplacementTexture( "
                         << displacementImage->m_image.m_imageShaderName.toUtf8() << ", displaceAmount, "
                         << "tmp.position.xy"
                         << ", attr_norm, attr_pos );"
                         << "\n";

            addInterpolationParameter("varWorldPos", "vec3");
            vertexShader.append("\tvec3 local_model_world_position = (modelMatrix * "
                                "vec4(displacedPos, 1.0)).xyz;");
            assignOutput("varWorldPos", "local_model_world_position");
        }
    }

    if (hasTessellation()) {
        vertexShader.append("\tgl_Position = vec4(attr_pos, 1.0);");
    } else {
        vertexShader.addUniform("modelViewProjection", "mat4");
        if (displacementImage)
            vertexShader.append("\tgl_Position = modelViewProjection * vec4(displacedPos, 1.0);");
        else
            vertexShader.append("\tgl_Position = modelViewProjection * vec4(attr_pos, 1.0);");
    }

    if (hasTessellation()) {
        generateWorldPosition();
        generateWorldNormal(inKey);
        generateObjectNormal();
        generateVarTangentAndBinormal(inKey);
    }
}

void QSSGCustomMaterialVertexPipeline::beginFragmentGeneration()
{
    fragment().addUniform("objectOpacity", "float");
    fragment() << "void main()"
               << "\n"
               << "{"
               << "\n";
}

void QSSGCustomMaterialVertexPipeline::assignOutput(const QByteArray &inVarName, const QByteArray &inVarValue)
{
    vertex() << "\t" << inVarName << " = " << inVarValue << ";\n";
}

void QSSGCustomMaterialVertexPipeline::generateUVCoords(const QSSGShaderDefaultMaterialKey &, quint32 inUVSet)
{
    if (inUVSet == 0 && setCode(GenerationFlag::UVCoords))
        return;
    if (inUVSet == 1 && setCode(GenerationFlag::UVCoords1))
        return;

    Q_ASSERT(inUVSet == 0 || inUVSet == 1);

    if (inUVSet == 0)
        addInterpolationParameter("varTexCoord0", "vec3");
    else if (inUVSet == 1)
        addInterpolationParameter("varTexCoord1", "vec3");

    doGenerateUVCoords(inUVSet);
}

void QSSGCustomMaterialVertexPipeline::generateWorldNormal(const QSSGShaderDefaultMaterialKey &)
{
    if (setCode(GenerationFlag::WorldNormal))
        return;
    addInterpolationParameter("varNormal", "vec3");
    doGenerateWorldNormal();
}

void QSSGCustomMaterialVertexPipeline::generateObjectNormal()
{
    if (setCode(GenerationFlag::ObjectNormal))
        return;
    doGenerateObjectNormal();
}

void QSSGCustomMaterialVertexPipeline::generateVarTangentAndBinormal(const QSSGShaderDefaultMaterialKey &)
{
    if (setCode(GenerationFlag::TangentBinormal))
        return;
    addInterpolationParameter("varTangent", "vec3");
    addInterpolationParameter("varBinormal", "vec3");
    addInterpolationParameter("varObjTangent", "vec3");
    addInterpolationParameter("varObjBinormal", "vec3");
    doGenerateVarTangent();
    doGenerateVarBinormal();
}

void QSSGCustomMaterialVertexPipeline::generateWorldPosition()
{
    if (setCode(GenerationFlag::WorldPosition))
        return;

    activeStage().addUniform("modelMatrix", "mat4");
    addInterpolationParameter("varWorldPos", "vec3");
    addInterpolationParameter("varObjPos", "vec3");
    doGenerateWorldPosition();
}

// responsible for closing all vertex and fragment generation
void QSSGCustomMaterialVertexPipeline::endVertexGeneration(bool customShader)
{
    if (hasTessellation()) {
        // finalize tess control shader
        finalizeTessControlShader();
        // finalize tess evaluation shader
        finalizeTessEvaluationShader();

        tessControl().append("}");
        tessEval().append("}");

        if (m_wireframe) {
            // finalize geometry shader
            finalizeWireframeGeometryShader();
            geometry().append("}");
        }
    }

    if (!customShader)
        vertex().append("}");
}

void QSSGCustomMaterialVertexPipeline::endFragmentGeneration(bool customShader)
{
    if (!customShader)
        fragment().append("}");
}

QSSGShaderStageGeneratorInterface &QSSGCustomMaterialVertexPipeline::activeStage()
{
    return vertex();
}

void QSSGCustomMaterialVertexPipeline::addInterpolationParameter(const QByteArray &inName, const QByteArray &inType)
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

void QSSGCustomMaterialVertexPipeline::doGenerateUVCoords(quint32 inUVSet)
{
    Q_ASSERT(inUVSet == 0 || inUVSet == 1);

    if (inUVSet == 0) {
        vertex().addIncoming("attr_uv0", "vec2");
        vertex() << "\tvec3 texCoord0 = vec3( attr_uv0, 0.0 );"
                 << "\n";
        assignOutput("varTexCoord0", "texCoord0");
    } else if (inUVSet == 1) {
        vertex().addIncoming("attr_uv1", "vec2");
        vertex() << "\tvec3 texCoord1 = vec3( attr_uv1, 1.0 );"
                 << "\n";
        assignOutput("varTexCoord1", "texCoord1");
    }
}

void QSSGCustomMaterialVertexPipeline::doGenerateWorldNormal()
{
    QSSGShaderStageGeneratorInterface &vertexGenerator(vertex());
    vertexGenerator.addIncoming("attr_norm", "vec3");
    vertexGenerator.addUniform("normalMatrix", "mat3");

    if (!hasTessellation()) {
        vertex().append("\tvarNormal = normalize( normalMatrix * attr_norm );");
    }
}

void QSSGCustomMaterialVertexPipeline::doGenerateObjectNormal()
{
    addInterpolationParameter("varObjectNormal", "vec3");
    vertex().append("\tvarObjectNormal = attr_norm;");
}

void QSSGCustomMaterialVertexPipeline::doGenerateWorldPosition()
{
    vertex().append("\tvarObjPos = attr_pos;");
    vertex().append("\tvec4 worldPos = (modelMatrix * vec4(attr_pos, 1.0));");
    assignOutput("varWorldPos", "worldPos.xyz");
}

void QSSGCustomMaterialVertexPipeline::doGenerateVarTangent()
{
    vertex().addIncoming("attr_textan", "vec3");

    vertex() << "\tvarTangent = normalMatrix * attr_textan;\n";
    vertex() << "\tvarObjTangent = attr_textan;\n";
}

void QSSGCustomMaterialVertexPipeline::doGenerateVarBinormal()
{
    vertex().addIncoming("attr_binormal", "vec3");

    vertex() << "\tvarBinormal = normalMatrix * attr_binormal;\n";
    vertex() << "\tvarObjBinormal = attr_binormal;\n";
}

void QSSGCustomMaterialVertexPipeline::doGenerateVertexColor(const QSSGShaderDefaultMaterialKey &)
{
    vertex().addIncoming("attr_color", "vec3");
    vertex().append("\tvarColor = attr_color;");
}

struct QSSGShaderMapKey
{
    TStrStrPair m_name;
    ShaderFeatureSetList m_features;
    TessellationModeValues m_tessMode;
    bool m_wireframeMode;
    QSSGShaderDefaultMaterialKey m_materialKey;
    uint m_hashCode;
    QSSGShaderMapKey(const TStrStrPair &inName,
                       const ShaderFeatureSetList &inFeatures,
                       TessellationModeValues inTessMode,
                       bool inWireframeMode,
                       QSSGShaderDefaultMaterialKey inMaterialKey)
        : m_name(inName), m_features(inFeatures), m_tessMode(inTessMode), m_wireframeMode(inWireframeMode), m_materialKey(inMaterialKey)
    {
        m_hashCode = qHash(m_name) ^ hashShaderFeatureSet(m_features) ^ qHash(m_tessMode) ^ qHash(m_wireframeMode)
                ^ qHash(inMaterialKey.hash());
    }
    bool operator==(const QSSGShaderMapKey &inKey) const
    {
        return m_name == inKey.m_name && m_features == inKey.m_features && m_tessMode == inKey.m_tessMode
                && m_wireframeMode == inKey.m_wireframeMode && m_materialKey == inKey.m_materialKey;
    }
};

uint qHash(const QSSGShaderMapKey &key)
{
    return key.m_hashCode;
}

struct QSSGCustomMaterialTextureData
{
    QAtomicInt ref;
    QSSGRef<QSSGRenderShaderProgram> shader;
    QSSGRenderCachedShaderProperty<QSSGRenderTexture2D *> sampler;
    QSSGRef<QSSGRenderTexture2D> texture;
    bool needsMips;

    QSSGCustomMaterialTextureData(const QSSGRef<QSSGRenderShaderProgram> &inShader,
                                    const QSSGRef<QSSGRenderTexture2D> &inTexture,
                                    const QByteArray &inTexName,
                                    bool inNeedMips)
        : shader(inShader), sampler(inTexName, inShader), texture(inTexture), needsMips(inNeedMips)
    {
    }

    void set(const QSSGRenderCustomMaterial::TextureProperty *inDefinition)
    {
        if (texture && inDefinition) {
            texture->setMagFilter(inDefinition->magFilterType);
            texture->setMinFilter(inDefinition->minFilterType);
            texture->setTextureWrapS(inDefinition->clampType);
            texture->setTextureWrapT(inDefinition->clampType);
        } else if (texture) {
            // set some defaults
            texture->setMinFilter(QSSGRenderTextureMinifyingOp::Linear);
            texture->setTextureWrapS(QSSGRenderTextureCoordOp::ClampToEdge);
            texture->setTextureWrapT(QSSGRenderTextureCoordOp::ClampToEdge);
        }

        if ((texture->numMipmaps() == 0) && needsMips)
            texture->generateMipmaps();

        sampler.set(texture.data());
    }

    static QSSGCustomMaterialTextureData createTextureEntry(const QSSGRef<QSSGRenderShaderProgram> &inShader,
                                                              const QSSGRef<QSSGRenderTexture2D> &inTexture,
                                                              const QByteArray &inTexName,
                                                              bool needMips)
    {
        return QSSGCustomMaterialTextureData(inShader, inTexture, inTexName, needMips);
    }
};

/**
 *	Cached tessellation property lookups this is on a per mesh base
 */
struct QSSGCustomMaterialsTessellationProperties
{
    QSSGRenderCachedShaderProperty<float> m_edgeTessLevel; ///< tesselation value for the edges
    QSSGRenderCachedShaderProperty<float> m_insideTessLevel; ///< tesselation value for the inside
    QSSGRenderCachedShaderProperty<float> m_phongBlend; ///< blending between linear and phong component
    QSSGRenderCachedShaderProperty<QVector2D> m_distanceRange; ///< distance range for min and max tess level
    QSSGRenderCachedShaderProperty<float> m_disableCulling; ///< if set to 1.0 this disables backface
    /// culling optimization in the tess shader

    QSSGCustomMaterialsTessellationProperties() = default;
    explicit QSSGCustomMaterialsTessellationProperties(const QSSGRef<QSSGRenderShaderProgram> &inShader)
        : m_edgeTessLevel("tessLevelOuter", inShader)
        , m_insideTessLevel("tessLevelInner", inShader)
        , m_phongBlend("phongBlend", inShader)
        , m_distanceRange("distanceRange", inShader)
        , m_disableCulling("disableCulling", inShader)
    {
    }
};

/* We setup some shared state on the custom material shaders */
struct QSSGRenderCustomMaterialShader
{
    QAtomicInt ref;
    QSSGRef<QSSGRenderShaderProgram> shader;
    QSSGRenderCachedShaderProperty<QMatrix4x4> modelMatrix;
    QSSGRenderCachedShaderProperty<QMatrix4x4> viewProjMatrix;
    QSSGRenderCachedShaderProperty<QMatrix4x4> viewMatrix;
    QSSGRenderCachedShaderProperty<QMatrix3x3> normalMatrix;
    QSSGRenderCachedShaderProperty<QVector3D> cameraPos;
    QSSGRenderCachedShaderProperty<QMatrix4x4> projMatrix;
    QSSGRenderCachedShaderProperty<QMatrix4x4> viewportMatrix;
    QSSGRenderCachedShaderProperty<QVector2D> camProperties;
    QSSGRenderCachedShaderProperty<QSSGRenderTexture2D *> depthTexture;
    QSSGRenderCachedShaderProperty<QSSGRenderTexture2D *> aoTexture;
    QSSGRenderCachedShaderProperty<QSSGRenderTexture2D *> lightProbe;
    QSSGRenderCachedShaderProperty<QVector4D> lightProbeProps;
    QSSGRenderCachedShaderProperty<QVector4D> lightProbeOpts;
    QSSGRenderCachedShaderProperty<QVector4D> lightProbeRot;
    QSSGRenderCachedShaderProperty<QVector4D> lightProbeOfs;
    QSSGRenderCachedShaderProperty<QSSGRenderTexture2D *> lightProbe2;
    QSSGRenderCachedShaderProperty<QVector4D> lightProbe2Props;
    QSSGRenderCachedShaderProperty<qint32> lightCount;
    QSSGRenderCachedShaderProperty<qint32> areaLightCount;
    QSSGRenderCachedShaderBuffer<QSSGRenderShaderConstantBuffer> aoShadowParams;
    QSSGCustomMaterialsTessellationProperties tessellation;
    dynamic::QSSGDynamicShaderProgramFlags programFlags;

    QSSGRenderCustomMaterialShader(const QSSGRef<QSSGRenderShaderProgram> &inShader, dynamic::QSSGDynamicShaderProgramFlags inFlags)
        : shader(inShader)
        , modelMatrix("modelMatrix", inShader)
        , viewProjMatrix("modelViewProjection", inShader)
        , viewMatrix("viewMatrix", inShader)
        , normalMatrix("normalMatrix", inShader)
        , cameraPos("cameraPosition", inShader)
        , projMatrix("viewProjectionMatrix", inShader)
        , viewportMatrix("viewportMatrix", inShader)
        , camProperties("cameraProperties", inShader)
        , depthTexture("depthTexture", inShader)
        , aoTexture("aoTexture", inShader)
        , lightProbe("lightProbe", inShader)
        , lightProbeProps("lightProbeProperties", inShader)
        , lightProbeOpts("lightProbeOptions", inShader)
        , lightProbeRot("lightProbeRotation", inShader)
        , lightProbeOfs("lightProbeOffset", inShader)
        , lightProbe2("lightProbe2", inShader)
        , lightProbe2Props("lightProbe2Properties", inShader)
        , lightCount("lightCount", inShader)
        , areaLightCount("areaLightCount", inShader)
        , aoShadowParams("aoShadow", inShader)
        , tessellation(inShader)
        , programFlags(inFlags)
    {
    }
};

struct QSSGMaterialOrComputeShader
{
    // TODO: struct/class?
    const QSSGRef<QSSGRenderCustomMaterialShader> m_materialShader;
    const QSSGRef<QSSGRenderShaderProgram> m_computeShader;
    QSSGMaterialOrComputeShader() = default;
    explicit QSSGMaterialOrComputeShader(const QSSGRef<QSSGRenderCustomMaterialShader> &inMaterialShader)
        : m_materialShader(inMaterialShader)
    {
    }
    explicit QSSGMaterialOrComputeShader(const QSSGRef<QSSGRenderShaderProgram> &inComputeShader)
        : m_computeShader(inComputeShader)
    {
        Q_ASSERT(inComputeShader->programType() == QSSGRenderShaderProgram::ProgramType::Compute);
    }
    bool isValid() const { return m_materialShader || m_computeShader; }
    bool isComputeShader() const { return m_computeShader != nullptr; }
    bool isMaterialShader() const { return m_materialShader != nullptr; }
    const QSSGRef<QSSGRenderCustomMaterialShader> &materialShader()
    {
        Q_ASSERT(isMaterialShader());
        return m_materialShader;
    }
    const QSSGRef<QSSGRenderShaderProgram> &computeShader()
    {
        Q_ASSERT(isComputeShader());
        return m_computeShader;
    }
};

struct QSSGRenderCustomMaterialBuffer
{
    QByteArray name;
    QSSGRef<QSSGRenderFrameBuffer> frameBuffer;
    QSSGRef<QSSGRenderTexture2D> texture;
    dynamic::QSSGAllocateBufferFlags flags;

    QSSGRenderCustomMaterialBuffer(const QByteArray &inName,
                               const QSSGRef<QSSGRenderFrameBuffer> &inFb,
                               const QSSGRef<QSSGRenderTexture2D> &inTexture,
                               dynamic::QSSGAllocateBufferFlags inFlags)
        : name(inName), frameBuffer(inFb), texture(inTexture), flags(inFlags)
    {
    }
    QSSGRenderCustomMaterialBuffer() = default;
};

struct QSSGStringMemoryBarrierFlagMap
{
    const char *name;
    QSSGRenderBufferBarrierValues value;
    constexpr QSSGStringMemoryBarrierFlagMap(const char *nm, QSSGRenderBufferBarrierValues val) : name(nm), value(val)
    {
    }
};

// TODO:
//const QSSGStringMemoryBarrierFlagMap g_StringMemoryFlagMap[] = {
//    QSSGStringMemoryBarrierFlagMap("vertex_attribute", QSSGRenderBufferBarrierValues::VertexAttribArray),
//    QSSGStringMemoryBarrierFlagMap("element_array", QSSGRenderBufferBarrierValues::ElementArray),
//    QSSGStringMemoryBarrierFlagMap("uniform_buffer", QSSGRenderBufferBarrierValues::UniformBuffer),
//    QSSGStringMemoryBarrierFlagMap("texture_fetch", QSSGRenderBufferBarrierValues::TextureFetch),
//    QSSGStringMemoryBarrierFlagMap("shader_image_access", QSSGRenderBufferBarrierValues::ShaderImageAccess),
//    QSSGStringMemoryBarrierFlagMap("command_buffer", QSSGRenderBufferBarrierValues::CommandBuffer),
//    QSSGStringMemoryBarrierFlagMap("pixel_buffer", QSSGRenderBufferBarrierValues::PixelBuffer),
//    QSSGStringMemoryBarrierFlagMap("texture_update", QSSGRenderBufferBarrierValues::TextureUpdate),
//    QSSGStringMemoryBarrierFlagMap("buffer_update", QSSGRenderBufferBarrierValues::BufferUpdate),
//    QSSGStringMemoryBarrierFlagMap("frame_buffer", QSSGRenderBufferBarrierValues::Framebuffer),
//    QSSGStringMemoryBarrierFlagMap("transform_feedback", QSSGRenderBufferBarrierValues::TransformFeedback),
//    QSSGStringMemoryBarrierFlagMap("atomic_counter", QSSGRenderBufferBarrierValues::AtomicCounter),
//    QSSGStringMemoryBarrierFlagMap("shader_storage", QSSGRenderBufferBarrierValues::ShaderStorage),
//};

struct QSSGStringBlendFuncMap
{
    const char *name;
    QSSGRenderSrcBlendFunc value;
    constexpr QSSGStringBlendFuncMap(const char *nm, QSSGRenderSrcBlendFunc val) : name(nm), value(val) {}
};

// TODO:
//const QSSGStringBlendFuncMap g_BlendFuncMap[] = {
//    QSSGStringBlendFuncMap("Unknown", QSSGRenderSrcBlendFunc::Unknown),
//    QSSGStringBlendFuncMap("Zero", QSSGRenderSrcBlendFunc::Zero),
//    QSSGStringBlendFuncMap("One", QSSGRenderSrcBlendFunc::One),
//    QSSGStringBlendFuncMap("SrcColor", QSSGRenderSrcBlendFunc::SrcColor),
//    QSSGStringBlendFuncMap("OneMinusSrcColor", QSSGRenderSrcBlendFunc::OneMinusSrcColor),
//    QSSGStringBlendFuncMap("DstColor", QSSGRenderSrcBlendFunc::DstColor),
//    QSSGStringBlendFuncMap("OneMinusDstColor", QSSGRenderSrcBlendFunc::OneMinusDstColor),
//    QSSGStringBlendFuncMap("SrcAlpha", QSSGRenderSrcBlendFunc::SrcAlpha),
//    QSSGStringBlendFuncMap("OneMinusSrcAlpha", QSSGRenderSrcBlendFunc::OneMinusSrcAlpha),
//    QSSGStringBlendFuncMap("DstAlpha", QSSGRenderSrcBlendFunc::DstAlpha),
//    QSSGStringBlendFuncMap("OneMinusDstAlpha", QSSGRenderSrcBlendFunc::OneMinusDstAlpha),
//    QSSGStringBlendFuncMap("ConstantColor", QSSGRenderSrcBlendFunc::ConstantColor),
//    QSSGStringBlendFuncMap("OneMinusConstantColor", QSSGRenderSrcBlendFunc::OneMinusConstantColor),
//    QSSGStringBlendFuncMap("ConstantAlpha", QSSGRenderSrcBlendFunc::ConstantAlpha),
//    QSSGStringBlendFuncMap("OneMinusConstantAlpha", QSSGRenderSrcBlendFunc::OneMinusConstantAlpha),
//    QSSGStringBlendFuncMap("SrcAlphaSaturate", QSSGRenderSrcBlendFunc::SrcAlphaSaturate)
//};


QSSGMaterialSystem::QSSGMaterialSystem(QSSGRenderContextInterface *ct)
    : context(ct)
{
}

QSSGMaterialSystem::~QSSGMaterialSystem()
{
    while (allocatedBuffers.size()) { // replace_with_last
        allocatedBuffers[0] = allocatedBuffers.back();
        allocatedBuffers.pop_back();
    }
}

void QSSGMaterialSystem::releaseBuffer(qint32 inIdx)
{
    // Don't call this on MaterialSystem destroy.
    // This causes issues for scene liftime buffers
    // because the resource manager is destroyed before
    const QSSGRef<QSSGResourceManager> &theManager(context->resourceManager());
    QSSGRenderCustomMaterialBuffer &theEntry(allocatedBuffers[inIdx]);
    theEntry.frameBuffer->attach(QSSGRenderFrameBufferAttachment::Color0, QSSGRenderTextureOrRenderBuffer());

    theManager->release(theEntry.frameBuffer);
    theManager->release(theEntry.texture);
    { // replace_with_last
        allocatedBuffers[inIdx] = allocatedBuffers.back();
        allocatedBuffers.pop_back();
    }
}

qint32 QSSGMaterialSystem::findBuffer(const QByteArray &inName) const
{
    for (qint32 idx = 0, end = allocatedBuffers.size(); idx < end; ++idx) {
        if (allocatedBuffers.at(idx).name == inName)
            return idx;
    }
    return allocatedBuffers.size();
}

bool QSSGMaterialSystem::textureNeedsMips(const QSSGRenderCustomMaterial::TextureProperty *inPropDec, QSSGRenderTexture2D *inTexture)
{
    if (inPropDec && inTexture) {
        return bool((inPropDec->minFilterType == QSSGRenderTextureMinifyingOp::LinearMipmapLinear)
                    && (inTexture->numMipmaps() == 0));
    }

    return false;
}

void QSSGMaterialSystem::setTexture(const QSSGRef<QSSGRenderShaderProgram> &inShader,
                                      const QByteArray &inPropName,
                                      const QSSGRef<QSSGRenderTexture2D> &inTexture,
                                      const QSSGRenderCustomMaterial::TextureProperty *inPropDec,
                                      bool needMips)
{
    QSSGRef<QSSGCustomMaterialTextureData> theTextureEntry;
    auto it = textureEntries.cbegin();
    const auto end = textureEntries.cend();
    for (; it != end && theTextureEntry == nullptr; ++it) {
        if (it->first == inPropName && it->second->shader == inShader
            && it->second->texture == inTexture) {
            theTextureEntry = it->second;
            break;
        }
    }
    if (theTextureEntry == nullptr) {
        QSSGRef<QSSGCustomMaterialTextureData> theNewEntry(new QSSGCustomMaterialTextureData(
                                                                   QSSGCustomMaterialTextureData::createTextureEntry(inShader, inTexture, inPropName, needMips)));
        textureEntries.push_back(QPair<QByteArray, QSSGRef<QSSGCustomMaterialTextureData>>(inPropName, theNewEntry));
        theTextureEntry = theNewEntry;
    }
    // TODO: Already set?
    theTextureEntry->set(inPropDec);
}

// TODO: Use an enum for the shader type?
// Remove and call the setShaderData func directly?
void QSSGMaterialSystem::setMaterialClassShader(const QByteArray &inName, const QByteArray &inShaderType, const QByteArray &inShaderVersion,
                                                  const QByteArray &inShaderData, bool inHasGeomShader, bool inIsComputeShader)
{
    context->dynamicObjectSystem()->setShaderData(inName, inShaderData, inShaderType, inShaderVersion, inHasGeomShader, inIsComputeShader);
}

QSSGRef<QSSGRenderShaderProgram> QSSGMaterialSystem::getShader(QSSGCustomMaterialRenderContext &inRenderContext,
                                                                     const QSSGRenderCustomMaterial &inMaterial,
                                                                     const dynamic::QSSGBindShader &inCommand,
                                                                     const ShaderFeatureSetList &inFeatureSet,
                                                                     const dynamic::QSSGDynamicShaderProgramFlags &inFlags)
{
    Q_UNUSED(inFlags);
    const QSSGRef<QSSGMaterialShaderGeneratorInterface> &theMaterialGenerator(context->customMaterialShaderGenerator());

    // generate key
    //        QString theKey = getShaderCacheKey(theShaderKeyBuffer, inCommand.m_shaderPath,
    //                                           inCommand.m_shaderDefine, inFlags);
    // ### TODO: Enable caching?

    QSSGCustomMaterialVertexPipeline thePipeline(context, inRenderContext.model.tessellationMode);

    const QSSGRef<QSSGRenderShaderProgram> &theProgram = theMaterialGenerator->generateShader(inMaterial,
                                                                                                  inRenderContext.materialKey,
                                                                                                  thePipeline,
                                                                                                  inFeatureSet,
                                                                                                  inRenderContext.lights,
                                                                                                  inRenderContext.firstImage,
                                                                                                  (inMaterial.m_hasTransparency || inMaterial.m_hasRefraction),
                                                                                                  "custom material pipeline-- ",
                                                                                                  inCommand.m_shaderPath);

    return theProgram;
}

QSSGMaterialOrComputeShader QSSGMaterialSystem::bindShader(QSSGCustomMaterialRenderContext &inRenderContext, const QSSGRenderCustomMaterial &inMaterial, const dynamic::QSSGBindShader &inCommand, const ShaderFeatureSetList &inFeatureSet)
{
    QSSGRef<QSSGRenderShaderProgram> theProgram;

    dynamic::QSSGDynamicShaderProgramFlags theFlags(inRenderContext.model.tessellationMode, inRenderContext.subset.wireframeMode);
    if (inRenderContext.model.tessellationMode != TessellationModeValues::NoTessellation)
        theFlags |= ShaderCacheProgramFlagValues::TessellationEnabled;
    if (inRenderContext.subset.wireframeMode)
        theFlags |= ShaderCacheProgramFlagValues::GeometryShaderEnabled;

    QSSGShaderMapKey skey = QSSGShaderMapKey(TStrStrPair(inCommand.m_shaderPath, inCommand.m_shaderDefine),
                                                 inFeatureSet,
                                                 theFlags.tessMode,
                                                 theFlags.wireframeMode,
                                                 inRenderContext.materialKey);
    auto theInsertResult = shaderMap.find(skey);
    // QPair<TShaderMap::iterator, bool> theInsertResult(m_ShaderMap.insert(skey, QSSGRef<SCustomMaterialShader>(nullptr)));

    QSSGShaderPreprocessorFeature noFragOutputFeature("NO_FRAG_OUTPUT", true);
    ShaderFeatureSetList features(inFeatureSet);
    features.push_back(noFragOutputFeature);

    if (theInsertResult == shaderMap.end()) {
        theProgram = getShader(inRenderContext, inMaterial, inCommand, features, theFlags);

        if (theProgram) {
            theInsertResult = shaderMap.insert(skey,
                                                 QSSGRef<QSSGRenderCustomMaterialShader>(
                                                     new QSSGRenderCustomMaterialShader(theProgram, theFlags)));
        }
    } else if (theInsertResult.value()) {
        theProgram = theInsertResult.value()->shader;
    }

    if (theProgram) {
        if (theProgram->programType() == QSSGRenderShaderProgram::ProgramType::Graphics) {
            if (theInsertResult.value()) {
                const QSSGRef<QSSGRenderContext> &theContext(context->renderContext());
                theContext->setActiveShader(theInsertResult.value()->shader);
            }

            return QSSGMaterialOrComputeShader(theInsertResult.value());
        } else {
            const QSSGRef<QSSGRenderContext> &theContext(context->renderContext());
            theContext->setActiveShader(theProgram);
            return QSSGMaterialOrComputeShader(theProgram);
        }
    }
    return QSSGMaterialOrComputeShader();
}

void QSSGMaterialSystem::doApplyInstanceValue(QSSGRenderCustomMaterial &inMaterial,
                                                const QByteArray &inPropertyName,
                                                const QVariant &propertyValue,
                                                QSSGRenderShaderDataType inPropertyType,
                                                const QSSGRef<QSSGRenderShaderProgram> &inShader)
{
    Q_UNUSED(inMaterial)
    const QSSGRef<QSSGRenderShaderConstantBase> &theConstant = inShader->shaderConstant(inPropertyName);
    if (Q_LIKELY(theConstant)) {
        if (theConstant->isCompatibleType(inPropertyType)) {
            if (inPropertyType == QSSGRenderShaderDataType::Texture2D) {
                //                    StaticAssert<sizeof(QString) == sizeof(QSSGRenderTexture2DPtr)>::valid_expression();
                QSSGRenderCustomMaterial::TextureProperty *textureProperty = reinterpret_cast<QSSGRenderCustomMaterial::TextureProperty *>(propertyValue.value<void *>());
                QSSGRenderImage *image = textureProperty->texImage;
                if (image) {
                    const QString &imageSource = image->m_imagePath;
                    const QSSGRef<QSSGBufferManager> &theBufferManager(context->bufferManager());
                    QSSGRef<QSSGRenderTexture2D> theTexture;

                    if (!imageSource.isEmpty()) {
                        QSSGRenderImageTextureData theTextureData = theBufferManager->loadRenderImage(imageSource);
                        if (theTextureData.m_texture) {
                            theTexture = theTextureData.m_texture;
                            setTexture(inShader,
                                       inPropertyName,
                                       theTexture,
                                       textureProperty, // TODO: Should not be null!
                                       textureNeedsMips(textureProperty /* TODO: Should not be null! */, theTexture.data()));
                        }
                    }
                }
            } else {
                // TODO:
                switch (inPropertyType) {
                case QSSGRenderShaderDataType::Integer:
                    inShader->setPropertyValue(theConstant.data(), propertyValue.toInt());
                    break;
                case QSSGRenderShaderDataType::IntegerVec2:
                    inShader->setPropertyValue(theConstant.data(), propertyValue.value<qint32_2>());
                    break;
                case QSSGRenderShaderDataType::IntegerVec3:
                    inShader->setPropertyValue(theConstant.data(), propertyValue.value<qint32_3>());
                    break;
                case QSSGRenderShaderDataType::IntegerVec4:
                    inShader->setPropertyValue(theConstant.data(), propertyValue.value<qint32_4>());
                    break;
                case QSSGRenderShaderDataType::Boolean:
                    inShader->setPropertyValue(theConstant.data(), propertyValue.value<bool>());
                    break;
                case QSSGRenderShaderDataType::BooleanVec2:
                    inShader->setPropertyValue(theConstant.data(), propertyValue.value<bool_2>());
                    break;
                case QSSGRenderShaderDataType::BooleanVec3:
                    inShader->setPropertyValue(theConstant.data(), propertyValue.value<bool_3>());
                    break;
                case QSSGRenderShaderDataType::BooleanVec4:
                    inShader->setPropertyValue(theConstant.data(), propertyValue.value<bool_4>());
                    break;
                case QSSGRenderShaderDataType::Float:
                    inShader->setPropertyValue(theConstant.data(), propertyValue.value<float>());
                    break;
                case QSSGRenderShaderDataType::Vec2:
                    inShader->setPropertyValue(theConstant.data(), propertyValue.value<QVector2D>());
                    break;
                case QSSGRenderShaderDataType::Vec3:
                    inShader->setPropertyValue(theConstant.data(), propertyValue.value<QVector3D>());
                    break;
                case QSSGRenderShaderDataType::Vec4:
                    inShader->setPropertyValue(theConstant.data(), propertyValue.value<QVector4D>());
                    break;
                case QSSGRenderShaderDataType::Rgba:
                    inShader->setPropertyValue(theConstant.data(), propertyValue.value<QColor>());
                    break;
                case QSSGRenderShaderDataType::UnsignedInteger:
                    inShader->setPropertyValue(theConstant.data(), propertyValue.value<quint32>());
                    break;
                case QSSGRenderShaderDataType::UnsignedIntegerVec2:
                    inShader->setPropertyValue(theConstant.data(), propertyValue.value<quint32_2>());
                    break;
                case QSSGRenderShaderDataType::UnsignedIntegerVec3:
                    inShader->setPropertyValue(theConstant.data(), propertyValue.value<quint32_3>());
                    break;
                case QSSGRenderShaderDataType::UnsignedIntegerVec4:
                    inShader->setPropertyValue(theConstant.data(), propertyValue.value<quint32_4>());
                    break;
                case QSSGRenderShaderDataType::Matrix3x3:
                    inShader->setPropertyValue(theConstant.data(), propertyValue.value<QMatrix3x3>());
                    break;
                case QSSGRenderShaderDataType::Matrix4x4:
                    inShader->setPropertyValue(theConstant.data(), propertyValue.value<QMatrix4x4>());
                    break;
                case QSSGRenderShaderDataType::Texture2D:
                    inShader->setPropertyValue(theConstant.data(), *(reinterpret_cast<QSSGRenderTexture2D **>(propertyValue.value<void *>())));
                    break;
                case QSSGRenderShaderDataType::Texture2DHandle:
                    inShader->setPropertyValue(theConstant.data(),
                                               *(reinterpret_cast<QSSGRenderTexture2D ***>(propertyValue.value<void *>())));
                    break;
                case QSSGRenderShaderDataType::TextureCube:
                    inShader->setPropertyValue(theConstant.data(), *(reinterpret_cast<QSSGRenderTextureCube **>(propertyValue.value<void *>())));
                    break;
                case QSSGRenderShaderDataType::TextureCubeHandle:
                    inShader->setPropertyValue(theConstant.data(),
                                               *(reinterpret_cast<QSSGRenderTextureCube ***>(propertyValue.value<void *>())));
                    break;
                case QSSGRenderShaderDataType::Image2D:
                    inShader->setPropertyValue(theConstant.data(), *(reinterpret_cast<QSSGRenderImage2D **>(propertyValue.value<void *>())));
                    break;
                case QSSGRenderShaderDataType::DataBuffer:
                    inShader->setPropertyValue(theConstant.data(), *(reinterpret_cast<QSSGRenderDataBuffer **>(propertyValue.value<void *>())));
                    break;
                default:
                    Q_UNREACHABLE();
                }
            }
        } else {
            qCCritical(INVALID_OPERATION,
                       "CustomMaterial ApplyInstanceValue command datatype and "
                       "shader datatypes differ for property %s",
                       inPropertyName.constData());
            Q_ASSERT(false);
        }
    }
}

void QSSGMaterialSystem::applyInstanceValue(QSSGRenderCustomMaterial &inMaterial,
                                              const QSSGRef<QSSGRenderShaderProgram> &inShader,
                                              const dynamic::QSSGApplyInstanceValue &inCommand)
{
    // sanity check
    if (!inCommand.m_propertyName.isNull()) {
        const auto &properties = inMaterial.properties;
        const auto foundIt = std::find_if(properties.cbegin(), properties.cend(), [&inCommand](const QSSGRenderCustomMaterial::Property &prop) { return (prop.name == inCommand.m_propertyName); });
        if (foundIt != properties.cend())
            doApplyInstanceValue(inMaterial, foundIt->name, foundIt->value, foundIt->shaderDataType, inShader);
    } else {
        const auto &properties = inMaterial.properties;
        for (const auto &prop : properties)
            doApplyInstanceValue(inMaterial, prop.name, prop.value, prop.shaderDataType, inShader);

        const auto textProps = inMaterial.textureProperties;
        for (const auto &prop : textProps)
            doApplyInstanceValue(inMaterial, prop.name, QVariant::fromValue((void *)&prop), prop.shaderDataType, inShader);
    }
}

void QSSGMaterialSystem::applyBlending(const dynamic::QSSGApplyBlending &inCommand)
{
    const QSSGRef<QSSGRenderContext> &theContext(context->renderContext());

    theContext->setBlendingEnabled(true);

    QSSGRenderBlendFunctionArgument blendFunc = QSSGRenderBlendFunctionArgument(inCommand.m_srcBlendFunc,
                                                                                    inCommand.m_dstBlendFunc,
                                                                                    inCommand.m_srcBlendFunc,
                                                                                    inCommand.m_dstBlendFunc);

    QSSGRenderBlendEquationArgument blendEqu(QSSGRenderBlendEquation::Add, QSSGRenderBlendEquation::Add);

    theContext->setBlendFunction(blendFunc);
    theContext->setBlendEquation(blendEqu);
}

void QSSGMaterialSystem::applyCullMode(const dynamic::QSSGApplyCullMode &inCommand)
{
    const QSSGRef<QSSGRenderContext> &theContext(context->renderContext());
    theContext->setCullFaceMode(inCommand.m_cullMode);
}

void QSSGMaterialSystem::applyRenderStateValue(const dynamic::QSSGApplyRenderState &inCommand)
{
    const QSSGRef<QSSGRenderContext> &theContext(context->renderContext());

    switch (inCommand.m_renderState) {
    case QSSGRenderState::Blend:
        theContext->setBlendingEnabled(inCommand.m_enabled);
        break;
    case QSSGRenderState::DepthTest:
        theContext->setDepthTestEnabled(inCommand.m_enabled);
        break;
    case QSSGRenderState::StencilTest:
        theContext->setStencilTestEnabled(inCommand.m_enabled);
        break;
    case QSSGRenderState::ScissorTest:
        theContext->setScissorTestEnabled(inCommand.m_enabled);
        break;
    case QSSGRenderState::DepthWrite:
        theContext->setDepthWriteEnabled(inCommand.m_enabled);
        break;
    case QSSGRenderState::Multisample:
        theContext->setMultisampleEnabled(inCommand.m_enabled);
        break;
    case QSSGRenderState::CullFace:
        theContext->setCullingEnabled(inCommand.m_enabled);
        break;
    case QSSGRenderState::Unknown:
        Q_ASSERT(false);
        break;
    }
}

QSSGRef<QSSGRenderTexture2D> QSSGMaterialSystem::applyBufferValue(const QSSGRenderCustomMaterial &inMaterial,
                                                                        const QSSGRef<QSSGRenderShaderProgram> &inShader,
                                                                        const dynamic::QSSGApplyBufferValue &inCommand,
                                                                        const QSSGRef<QSSGRenderTexture2D> &inSourceTexture)
{
    QSSGRef<QSSGRenderTexture2D> theTexture = nullptr;

    if (!inCommand.m_bufferName.isNull()) {
        qint32 bufferIdx = findBuffer(inCommand.m_bufferName);
        if (bufferIdx < allocatedBuffers.size()) {
            const QSSGRenderCustomMaterialBuffer &theEntry = allocatedBuffers.at(bufferIdx);
            theTexture = theEntry.texture;
        } else {
            // we must have allocated the read target before
            qCCritical(INTERNAL_ERROR, "CustomMaterial: ApplyBufferValue: Failed to setup read target");
            Q_ASSERT(false);
        }
    } else {
        theTexture = inSourceTexture;
    }

    if (!inCommand.m_paramName.isNull()) {
        QSSGRef<QSSGRenderShaderConstantBase> theConstant = inShader->shaderConstant(inCommand.m_paramName);

        if (theConstant) {
            if (theConstant->getShaderConstantType() != QSSGRenderShaderDataType::Texture2D) {
                qCCritical(INVALID_OPERATION,
                           "CustomMaterial %s: Binding buffer to parameter %s that is not a texture",
                           inMaterial.className,
                           inCommand.m_paramName.constData());
                Q_ASSERT(false);
            } else {
                setTexture(inShader, inCommand.m_paramName, theTexture);
            }
        }
    }

    return (theTexture != nullptr) ? theTexture : nullptr;
}

void QSSGMaterialSystem::allocateBuffer(const dynamic::QSSGAllocateBuffer &inCommand, const QSSGRef<QSSGRenderFrameBuffer> &inTarget)
{
    QSSGTextureDetails theSourceTextureDetails;
    QSSGRef<QSSGRenderTexture2D> theTexture;
    // get color attachment we always assume at location 0
    if (inTarget) {
        QSSGRenderTextureOrRenderBuffer theSourceTexture = inTarget->attachment(QSSGRenderFrameBufferAttachment::Color0);
        // we need a texture
        if (theSourceTexture.hasTexture2D()) {
            theSourceTextureDetails = theSourceTexture.texture2D()->textureDetails();
        } else {
            qCCritical(INVALID_OPERATION, "CustomMaterial %s: Invalid source texture", inCommand.m_name.constData());
            Q_ASSERT(false);
            return;
        }
    } else {
        const QSSGRef<QSSGRenderContext> &theContext = context->renderContext();
        // if we allocate a buffer based on the default target use viewport to get the dimension
        QRect theViewport(theContext->viewport());
        theSourceTextureDetails.height = theViewport.height();
        theSourceTextureDetails.width = theViewport.width();
    }

    const qint32 theWidth = qint32(theSourceTextureDetails.width * inCommand.m_sizeMultiplier);
    const qint32 theHeight = qint32(theSourceTextureDetails.height * inCommand.m_sizeMultiplier);
    QSSGRenderTextureFormat theFormat = inCommand.m_format;
    if (theFormat == QSSGRenderTextureFormat::Unknown
            && theSourceTextureDetails.format != QSSGRenderTextureFormat::Unknown) {
        theFormat = theSourceTextureDetails.format;
    } else {
        theFormat = QSSGRenderTextureFormat::RGBA8;
    }
    const QSSGRef<QSSGResourceManager> &theResourceManager(context->resourceManager());
    // size intentionally requiried every loop;
    qint32 bufferIdx = findBuffer(inCommand.m_name);
    if (bufferIdx < allocatedBuffers.size()) {
        const QSSGRenderCustomMaterialBuffer &theEntry = allocatedBuffers.at(bufferIdx);
        QSSGTextureDetails theDetails = theEntry.texture->textureDetails();
        if (theDetails.width == theWidth && theDetails.height == theHeight && theDetails.format == theFormat) {
            theTexture = theEntry.texture;
        } else {
            releaseBuffer(bufferIdx);
        }
    }

    if (theTexture == nullptr) {
        QSSGRef<QSSGRenderFrameBuffer> theFB(theResourceManager->allocateFrameBuffer());
        QSSGRef<QSSGRenderTexture2D> theTexture(theResourceManager->allocateTexture2D(theWidth, theHeight, theFormat));
        theTexture->setMagFilter(inCommand.m_filterOp);
        theTexture->setMinFilter(static_cast<QSSGRenderTextureMinifyingOp>(inCommand.m_filterOp));
        theTexture->setTextureWrapS(inCommand.m_texCoordOp);
        theTexture->setTextureWrapT(inCommand.m_texCoordOp);
        theFB->attach(QSSGRenderFrameBufferAttachment::Color0, theTexture);
        allocatedBuffers.push_back(QSSGRenderCustomMaterialBuffer(inCommand.m_name, theFB, theTexture, inCommand.m_bufferFlags));
    }
}

QSSGRef<QSSGRenderFrameBuffer> QSSGMaterialSystem::bindBuffer(const QSSGRenderCustomMaterial &inMaterial, const dynamic::QSSGBindBuffer &inCommand, bool &outClearTarget, QVector2D &outDestSize)
{
    QSSGRef<QSSGRenderFrameBuffer> theBuffer;
    QSSGRef<QSSGRenderTexture2D> theTexture;

    // search for the buffer
    qint32 bufferIdx = findBuffer(inCommand.m_bufferName);
    if (bufferIdx < allocatedBuffers.size()) {
        theBuffer = allocatedBuffers[bufferIdx].frameBuffer;
        theTexture = allocatedBuffers[bufferIdx].texture;
    }

    if (theBuffer == nullptr) {
        qCCritical(INVALID_OPERATION,
                   "Material %s: Failed to find buffer %s for bind",
                   inMaterial.className,
                   inCommand.m_bufferName.constData());
        Q_ASSERT(false);
        return nullptr;
    }

    if (theTexture) {
        QSSGTextureDetails theDetails(theTexture->textureDetails());
        context->renderContext()->setViewport(QRect(0, 0, theDetails.width, theDetails.height));
        outDestSize = QVector2D(float(theDetails.width), float(theDetails.height));
        outClearTarget = inCommand.m_needsClear;
    }

    return theBuffer;
}

void QSSGMaterialSystem::computeScreenCoverage(QSSGCustomMaterialRenderContext &inRenderContext, qint32 *xMin, qint32 *yMin, qint32 *xMax, qint32 *yMax)
{
    const QSSGRef<QSSGRenderContext> &theContext(context->renderContext());
    QSSGBounds2BoxPoints outPoints;
    const float MaxFloat = std::numeric_limits<float>::max();
    QVector4D projMin(MaxFloat, MaxFloat, MaxFloat, MaxFloat);
    QVector4D projMax(-MaxFloat, -MaxFloat, -MaxFloat, -MaxFloat);

    // get points
    inRenderContext.subset.bounds.expand(outPoints);
    for (quint32 idx = 0; idx < 8; ++idx) {
        QVector4D homPoint(outPoints[idx], 1.0);
        QVector4D projPoint = mat44::transform(inRenderContext.modelViewProjection, homPoint);
        projPoint /= projPoint.w();

        if (projMin.x() > projPoint.x())
            projMin.setX(projPoint.x());
        if (projMin.y() > projPoint.y())
            projMin.setY(projPoint.y());
        if (projMin.z() > projPoint.z())
            projMin.setZ(projPoint.z());

        if (projMax.x() < projPoint.x())
            projMax.setX(projPoint.x());
        if (projMax.y() < projPoint.y())
            projMax.setY(projPoint.y());
        if (projMax.z() < projPoint.z())
            projMax.setZ(projPoint.z());
    }

    QRect theViewport(theContext->viewport());
    qint32 x1 = qint32(projMax.x() * (theViewport.width() / 2) + (theViewport.x() + (theViewport.width() / 2)));
    qint32 y1 = qint32(projMax.y() * (theViewport.height() / 2) + (theViewport.y() + (theViewport.height() / 2)));

    qint32 x2 = qint32(projMin.x() * (theViewport.width() / 2) + (theViewport.x() + (theViewport.width() / 2)));
    qint32 y2 = qint32(projMin.y() * (theViewport.height() / 2) + (theViewport.y() + (theViewport.height() / 2)));

    if (x1 > x2) {
        *xMin = x2;
        *xMax = x1;
    } else {
        *xMin = x1;
        *xMax = x2;
    }
    if (y1 > y2) {
        *yMin = y2;
        *yMax = y1;
    } else {
        *yMin = y1;
        *yMax = y2;
    }
}

void QSSGMaterialSystem::blitFramebuffer(QSSGCustomMaterialRenderContext &inRenderContext, const dynamic::QSSGApplyBlitFramebuffer &inCommand, const QSSGRef<QSSGRenderFrameBuffer> &inTarget)
{
    const QSSGRef<QSSGRenderContext> &theContext(context->renderContext());
    // we change the read/render targets here
    QSSGRenderContextScopedProperty<const QSSGRef<QSSGRenderFrameBuffer> &> __framebuffer(*theContext,
                                                                                        &QSSGRenderContext::renderTarget,
                                                                                        &QSSGRenderContext::setRenderTarget);
    // we may alter scissor
    QSSGRenderContextScopedProperty<bool> theScissorEnabled(*theContext,
                                                              &QSSGRenderContext::isScissorTestEnabled,
                                                              &QSSGRenderContext::setScissorTestEnabled);

    if (!inCommand.m_destBufferName.isNull()) {
        qint32 bufferIdx = findBuffer(inCommand.m_destBufferName);
        if (bufferIdx < allocatedBuffers.size()) {
            QSSGRenderCustomMaterialBuffer &theEntry(allocatedBuffers[bufferIdx]);
            theContext->setRenderTarget(theEntry.frameBuffer);
        } else {
            // we must have allocated the read target before
            qCCritical(INTERNAL_ERROR, "CustomMaterial: BlitFramebuffer: Failed to setup render target");
            Q_ASSERT(false);
        }
    } else {
        // our dest is the default render target
        theContext->setRenderTarget(inTarget);
    }

    if (!inCommand.m_sourceBufferName.isNull()) {
        qint32 bufferIdx = findBuffer(inCommand.m_sourceBufferName);
        if (bufferIdx < allocatedBuffers.size()) {
            QSSGRenderCustomMaterialBuffer &theEntry(allocatedBuffers[bufferIdx]);
            theContext->setReadTarget(theEntry.frameBuffer);
            theContext->setReadBuffer(QSSGReadFace::Color0);
        } else {
            // we must have allocated the read target before
            qCCritical(INTERNAL_ERROR, "CustomMaterial: BlitFramebuffer: Failed to setup read target");
            Q_ASSERT(false);
        }
    } else {
        // our source is the default read target
        // depending on what we render we assume color0 or back
        theContext->setReadTarget(inTarget);
        QSSGReadFace value = (inTarget) ? QSSGReadFace::Color0 : QSSGReadFace::Back;
        theContext->setReadBuffer(value);
    }

    QRect theViewport(theContext->viewport());
    theContext->setScissorTestEnabled(false);

    if (!useFastBlits) {
        // only copy sreen amount of pixels
        qint32 xMin, yMin, xMax, yMax;
        computeScreenCoverage(inRenderContext, &xMin, &yMin, &xMax, &yMax);

        // same dimension
        theContext->blitFramebuffer(xMin, yMin, xMax, yMax, xMin, yMin, xMax, yMax, QSSGRenderClearValues::Color, QSSGRenderTextureMagnifyingOp::Nearest);
    } else {
        // same dimension
        theContext->blitFramebuffer(theViewport.x(),
                                    theViewport.y(),
                                    theViewport.x() + theViewport.width(),
                                    theViewport.y() + theViewport.height(),
                                    theViewport.x(),
                                    theViewport.y(),
                                    theViewport.x() + theViewport.width(),
                                    theViewport.y() + theViewport.height(),
                                    QSSGRenderClearValues::Color,
                                    QSSGRenderTextureMagnifyingOp::Nearest);
    }
}

QSSGLayerGlobalRenderProperties QSSGMaterialSystem::getLayerGlobalRenderProperties(QSSGCustomMaterialRenderContext &inRenderContext)
{
    const QSSGRenderLayer &theLayer = inRenderContext.layer;
    const QSSGLayerRenderData &theData = inRenderContext.layerData;

    QVector<QVector3D> tempDirection;

    return QSSGLayerGlobalRenderProperties{ theLayer,
                const_cast<QSSGRenderCamera &>(inRenderContext.camera),
                theData.cameraDirection,
                const_cast<QVector<QSSGRenderLight *> &>(inRenderContext.lights),
                tempDirection,
                theData.shadowMapManager,
                inRenderContext.depthTexture,
                inRenderContext.aoTexture,
                theLayer.lightProbe,
                theLayer.lightProbe2,
                theLayer.probeHorizon,
                theLayer.probeBright,
                theLayer.probe2Window,
                theLayer.probe2Pos,
                theLayer.probe2Fade,
                theLayer.probeFov };
}

void QSSGMaterialSystem::renderPass(QSSGCustomMaterialRenderContext &inRenderContext, const QSSGRef<QSSGRenderCustomMaterialShader> &inShader, const QSSGRef<QSSGRenderTexture2D> &, const QSSGRef<QSSGRenderFrameBuffer> &inFrameBuffer, bool inRenderTargetNeedsClear, const QSSGRef<QSSGRenderInputAssembler> &inAssembler, quint32 inCount, quint32 inOffset, bool applyCullMode)
{
    const QSSGRef<QSSGRenderContext> &theContext(context->renderContext());
    theContext->setRenderTarget(inFrameBuffer);

    QVector4D clearColor(0.0, 0.0, 0.0, 0.0);
    QSSGRenderContextScopedProperty<QVector4D> __clearColor(*theContext,
                                                              &QSSGRenderContext::clearColor,
                                                              &QSSGRenderContext::setClearColor,
                                                              clearColor);
    if (inRenderTargetNeedsClear) {
        theContext->clear(QSSGRenderClearValues::Color);
    }

    const QSSGRef<QSSGMaterialShaderGeneratorInterface> &theMaterialGenerator(context->customMaterialShaderGenerator());

    theMaterialGenerator->setMaterialProperties(inShader->shader,
                                                inRenderContext.material,
                                                QVector2D(1.0, 1.0),
                                                inRenderContext.modelViewProjection,
                                                inRenderContext.normalMatrix,
                                                inRenderContext.modelMatrix,
                                                inRenderContext.firstImage,
                                                inRenderContext.opacity,
                                                getLayerGlobalRenderProperties(inRenderContext));

    // I think the prim type should always be fetched from the
    // current mesh subset setup because there you get the actual draw mode
    // for this frame
    QSSGRenderDrawMode theDrawMode = inAssembler->drawMode();

    // tesselation
    if (inRenderContext.subset.primitiveType == QSSGRenderDrawMode::Patches) {
        QVector2D camProps(inRenderContext.camera.clipNear, inRenderContext.camera.clipFar);
        theDrawMode = inRenderContext.subset.primitiveType;
        inShader->tessellation.m_edgeTessLevel.set(inRenderContext.subset.edgeTessFactor);
        inShader->tessellation.m_insideTessLevel.set(inRenderContext.subset.innerTessFactor);
        // the blend value is hardcoded
        inShader->tessellation.m_phongBlend.set(0.75);
        // this should finally be based on some user input
        inShader->tessellation.m_distanceRange.set(camProps);
        // enable culling
        inShader->tessellation.m_disableCulling.set(0.0);
    }

    if (inRenderContext.subset.wireframeMode) {
        QRect theViewport(theContext->viewport());
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

        inShader->viewportMatrix.set(vpMatrix);
    }

    theContext->setInputAssembler(inAssembler);
    if (applyCullMode)
        theContext->solveCullingOptions(inRenderContext.material.cullMode);

    quint32 count = inCount;
    quint32 offset = inOffset;

    theContext->draw(theDrawMode, count, offset);
}

void QSSGMaterialSystem::doRenderCustomMaterial(QSSGCustomMaterialRenderContext &inRenderContext,
                                                const QSSGRenderCustomMaterial &inMaterial,
                                                const ShaderFeatureSetList &inFeatureSet)
{
    const QSSGRef<QSSGRenderContext> &theContext = context->renderContext();
    QSSGRef<QSSGRenderCustomMaterialShader> theCurrentShader(nullptr);

    QRect theOriginalViewport(theContext->viewport());
    QSSGRef<QSSGRenderTexture2D> theCurrentSourceTexture;

    // for refrative materials we come from the transparent render path
    // but we do not want to do blending
    bool wasBlendingEnabled = theContext->isBlendingEnabled();
    if (inMaterial.m_hasRefraction)
        theContext->setBlendingEnabled(false);

    QSSGRenderContextScopedProperty<const QSSGRef<QSSGRenderFrameBuffer> &> __framebuffer(*theContext,
                                                                                        &QSSGRenderContext::renderTarget,
                                                                                        &QSSGRenderContext::setRenderTarget);
    const auto &originalTarget = __framebuffer.m_initialValue;
    QSSGRef<QSSGRenderFrameBuffer> theCurrentRenderTarget(originalTarget);
    QSSGRenderContextScopedProperty<QRect> __viewport(*theContext, &QSSGRenderContext::viewport, &QSSGRenderContext::setViewport);

    QVector2D theDestSize;
    bool theRenderTargetNeedsClear = false;
    bool applyMaterialCullMode = true;

    const auto &commands = inMaterial.commands;
    for (const auto &command : commands) {
        switch (command->m_type) {
        case dynamic::CommandType::AllocateBuffer:
            allocateBuffer(static_cast<const dynamic::QSSGAllocateBuffer &>(*command), originalTarget);
            break;
        case dynamic::CommandType::BindBuffer:
            theCurrentRenderTarget = bindBuffer(inMaterial,
                                                static_cast<const dynamic::QSSGBindBuffer &>(*command),
                                                theRenderTargetNeedsClear,
                                                theDestSize);
            break;
        case dynamic::CommandType::BindTarget:
            // Restore the previous render target and info.
            theCurrentRenderTarget = originalTarget;
            theContext->setViewport(theOriginalViewport);
            break;
        case dynamic::CommandType::BindShader: {
            theCurrentShader = nullptr;
            QSSGMaterialOrComputeShader theBindResult = bindShader(inRenderContext,
                                                                     inMaterial,
                                                                     static_cast<const dynamic::QSSGBindShader &>(*command),
                                                                     inFeatureSet);
            if (theBindResult.isMaterialShader())
                theCurrentShader = theBindResult.materialShader();
        } break;
        case dynamic::CommandType::ApplyInstanceValue:
            // we apply the property update explicitly at the render pass
            break;
        case dynamic::CommandType::Render:
            if (theCurrentShader) {
                renderPass(inRenderContext,
                           theCurrentShader,
                           theCurrentSourceTexture,
                           theCurrentRenderTarget,
                           theRenderTargetNeedsClear,
                           inRenderContext.subset.inputAssembler,
                           inRenderContext.subset.count,
                           inRenderContext.subset.offset,
                           applyMaterialCullMode);
            }
            // reset
            theRenderTargetNeedsClear = false;
            applyMaterialCullMode = true;
            break;
        case dynamic::CommandType::ApplyBlending:
            applyBlending(static_cast<const dynamic::QSSGApplyBlending &>(*command));
            break;
        case dynamic::CommandType::ApplyCullMode:
            applyCullMode(static_cast<const dynamic::QSSGApplyCullMode &>(*command));
            applyMaterialCullMode = false;
            break;
        case dynamic::CommandType::ApplyBufferValue:
            if (theCurrentShader)
                applyBufferValue(inMaterial,
                                 theCurrentShader->shader,
                                 static_cast<const dynamic::QSSGApplyBufferValue &>(*command),
                                 theCurrentSourceTexture);
            break;
        case dynamic::CommandType::ApplyBlitFramebuffer:
            blitFramebuffer(inRenderContext, static_cast<const dynamic::QSSGApplyBlitFramebuffer &>(*command), originalTarget);
            break;
        case dynamic::CommandType::ApplyRenderState:
            // TODO: The applyRenderStateValue() function is a very naive implementation
            applyRenderStateValue(static_cast<const dynamic::QSSGApplyRenderState &>(*command));
            break;
        default:
            Q_ASSERT(false);
            break;
        }
    }

    if (inMaterial.m_hasRefraction)
        theContext->setBlendingEnabled(wasBlendingEnabled);

    // Release any per-frame buffers
    for (qint32 idx = 0; idx < allocatedBuffers.size(); ++idx) {
        if (!allocatedBuffers[idx].flags.isSceneLifetime()) {
            releaseBuffer(idx);
            --idx;
        }
    }
}

QByteArray QSSGMaterialSystem::getShaderName(const QSSGRenderCustomMaterial &inMaterial)
{
    auto it = inMaterial.commands.cbegin();
    const auto end = inMaterial.commands.cend();
    for (; it != end; ++it) {
        if ((*it)->m_type == dynamic::CommandType::BindShader) {
            dynamic::QSSGBindShader *bindCommand = static_cast<dynamic::QSSGBindShader *>(*it);
            return bindCommand->m_shaderPath;
        }
    }

    Q_UNREACHABLE();
    return QByteArray();
}

void QSSGMaterialSystem::applyShaderPropertyValues(const QSSGRenderCustomMaterial &inMaterial, const QSSGRef<QSSGRenderShaderProgram> &inProgram)
{
    dynamic::QSSGApplyInstanceValue applier;
    applyInstanceValue(const_cast<QSSGRenderCustomMaterial &>(inMaterial), inProgram, applier);
}

void QSSGMaterialSystem::prepareDisplacementForRender(QSSGRenderCustomMaterial &inMaterial)
{
    // TODO: Shouldn't be needed anymore, as there's only one place where the values are updated
    if (inMaterial.m_displacementMap == nullptr)
        return;

    // our displacement mappin in MDL has fixed naming
    const auto &props = inMaterial.properties;
    for (const auto &prop : props) {
        if (prop.shaderDataType == QSSGRenderShaderDataType::Float && prop.name == QByteArrayLiteral("displaceAmount")) {
            bool ok = false;
            const float theValue = prop.value.toFloat(&ok); //*reinterpret_cast<const float *>(inMaterial.getDataSectionBegin() + thePropDefs[idx].offset);
            if (ok)
                inMaterial.m_displaceAmount = theValue;
        } else if (prop.shaderDataType == QSSGRenderShaderDataType::Vec3 && prop.name == QByteArrayLiteral("displace_tiling")) {
            const QVector3D theValue = prop.value.value<QVector3D>(); // = *reinterpret_cast<const QVector3D *>(inMaterial.getDataSectionBegin() + thePropDefs[idx].offset);
            if (theValue.x() != inMaterial.m_displacementMap->m_scale.x()
                || theValue.y() != inMaterial.m_displacementMap->m_scale.y()) {
                inMaterial.m_displacementMap->m_scale = QVector2D(theValue.x(), theValue.y());
                inMaterial.m_displacementMap->m_flags.setFlag(QSSGRenderImage::Flag::TransformDirty);
            }
        }
    }
}

void QSSGMaterialSystem::prepareMaterialForRender(QSSGRenderCustomMaterial &inMaterial)
{
    if (inMaterial.m_displacementMap) // inClass->m_hasDisplacement
        prepareDisplacementForRender(inMaterial);
}

// Returns true if the material is dirty and thus will produce a different render result
// than previously.  This effects things like progressive AA.
// TODO - return more information, specifically about transparency (object is transparent,
// object is completely transparent
bool QSSGMaterialSystem::prepareForRender(const QSSGRenderModel &, const QSSGRenderSubset &, QSSGRenderCustomMaterial &inMaterial)
{
    prepareMaterialForRender(inMaterial);
    const bool wasDirty = inMaterial.isDirty(); // TODO: Always dirty flag?

    return wasDirty;
}

// TODO - handle UIC specific features such as vertex offsets for prog-aa and opacity.
void QSSGMaterialSystem::renderSubset(QSSGCustomMaterialRenderContext &inRenderContext, const ShaderFeatureSetList &inFeatureSet)
{
    // Ensure that our overall render context comes back no matter what the client does.
    QSSGRenderContextScopedProperty<QSSGRenderBlendFunctionArgument> __blendFunction(*context->renderContext(),
                                                                                         &QSSGRenderContext::blendFunction,
                                                                                         &QSSGRenderContext::setBlendFunction,
                                                                                         QSSGRenderBlendFunctionArgument());
    QSSGRenderContextScopedProperty<QSSGRenderBlendEquationArgument> __blendEquation(*context->renderContext(),
                                                                                         &QSSGRenderContext::blendEquation,
                                                                                         &QSSGRenderContext::setBlendEquation,
                                                                                         QSSGRenderBlendEquationArgument());

    QSSGRenderContextScopedProperty<bool> theBlendEnabled(*context->renderContext(),
                                                            &QSSGRenderContext::isBlendingEnabled,
                                                            &QSSGRenderContext::setBlendingEnabled);

    doRenderCustomMaterial(inRenderContext, inRenderContext.material, inFeatureSet);
}

bool QSSGMaterialSystem::renderDepthPrepass(const QMatrix4x4 &inMVP, const QSSGRenderCustomMaterial &inMaterial, const QSSGRenderSubset &inSubset)
{
    const auto &commands = inMaterial.commands;
    auto it = commands.cbegin();
    const auto end = commands.cend();
    TShaderAndFlags thePrepassShader;
    for (; it != end && thePrepassShader.first == nullptr; ++it) {
        if ((*it)->m_type == dynamic::CommandType::BindShader) {
            const dynamic::QSSGBindShader &theBindCommand = static_cast<const dynamic::QSSGBindShader &>(*(*it));
            thePrepassShader = context->dynamicObjectSystem()->getDepthPrepassShader(theBindCommand.m_shaderPath,
                                                                                     QByteArray(),
                                                                                     ShaderFeatureSetList());
        }
    }

    if (thePrepassShader.first == nullptr)
        return false;

    const QSSGRef<QSSGRenderContext> &theContext = context->renderContext();
    const QSSGRef<QSSGRenderShaderProgram> &theProgram = thePrepassShader.first;
    theContext->setActiveShader(theProgram);
    theProgram->setPropertyValue("modelViewProjection", inMVP);
    theContext->setInputAssembler(inSubset.inputAssemblerPoints);
    theContext->draw(QSSGRenderDrawMode::Lines, inSubset.posVertexBuffer->numVertexes(), 0);
    return true;
}

void QSSGMaterialSystem::endFrame()
{
#ifdef QQ3D_UNUSED_TIMER
    if (lastFrameTime.elapsed() != 0)
        msSinceLastFrame = lastFrameTime.elapsed()/1000000.0f;

    lastFrameTime.restart();
#endif
}

void QSSGMaterialSystem::setRenderContextInterface(QSSGRenderContextInterface *inContext)
{
    context = inContext;

    // check for fast blits
    const QSSGRef<QSSGRenderContext> &theContext = context->renderContext();
    useFastBlits = theContext->renderBackendCap(QSSGRenderBackend::QSSGRenderBackendCaps::FastBlits);
}

QT_END_NAMESPACE
