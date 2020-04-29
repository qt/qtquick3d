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
void QSSGCustomMaterialVertexPipeline::beginVertexGeneration(quint32 displacementImageIdx, QSSGRenderableImage *displacementImage)
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
        generateUVCoords(0, m_materialGenerator->key());
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
        generateWorldNormal(m_materialGenerator->key());
        generateObjectNormal();
        generateVarTangentAndBinormal(m_materialGenerator->key());
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

void QSSGCustomMaterialVertexPipeline::generateUVCoords(quint32 inUVSet, const QSSGShaderDefaultMaterialKey &inKey)
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

    doGenerateUVCoords(inUVSet, inKey);
}

void QSSGCustomMaterialVertexPipeline::generateWorldNormal(const QSSGShaderDefaultMaterialKey &inKey)
{
    if (setCode(GenerationFlag::WorldNormal))
        return;
    addInterpolationParameter("varNormal", "vec3");
    doGenerateWorldNormal(inKey);
}

void QSSGCustomMaterialVertexPipeline::generateObjectNormal()
{
    if (setCode(GenerationFlag::ObjectNormal))
        return;
    doGenerateObjectNormal();
}

void QSSGCustomMaterialVertexPipeline::generateVarTangentAndBinormal(const QSSGShaderDefaultMaterialKey &inKey)
{
    if (setCode(GenerationFlag::TangentBinormal))
        return;
    addInterpolationParameter("varTangent", "vec3");
    addInterpolationParameter("varBinormal", "vec3");
    addInterpolationParameter("varObjTangent", "vec3");
    addInterpolationParameter("varObjBinormal", "vec3");
    doGenerateVarTangentAndBinormal(inKey);
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

void QSSGCustomMaterialVertexPipeline::doGenerateUVCoords(quint32 inUVSet, const QSSGShaderDefaultMaterialKey &inKey)
{
    Q_ASSERT(inUVSet == 0 || inUVSet == 1);

    Q_UNUSED(inKey); // ###

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

void QSSGCustomMaterialVertexPipeline::doGenerateWorldNormal(const QSSGShaderDefaultMaterialKey &inKey)
{
    Q_UNUSED(inKey); // ###

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

void QSSGCustomMaterialVertexPipeline::doGenerateVarTangentAndBinormal(const QSSGShaderDefaultMaterialKey &inKey)
{
    Q_UNUSED(inKey); // ###

    vertex().addIncoming("attr_textan", "vec3");
    vertex().addIncoming("attr_binormal", "vec3");

    vertex() << "\tvarTangent = normalMatrix * attr_textan;"
             << "\n"
             << "\tvarBinormal = normalMatrix * attr_binormal;"
             << "\n";

    vertex() << "\tvarObjTangent = attr_textan;"
             << "\n"
             << "\tvarObjBinormal = attr_binormal;"
             << "\n";
}

void QSSGCustomMaterialVertexPipeline::doGenerateVertexColor(const QSSGShaderDefaultMaterialKey &inKey)
{
    Q_UNUSED(inKey); // ###

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
    size_t m_hashCode;
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

size_t qHash(const QSSGShaderMapKey &key)
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

    const bool isYUpInFramebuffer = context->renderContext()->rhiContext()->isValid()
            ? context->renderContext()->rhiContext()->rhi()->isYUpInFramebuffer()
            : true;

    return QSSGLayerGlobalRenderProperties{ theLayer,
                const_cast<QSSGRenderCamera &>(inRenderContext.camera),
                theData.cameraDirection,
                const_cast<QVector<QSSGRenderLight *> &>(inRenderContext.lights),
                tempDirection,
                theData.shadowMapManager,
                inRenderContext.rhiDepthTexture,
                inRenderContext.rhiAoTexture,
                theLayer.lightProbe,
                theLayer.lightProbe2,
                theLayer.probeHorizon,
                theLayer.probeBright,
                theLayer.probe2Window,
                theLayer.probe2Pos,
                theLayer.probe2Fade,
                theLayer.probeFov,
                isYUpInFramebuffer };
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

void QSSGMaterialSystem::endFrame()
{
}

void QSSGMaterialSystem::setRenderContextInterface(QSSGRenderContextInterface *inContext)
{
    context = inContext;

    // check for fast blits
    const QSSGRef<QSSGRenderContext> &theContext = context->renderContext();
    useFastBlits = theContext->renderBackendCap(QSSGRenderBackend::QSSGRenderBackendCaps::FastBlits);
}

QSSGRef<QSSGRhiShaderStagesWithResources> QSSGMaterialSystem::prepareRhiShader(QSSGCustomMaterialRenderContext &inRenderContext,
                                                                               const QSSGRenderCustomMaterial &inMaterial,
                                                                               const dynamic::QSSGBindShader &inCommand,
                                                                               const ShaderFeatureSetList &inFeatureSet)
{
    const QSSGShaderMapKey skey = QSSGShaderMapKey(TStrStrPair(inCommand.m_shaderPath, inCommand.m_shaderDefine),
                                                   inFeatureSet,
                                                   TessellationModeValues::NoTessellation,
                                                   false,
                                                   inRenderContext.materialKey);

    QSSGShaderPreprocessorFeature noFragOutputFeature("NO_FRAG_OUTPUT", true);
    ShaderFeatureSetList features(inFeatureSet);
    features.push_back(noFragOutputFeature);

    QSSGRef<QSSGRhiShaderStagesWithResources> result;
    auto it = rhiShaderMap.find(skey);
    if (it == rhiShaderMap.end()) {
        const QSSGRef<QSSGMaterialShaderGeneratorInterface> &theMaterialGenerator(context->customMaterialShaderGenerator());
        QSSGCustomMaterialVertexPipeline thePipeline(context, inRenderContext.model.tessellationMode);
        QSSGRef<QSSGRhiShaderStages> shaderStages = theMaterialGenerator->generateRhiShaderStages(inMaterial,
                                                                                                  inRenderContext.materialKey,
                                                                                                  thePipeline,
                                                                                                  features,
                                                                                                  inRenderContext.lights,
                                                                                                  inRenderContext.firstImage,
                                                                                                  (inMaterial.m_hasTransparency || inMaterial.m_hasRefraction),
                                                                                                  QByteArrayLiteral("custom material pipeline-- "),
                                                                                                  inCommand.m_shaderPath);
        if (shaderStages)
            result = QSSGRhiShaderStagesWithResources::fromShaderStages(shaderStages);
        // insert it no matter what, no point in trying over and over again
        rhiShaderMap.insert(skey, result);
    } else {
        result = it.value();
    }
    return result;
}

static void fillSrcTargetBlendFactor(QRhiGraphicsPipeline::BlendFactor *dst, QSSGRenderSrcBlendFunc src)
{
    switch (src) {
    case QSSGRenderSrcBlendFunc::Zero:
        *dst = QRhiGraphicsPipeline::Zero;
        break;
    case QSSGRenderSrcBlendFunc::One:
        *dst = QRhiGraphicsPipeline::One;
        break;
    case QSSGRenderSrcBlendFunc::SrcColor:
        *dst = QRhiGraphicsPipeline::SrcColor;
        break;
    case QSSGRenderSrcBlendFunc::OneMinusSrcColor:
        *dst = QRhiGraphicsPipeline::OneMinusSrcColor;
        break;
    case QSSGRenderSrcBlendFunc::DstColor:
        *dst = QRhiGraphicsPipeline::DstColor;
        break;
    case QSSGRenderSrcBlendFunc::OneMinusDstColor:
        *dst = QRhiGraphicsPipeline::OneMinusDstColor;
        break;
    case QSSGRenderSrcBlendFunc::SrcAlpha:
        *dst = QRhiGraphicsPipeline::SrcAlpha;
        break;
    case QSSGRenderSrcBlendFunc::OneMinusSrcAlpha:
        *dst = QRhiGraphicsPipeline::OneMinusSrcAlpha;
        break;
    case QSSGRenderSrcBlendFunc::DstAlpha:
        *dst = QRhiGraphicsPipeline::DstAlpha;
        break;
    case QSSGRenderSrcBlendFunc::OneMinusDstAlpha:
        *dst = QRhiGraphicsPipeline::OneMinusDstAlpha;
        break;
    case QSSGRenderSrcBlendFunc::ConstantColor:
        *dst = QRhiGraphicsPipeline::ConstantColor;
        break;
    case QSSGRenderSrcBlendFunc::OneMinusConstantColor:
        *dst = QRhiGraphicsPipeline::OneMinusConstantColor;
        break;
    case QSSGRenderSrcBlendFunc::ConstantAlpha:
        *dst = QRhiGraphicsPipeline::ConstantAlpha;
        break;
    case QSSGRenderSrcBlendFunc::OneMinusConstantAlpha:
        *dst = QRhiGraphicsPipeline::OneMinusConstantAlpha;
        break;
    case QSSGRenderSrcBlendFunc::SrcAlphaSaturate:
        *dst = QRhiGraphicsPipeline::SrcAlphaSaturate;
        break;
    default:
        break;
    }
}

static void fillDstTargetBlendFactor(QRhiGraphicsPipeline::BlendFactor *dst, QSSGRenderDstBlendFunc src)
{
    switch (src) {
    case QSSGRenderDstBlendFunc::Zero:
        *dst = QRhiGraphicsPipeline::Zero;
        break;
    case QSSGRenderDstBlendFunc::One:
        *dst = QRhiGraphicsPipeline::One;
        break;
    case QSSGRenderDstBlendFunc::SrcColor:
        *dst = QRhiGraphicsPipeline::SrcColor;
        break;
    case QSSGRenderDstBlendFunc::OneMinusSrcColor:
        *dst = QRhiGraphicsPipeline::OneMinusSrcColor;
        break;
    case QSSGRenderDstBlendFunc::DstColor:
        *dst = QRhiGraphicsPipeline::DstColor;
        break;
    case QSSGRenderDstBlendFunc::OneMinusDstColor:
        *dst = QRhiGraphicsPipeline::OneMinusDstColor;
        break;
    case QSSGRenderDstBlendFunc::SrcAlpha:
        *dst = QRhiGraphicsPipeline::SrcAlpha;
        break;
    case QSSGRenderDstBlendFunc::OneMinusSrcAlpha:
        *dst = QRhiGraphicsPipeline::OneMinusSrcAlpha;
        break;
    case QSSGRenderDstBlendFunc::DstAlpha:
        *dst = QRhiGraphicsPipeline::DstAlpha;
        break;
    case QSSGRenderDstBlendFunc::OneMinusDstAlpha:
        *dst = QRhiGraphicsPipeline::OneMinusDstAlpha;
        break;
    case QSSGRenderDstBlendFunc::ConstantColor:
        *dst = QRhiGraphicsPipeline::ConstantColor;
        break;
    case QSSGRenderDstBlendFunc::OneMinusConstantColor:
        *dst = QRhiGraphicsPipeline::OneMinusConstantColor;
        break;
    case QSSGRenderDstBlendFunc::ConstantAlpha:
        *dst = QRhiGraphicsPipeline::ConstantAlpha;
        break;
    case QSSGRenderDstBlendFunc::OneMinusConstantAlpha:
        *dst = QRhiGraphicsPipeline::OneMinusConstantAlpha;
        break;
    default:
        break;
    }
}

static const QRhiShaderResourceBinding::StageFlags VISIBILITY_ALL =
        QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage;

void QSSGMaterialSystem::prepareRhiSubset(QSSGCustomMaterialRenderContext &customMaterialContext,
                                          const ShaderFeatureSetList &featureSet,
                                          QSSGRhiGraphicsPipelineState *ps,
                                          QSSGCustomMaterialRenderable &renderable)
{
    const QSSGRenderCustomMaterial &material(customMaterialContext.material);
    QSSGRef<QSSGRhiShaderStagesWithResources> shaderPipeline;
    QSSGCullFaceMode cullMode = material.cullMode;
    QRhiGraphicsPipeline::TargetBlend blend;

    //qDebug("Prepare custom material %p with commands:", &material);
    for (const dynamic::QSSGCommand *command : qAsConst(material.commands)) {
        //qDebug("  %s", command->typeAsString());
        switch (command->m_type) {
        case dynamic::CommandType::BindShader:
            shaderPipeline = prepareRhiShader(customMaterialContext,
                                              material,
                                              static_cast<const dynamic::QSSGBindShader &>(*command),
                                              featureSet);
            if (shaderPipeline)
                shaderPipeline->resetExtraTextures();
            break;

        case dynamic::CommandType::ApplyInstanceValue:
            break; // nothing to do here, handled by setRhiMaterialProperties()

        case dynamic::CommandType::ApplyBlending:
        {
            const dynamic::QSSGApplyBlending &blendParams(static_cast<const dynamic::QSSGApplyBlending &>(*command));
            blend.enable = true;
            fillSrcTargetBlendFactor(&blend.srcColor, blendParams.m_srcBlendFunc);
            fillSrcTargetBlendFactor(&blend.srcAlpha, blendParams.m_srcBlendFunc);
            fillDstTargetBlendFactor(&blend.dstColor, blendParams.m_dstBlendFunc);
            fillDstTargetBlendFactor(&blend.dstAlpha, blendParams.m_dstBlendFunc);
        }
            break;

        case dynamic::CommandType::ApplyCullMode:
            cullMode = static_cast<const dynamic::QSSGApplyCullMode &>(*command).m_cullMode;
            break;

        default:
            break;
        }
    }

    if (shaderPipeline) {
        ps->shaderStages = shaderPipeline->stages();

        QSSGMaterialShaderGeneratorInterface *materialGenerator = context->customMaterialShaderGenerator().data();
        materialGenerator->setRhiMaterialProperties(shaderPipeline,
                                                    ps,
                                                    material,
                                                    QVector2D(1.0, 1.0),
                                                    customMaterialContext.modelViewProjection,
                                                    customMaterialContext.normalMatrix,
                                                    customMaterialContext.modelMatrix,
                                                    customMaterialContext.firstImage,
                                                    customMaterialContext.opacity,
                                                    getLayerGlobalRenderProperties(customMaterialContext));

        //shaderPipeline->dumpUniforms();

        QSSGRhiContext *rhiCtx = context->renderContext()->rhiContext().data();
        QRhiCommandBuffer *cb = rhiCtx->commandBuffer();

        ps->samples = rhiCtx->mainPassSampleCount();

        ps->cullMode = QSSGRhiGraphicsPipelineState::toCullMode(cullMode);

        ps->targetBlend = blend;

        ps->ia = customMaterialContext.subset.rhi.ia;
        ps->ia.bakeVertexInputLocations(*shaderPipeline);

        QRhiResourceUpdateBatch *resourceUpdates = rhiCtx->rhi()->nextResourceUpdateBatch();
        QSSGRhiUniformBufferSet &uniformBuffers(rhiCtx->uniformBufferSet({ &customMaterialContext.layer,
                                                                           &customMaterialContext.model,
                                                                           &material,
                                                                           QSSGRhiUniformBufferSetKey::Main }));
        shaderPipeline->bakeMainUniformBuffer(&uniformBuffers.ubuf, resourceUpdates);

        // non-area lights
        shaderPipeline->bakeLightsUniformBuffer(QSSGRhiShaderStagesWithResources::LightBuffer0,
                                                &uniformBuffers.lightsUbuf0,
                                                resourceUpdates);
        // area lights
        shaderPipeline->bakeLightsUniformBuffer(QSSGRhiShaderStagesWithResources::LightBuffer1,
                                                &uniformBuffers.lightsUbuf1,
                                                resourceUpdates);

        QRhiTexture *dummyTexture = rhiCtx->dummyTexture({}, resourceUpdates);
        QRhiTexture *dummyCubeTexture = rhiCtx->dummyTexture(QRhiTexture::CubeMap, resourceUpdates);

        cb->resourceUpdate(resourceUpdates);

        QSSGRhiContext::ShaderResourceBindingList bindings;

        bindings.append(QRhiShaderResourceBinding::uniformBuffer(0, VISIBILITY_ALL, uniformBuffers.ubuf));

        // cbBufferLights
        bindings.append(QRhiShaderResourceBinding::uniformBuffer(1, VISIBILITY_ALL, uniformBuffers.lightsUbuf0));

        // cbBufferAreaLights
        bindings.append(QRhiShaderResourceBinding::uniformBuffer(2, VISIBILITY_ALL, uniformBuffers.lightsUbuf1));

        QVector<QShaderDescription::InOutVariable> samplerVars =
                shaderPipeline->stages()->fragmentStage()->shader().description().combinedImageSamplers();
        int maxSamplerBinding = -1;
        for (const QShaderDescription::InOutVariable &var : samplerVars)
            maxSamplerBinding = qMax(maxSamplerBinding, var.binding);

        // Will need to set unused image-samplers to something dummy
        // because the shader code contains all custom property textures,
        // and not providing a binding for all of them is invalid with some
        // graphics APIs (and will need a real texture because setting a
        // null handle or similar is not permitted with some of them so the
        // srb does not accept null QRhiTextures either; but first let's
        // figure out what bindings are unused in this frame)
        QBitArray samplerBindingsSpecified(maxSamplerBinding + 1);

        if (shaderPipeline->lightProbeTexture()) {
            int binding = shaderPipeline->bindingForTexture(QLatin1String("lightProbe"));
            if (binding >= 0) {
                samplerBindingsSpecified.setBit(binding);
                QPair<QSSGRenderTextureCoordOp, QSSGRenderTextureCoordOp> tiling = shaderPipeline->lightProbeTiling();
                QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::Linear, // enables mipmapping
                                                         toRhi(tiling.first), toRhi(tiling.second) });
                bindings.append(QRhiShaderResourceBinding::sampledTexture(binding,
                                                                          QRhiShaderResourceBinding::FragmentStage,
                                                                          shaderPipeline->lightProbeTexture(), sampler));
            } else {
                qWarning("Could not find sampler for light probe");
            }
        }

        if (shaderPipeline->depthTexture()) {
            int binding = shaderPipeline->bindingForTexture(QLatin1String("depthTexture"));
            if (binding >= 0) {
                samplerBindingsSpecified.setBit(binding);
                // nearest min/mag, no mipmap
                QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Nearest, QRhiSampler::Nearest, QRhiSampler::None,
                                                         QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge });
                bindings.append(QRhiShaderResourceBinding::sampledTexture(binding,
                                                                          QRhiShaderResourceBinding::FragmentStage,
                                                                          shaderPipeline->depthTexture(), sampler));
            } // else ignore, not an error
        }

        if (shaderPipeline->ssaoTexture()) {
            int binding = shaderPipeline->bindingForTexture(QLatin1String("aoTexture"));
            if (binding >= 0) {
                samplerBindingsSpecified.setBit(binding);
                // linear min/mag, no mipmap
                QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                                         QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge });
                bindings.append(QRhiShaderResourceBinding::sampledTexture(binding,
                                                                          QRhiShaderResourceBinding::FragmentStage,
                                                                          shaderPipeline->ssaoTexture(), sampler));
            } // else ignore, not an error
        }

        QVarLengthArray<QRhiShaderResourceBinding::TextureAndSampler, 16> texSamplers;
        QRhiSampler *dummySampler = rhiCtx->sampler({ QRhiSampler::Nearest, QRhiSampler::Nearest, QRhiSampler::None,
                                                      QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge });

        for (int i = 0, ie = shaderPipeline->shadowMapArrayCount(); i < ie; ++i) {
            QSSGRhiShadowMapArrayProperties &p(shaderPipeline->shadowMapArrayAt(i));
            if (p.shadowMapTextures.isEmpty())
                continue;
            if (p.cachedBinding < 0) {
                const QVector<int> *arrayDims = nullptr;
                p.cachedBinding = shaderPipeline->bindingForTexture(p.shadowMapArrayUniformName, &arrayDims);
                if (arrayDims && !arrayDims->isEmpty()) {
                    p.shaderArrayDim = arrayDims->first();
                } else {
                    qWarning("No array dimension for array of shadow map textures '%s', this should not happen.",
                             p.shadowMapArrayUniformName.constData());
                    continue;
                }
            }
            if (p.cachedBinding < 0) {
                qWarning("No combined image sampler for array of shadow map textures '%s'",
                         p.shadowMapArrayUniformName.constData());
                continue;
            }
            QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                                     QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge });
            samplerBindingsSpecified.setBit(p.cachedBinding);
            texSamplers.clear();
            for (QRhiTexture *texture : p.shadowMapTextures)
                texSamplers.append({ texture, sampler });
            // fill the rest with dummy texture-sampler pairs (the array must always be fully specified)
            if (texSamplers.count() < p.shaderArrayDim) {
                for (int dummyIdx = texSamplers.count(); dummyIdx < p.shaderArrayDim; ++dummyIdx)
                    texSamplers.append({ p.isCubemap ? dummyCubeTexture : dummyTexture, dummySampler });
            }
            bindings.append(QRhiShaderResourceBinding::sampledTextures(p.cachedBinding,
                                                                       QRhiShaderResourceBinding::FragmentStage,
                                                                       texSamplers.count(),
                                                                       texSamplers.constData()));
        }

        if (maxSamplerBinding >= 0) {
            int customTexCount = shaderPipeline->extraTextureCount();
            for (int i = 0; i < customTexCount; ++i) {
                const QSSGRhiTexture &t(shaderPipeline->extraTextureAt(i));
                const int samplerBinding = shaderPipeline->bindingForTexture(t.name);
                if (samplerBinding >= 0) {
                    samplerBindingsSpecified.setBit(samplerBinding);
                    QRhiSampler *sampler = rhiCtx->sampler(t.samplerDesc);
                    bindings.append(QRhiShaderResourceBinding::sampledTexture(samplerBinding,
                                                                              QRhiShaderResourceBinding::FragmentStage,
                                                                              t.texture,
                                                                              sampler));
                }
            }

            for (const QShaderDescription::InOutVariable &var : samplerVars) {
                QRhiTexture *t = var.type == QShaderDescription::SamplerCube ? dummyCubeTexture : dummyTexture;
                texSamplers.clear();
                const int count = var.arrayDims.isEmpty() ? 1 : var.arrayDims.first();
                for (int i = 0; i < count; ++i)
                    texSamplers.append({ t, dummySampler });
                if (!samplerBindingsSpecified.testBit(var.binding)) {
                    bindings.append(QRhiShaderResourceBinding::sampledTextures(var.binding,
                                                                               QRhiShaderResourceBinding::FragmentStage,
                                                                               texSamplers.count(),
                                                                               texSamplers.constData()));
                }
            }
        }

        QRhiShaderResourceBindings *srb = rhiCtx->srb(bindings);

        const QSSGGraphicsPipelineStateKey pipelineKey { *ps, rhiCtx->mainRenderPassDescriptor(), srb };
        renderable.rhiRenderData.mainPass.pipeline = rhiCtx->pipeline(pipelineKey);
        renderable.rhiRenderData.mainPass.srb = srb;
    }
}

