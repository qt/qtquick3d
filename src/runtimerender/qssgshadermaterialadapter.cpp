// Copyright (C) 2008-2012 NVIDIA Corporation.
// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

/* clang-format off */

#include <QtQuick3DRuntimeRender/private/qssgshadermaterialadapter_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercontextcore_p.h>

QT_BEGIN_NAMESPACE

QSSGShaderMaterialAdapter::~QSSGShaderMaterialAdapter() = default;

QSSGShaderMaterialAdapter *QSSGShaderMaterialAdapter::create(const QSSGRenderGraphObject &materialNode)
{
    switch (materialNode.type) {
    case QSSGRenderGraphObject::Type::DefaultMaterial:
    case QSSGRenderGraphObject::Type::PrincipledMaterial:
    case QSSGRenderGraphObject::Type::SpecularGlossyMaterial:
        return new QSSGShaderDefaultMaterialAdapter(static_cast<const QSSGRenderDefaultMaterial &>(materialNode));

    case QSSGRenderGraphObject::Type::CustomMaterial:
        return new QSSGShaderCustomMaterialAdapter(static_cast<const QSSGRenderCustomMaterial &>(materialNode));

    default:
        break;
    }

    return nullptr;
}

bool QSSGShaderMaterialAdapter::isUnshaded()
{
    return false;
}

bool QSSGShaderMaterialAdapter::hasCustomShaderSnippet(QSSGShaderCache::ShaderType)
{
    return false;
}

QByteArray QSSGShaderMaterialAdapter::customShaderSnippet(QSSGShaderCache::ShaderType,
                                                          QSSGShaderLibraryManager &)
{
    return QByteArray();
}

bool QSSGShaderMaterialAdapter::hasCustomShaderFunction(QSSGShaderCache::ShaderType,
                                                        const QByteArray &,
                                                        QSSGShaderLibraryManager &)
{
    return false;
}

void QSSGShaderMaterialAdapter::setCustomPropertyUniforms(char *,
                                                          QSSGRhiShaderPipeline &,
                                                          const QSSGRenderContextInterface &)
{
}

bool QSSGShaderMaterialAdapter::usesSharedVariables()
{
    return false;
}



QSSGShaderDefaultMaterialAdapter::QSSGShaderDefaultMaterialAdapter(const QSSGRenderDefaultMaterial &material)
    : m_material(material)
{
}

bool QSSGShaderDefaultMaterialAdapter::isPrincipled()
{
    return m_material.type == QSSGRenderGraphObject::Type::PrincipledMaterial;
}

bool QSSGShaderDefaultMaterialAdapter::isSpecularGlossy()
{
    return m_material.type == QSSGRenderGraphObject::Type::SpecularGlossyMaterial;
}

bool QSSGShaderDefaultMaterialAdapter::isMetalnessEnabled()
{
    return m_material.isMetalnessEnabled();
}

bool QSSGShaderDefaultMaterialAdapter::isSpecularEnabled()
{
    return m_material.isSpecularEnabled();
}

bool QSSGShaderDefaultMaterialAdapter::isVertexColorsEnabled()
{
    return m_material.isVertexColorsEnabled();
}

bool QSSGShaderDefaultMaterialAdapter::isClearcoatEnabled()
{
    return m_material.isClearcoatEnabled();
}

bool QSSGShaderDefaultMaterialAdapter::isTransmissionEnabled()
{
    return m_material.isTransmissionEnabled();
}

bool QSSGShaderDefaultMaterialAdapter::hasLighting()
{
    return m_material.hasLighting();
}

bool QSSGShaderDefaultMaterialAdapter::usesCustomSkinning()
{
    return false;
}

bool QSSGShaderDefaultMaterialAdapter::usesCustomMorphing()
{
    return false;
}

QSSGRenderDefaultMaterial::MaterialSpecularModel QSSGShaderDefaultMaterialAdapter::specularModel()
{
    return m_material.specularModel;
}

QSSGRenderDefaultMaterial::MaterialAlphaMode QSSGShaderDefaultMaterialAdapter::alphaMode()
{
    return m_material.alphaMode;
}

QSSGRenderImage *QSSGShaderDefaultMaterialAdapter::iblProbe()
{
    return m_material.iblProbe;
}

QVector3D QSSGShaderDefaultMaterialAdapter::emissiveColor()
{
    return m_material.emissiveColor;
}

