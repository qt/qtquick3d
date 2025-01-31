// Copyright (C) 2008-2012 NVIDIA Corporation.
// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

/* clang-format off */

#include <QtQuick3DUtils/private/qssgutils_p.h>
#include <QtQuick3DUtils/private/qssgassert_p.h>

#include <QtQuick3DRuntimeRender/private/qssgrenderdefaultmaterialshadergenerator_p.h>
#include "qssgrendercontextcore.h"
#include <QtQuick3DRuntimeRender/private/qssgrendershadercodegenerator_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderimage_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderlight_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercamera_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadowmap_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercustommaterial_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershaderlibrarymanager_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershaderkeys_p.h>
#include <QtQuick3DRuntimeRender/private/qssgshadermaterialadapter_p.h>
#include <QtQuick3DRuntimeRender/private/qssgvertexpipelineimpl_p.h>
#include <QtQuick3DRuntimeRender/private/qssglayerrenderdata_p.h>

#include <QtCore/QByteArray>

#include <cstdio>
#include <limits>

QT_BEGIN_NAMESPACE

namespace  {
using Type = QSSGRenderableImage::Type;
template<Type> struct ImageStrings {};
#define DefineImageStrings(V) template<> struct ImageStrings<Type::V> \
{\
    static constexpr const char* sampler() { return "qt_"#V"Map_sampler"; }\
    static constexpr const char* offsets() { return "qt_"#V"Map_offsets"; }\
    static constexpr const char* rotations() { return "qt_"#V"Map_rotations"; }\
    static constexpr const char* fragCoords1() { return "qt_"#V"Map_uv_coords1"; }\
    static constexpr const char* fragCoords2() { return "qt_"#V"Map_uv_coords2"; }\
    static constexpr const char* samplerSize() { return "qt_"#V"Map_size"; }\
}

DefineImageStrings(Unknown);
DefineImageStrings(Diffuse);
DefineImageStrings(Opacity);
DefineImageStrings(Specular);
DefineImageStrings(Emissive);
DefineImageStrings(Bump);
DefineImageStrings(SpecularAmountMap);
DefineImageStrings(Normal);
DefineImageStrings(Translucency);
DefineImageStrings(Roughness);
DefineImageStrings(BaseColor);
DefineImageStrings(Metalness);
DefineImageStrings(Occlusion);
DefineImageStrings(Height);
DefineImageStrings(Clearcoat);
DefineImageStrings(ClearcoatRoughness);
DefineImageStrings(ClearcoatNormal);
DefineImageStrings(Transmission);
DefineImageStrings(Thickness);

struct ImageStringSet
{
    const char *imageSampler;
    const char *imageFragCoords;
    const char *imageFragCoordsTemp;
    const char *imageOffsets;
    const char *imageRotations;
    const char *imageSamplerSize;
};

#define DefineImageStringTableEntry(V) \
    { ImageStrings<Type::V>::sampler(), ImageStrings<Type::V>::fragCoords1(), ImageStrings<Type::V>::fragCoords2(), \
      ImageStrings<Type::V>::offsets(), ImageStrings<Type::V>::rotations(), ImageStrings<Type::V>::samplerSize() }

constexpr ImageStringSet imageStringTable[] {
    DefineImageStringTableEntry(Unknown),
    DefineImageStringTableEntry(Diffuse),
    DefineImageStringTableEntry(Opacity),
    DefineImageStringTableEntry(Specular),
    DefineImageStringTableEntry(Emissive),
    DefineImageStringTableEntry(Bump),
    DefineImageStringTableEntry(SpecularAmountMap),
    DefineImageStringTableEntry(Normal),
    DefineImageStringTableEntry(Translucency),
    DefineImageStringTableEntry(Roughness),
    DefineImageStringTableEntry(BaseColor),
    DefineImageStringTableEntry(Metalness),
    DefineImageStringTableEntry(Occlusion),
    DefineImageStringTableEntry(Height),
    DefineImageStringTableEntry(Clearcoat),
    DefineImageStringTableEntry(ClearcoatRoughness),
    DefineImageStringTableEntry(ClearcoatNormal),
    DefineImageStringTableEntry(Transmission),
    DefineImageStringTableEntry(Thickness)
};

const int TEXCOORD_VAR_LEN = 16;

void textureCoordVaryingName(char (&outString)[TEXCOORD_VAR_LEN], quint8 uvSet)
{
    // For now, uvSet will be less than 2.
    // But this value will be verified in the setProperty function.
    Q_ASSERT(uvSet < 9);
    qstrncpy(outString, "qt_varTexCoordX", TEXCOORD_VAR_LEN);
    outString[14] = '0' + uvSet;
}

void textureCoordVariableName(char (&outString)[TEXCOORD_VAR_LEN], quint8 uvSet)
{
    // For now, uvSet will be less than 2.
    // But this value will be verified in the setProperty function.
    Q_ASSERT(uvSet < 9);
    qstrncpy(outString, "qt_texCoordX", TEXCOORD_VAR_LEN);
    outString[11] = '0' + uvSet;
}

}

const char *QSSGMaterialShaderGenerator::getSamplerName(QSSGRenderableImage::Type type)
{
    return imageStringTable[int(type)].imageSampler;
}

static void addLocalVariable(QSSGStageGeneratorBase &inGenerator, const QByteArray &inName, const QByteArray &inType)
{
    inGenerator << "    " << inType << " " << inName << ";\n";
}

static QByteArray uvTransform(const QByteArray& imageRotations, const QByteArray& imageOffsets)
{
    QByteArray transform;
    transform = "    qt_uTransform = vec3(" + imageRotations + ".x, " + imageRotations + ".y, " + imageOffsets + ".x);\n";
    transform += "    qt_vTransform = vec3(" + imageRotations + ".z, " + imageRotations + ".w, " + imageOffsets + ".y);\n";
    return transform;
}

static void sanityCheckImageForSampler(const QSSGRenderableImage &image, const char *samplerName)
{
    if (image.m_imageNode.type == QSSGRenderGraphObject::Type::ImageCube) {
        qWarning("Sampler %s expects a 2D texture but the associated texture is a cube map. "
                 "This will lead to problems.",
                 samplerName);
    }
}

static void generateImageUVCoordinates(QSSGMaterialVertexPipeline &vertexShader,
                                       QSSGStageGeneratorBase &fragmentShader,
                                       const QSSGShaderDefaultMaterialKey &key,
                                       QSSGRenderableImage &image,
                                       bool forceFragmentShader = false,
                                       quint32 uvSet = 0,
                                       bool reuseImageCoords = false)
{
    const auto &names = imageStringTable[int(image.m_mapType)];
    char textureCoordName[TEXCOORD_VAR_LEN];
    sanityCheckImageForSampler(image, names.imageSampler);
    fragmentShader.addUniform(names.imageSampler, "sampler2D");
    if (!forceFragmentShader) {
        vertexShader.addUniform(names.imageOffsets, "vec3");
        vertexShader.addUniform(names.imageRotations, "vec4");
    } else {
        fragmentShader.addUniform(names.imageOffsets, "vec3");
        fragmentShader.addUniform(names.imageRotations, "vec4");
    }
    QByteArray uvTrans = uvTransform(names.imageRotations, names.imageOffsets);
    if (image.m_imageNode.m_mappingMode == QSSGRenderImage::MappingModes::Normal) {
        if (!forceFragmentShader) {
            vertexShader << uvTrans;
            vertexShader.addOutgoing(names.imageFragCoords, "vec2");
            vertexShader.addFunction("getTransformedUVCoords");
        } else {
            fragmentShader << uvTrans;
            fragmentShader.addFunction("getTransformedUVCoords");
        }
        vertexShader.generateUVCoords(uvSet, key);
        if (!forceFragmentShader) {
            textureCoordVaryingName(textureCoordName, uvSet);
            vertexShader << "    vec2 " << names.imageFragCoordsTemp << " = qt_getTransformedUVCoords(vec3(" << textureCoordName << ", 1.0), qt_uTransform, qt_vTransform);\n";
            vertexShader.assignOutput(names.imageFragCoords, names.imageFragCoordsTemp);
        } else {
            textureCoordVariableName(textureCoordName, uvSet);
            if (reuseImageCoords)
                fragmentShader << "    ";
            else
                fragmentShader << "    vec2 ";
            fragmentShader << names.imageFragCoords << " = qt_getTransformedUVCoords(vec3(" << textureCoordName << ", 1.0), qt_uTransform, qt_vTransform);\n";
        }
    } else {
        fragmentShader.addUniform(names.imageOffsets, "vec3");
        fragmentShader.addUniform(names.imageRotations, "vec4");
        fragmentShader << uvTrans;
        vertexShader.generateEnvMapReflection(key);
        fragmentShader.addFunction("getTransformedUVCoords");
        if (reuseImageCoords)
            fragmentShader << "    ";
        else
            fragmentShader << "    vec2 ";
        fragmentShader << names.imageFragCoords << " = qt_getTransformedUVCoords(environment_map_reflection, qt_uTransform, qt_vTransform);\n";
    }
}

static void generateImageUVSampler(QSSGMaterialVertexPipeline &vertexGenerator,
                                   QSSGStageGeneratorBase &fragmentShader,
                                   const QSSGShaderDefaultMaterialKey &key,
                                   const QSSGRenderableImage &image,
                                   char (&outString)[TEXCOORD_VAR_LEN],
                                   quint8 uvSet = 0)
{
    const auto &names = imageStringTable[int(image.m_mapType)];
    sanityCheckImageForSampler(image, names.imageSampler);
    fragmentShader.addUniform(names.imageSampler, "sampler2D");
    // NOTE: Actually update the uniform name here
    textureCoordVariableName(outString, uvSet);
    vertexGenerator.generateUVCoords(uvSet, key);
}

static void outputSpecularEquation(QSSGRenderDefaultMaterial::MaterialSpecularModel inSpecularModel,
                                   QSSGStageGeneratorBase &fragmentShader,
                                   const QByteArray &inLightDir,
                                   const QByteArray &inLightSpecColor)
{
    if (inSpecularModel == QSSGRenderDefaultMaterial::MaterialSpecularModel::KGGX) {
        fragmentShader.addInclude("physGlossyBSDF.glsllib");
        fragmentShader << "    global_specular_light += qt_lightAttenuation * qt_shadow_map_occl * qt_specularAmount"
                          " * qt_kggxGlossyDefaultMtl(qt_world_normal, qt_tangent, -" << inLightDir << ".xyz, qt_view_vector, " << inLightSpecColor << ".rgb, qt_specularTint, qt_roughnessAmount).rgb;\n";
    } else {
        fragmentShader.addFunction("specularBSDF");
        fragmentShader << "    global_specular_light += qt_lightAttenuation * qt_shadow_map_occl * qt_specularAmount"
                          " * qt_specularBSDF(qt_world_normal, -" << inLightDir << ".xyz, qt_view_vector, " << inLightSpecColor << ".rgb, 2.56 / (qt_roughnessAmount + 0.01)).rgb;\n";
    }
}

static void addTranslucencyIrradiance(QSSGStageGeneratorBase &infragmentShader,
                                      QSSGRenderableImage *image,
                                      const QSSGMaterialShaderGenerator::LightVariableNames &lightVarNames)
{
    if (image == nullptr)
        return;

    infragmentShader.addFunction("diffuseReflectionWrapBSDF");
    infragmentShader << "    tmp_light_color = " << lightVarNames.lightColor << ".rgb * (1.0 - qt_metalnessAmount);\n";
    infragmentShader << "    global_diffuse_light.rgb += qt_lightAttenuation * qt_shadow_map_occl * qt_translucent_thickness_exp * qt_diffuseReflectionWrapBSDF(-qt_world_normal, -"
                     << lightVarNames.normalizedDirection << ", tmp_light_color, qt_material_properties2.w).rgb;\n";
}

using ShadowNameMap = QHash<QPair<qsizetype, quint32>, QSSGMaterialShaderGenerator::ShadowVariableNames>;
Q_GLOBAL_STATIC(ShadowNameMap, q3ds_shadowMapVariableNames);

static const QSSGMaterialShaderGenerator::ShadowVariableNames& setupShadowMapVariableNames(qsizetype shadowMapIdx, quint32 shadowMapRes, bool is32bit)
{
    QSSGMaterialShaderGenerator::ShadowVariableNames &names = (*q3ds_shadowMapVariableNames)[{shadowMapIdx, shadowMapRes}];
    if (names.shadowMapTexture.isEmpty()) {
        names.shadowCube = QByteArrayLiteral("qt_shadowcube");
        char buf[std::numeric_limits<qsizetype>::digits10 + 1 + 2 + 1]; // digits10 is one short; 2 for [] and 1 for NUL
        std::snprintf(buf, sizeof buf, "%lld", qlonglong(shadowMapIdx));
        names.shadowCube.append(buf);
        names.shadowData = QByteArrayLiteral("ubShadows.shadowData");
        std::snprintf(buf, sizeof buf, "[%lld]", qlonglong(shadowMapIdx));
        names.shadowData.append(buf);
        names.shadowMapTexture = is32bit ? QByteArrayLiteral("qt_shadowmap_texture_32_") : QByteArrayLiteral("qt_shadowmap_texture_16_");
        std::snprintf(buf, sizeof buf, "%d", shadowMapRes);
        names.shadowMapTexture.append(buf);
    }

    return names;
}

// this is for DefaultMaterial only
static void maybeAddMaterialFresnel(QSSGStageGeneratorBase &fragmentShader,
                                    const QSSGShaderDefaultMaterialKeyProperties &keyProps,
                                    QSSGDataView<quint32> inKey,
                                    bool hasMetalness)
{
    if (keyProps.m_fresnelEnabled.getValue(inKey)) {
        fragmentShader.addInclude("defaultMaterialFresnel.glsllib");
        fragmentShader << "    // Add fresnel ratio\n";
        if (hasMetalness) { // this won't be hit in practice since DefaultMaterial does not offer metalness as a property
            fragmentShader << "    qt_specularAmount *= qt_defaultMaterialSimpleFresnel(qt_specularBase, qt_metalnessAmount, qt_world_normal, qt_view_vector, "
                              "qt_dielectricSpecular(qt_iOR), qt_material_properties2.x);\n";
        } else {
            fragmentShader << "    qt_specularAmount *= qt_defaultMaterialSimpleFresnelNoMetalness(qt_world_normal, qt_view_vector, "
                              "qt_dielectricSpecular(qt_iOR), qt_material_properties2.x);\n";
        }
    }
}

static QSSGMaterialShaderGenerator::LightVariableNames setupLightVariableNames(qint32 lightIdx, QSSGRenderLight &inLight)
{
    Q_ASSERT(lightIdx > -1);
    QSSGMaterialShaderGenerator::LightVariableNames names;

    // See funcsampleLightVars.glsllib. Using an instance name (ubLights) is
    // intentional. The only uniform block that does not have an instance name
    // is cbMain (the main block with all default and custom material
    // uniforms). Any other uniform block must have an instance name in order
    // to avoid trouble with the OpenGL-targeted shaders generated by the
    // shader pipeline (as those do not use uniform blocks, and in absence of a
    // block instance name SPIR-Cross generates a struct uniform name based on
    // whatever SPIR-V ID glslang made up for the variable - this can lead to
    // clashes between the vertex and fragment shaders if there are blocks with
    // different names (but no instance names) that are only present in one of
    // the shaders). For cbMain the issue cannot happen since the exact same
    // block is present in both shaders. For cbLights it is simple enough to
    // use the correct prefix right here, so there is no reason not to use an
    // instance name.
    QByteArray lightStem = "ubLights.lights";
    char buf[16];
    std::snprintf(buf, 16, "[%d].", lightIdx);
    lightStem.append(buf);

    names.lightColor = lightStem;
    names.lightColor.append("diffuse");
    names.lightDirection = lightStem;
    names.lightDirection.append("direction");
    names.lightSpecularColor = lightStem;
    names.lightSpecularColor.append("specular");
    if (inLight.type == QSSGRenderLight::Type::PointLight) {
        names.lightPos = lightStem;
        names.lightPos.append("position");
        names.lightConstantAttenuation = lightStem;
        names.lightConstantAttenuation.append("constantAttenuation");
        names.lightLinearAttenuation = lightStem;
        names.lightLinearAttenuation.append("linearAttenuation");
        names.lightQuadraticAttenuation = lightStem;
        names.lightQuadraticAttenuation.append("quadraticAttenuation");
    } else if (inLight.type == QSSGRenderLight::Type::SpotLight) {
        names.lightPos = lightStem;
        names.lightPos.append("position");
        names.lightConstantAttenuation = lightStem;
        names.lightConstantAttenuation.append("constantAttenuation");
        names.lightLinearAttenuation = lightStem;
        names.lightLinearAttenuation.append("linearAttenuation");
        names.lightQuadraticAttenuation = lightStem;
        names.lightQuadraticAttenuation.append("quadraticAttenuation");
        names.lightConeAngle = lightStem;
        names.lightConeAngle.append("coneAngle");
        names.lightInnerConeAngle = lightStem;
        names.lightInnerConeAngle.append("innerConeAngle");
    }

    return names;
}

static void generateShadowMapOcclusion(QSSGStageGeneratorBase &fragmentShader,
                                       QSSGMaterialVertexPipeline &vertexShader,
                                       quint32 shadowMapIdx,
                                       quint32 shadowMapRes,
                                       bool is32bit,
                                       QSSGRenderLight::SoftShadowQuality softShadowQuality,
                                       bool inShadowEnabled,
                                       QSSGRenderLight::Type inType,
                                       const QSSGMaterialShaderGenerator::LightVariableNames &lightVarNames,
                                       const QSSGShaderDefaultMaterialKey &inKey)
{
    if (inShadowEnabled) {
        vertexShader.generateWorldPosition(inKey);
        const auto& names = setupShadowMapVariableNames(shadowMapIdx, shadowMapRes, is32bit);
        fragmentShader.addInclude("shadowMapping.glsllib");

        QByteArray sampleFunctionSuffix;
        switch (softShadowQuality) {
            case QSSGRenderLight::SoftShadowQuality::Hard:
                sampleFunctionSuffix += "hard";
                break;
            case QSSGRenderLight::SoftShadowQuality::PCF4:
                sampleFunctionSuffix += "pcf_4";
                break;
            case QSSGRenderLight::SoftShadowQuality::PCF8:
                sampleFunctionSuffix += "pcf_8";
                break;
            case QSSGRenderLight::SoftShadowQuality::PCF16:
                sampleFunctionSuffix += "pcf_16";
                break;
            case QSSGRenderLight::SoftShadowQuality::PCF32:
                sampleFunctionSuffix += "pcf_32";
                break;
            case QSSGRenderLight::SoftShadowQuality::PCF64:
                sampleFunctionSuffix += "pcf_64";
                break;
            default:
                Q_UNREACHABLE();
        };

        if (inType == QSSGRenderLight::Type::PointLight) {
            fragmentShader.addUniform(names.shadowCube, "samplerCube");
            fragmentShader << "    qt_shadow_map_occl = qt_samplePointLight_" << sampleFunctionSuffix << "(" << names.shadowCube << ", " << names.shadowData << ", " << lightVarNames.lightPos << ".xyz, qt_varWorldPos);\n";
        } else if (inType == QSSGRenderLight::Type::DirectionalLight || inType == QSSGRenderLight::Type::SpotLight) {
            if (!fragmentShader.m_uniforms.contains(names.shadowMapTexture)) {
                fragmentShader.addUniform(names.shadowMapTexture, "sampler2DArray");
            }
            if (inType == QSSGRenderLight::Type::DirectionalLight) {
                fragmentShader << "    qt_shadow_map_occl = qt_sampleDirectionalLight_" << sampleFunctionSuffix << "(" << names.shadowMapTexture << ", " << names.shadowData << ", qt_zDepthViewSpace, qt_varWorldPos);\n";
            } else {
                fragmentShader << "    qt_shadow_map_occl = qt_sampleSpotLight_" << sampleFunctionSuffix << "(" << names.shadowMapTexture << ", " << names.shadowData << ", " << lightVarNames.lightPos << ".xyz, qt_varWorldPos, " << lightVarNames.lightDirection << ".xyz, " << lightVarNames.lightConeAngle << ");\n";
            }
        } else {
            Q_UNREACHABLE();
        }
    } else {
        fragmentShader << "    qt_shadow_map_occl = 1.0;\n";
    }
}

static inline QSSGShaderMaterialAdapter *getMaterialAdapter(const QSSGRenderGraphObject &inMaterial)
{
    switch (inMaterial.type) {
    case QSSGRenderGraphObject::Type::DefaultMaterial:
    case QSSGRenderGraphObject::Type::PrincipledMaterial:
    case QSSGRenderGraphObject::Type::SpecularGlossyMaterial:
        return static_cast<const QSSGRenderDefaultMaterial &>(inMaterial).adapter;
    case QSSGRenderGraphObject::Type::CustomMaterial:
        return static_cast<const QSSGRenderCustomMaterial &>(inMaterial).adapter;
    default:
        break;
    }
    return nullptr;
}

// NOTE!!!: PLEASE ADD NEW VARS HERE!
static constexpr QByteArrayView qssg_shader_arg_names[] {
    { "DIFFUSE" },
    { "BASE_COLOR" },
    { "METALNESS" },
    { "ROUGHNESS" },
    { "EMISSIVE" },
    { "SPECULAR_AMOUNT" },
    { "EMISSIVE_COLOR" },
    { "LIGHT_COLOR" },
    { "LIGHT_ATTENUATION" },
    { "SPOT_FACTOR" },
    { "SHADOW_CONTRIB" },
    { "FRESNEL_CONTRIB" },
    { "TO_LIGHT_DIR" },
    { "NORMAL" },
    { "VIEW_VECTOR" },
    { "TOTAL_AMBIENT_COLOR" },
    { "COLOR_SUM" },
    { "BINORMAL" },
    { "TANGENT" },
    { "FRESNEL_POWER" },
    { "INSTANCE_MODEL_MATRIX" },
    { "INSTANCE_MODELVIEWPROJECTION_MATRIX" },
    { "UV0" },
    { "UV1" },
    { "VERTEX" },
    { "FRESNEL_SCALE" },
    { "FRESNEL_BIAS" },
    { "CLEARCOAT_FRESNEL_POWER" },
    { "CLEARCOAT_FRESNEL_SCALE" },
    { "CLEARCOAT_FRESNEL_BIAS" },
    { "CLEARCOAT_AMOUNT" },
    { "CLEARCOAT_NORMAL" },
    { "CLEARCOAT_ROUGHNESS" },
    { "IOR" },
    { "TRANSMISSION_FACTOR" },
    { "THICKNESS_FACTOR" },
    { "ATTENUATION_COLOR" },
    { "ATTENUATION_DISTANCE" },
    { "OCCLUSION_AMOUNT" },
};

const char *QSSGMaterialShaderGenerator::directionalLightProcessorArgumentList()
{
    return "inout vec3 DIFFUSE, in vec3 LIGHT_COLOR, in float SHADOW_CONTRIB, in vec3 TO_LIGHT_DIR, in vec3 NORMAL, in vec4 BASE_COLOR, in float METALNESS, in float ROUGHNESS, in vec3 VIEW_VECTOR";
}

const char *QSSGMaterialShaderGenerator::pointLightProcessorArgumentList()
{
    return "inout vec3 DIFFUSE, in vec3 LIGHT_COLOR, in float LIGHT_ATTENUATION, in float SHADOW_CONTRIB, in vec3 TO_LIGHT_DIR, in vec3 NORMAL, in vec4 BASE_COLOR, in float METALNESS, in float ROUGHNESS, in vec3 VIEW_VECTOR";
}

const char *QSSGMaterialShaderGenerator::spotLightProcessorArgumentList()
{
    return "inout vec3 DIFFUSE, in vec3 LIGHT_COLOR, in float LIGHT_ATTENUATION, float SPOT_FACTOR, in float SHADOW_CONTRIB, in vec3 TO_LIGHT_DIR, in vec3 NORMAL, in vec4 BASE_COLOR, in float METALNESS, in float ROUGHNESS, in vec3 VIEW_VECTOR";
}

const char *QSSGMaterialShaderGenerator::ambientLightProcessorArgumentList()
{
    return "inout vec3 DIFFUSE, in vec3 TOTAL_AMBIENT_COLOR, in vec3 NORMAL, in vec3 VIEW_VECTOR";
}

const char *QSSGMaterialShaderGenerator::specularLightProcessorArgumentList()
{
    return "inout vec3 SPECULAR, in vec3 LIGHT_COLOR, in float LIGHT_ATTENUATION, in float SHADOW_CONTRIB, in vec3 FRESNEL_CONTRIB, in vec3 TO_LIGHT_DIR, in vec3 NORMAL, in vec4 BASE_COLOR, in float METALNESS, in float ROUGHNESS, in float SPECULAR_AMOUNT, in vec3 VIEW_VECTOR";
}

const char *QSSGMaterialShaderGenerator::shadedFragmentMainArgumentList()
{
    return "inout vec4 BASE_COLOR, inout vec3 EMISSIVE_COLOR, inout float METALNESS, inout float ROUGHNESS, inout float SPECULAR_AMOUNT, inout float FRESNEL_POWER, inout vec3 NORMAL, inout vec3 TANGENT, inout vec3 BINORMAL, in vec2 UV0, in vec2 UV1, in vec3 VIEW_VECTOR, inout float IOR, inout float OCCLUSION_AMOUNT";
}

const char *QSSGMaterialShaderGenerator::postProcessorArgumentList()
{
    return "inout vec4 COLOR_SUM, in vec4 DIFFUSE, in vec3 SPECULAR, in vec3 EMISSIVE, in vec2 UV0, in vec2 UV1";
}

const char *QSSGMaterialShaderGenerator::iblProbeProcessorArgumentList()
{
    return "inout vec3 DIFFUSE, inout vec3 SPECULAR, in vec4 BASE_COLOR, in float AO_FACTOR, in float SPECULAR_AMOUNT, in float ROUGHNESS, in vec3 NORMAL, in vec3 VIEW_VECTOR, in mat3 IBL_ORIENTATION";
}

const char *QSSGMaterialShaderGenerator::vertexMainArgumentList()
{
    return "inout vec3 VERTEX, inout vec3 NORMAL, inout vec2 UV0, inout vec2 UV1, inout vec3 TANGENT, inout vec3 BINORMAL, inout ivec4 JOINTS, inout vec4 WEIGHTS, inout vec4 COLOR";
}

const char *QSSGMaterialShaderGenerator::vertexInstancedMainArgumentList()
{
    return "inout vec3 VERTEX, inout vec3 NORMAL, inout vec2 UV0, inout vec2 UV1, inout vec3 TANGENT, inout vec3 BINORMAL, inout ivec4 JOINTS, inout vec4 WEIGHTS, inout vec4 COLOR, inout mat4 INSTANCE_MODEL_MATRIX, inout mat4 INSTANCE_MODELVIEWPROJECTION_MATRIX";
}

#define MAX_MORPH_TARGET 8

static bool hasCustomFunction(const QByteArray &funcName,
                              QSSGShaderMaterialAdapter *materialAdapter,
                              QSSGShaderLibraryManager &shaderLibraryManager)
{
    return materialAdapter->hasCustomShaderFunction(QSSGShaderCache::ShaderType::Fragment, funcName, shaderLibraryManager);
}

static void generateTempLightColor(QSSGStageGeneratorBase &fragmentShader,
                                   QSSGMaterialShaderGenerator::LightVariableNames& lightVarNames,
                                   QSSGShaderMaterialAdapter *materialAdapter)
{
    if (materialAdapter->isSpecularGlossy())
        fragmentShader << "    tmp_light_color = " << lightVarNames.lightColor << ".rgb;\n";
    else
        fragmentShader << "    tmp_light_color = " << lightVarNames.lightColor << ".rgb * (1.0 - qt_metalnessAmount);\n";
}

static void handleSpecularLight(QSSGStageGeneratorBase &fragmentShader,
                                QSSGMaterialShaderGenerator::LightVariableNames& lightVarNames,
                                QSSGShaderMaterialAdapter *materialAdapter,
                                QSSGShaderLibraryManager &shaderLibraryManager,
                                bool usesSharedVar,
                                bool hasCustomFrag,
                                bool specularLightingEnabled,
                                bool enableClearcoat,
                                bool enableTransmission,
                                bool useNormalizedDirection)
{
    QByteArray directionToUse = useNormalizedDirection ? lightVarNames.normalizedDirection : lightVarNames.lightDirection;

    if (hasCustomFrag && hasCustomFunction(QByteArrayLiteral("qt_specularLightProcessor"), materialAdapter, shaderLibraryManager))
    {
        // SPECULAR, LIGHT_COLOR, LIGHT_ATTENUATION, SHADOW_CONTRIB, FRESNEL_CONTRIB, TO_LIGHT_DIR, NORMAL, BASE_COLOR, METALNESS, ROUGHNESS, SPECULAR_AMOUNT, VIEW_VECTOR(, SHARED)
        fragmentShader << "    qt_specularLightProcessor(global_specular_light, " << lightVarNames.lightSpecularColor << ".rgb, qt_lightAttenuation, qt_shadow_map_occl, "
                       << "qt_specularAmount, -" << directionToUse << ".xyz, qt_world_normal, qt_customBaseColor, "
                       << "qt_metalnessAmount, qt_roughnessAmount, qt_customSpecularAmount, qt_view_vector";
        if (usesSharedVar)
            fragmentShader << ", qt_customShared);\n";
        else
            fragmentShader << ");\n";
    }
    else
    {
        if (specularLightingEnabled)
        {
            if (materialAdapter->isPrincipled() || materialAdapter->isSpecularGlossy())
            {
                // Principled materials (and Custom without a specular processor function) always use GGX SpecularModel
                fragmentShader.addFunction("specularGGXBSDF");
                fragmentShader << "    global_specular_light += qt_lightAttenuation * qt_shadow_map_occl * qt_specularTint"
                                  " * qt_specularGGXBSDF(qt_world_normal, -"
                               << directionToUse << ".xyz, qt_view_vector, "
                               << lightVarNames.lightSpecularColor << ".rgb, qt_f0, qt_f90, qt_roughnessAmount).rgb;\n";
            }
            else
            {
                outputSpecularEquation(materialAdapter->specularModel(), fragmentShader, directionToUse, lightVarNames.lightSpecularColor);
            }

            if (enableClearcoat)
            {
                fragmentShader.addFunction("specularGGXBSDF");
                fragmentShader << "    qt_global_clearcoat += qt_lightAttenuation * qt_shadow_map_occl"
                                  " * qt_specularGGXBSDF(qt_clearcoatNormal, -"
                               << directionToUse << ".xyz, qt_view_vector, "
                               << lightVarNames.lightSpecularColor << ".rgb, qt_clearcoatF0, qt_clearcoatF90, qt_clearcoatRoughness).rgb;\n";
            }

            if (enableTransmission)
            {
                fragmentShader << "    {\n";
                fragmentShader << "        vec3 transmissionRay = qt_getVolumeTransmissionRay(qt_world_normal, qt_view_vector, qt_thicknessFactor, qt_iOR);\n";
                fragmentShader << "        vec3 pointToLight = -" << directionToUse << ".xyz;\n";
                fragmentShader << "        pointToLight -= transmissionRay;\n";
                fragmentShader << "        vec3 l = normalize(pointToLight);\n";
                fragmentShader << "        vec3 intensity = vec3(1.0);\n"; // Directional light is always 1.0
                fragmentShader << "        vec3 transmittedLight = intensity * qt_getPunctualRadianceTransmission(qt_world_normal, "
                                  "qt_view_vector, l, qt_roughnessAmount, qt_f0, qt_f90, qt_diffuseColor.rgb, qt_iOR);\n";
                fragmentShader << "        transmittedLight = qt_applyVolumeAttenuation(transmittedLight, length(transmissionRay), "
                                  "qt_attenuationColor, qt_attenuationDistance);\n";
                fragmentShader << "        qt_global_transmission += qt_transmissionFactor * transmittedLight;\n";
                fragmentShader << "    }\n";
            }
        }
    }
}

static void handleDirectionalLight(QSSGStageGeneratorBase &fragmentShader,
                                   QSSGMaterialShaderGenerator::LightVariableNames& lightVarNames,
                                   bool usesSharedVar,
                                   bool hasCustomFrag,
                                   QSSGShaderMaterialAdapter *materialAdapter,
                                   QSSGShaderLibraryManager &shaderLibraryManager,
                                   bool specularLightingEnabled,
                                   bool enableClearcoat,
                                   bool enableTransmission)
{
    if (hasCustomFrag && hasCustomFunction(QByteArrayLiteral("qt_directionalLightProcessor"), materialAdapter, shaderLibraryManager)) {
        // DIFFUSE, LIGHT_COLOR, SHADOW_CONTRIB, TO_LIGHT_DIR, NORMAL, BASE_COLOR, METALNESS, ROUGHNESS, VIEW_VECTOR(, SHARED)
        fragmentShader << "    qt_directionalLightProcessor(global_diffuse_light.rgb, tmp_light_color, qt_shadow_map_occl, -"
                       << lightVarNames.lightDirection << ".xyz, qt_world_normal, qt_customBaseColor, "
                       << "qt_metalnessAmount, qt_roughnessAmount, qt_view_vector";
        if (usesSharedVar)
            fragmentShader << ", qt_customShared);\n";
        else
            fragmentShader << ");\n";
    } else {
        if (materialAdapter->isPrincipled() || materialAdapter->isSpecularGlossy()) {
            fragmentShader << "    global_diffuse_light.rgb += qt_diffuseColor.rgb * qt_shadow_map_occl * "
                           << "qt_diffuseBurleyBSDF(qt_world_normal, -" << lightVarNames.lightDirection << ".xyz, "
                           << "qt_view_vector, tmp_light_color, qt_roughnessAmount).rgb;\n";
        } else {
            fragmentShader << "    global_diffuse_light.rgb += qt_diffuseColor.rgb * qt_shadow_map_occl * qt_diffuseReflectionBSDF(qt_world_normal, -"
                           << lightVarNames.lightDirection << ".xyz, tmp_light_color).rgb;\n";
        }
    }

    // Since handleSpecularLight uses qt_lightAttenuation make sure to reset it
    fragmentShader << "    qt_lightAttenuation = 1.0;\n";

    handleSpecularLight(fragmentShader,
                        lightVarNames,
                        materialAdapter,
                        shaderLibraryManager,
                        usesSharedVar,
                        hasCustomFrag,
                        specularLightingEnabled,
                        enableClearcoat,
                        enableTransmission,
                        false);
}

static void generateDirections(QSSGStageGeneratorBase &fragmentShader,
                               QSSGMaterialShaderGenerator::LightVariableNames& lightVarNames,
                               const QByteArray& lightVarPrefix,
                               QSSGMaterialVertexPipeline &vertexShader,
                               const QSSGShaderDefaultMaterialKey &inKey)
{
    vertexShader.generateWorldPosition(inKey);

    lightVarNames.relativeDirection = lightVarPrefix;
    lightVarNames.relativeDirection.append("relativeDirection");

    lightVarNames.normalizedDirection = lightVarNames.relativeDirection;
    lightVarNames.normalizedDirection.append("_normalized");

    lightVarNames.relativeDistance = lightVarPrefix;
    lightVarNames.relativeDistance.append("distance");

    fragmentShader << "    vec3 " << lightVarNames.relativeDirection << " = qt_varWorldPos - " << lightVarNames.lightPos << ".xyz;\n"
                   << "    float " << lightVarNames.relativeDistance << " = length(" << lightVarNames.relativeDirection << ");\n"
                   << "    vec3 " << lightVarNames.normalizedDirection << " = " << lightVarNames.relativeDirection << " / " << lightVarNames.relativeDistance << ";\n";

}

static void handlePointLight(QSSGStageGeneratorBase &fragmentShader,
                             QSSGMaterialShaderGenerator::LightVariableNames& lightVarNames,
                             QSSGShaderMaterialAdapter *materialAdapter,
                             QSSGShaderLibraryManager &shaderLibraryManager,
                             bool usesSharedVar,
                             bool hasCustomFrag,
                             bool specularLightingEnabled,
                             bool enableClearcoat,
                             bool enableTransmission)
{
    if (hasCustomFrag && hasCustomFunction(QByteArrayLiteral("qt_pointLightProcessor"), materialAdapter, shaderLibraryManager)) {
        // DIFFUSE, LIGHT_COLOR, LIGHT_ATTENUATION, SHADOW_CONTRIB, TO_LIGHT_DIR, NORMAL, BASE_COLOR, METALNESS, ROUGHNESS, VIEW_VECTOR(, SHARED)
        fragmentShader << "    qt_pointLightProcessor(global_diffuse_light.rgb, tmp_light_color, qt_lightAttenuation, qt_shadow_map_occl, -"
                       << lightVarNames.normalizedDirection << ".xyz, qt_world_normal, qt_customBaseColor, "
                       << "qt_metalnessAmount, qt_roughnessAmount, qt_view_vector";
        if (usesSharedVar)
            fragmentShader << ", qt_customShared);\n";
        else
            fragmentShader << ");\n";
    } else {
        if (materialAdapter->isPrincipled() || materialAdapter->isSpecularGlossy()) {
            fragmentShader << "    global_diffuse_light.rgb += qt_diffuseColor.rgb * qt_lightAttenuation * qt_shadow_map_occl * "
                           << "qt_diffuseBurleyBSDF(qt_world_normal, -" << lightVarNames.normalizedDirection << ".xyz, qt_view_vector, "
                           << "tmp_light_color, qt_roughnessAmount).rgb;\n";
        } else {
            fragmentShader << "    global_diffuse_light.rgb += qt_diffuseColor.rgb * qt_lightAttenuation * qt_shadow_map_occl * "
                           << "qt_diffuseReflectionBSDF(qt_world_normal, -" << lightVarNames.normalizedDirection << ".xyz, tmp_light_color).rgb;\n";
        }
    }

    handleSpecularLight(fragmentShader,
                        lightVarNames,
                        materialAdapter,
                        shaderLibraryManager,
                        usesSharedVar,
                        hasCustomFrag,
                        specularLightingEnabled,
                        enableClearcoat,
                        enableTransmission,
                        true);
}

static void handleSpotLight(QSSGStageGeneratorBase &fragmentShader,
                             QSSGMaterialShaderGenerator::LightVariableNames& lightVarNames,
                             const QByteArray& lightVarPrefix,
                             QSSGShaderMaterialAdapter *materialAdapter,
                             QSSGShaderLibraryManager &shaderLibraryManager,
                             bool usesSharedVar,
                             bool hasCustomFrag,
                             bool specularLightingEnabled,
                             bool enableClearcoat,
                             bool enableTransmission)
{
    lightVarNames.spotAngle = lightVarPrefix;
    lightVarNames.spotAngle.append("spotAngle");

    fragmentShader << "    float " << lightVarNames.spotAngle << " = dot(" << lightVarNames.normalizedDirection
                   << ", normalize(vec3(" << lightVarNames.lightDirection << ")));\n";
    fragmentShader << "    if (" << lightVarNames.spotAngle << " > " << lightVarNames.lightConeAngle << ") {\n";
    fragmentShader << "    float spotFactor = smoothstep(" << lightVarNames.lightConeAngle
                   << ", " << lightVarNames.lightInnerConeAngle << ", " << lightVarNames.spotAngle
                   << ");\n";

    if (hasCustomFrag && hasCustomFunction(QByteArrayLiteral("qt_spotLightProcessor"), materialAdapter, shaderLibraryManager)) {
        // DIFFUSE, LIGHT_COLOR, LIGHT_ATTENUATION, SPOT_FACTOR, SHADOW_CONTRIB, TO_LIGHT_DIR, NORMAL, BASE_COLOR, METALNESS, ROUGHNESS, VIEW_VECTOR(, SHARED)
        fragmentShader << "    qt_spotLightProcessor(global_diffuse_light.rgb, tmp_light_color, qt_lightAttenuation, spotFactor, qt_shadow_map_occl, -"
                       << lightVarNames.normalizedDirection << ".xyz, qt_world_normal, qt_customBaseColor, "
                       << "qt_metalnessAmount, qt_roughnessAmount, qt_view_vector";
        if (usesSharedVar)
            fragmentShader << ", qt_customShared);\n";
        else
            fragmentShader << ");\n";
    } else {
        if (materialAdapter->isPrincipled() || materialAdapter->isSpecularGlossy()) {
            fragmentShader << "    global_diffuse_light.rgb += qt_diffuseColor.rgb * spotFactor * qt_lightAttenuation * qt_shadow_map_occl * "
                           << "qt_diffuseBurleyBSDF(qt_world_normal, -" << lightVarNames.normalizedDirection << ".xyz, qt_view_vector, "
                           << "tmp_light_color, qt_roughnessAmount).rgb;\n";
        } else {
            fragmentShader << "    global_diffuse_light.rgb += qt_diffuseColor.rgb * spotFactor * qt_lightAttenuation * qt_shadow_map_occl * "
                           << "qt_diffuseReflectionBSDF(qt_world_normal, -" << lightVarNames.normalizedDirection << ".xyz, tmp_light_color).rgb;\n";
        }
    }

    // spotFactor is multipled to qt_lightAttenuation and have an effect on the specularLight.
    fragmentShader << "    qt_lightAttenuation *= spotFactor;\n";

    handleSpecularLight(fragmentShader,
                        lightVarNames,
                        materialAdapter,
                        shaderLibraryManager,
                        usesSharedVar,
                        hasCustomFrag,
                        specularLightingEnabled,
                        enableClearcoat,
                        enableTransmission,
                        true);

    fragmentShader << "    }\n";
}

static void calculatePointLightAttenuation(QSSGStageGeneratorBase &fragmentShader,
                                           QSSGMaterialShaderGenerator::LightVariableNames& lightVarNames)
{
    fragmentShader.addFunction("calculatePointLightAttenuation");

    fragmentShader << "    qt_lightAttenuation = qt_calculatePointLightAttenuation(vec3("
                   << lightVarNames.lightConstantAttenuation << ", " << lightVarNames.lightLinearAttenuation << ", "
                   << lightVarNames.lightQuadraticAttenuation << "), " << lightVarNames.relativeDistance << ");\n";
}

static void generateMainLightCalculation(QSSGStageGeneratorBase &fragmentShader,
                                           QSSGMaterialVertexPipeline &vertexShader,
                                           const QSSGShaderDefaultMaterialKey &inKey,
                                           const QSSGRenderGraphObject &inMaterial,
                                           const QSSGShaderLightListView &lights,
                                           QSSGShaderLibraryManager &shaderLibraryManager,
                                           QSSGRenderableImage *translucencyImage,
                                           bool hasCustomFrag,
                                           bool usesSharedVar,
                                           bool enableLightmap,
                                           bool enableShadowMaps,
                                           bool specularLightingEnabled,
                                           bool enableClearcoat,
                                           bool enableTransmission)
{
    QSSGShaderMaterialAdapter *materialAdapter = getMaterialAdapter(inMaterial);

    // Iterate through all lights
    Q_ASSERT(lights.size() < INT32_MAX);

    int shadowMapCount = 0;
    bool hasAddedZDepthViewSpaceVariable = false;

    for (qint32 lightIdx = 0; lightIdx < lights.size(); ++lightIdx) {
        auto &shaderLight = lights[lightIdx];
        QSSGRenderLight *lightNode = shaderLight.light;

        if (enableLightmap && lightNode->m_fullyBaked)
            continue;

        auto lightVarNames = setupLightVariableNames(lightIdx, *lightNode);

        const bool isDirectional = lightNode->type == QSSGRenderLight::Type::DirectionalLight;
        const bool isSpot = lightNode->type == QSSGRenderLight::Type::SpotLight;
        bool castsShadow = enableShadowMaps && lightNode->m_castShadow && shadowMapCount < QSSG_MAX_NUM_SHADOW_MAPS;

        fragmentShader.append("");
        char lightIdxStr[11];
        std::snprintf(lightIdxStr, 11, "%d", lightIdx);

        QByteArray lightVarPrefix = "light";
        lightVarPrefix.append(lightIdxStr);

        if (isDirectional && !hasAddedZDepthViewSpaceVariable) {
            fragmentShader.append("#if QSHADER_VIEW_COUNT >= 2");
            fragmentShader.append("    float qt_zDepthViewSpace = abs((qt_viewMatrix[0] * vec4(qt_varWorldPos, 1.0)).z);");
            fragmentShader.append("#else");
            fragmentShader.append("    float qt_zDepthViewSpace = abs((qt_viewMatrix * vec4(qt_varWorldPos, 1.0)).z);");
            fragmentShader.append("#endif");
            hasAddedZDepthViewSpaceVariable = true;
        }

        fragmentShader << "    //Light " << lightIdxStr << (isDirectional ? " [directional]" : isSpot ? " [spot]" : " [point]") << "\n";

        lightVarPrefix.append("_");

        generateShadowMapOcclusion(fragmentShader, vertexShader, shadowMapCount, lightNode->m_shadowMapRes, lightNode->m_use32BitShadowmap, lightNode->m_softShadowQuality, castsShadow, lightNode->type, lightVarNames, inKey);

        generateTempLightColor(fragmentShader, lightVarNames, materialAdapter);

        if (isDirectional) {
            handleDirectionalLight(fragmentShader,
                                   lightVarNames,
                                   usesSharedVar,
                                   hasCustomFrag,
                                   materialAdapter,
                                   shaderLibraryManager,
                                   specularLightingEnabled,
                                   enableClearcoat,
                                   enableTransmission);
        } else {
            generateDirections(fragmentShader, lightVarNames, lightVarPrefix, vertexShader, inKey);

            calculatePointLightAttenuation(fragmentShader, lightVarNames);

            addTranslucencyIrradiance(fragmentShader, translucencyImage, lightVarNames);

            if (isSpot) {
                handleSpotLight(fragmentShader,
                                lightVarNames,
                                lightVarPrefix,
                                materialAdapter,
                                shaderLibraryManager,
                                usesSharedVar,
                                hasCustomFrag,
                                specularLightingEnabled,
                                enableClearcoat,
                                enableTransmission);
            } else {
                handlePointLight(fragmentShader,
                                 lightVarNames,
                                 materialAdapter,
                                 shaderLibraryManager,
                                 usesSharedVar,
                                 hasCustomFrag,
                                 specularLightingEnabled,
                                 enableClearcoat,
                                 enableTransmission);
            }
        }

        if (castsShadow)
            ++shadowMapCount;
    }

    fragmentShader.append("");
}

static void generateFragmentShader(QSSGStageGeneratorBase &fragmentShader,
                                   QSSGMaterialVertexPipeline &vertexShader,
                                   const QSSGShaderDefaultMaterialKey &inKey,
                                   const QSSGShaderDefaultMaterialKeyProperties &keyProps,
                                   const QSSGShaderFeatures &featureSet,
                                   const QSSGRenderGraphObject &inMaterial,
                                   const QSSGShaderLightListView &lights,
                                   QSSGRenderableImage *firstImage,
                                   QSSGShaderLibraryManager &shaderLibraryManager)
{
    QSSGShaderMaterialAdapter *materialAdapter = getMaterialAdapter(inMaterial);
    auto hasCustomFunction = [&shaderLibraryManager, materialAdapter](const QByteArray &funcName) {
        return materialAdapter->hasCustomShaderFunction(QSSGShaderCache::ShaderType::Fragment,
                                                 funcName,
                                                 shaderLibraryManager);
    };

    bool metalnessEnabled = materialAdapter->isMetalnessEnabled(); // always true for Custom, true if > 0 with Principled

    // alwayas true for Custom,
    // true if vertexColorsEnabled, usesInstancing and blendParticles for others
    bool vertexColorsEnabled = materialAdapter->isVertexColorsEnabled()
                            || materialAdapter->isVertexColorsMaskEnabled()
                            || keyProps.m_usesInstancing.getValue(inKey)
                            || keyProps.m_blendParticles.getValue(inKey);

    bool hasLighting = materialAdapter->hasLighting();
    bool isDoubleSided = keyProps.m_isDoubleSided.getValue(inKey);
    bool hasImage = firstImage != nullptr;

    QSSGRenderLayer::OITMethod oitMethod = static_cast<QSSGRenderLayer::OITMethod>(keyProps.m_orderIndependentTransparency.getValue(inKey));
    bool hasIblProbe = keyProps.m_hasIbl.getValue(inKey);
    bool specularLightingEnabled = metalnessEnabled || materialAdapter->isSpecularEnabled() || hasIblProbe; // always true for Custom, depends for others
    bool specularAAEnabled = keyProps.m_specularAAEnabled.getValue(inKey);
    quint32 numMorphTargets = keyProps.m_targetCount.getValue(inKey);
    // Pull the bump out as
    QSSGRenderableImage *bumpImage = nullptr;
    quint32 imageIdx = 0;
    QSSGRenderableImage *specularAmountImage = nullptr;
    QSSGRenderableImage *roughnessImage = nullptr;
    QSSGRenderableImage *metalnessImage = nullptr;
    QSSGRenderableImage *occlusionImage = nullptr;
    // normal mapping
    QSSGRenderableImage *normalImage = nullptr;
    // translucency map
    QSSGRenderableImage *translucencyImage = nullptr;
    // opacity map
    QSSGRenderableImage *opacityImage = nullptr;
    // height map
    QSSGRenderableImage *heightImage = nullptr;
    // clearcoat maps
    QSSGRenderableImage *clearcoatImage = nullptr;
    QSSGRenderableImage *clearcoatRoughnessImage = nullptr;
    QSSGRenderableImage *clearcoatNormalImage = nullptr;
    // transmission map
    QSSGRenderableImage *transmissionImage = nullptr;
    // thickness
    QSSGRenderableImage *thicknessImage = nullptr;

    QSSGRenderableImage *baseImage = nullptr;

    // Use shared texcoord when transforms are identity
    QVector<QSSGRenderableImage *> identityImages;
    char imageFragCoords[TEXCOORD_VAR_LEN];

    auto channelStr = [](const QSSGShaderKeyTextureChannel &chProp, const QSSGShaderDefaultMaterialKey &inKey) -> QByteArray {
        QByteArray ret;
        switch (chProp.getTextureChannel(inKey)) {
        case QSSGShaderKeyTextureChannel::R:
            ret.append(".r");
            break;
        case QSSGShaderKeyTextureChannel::G:
            ret.append(".g");
            break;
        case QSSGShaderKeyTextureChannel::B:
            ret.append(".b");
            break;
        case QSSGShaderKeyTextureChannel::A:
            ret.append(".a");
            break;
        }
        return ret;
    };

    auto maskVariableByVertexColorChannel = [&fragmentShader, materialAdapter]( const QByteArray &maskVariable
                                                                        , const QSSGRenderDefaultMaterial::VertexColorMask &maskEnum ){
        if (materialAdapter->isVertexColorsMaskEnabled()) {
            if ( materialAdapter->vertexColorRedMask() & maskEnum )
                fragmentShader << "    " << maskVariable << " *= qt_vertColorMask.r;\n";
            else if ( materialAdapter->vertexColorGreenMask() & maskEnum )
                fragmentShader << "    " << maskVariable << " *= qt_vertColorMask.g;\n";
            else if ( materialAdapter->vertexColorBlueMask() & maskEnum )
                fragmentShader << "    " << maskVariable << " *= qt_vertColorMask.b;\n";
            else if ( materialAdapter->vertexColorAlphaMask() & maskEnum )
                fragmentShader << "    " << maskVariable << " *= qt_vertColorMask.a;\n";
        }
    };

    for (QSSGRenderableImage *img = firstImage; img != nullptr; img = img->m_nextImage, ++imageIdx) {
        if (img->m_imageNode.isImageTransformIdentity())
            identityImages.push_back(img);
        if (img->m_mapType == QSSGRenderableImage::Type::BaseColor || img->m_mapType == QSSGRenderableImage::Type::Diffuse) {
            baseImage = img;
        } else if (img->m_mapType == QSSGRenderableImage::Type::Bump) {
            bumpImage = img;
        } else if (img->m_mapType == QSSGRenderableImage::Type::SpecularAmountMap) {
            specularAmountImage = img;
        } else if (img->m_mapType == QSSGRenderableImage::Type::Roughness) {
            roughnessImage = img;
        } else if (img->m_mapType == QSSGRenderableImage::Type::Metalness) {
            metalnessImage = img;
        } else if (img->m_mapType == QSSGRenderableImage::Type::Occlusion) {
            occlusionImage = img;
        } else if (img->m_mapType == QSSGRenderableImage::Type::Normal) {
            normalImage = img;
        } else if (img->m_mapType == QSSGRenderableImage::Type::Translucency) {
            translucencyImage = img;
        } else if (img->m_mapType == QSSGRenderableImage::Type::Opacity) {
            opacityImage = img;
        } else if (img->m_mapType == QSSGRenderableImage::Type::Height) {
            heightImage = img;
        } else if (img->m_mapType == QSSGRenderableImage::Type::Clearcoat) {
            clearcoatImage = img;
        } else if (img->m_mapType == QSSGRenderableImage::Type::ClearcoatRoughness) {
            clearcoatRoughnessImage = img;
        } else if (img->m_mapType == QSSGRenderableImage::Type::ClearcoatNormal) {
            clearcoatNormalImage = img;
        } else if (img->m_mapType == QSSGRenderableImage::Type::Transmission) {
            transmissionImage = img;
        } else if (img->m_mapType == QSSGRenderableImage::Type::Thickness) {
            thicknessImage = img;
        }
    }

    const bool isDepthPass = featureSet.isSet(QSSGShaderFeatures::Feature::DepthPass);
    const bool isOrthoShadowPass = featureSet.isSet(QSSGShaderFeatures::Feature::OrthoShadowPass);
    const bool isPerspectiveShadowPass = featureSet.isSet(QSSGShaderFeatures::Feature::PerspectiveShadowPass);
    const bool isOpaqueDepthPrePass = featureSet.isSet(QSSGShaderFeatures::Feature::OpaqueDepthPrePass);
    const bool hasIblOrientation = featureSet.isSet(QSSGShaderFeatures::Feature::IblOrientation);
    bool enableShadowMaps = featureSet.isSet(QSSGShaderFeatures::Feature::Ssm);
    bool enableSSAO = featureSet.isSet(QSSGShaderFeatures::Feature::Ssao);
    bool enableLightmap = featureSet.isSet(QSSGShaderFeatures::Feature::Lightmap);
    bool hasReflectionProbe = featureSet.isSet(QSSGShaderFeatures::Feature::ReflectionProbe);
    bool enableBumpNormal = normalImage || bumpImage;
    bool genBumpNormalImageCoords = false;
    bool enableParallaxMapping = heightImage != nullptr;
    const bool enableClearcoat = materialAdapter->isClearcoatEnabled();
    const bool enableTransmission = materialAdapter->isTransmissionEnabled();
    const bool enableFresnelScaleBias = materialAdapter->isFresnelScaleBiasEnabled();
    const bool enableClearcoatFresnelScaleBias = materialAdapter->isClearcoatFresnelScaleBiasEnabled();

    specularLightingEnabled |= specularAmountImage != nullptr;
    specularLightingEnabled |= hasReflectionProbe;

    const bool hasCustomVert = materialAdapter->hasCustomShaderSnippet(QSSGShaderCache::ShaderType::Vertex);
    auto debugMode = QSSGRenderLayer::MaterialDebugMode(keyProps.m_debugMode.getValue(inKey));
    const bool enableFog = keyProps.m_fogEnabled.getValue(inKey);

    const int viewCount = featureSet.isSet(QSSGShaderFeatures::Feature::DisableMultiView)
        ? 1 : keyProps.m_viewCount.getValue(inKey);

    // Morphing
    if (numMorphTargets > 0 || hasCustomVert) {
        vertexShader.addDefinition(QByteArrayLiteral("QT_MORPH_MAX_COUNT"),
                    QByteArray::number(numMorphTargets));
        quint8 offset;
        if ((offset = keyProps.m_targetPositionOffset.getValue(inKey)) < UINT8_MAX) {
            vertexShader.addDefinition(QByteArrayLiteral("QT_TARGET_POSITION_OFFSET"),
                                       QByteArray::number(offset));
        }
        if ((offset = keyProps.m_targetNormalOffset.getValue(inKey)) < UINT8_MAX) {
            vertexShader.addDefinition(QByteArrayLiteral("QT_TARGET_NORMAL_OFFSET"),
                                       QByteArray::number(offset));
        }
        if ((offset = keyProps.m_targetTangentOffset.getValue(inKey)) < UINT8_MAX) {
            vertexShader.addDefinition(QByteArrayLiteral("QT_TARGET_TANGENT_OFFSET"),
                                       QByteArray::number(offset));
        }
        if ((offset = keyProps.m_targetBinormalOffset.getValue(inKey)) < UINT8_MAX) {
            vertexShader.addDefinition(QByteArrayLiteral("QT_TARGET_BINORMAL_OFFSET"),
                                       QByteArray::number(offset));
        }
        if ((offset = keyProps.m_targetTexCoord0Offset.getValue(inKey)) < UINT8_MAX) {
            vertexShader.addDefinition(QByteArrayLiteral("QT_TARGET_TEX0_OFFSET"),
                                       QByteArray::number(offset));
        }
        if ((offset = keyProps.m_targetTexCoord1Offset.getValue(inKey)) < UINT8_MAX) {
            vertexShader.addDefinition(QByteArrayLiteral("QT_TARGET_TEX1_OFFSET"),
                                       QByteArray::number(offset));
        }
        if ((offset = keyProps.m_targetColorOffset.getValue(inKey)) < UINT8_MAX) {
            vertexShader.addDefinition(QByteArrayLiteral("QT_TARGET_COLOR_OFFSET"),
                                       QByteArray::number(offset));
        }
    }

    bool includeCustomFragmentMain = true;
    if (isDepthPass || isOrthoShadowPass || isPerspectiveShadowPass) {
        hasLighting = false;
        enableSSAO = false;
        enableShadowMaps = false;
        enableLightmap = false;

        metalnessEnabled = false;
        specularLightingEnabled = false;

        oitMethod = QSSGRenderLayer::OITMethod::None;

        if (!isOpaqueDepthPrePass) {
            vertexColorsEnabled = false;
            baseImage = nullptr;
            includeCustomFragmentMain = false;
        }
    }

    bool includeSSAOVars = enableSSAO || enableShadowMaps;

    vertexShader.beginFragmentGeneration(shaderLibraryManager, oitMethod);

    // Unshaded custom materials need no code in main (apart from calling qt_customMain)
    const bool hasCustomFrag = materialAdapter->hasCustomShaderSnippet(QSSGShaderCache::ShaderType::Fragment);
    const bool usesSharedVar = materialAdapter->usesSharedVariables();
    if (hasCustomFrag && materialAdapter->isUnshaded())
        return;

    // hasCustomFrag == Shaded custom material from this point on, for Unshaded we returned above

    // The fragment or vertex shaders may not use the material_properties or diffuse
    // uniforms in all cases but it is simpler to just add them and let the linker strip them.
    fragmentShader.addUniform("qt_material_emissive_color", "vec3");
    fragmentShader.addUniform("qt_material_base_color", "vec4");
    fragmentShader.addUniform("qt_material_properties", "vec4");
    fragmentShader.addUniform("qt_material_properties2", "vec4");
    fragmentShader.addUniform("qt_material_properties3", "vec4");
    if (enableParallaxMapping || enableTransmission)
        fragmentShader.addUniform("qt_material_properties4", "vec4");
    if (!hasCustomFrag) {
        if (enableTransmission) {
            fragmentShader.addUniform("qt_material_attenuation", "vec4");
            fragmentShader.addUniform("qt_material_thickness", "float");
        }
        if (enableFresnelScaleBias || enableClearcoatFresnelScaleBias)
            fragmentShader.addUniform("qt_material_properties5", "vec4");
        fragmentShader.addUniform("qt_material_clearcoat_normal_strength", "float");
        fragmentShader.addUniform("qt_material_clearcoat_fresnel_power", "float");
    }

    if (vertexColorsEnabled) {
        vertexShader.generateVertexColor(inKey);
    }else {
        fragmentShader.append("    vec4 qt_vertColorMask = vec4(1.0);");
        fragmentShader.append("    vec4 qt_vertColor = vec4(1.0);");
    }

    if (hasImage && ((!isDepthPass && !isOrthoShadowPass && !isPerspectiveShadowPass) || isOpaqueDepthPrePass)) {
        fragmentShader.append("    vec3 qt_uTransform;");
        fragmentShader.append("    vec3 qt_vTransform;");
    }

    if (hasLighting || hasCustomFrag) {
        // Do not move these three. These varyings are exposed to custom material shaders too.
        vertexShader.generateViewVector(inKey);
        if (keyProps.m_usesProjectionMatrix.getValue(inKey)) {
            if (viewCount >= 2)
                fragmentShader.addUniformArray("qt_projectionMatrix", "mat4", viewCount);
            else
                fragmentShader.addUniform("qt_projectionMatrix", "mat4");
        }
        if (keyProps.m_usesInverseProjectionMatrix.getValue(inKey)) {
            if (viewCount >= 2)
                fragmentShader.addUniformArray("qt_inverseProjectionMatrix", "mat4", viewCount);
            else
                fragmentShader.addUniform("qt_inverseProjectionMatrix", "mat4");
        }
        vertexShader.generateWorldNormal(inKey);
        vertexShader.generateWorldPosition(inKey);

        const bool usingDefaultMaterialSpecularGGX = !(materialAdapter->isPrincipled() || materialAdapter->isSpecularGlossy()) && materialAdapter->specularModel() == QSSGRenderDefaultMaterial::MaterialSpecularModel::KGGX;
        // Note: tangetOrBinormalDebugMode doesn't force generation, it just makes sure that qt_tangent and qt_binormal variables exist
        const bool tangentOrBinormalDebugMode = (debugMode == QSSGRenderLayer::MaterialDebugMode::Tangent) || (debugMode == QSSGRenderLayer::MaterialDebugMode::Binormal);
        const bool needsTangentAndBinormal = hasCustomFrag || enableParallaxMapping || clearcoatNormalImage || enableBumpNormal || usingDefaultMaterialSpecularGGX || tangentOrBinormalDebugMode;


        if (needsTangentAndBinormal) {
            bool genTangent = false;
            bool genBinormal = false;
            vertexShader.generateVarTangentAndBinormal(inKey, genTangent, genBinormal);

            if (enableBumpNormal && !genTangent) {
                // Generate imageCoords for bump/normal map first.
                // Some operations needs to use the TBN transform and if the
                // tangent vector is not provided, it is necessary.
                auto *bumpNormalImage = bumpImage != nullptr ? bumpImage : normalImage;
                generateImageUVCoordinates(vertexShader, fragmentShader, inKey, *bumpNormalImage, true, bumpNormalImage->m_imageNode.m_indexUV);
                genBumpNormalImageCoords = true;

                int id = (bumpImage != nullptr) ? int(QSSGRenderableImage::Type::Bump) : int(QSSGRenderableImage::Type::Normal);
                const auto &names = imageStringTable[id];
                fragmentShader << "    vec2 dUVdx = dFdx(" << names.imageFragCoords << ");\n"
                               << "    vec2 dUVdy = dFdy(" << names.imageFragCoords << ");\n";
                fragmentShader << "    qt_tangent = (dUVdy.y * dFdx(qt_varWorldPos) - dUVdx.y * dFdy(qt_varWorldPos)) / (dUVdx.x * dUVdy.y - dUVdx.y * dUVdy.x);\n"
                               << "    qt_tangent = qt_tangent - dot(qt_world_normal, qt_tangent) * qt_world_normal;\n"
                               << "    qt_tangent = normalize(qt_tangent);\n";
            }
            if (!genBinormal)
                fragmentShader << "    qt_binormal = cross(qt_world_normal, qt_tangent);\n";
        }

        if (isDoubleSided) {
            fragmentShader.append("#if QSHADER_HLSL && QSHADER_VIEW_COUNT >= 2");
            fragmentShader.append("    const float qt_facing = 1.0;");
            fragmentShader.append("#else");
            fragmentShader.append("    const float qt_facing = gl_FrontFacing ? 1.0 : -1.0;");
            fragmentShader.append("#endif");
            fragmentShader.append("    qt_world_normal *= qt_facing;\n");
            if (needsTangentAndBinormal) {
                fragmentShader.append("    qt_tangent *= qt_facing;");
                fragmentShader.append("    qt_binormal *= qt_facing;");
            }
        }
    }

    if (hasCustomFrag) {
        // A custom shaded material is effectively a principled material for
        // our purposes here. The defaults are different from a
        // PrincipledMaterial however, since this is more sensible here.
        // (because the shader has to state it to get things)
        // These should match the defaults of PrincipledMaterial.
        fragmentShader << "    float qt_customOcclusionAmount = 1.0;\n";
        fragmentShader << "    float qt_customIOR = 1.5;\n";
        fragmentShader << "    float qt_customSpecularAmount = 0.5;\n"; // overrides qt_material_properties.x
        fragmentShader << "    float qt_customSpecularRoughness = 0.0;\n"; // overrides qt_material_properties.y
        fragmentShader << "    float qt_customMetalnessAmount = 0.0;\n"; // overrides qt_material_properties.z
        fragmentShader << "    float qt_customFresnelPower = 5.0;\n"; // overrides qt_material_properties2.x
        fragmentShader << "    vec4 qt_customBaseColor = vec4(1.0);\n"; // overrides qt_material_base_color
        fragmentShader << "    vec3 qt_customEmissiveColor = vec3(0.0);\n"; // overrides qt_material_emissive_color
        if (enableClearcoat) {
            fragmentShader << "    float qt_customClearcoatAmount = 0.0;\n";
            fragmentShader << "    float qt_customClearcoatFresnelPower = 5.0;\n";
            fragmentShader << "    float qt_customClearcoatRoughness = 0.0;\n";
            fragmentShader << "    vec3 qt_customClearcoatNormal = qt_world_normal;\n";
            if (enableClearcoatFresnelScaleBias) {
                fragmentShader << "    float qt_customClearcoatFresnelScale = 1.0;\n";
                fragmentShader << "    float qt_customClearcoatFresnelBias = 0.0;\n";
            }
        }
        if (enableFresnelScaleBias) {
            fragmentShader << "    float qt_customFresnelScale = 1.0;\n";
            fragmentShader << "    float qt_customFresnelBias = 0.0;\n";
        }

        if (enableTransmission) {
            fragmentShader << "    float qt_customTransmissionFactor = 0.0;\n";
            fragmentShader << "    float qt_customThicknessFactor = 0.0;\n";
            fragmentShader << "    vec3 qt_customAttenuationColor = vec3(1.0);\n";
            fragmentShader << "    float qt_customAttenuationDistance = 0.0;\n";
        }
        if (usesSharedVar)
            fragmentShader << "    QT_SHARED_VARS qt_customShared;\n";
        // Generate the varyings for UV0 and UV1 since customer materials don't use image
        // properties directly.
        vertexShader.generateUVCoords(0, inKey);
        vertexShader.generateUVCoords(1, inKey);
        if (includeCustomFragmentMain && hasCustomFunction(QByteArrayLiteral("qt_customMain"))) {
            fragmentShader << "    qt_customMain(qt_customBaseColor, qt_customEmissiveColor, qt_customMetalnessAmount, qt_customSpecularRoughness,"
                              " qt_customSpecularAmount, qt_customFresnelPower, qt_world_normal, qt_tangent, qt_binormal,"
                              " qt_texCoord0, qt_texCoord1, qt_view_vector, qt_customIOR, qt_customOcclusionAmount";
            if (enableClearcoat) {
                fragmentShader << ", qt_customClearcoatAmount, qt_customClearcoatFresnelPower, qt_customClearcoatRoughness, qt_customClearcoatNormal";
                if (enableClearcoatFresnelScaleBias) {
                    fragmentShader << ", qt_customClearcoatFresnelScale, qt_customClearcoatFresnelBias";
                }
            }
            if (enableFresnelScaleBias) {
                fragmentShader << ", qt_customFresnelScale, qt_customFresnelBias";
            }
            if (enableTransmission) {
                fragmentShader << ", qt_customTransmissionFactor, qt_customThicknessFactor, qt_customAttenuationColor, qt_customAttenuationDistance";
            }
            if (usesSharedVar)
                fragmentShader << ", qt_customShared);\n";
            else
                fragmentShader << ");\n";
        }
        fragmentShader << "    vec4 qt_diffuseColor = qt_customBaseColor * qt_vertColor;\n";
        fragmentShader << "    vec3 qt_global_emission = qt_customEmissiveColor;\n";
        fragmentShader << "    float qt_iOR = qt_customIOR;\n";
    } else {
        fragmentShader << "    vec4 qt_diffuseColor = qt_material_base_color * qt_vertColor;\n";
        fragmentShader << "    vec3 qt_global_emission = qt_material_emissive_color;\n";
        if (specularLightingEnabled || hasImage)
            fragmentShader << "    float qt_iOR = qt_material_specular.w;\n";
    }

    const bool hasCustomIblProbe = hasCustomFrag && hasCustomFunction(QByteArrayLiteral("qt_iblProbeProcessor"));

    if (isDepthPass)
        fragmentShader << "    vec4 fragOutput = vec4(0.0);\n";

    if (isOrthoShadowPass)
        vertexShader.generateDepth();

    if (isPerspectiveShadowPass)
        vertexShader.generateShadowWorldPosition(inKey);

    // !hasLighting does not mean 'no light source'
    // it should be KHR_materials_unlit
    // https://github.com/KhronosGroup/glTF/tree/master/extensions/2.0/Khronos/KHR_materials_unlit
    if (hasLighting) {
        if (includeSSAOVars)
            fragmentShader.addInclude("ssao.glsllib");

        if (enableLightmap) {
            vertexShader.generateLightmapUVCoords(inKey);
            fragmentShader.addFunction("lightmap");
        }

        fragmentShader.addFunction("sampleLightVars");
        if (materialAdapter->isPrincipled() || materialAdapter->isSpecularGlossy())
            fragmentShader.addFunction("diffuseBurleyBSDF");
        else
            fragmentShader.addFunction("diffuseReflectionBSDF");

        if (enableParallaxMapping) {
            // Adjust UV coordinates to account for parallaxMapping before
            // reading any other texture.
            const bool hasIdentityMap = identityImages.contains(heightImage);
            if (hasIdentityMap)
                generateImageUVSampler(vertexShader, fragmentShader, inKey, *heightImage, imageFragCoords, heightImage->m_imageNode.m_indexUV);
            else
                generateImageUVCoordinates(vertexShader, fragmentShader, inKey, *heightImage, true, heightImage->m_imageNode.m_indexUV);
            const auto &names = imageStringTable[int(QSSGRenderableImage::Type::Height)];
            fragmentShader.addInclude("parallaxMapping.glsllib");
            fragmentShader << "    float qt_heightAmount = qt_material_properties4.x;\n";
            maskVariableByVertexColorChannel( "qt_heightAmount", QSSGRenderDefaultMaterial::HeightAmountMask );
            if (viewCount < 2) {
                fragmentShader << "    qt_texCoord0 = qt_parallaxMapping(" << (hasIdentityMap ? imageFragCoords : names.imageFragCoords) << ", " << names.imageSampler
                            <<", qt_tangent, qt_binormal, qt_world_normal, qt_varWorldPos, qt_cameraPosition, qt_heightAmount, qt_material_properties4.y, qt_material_properties4.z);\n";
            } else {
                fragmentShader << "    qt_texCoord0 = qt_parallaxMapping(" << (hasIdentityMap ? imageFragCoords : names.imageFragCoords) << ", " << names.imageSampler
                            <<", qt_tangent, qt_binormal, qt_world_normal, qt_varWorldPos, qt_cameraPosition[qt_viewIndex], qt_heightAmount, qt_material_properties4.y, qt_material_properties4.z);\n";
            }
        }

        // Clearcoat Setup (before normalImage code has a change to overwrite qt_world_normal)
        if (enableClearcoat) {
            addLocalVariable(fragmentShader, "qt_clearcoatNormal", "vec3");
            // Clearcoat normal should be calculated not considering the normalImage for the base material
            // If both are to be the same then just set the same normalImage for the base and clearcoat
            // This does mean that this value should be calculated before qt_world_normal is overwritten by
            // the normalMap.
            if (hasCustomFrag) {
                fragmentShader << "    qt_clearcoatNormal = qt_customClearcoatNormal;\n";
            } else {
                if (clearcoatNormalImage) {
                    generateImageUVCoordinates(vertexShader, fragmentShader, inKey, *clearcoatNormalImage, enableParallaxMapping, clearcoatNormalImage->m_imageNode.m_indexUV);
                    const auto &names = imageStringTable[int(QSSGRenderableImage::Type::ClearcoatNormal)];
                    fragmentShader.addFunction("sampleNormalTexture");
                    fragmentShader << "    float qt_clearcoat_normal_strength = qt_material_clearcoat_normal_strength;\n";
                    maskVariableByVertexColorChannel( "qt_clearcoat_normal_strength", QSSGRenderDefaultMaterial::ClearcoatNormalStrengthMask  );
                    fragmentShader << "    qt_clearcoatNormal = qt_sampleNormalTexture3(" << names.imageSampler << ", qt_clearcoat_normal_strength, " << names.imageFragCoords << ", qt_tangent, qt_binormal, qt_world_normal);\n";

                } else {
                    // same as qt_world_normal then
                    fragmentShader << "    qt_clearcoatNormal = qt_world_normal;\n";
                }
            }
        }

        if (bumpImage != nullptr) {
            if (enableParallaxMapping || !genBumpNormalImageCoords) {
                generateImageUVCoordinates(vertexShader, fragmentShader, inKey,
                                           *bumpImage, enableParallaxMapping,
                                           bumpImage->m_imageNode.m_indexUV,
                                           genBumpNormalImageCoords);
            }
            const auto &names = imageStringTable[int(QSSGRenderableImage::Type::Bump)];
            fragmentShader.addUniform(names.imageSamplerSize, "vec2");
            fragmentShader.append("    float qt_bumpAmount = qt_material_properties2.y;\n");
            maskVariableByVertexColorChannel( "qt_bumpAmount", QSSGRenderDefaultMaterial::NormalStrengthMask );
            fragmentShader.addInclude("defaultMaterialBumpNoLod.glsllib");
            fragmentShader << "    qt_world_normal = qt_defaultMaterialBumpNoLod(" << names.imageSampler << ", qt_bumpAmount, " << names.imageFragCoords << ", qt_tangent, qt_binormal, qt_world_normal, " << names.imageSamplerSize << ");\n";
        } else if (normalImage != nullptr) {
            if (enableParallaxMapping || !genBumpNormalImageCoords) {
                generateImageUVCoordinates(vertexShader, fragmentShader, inKey,
                                           *normalImage, enableParallaxMapping,
                                           normalImage->m_imageNode.m_indexUV,
                                           genBumpNormalImageCoords);
            }
            const auto &names = imageStringTable[int(QSSGRenderableImage::Type::Normal)];
            fragmentShader.append("    float qt_normalStrength = qt_material_properties2.y;\n");
            maskVariableByVertexColorChannel( "qt_normalStrength", QSSGRenderDefaultMaterial::NormalStrengthMask );
            fragmentShader.addFunction("sampleNormalTexture");
            fragmentShader << "    qt_world_normal = qt_sampleNormalTexture3(" << names.imageSampler << ", qt_normalStrength, " << names.imageFragCoords << ", qt_tangent, qt_binormal, qt_world_normal);\n";
        }

        fragmentShader.append("    vec3 tmp_light_color;");
    }

    if (specularLightingEnabled || hasImage) {
        fragmentShader.append("    vec3 qt_specularBase;");
        fragmentShader.addUniform("qt_material_specular", "vec4");
        if (hasCustomFrag)
            fragmentShader.append("    vec3 qt_specularTint = vec3(1.0);");
        else
            fragmentShader.append("    vec3 qt_specularTint = qt_material_specular.rgb;");
    }

    if (baseImage) {
        const bool hasIdentityMap = identityImages.contains(baseImage);
        if (hasIdentityMap)
            generateImageUVSampler(vertexShader, fragmentShader, inKey, *baseImage, imageFragCoords, baseImage->m_imageNode.m_indexUV);
        else
            generateImageUVCoordinates(vertexShader, fragmentShader, inKey, *baseImage, enableParallaxMapping, baseImage->m_imageNode.m_indexUV);

        // NOTE: The base image hande is used for both the diffuse map and the base color map, so we can't hard-code the type here...
        const auto &names = imageStringTable[int(baseImage->m_mapType)];
        // Diffuse and BaseColor maps need to converted to linear color space
        fragmentShader.addInclude("tonemapping.glsllib");
        if (materialAdapter->isBaseColorSingleChannelEnabled()) {
            const auto &channelProps = keyProps.m_textureChannels[QSSGShaderDefaultMaterialKeyProperties::BaseColorChannel];
            fragmentShader << "    vec4 qt_base_texture_color = vec4(vec3(texture2D(" << names.imageSampler << ", " << (hasIdentityMap ? imageFragCoords : names.imageFragCoords) << ")" << channelStr(channelProps, inKey) << "), 1.0f);\n";
        } else {
            fragmentShader << "    vec4 qt_base_texture_color = texture2D(" << names.imageSampler << ", " << (hasIdentityMap ? imageFragCoords : names.imageFragCoords) << ");\n";
        }
        if (!keyProps.m_imageMaps[QSSGShaderDefaultMaterialKeyProperties::BaseColorMap].isLinear(inKey))
            fragmentShader << "    qt_base_texture_color = qt_sRGBToLinear(qt_base_texture_color);\n";
        fragmentShader << "    qt_diffuseColor *= qt_base_texture_color;\n";
    }

    // alpha cutoff
    if (materialAdapter->alphaMode() == QSSGRenderDefaultMaterial::MaterialAlphaMode::Mask) {
        // The Implementation Notes from
        // https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#alpha-coverage
        // must be met. Hence the discard.
        fragmentShader << "    if (qt_diffuseColor.a < qt_material_properties3.y) {\n"
                       << "        qt_diffuseColor = vec4(0.0);\n"
                       << "        discard;\n"
                       << "    } else {\n"
                       << "        qt_diffuseColor.a = 1.0;\n"
                       << "    }\n";
    } else if (materialAdapter->alphaMode() == QSSGRenderDefaultMaterial::MaterialAlphaMode::Opaque) {
        fragmentShader << "    qt_diffuseColor.a = 1.0;\n";
    }

    if (opacityImage) {
        const bool hasIdentityMap = identityImages.contains(opacityImage);
        if (hasIdentityMap)
            generateImageUVSampler(vertexShader, fragmentShader, inKey, *opacityImage, imageFragCoords, opacityImage->m_imageNode.m_indexUV);
        else
            generateImageUVCoordinates(vertexShader, fragmentShader, inKey, *opacityImage, enableParallaxMapping, opacityImage->m_imageNode.m_indexUV);

        const auto &names = imageStringTable[int(QSSGRenderableImage::Type::Opacity)];
        const auto &channelProps = keyProps.m_textureChannels[QSSGShaderDefaultMaterialKeyProperties::OpacityChannel];
        fragmentShader << "    float qt_opacity_map_value = texture2D(" << names.imageSampler << ", " << (hasIdentityMap ? imageFragCoords : names.imageFragCoords) << ")" << channelStr(channelProps, inKey) << ";\n";
        if ( materialAdapter->isInvertOpacityMapValue() )
            fragmentShader << "    qt_opacity_map_value = 1.0 - qt_opacity_map_value;\n";
        fragmentShader << "    qt_objectOpacity *= qt_opacity_map_value;\n";
    }

    if (hasLighting) {
        if (specularLightingEnabled) {
            vertexShader.generateViewVector(inKey);
            fragmentShader.addUniform("qt_material_properties", "vec4");

            if (materialAdapter->isPrincipled() || materialAdapter->isSpecularGlossy())
                fragmentShader << "    qt_specularBase = vec3(1.0);\n";
            else
                fragmentShader << "    qt_specularBase = qt_diffuseColor.rgb;\n";
            if (hasCustomFrag)
                fragmentShader << "    float qt_specularFactor = qt_customSpecularAmount;\n";
            else
                fragmentShader << "    float qt_specularFactor = qt_material_properties.x;\n";

            maskVariableByVertexColorChannel( "qt_specularFactor", QSSGRenderDefaultMaterial::SpecularAmountMask  );
        }

        // Metalness must be setup fairly earily since so many factors depend on the runtime value
        if (hasCustomFrag)
            fragmentShader << "    float qt_metalnessAmount = qt_customMetalnessAmount;\n";
        else if (!materialAdapter->isSpecularGlossy())
            fragmentShader << "    float qt_metalnessAmount = qt_material_properties.z;\n";
        else
            fragmentShader << "    float qt_metalnessAmount = 0.0;\n";

        maskVariableByVertexColorChannel( "qt_metalnessAmount", QSSGRenderDefaultMaterial::MetalnessMask );

        if (specularLightingEnabled && metalnessImage) {
            const auto &channelProps = keyProps.m_textureChannels[QSSGShaderDefaultMaterialKeyProperties::MetalnessChannel];
            const bool hasIdentityMap = identityImages.contains(metalnessImage);
            if (hasIdentityMap)
                generateImageUVSampler(vertexShader, fragmentShader, inKey, *metalnessImage, imageFragCoords, metalnessImage->m_imageNode.m_indexUV);
            else
                generateImageUVCoordinates(vertexShader, fragmentShader, inKey, *metalnessImage, enableParallaxMapping, metalnessImage->m_imageNode.m_indexUV);

            const auto &names = imageStringTable[int(QSSGRenderableImage::Type::Metalness)];
            fragmentShader << "    float qt_sampledMetalness = texture2D(" << names.imageSampler << ", "
                           << (hasIdentityMap ? imageFragCoords : names.imageFragCoords) << ")" << channelStr(channelProps, inKey) << ";\n";
            fragmentShader << "    qt_metalnessAmount = clamp(qt_metalnessAmount * qt_sampledMetalness, 0.0, 1.0);\n";
        }

        fragmentShader.addUniform("qt_light_ambient_total", "vec3");

        fragmentShader.append("    vec4 global_diffuse_light = vec4(0.0);");

        if (enableLightmap) {
            fragmentShader << "    global_diffuse_light.rgb = qt_lightmap_color(qt_texCoordLightmap) * (1.0 - qt_metalnessAmount) * qt_diffuseColor.rgb;\n";
        } else {
            if (hasCustomFrag && hasCustomFunction(QByteArrayLiteral("qt_ambientLightProcessor"))) {
                // DIFFUSE, TOTAL_AMBIENT_COLOR, NORMAL, VIEW_VECTOR(, SHARED)
                fragmentShader.append("    qt_ambientLightProcessor(global_diffuse_light.rgb, qt_light_ambient_total.rgb * (1.0 - qt_metalnessAmount) * qt_diffuseColor.rgb, qt_world_normal, qt_view_vector");
                if (usesSharedVar)
                    fragmentShader << ", qt_customShared);\n";
                else
                    fragmentShader << ");\n";
            } else {
                fragmentShader.append("    global_diffuse_light = vec4(qt_light_ambient_total.rgb * (1.0 - qt_metalnessAmount) * qt_diffuseColor.rgb, 0.0);");
            }
        }

        fragmentShader.append("    vec3 global_specular_light = vec3(0.0);");

        if (!lights.isEmpty() || hasCustomFrag) {
            fragmentShader.append("    float qt_shadow_map_occl = 1.0;");
            fragmentShader.append("    float qt_lightAttenuation = 1.0;");
        }

        // Fragment lighting means we can perhaps attenuate the specular amount by a texture
        // lookup.
        if (specularAmountImage) {
            const bool hasIdentityMap = identityImages.contains(specularAmountImage);
            if (hasIdentityMap)
                generateImageUVSampler(vertexShader, fragmentShader, inKey, *specularAmountImage, imageFragCoords, specularAmountImage->m_imageNode.m_indexUV);
            else
                generateImageUVCoordinates(vertexShader, fragmentShader, inKey, *specularAmountImage, enableParallaxMapping, specularAmountImage->m_imageNode.m_indexUV);

            const auto &names = imageStringTable[int(QSSGRenderableImage::Type::SpecularAmountMap )];

        if (materialAdapter->isSpecularAmountSingleChannelEnabled()) {
            const auto &channelProps = keyProps.m_textureChannels[QSSGShaderDefaultMaterialKeyProperties::SpecularAmountChannel];
            fragmentShader << "    vec4 qt_specular_amount_map = vec4(vec3(texture2D(" << names.imageSampler << ", " << (hasIdentityMap ? imageFragCoords : names.imageFragCoords) << ")" << channelStr(channelProps, inKey) << "), 1.0f);\n";
        } else {
            fragmentShader << "    vec4 qt_specular_amount_map = texture2D(" << names.imageSampler << ", " << (hasIdentityMap ? imageFragCoords : names.imageFragCoords) << ");\n";
        }
            fragmentShader << "    qt_specularBase *= qt_sRGBToLinear(qt_specular_amount_map).rgb;\n";
        }

        if (specularLightingEnabled) {
            if (materialAdapter->isSpecularGlossy()) {
                fragmentShader << "    qt_specularTint *= qt_specularBase;\n";
                fragmentShader << "    vec3 qt_specularAmount = vec3(1.0);\n";
            } else {
                fragmentShader << "    vec3 qt_specularAmount = qt_specularBase * vec3(qt_metalnessAmount + qt_specularFactor * (1.0 - qt_metalnessAmount));\n";
            }
        }

        if (translucencyImage != nullptr) {
            const bool hasIdentityMap = identityImages.contains(translucencyImage);
            if (hasIdentityMap)
                generateImageUVSampler(vertexShader, fragmentShader, inKey, *translucencyImage, imageFragCoords);
            else
                generateImageUVCoordinates(vertexShader, fragmentShader, inKey, *translucencyImage, enableParallaxMapping);

            const auto &names = imageStringTable[int(QSSGRenderableImage::Type::Translucency)];
            const auto &channelProps = keyProps.m_textureChannels[QSSGShaderDefaultMaterialKeyProperties::TranslucencyChannel];
            fragmentShader << "    float qt_translucent_depth_range = texture2D(" << names.imageSampler
                           << ", " << (hasIdentityMap ? imageFragCoords : names.imageFragCoords) << ")" << channelStr(channelProps, inKey) << ";\n";
            fragmentShader << "    float qt_translucent_thickness = qt_translucent_depth_range * qt_translucent_depth_range;\n";
            fragmentShader << "    float qt_translucent_thickness_exp = exp(qt_translucent_thickness * qt_material_properties2.z);\n";
        }

        addLocalVariable(fragmentShader, "qt_aoFactor", "float");

        if (enableSSAO)
            fragmentShader.append("    qt_aoFactor = qt_screenSpaceAmbientOcclusionFactor();");
        else
            fragmentShader.append("    qt_aoFactor = 1.0;");

        if (hasCustomFrag) {
            fragmentShader << "    float qt_roughnessAmount = qt_customSpecularRoughness;\n";
            fragmentShader << "    qt_aoFactor *= qt_customOcclusionAmount;\n";
        }
        else {
            fragmentShader << "    float qt_roughnessAmount = qt_material_properties.y;\n";
        }

        maskVariableByVertexColorChannel( "qt_roughnessAmount", QSSGRenderDefaultMaterial::RoughnessMask );


        // Occlusion Map
        if (occlusionImage) {
            addLocalVariable(fragmentShader, "qt_ao", "float");
            const auto &channelProps = keyProps.m_textureChannels[QSSGShaderDefaultMaterialKeyProperties::OcclusionChannel];
            const bool hasIdentityMap = identityImages.contains(occlusionImage);
            if (hasIdentityMap)
                generateImageUVSampler(vertexShader, fragmentShader, inKey, *occlusionImage, imageFragCoords, occlusionImage->m_imageNode.m_indexUV);
            else
                generateImageUVCoordinates(vertexShader, fragmentShader, inKey, *occlusionImage, enableParallaxMapping, occlusionImage->m_imageNode.m_indexUV);
            const auto &names = imageStringTable[int(QSSGRenderableImage::Type::Occlusion)];
            fragmentShader << "    qt_ao = texture2D(" << names.imageSampler << ", "
                           << (hasIdentityMap ? imageFragCoords : names.imageFragCoords) << ")" << channelStr(channelProps, inKey) << ";\n";
            fragmentShader << "    qt_aoFactor *= qt_ao * qt_material_properties3.x;\n";
            // qt_material_properties3.x is the OcclusionAmount
            maskVariableByVertexColorChannel( "qt_aoFactor", QSSGRenderDefaultMaterial::OcclusionAmountMask );
        }

        if (specularLightingEnabled && roughnessImage) {
            const auto &channelProps = keyProps.m_textureChannels[QSSGShaderDefaultMaterialKeyProperties::RoughnessChannel];
            const bool hasIdentityMap = identityImages.contains(roughnessImage);
            if (hasIdentityMap)
                generateImageUVSampler(vertexShader, fragmentShader, inKey, *roughnessImage, imageFragCoords, roughnessImage->m_imageNode.m_indexUV);
            else
                generateImageUVCoordinates(vertexShader, fragmentShader, inKey, *roughnessImage, enableParallaxMapping, roughnessImage->m_imageNode.m_indexUV);

            const auto &names = imageStringTable[int(QSSGRenderableImage::Type::Roughness)];
            fragmentShader << "    qt_roughnessAmount *= texture2D(" << names.imageSampler << ", "
                           << (hasIdentityMap ? imageFragCoords : names.imageFragCoords) << ")" << channelStr(channelProps, inKey) << ";\n";
        }

        // Convert Glossy to Roughness
        if (materialAdapter->isSpecularGlossy())
            fragmentShader << "    qt_roughnessAmount = clamp(1.0 - qt_roughnessAmount, 0.0, 1.0);\n";

        if (enableClearcoat) {
            addLocalVariable(fragmentShader, "qt_clearcoatAmount", "float");
            addLocalVariable(fragmentShader, "qt_clearcoatRoughness", "float");
            addLocalVariable(fragmentShader, "qt_clearcoatF0", "vec3");
            addLocalVariable(fragmentShader, "qt_clearcoatF90", "vec3");
            addLocalVariable(fragmentShader, "qt_global_clearcoat", "vec3");

            if (hasCustomFrag)
                fragmentShader << "    qt_clearcoatAmount = qt_customClearcoatAmount;\n";
            else
                fragmentShader << "    qt_clearcoatAmount = qt_material_properties3.z;\n";
            maskVariableByVertexColorChannel( "qt_clearcoatAmount", QSSGRenderDefaultMaterial::ClearcoatAmountMask  );
            if (hasCustomFrag)
                fragmentShader << "    qt_clearcoatRoughness = qt_customClearcoatRoughness;\n";
            else
                fragmentShader << "    qt_clearcoatRoughness = qt_material_properties3.w;\n";
            maskVariableByVertexColorChannel( "qt_clearcoatRoughness", QSSGRenderDefaultMaterial::ClearcoatRoughnessAmountMask  );
            fragmentShader << "    qt_clearcoatF0 = vec3(((1.0-qt_iOR) * (1.0-qt_iOR)) / ((1.0+qt_iOR) * (1.0+qt_iOR)));\n";
            fragmentShader << "    qt_clearcoatF90 = vec3(1.0);\n";
            fragmentShader << "    qt_global_clearcoat = vec3(0.0);\n";

            if (clearcoatImage) {
                const auto &channelProps = keyProps.m_textureChannels[QSSGShaderDefaultMaterialKeyProperties::ClearcoatChannel];
                const bool hasIdentityMap = identityImages.contains(clearcoatImage);
                if (hasIdentityMap)
                    generateImageUVSampler(vertexShader, fragmentShader, inKey, *clearcoatImage, imageFragCoords, clearcoatImage->m_imageNode.m_indexUV);
                else
                    generateImageUVCoordinates(vertexShader, fragmentShader, inKey, *clearcoatImage, enableParallaxMapping, clearcoatImage->m_imageNode.m_indexUV);
                const auto &names = imageStringTable[int(QSSGRenderableImage::Type::Clearcoat)];
                fragmentShader << "    qt_clearcoatAmount *= texture2D(" << names.imageSampler << ", "
                               << (hasIdentityMap ? imageFragCoords : names.imageFragCoords) << ")" << channelStr(channelProps, inKey) << ";\n";
            }

            if (clearcoatRoughnessImage) {
                const auto &channelProps = keyProps.m_textureChannels[QSSGShaderDefaultMaterialKeyProperties::ClearcoatRoughnessChannel];
                const bool hasIdentityMap = identityImages.contains(clearcoatRoughnessImage);
                if (hasIdentityMap)
                    generateImageUVSampler(vertexShader, fragmentShader, inKey, *clearcoatRoughnessImage, imageFragCoords, clearcoatRoughnessImage->m_imageNode.m_indexUV);
                else
                    generateImageUVCoordinates(vertexShader, fragmentShader, inKey, *clearcoatRoughnessImage, enableParallaxMapping, clearcoatRoughnessImage->m_imageNode.m_indexUV);
                const auto &names = imageStringTable[int(QSSGRenderableImage::Type::ClearcoatRoughness)];
                fragmentShader << "    qt_clearcoatRoughness *= texture2D(" << names.imageSampler << ", "
                               << (hasIdentityMap ? imageFragCoords : names.imageFragCoords) << ")" << channelStr(channelProps, inKey) << ";\n";
                fragmentShader << "    qt_clearcoatRoughness = clamp(qt_clearcoatRoughness, 0.0, 1.0);\n";
            }
        }

        if (enableTransmission) {
            fragmentShader.addInclude("transmission.glsllib");
            addLocalVariable(fragmentShader, "qt_transmissionFactor", "float");
            addLocalVariable(fragmentShader, "qt_global_transmission", "vec3");
            // Volume
            addLocalVariable(fragmentShader, "qt_thicknessFactor", "float");
            addLocalVariable(fragmentShader, "qt_attenuationColor", "vec3");
            addLocalVariable(fragmentShader, "qt_attenuationDistance", "float");
            fragmentShader << "    qt_global_transmission = vec3(0.0);\n";

            if (hasCustomFrag) {
                fragmentShader << "    qt_transmissionFactor = qt_customTransmissionFactor;\n";
                fragmentShader << "    qt_thicknessFactor = qt_customThicknessFactor;\n";
                fragmentShader << "    qt_attenuationColor = qt_customAttenuationColor;\n";
                fragmentShader << "    qt_attenuationDistance = qt_customAttenuationDistance;\n";
            } else {
                fragmentShader << "    qt_transmissionFactor = qt_material_properties4.w;\n";
                maskVariableByVertexColorChannel( "qt_transmissionFactor", QSSGRenderDefaultMaterial::TransmissionFactorMask );

                if (transmissionImage) {
                    const auto &channelProps = keyProps.m_textureChannels[QSSGShaderDefaultMaterialKeyProperties::TransmissionChannel];
                    const bool hasIdentityMap = identityImages.contains(transmissionImage);
                    if (hasIdentityMap)
                        generateImageUVSampler(vertexShader, fragmentShader, inKey, *transmissionImage, imageFragCoords, transmissionImage->m_imageNode.m_indexUV);
                    else
                        generateImageUVCoordinates(vertexShader, fragmentShader, inKey, *transmissionImage, enableParallaxMapping, transmissionImage->m_imageNode.m_indexUV);
                    const auto &names = imageStringTable[int(QSSGRenderableImage::Type::Transmission)];
                    fragmentShader << "    qt_transmissionFactor *= texture2D(" << names.imageSampler << ", "
                                   << (hasIdentityMap ? imageFragCoords : names.imageFragCoords) << ")" << channelStr(channelProps, inKey) << ";\n";
                }

                fragmentShader << "    qt_thicknessFactor = qt_material_thickness;\n";
                maskVariableByVertexColorChannel( "qt_thicknessFactor", QSSGRenderDefaultMaterial::ThicknessFactorMask );
                fragmentShader << "    qt_attenuationColor = qt_material_attenuation.xyz;\n";
                fragmentShader << "    qt_attenuationDistance = qt_material_attenuation.w;\n";

                if (thicknessImage) {
                    const auto &channelProps = keyProps.m_textureChannels[QSSGShaderDefaultMaterialKeyProperties::ThicknessChannel];
                    const bool hasIdentityMap = identityImages.contains(thicknessImage);
                    if (hasIdentityMap)
                        generateImageUVSampler(vertexShader, fragmentShader, inKey, *thicknessImage, imageFragCoords, thicknessImage->m_imageNode.m_indexUV);
                    else
                        generateImageUVCoordinates(vertexShader, fragmentShader, inKey, *thicknessImage, enableParallaxMapping, thicknessImage->m_imageNode.m_indexUV);
                    const auto &names = imageStringTable[int(QSSGRenderableImage::Type::Thickness)];
                    fragmentShader << "    qt_thicknessFactor *= texture2D(" << names.imageSampler << ", "
                                   << (hasIdentityMap ? imageFragCoords : names.imageFragCoords) << ")" << channelStr(channelProps, inKey) << ";\n";
                }
            }
        }

        if (specularLightingEnabled) {
            if (materialAdapter->isPrincipled() || materialAdapter->isSpecularGlossy()) {
                fragmentShader.addInclude("principledMaterialFresnel.glsllib");
                const bool useF90 = !lights.isEmpty() || enableTransmission;
                addLocalVariable(fragmentShader, "qt_f0", "vec3");
                if (useF90)
                    addLocalVariable(fragmentShader, "qt_f90", "vec3");
                if (materialAdapter->isPrincipled()) {
                    fragmentShader << "    qt_f0 = qt_F0_ior(qt_iOR, qt_metalnessAmount, qt_diffuseColor.rgb);\n";
                    if (useF90)
                        fragmentShader << "    qt_f90 = vec3(1.0);\n";
                } else {
                    addLocalVariable(fragmentShader, "qt_reflectance", "float");

                    fragmentShader << "    qt_reflectance = max(max(qt_specularTint.r, qt_specularTint.g), qt_specularTint.b);\n";
                    fragmentShader << "    qt_f0 = qt_specularTint;\n";
                    fragmentShader << "    qt_specularTint = vec3(1.0);\n";
                    if (useF90)
                        fragmentShader << "    qt_f90 = vec3(clamp(qt_reflectance * 50.0, 0.0, 1.0));\n";
                    fragmentShader << "    qt_diffuseColor.rgb *= (1 - qt_reflectance);\n";
                }

                if (specularAAEnabled) {
                    fragmentShader.append("    vec3 vNormalWsDdx = dFdx(qt_world_normal.xyz);\n");
                    fragmentShader.append("    vec3 vNormalWsDdy = dFdy(qt_world_normal.xyz);\n");
                    fragmentShader.append("    float flGeometricRoughnessFactor = pow(clamp(max(dot(vNormalWsDdx, vNormalWsDdx), dot(vNormalWsDdy, vNormalWsDdy)), 0.0, 1.0), 0.333);\n");
                    fragmentShader.append("    qt_roughnessAmount = max(flGeometricRoughnessFactor, qt_roughnessAmount);\n");
                }

                if (hasCustomFrag)
                    fragmentShader << "    float qt_fresnelPower = qt_customFresnelPower;\n";
                else
                    fragmentShader << "    float qt_fresnelPower = qt_material_properties2.x;\n";

                if (materialAdapter->isPrincipled() || materialAdapter->isSpecularGlossy()) {
                    fragmentShader << "    vec3 qt_principledMaterialFresnelValue = qt_principledMaterialFresnel(qt_world_normal, qt_view_vector, "
                                   << "qt_f0, qt_roughnessAmount, qt_fresnelPower);\n";
                    if (enableFresnelScaleBias) {
                        if (hasCustomFrag) {
                            fragmentShader << "    float qt_fresnelScale = qt_customFresnelScale;\n";
                            fragmentShader << "    float qt_fresnelBias = qt_customFresnelBias;\n";
                        }else {
                            fragmentShader << "    float qt_fresnelScale = qt_material_properties5.x;\n";
                            fragmentShader << "    float qt_fresnelBias = qt_material_properties5.y;\n";
                        }
                        fragmentShader << "    qt_principledMaterialFresnelValue = clamp(vec3(qt_fresnelBias) + "
                                       << "qt_fresnelScale * qt_principledMaterialFresnelValue, 0.0, 1.0);\n";
                    }
                    fragmentShader << "    qt_specularAmount *= qt_principledMaterialFresnelValue;\n";
                    if (materialAdapter->isPrincipled()) {
                        // Make sure that we scale the specularTint with repsect to metalness (no tint if qt_metalnessAmount == 1)
                        // We actually need to do this here because we won't know the final metalness value until this point.
                        fragmentShader << "    qt_specularTint = mix(vec3(1.0), qt_specularTint, 1.0 - qt_metalnessAmount);\n";
                    }
                } else {
                    fragmentShader << "    qt_specularAmount *= qt_principledMaterialFresnel(qt_world_normal, qt_view_vector, "
                                   << "qt_f0, qt_roughnessAmount, qt_fresnelPower);\n";
                }
            } else {
                Q_ASSERT(!hasCustomFrag);
                fragmentShader.addInclude("defaultMaterialFresnel.glsllib");
                fragmentShader << "    qt_diffuseColor.rgb *= (1.0 - qt_dielectricSpecular(qt_iOR)) * (1.0 - qt_metalnessAmount);\n";
                maybeAddMaterialFresnel(fragmentShader, keyProps, inKey, metalnessEnabled);
            }
        }

        if (!lights.isEmpty()) {
            generateMainLightCalculation(fragmentShader,
                                         vertexShader,
                                         inKey,
                                         inMaterial,
                                         lights,
                                         shaderLibraryManager,
                                         translucencyImage,
                                         hasCustomFrag,
                                         usesSharedVar,
                                         enableLightmap,
                                         enableShadowMaps,
                                         specularLightingEnabled,
                                         enableClearcoat,
                                         enableTransmission);
        }

        // The color in rgb is ready, including shadowing, just need to apply
        // the ambient occlusion factor. The alpha is the model opacity
        // multiplied by the alpha from the material color and/or the vertex colors.
        fragmentShader << "    global_diffuse_light = vec4(global_diffuse_light.rgb * qt_aoFactor, qt_objectOpacity * qt_diffuseColor.a);\n";

        if (hasReflectionProbe) {
            vertexShader.generateWorldNormal(inKey);
            fragmentShader.addInclude("sampleReflectionProbe.glsllib");

            fragmentShader << "    vec3 qt_reflectionDiffuse = vec3(0.0);\n";
            if (materialAdapter->isPrincipled() || materialAdapter->isSpecularGlossy()) {
                fragmentShader << "    qt_reflectionDiffuse = qt_diffuseColor.rgb * (1.0 - qt_specularAmount) * qt_sampleDiffuseReflection(qt_reflectionMap, qt_world_normal).rgb;\n";
            } else {
                fragmentShader << "    qt_reflectionDiffuse = qt_diffuseColor.rgb * qt_sampleDiffuseReflection(qt_reflectionMap, qt_world_normal).rgb;\n";
            }

            if (specularLightingEnabled) {
                fragmentShader << "    vec3 qt_reflectionSpecular = vec3(0.0);\n";
                if (materialAdapter->isPrincipled() || materialAdapter->isSpecularGlossy()) {
                    fragmentShader << "    qt_reflectionSpecular = "
                                   << "qt_specularTint * qt_sampleGlossyReflectionPrincipled(qt_reflectionMap, qt_world_normal, qt_view_vector, qt_specularAmount, qt_roughnessAmount).rgb;\n";
                } else {
                    fragmentShader << "    qt_reflectionSpecular = qt_specularAmount * "
                                   << "qt_specularTint * qt_sampleGlossyReflection(qt_reflectionMap, qt_world_normal, qt_view_vector, qt_roughnessAmount).rgb;\n";
                }
            }
            if (enableClearcoat) {
                fragmentShader << "   vec3 qt_iblClearcoat = qt_sampleGlossyReflectionPrincipled(qt_reflectionMap, qt_clearcoatNormal, qt_view_vector, qt_clearcoatF0, qt_clearcoatRoughness).rgb;\n";
            }

            fragmentShader << "    global_diffuse_light.rgb += qt_reflectionDiffuse;\n";
            if (specularLightingEnabled)
                fragmentShader << "    global_specular_light += qt_reflectionSpecular;\n";
            if (enableClearcoat)
                fragmentShader << "    qt_global_clearcoat += qt_iblClearcoat;\n";
        } else if (hasIblProbe) {
            vertexShader.generateWorldNormal(inKey);
            fragmentShader.addInclude("sampleProbe.glsllib");
            if (hasCustomIblProbe) {
                // DIFFUSE, SPECULAR, BASE_COLOR, AO_FACTOR, SPECULAR_AMOUNT, NORMAL, VIEW_VECTOR, IBL_ORIENTATION(, SHARED)
                fragmentShader << "    vec3 qt_iblDiffuse = vec3(0.0);\n";
                fragmentShader << "    vec3 qt_iblSpecular = vec3(0.0);\n";
                fragmentShader << "    qt_iblProbeProcessor(qt_iblDiffuse, qt_iblSpecular, qt_customBaseColor, qt_aoFactor, qt_specularFactor, qt_roughnessAmount, qt_world_normal, qt_view_vector";
                if (hasIblOrientation)
                    fragmentShader << ", qt_lightProbeOrientation";
                else
                    fragmentShader << ", mat3(1.0)";
                if (usesSharedVar)
                    fragmentShader << ", qt_customShared);\n";
                else
                    fragmentShader << ");\n";
            } else {
                if (materialAdapter->isPrincipled() || materialAdapter->isSpecularGlossy()) {
                    fragmentShader << "    vec3 qt_iblDiffuse = qt_diffuseColor.rgb * (1.0 - qt_specularAmount) * qt_sampleDiffuse(qt_world_normal).rgb;\n";
                } else {
                    fragmentShader << "    vec3 qt_iblDiffuse = qt_diffuseColor.rgb * qt_sampleDiffuse(qt_world_normal).rgb;\n";
                }
                if (specularLightingEnabled) {
                    if (materialAdapter->isPrincipled() || materialAdapter->isSpecularEnabled()) {
                        fragmentShader << "    vec3 qt_iblSpecular = "
                                       << "qt_specularTint * qt_sampleGlossyPrincipled(qt_world_normal, qt_view_vector, qt_specularAmount, qt_roughnessAmount).rgb;\n";
                    } else {
                        fragmentShader << "    vec3 qt_iblSpecular = qt_specularAmount * "
                                       << "qt_specularTint * qt_sampleGlossy(qt_world_normal, qt_view_vector, qt_roughnessAmount).rgb;\n";
                    }
                }
                if (enableClearcoat) {
                    fragmentShader << "   vec3 qt_iblClearcoat = qt_sampleGlossyPrincipled(qt_clearcoatNormal, qt_view_vector, qt_clearcoatF0, qt_clearcoatRoughness).rgb;\n";
                }
            }

            fragmentShader << "    global_diffuse_light.rgb += qt_iblDiffuse * qt_aoFactor;\n";
            if (specularLightingEnabled)
                fragmentShader << "    global_specular_light += qt_iblSpecular * qt_aoFactor;\n";
            if (enableClearcoat)
                fragmentShader << "    qt_global_clearcoat += qt_iblClearcoat * qt_aoFactor;\n";
        } else if (hasCustomIblProbe) {
            // Prevent breaking the fragment code while seeking uniforms
            fragmentShader.addUniform("qt_lightProbe", "samplerCube");
            fragmentShader.addUniform("qt_lightProbeProperties", "vec4");
        }

        // This can run even without a IBL probe
        if (enableTransmission) {
            fragmentShader << "    qt_global_transmission += qt_transmissionFactor * qt_getIBLVolumeRefraction(qt_world_normal, qt_view_vector, qt_roughnessAmount, "
                              "qt_diffuseColor.rgb, qt_specularAmount, qt_varWorldPos, qt_iOR, qt_thicknessFactor, qt_attenuationColor, qt_attenuationDistance);\n";
        }

        if (hasImage) {
            bool texColorDeclared = false;
            for (QSSGRenderableImage *image = firstImage; image; image = image->m_nextImage) {
                // map types other than these 2 are handled elsewhere
                if (image->m_mapType != QSSGRenderableImage::Type::Specular
                        && image->m_mapType != QSSGRenderableImage::Type::Emissive)
                {
                    continue;
                }

                if (!texColorDeclared) {
                    fragmentShader.append("    vec4 qt_texture_color;");
                    texColorDeclared = true;
                }

                const bool hasIdentityMap = identityImages.contains(image);
                if (hasIdentityMap)
                    generateImageUVSampler(vertexShader, fragmentShader, inKey, *image, imageFragCoords, image->m_imageNode.m_indexUV);
                else
                    generateImageUVCoordinates(vertexShader, fragmentShader, inKey, *image, enableParallaxMapping, image->m_imageNode.m_indexUV);

                const auto &names = imageStringTable[int(image->m_mapType)];
                fragmentShader << "    qt_texture_color = texture2D(" << names.imageSampler
                               << ", " << (hasIdentityMap ? imageFragCoords : names.imageFragCoords) << ");\n";

                switch (image->m_mapType) {
                case QSSGRenderableImage::Type::Specular:
                    fragmentShader.addInclude("tonemapping.glsllib");
                    fragmentShader.append("    global_specular_light += qt_sRGBToLinear(qt_texture_color.rgb) * qt_specularTint;");
                    fragmentShader.append("    global_diffuse_light.a *= qt_texture_color.a;");
                    break;
                case QSSGRenderableImage::Type::Emissive:
                    fragmentShader.addInclude("tonemapping.glsllib");
                    if (materialAdapter->isEmissiveSingleChannelEnabled()) {
                        const auto &channelProps = keyProps.m_textureChannels[QSSGShaderDefaultMaterialKeyProperties::EmissiveChannel];
                        fragmentShader << "    qt_global_emission *= qt_sRGBToLinear(vec3(qt_texture_color" <<
                                                        channelStr(channelProps, inKey) << "));\n";
                    } else {
                        fragmentShader.append("    qt_global_emission *= qt_sRGBToLinear(qt_texture_color.rgb);");
                    }
                    break;
                default:
                    Q_ASSERT(false);
                    break;
                }
            }
        }

        if (enableTransmission)
            fragmentShader << "    global_diffuse_light.rgb = mix(global_diffuse_light.rgb, qt_global_transmission, qt_transmissionFactor);\n";

        if (materialAdapter->isPrincipled()) {
            fragmentShader << "    global_diffuse_light.rgb *= 1.0 - qt_metalnessAmount;\n";
        }

        if (enableFog) {
            fragmentShader.addInclude("fog.glsllib");
            fragmentShader << "    calculateFog(qt_global_emission, global_specular_light, global_diffuse_light.rgb);\n";
        }

        fragmentShader << "    vec4 qt_color_sum = vec4(global_diffuse_light.rgb + global_specular_light + qt_global_emission, global_diffuse_light.a);\n";

        if (enableClearcoat) {
            fragmentShader.addInclude("bsdf.glsllib");
            if (hasCustomFrag)
                fragmentShader << "    float qt_clearcoatFresnelPower = qt_customClearcoatFresnelPower;\n";
            else
                fragmentShader << "    float qt_clearcoatFresnelPower = qt_material_clearcoat_fresnel_power;\n";
            fragmentShader << "    vec3 qt_clearcoatFresnel = qt_schlick3(qt_clearcoatF0, qt_clearcoatF90, clamp(dot(qt_clearcoatNormal, qt_view_vector), 0.0, 1.0), qt_clearcoatFresnelPower);\n";
            if (enableClearcoatFresnelScaleBias) {
                if (hasCustomFrag) {
                    fragmentShader << "    float qt_clearcoatFresnelScale = qt_customClearcoatFresnelScale;\n";
                    fragmentShader << "    float qt_clearcoatFresnelBias = qt_customClearcoatFresnelBias;\n";
                }else {
                    fragmentShader << "    float qt_clearcoatFresnelScale = qt_material_properties5.z;\n";
                    fragmentShader << "    float qt_clearcoatFresnelBias = qt_material_properties5.w;\n";
                }
                fragmentShader << "    qt_clearcoatFresnel = clamp(vec3(qt_clearcoatFresnelBias) + qt_clearcoatFresnelScale * qt_clearcoatFresnel, 0.0, 1.0);\n";
            }
            fragmentShader << "    qt_global_clearcoat = qt_global_clearcoat * qt_clearcoatAmount;\n";
            fragmentShader << "    qt_color_sum.rgb = qt_color_sum.rgb * (1.0 - qt_clearcoatAmount * qt_clearcoatFresnel) + qt_global_clearcoat;\n";
        }

        if (hasCustomFrag && hasCustomFunction(QByteArrayLiteral("qt_customPostProcessor"))) {
            // COLOR_SUM, DIFFUSE, SPECULAR, EMISSIVE, UV0, UV1(, SHARED)
            fragmentShader << "    qt_customPostProcessor(qt_color_sum, global_diffuse_light, global_specular_light, qt_global_emission, qt_texCoord0, qt_texCoord1";
            if (usesSharedVar)
                fragmentShader << ", qt_customShared);\n";
            else
                fragmentShader << ");\n";
        }

        Q_ASSERT(!isDepthPass && !isOrthoShadowPass && !isPerspectiveShadowPass);

        if (oitMethod == QSSGRenderLayer::OITMethod::WeightedBlended) {
            fragmentShader.addInclude("orderindependenttransparency.glsllib");
            fragmentShader.addInclude("tonemapping.glsllib");
            fragmentShader.addUniform("qt_cameraPosition", "vec3");
            fragmentShader.addUniform("qt_cameraProperties", "vec2");
            fragmentShader.append("    float z = abs(gl_FragCoord.z);");
            fragmentShader.append("    qt_color_sum.rgb = qt_tonemap(qt_color_sum.rgb) * qt_color_sum.a;");
            fragmentShader.append("    fragOutput = qt_color_sum * qt_transparencyWeight(z, qt_color_sum.a, qt_cameraProperties.y);");
            fragmentShader.append("    revealageOutput = vec4(qt_color_sum.a);");
        } else {
            fragmentShader.addInclude("tonemapping.glsllib");
            fragmentShader.append("    fragOutput = vec4(qt_tonemap(qt_color_sum));");
        }
        // Debug Overrides for viewing various parts of the shading process
        if (Q_UNLIKELY(debugMode != QSSGRenderLayer::MaterialDebugMode::None)) {
            fragmentShader.append("    vec3 debugOutput = vec3(0.0);\n");
            switch (debugMode) {
            case QSSGRenderLayer::MaterialDebugMode::BaseColor:
                fragmentShader.append("    debugOutput += qt_tonemap(qt_diffuseColor.rgb);\n");
                break;
            case QSSGRenderLayer::MaterialDebugMode::Roughness:
                fragmentShader.append("    debugOutput += vec3(qt_roughnessAmount);\n");
                break;
            case QSSGRenderLayer::MaterialDebugMode::Metalness:
                fragmentShader.append("    debugOutput += vec3(qt_metalnessAmount);\n");
                break;
            case QSSGRenderLayer::MaterialDebugMode::Diffuse:
                fragmentShader.append("    debugOutput += qt_tonemap(global_diffuse_light.rgb);\n");
                break;
            case QSSGRenderLayer::MaterialDebugMode::Specular:
                fragmentShader.append("    debugOutput += qt_tonemap(global_specular_light);\n");
                break;
            case QSSGRenderLayer::MaterialDebugMode::ShadowOcclusion:
                fragmentShader.append("    debugOutput += vec3(qt_shadow_map_occl);\n");
                break;
            case QSSGRenderLayer::MaterialDebugMode::Emission:
                fragmentShader.append("    debugOutput += qt_tonemap(qt_global_emission);\n");
                break;
            case QSSGRenderLayer::MaterialDebugMode::AmbientOcclusion:
                fragmentShader.append("    debugOutput += vec3(qt_aoFactor);\n");
                break;
            case QSSGRenderLayer::MaterialDebugMode::Normal:
                fragmentShader.append("    debugOutput += qt_world_normal * 0.5 + 0.5;\n");
                break;
            case QSSGRenderLayer::MaterialDebugMode::Tangent:
                fragmentShader.append("    debugOutput += qt_tangent * 0.5 + 0.5;\n");
                break;
            case QSSGRenderLayer::MaterialDebugMode::Binormal:
                fragmentShader.append("    debugOutput += qt_binormal * 0.5 + 0.5;\n");
                break;
            case QSSGRenderLayer::MaterialDebugMode::F0:
                if (materialAdapter->isPrincipled() || materialAdapter->isSpecularGlossy())
                    fragmentShader.append("    debugOutput += qt_f0;");
                break;
            case QSSGRenderLayer::MaterialDebugMode::None:
                Q_UNREACHABLE();
                break;
            }
            fragmentShader.append("    fragOutput = vec4(debugOutput, 1.0);\n");
        }
    } else {
        if ((isOrthoShadowPass || isPerspectiveShadowPass || isDepthPass) && isOpaqueDepthPrePass) {
            fragmentShader << "    if ((qt_diffuseColor.a * qt_objectOpacity) < 1.0)\n";
            fragmentShader << "        discard;\n";
        }

        if (isOrthoShadowPass) {
            Q_ASSERT(viewCount == 1);
            fragmentShader.addUniform("qt_shadowDepthAdjust", "vec2");
            fragmentShader << "    // directional shadow pass\n"
                           << "    float qt_shadowDepth = (qt_varDepth + qt_shadowDepthAdjust.x) * qt_shadowDepthAdjust.y;\n"
                           << "    fragOutput = vec4(qt_shadowDepth);\n";
        } else if (isPerspectiveShadowPass) {
            Q_ASSERT(viewCount == 1);
            fragmentShader.addUniform("qt_cameraPosition", "vec3");
            fragmentShader.addUniform("qt_cameraProperties", "vec2");
            fragmentShader << "    // omnidirectional shadow pass\n"
                           << "    vec3 qt_shadowCamPos = vec3(qt_cameraPosition.x, qt_cameraPosition.y, qt_cameraPosition.z);\n"
                           << "    float qt_shadowDist = length(qt_varShadowWorldPos - qt_shadowCamPos);\n"
                           << "    qt_shadowDist = (qt_shadowDist - qt_cameraProperties.x) / (qt_cameraProperties.y - qt_cameraProperties.x);\n"
                           << "    fragOutput = vec4(qt_shadowDist, qt_shadowDist, qt_shadowDist, 1.0);\n";
        } else {
            if (oitMethod == QSSGRenderLayer::OITMethod::WeightedBlended) {
                fragmentShader.addInclude("orderindependenttransparency.glsllib");
                fragmentShader.addUniform("qt_cameraPosition", "vec3");
                fragmentShader.addUniform("qt_cameraProperties", "vec2");
                fragmentShader.append("    float z = abs(gl_FragCoord.z);");
                fragmentShader.append("    vec4 color = vec4(qt_diffuseColor.rgb, qt_diffuseColor.a * qt_objectOpacity);");
                fragmentShader.append("    fragOutput = qt_transparencyWeight(z, color.a, qt_cameraProperties.y) * color;");
                fragmentShader.append("    revealageOutput = vec4(color.a);");
            } else {
                fragmentShader.addInclude("tonemapping.glsllib");
                fragmentShader.append("    fragOutput = vec4(qt_tonemap(qt_diffuseColor.rgb), qt_diffuseColor.a * qt_objectOpacity);");
            }
        }
    }
}

QSSGRhiShaderPipelinePtr QSSGMaterialShaderGenerator::generateMaterialRhiShader(const QByteArray &inShaderKeyPrefix,
                                                                                QSSGMaterialVertexPipeline &vertexPipeline,
                                                                                const QSSGShaderDefaultMaterialKey &key,
                                                                                const QSSGShaderDefaultMaterialKeyProperties &inProperties,
                                                                                const QSSGShaderFeatures &inFeatureSet,
                                                                                const QSSGRenderGraphObject &inMaterial,
                                                                                const QSSGShaderLightListView &inLights,
                                                                                QSSGRenderableImage *inFirstImage,
                                                                                QSSGShaderLibraryManager &shaderLibraryManager,
                                                                                QSSGShaderCache &theCache)
{
    const int viewCount = inFeatureSet.isSet(QSSGShaderFeatures::Feature::DisableMultiView)
        ? 1 : inProperties.m_viewCount.getValue(key);

    bool perTargetCompilation = false;
    // Cull mode None implies doing the gl_FrontFacing-based double sided logic
    // in the shader. This may want to ifdef the generated shader code based the
    // target shading language. This is only possible if the QShaderBaker is
    // told to compile to SPIR-V (and then transpile) separately for each target
    // (GLSL, HLSL, etc.), instead of just compiling to SPIR-V once (and
    // transpiling the same bytecode to each target language). This takes more
    // time, so we only do it for multiview since that's also how the logic is
    // going to be written in the generated shader code.
    if (viewCount >= 2) {
        const bool isDoubleSided = inProperties.m_isDoubleSided.getValue(key);
        if (isDoubleSided)
            perTargetCompilation = true;
    }

    QByteArray materialInfoString; // also serves as the key for the cache in compileGeneratedRhiShader
    // inShaderKeyPrefix can be a static string for default materials, but must
    // be unique for different sets of shaders in custom materials.
    materialInfoString = inShaderKeyPrefix;
    key.toString(materialInfoString, inProperties);

    // the call order is: beginVertex, beginFragment, endVertex, endFragment
    vertexPipeline.beginVertexGeneration(key, inFeatureSet, shaderLibraryManager);
    generateFragmentShader(vertexPipeline.fragment(), vertexPipeline, key, inProperties, inFeatureSet, inMaterial, inLights, inFirstImage, shaderLibraryManager);
    vertexPipeline.endVertexGeneration();
    vertexPipeline.endFragmentGeneration();

    return vertexPipeline.programGenerator()->compileGeneratedRhiShader(materialInfoString,
                                                                        inFeatureSet,
                                                                        shaderLibraryManager,
                                                                        theCache,
                                                                        {},
                                                                        viewCount,
                                                                        perTargetCompilation);
}

void QSSGMaterialShaderGenerator::setRhiMaterialProperties(const QSSGRenderContextInterface &renderContext,
                                                           QSSGRhiShaderPipeline &shaders,
                                                           char *ubufData,
                                                           QSSGRhiGraphicsPipelineState *inPipelineState,
                                                           const QSSGRenderGraphObject &inMaterial,
                                                           const QSSGShaderDefaultMaterialKey &inKey,
                                                           const QSSGShaderDefaultMaterialKeyProperties &inProperties,
                                                           const QSSGRenderCameraList &inCameras,
                                                           const QSSGRenderMvpArray &inModelViewProjections,
                                                           const QMatrix3x3 &inNormalMatrix,
                                                           const QMatrix4x4 &inGlobalTransform,
                                                           const QMatrix4x4 &clipSpaceCorrMatrix,
                                                           const QMatrix4x4 &localInstanceTransform,
                                                           const QMatrix4x4 &globalInstanceTransform,
                                                           const QSSGDataView<float> &inMorphWeights,
                                                           QSSGRenderableImage *inFirstImage,
                                                           float inOpacity,
                                                           const QSSGLayerRenderData &inRenderProperties,
                                                           const QSSGShaderLightListView &inLights,
                                                           const QSSGShaderReflectionProbe &reflectionProbe,
                                                           bool receivesShadows,
                                                           bool receivesReflections,
                                                           const QVector2D *shadowDepthAdjust,
                                                           QRhiTexture *lightmapTexture)
{
    QSSGShaderMaterialAdapter *materialAdapter = getMaterialAdapter(inMaterial);
    QSSGRhiShaderPipeline::CommonUniformIndices &cui = shaders.commonUniformIndices;

    materialAdapter->setCustomPropertyUniforms(ubufData, shaders, renderContext);

    const QVector2D camProperties(inCameras[0]->clipNear, inCameras[0]->clipFar);
    shaders.setUniform(ubufData, "qt_cameraProperties", &camProperties, 2 * sizeof(float), &cui.cameraPropertiesIdx);

    const int viewCount = inRenderProperties.layer.viewCount;
    if (viewCount < 2) {
        const QVector3D camGlobalPos = inCameras[0]->getGlobalPos();
        shaders.setUniform(ubufData, "qt_cameraPosition", &camGlobalPos, 3 * sizeof(float), &cui.cameraPositionIdx);
        const QVector3D camDirection = QSSG_GUARD(inRenderProperties.renderedCameraData.has_value())
                                                  ? inRenderProperties.renderedCameraData.value()[0].direction
                                                  : QVector3D{ 0.0f, 0.0f, -1.0f };
        shaders.setUniform(ubufData, "qt_cameraDirection", &camDirection, 3 * sizeof(float), &cui.cameraDirectionIdx);
    } else {
        QVarLengthArray<QVector3D, 2> camGlobalPos(viewCount);
        QVarLengthArray<QVector3D> camDirection(viewCount);
        for (int viewIndex = 0; viewIndex < viewCount; ++viewIndex) {
            camGlobalPos[viewIndex] = inCameras[viewIndex]->getGlobalPos();
            camDirection[viewIndex] = QSSG_GUARD(inRenderProperties.renderedCameraData.has_value())
                                                ? inRenderProperties.renderedCameraData.value()[viewIndex].direction
                                                : QVector3D{ 0.0f, 0.0f, -1.0f };
        }
        shaders.setUniformArray(ubufData, "qt_cameraPosition", camGlobalPos.constData(), viewCount, QSSGRenderShaderValue::Vec3, &cui.cameraPositionIdx);
        shaders.setUniformArray(ubufData, "qt_cameraDirection", camDirection.constData(), viewCount, QSSGRenderShaderValue::Vec3, &cui.cameraDirectionIdx);
    }

    const auto globalRenderData = QSSGLayerRenderData::globalRenderProperties(renderContext);

    // Only calculate and update Matrix uniforms if they are needed
    bool usesProjectionMatrix = false;
    bool usesInvProjectionMatrix = false;
    bool usesViewProjectionMatrix = false;
    bool usesModelViewProjectionMatrix = false;
    bool usesNormalMatrix = false;
    bool usesParentMatrix = false;

    if (inMaterial.type == QSSGRenderGraphObject::Type::CustomMaterial) {
        const auto *customMaterial = static_cast<const QSSGRenderCustomMaterial *>(&inMaterial);
        usesProjectionMatrix = customMaterial->m_renderFlags.testFlag(QSSGRenderCustomMaterial::RenderFlag::ProjectionMatrix);
        usesInvProjectionMatrix = customMaterial->m_renderFlags.testFlag(QSSGRenderCustomMaterial::RenderFlag::InverseProjectionMatrix);
        // ### these should use flags like the above two
        usesViewProjectionMatrix = true;
    }

    const bool usesInstancing = inProperties.m_usesInstancing.getValue(inKey);
    if (usesInstancing) {
        // Instanced calls have to calculate MVP and normalMatrix in the vertex shader
        usesViewProjectionMatrix = true;
        usesParentMatrix = true;
    } else {
        usesModelViewProjectionMatrix = true;
        usesNormalMatrix = true;
    }

    if (materialAdapter->isTransmissionEnabled())
        usesViewProjectionMatrix = true;

    // Update matrix uniforms
    if (usesProjectionMatrix || usesInvProjectionMatrix) {
        if (viewCount < 2) {
            const QMatrix4x4 projection = clipSpaceCorrMatrix * inCameras[0]->projection;
            if (usesProjectionMatrix)
                shaders.setUniform(ubufData, "qt_projectionMatrix", projection.constData(), 16 * sizeof(float), &cui.projectionMatrixIdx);
            if (usesInvProjectionMatrix)
                shaders.setUniform(ubufData, "qt_inverseProjectionMatrix", projection.inverted().constData(), 16 * sizeof (float), &cui.inverseProjectionMatrixIdx);
        } else {
            QVarLengthArray<QMatrix4x4, 2> projections(viewCount);
            QVarLengthArray<QMatrix4x4, 2> invertedProjections(viewCount);
            for (int viewIndex = 0; viewIndex < viewCount; ++viewIndex) {
                projections[viewIndex] = clipSpaceCorrMatrix * inCameras[viewIndex]->projection;
                if (usesInvProjectionMatrix)
                    invertedProjections[viewIndex] = projections[viewIndex].inverted();
            }
            if (usesProjectionMatrix)
                shaders.setUniformArray(ubufData, "qt_projectionMatrix", projections.constData(), viewCount, QSSGRenderShaderValue::Matrix4x4, &cui.projectionMatrixIdx);
            if (usesInvProjectionMatrix)
                shaders.setUniformArray(ubufData, "qt_inverseProjectionMatrix", invertedProjections.constData(), viewCount, QSSGRenderShaderValue::Matrix4x4, &cui.inverseProjectionMatrixIdx);
        }
    }

    if (viewCount < 2) {
        const QMatrix4x4 viewMatrix = inCameras[0]->globalTransform.inverted();
        shaders.setUniform(ubufData, "qt_viewMatrix", viewMatrix.constData(), 16 * sizeof(float), &cui.viewMatrixIdx);
    } else {
        QVarLengthArray<QMatrix4x4, 2> viewMatrices(viewCount);
        for (int viewIndex = 0; viewIndex < viewCount; ++viewIndex)
            viewMatrices[viewIndex] = inCameras[viewIndex]->globalTransform.inverted();
        shaders.setUniformArray(ubufData, "qt_viewMatrix", viewMatrices.constData(), viewCount, QSSGRenderShaderValue::Matrix4x4, &cui.viewMatrixIdx);
    }

    if (usesViewProjectionMatrix) {
        if (viewCount < 2) {
            QMatrix4x4 viewProj(Qt::Uninitialized);
            inCameras[0]->calculateViewProjectionMatrix(viewProj);
            viewProj = clipSpaceCorrMatrix * viewProj;
            shaders.setUniform(ubufData, "qt_viewProjectionMatrix", viewProj.constData(), 16 * sizeof(float), &cui.viewProjectionMatrixIdx);
        } else {
            QVarLengthArray<QMatrix4x4, 2> viewProjections(viewCount);
            for (int viewIndex = 0; viewIndex < viewCount; ++viewIndex) {
                inCameras[viewIndex]->calculateViewProjectionMatrix(viewProjections[viewIndex]);
                viewProjections[viewIndex] = clipSpaceCorrMatrix * viewProjections[viewIndex];
            }
            shaders.setUniformArray(ubufData, "qt_viewProjectionMatrix", viewProjections.constData(), viewCount, QSSGRenderShaderValue::Matrix4x4, &cui.viewProjectionMatrixIdx);
        }
    }

    // qt_modelMatrix is always available, but differnt when using instancing
    if (usesInstancing)
        shaders.setUniform(ubufData, "qt_modelMatrix", localInstanceTransform.constData(), 16 * sizeof(float), &cui.modelMatrixIdx);
    else
        shaders.setUniform(ubufData, "qt_modelMatrix", inGlobalTransform.constData(), 16 * sizeof(float), &cui.modelMatrixIdx);

    if (usesModelViewProjectionMatrix) {
        if (viewCount < 2) {
            QMatrix4x4 mvp { clipSpaceCorrMatrix };
            mvp *= inModelViewProjections[0];
            shaders.setUniform(ubufData, "qt_modelViewProjection", mvp.constData(), 16 * sizeof(float), &cui.modelViewProjectionIdx);
        } else {
            QVarLengthArray<QMatrix4x4, 2> mvps(viewCount);
            for (int viewIndex = 0; viewIndex < viewCount; ++viewIndex)
                mvps[viewIndex] = clipSpaceCorrMatrix * inModelViewProjections[viewIndex];
            shaders.setUniformArray(ubufData, "qt_modelViewProjection", mvps.constData(), viewCount, QSSGRenderShaderValue::Matrix4x4, &cui.modelViewProjectionIdx);
        }
    }
    if (usesNormalMatrix)
        shaders.setUniform(ubufData, "qt_normalMatrix", inNormalMatrix.constData(), 12 * sizeof(float), &cui.normalMatrixIdx,
                            QSSGRhiShaderPipeline::UniformFlag::Mat3); // real size will be 12 floats, setUniform repacks as needed
    if (usesParentMatrix)
        shaders.setUniform(ubufData, "qt_parentMatrix", globalInstanceTransform.constData(), 16 * sizeof(float));

    // Morphing
    const qsizetype morphSize = inProperties.m_targetCount.getValue(inKey);
    if (morphSize > 0) {
        if (inMorphWeights.mSize >= morphSize) {
            shaders.setUniformArray(ubufData, "qt_morphWeights", inMorphWeights.mData, morphSize,
                                     QSSGRenderShaderValue::Float, &cui.morphWeightsIdx);
        } else {
            const QList<float> zeroWeights(morphSize - inMorphWeights.mSize, 0.0f);
            QList<float> newWeights(inMorphWeights.mData, inMorphWeights.mData + inMorphWeights.mSize);
            newWeights.append(zeroWeights);
            shaders.setUniformArray(ubufData, "qt_morphWeights", newWeights.constData(), morphSize,
                                     QSSGRenderShaderValue::Float, &cui.morphWeightsIdx);
        }
    }

    QVector3D theLightAmbientTotal;
    shaders.resetShadowMaps();
    float lightColor[QSSG_MAX_NUM_LIGHTS][3];
    QSSGShaderLightsUniformData &lightsUniformData(shaders.lightsUniformData());
    lightsUniformData.count = 0;
    QSSGShaderShadowsUniformData &shadowsUniformData(shaders.shadowsUniformData());
    shadowsUniformData.count = 0;

    for (quint32 lightIdx = 0, lightEnd = inLights.size();
         lightIdx < lightEnd && lightIdx < QSSG_MAX_NUM_LIGHTS; ++lightIdx)
    {
        QSSGRenderLight *theLight(inLights[lightIdx].light);
        const bool lightShadows = inLights[lightIdx].shadows;
        const float brightness = theLight->m_brightness;
        lightColor[lightIdx][0] = theLight->m_diffuseColor.x() * brightness;
        lightColor[lightIdx][1] = theLight->m_diffuseColor.y() * brightness;
        lightColor[lightIdx][2] = theLight->m_diffuseColor.z() * brightness;
        lightsUniformData.count += 1;
        QSSGShaderLightData &lightData(lightsUniformData.lightData[lightIdx]);
        const QVector3D &lightSpecular(theLight->m_specularColor);
        lightData.specular[0] = lightSpecular.x() * brightness;
        lightData.specular[1] = lightSpecular.y() * brightness;
        lightData.specular[2] = lightSpecular.z() * brightness;
        lightData.specular[3] = 1.0f;
        const QVector3D &lightDirection(inLights[lightIdx].direction);
        lightData.direction[0] = lightDirection.x();
        lightData.direction[1] = lightDirection.y();
        lightData.direction[2] = lightDirection.z();
        lightData.direction[3] = 1.0f;

        // When it comes to receivesShadows, it is a bit tricky: to stay
        // compatible with the old, direct OpenGL rendering path (and the
        // generated shader code), we will need to ensure the texture
        // (shadowmap0, shadowmap1, ...) and sampler bindings are present.
        // So receivesShadows must not be included in the following
        // condition. Instead, it is the other shadow-related uniforms that
        // get an all-zero value, which then ensures no shadow contribution
        // for the object in question.

        if (lightShadows && shadowsUniformData.count < QSSG_MAX_NUM_SHADOW_MAPS) {
            QSSGRhiShadowMapProperties &theShadowMapProperties(shaders.addShadowMap());
            QSSGShadowMapEntry *pEntry = inRenderProperties.getShadowMapManager()->shadowMapEntry(lightIdx);
            Q_ASSERT(pEntry);

            const bool is32bit = theLight->m_use32BitShadowmap;
            const auto& names = setupShadowMapVariableNames(lightIdx, theLight->m_shadowMapRes, is32bit);

            QSSGShaderShadowData &shadowData(shadowsUniformData.shadowData[shadowsUniformData.count]);

            if (theLight->type == QSSGRenderLight::Type::DirectionalLight ||
                theLight->type == QSSGRenderLight::Type::SpotLight) {
                theShadowMapProperties.shadowMapTexture = pEntry->m_rhiDepthTextureArray;
                theShadowMapProperties.shadowMapTextureUniformName = names.shadowMapTexture;
            } else if (theLight->type == QSSGRenderLight::Type::PointLight) {
                theShadowMapProperties.shadowMapTexture = pEntry->m_rhiDepthCube;
                theShadowMapProperties.shadowMapTextureUniformName = names.shadowCube;
            } else {
                Q_UNREACHABLE();
            }

            if (receivesShadows) {
                if (theLight->type == QSSGRenderLight::Type::DirectionalLight ||
                    theLight->type == QSSGRenderLight::Type::SpotLight) {
                    // add fixed scale bias matrix
                    static const QMatrix4x4 bias = {
                        0.5, 0.0, 0.0, 0.5,
                        0.0, 0.5, 0.0, 0.5,
                        0.0, 0.0, 0.5, 0.5,
                        0.0, 0.0, 0.0, 1.0 };
                    for (int i = 0; i < 4; i++) {
                        const QMatrix4x4 m = bias * pEntry->m_lightViewProjection[i];
                        memcpy(shadowData.matrices[i], m.constData(), 16 * sizeof(float));
                    }
                } else {
                    Q_ASSERT(theLight->type == QSSGRenderLight::Type::PointLight);
                    memcpy(&shadowData.matrices[0], pEntry->m_lightView.constData(), 16 * sizeof(float));
                }

                shadowData.bias = theLight->m_shadowBias;
                // Optimization: if directional light with no cascades then set shadowFactor to 0 to disable shadow map sampling
                const bool noCascades = !(pEntry->m_csmActive[0] || pEntry->m_csmActive[1] || pEntry->m_csmActive[2] || pEntry->m_csmActive[3]);
                shadowData.factor = theLight->type == QSSGRenderLight::Type::DirectionalLight && noCascades ? 0.0f : theLight->m_shadowFactor;
                shadowData.clipNear = 1.0f;
                shadowData.shadowMapFar = pEntry->m_shadowMapFar;
                shadowData.isYUp = globalRenderData.isYUpInFramebuffer ? 0.0f : 1.0f;
                shadowData.layerIndex = pEntry->m_depthArrayIndex;
                shadowData.csmNumSplits = pEntry->m_csmNumSplits;
                memcpy(shadowData.csmSplits, pEntry->m_csmSplits, 4 * sizeof(float));
                memcpy(shadowData.csmActive, pEntry->m_csmActive, 4 * sizeof(float));
                shadowData.csmBlendRatio = theLight->m_csmBlendRatio;
                for (int i = 0; i < 4; i++) {
                    QMatrix4x4 inv = pEntry->m_lightViewProjection[i].inverted();
                    const float x = 0.5f / (inv * QVector4D(1, 0, 0, 0)).length();
                    const float y = 0.5f / (inv * QVector4D(0, 1, 0, 0)).length();
                    const float z = 0.5f / (inv * QVector4D(0, 0, 1, 0)).length();
                    QVector4D dimensionsInverted = QVector4D(x, y, z, 0);
                    memcpy(shadowData.dimensionsInverted[i], &dimensionsInverted, 4 * sizeof(float));
                }
                shadowData.pcfFactor = theLight->m_pcfFactor;
            } else {
                memset(&shadowData, '\0', sizeof(shadowData));
            }

            ++shadowsUniformData.count;
        }

        if (theLight->type == QSSGRenderLight::Type::PointLight
                || theLight->type == QSSGRenderLight::Type::SpotLight) {
            const QVector3D globalPos = theLight->getGlobalPos();
            lightData.position[0] = globalPos.x();
            lightData.position[1] = globalPos.y();
            lightData.position[2] = globalPos.z();
            lightData.position[3] = 1.0f;
            lightData.constantAttenuation = QSSGUtils::aux::translateConstantAttenuation(theLight->m_constantFade);
            lightData.linearAttenuation = QSSGUtils::aux::translateLinearAttenuation(theLight->m_linearFade);
            lightData.quadraticAttenuation = QSSGUtils::aux::translateQuadraticAttenuation(theLight->m_quadraticFade);
            lightData.coneAngle = 180.0f;
            if (theLight->type == QSSGRenderLight::Type::SpotLight) {
                const float coneAngle = theLight->m_coneAngle;
                const float innerConeAngle = (theLight->m_innerConeAngle > coneAngle) ?
                                                coneAngle : theLight->m_innerConeAngle;
                lightData.coneAngle = qCos(qDegreesToRadians(coneAngle));
                lightData.innerConeAngle = qCos(qDegreesToRadians(innerConeAngle));
            }
        }

        theLightAmbientTotal += theLight->m_ambientColor;
    }

    const QSSGRhiRenderableTexture *depthTexture = inRenderProperties.getRenderResult(QSSGFrameData::RenderResult::DepthTexture);
    const QSSGRhiRenderableTexture *ssaoTexture = inRenderProperties.getRenderResult(QSSGFrameData::RenderResult::AoTexture);
    const QSSGRhiRenderableTexture *screenTexture = inRenderProperties.getRenderResult(QSSGFrameData::RenderResult::ScreenTexture);

    shaders.setDepthTexture(depthTexture->texture);
    shaders.setSsaoTexture(ssaoTexture->texture);
    shaders.setScreenTexture(screenTexture->texture);
    shaders.setLightmapTexture(lightmapTexture);

    const QSSGRenderLayer &layer = QSSGLayerRenderData::getCurrent(*renderContext.renderer())->layer;
    QSSGRenderImage *theLightProbe = layer.lightProbe;
    const auto &lightProbeData = layer.lightProbeSettings;

    // If the material has its own IBL Override, we should use that image instead.
    QSSGRenderImage *materialIblProbe = materialAdapter->iblProbe();
    if (materialIblProbe)
        theLightProbe = materialIblProbe;
    QSSGRenderImageTexture lightProbeTexture;
    if (theLightProbe)
        lightProbeTexture = renderContext.bufferManager()->loadRenderImage(theLightProbe, QSSGBufferManager::MipModeBsdf);
    if (theLightProbe && lightProbeTexture.m_texture) {
        QSSGRenderTextureCoordOp theHorzLightProbeTilingMode = theLightProbe->m_horizontalTilingMode;
        QSSGRenderTextureCoordOp theVertLightProbeTilingMode = theLightProbe->m_verticalTilingMode;
        const int maxMipLevel = lightProbeTexture.m_mipmapCount - 1;

        if (!materialIblProbe && !lightProbeData.probeOrientation.isIdentity()) {
            shaders.setUniform(ubufData, "qt_lightProbeOrientation",
                                lightProbeData.probeOrientation.constData(),
                                12 * sizeof(float), &cui.lightProbeOrientationIdx,
                                QSSGRhiShaderPipeline::UniformFlag::Mat3);
        }

        const float props[4] = { 0.0f, float(maxMipLevel), lightProbeData.probeHorizon, lightProbeData.probeExposure };
        shaders.setUniform(ubufData, "qt_lightProbeProperties", props, 4 * sizeof(float), &cui.lightProbePropertiesIdx);

        shaders.setLightProbeTexture(lightProbeTexture.m_texture, theHorzLightProbeTilingMode, theVertLightProbeTilingMode);
    } else {
        // no lightprobe
        const float emptyProps[4] = { 0.0f, 0.0f, -1.0f, 0.0f };
        shaders.setUniform(ubufData, "qt_lightProbeProperties", emptyProps, 4 * sizeof(float), &cui.lightProbePropertiesIdx);

        shaders.setLightProbeTexture(nullptr);
    }

    if (receivesReflections && reflectionProbe.enabled) {
        shaders.setUniform(ubufData, "qt_reflectionProbeCubeMapCenter", &reflectionProbe.probeCubeMapCenter, 3 * sizeof(float), &cui.reflectionProbeCubeMapCenter);
        shaders.setUniform(ubufData, "qt_reflectionProbeBoxMin", &reflectionProbe.probeBoxMin, 3 * sizeof(float), &cui.reflectionProbeBoxMin);
        shaders.setUniform(ubufData, "qt_reflectionProbeBoxMax", &reflectionProbe.probeBoxMax, 3 * sizeof(float), &cui.reflectionProbeBoxMax);
        shaders.setUniform(ubufData, "qt_reflectionProbeCorrection", &reflectionProbe.parallaxCorrection, sizeof(int), &cui.reflectionProbeCorrection);
    }

    const QVector3D emissiveColor = materialAdapter->emissiveColor();
    shaders.setUniform(ubufData, "qt_material_emissive_color", &emissiveColor, 3 * sizeof(float), &cui.material_emissiveColorIdx);

    const auto qMix = [](float x, float y, float a) {
        return (x * (1.0f - a) + (y * a));
    };

    const auto qMix3 = [&qMix](const QVector3D &x, const QVector3D &y, float a) {
        return QVector3D{qMix(x.x(), y.x(), a), qMix(x.y(), y.y(), a), qMix(x.z(), y.z(), a)};
    };

    const QVector4D color = materialAdapter->color();
    const QVector3D materialSpecularTint = materialAdapter->specularTint();
    const QVector3D specularTint = materialAdapter->isPrincipled() ? qMix3(QVector3D(1.0f, 1.0f, 1.0f), color.toVector3D(), materialSpecularTint.x())
                                                                   : materialSpecularTint;
    shaders.setUniform(ubufData, "qt_material_base_color", &color, 4 * sizeof(float), &cui.material_baseColorIdx);

    const float ior = materialAdapter->ior();
    QVector4D specularColor(specularTint, ior);
    shaders.setUniform(ubufData, "qt_material_specular", &specularColor, 4 * sizeof(float), &cui.material_specularIdx);

     // metalnessAmount cannot be multiplied in here yet due to custom materials
    const bool hasLighting = materialAdapter->hasLighting();
    shaders.setLightsEnabled(hasLighting);
    if (hasLighting) {
        for (int lightIdx = 0; lightIdx < lightsUniformData.count; ++lightIdx) {
            QSSGShaderLightData &lightData(lightsUniformData.lightData[lightIdx]);
            lightData.diffuse[0] = lightColor[lightIdx][0];
            lightData.diffuse[1] = lightColor[lightIdx][1];
            lightData.diffuse[2] = lightColor[lightIdx][2];
            lightData.diffuse[3] = 1.0f;
        }
        memcpy(ubufData + shaders.ub0LightDataOffset(), &lightsUniformData, shaders.ub0LightDataSize());

        if (shadowsUniformData.count)
            memcpy(ubufData + shaders.ub0ShadowDataOffset(), &shadowsUniformData, shaders.ub0ShadowDataSize());
    }

    shaders.setUniform(ubufData, "qt_light_ambient_total", &theLightAmbientTotal, 3 * sizeof(float), &cui.light_ambient_totalIdx);

    const float materialProperties[4] = {
        materialAdapter->specularAmount(),
        materialAdapter->specularRoughness(),
        materialAdapter->metalnessAmount(),
        inOpacity
    };
    shaders.setUniform(ubufData, "qt_material_properties", materialProperties, 4 * sizeof(float), &cui.material_propertiesIdx);

    const float materialProperties2[4] = {
        materialAdapter->fresnelPower(),
        materialAdapter->bumpAmount(),
        materialAdapter->translucentFallOff(),
        materialAdapter->diffuseLightWrap()
    };
    shaders.setUniform(ubufData, "qt_material_properties2", materialProperties2, 4 * sizeof(float), &cui.material_properties2Idx);

    const float materialProperties3[4] = {
        materialAdapter->occlusionAmount(),
        materialAdapter->alphaCutOff(),
        materialAdapter->clearcoatAmount(),
        materialAdapter->clearcoatRoughnessAmount()
    };
    shaders.setUniform(ubufData, "qt_material_properties3", materialProperties3, 4 * sizeof(float), &cui.material_properties3Idx);

    const float materialProperties4[4] = {
        materialAdapter->heightAmount(),
        materialAdapter->minHeightSamples(),
        materialAdapter->maxHeightSamples(),
        materialAdapter->transmissionFactor()
    };
    shaders.setUniform(ubufData, "qt_material_properties4", materialProperties4, 4 * sizeof(float), &cui.material_properties4Idx);

    const bool hasCustomFrag = materialAdapter->hasCustomShaderSnippet(QSSGShaderCache::ShaderType::Fragment);
    if (!hasCustomFrag) {
        if (inProperties.m_fresnelScaleBiasEnabled.getValue(inKey) || inProperties.m_clearcoatFresnelScaleBiasEnabled.getValue(inKey)) {
            const float materialProperties5[4] = {
                materialAdapter->fresnelScale(),
                materialAdapter->fresnelBias(),
                materialAdapter->clearcoatFresnelScale(),
                materialAdapter->clearcoatFresnelBias()
            };
            shaders.setUniform(ubufData, "qt_material_properties5", materialProperties5, 4 * sizeof(float), &cui.material_properties5Idx);
        }

        const float material_clearcoat_normal_strength = materialAdapter->clearcoatNormalStrength();
        shaders.setUniform(ubufData, "qt_material_clearcoat_normal_strength", &material_clearcoat_normal_strength, sizeof(float), &cui.clearcoatNormalStrengthIdx);

        const float material_clearcoat_fresnel_power = materialAdapter->clearcoatFresnelPower();
        shaders.setUniform(ubufData, "qt_material_clearcoat_fresnel_power", &material_clearcoat_fresnel_power, sizeof(float), &cui.clearcoatFresnelPowerIdx);
        // We only ever use attenuation and thickness uniforms when using transmission
        if (materialAdapter->isTransmissionEnabled()) {
            const QVector4D attenuationProperties(materialAdapter->attenuationColor(), materialAdapter->attenuationDistance());
            shaders.setUniform(ubufData, "qt_material_attenuation", &attenuationProperties, 4 * sizeof(float), &cui.material_attenuationIdx);

            const float thickness = materialAdapter->thicknessFactor();
            shaders.setUniform(ubufData, "qt_material_thickness", &thickness, sizeof(float), &cui.thicknessFactorIdx);
        }
    }

    const float rhiProperties[4] = {
        globalRenderData.isYUpInFramebuffer ? 1.0f : -1.0f,
        globalRenderData.isYUpInNDC ? 1.0f : -1.0f,
        globalRenderData.isClipDepthZeroToOne ? 0.0f : -1.0f,
        0.0f // unused
    };
    shaders.setUniform(ubufData, "qt_rhi_properties", rhiProperties, 4 * sizeof(float), &cui.rhiPropertiesIdx);

    qsizetype imageIdx = 0;
    for (QSSGRenderableImage *theImage = inFirstImage; theImage; theImage = theImage->m_nextImage, ++imageIdx) {
        // we need to map image to uniform name: "image0_rotations", "image0_offsets", etc...
        const auto &names = imageStringTable[int(theImage->m_mapType)];
        if (imageIdx == cui.imageIndices.size())
            cui.imageIndices.append(QSSGRhiShaderPipeline::CommonUniformIndices::ImageIndices());
        auto &indices = cui.imageIndices[imageIdx];

        const QMatrix4x4 &textureTransform = theImage->m_imageNode.m_textureTransform;
        // We separate rotational information from offset information so that just maybe the shader
        // will attempt to push less information to the card.
        const float *dataPtr(textureTransform.constData());
        // The third member of the offsets contains a flag indicating if the texture was
        // premultiplied or not.
        // We use this to mix the texture alpha.
        const float offsets[3] = { dataPtr[12], dataPtr[13], 0.0f /* non-premultiplied */ };
        shaders.setUniform(ubufData, names.imageOffsets, offsets, sizeof(offsets), &indices.imageOffsetsUniformIndex);
        // Grab just the upper 2x2 rotation matrix from the larger matrix.
        const float rotations[4] = { dataPtr[0], dataPtr[4], dataPtr[1], dataPtr[5] };
        shaders.setUniform(ubufData, names.imageRotations, rotations, sizeof(rotations), &indices.imageRotationsUniformIndex);
    }

    if (shadowDepthAdjust)
        shaders.setUniform(ubufData, "qt_shadowDepthAdjust", shadowDepthAdjust, 2 * sizeof(float), &cui.shadowDepthAdjustIdx);

    const bool usesPointsTopology = inProperties.m_usesPointsTopology.getValue(inKey);
    if (usesPointsTopology) {
        const float pointSize = materialAdapter->pointSize();
        shaders.setUniform(ubufData, "qt_materialPointSize", &pointSize, sizeof(float), &cui.pointSizeIdx);
    }

    // qt_fogColor = (fogColor.x, fogColor.y, fogColor.z, fogDensity)
    // qt_fogDepthProperties = (fogDepthBegin, fogDepthEnd, fogDepthCurve, fogDepthEnabled ? 1.0 : 0.0)
    // qt_fogHeightProperties = (fogHeightMin, fogHeightMax, fogHeightCurve, fogHeightEnabled ? 1.0 : 0.0)
    // qt_fogTransmitProperties = (fogTransmitCurve, 0.0, 0.0, fogTransmitEnabled ? 1.0 : 0.0)
    if (inRenderProperties.layer.fog.enabled) {
        const float fogColor[4] = {
            inRenderProperties.layer.fog.color.x(),
            inRenderProperties.layer.fog.color.y(),
            inRenderProperties.layer.fog.color.z(),
            inRenderProperties.layer.fog.density
        };
        shaders.setUniform(ubufData, "qt_fogColor", fogColor, 4 * sizeof(float), &cui.fogColorIdx);
        const float fogDepthProperties[4] = {
            inRenderProperties.layer.fog.depthBegin,
            inRenderProperties.layer.fog.depthEnd,
            inRenderProperties.layer.fog.depthCurve,
            inRenderProperties.layer.fog.depthEnabled ? 1.0f : 0.0f
        };
        shaders.setUniform(ubufData, "qt_fogDepthProperties", fogDepthProperties, 4 * sizeof(float), &cui.fogDepthPropertiesIdx);
        const float fogHeightProperties[4] = {
            inRenderProperties.layer.fog.heightMin,
            inRenderProperties.layer.fog.heightMax,
            inRenderProperties.layer.fog.heightCurve,
            inRenderProperties.layer.fog.heightEnabled ? 1.0f : 0.0f
        };
        shaders.setUniform(ubufData, "qt_fogHeightProperties", fogHeightProperties, 4 * sizeof(float), &cui.fogHeightPropertiesIdx);
        const float fogTransmitProperties[4] = {
            inRenderProperties.layer.fog.transmitCurve,
            0.0f,
            0.0f,
            inRenderProperties.layer.fog.transmitEnabled ? 1.0f : 0.0f
        };
        shaders.setUniform(ubufData, "qt_fogTransmitProperties", fogTransmitProperties, 4 * sizeof(float), &cui.fogTransmitPropertiesIdx);
    }

    inPipelineState->lineWidth = materialAdapter->lineWidth();
}

QT_END_NAMESPACE

QList<QByteArrayView> QtQuick3DEditorHelpers::CustomMaterial::reservedArgumentNames()
{
    return {std::begin(qssg_shader_arg_names), std::end(qssg_shader_arg_names) };;
}