void QSSGMaterialSystem::doApplyRhiInstanceValue(const QSSGRenderCustomMaterial &inMaterial,
                                                 const QByteArray &inPropertyName,
                                                 const QVariant &propertyValue,
                                                 QSSGRenderShaderDataType inPropertyType,
                                                 const QSSGRef<QSSGRhiShaderStagesWithResources> &shaderPipeline)
{
    Q_UNUSED(inMaterial);

    if (inPropertyType == QSSGRenderShaderDataType::Texture2D) {
        QSSGRenderCustomMaterial::TextureProperty *textureProperty =
                reinterpret_cast<QSSGRenderCustomMaterial::TextureProperty *>(propertyValue.value<void *>());
        QSSGRenderImage *image = textureProperty->texImage;
        if (image) {
            const QString &imageSource = image->m_imagePath;
            const QSSGRef<QSSGBufferManager> &theBufferManager(context->bufferManager());
            QSSGRef<QSSGRenderTexture2D> theTexture;
            if (!imageSource.isEmpty()) {
                QSSGRenderImageTextureData theTextureData = theBufferManager->loadRenderImage(imageSource);
                if (theTextureData.m_rhiTexture) {
                    const QSSGRhiTexture t = {
                        inPropertyName,
                        theTextureData.m_rhiTexture,
                        { toRhi(textureProperty->minFilterType),
                          toRhi(textureProperty->magFilterType),
                          theTextureData.m_mipmaps > 0 ? QRhiSampler::Linear : QRhiSampler::None,
                          toRhi(textureProperty->clampType),
                          toRhi(textureProperty->clampType) }
                    };
                    shaderPipeline->addExtraTexture(t);
                }
            }
        }
    } else {
        shaderPipeline->setUniformValue(inPropertyName, propertyValue, inPropertyType);
#if 0
        case QSSGRenderShaderDataType::Texture2DHandle: // 2D sampler array
            inShader->setPropertyValue(theConstant.data(),
                                       *(reinterpret_cast<QSSGRenderTexture2D ***>(propertyValue.value<void *>())));
            break;
        case QSSGRenderShaderDataType::TextureCube:
            inShader->setPropertyValue(theConstant.data(), *(reinterpret_cast<QSSGRenderTextureCube **>(propertyValue.value<void *>())));
            break;
        case QSSGRenderShaderDataType::TextureCubeHandle: // cube sampler array
            inShader->setPropertyValue(theConstant.data(),
                                       *(reinterpret_cast<QSSGRenderTextureCube ***>(propertyValue.value<void *>())));
            break;
        case QSSGRenderShaderDataType::Image2D:
            inShader->setPropertyValue(theConstant.data(), *(reinterpret_cast<QSSGRenderImage2D **>(propertyValue.value<void *>())));
            break;
        case QSSGRenderShaderDataType::DataBuffer:
            inShader->setPropertyValue(theConstant.data(), *(reinterpret_cast<QSSGRenderDataBuffer **>(propertyValue.value<void *>())));
            break;
#endif
    }
}