QVector4D QSSGShaderDefaultMaterialAdapter::color()
{
    return m_material.color;
}

QVector3D QSSGShaderDefaultMaterialAdapter::specularTint()
{
    return m_material.specularTint;
}

float QSSGShaderDefaultMaterialAdapter::ior()
{
    return m_material.ior;
}

float QSSGShaderDefaultMaterialAdapter::fresnelPower()
{
    return m_material.fresnelPower;
}

float QSSGShaderDefaultMaterialAdapter::metalnessAmount()
{
    return m_material.metalnessAmount;
}

float QSSGShaderDefaultMaterialAdapter::specularAmount()
{
    return m_material.specularAmount;
}

float QSSGShaderDefaultMaterialAdapter::specularRoughness()
{
    return m_material.specularRoughness;
}

float QSSGShaderDefaultMaterialAdapter::bumpAmount()
{
    return m_material.bumpAmount;
}

float QSSGShaderDefaultMaterialAdapter::translucentFallOff()
{
    return m_material.translucentFalloff;
}

float QSSGShaderDefaultMaterialAdapter::diffuseLightWrap()
{
    return m_material.diffuseLightWrap;
}

float QSSGShaderDefaultMaterialAdapter::occlusionAmount()
{
    return m_material.occlusionAmount;
}

float QSSGShaderDefaultMaterialAdapter::alphaCutOff()
{
    return m_material.alphaCutoff;
}

float QSSGShaderDefaultMaterialAdapter::pointSize()
{
    return m_material.pointSize;
}

float QSSGShaderDefaultMaterialAdapter::lineWidth()
{
    return m_material.lineWidth;
}

float QSSGShaderDefaultMaterialAdapter::heightAmount()
{
    return m_material.heightAmount;
}

float QSSGShaderDefaultMaterialAdapter::minHeightSamples()
{
    return m_material.minHeightSamples;
}

float QSSGShaderDefaultMaterialAdapter::maxHeightSamples()
{
    return m_material.maxHeightSamples;
}

float QSSGShaderDefaultMaterialAdapter::clearcoatAmount()
{
    return m_material.clearcoatAmount;
}

float QSSGShaderDefaultMaterialAdapter::clearcoatRoughnessAmount()
{
    return m_material.clearcoatRoughnessAmount;
}

float QSSGShaderDefaultMaterialAdapter::transmissionFactor()
{
    return m_material.transmissionFactor;
}

float QSSGShaderDefaultMaterialAdapter::thicknessFactor()
{
    return m_material.thicknessFactor;
}

float QSSGShaderDefaultMaterialAdapter::attenuationDistance()
{
    return m_material.attenuationDistance;
}

QVector3D QSSGShaderDefaultMaterialAdapter::attenuationColor()
{
    return m_material.attenuationColor;
}

QSSGShaderCustomMaterialAdapter::QSSGShaderCustomMaterialAdapter(const QSSGRenderCustomMaterial &material)
    : m_material(material)
{
}

// Act like Principled. Lighting is always on, specular, metalness, etc. support should all be enabled.
// Unlike Principled, the *enabled values do not depend on the metalness or specularAmount values
// (we cannot tell what those are if they are written in the shader).

bool QSSGShaderCustomMaterialAdapter::isPrincipled()
{
    return true;
}

bool QSSGShaderCustomMaterialAdapter::isSpecularGlossy()
{
    return false;
}

bool QSSGShaderCustomMaterialAdapter::isMetalnessEnabled()
{
    return true;
}

bool QSSGShaderCustomMaterialAdapter::isSpecularEnabled()
{
    return true;
}

bool QSSGShaderCustomMaterialAdapter::isVertexColorsEnabled()
{
    return m_material.m_renderFlags.testFlag(QSSGRenderCustomMaterial::RenderFlag::VarColor);
}

bool QSSGShaderCustomMaterialAdapter::isClearcoatEnabled()
{
    // TODO: Expose Clearcoat properties to custom material
    return false;
}

bool QSSGShaderCustomMaterialAdapter::isTransmissionEnabled()
{
    // TODO: Expose Transmission to custom material
    return false;
}

bool QSSGShaderCustomMaterialAdapter::hasLighting()
{
    return true;
}

bool QSSGShaderCustomMaterialAdapter::usesCustomSkinning()
{
    return m_material.m_renderFlags.testFlag(QSSGRenderCustomMaterial::RenderFlag::Skinning);
}

bool QSSGShaderCustomMaterialAdapter::usesCustomMorphing()
{
    return m_material.m_renderFlags.testFlag(QSSGRenderCustomMaterial::RenderFlag::Morphing);
}

QSSGRenderDefaultMaterial::MaterialSpecularModel QSSGShaderCustomMaterialAdapter::specularModel()
{
    return QSSGRenderDefaultMaterial::MaterialSpecularModel::Default;
}

QSSGRenderDefaultMaterial::MaterialAlphaMode QSSGShaderCustomMaterialAdapter::alphaMode()
{
    return QSSGRenderDefaultMaterial::MaterialAlphaMode::Default;
}

QSSGRenderImage *QSSGShaderCustomMaterialAdapter::iblProbe()
{
    return m_material.m_iblProbe;
}

// The following are the values that get set into uniforms such as
// qt_material_properties etc. When a custom shader is present, these values
// are not used at all. However, a CustomMaterial is also valid without a
// vertex/fragment shader, or with no custom shaders at all. Therefore the
// values here must match the defaults of PrincipledMaterial, in order to make
// PrincipledMaterial { } and CustomMaterial { } identical.

QVector3D QSSGShaderCustomMaterialAdapter::emissiveColor()
{
    return QVector3D(0, 0, 0);
}

QVector4D QSSGShaderCustomMaterialAdapter::color()
{
    return QVector4D(1, 1, 1, 1);
}

QVector3D QSSGShaderCustomMaterialAdapter::specularTint()
{
    return QVector3D(1, 1, 1);
}

float QSSGShaderCustomMaterialAdapter::ior()
{
    return 1.45f;
}

float QSSGShaderCustomMaterialAdapter::fresnelPower()
{
    return 0.0f;
}

float QSSGShaderCustomMaterialAdapter::metalnessAmount()
{
    return 0.0f;
}

float QSSGShaderCustomMaterialAdapter::specularAmount()
{
    return 0.5f;
}

float QSSGShaderCustomMaterialAdapter::specularRoughness()
{
    return 0.0f;
}

float QSSGShaderCustomMaterialAdapter::bumpAmount()
{
    return 0.0f;
}

float QSSGShaderCustomMaterialAdapter::translucentFallOff()
{
    return 0.0f;
}

float QSSGShaderCustomMaterialAdapter::diffuseLightWrap()
{
    return 0.0f;
}

float QSSGShaderCustomMaterialAdapter::occlusionAmount()
{
    return 1.0f;
}

float QSSGShaderCustomMaterialAdapter::alphaCutOff()
{
    return 0.5f;
}

float QSSGShaderCustomMaterialAdapter::pointSize()
{
    return 1.0f;
}

float QSSGShaderCustomMaterialAdapter::lineWidth()
{
    return m_material.m_lineWidth;
}

float QSSGShaderCustomMaterialAdapter::heightAmount()
{
    return 0.0f;
}

float QSSGShaderCustomMaterialAdapter::minHeightSamples()
{
    return 0.0f;
}

float QSSGShaderCustomMaterialAdapter::maxHeightSamples()
{
    return 0.0f;
}

float QSSGShaderCustomMaterialAdapter::clearcoatAmount()
{
    return 0.0f;
}

float QSSGShaderCustomMaterialAdapter::clearcoatRoughnessAmount()
{
    return 0.0f;
}

float QSSGShaderCustomMaterialAdapter::transmissionFactor()
{
    return 0.0f;
}

float QSSGShaderCustomMaterialAdapter::thicknessFactor()
{
    return 0.0f;
}

float QSSGShaderCustomMaterialAdapter::attenuationDistance()
{
    return std::numeric_limits<float>::infinity();
}

QVector3D QSSGShaderCustomMaterialAdapter::attenuationColor()
{
    return { 1.0f, 1.0f, 1.0f };
}

bool QSSGShaderCustomMaterialAdapter::isUnshaded()
{
    return m_material.m_shadingMode == QSSGRenderCustomMaterial::ShadingMode::Unshaded;
}

bool QSSGShaderCustomMaterialAdapter::hasCustomShaderSnippet(QSSGShaderCache::ShaderType type)
{
    if (type == QSSGShaderCache::ShaderType::Vertex)
        return m_material.m_customShaderPresence.testFlag(QSSGRenderCustomMaterial::CustomShaderPresenceFlag::Vertex);

    return m_material.m_customShaderPresence.testFlag(QSSGRenderCustomMaterial::CustomShaderPresenceFlag::Fragment);
}