void QSSGMaterialSystem::applyRhiInstanceValue(const QSSGRenderCustomMaterial &material,
                                               const QSSGRef<QSSGRhiShaderStagesWithResources> &shaderPipeline,
                                               const dynamic::QSSGApplyInstanceValue &command)
{
    if (!command.m_propertyName.isNull()) {
        const auto &properties = material.properties;
        const auto foundIt = std::find_if(properties.cbegin(), properties.cend(),
                                          [&command](const QSSGRenderCustomMaterial::Property &prop) { return (prop.name == command.m_propertyName); });
        if (foundIt != properties.cend())
            doApplyRhiInstanceValue(material, foundIt->name, foundIt->value, foundIt->shaderDataType, shaderPipeline);
    } else {
        const auto &properties = material.properties;
        for (const auto &prop : properties)
            doApplyRhiInstanceValue(material, prop.name, prop.value, prop.shaderDataType, shaderPipeline);

        const auto textProps = material.textureProperties;
        for (const auto &prop : textProps)
            doApplyRhiInstanceValue(material, prop.name, QVariant::fromValue((void *)&prop), prop.shaderDataType, shaderPipeline);
    }
}

void QSSGMaterialSystem::applyRhiShaderPropertyValues(const QSSGRenderCustomMaterial &material,
                                                      const QSSGRef<QSSGRhiShaderStagesWithResources> &shaderPipeline)
{
    dynamic::QSSGApplyInstanceValue allProperties;
    applyRhiInstanceValue(material, shaderPipeline, allProperties);
}