QByteArray QSSGShaderCustomMaterialAdapter::customShaderSnippet(QSSGShaderCache::ShaderType type,
                                                                QSSGShaderLibraryManager &shaderLibraryManager)
{
    if (hasCustomShaderSnippet(type))
        return shaderLibraryManager.getShaderSource(m_material.m_shaderPathKey, type);

    return QByteArray();
}

bool QSSGShaderCustomMaterialAdapter::hasCustomShaderFunction(QSSGShaderCache::ShaderType shaderType,
                                                              const QByteArray &funcName,
                                                              QSSGShaderLibraryManager &shaderLibraryManager)
{
    if (hasCustomShaderSnippet(shaderType))
        return shaderLibraryManager.getShaderMetaData(m_material.m_shaderPathKey, shaderType).customFunctions.contains(funcName);

    return false;
}

void QSSGShaderCustomMaterialAdapter::setCustomPropertyUniforms(char *ubufData,
                                                                QSSGRhiShaderPipeline &shaderPipeline,
                                                                const QSSGRenderContextInterface &context)
{
    context.customMaterialSystem()->applyRhiShaderPropertyValues(ubufData, m_material, shaderPipeline);
}

bool QSSGShaderCustomMaterialAdapter::usesSharedVariables()
{
    return m_material.m_usesSharedVariables;
}

namespace {

// Custom material shader substitution table.
// Must be in sync with the shader generator.
static const QSSGCustomMaterialVariableSubstitution qssg_var_subst_tab[] = {
    // uniform (block members)
    { "MODELVIEWPROJECTION_MATRIX", "qt_modelViewProjection" },
    { "VIEWPROJECTION_MATRIX", "qt_viewProjectionMatrix" },
    { "MODEL_MATRIX", "qt_modelMatrix" },
    { "VIEW_MATRIX", "qt_viewMatrix" },
    { "NORMAL_MATRIX", "qt_normalMatrix"},
    { "BONE_TRANSFORMS", "qt_getTexMatrix" },
    { "BONE_NORMAL_TRANSFORMS", "qt_getTexMatrix" },
    { "PROJECTION_MATRIX", "qt_projectionMatrix" },
    { "INVERSE_PROJECTION_MATRIX", "qt_inverseProjectionMatrix" },
    { "CAMERA_POSITION", "qt_cameraPosition" },
    { "CAMERA_DIRECTION", "qt_cameraDirection" },
    { "CAMERA_PROPERTIES", "qt_cameraProperties" },
    { "FRAMEBUFFER_Y_UP", "qt_rhi_properties.x" },
    { "NDC_Y_UP", "qt_rhi_properties.y" },
    { "NEAR_CLIP_VALUE", "qt_rhi_properties.z" },
    { "IBL_MAXMIPMAP", "qt_lightProbeProperties.y" },
    { "IBL_HORIZON", "qt_lightProbeProperties.z" },
    { "IBL_EXPOSE", "qt_lightProbeProperties.w" },

    // outputs
    { "POSITION", "gl_Position" },
    { "FRAGCOLOR", "fragOutput" },
    { "POINT_SIZE", "gl_PointSize" },

    // fragment inputs
    { "FRAGCOORD", "gl_FragCoord"},

    // functions
    { "DIRECTIONAL_LIGHT", "qt_directionalLightProcessor" },
    { "POINT_LIGHT", "qt_pointLightProcessor" },
    { "SPOT_LIGHT", "qt_spotLightProcessor" },
    { "AMBIENT_LIGHT", "qt_ambientLightProcessor" },
    { "SPECULAR_LIGHT", "qt_specularLightProcessor" },
    { "MAIN", "qt_customMain" },
    { "POST_PROCESS", "qt_customPostProcessor" },
    { "IBL_PROBE", "qt_iblProbeProcessor" },

    // textures
    { "SCREEN_TEXTURE", "qt_screenTexture" },
    { "SCREEN_MIP_TEXTURE", "qt_screenTexture" }, // same resource as SCREEN_TEXTURE under the hood
    { "DEPTH_TEXTURE", "qt_depthTexture" },
    { "AO_TEXTURE", "qt_aoTexture" },
    { "IBL_TEXTURE", "qt_lightProbe" },
    { "LIGHTMAP", "qt_lightmap" },

    // For shaded only: vertex outputs, for convenience and perf. (only those
    // that are always present when lighting is enabled) The custom vertex main
    // can also calculate on its own and pass them on with VARYING but that's a
    // bit wasteful since we calculate these anyways.
    { "VAR_WORLD_NORMAL", "qt_varNormal" },
    { "VAR_WORLD_TANGENT", "qt_varTangent" },
    { "VAR_WORLD_BINORMAL", "qt_varBinormal" },
    { "VAR_WORLD_POSITION", "qt_varWorldPos" },
    // vertex color is always enabled for custom materials (shaded)
    { "VAR_COLOR", "qt_varColor" },

    // effects
    { "INPUT", "qt_inputTexture" },
    { "INPUT_UV", "qt_inputUV" },
    { "TEXTURE_UV", "qt_textureUV" },
    { "INPUT_SIZE", "qt_inputSize" },
    { "OUTPUT_SIZE", "qt_outputSize" },
    { "FRAME", "qt_frame_num" },

    // instancing
    { "INSTANCE_COLOR", "qt_instanceColor"},
    { "INSTANCE_DATA", "qt_instanceData"},
    { "INSTANCE_INDEX", "gl_InstanceIndex"},

    // morphing
    { "MORPH_POSITION", "qt_getTargetPositionFromTargetId"},
    { "MORPH_NORMAL", "qt_getTargetNormalFromTargetId"},
    { "MORPH_TANGENT", "qt_getTargetTangentFromTargetId"},
    { "MORPH_BINORMAL", "qt_getTargetBinormalFromTargetId"},
    { "MORPH_WEIGHTS", "qt_morphWeights"},

    // custom variables
    { "SHARED_VARS", "struct QT_SHARED_VARS"}
};

// Functions that, if present, get an argument list injected.
static const QByteArrayView qssg_func_injectarg_tab[] = {
    "DIRECTIONAL_LIGHT",
    "POINT_LIGHT",
    "SPOT_LIGHT",
    "AMBIENT_LIGHT",
    "SPECULAR_LIGHT",
    "MAIN",
    "POST_PROCESS",
    "IBL_PROBE"
};

// This is based on the Qt Quick shader rewriter (with fixes)
struct Tokenizer {
    enum Token {
        Token_Comment,
        Token_OpenBrace,
        Token_CloseBrace,
        Token_OpenParen,
        Token_CloseParen,
        Token_SemiColon,
        Token_Identifier,
        Token_Macro,
        Token_OpenBraket,
        Token_CloseBraket,
        Token_Unspecified,

        Token_EOF
    };

    void initialize(const QByteArray &input);
    Token next();

    const char *stream;
    const char *pos;
    const char *identifier;
};

void Tokenizer::initialize(const QByteArray &input)
{
    stream = input.constData();
    pos = input;
    identifier = input;
}

Tokenizer::Token Tokenizer::next()
{
    while (*pos) {
        char c = *pos++;
        switch (c) {
        case '/':
            if (*pos == '/') {
                // '//' comment
                ++pos;
                while (*pos && *pos != '\n') ++pos;
                return Token_Comment;
            } else if (*pos == '*') {
                // /* */ comment
                ++pos;
                while (*pos && (*pos != '*' || pos[1] != '/')) ++pos;
                if (*pos) pos += 2;
                return Token_Comment;
            }
            return Token_Unspecified;

        case '#': {
            while (*pos) {
                if (*pos == '\n') {
                    ++pos;
                    break;
                } else if (*pos == '\\') {
                    ++pos;
                    while (*pos && (*pos == ' ' || *pos == '\t'))
                        ++pos;
                    if (*pos && (*pos == '\n' || (*pos == '\r' && pos[1] == '\n')))
                        pos += 2;
                } else {
                    ++pos;
                }
            }
            return Token_Unspecified;
        }

        case ';': return Token_SemiColon;
        case '\0': return Token_EOF;
        case '{': return Token_OpenBrace;
        case '}': return Token_CloseBrace;
        case '(': return Token_OpenParen;
        case ')': return Token_CloseParen;
        case '[': return Token_OpenBraket;
        case ']': return Token_CloseBraket;

        case ' ':
        case '\n':
        case '\r': break;
        default:
            // Identifier...
            if ((c >= 'a' && c <= 'z' ) || (c >= 'A' && c <= 'Z' ) || c == '_') {
                identifier = pos - 1;
                while (*pos && ((*pos >= 'a' && *pos <= 'z')
                                     || (*pos >= 'A' && *pos <= 'Z')
                                     || *pos == '_'
                                     || (*pos >= '0' && *pos <= '9'))) {
                    ++pos;
                }
                return Token_Identifier;
            } else {
                return Token_Unspecified;
            }
        }
    }

    return Token_EOF;
}
} // namespace