void QSSGMaterialSystem::recordRhiSubsetDrawCalls(QSSGRhiContext *rhiCtx,
                                                  QSSGCustomMaterialRenderable &renderable,
                                                  QSSGLayerRenderData &inData,
                                                  bool *needsSetViewport)
{
    QRhiGraphicsPipeline *ps = renderable.rhiRenderData.mainPass.pipeline;
    QRhiShaderResourceBindings *srb = renderable.rhiRenderData.mainPass.srb;
    if (!ps || !srb)
        return;

    QRhiBuffer *vertexBuffer = renderable.subset.rhi.ia.vertexBuffer->buffer();
    QRhiBuffer *indexBuffer = renderable.subset.rhi.ia.indexBuffer ? renderable.subset.rhi.ia.indexBuffer->buffer() : nullptr;

    QRhiCommandBuffer *cb = rhiCtx->commandBuffer();
    cb->setGraphicsPipeline(ps);
    cb->setShaderResources(srb);

    if (*needsSetViewport) {
        cb->setViewport(rhiCtx->graphicsPipelineState(&inData)->viewport);
        *needsSetViewport = false;
    }

    QRhiCommandBuffer::VertexInput vb(vertexBuffer, 0);
    if (indexBuffer) {
        cb->setVertexInput(0, 1, &vb, indexBuffer, 0, renderable.subset.rhi.ia.indexBuffer->indexFormat());
        cb->drawIndexed(renderable.subset.count, 1, renderable.subset.offset);
    } else {
        cb->setVertexInput(0, 1, &vb);
        cb->draw(renderable.subset.count, 1, renderable.subset.offset);
    }
}

void QSSGMaterialSystem::renderRhiSubset(QSSGRhiContext *rhiCtx,
                                         QSSGCustomMaterialRenderable &renderable,
                                         QSSGLayerRenderData &inData,
                                         bool *needsSetViewport)
{
    const QSSGRenderCustomMaterial &material(renderable.material);
    for (const dynamic::QSSGCommand *command : qAsConst(material.commands)) {
        switch (command->m_type) {
        case dynamic::CommandType::Render:
            recordRhiSubsetDrawCalls(rhiCtx, renderable, inData, needsSetViewport);
            break;

        default:
            break;
        }
    }
}

QT_END_NAMESPACE