QSSGShaderCustomMaterialAdapter::ShaderCodeAndMetaData
QSSGShaderCustomMaterialAdapter::prepareCustomShader(QByteArray &dst,
                                                     const QByteArray &shaderCode,
                                                     QSSGShaderCache::ShaderType type,
                                                     const StringPairList &baseUniforms,
                                                     const StringPairList &baseInputs,
                                                     const StringPairList &baseOutputs)
{
    QByteArrayList inputs;
    QByteArrayList outputs;

    Tokenizer tok;
    tok.initialize(shaderCode);

    QSSGCustomShaderMetaData md = {};
    QByteArray result;
    result.reserve(1024);
    // If shader debugging is not enabled we reset the line count to make error message
    // when a shader fails more useful. When shader debugging is enabled the whole shader
    // will be printed and not just the user written part, so in that case we do not want
    // to adjust the line numbers.
    //
    // NOTE: This is not perfect, we do expend the custom material and effect shaders, so
    // there cane still be cases where the reported line numbers are slightly off.
    if (!QSSGRhiContext::shaderDebuggingEnabled())
        result.prepend("#line 1\n");
    const char *lastPos = shaderCode.constData();

    int funcFinderState = 0;
    int useJointTexState = -1;
    int useJointNormalTexState = -1;
    QByteArray currentShadedFunc;
    Tokenizer::Token t = tok.next();
    while (t != Tokenizer::Token_EOF) {
        switch (t) {
        case Tokenizer::Token_Comment:
            break;
        case Tokenizer::Token_Identifier:
        {
            QByteArray id = QByteArray::fromRawData(lastPos, tok.pos - lastPos);
            if (id.trimmed() == QByteArrayLiteral("VARYING")) {
                QByteArray vtype;
                QByteArray vname;
                lastPos = tok.pos;
                t = tok.next();
                while (t != Tokenizer::Token_EOF) {
                    QByteArray data = QByteArray::fromRawData(lastPos, tok.pos - lastPos);
                    if (t == Tokenizer::Token_Identifier) {
                        if (vtype.isEmpty())
                            vtype = data.trimmed();
                        else if (vname.isEmpty())
                            vname = data.trimmed();
                    }
                    if (t == Tokenizer::Token_SemiColon)
                        break;
                    lastPos = tok.pos;
                    t = tok.next();
                }
                if (type == QSSGShaderCache::ShaderType::Vertex)
                    outputs.append(vtype + " " + vname);
                else
                    inputs.append(vtype + " " + vname);
            } else {
                const QByteArray trimmedId = id.trimmed();
                if (funcFinderState == 0 && trimmedId == QByteArrayLiteral("void")) {
                    funcFinderState += 1;
                } else if (funcFinderState == 1) {
                    auto begin = qssg_func_injectarg_tab;
                    const auto end = qssg_func_injectarg_tab + (sizeof(qssg_func_injectarg_tab) / sizeof(qssg_func_injectarg_tab[0]));
                    const auto foundIt = std::find_if(begin, end, [trimmedId](const QByteArrayView &entry) { return entry == trimmedId; });
                    if (foundIt != end) {
                        currentShadedFunc = trimmedId;
                        funcFinderState += 1;
                    }
                } else {
                    funcFinderState = 0;
                }

                if (trimmedId == QByteArrayLiteral("SCREEN_TEXTURE"))
                    md.flags |= QSSGCustomShaderMetaData::UsesScreenTexture;
                else if (trimmedId == QByteArrayLiteral("SCREEN_MIP_TEXTURE"))
                    md.flags |= QSSGCustomShaderMetaData::UsesScreenMipTexture;
                else if (trimmedId == QByteArrayLiteral("DEPTH_TEXTURE"))
                    md.flags |= QSSGCustomShaderMetaData::UsesDepthTexture;
                else if (trimmedId == QByteArrayLiteral("AO_TEXTURE"))
                    md.flags |= QSSGCustomShaderMetaData::UsesAoTexture;
                else if (trimmedId == QByteArrayLiteral("POSITION"))
                    md.flags |= QSSGCustomShaderMetaData::OverridesPosition;
                else if (trimmedId == QByteArrayLiteral("PROJECTION_MATRIX"))
                    md.flags |= QSSGCustomShaderMetaData::UsesProjectionMatrix;
                else if (trimmedId == QByteArrayLiteral("INVERSE_PROJECTION_MATRIX"))
                    md.flags |= QSSGCustomShaderMetaData::UsesInverseProjectionMatrix;
                else if (trimmedId == QByteArrayLiteral("VAR_COLOR"))
                    md.flags |= QSSGCustomShaderMetaData::UsesVarColor;
                else if (trimmedId == QByteArrayLiteral("SHARED_VARS"))
                    md.flags |= QSSGCustomShaderMetaData::UsesSharedVars;
                else if (trimmedId == QByteArrayLiteral("IBL_ORIENTATION"))
                    md.flags |= QSSGCustomShaderMetaData::UsesIblOrientation;
                else if (trimmedId == QByteArrayLiteral("LIGHTMAP"))
                    md.flags |= QSSGCustomShaderMetaData::UsesLightmap;

                for (const QSSGCustomMaterialVariableSubstitution &subst : qssg_var_subst_tab) {
                    if (trimmedId == subst.builtin) {
                        id.replace(subst.builtin, subst.actualName); // replace, not assignment, to keep whitespace etc.
                        if (trimmedId == QByteArrayLiteral("BONE_TRANSFORMS")) {
                            useJointTexState = 0;
                            md.flags |= QSSGCustomShaderMetaData::UsesSkinning;
                        } else if (trimmedId == QByteArrayLiteral("BONE_NORMAL_TRANSFORMS")) {
                            useJointNormalTexState = 0;
                            md.flags |= QSSGCustomShaderMetaData::UsesSkinning;
                        }
                        if (trimmedId == QByteArrayLiteral("MORPH_POSITION") ||
                                trimmedId == QByteArrayLiteral("MORPH_NORMAL") ||
                                trimmedId == QByteArrayLiteral("MORPH_TANGENT") ||
                                trimmedId == QByteArrayLiteral("MORPH_BINORMAL"))
                            md.flags |= QSSGCustomShaderMetaData::UsesMorphing;
                        break;
                    }
                }
                result += id;
            }
        }
            break;
        case Tokenizer::Token_OpenParen:
            result += QByteArray::fromRawData(lastPos, tok.pos - lastPos);
            if (funcFinderState == 2) {
                result += QByteArrayLiteral("/*%QT_ARGS_");
                result += currentShadedFunc;
                result += QByteArrayLiteral("%*/");
                for (const QSSGCustomMaterialVariableSubstitution &subst : qssg_var_subst_tab) {
                    if (currentShadedFunc == subst.builtin) {
                        currentShadedFunc = subst.actualName.toByteArray();
                        break;
                    }
                }
                md.customFunctions.insert(currentShadedFunc);
                currentShadedFunc.clear();
            }
            funcFinderState = 0;
            break;
        case Tokenizer::Token_OpenBraket:
            if (useJointTexState == 0) {
                result += QByteArrayLiteral("(2 * (");
                ++useJointTexState;
                break;
            } else if (useJointNormalTexState == 0) {
                result += QByteArrayLiteral("(1 + 2 * (");
                ++useJointNormalTexState;
                break;
            }

            if (useJointTexState >= 0)
                ++useJointTexState;
            else if (useJointNormalTexState >= 0)
                ++useJointNormalTexState;
            result += QByteArrayLiteral("[");
            break;
        case Tokenizer::Token_CloseBraket:
            // This implementation will not allow mixed usages of BONE_TRANSFORMS and
            // BONE_NORMAL_TRANSFORMS.
            // For example, BONE_TRANSFORM[int(BONE_NORMAL_TRANFORMS[i][0].x)]
            // cannot be compiled successfully.
            if (useJointTexState <= 0 && useJointNormalTexState <= 0) {
                result += QByteArrayLiteral("]");
                break;
            }
            if (useJointTexState > 1) {
                result += QByteArrayLiteral("]");
                --useJointTexState;
                break;
            } else if (useJointNormalTexState > 1) {
                result += QByteArrayLiteral("]");
                --useJointNormalTexState;
                break;
            }
            result += QByteArrayLiteral("))");
            useJointTexState = -1;
            useJointNormalTexState = -1;
            break;
        default:
            result += QByteArray::fromRawData(lastPos, tok.pos - lastPos);
            break;
        }
        lastPos = tok.pos;
        t = tok.next();
    }

    result += '\n';

    StringPairList allUniforms = baseUniforms;
    if (md.flags.testFlag(QSSGCustomShaderMetaData::UsesScreenTexture) || md.flags.testFlag(QSSGCustomShaderMetaData::UsesScreenMipTexture))
        allUniforms.append({ "sampler2D", "qt_screenTexture" });
    if (md.flags.testFlag(QSSGCustomShaderMetaData::UsesDepthTexture))
        allUniforms.append({ "sampler2D", "qt_depthTexture" });
    if (md.flags.testFlag(QSSGCustomShaderMetaData::UsesAoTexture))
        allUniforms.append({ "sampler2D", "qt_aoTexture" });
    if (md.flags.testFlag(QSSGCustomShaderMetaData::UsesLightmap))
        allUniforms.append({ "sampler2D", "qt_lightmap" });

    static const char *metaStart = "#ifdef QQ3D_SHADER_META\n/*{\n  \"uniforms\": [\n";
    static const char *metaEnd = "  ]\n}*/\n#endif\n";
    dst.append(metaStart);
    for (int i = 0, count = allUniforms.size(); i < count; ++i) {
        const auto &typeAndName(allUniforms[i]);
        dst.append("    { \"type\": \"" + typeAndName.first + "\", \"name\": \"" + typeAndName.second + "\" }");
        if (i < count - 1)
            dst.append(",");
        dst.append("\n");
    }
    dst.append(metaEnd);

    const char *stageStr = type == QSSGShaderCache::ShaderType::Vertex ? "vertex" : "fragment";
    StringPairList allInputs = baseInputs;
    for (const QByteArray &inputTypeAndName : inputs) {
        const QByteArrayList typeAndName = inputTypeAndName.split(' ');
        if (typeAndName.size() == 2)
            allInputs.append({ typeAndName[0].trimmed(), typeAndName[1].trimmed() });
    }
    if (!allInputs.isEmpty()) {
        static const char *metaStart = "#ifdef QQ3D_SHADER_META\n/*{\n  \"inputs\": [\n";
        static const char *metaEnd = "  ]\n}*/\n#endif\n";
        dst.append(metaStart);
        for (int i = 0, count = allInputs.size(); i < count; ++i) {
            dst.append("    { \"type\": \"" + allInputs[i].first
                    + "\", \"name\": \"" + allInputs[i].second
                    + "\", \"stage\": \"" + stageStr + "\" }");
            if (i < count - 1)
                dst.append(",");
            dst.append("\n");
        }
        dst.append(metaEnd);
    }

    StringPairList allOutputs = baseOutputs;
    for (const QByteArray &outputTypeAndName : outputs) {
        const QByteArrayList typeAndName = outputTypeAndName.split(' ');
        if (typeAndName.size() == 2)
            allOutputs.append({ typeAndName[0].trimmed(), typeAndName[1].trimmed() });
    }
    if (!allOutputs.isEmpty()) {
        static const char *metaStart = "#ifdef QQ3D_SHADER_META\n/*{\n  \"outputs\": [\n";
        static const char *metaEnd = "  ]\n}*/\n#endif\n";
        dst.append(metaStart);
        for (int i = 0, count = allOutputs.size(); i < count; ++i) {
            dst.append("    { \"type\": \"" + allOutputs[i].first
                    + "\", \"name\": \"" + allOutputs[i].second
                    + "\", \"stage\": \"" + stageStr + "\" }");
            if (i < count - 1)
                dst.append(",");
            dst.append("\n");
        }
        dst.append(metaEnd);
    }

    return { result, md };
}

QList<QByteArrayView> QtQuick3DEditorHelpers::CustomMaterial::preprocessorVars()
{
    QList<QByteArrayView> k;
    k.reserve(std::size(qssg_var_subst_tab));
    for (const auto &v : qssg_var_subst_tab)
        k.push_back(v.builtin);
    return k;
}

QT_END_NAMESPACE

