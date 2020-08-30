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

/* clang-format off */

#include <QtQuick3DUtils/private/qssgutils_p.h>

#include <QtQuick3DRuntimeRender/private/qssgrenderdefaultmaterialshadergenerator_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercontextcore_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercodegenerator_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderimage_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderlight_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercamera_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadowmap_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercustommaterial_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershaderlibrarymanager_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershaderkeys_p.h>
#include <QtQuick3DRuntimeRender/private/qssgshadermaterialadapter_p.h>

#include <QtCore/QByteArray>

QT_BEGIN_NAMESPACE

//    void generateTextureSwizzle(QSSGRenderTextureSwizzleMode swizzleMode, QByteArray &texSwizzle, QByteArray &lookupSwizzle)
//    {
//        if (!(deprecatedContextFlags & m_renderContext->renderContext()->renderContextType())) {
//            switch (swizzleMode) {
//            case QSSGRenderTextureSwizzleMode::L8toR8:
//            case QSSGRenderTextureSwizzleMode::L16toR16:
//                texSwizzle.append(".rgb");
//                lookupSwizzle.append(".rrr");
//                break;
//            case QSSGRenderTextureSwizzleMode::L8A8toRG8:
//                texSwizzle.append(".rgba");
//                lookupSwizzle.append(".rrrg");
//                break;
//            case QSSGRenderTextureSwizzleMode::A8toR8:
//                texSwizzle.append(".a");
//                lookupSwizzle.append(".r");
//                break;
//            default:
//                break;
//            }
//        }
//    }

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
DefineImageStrings(LightmapIndirect);
DefineImageStrings(LightmapRadiosity);
DefineImageStrings(LightmapShadow);
DefineImageStrings(Roughness);
DefineImageStrings(BaseColor);
DefineImageStrings(Metalness);
DefineImageStrings(Occlusion);

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
    DefineImageStringTableEntry(LightmapIndirect),
    DefineImageStringTableEntry(LightmapRadiosity),
    DefineImageStringTableEntry(LightmapShadow),
    DefineImageStringTableEntry(Roughness),
    DefineImageStringTableEntry(BaseColor),
    DefineImageStringTableEntry(Metalness),
    DefineImageStringTableEntry(Occlusion)
};

const int TEXCOORD_VAR_LEN = 16;

void textureCoordVariableName(char (&outString)[TEXCOORD_VAR_LEN], quint8 uvSet)
{
    // For now, uvSet will be less than 2.
    // But this value will be verified in the setProperty function.
    Q_ASSERT(uvSet < 9);
    qstrncpy(outString, "qt_varTexCoordX", TEXCOORD_VAR_LEN);
    outString[14] = '0' + uvSet;
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

static void generateImageUVCoordinates(QSSGMaterialVertexPipeline &vertexShader,
                                       QSSGStageGeneratorBase &fragmentShader,
                                       const QSSGShaderDefaultMaterialKey &key,
                                       QSSGRenderableImage &image,
                                       quint32 uvSet = 0)
{
    if (image.uvCoordsGenerated)
        return;

    const auto &names = imageStringTable[int(image.m_mapType)];
    char textureCoordName[TEXCOORD_VAR_LEN];
    textureCoordVariableName(textureCoordName, uvSet);
    fragmentShader.addUniform(names.imageSampler, "sampler2D");
    vertexShader.addUniform(names.imageOffsets, "vec3");
    vertexShader.addUniform(names.imageRotations, "vec4");
    QByteArray uvTrans = uvTransform(names.imageRotations, names.imageOffsets);
    if (image.m_image.m_mappingMode == QSSGRenderImage::MappingModes::Normal) {
        vertexShader << uvTrans;
        vertexShader.addOutgoing(names.imageFragCoords, "vec2");
        vertexShader.addFunction("getTransformedUVCoords");
        vertexShader.generateUVCoords(uvSet, key);
        vertexShader << "    vec2 " << names.imageFragCoordsTemp << " = qt_getTransformedUVCoords(vec3(" << textureCoordName << ", 1.0), qt_uTransform, qt_vTransform);\n";
        if (image.m_image.m_textureData.m_textureFlags.isInvertUVCoords())
            vertexShader << "    " << names.imageFragCoordsTemp << ".y = 1.0 - " << names.imageFragCoordsTemp << ".y;\n";

        vertexShader.assignOutput(names.imageFragCoords, names.imageFragCoordsTemp);
    } else {
        fragmentShader.addUniform(names.imageOffsets, "vec3");
        fragmentShader.addUniform(names.imageRotations, "vec4");
        fragmentShader << uvTrans;
        vertexShader.generateEnvMapReflection(key);
        fragmentShader.addFunction("getTransformedUVCoords");
        fragmentShader << "    vec2 " << names.imageFragCoords << " = qt_getTransformedUVCoords(environment_map_reflection, qt_uTransform, qt_vTransform);\n";
        if (image.m_image.m_textureData.m_textureFlags.isInvertUVCoords())
            fragmentShader << "    " << names.imageFragCoords << ".y = 1.0 - " << names.imageFragCoords << ".y;\n";
    }
    image.uvCoordsGenerated = true;
}

static void generateImageUVSampler(QSSGMaterialVertexPipeline &vertexGenerator,
                                   QSSGStageGeneratorBase &fragmentShader,
                                   const QSSGShaderDefaultMaterialKey &key,
                                   const QSSGRenderableImage &image,
                                   char (&outString)[TEXCOORD_VAR_LEN],
                                   quint8 uvSet = 0)
{
    const auto &names = imageStringTable[int(image.m_mapType)];
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
    switch (inSpecularModel) {
    case QSSGRenderDefaultMaterial::MaterialSpecularModel::KGGX: {
        fragmentShader.addInclude("physGlossyBSDF.glsllib");
        fragmentShader << "    global_specular_light.rgb += qt_lightAttenuation * qt_shadow_map_occl * qt_specularAmount"
                          " * kggxGlossyDefaultMtl(qt_world_normal, qt_tangent, -" << inLightDir << ".xyz, qt_view_vector, " << inLightSpecColor << ".rgb, qt_specularTint, qt_roughnessAmount).rgb;\n";
    } break;
    case QSSGRenderDefaultMaterial::MaterialSpecularModel::KWard: {
        fragmentShader.addInclude("physGlossyBSDF.glsllib");
        fragmentShader << "    global_specular_light.rgb += qt_lightAttenuation * qt_shadow_map_occl * qt_specularAmount"
                          " * qt_wardGlossyDefaultMtl(qt_world_normal, qt_tangent, -" << inLightDir << ".xyz, qt_view_vector, " << inLightSpecColor << ".rgb, qt_specularTint, qt_roughnessAmount).rgb;\n";
    } break;
    default:
        fragmentShader.addFunction("specularBSDF");
        fragmentShader << "    global_specular_light.rgb += qt_lightAttenuation * qt_shadow_map_occl * qt_specularAmount"
                          " * qt_specularBSDF(qt_world_normal, -" << inLightDir << ".xyz, qt_view_vector, " << inLightSpecColor << ".rgb, 2.56 / (qt_roughnessAmount + 0.01)).rgb;\n";
        break;
    }
}

static void outputSpecularAreaLighting(QSSGStageGeneratorBase &infragmentShader,
                                       const QByteArray &inPos,
                                       const QByteArray &inView,
                                       const QByteArray &inLightSpecColor,
                                       const QSSGMaterialShaderGenerator::LightVariableNames &lightVarNames)
{
    infragmentShader.addFunction("sampleAreaGlossyDefault");
    infragmentShader << "global_specular_light.rgb += " << inLightSpecColor << ".rgb * qt_lightAttenuation * qt_shadow_map_occl * qt_specularTint * qt_specularAmount * qt_sampleAreaGlossyDefault(qt_tanFrame, "
                     << inPos << ", "
                     << lightVarNames.normalizedDirection << ", "
                     << lightVarNames.lightPos << ".xyz, "
                     << lightVarNames.lightRt << ".w, "
                     << lightVarNames.lightUp << ".w, "
                     << inView << ", qt_roughnessAmount).rgb;\n";
}

static void addTranslucencyIrradiance(QSSGStageGeneratorBase &infragmentShader,
                                      QSSGRenderableImage *image,
                                      bool areaLight,
                                      const QSSGMaterialShaderGenerator::LightVariableNames &lightVarNames)
{
    if (image == nullptr)
        return;

    infragmentShader.addFunction("diffuseReflectionWrapBSDF");
    infragmentShader << "    tmp_light_color = " << lightVarNames.lightColor << ".rgb * (1.0 - qt_metalnessAmount);\n";
    if (areaLight) {
        infragmentShader << "    global_diffuse_light.rgb += qt_lightAttenuation * qt_shadow_map_occl * qt_translucent_thickness_exp * qt_diffuseReflectionWrapBSDF(-qt_world_normal, "
                         << lightVarNames.normalizedDirection << ", tmp_light_color, qt_diffuseLightWrap).rgb;\n";
    } else {
        infragmentShader << "    global_diffuse_light.rgb += qt_lightAttenuation * qt_shadow_map_occl * qt_translucent_thickness_exp * qt_diffuseReflectionWrapBSDF(-qt_world_normal, -"
                         << lightVarNames.normalizedDirection << ", tmp_light_color, qt_diffuseLightWrap).rgb;\n";
    }
}

static QSSGMaterialShaderGenerator::ShadowVariableNames setupShadowMapVariableNames(size_t lightIdx)
{
    QSSGMaterialShaderGenerator::ShadowVariableNames names;
    names.shadowMapStem = QByteArrayLiteral("qt_shadowmap");
    names.shadowCubeStem = QByteArrayLiteral("qt_shadowcube");
    char buf[16];
    qsnprintf(buf, 16, "%d", int(lightIdx));
    names.shadowCubeStem.append(buf);
    names.shadowMapStem.append(buf);
    names.shadowMatrixStem = names.shadowMapStem;
    names.shadowMatrixStem.append("_matrix");
    names.shadowCoordStem = names.shadowMapStem;
    names.shadowCoordStem.append("_coord");
    names.shadowControlStem = names.shadowMapStem;
    names.shadowControlStem.append("_control");

    return names;
}

static void addSpecularAmount(QSSGStageGeneratorBase &fragmentShader, bool &fragmentHasSpecularAmount, bool hasCustomFrag = false, bool reapply = false)
{
    // specularBase * vec3(metalnessAmount + specularAmount * (1 - metalnessAmount))
    if (!fragmentHasSpecularAmount) {
        if (hasCustomFrag)
            fragmentShader << "    vec3 qt_specularAmount = qt_specularBase * vec3(qt_customMetalnessAmount + qt_customSpecularAmount * (1.0 - qt_customMetalnessAmount));\n";
        else
            fragmentShader << "    vec3 qt_specularAmount = qt_specularBase * vec3(qt_material_properties.z + qt_material_properties.x * (1.0 - qt_material_properties.z));\n";
    } else if (reapply) {
        if (hasCustomFrag)
            fragmentShader << "    qt_specularAmount = qt_specularBase * vec3(qt_customMetalnessAmount + qt_customSpecularAmount * (1.0 - qt_customMetalnessAmount));\n";
        else
            fragmentShader << "    qt_specularAmount = qt_specularBase * vec3(qt_material_properties.z + qt_material_properties.x * (1.0 - qt_material_properties.z));\n";
    }
    fragmentHasSpecularAmount = true;
}

static void maybeAddMaterialFresnel(QSSGStageGeneratorBase &fragmentShader,
                                    const QSSGShaderDefaultMaterialKeyProperties &keyProps,
                                    QSSGDataView<quint32> inKey,
                                    bool &fragmentHasSpecularAmount,
                                    bool hasMetalness,
                                    bool hasCustomFrag)
{
    if (hasCustomFrag || keyProps.m_fresnelEnabled.getValue(inKey)) {
        addSpecularAmount(fragmentShader, fragmentHasSpecularAmount);
        fragmentShader.addInclude("defaultMaterialFresnel.glsllib");
        fragmentShader << "    // Add fresnel ratio\n";
        if (hasCustomFrag) {
            // just use the metallic version, the results are identical to qt_defaultMaterialSimpleFresnelNoMetalness when metalnessAmount is 0
            fragmentShader << "    qt_specularAmount *= qt_defaultMaterialSimpleFresnel(qt_specularBase, qt_metalnessAmount, qt_world_normal, qt_view_vector, "
                              "qt_dielectricSpecular(qt_customIOR), qt_customFresnelPower);\n";
        } else {
            fragmentShader.addUniform("qt_fresnelPower", "float");
            if (hasMetalness) {
                fragmentShader << "    qt_specularAmount *= qt_defaultMaterialSimpleFresnel(qt_specularBase, qt_metalnessAmount, qt_world_normal, qt_view_vector, "
                                  "qt_dielectricSpecular(qt_material_specular.w), qt_fresnelPower);\n";
            } else {
                fragmentShader << "    qt_specularAmount *= qt_defaultMaterialSimpleFresnelNoMetalness(qt_world_normal, qt_view_vector, "
                                  "qt_dielectricSpecular(qt_material_specular.w), qt_fresnelPower);\n";
            }
        }
    }
}

static QSSGMaterialShaderGenerator::LightVariableNames setupLightVariableNames(qint32 lightIdx, QSSGRenderLight &inLight)
{
    Q_ASSERT(lightIdx > -1);
    QSSGMaterialShaderGenerator::LightVariableNames names;
    QByteArray lightStem = "lights";
    char buf[16];
    qsnprintf(buf, 16, "[%d].", int(lightIdx));
    lightStem.append(buf);

    names.lightColor = lightStem;
    names.lightColor.append("diffuse");
    names.lightDirection = lightStem;
    names.lightDirection.append("direction");
    names.lightSpecularColor = lightStem;
    names.lightSpecularColor.append("specular");
    if (inLight.m_lightType == QSSGRenderLight::Type::Point) {
        names.lightPos = lightStem;
        names.lightPos.append("position");
        names.lightConstantAttenuation = lightStem;
        names.lightConstantAttenuation.append("constantAttenuation");
        names.lightLinearAttenuation = lightStem;
        names.lightLinearAttenuation.append("linearAttenuation");
        names.lightQuadraticAttenuation = lightStem;
        names.lightQuadraticAttenuation.append("quadraticAttenuation");
    } else if (inLight.m_lightType == QSSGRenderLight::Type::Area) {
        names.lightPos = lightStem;
        names.lightPos.append("position");
        names.lightUp = lightStem;
        names.lightUp.append("up");
        names.lightRt = lightStem;
        names.lightRt.append("right");
    } else if (inLight.m_lightType == QSSGRenderLight::Type::Spot) {
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
                                       quint32 lightIdx,
                                       bool inShadowEnabled,
                                       QSSGRenderLight::Type inType,
                                       const QSSGMaterialShaderGenerator::LightVariableNames &lightVarNames)
{
    if (inShadowEnabled) {
        vertexShader.generateWorldPosition();
        const auto names = setupShadowMapVariableNames(lightIdx);
        fragmentShader.addInclude("shadowMapping.glsllib");
        if (inType == QSSGRenderLight::Type::Directional) {
            fragmentShader.addUniform(names.shadowMapStem, "sampler2D");
        } else {
            fragmentShader.addUniform(names.shadowCubeStem, "samplerCube");
        }
        fragmentShader.addUniform(names.shadowControlStem, "vec4");
        fragmentShader.addUniform(names.shadowMatrixStem, "mat4");

        if (inType != QSSGRenderLight::Type::Directional) {
            fragmentShader << "    qt_shadow_map_occl = qt_sampleCubemap(" << names.shadowCubeStem << ", " << names.shadowControlStem << ", " << names.shadowMatrixStem << ", " << lightVarNames.lightPos << ".xyz, qt_varWorldPos, vec2(1.0, " << names.shadowControlStem << ".z));\n";
        } else {
            fragmentShader << "    qt_shadow_map_occl = qt_sampleOrthographic(" << names.shadowMapStem << ", " << names.shadowControlStem << ", " << names.shadowMatrixStem << ", qt_varWorldPos, vec2(1.0, " << names.shadowControlStem << ".z));\n";
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
        return static_cast<const QSSGRenderDefaultMaterial &>(inMaterial).adapter;
    case QSSGRenderGraphObject::Type::CustomMaterial:
        return static_cast<const QSSGRenderCustomMaterial &>(inMaterial).adapter;
    default:
        break;
    }
    return nullptr;
}

const char *QSSGMaterialShaderGenerator::directionalLightProcessorArgumentList()
{
    return "inout vec3 DIFFUSE, in vec3 LIGHT_COLOR, in float SHADOW_CONTRIB, in vec3 TO_LIGHT_DIR, in vec3 NORMAL, in vec4 BASE_COLOR";
}

const char *QSSGMaterialShaderGenerator::pointLightProcessorArgumentList()
{
    return "inout vec3 DIFFUSE, in vec3 LIGHT_COLOR, in float LIGHT_ATTENUATION, in float SHADOW_CONTRIB, in vec3 TO_LIGHT_DIR, in vec3 NORMAL, in vec4 BASE_COLOR";
}

const char *QSSGMaterialShaderGenerator::areaLightProcessorArgumentList()
{
    return "inout vec3 DIFFUSE, in vec3 LIGHT_COLOR, in float LIGHT_ATTENUATION, in float SHADOW_CONTRIB, in vec3 TO_LIGHT_DIR, in vec3 NORMAL, in vec4 BASE_COLOR";
}

const char *QSSGMaterialShaderGenerator::spotLightProcessorArgumentList()
{
    return "inout vec3 DIFFUSE, in vec3 LIGHT_COLOR, in float LIGHT_ATTENUATION, float SPOT_FACTOR, in float SHADOW_CONTRIB, in vec3 TO_LIGHT_DIR, in vec3 NORMAL, in vec4 BASE_COLOR";
}

const char *QSSGMaterialShaderGenerator::ambientLightProcessorArgumentList()
{
    return "inout vec3 DIFFUSE, in vec3 TOTAL_AMBIENT_COLOR, in vec3 NORMAL";
}

const char *QSSGMaterialShaderGenerator::specularLightProcessorArgumentList()
{
    return "inout vec3 SPECULAR, in vec3 LIGHT_COLOR, in float LIGHT_ATTENUATION, in float SHADOW_CONTRIB, in vec3 TO_LIGHT_DIR, in vec3 NORMAL, in vec4 BASE_COLOR";
}

const char *QSSGMaterialShaderGenerator::shadedFragmentMainArgumentList()
{
    return "inout vec4 BASE_COLOR, inout vec3 EMISSIVE_COLOR, inout float METALNESS, inout float ROUGHNESS, inout float SPECULAR_AMOUNT, inout float FRESNEL_POWER, inout float IOR, inout vec3 NORMAL, inout vec3 TANGENT, inout vec3 BINORMAL, in vec2 UV0, in vec2 UV1";
}

const char *QSSGMaterialShaderGenerator::vertexMainArgumentList()
{
    return "inout vec3 VERTEX, inout vec3 NORMAL, inout vec2 UV0, inout vec2 UV1, inout vec3 TANGENT, inout vec3 BINORMAL, inout vec4 COLOR";
}

static void generateFragmentShader(QSSGStageGeneratorBase &fragmentShader,
                                   QSSGMaterialVertexPipeline &vertexShader,
                                   const QSSGShaderDefaultMaterialKey &inKey,
                                   const QSSGShaderDefaultMaterialKeyProperties &keyProps,
                                   const ShaderFeatureSetList &featureSet,
                                   const QSSGRenderGraphObject &inMaterial,
                                   const QSSGShaderLightList &lights,
                                   QSSGRenderableImage *firstImage,
                                   const QSSGRef<QSSGShaderLibraryManager> &shaderLibraryManager)
{
    QSSGShaderMaterialAdapter *materialAdapter = getMaterialAdapter(inMaterial);
    auto hasCustomFunction = [&shaderLibraryManager, materialAdapter](const QByteArray &funcName) {
        return materialAdapter->hasCustomShaderFunction(QSSGShaderCache::ShaderType::Fragment,
                                                 funcName,
                                                 shaderLibraryManager);
    };

    bool metalnessEnabled = materialAdapter->isMetalnessEnabled();
    bool specularLightingEnabled = metalnessEnabled || materialAdapter->isSpecularEnabled();
    bool vertexColorsEnabled = materialAdapter->isVertexColorsEnabled();

    bool hasLighting = materialAdapter->hasLighting();
    bool isDoubleSided = keyProps.m_isDoubleSided.getValue(inKey);
    bool hasImage = firstImage != nullptr;

    bool hasIblProbe = keyProps.m_hasIbl.getValue(inKey);
    bool hasEmissiveMap = false;
    bool hasLightmaps = false;
    bool hasBaseColorMap = false;
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
    // lightmaps
    QSSGRenderableImage *lightmapIndirectImage = nullptr;
    QSSGRenderableImage *lightmapRadiosityImage = nullptr;
    QSSGRenderableImage *lightmapShadowImage = nullptr;

    QSSGRenderableImage *baseImage = nullptr;

    // Use shared texcoord when transforms are identity
    QVector<QSSGRenderableImage *> identityImages;
    char imageFragCoords[TEXCOORD_VAR_LEN];

    Q_UNUSED(lightmapShadowImage);

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

    for (QSSGRenderableImage *img = firstImage; img != nullptr; img = img->m_nextImage, ++imageIdx) {
        if (img->m_image.isImageTransformIdentity())
            identityImages.push_back(img);
        if (img->m_mapType == QSSGRenderableImage::Type::BaseColor || img->m_mapType == QSSGRenderableImage::Type::Diffuse) {
            hasBaseColorMap = img->m_mapType == QSSGRenderableImage::Type::BaseColor;
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
        } else if (img->m_mapType == QSSGRenderableImage::Type::Emissive) {
            hasEmissiveMap = true;
        } else if (img->m_mapType == QSSGRenderableImage::Type::LightmapIndirect) {
            // LightmapIndirect/Radiosity/Shadow are possible also with a custom material, unlike all other image maps
            lightmapIndirectImage = img;
            hasLightmaps = true;
        } else if (img->m_mapType == QSSGRenderableImage::Type::LightmapRadiosity) {
            lightmapRadiosityImage = img;
            hasLightmaps = true;
        } else if (img->m_mapType == QSSGRenderableImage::Type::LightmapShadow) {
            lightmapShadowImage = img;
            hasLightmaps = true;
        }
    }

    bool enableSSAO = false;
    bool enableShadowMaps = false;
    bool isDepthPass = false;
    bool isOrthoShadowPass = false;
    bool isCubeShadowPass = false;
    bool enableBumpNormal = normalImage || bumpImage;
    specularLightingEnabled |= specularAmountImage != nullptr;

    for (qint32 idx = 0; idx < featureSet.size(); ++idx) {
        const auto &name = featureSet.at(idx).name;
        if (name == QSSGShaderDefines::asString(QSSGShaderDefines::Ssao))
            enableSSAO = featureSet.at(idx).enabled;
        else if (name == QSSGShaderDefines::asString(QSSGShaderDefines::Ssm))
            enableShadowMaps = featureSet.at(idx).enabled;
        else if (name == QSSGShaderDefines::asString(QSSGShaderDefines::DepthPass))
            isDepthPass = featureSet.at(idx).enabled;
        else if (name == QSSGShaderDefines::asString(QSSGShaderDefines::OrthoShadowPass))
            isOrthoShadowPass = featureSet.at(idx).enabled;
        else if (name == QSSGShaderDefines::asString(QSSGShaderDefines::CubeShadowPass))
            isCubeShadowPass = featureSet.at(idx).enabled;
    }

    bool includeCustomFragmentMain = true;
    if (isDepthPass || isOrthoShadowPass || isCubeShadowPass) {
        hasLighting = false;
        enableSSAO = false;
        enableShadowMaps = false;

        metalnessEnabled = false;
        specularLightingEnabled = false;
        vertexColorsEnabled = false;

        hasBaseColorMap = false;
        baseImage = nullptr;

        includeCustomFragmentMain = false;
    }

    bool includeSSAOVars = enableSSAO || enableShadowMaps;

    vertexShader.beginFragmentGeneration(shaderLibraryManager);

    // Unshaded custom materials need no code in main (apart from calling qt_customMain)
    const bool hasCustomFrag = materialAdapter->hasCustomShaderSnippet(QSSGShaderCache::ShaderType::Fragment);
    if (hasCustomFrag && materialAdapter->isUnshaded())
        return;

    // hasCustomFrag == Shaded custom material from this point on, for Unshaded we returned above

    // The fragment or vertex shaders may not use the material_properties or diffuse
    // uniforms in all cases but it is simpler to just add them and let the linker strip them.
    fragmentShader.addUniform("qt_material_emissive_color", "vec3");
    fragmentShader.addUniform("qt_material_base_color", "vec4");
    fragmentShader.addUniform("qt_material_properties", "vec4");

    if (vertexColorsEnabled)
        vertexShader.generateVertexColor(inKey);
    else
        fragmentShader.append("    vec4 qt_vertColor = vec4(1.0);");

    if (hasLighting || hasCustomFrag) {
        // Do not move these three. These varyings are exposed to custom material shaders too.
        vertexShader.generateViewVector();
        if (keyProps.m_usesProjectionMatrix.getValue(inKey))
            fragmentShader.addUniform("qt_projectionMatrix", "mat4");
        if (keyProps.m_usesInverseProjectionMatrix.getValue(inKey))
            fragmentShader.addUniform("qt_inverseProjectionMatrix", "mat4");
        vertexShader.generateWorldNormal(inKey);
        vertexShader.generateWorldPosition();

        fragmentShader.append("    vec3 qt_org_normal = qt_world_normal;\n");
        fragmentShader.addUniform("qt_normalAdjustViewportFactor", "float"); // aka FRAMEBUFFER_Y_UP
        fragmentShader.addUniform("qt_nearClipValue", "float"); // aka NEAR_CLIP_VALUE
        if (isDoubleSided) {
            fragmentShader.addInclude("doubleSided.glsllib");
            fragmentShader.append("    qt_world_normal = qt_adjustNormalForFace(qt_world_normal, qt_varWorldPos, qt_normalAdjustViewportFactor);\n");
        }
    }

    if (hasCustomFrag) {
        // A custom shaded material is effectively a principled material for
        // our purposes here. The defaults are different from a
        // PrincipledMaterial however, since this is more sensible here.
        // (because the shader has to state it to get things)
        vertexShader.generateVarTangentAndBinormal(inKey);
        fragmentShader << "    float qt_customSpecularAmount = 0.0;\n"; // overrides qt_material_properties.x
        fragmentShader << "    float qt_customSpecularRoughness = 0.0;\n"; // overrides qt_material_properties.y
        fragmentShader << "    float qt_customMetalnessAmount = 0.0;\n"; // overrides qt_material_properties.z
        fragmentShader << "    float qt_customFresnelPower = 0.0;\n"; // overrides qt_fresnelPower
        fragmentShader << "    float qt_customIOR = 1.0;\n"; // overrides qt_material_specular.a
        fragmentShader << "    vec4 qt_customBaseColor = vec4(1.0);\n"; // overrides qt_material_base_color
        fragmentShader << "    vec3 qt_customEmissiveColor = vec3(0.0);\n"; // overrides qt_material_emissive_color
        // Generate the varyings for UV0 and UV1 since customer materials don't use image
        // properties directly.
        vertexShader.generateUVCoords(0, inKey);
        vertexShader.generateUVCoords(1, inKey);
        if (includeCustomFragmentMain && hasCustomFunction(QByteArrayLiteral("qt_customMain"))) {
            fragmentShader << "    qt_customMain(qt_customBaseColor, qt_customEmissiveColor, qt_customMetalnessAmount, qt_customSpecularRoughness,"
                              " qt_customSpecularAmount, qt_customFresnelPower, qt_customIOR, qt_world_normal, qt_tangent, qt_binormal,"
                              " qt_varTexCoord0, qt_varTexCoord1);\n";
        }
        fragmentShader << "    vec4 qt_diffuseColor = qt_customBaseColor * qt_vertColor;\n";
    } else {
        fragmentShader << "    vec4 qt_diffuseColor = qt_material_base_color * qt_vertColor;\n";
    }

    if (isDepthPass)
        fragmentShader << "    vec4 fragOutput = vec4(0.0);\n";

    if (isOrthoShadowPass)
        vertexShader.generateDepth();

    if (isCubeShadowPass)
        vertexShader.generateShadowWorldPosition();

    // !hasLighting does not mean 'no light source'
    // it should be KHR_materials_unlit
    // https://github.com/KhronosGroup/glTF/tree/master/extensions/2.0/Khronos/KHR_materials_unlit
    if (hasLighting) {
        if (includeSSAOVars)
            fragmentShader.addInclude("ssao.glsllib");

        if (hasIblProbe)
            fragmentShader.addInclude("sampleProbe.glsllib");

        fragmentShader.addFunction("sampleLightVars");
        fragmentShader.addFunction("diffuseReflectionBSDF");

        if (hasLightmaps)
            fragmentShader.addInclude("evalLightmaps.glsllib");

        if (hasImage) {
            fragmentShader.append("    vec3 qt_uTransform;");
            fragmentShader.append("    vec3 qt_vTransform;");
        }

        if (includeSSAOVars || specularLightingEnabled || hasIblProbe || enableBumpNormal)
            vertexShader.generateVarTangentAndBinormal(inKey);

        fragmentShader.append("    float qt_facing = step(0.0, dot(qt_view_vector, qt_org_normal)) * 2.0 - 1.0;\n");

        if (bumpImage != nullptr) {
            generateImageUVCoordinates(vertexShader, fragmentShader, inKey, *bumpImage, bumpImage->m_image.m_indexUV);
            const auto &names = imageStringTable[int(QSSGRenderableImage::Type::Bump)];

            fragmentShader << "    if (qt_tangent == vec3(0.0)) {\n"
                           << "        vec2 dUVdx = dFdx(" << names.imageFragCoords << ");\n"
                           << "        vec2 dUVdy = dFdy(" << names.imageFragCoords << ");\n";
        } else if (normalImage != nullptr) {
            generateImageUVCoordinates(vertexShader, fragmentShader, inKey, *normalImage, normalImage->m_image.m_indexUV);
            const auto &names = imageStringTable[int(QSSGRenderableImage::Type::Normal)];
            fragmentShader << "    if (qt_tangent == vec3(0.0)) {\n"
                           << "        vec2 dUVdx = dFdx(" << names.imageFragCoords << ");\n"
                           << "        vec2 dUVdy = dFdy(" << names.imageFragCoords << ");\n";
        }

        if (enableBumpNormal) {
            fragmentShader.addUniform("qt_bumpAmount", "float");
            fragmentShader << "        qt_tangent = (dUVdy.y * dFdx(qt_varWorldPos) - dUVdx.y * dFdy(qt_varWorldPos)) / (dUVdx.x * dUVdy.y - dUVdx.y * dUVdy.x);\n"
                           << "        qt_tangent = qt_tangent - dot(qt_org_normal, qt_tangent) * qt_org_normal;\n"
                           << "        qt_tangent = normalize(qt_tangent);\n"
                           << "    }\n";
            fragmentShader << "    if (qt_binormal == vec3(0.0))\n"
                           << "        qt_binormal = cross(qt_org_normal, qt_tangent);\n";
        }

        // apply facing factor before fetching texture
        fragmentShader.append("    qt_org_normal *= qt_facing;");
        if (includeSSAOVars || specularLightingEnabled || hasIblProbe || enableBumpNormal) {
            fragmentShader.append("    qt_tangent *= qt_facing;");
            fragmentShader.append("    qt_binormal *= qt_facing;");
        }

        if (bumpImage != nullptr) {
            const auto &names = imageStringTable[int(QSSGRenderableImage::Type::Bump)];
            fragmentShader.addUniform(names.imageSamplerSize, "vec2");
            fragmentShader.addInclude("defaultMaterialBumpNoLod.glsllib");
            fragmentShader << "    qt_world_normal = qt_defaultMaterialBumpNoLod(" << names.imageSampler << ", qt_bumpAmount, " << names.imageFragCoords << ", qt_tangent, qt_binormal, qt_org_normal, " << names.imageSamplerSize << ");\n";
            if (isDoubleSided)
                fragmentShader.append("    qt_world_normal = qt_adjustNormalForFace(qt_world_normal, qt_varWorldPos, qt_normalAdjustViewportFactor);\n");
        } else if (normalImage != nullptr) {
            const auto &names = imageStringTable[int(QSSGRenderableImage::Type::Normal)];
            fragmentShader.addFunction("sampleNormalTexture");
            fragmentShader << "    qt_world_normal = qt_sampleNormalTexture(" << names.imageSampler << ", qt_bumpAmount, " << names.imageFragCoords << ", qt_tangent, qt_binormal, qt_org_normal);\n";
            if (isDoubleSided)
                fragmentShader.append("    qt_world_normal = qt_adjustNormalForFace(qt_world_normal, qt_varWorldPos, qt_normalAdjustViewportFactor);\n");
        }

        if (includeSSAOVars || specularLightingEnabled || hasIblProbe || enableBumpNormal)
            fragmentShader << "    mat3 qt_tanFrame = mat3(qt_tangent, qt_binormal, qt_world_normal);\n";

        if (hasEmissiveMap)
            fragmentShader.append("    vec3 qt_global_emission = qt_material_emissive_color;");

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
        QByteArray texSwizzle;
        QByteArray lookupSwizzle;

        // NoLighting also needs to fetch baseImage (but not if it's a depth or
        // shadow pass, those won't hit this due to baseImage forced to null then)
        if (!hasLighting) {
            fragmentShader.append("    vec3 qt_uTransform;");
            fragmentShader.append("    vec3 qt_vTransform;");
        }

        const bool hasIdentityMap = identityImages.contains(baseImage);
        if (hasIdentityMap)
            generateImageUVSampler(vertexShader, fragmentShader, inKey, *baseImage, imageFragCoords, baseImage->m_image.m_indexUV);
        else
            generateImageUVCoordinates(vertexShader, fragmentShader, inKey, *baseImage, baseImage->m_image.m_indexUV);

        //            if (baseImage->m_image.m_textureData.m_texture) {
        //                // not supported for rhi
        //                generateTextureSwizzle(baseImage->m_image.m_textureData.m_texture->textureSwizzleMode(), texSwizzle, lookupSwizzle);
        //            }

        // NOTE: The base image hande is used for both the diffuse map and the base color map, so we can't hard-code the type here...
        const auto &names = imageStringTable[int(baseImage->m_mapType)];
        // Diffuse and BaseColor maps need to converted to linear color space
        fragmentShader.addInclude("tonemapping.glsllib");
        fragmentShader << "    vec4 qt_base_texture_color" << texSwizzle << " = qt_sRGBToLinear(texture2D(" << names.imageSampler << ", " << (hasIdentityMap ? imageFragCoords : names.imageFragCoords) << "))" << lookupSwizzle << ";\n";
        fragmentShader << "    qt_diffuseColor *= qt_base_texture_color;\n";
    }

    if (hasLighting) {
        bool fragmentHasSpecularAmount = false;

        if (hasCustomFrag)
            fragmentShader << "    float qt_metalnessAmount = qt_customMetalnessAmount;\n";
        else
            fragmentShader << "    float qt_metalnessAmount = qt_material_properties.z;\n";

        fragmentShader.addUniform("qt_light_ambient_total", "vec3");

        if (hasCustomFrag && hasCustomFunction(QByteArrayLiteral("qt_ambientLightProcessor"))) {
            // DIFFUSE, TOTAL_AMBIENT_COLOR, NORMAL
            fragmentShader.append("    vec4 global_diffuse_light = vec4(0.0);\n"
                                  "    qt_ambientLightProcessor(global_diffuse_light.rgb, qt_light_ambient_total.rgb * (1.0 - qt_metalnessAmount) * qt_diffuseColor.rgb, qt_world_normal);\n");
        } else {
            fragmentShader.append("    vec4 global_diffuse_light = vec4(qt_light_ambient_total.rgb * (1.0 - qt_metalnessAmount) * qt_diffuseColor.rgb, 0.0);");
        }

        fragmentShader.append("    vec3 global_specular_light = vec3(0.0);");
        fragmentShader.append("    float qt_shadow_map_occl = 1.0;");

        if (specularLightingEnabled) {
            fragmentShader << "    qt_specularBase = qt_diffuseColor.rgb;\n";
            vertexShader.generateViewVector();
            fragmentShader.addUniform("qt_material_properties", "vec4");
            addSpecularAmount(fragmentShader, fragmentHasSpecularAmount, hasCustomFrag);
        }

        if (lightmapIndirectImage != nullptr) {
            const bool hasIdentityMap = identityImages.contains(lightmapIndirectImage);
            if (hasIdentityMap)
                generateImageUVSampler(vertexShader, fragmentShader, inKey, *lightmapIndirectImage, imageFragCoords, 1);
            else
                generateImageUVCoordinates(vertexShader, fragmentShader, inKey, *lightmapIndirectImage, 1);

            const auto &names = imageStringTable[int(QSSGRenderableImage::Type::LightmapIndirect)];
            // NOTE: When we start baking lightmaps, we need to make sure they are in linear colorspace
            fragmentShader << "    vec4 qt_indirect_light = texture2D(" << names.imageSampler << ", " << (hasIdentityMap ? imageFragCoords : names.imageFragCoords) << ");\n";
            fragmentShader << "    global_diffuse_light += qt_indirect_light;\n";
            if (specularLightingEnabled)
                fragmentShader << "    global_specular_light += qt_indirect_light.rgb * qt_specularAmount;\n";
        }

        if (lightmapRadiosityImage != nullptr) {
            const bool hasIdentityMap = identityImages.contains(lightmapRadiosityImage);
            if (identityImages.contains(lightmapRadiosityImage))
                generateImageUVSampler(vertexShader, fragmentShader, inKey, *lightmapRadiosityImage, imageFragCoords, 1);
            else
                generateImageUVCoordinates(vertexShader, fragmentShader, inKey, *lightmapRadiosityImage, 1);

            const auto &names = imageStringTable[int(QSSGRenderableImage::Type::LightmapRadiosity)];
            // NOTE: When we start baking lightmaps, we need to make sure they are in linear colorspace
            fragmentShader << "    vec4 qt_direct_light = texture2D(" << names.imageSampler << ", " << (hasIdentityMap ? imageFragCoords : names.imageFragCoords) << ");\n";
            fragmentShader << "    global_diffuse_light += qt_direct_light;\n";
            if (specularLightingEnabled)
                fragmentShader << "    global_specular_light += qt_direct_light.rgb * qt_specularAmount;\n";
        }

        if (translucencyImage != nullptr) {
            fragmentShader.addUniform("qt_translucentFalloff", "float");
            fragmentShader.addUniform("qt_diffuseLightWrap", "float");

            const bool hasIdentityMap = identityImages.contains(translucencyImage);
            if (hasIdentityMap)
                generateImageUVSampler(vertexShader, fragmentShader, inKey, *translucencyImage, imageFragCoords);
            else
                generateImageUVCoordinates(vertexShader, fragmentShader, inKey, *translucencyImage);

            const auto &names = imageStringTable[int(QSSGRenderableImage::Type::Translucency)];
            const auto &channelProps = keyProps.m_textureChannels[QSSGShaderDefaultMaterialKeyProperties::TranslucencyChannel];
            fragmentShader << "    float qt_translucent_depth_range = texture2D(" << names.imageSampler
                           << ", " << (hasIdentityMap ? imageFragCoords : names.imageFragCoords) << ")" << channelStr(channelProps, inKey) << ";\n";
            fragmentShader << "    float qt_translucent_thickness = qt_translucent_depth_range * qt_translucent_depth_range;\n";
            fragmentShader << "    float qt_translucent_thickness_exp = exp(qt_translucent_thickness * qt_translucentFalloff);\n";
        }

        fragmentShader.append("    float qt_lightAttenuation = 1.0;");

        addLocalVariable(fragmentShader, "qt_aoFactor", "float");

        if (enableSSAO)
            fragmentShader.append("    qt_aoFactor = qt_screenSpaceAmbientOcclusionFactor();");
        else
            fragmentShader.append("    qt_aoFactor = 1.0;");

        // Fragment lighting means we can perhaps attenuate the specular amount by a texture
        // lookup.
        if (specularAmountImage) {
            addSpecularAmount(fragmentShader, fragmentHasSpecularAmount);

            const bool hasIdentityMap = identityImages.contains(specularAmountImage);
            if (hasIdentityMap)
                generateImageUVSampler(vertexShader, fragmentShader, inKey, *specularAmountImage, imageFragCoords, specularAmountImage->m_image.m_indexUV);
            else
                generateImageUVCoordinates(vertexShader, fragmentShader, inKey, *specularAmountImage, specularAmountImage->m_image.m_indexUV);

            const auto &names = imageStringTable[int(QSSGRenderableImage::Type::SpecularAmountMap)];
            // TODO: This might need to be colorspace corrected to linear
            fragmentShader << "    qt_specularBase *= texture2D(" << names.imageSampler << ", " << (hasIdentityMap ? imageFragCoords : names.imageFragCoords) << ").rgb;\n";
        }

        if (hasCustomFrag)
            fragmentShader << "    float qt_roughnessAmount = qt_customSpecularRoughness;\n";
        else
            fragmentShader << "    float qt_roughnessAmount = qt_material_properties.y;\n";

        if (specularLightingEnabled && roughnessImage) {
            const auto &channelProps = keyProps.m_textureChannels[QSSGShaderDefaultMaterialKeyProperties::RoughnessChannel];
            const bool hasIdentityMap = identityImages.contains(roughnessImage);
            if (hasIdentityMap)
                generateImageUVSampler(vertexShader, fragmentShader, inKey, *roughnessImage, imageFragCoords, roughnessImage->m_image.m_indexUV);
            else
                generateImageUVCoordinates(vertexShader, fragmentShader, inKey, *roughnessImage, roughnessImage->m_image.m_indexUV);

            const auto &names = imageStringTable[int(QSSGRenderableImage::Type::Roughness)];
            fragmentShader << "    qt_roughnessAmount *= texture2D(" << names.imageSampler << ", "
                           << (hasIdentityMap ? imageFragCoords : names.imageFragCoords) << ")" << channelStr(channelProps, inKey) << ";\n";
        }

        if (specularLightingEnabled && metalnessImage) {
            const auto &channelProps = keyProps.m_textureChannels[QSSGShaderDefaultMaterialKeyProperties::MetalnessChannel];
            const bool hasIdentityMap = identityImages.contains(metalnessImage);
            if (hasIdentityMap)
                generateImageUVSampler(vertexShader, fragmentShader, inKey, *metalnessImage, imageFragCoords, metalnessImage->m_image.m_indexUV);
            else
                generateImageUVCoordinates(vertexShader, fragmentShader, inKey, *metalnessImage, metalnessImage->m_image.m_indexUV);

            const auto &names = imageStringTable[int(QSSGRenderableImage::Type::Metalness)];
            fragmentShader << "    float qt_sampledMetalness = texture2D(" << names.imageSampler << ", "
                           << (hasIdentityMap ? imageFragCoords : names.imageFragCoords) << ")" << channelStr(channelProps, inKey) << ";\n";
            fragmentShader << "    qt_metalnessAmount = clamp(qt_metalnessAmount * qt_sampledMetalness, 0.0, 1.0);\n";
            addSpecularAmount(fragmentShader, fragmentHasSpecularAmount, false, true);
        }
        if (specularLightingEnabled) {
            fragmentShader.addInclude("defaultMaterialFresnel.glsllib");
            if (hasCustomFrag)
                fragmentShader << "    qt_diffuseColor.rgb *= (1.0 - qt_dielectricSpecular(qt_customIOR)) * (1.0 - qt_metalnessAmount);\n";
            else
                fragmentShader << "    qt_diffuseColor.rgb *= (1.0 - qt_dielectricSpecular(qt_material_specular.w)) * (1.0 - qt_metalnessAmount);\n";
            if (!hasBaseColorMap && materialAdapter->isPrincipled()) {
                if (hasCustomFrag) {
                    fragmentShader << "    float qt_lum = dot(qt_customBaseColor.rgb, vec3(0.21, 0.72, 0.07));\n"
                                      "    qt_specularBase += (qt_lum > 0.0) ? (qt_customBaseColor.rgb) / qt_lum : vec3(1.0);\n";
                } else {
                    fragmentShader << "    float qt_lum = dot(qt_material_base_color.rgb, vec3(0.21, 0.72, 0.07));\n"
                                      "    qt_specularBase += (qt_lum > 0.0) ? (qt_material_base_color.rgb) / qt_lum : vec3(1.0);\n";
                }
            }

            maybeAddMaterialFresnel(fragmentShader, keyProps, inKey, fragmentHasSpecularAmount, metalnessEnabled, hasCustomFrag);
        }

        // Iterate through all lights
        Q_ASSERT(lights.size() < INT32_MAX);
        for (qint32 lightIdx = 0; lightIdx < lights.size(); ++lightIdx) {
            auto &shaderLight = lights[lightIdx];
            if (!shaderLight.enabled)
                continue;
            QSSGRenderLight *lightNode = shaderLight.light;
            auto lightVarNames = setupLightVariableNames(lightIdx, *lightNode);

            bool isDirectional = lightNode->m_lightType == QSSGRenderLight::Type::Directional;
            bool isArea = lightNode->m_lightType == QSSGRenderLight::Type::Area;
            bool isSpot = lightNode->m_lightType == QSSGRenderLight::Type::Spot;
            bool castsShadow = enableShadowMaps && lightNode->m_castShadow;

            fragmentShader.append("");
            char lightIdxStr[11];
            snprintf(lightIdxStr, 11, "%d", lightIdx);

            QByteArray lightVarPrefix = "light";
            lightVarPrefix.append(lightIdxStr);

            fragmentShader << "    //Light " << lightIdxStr << (isDirectional ? " [directional]" : isArea ? " [area]" : isSpot ? " [spot]" : " [point]") << "\n";
            fragmentShader << "    qt_lightAttenuation = 1.0;\n";

            if (isDirectional) {
                generateShadowMapOcclusion(fragmentShader, vertexShader, lightIdx, castsShadow, lightNode->m_lightType, lightVarNames);
                fragmentShader << "    tmp_light_color = " << lightVarNames.lightColor << ".rgb * (1.0 - qt_metalnessAmount);\n";

                if (hasCustomFrag && hasCustomFunction(QByteArrayLiteral("qt_directionalLightProcessor"))) {
                    // DIFFUSE, LIGHT_COLOR, SHADOW_CONTRIB, TO_LIGHT_DIR, NORMAL, BASE_COLOR
                    fragmentShader << "    qt_directionalLightProcessor(global_diffuse_light.rgb, tmp_light_color, qt_shadow_map_occl, -"
                                   << lightVarNames.lightDirection << ".xyz, qt_world_normal, qt_customBaseColor);\n";
                } else {
                    fragmentShader << "    global_diffuse_light.rgb += qt_diffuseColor.rgb * qt_shadow_map_occl * qt_diffuseReflectionBSDF(qt_world_normal, -"
                                   << lightVarNames.lightDirection << ".xyz, tmp_light_color).rgb;\n";
                }

                if (hasCustomFrag && hasCustomFunction(QByteArrayLiteral("qt_specularLightProcessor"))) {
                    // SPECULAR, LIGHT_COLOR, LIGHT_ATTENUATION, SHADOW_CONTRIB, TO_LIGHT_DIR, NORMAL, BASE_COLOR
                    fragmentShader << "    qt_specularLightProcessor(global_specular_light.rgb, " << lightVarNames.lightSpecularColor << ".rgb, 1.0, qt_shadow_map_occl, -"
                                   << lightVarNames.lightDirection << ".xyz, qt_world_normal, qt_customBaseColor);\n";
                } else {
                    if (specularLightingEnabled)
                        outputSpecularEquation(materialAdapter->specularModel(), fragmentShader, lightVarNames.lightDirection, lightVarNames.lightSpecularColor);
                }
            } else if (isArea) {
                fragmentShader.addFunction("calculateDiffuseAreaOld");
                vertexShader.generateWorldPosition();
                generateShadowMapOcclusion(fragmentShader, vertexShader, lightIdx, castsShadow, lightNode->m_lightType, lightVarNames);

                lightVarNames.normalizedDirection = lightVarPrefix;
                lightVarNames.normalizedDirection.append("_Frame");

                addLocalVariable(fragmentShader, lightVarNames.normalizedDirection, "mat3");
                fragmentShader << "    " << lightVarNames.normalizedDirection << " = mat3(" << lightVarNames.lightRt << ".xyz, " << lightVarNames.lightUp << ".xyz, -" << lightVarNames.lightDirection << ".xyz);\n";

                if (hasCustomFrag && hasCustomFunction(QByteArrayLiteral("qt_specularLightProcessor"))) {
                    // SPECULAR, LIGHT_COLOR, LIGHT_ATTENUATION, SHADOW_CONTRIB, TO_LIGHT_DIR, NORMAL, BASE_COLOR
                    fragmentShader << "    qt_specularLightProcessor(global_specular_light.rgb, " << lightVarNames.lightSpecularColor << ".rgb, qt_lightAttenuation, qt_shadow_map_occl, -"
                                   << lightVarNames.lightDirection << ".xyz, qt_world_normal, qt_customBaseColor);\n";
                } else {
                    if (specularLightingEnabled) {
                        vertexShader.generateViewVector();
                        outputSpecularAreaLighting(fragmentShader, "qt_varWorldPos", "qt_view_vector", lightVarNames.lightSpecularColor, lightVarNames);
                    }
                }

                lightVarNames.normalizedDirection = lightVarPrefix + "_areaDir";
                addLocalVariable(fragmentShader, lightVarNames.normalizedDirection, "vec3");
                fragmentShader << "    qt_lightAttenuation = qt_calculateDiffuseAreaOld(" << lightVarNames.lightDirection << ".xyz, "
                               << lightVarNames.lightPos << ".xyz, "
                               << lightVarNames.lightUp << ", "
                               << lightVarNames.lightRt << ", "
                               << "qt_varWorldPos, "
                               << lightVarNames.normalizedDirection << ");\n";

                addTranslucencyIrradiance(fragmentShader, translucencyImage, true, lightVarNames);
                fragmentShader << "    tmp_light_color = " << lightVarNames.lightColor << ".rgb * (1.0 - qt_metalnessAmount);\n";
                if (hasCustomFrag && hasCustomFunction(QByteArrayLiteral("qt_areaLightProcessor"))) {
                    // DIFFUSE, LIGHT_COLOR, LIGHT_ATTENUATION, SHADOW_CONTRIB, TO_LIGHT_DIR, NORMAL, BASE_COLOR
                    fragmentShader << "    qt_areaLightProcessor(global_diffuse_light.rgb, tmp_light_color, qt_lightAttenuation, qt_shadow_map_occl, "
                                   << lightVarNames.normalizedDirection << ".xyz, qt_world_normal, qt_customBaseColor);\n";
                } else {
                    fragmentShader << "    global_diffuse_light.rgb += qt_diffuseColor.rgb * qt_lightAttenuation * qt_shadow_map_occl * qt_diffuseReflectionBSDF(qt_world_normal, "
                                   << lightVarNames.normalizedDirection << ", tmp_light_color).rgb;\n";
                }
            } else {
                vertexShader.generateWorldPosition();

                lightVarNames.relativeDirection = lightVarPrefix;
                lightVarNames.relativeDirection.append("_relativeDirection");

                lightVarNames.normalizedDirection = lightVarNames.relativeDirection;
                lightVarNames.normalizedDirection.append("_normalized");

                lightVarNames.relativeDistance = lightVarPrefix;
                lightVarNames.relativeDistance.append("_distance");

                fragmentShader << "    vec3 " << lightVarNames.relativeDirection << " = qt_varWorldPos - " << lightVarNames.lightPos << ".xyz;\n"
                               << "    float " << lightVarNames.relativeDistance << " = length(" << lightVarNames.relativeDirection << ");\n"
                               << "    vec3 " << lightVarNames.normalizedDirection << " = " << lightVarNames.relativeDirection << " / " << lightVarNames.relativeDistance << ";\n";

                if (isSpot) {
                    lightVarNames.spotAngle = lightVarPrefix;
                    lightVarNames.spotAngle.append("_spotAngle");

                    fragmentShader << "    float " << lightVarNames.spotAngle << " = dot(" << lightVarNames.normalizedDirection
                                   << ", normalize(vec3(" << lightVarNames.lightDirection << ")));\n";
                    fragmentShader << "    if (" << lightVarNames.spotAngle << " > " << lightVarNames.lightConeAngle << ") {\n";
                }

                generateShadowMapOcclusion(fragmentShader, vertexShader, lightIdx, castsShadow, lightNode->m_lightType, lightVarNames);

                fragmentShader.addFunction("calculatePointLightAttenuation");

                fragmentShader << "    qt_lightAttenuation = qt_calculatePointLightAttenuation(vec3("
                               << lightVarNames.lightConstantAttenuation << ", " << lightVarNames.lightLinearAttenuation << ", "
                               << lightVarNames.lightQuadraticAttenuation << "), " << lightVarNames.relativeDistance << ");\n";

                addTranslucencyIrradiance(fragmentShader, translucencyImage, false, lightVarNames);
                fragmentShader << "    tmp_light_color = " << lightVarNames.lightColor << ".rgb * (1.0 - qt_metalnessAmount);\n";

                if (isSpot) {
                    fragmentShader << "    float spotFactor = smoothstep(" << lightVarNames.lightConeAngle
                                   << ", " << lightVarNames.lightInnerConeAngle << ", " << lightVarNames.spotAngle
                                   << ");\n";
                    if (hasCustomFrag && hasCustomFunction(QByteArrayLiteral("qt_spotLightProcessor"))) {
                        // DIFFUSE, LIGHT_COLOR, LIGHT_ATTENUATION, SPOT_FACTOR, SHADOW_CONTRIB, TO_LIGHT_DIR, NORMAL, BASE_COLOR
                        fragmentShader << "    qt_spotLightProcessor(global_diffuse_light.rgb, tmp_light_color, qt_lightAttenuation, spotFactor, qt_shadow_map_occl, -"
                                       << lightVarNames.normalizedDirection << ".xyz, qt_world_normal, qt_customBaseColor);\n";
                                                           } else {
                        fragmentShader << "    global_diffuse_light.rgb += qt_diffuseColor.rgb * spotFactor * qt_lightAttenuation * qt_shadow_map_occl * qt_diffuseReflectionBSDF(qt_world_normal, -"
                                       << lightVarNames.normalizedDirection << ", tmp_light_color).rgb;\n";
                    }

                } else {
                    // point light
                    if (hasCustomFrag && hasCustomFunction(QByteArrayLiteral("qt_pointLightProcessor"))) {
                        // DIFFUSE, LIGHT_COLOR, LIGHT_ATTENUATION, SHADOW_CONTRIB, TO_LIGHT_DIR, NORMAL, BASE_COLOR
                        fragmentShader << "    qt_pointLightProcessor(global_diffuse_light.rgb, tmp_light_color, qt_lightAttenuation, qt_shadow_map_occl, -"
                                       << lightVarNames.normalizedDirection << ".xyz, qt_world_normal, qt_customBaseColor);\n";
                    } else {
                        fragmentShader << "    global_diffuse_light.rgb += qt_diffuseColor.rgb * qt_lightAttenuation * qt_shadow_map_occl * qt_diffuseReflectionBSDF(qt_world_normal, -"
                                       << lightVarNames.normalizedDirection << ", tmp_light_color).rgb;\n";
                    }
                }

                if (hasCustomFrag && hasCustomFunction(QByteArrayLiteral("qt_specularLightProcessor"))) {
                    // SPECULAR, LIGHT_COLOR, LIGHT_ATTENUATION, SHADOW_CONTRIB, TO_LIGHT_DIR, NORMAL, BASE_COLOR
                    fragmentShader << "    qt_specularLightProcessor(global_specular_light.rgb, " << lightVarNames.lightSpecularColor << ".rgb, qt_lightAttenuation, qt_shadow_map_occl, -"
                                   << lightVarNames.normalizedDirection << ".xyz, qt_world_normal, qt_customBaseColor);\n";
                } else {
                    if (specularLightingEnabled)
                        outputSpecularEquation(materialAdapter->specularModel(), fragmentShader, lightVarNames.normalizedDirection, lightVarNames.lightSpecularColor);
                }

                if (isSpot)
                    fragmentShader << "    }\n";
            }
        }
        if (!lights.isEmpty())
            fragmentShader.append("");

        // The color in rgb is ready, including shadowing, just need to apply
        // the ambient occlusion factor. The alpha is the model opacity
        // multiplied by the alpha from the material color and/or the vertex colors.
        fragmentShader << "    global_diffuse_light = vec4(global_diffuse_light.rgb * qt_aoFactor, qt_objectOpacity * qt_diffuseColor.a);\n";

        if (!hasEmissiveMap) {
            if (hasCustomFrag) {
                // Cannot have a (built-in) emissive map with a custom
                // material, but it can provide a custom value for the emissive
                // color in MAIN. Otherwise the default vec3(0.0) will cause
                // this to do nothing effectively.
                fragmentShader << "    global_diffuse_light.rgb += qt_diffuseColor.rgb * qt_customEmissiveColor;\n";
            } else {
                fragmentShader << "    global_diffuse_light.rgb += qt_diffuseColor.rgb * qt_material_emissive_color;\n";
            }
        }

        // since we already modulate our material diffuse color
        // into the light color we will miss it entirely if no IBL
        // or light is used
        if (hasLightmaps && !(lights.size() || hasIblProbe))
            fragmentShader << "    global_diffuse_light.rgb *= qt_diffuseColor.rgb;\n";

        if (hasIblProbe) {
            vertexShader.generateWorldNormal(inKey);

            fragmentShader << "    global_diffuse_light.rgb += qt_diffuseColor.rgb * qt_aoFactor * qt_sampleDiffuse(qt_tanFrame).rgb;\n";

            if (specularLightingEnabled) {
                fragmentShader << "    global_specular_light.rgb += qt_specularAmount * qt_specularTint * "
                                  "qt_sampleGlossy(qt_tanFrame, qt_view_vector, qt_roughnessAmount).rgb;\n";
            }
        }

        if (hasImage) {
            fragmentShader.append("    vec4 qt_texture_color;");
            for (QSSGRenderableImage *image = firstImage; image; image = image->m_nextImage) {
                // Various maps are handled on a different locations
                if (image->m_mapType == QSSGRenderableImage::Type::Bump || image->m_mapType == QSSGRenderableImage::Type::Normal
                        || image->m_mapType == QSSGRenderableImage::Type::SpecularAmountMap
                        || image->m_mapType == QSSGRenderableImage::Type::Roughness || image->m_mapType == QSSGRenderableImage::Type::Translucency
                        || image->m_mapType == QSSGRenderableImage::Type::Metalness || image->m_mapType == QSSGRenderableImage::Type::Occlusion
                        || image->m_mapType == QSSGRenderableImage::Type::LightmapIndirect
                        || image->m_mapType == QSSGRenderableImage::Type::LightmapRadiosity) {
                    continue;
                }

                QByteArray texSwizzle;
                QByteArray lookupSwizzle;

                const bool hasIdentityMap = identityImages.contains(image);
                if (hasIdentityMap)
                    generateImageUVSampler(vertexShader, fragmentShader, inKey, *image, imageFragCoords, image->m_image.m_indexUV);
                else
                    generateImageUVCoordinates(vertexShader, fragmentShader, inKey, *image, image->m_image.m_indexUV);

                //                if (image->m_image.m_textureData.m_texture) {
                //                    // not supported for rhi
                //                    generateTextureSwizzle(image->m_image.m_textureData.m_texture->textureSwizzleMode(), texSwizzle, lookupSwizzle);
                //                }

                const auto &names = imageStringTable[int(image->m_mapType)];
                fragmentShader << "    qt_texture_color" << texSwizzle << " = texture2D(" << names.imageSampler
                               << ", " << (hasIdentityMap ? imageFragCoords : names.imageFragCoords) << ")" << lookupSwizzle << ";\n";

                if (image->m_image.m_textureData.m_textureFlags.isPreMultiplied())
                    fragmentShader << "    qt_texture_color.rgb = qt_texture_color.a > 0.0 ? qt_texture_color.rgb / qt_texture_color.a : vec3(0.0);\n";

                switch (image->m_mapType) {
                case QSSGRenderableImage::Type::BaseColor:
                    // color already taken care of
                    if (materialAdapter->alphaMode() == QSSGRenderDefaultMaterial::MaterialAlphaMode::Mask) {
                        // Apply the cutoff test. This matches the behavior documented in PrincipledMaterial.alphaMode.
                        fragmentShader.addUniform("qt_alphaCutoff", "float");
                        fragmentShader << "    if ((qt_texture_color.a * qt_material_base_color.a) < qt_alphaCutoff) {\n"
                                          "        fragOutput = vec4(0);\n"
                                          "        return;\n"
                                          "    }\n";
                    }
                    break;
                case QSSGRenderableImage::Type::Diffuse: // assume images are premultiplied.
                    // color already taken care of
                    fragmentShader.append("    global_diffuse_light.a *= qt_material_base_color.a * qt_texture_color.a;");
                    break;
                case QSSGRenderableImage::Type::LightmapShadow:
                    // We use image offsets.z to switch between incoming premultiplied textures or
                    // not premultiplied textures.
                    // If Z is 1, then we assume the incoming texture is already premultiplied, else
                    // we just read the rgb value.
                    fragmentShader.append("    global_diffuse_light *= qt_texture_color;");
                    break;
                case QSSGRenderableImage::Type::Specular:
                    if (fragmentHasSpecularAmount)
                        fragmentShader.append("    global_specular_light.rgb += qt_specularAmount * qt_texture_color.rgb * qt_specularTint;");
                    else
                        fragmentShader.append("    global_specular_light.rgb += qt_texture_color.rgb * qt_specularTint;");
                    fragmentShader.append("    global_diffuse_light.a *= qt_texture_color.a;");
                    break;
                case QSSGRenderableImage::Type::Opacity:
                {
                    const auto &channelProps = keyProps.m_textureChannels[QSSGShaderDefaultMaterialKeyProperties::OpacityChannel];
                    fragmentShader << "    global_diffuse_light.a *= qt_texture_color" << channelStr(channelProps, inKey) << ";\n";
                    break;
                }
                case QSSGRenderableImage::Type::Emissive:
                    fragmentShader.append("    qt_global_emission *= qt_texture_color.rgb * qt_texture_color.a;");
                    break;
                default:
                    Q_ASSERT(false); // fallthrough intentional
                }
            }
        }

        // Occlusion Map
        if (occlusionImage) {
            fragmentShader.addUniform("qt_occlusionAmount", "float");
            const auto &channelProps = keyProps.m_textureChannels[QSSGShaderDefaultMaterialKeyProperties::OcclusionChannel];
            const bool hasIdentityMap = identityImages.contains(occlusionImage);
            if (hasIdentityMap)
                generateImageUVSampler(vertexShader, fragmentShader, inKey, *occlusionImage, imageFragCoords, occlusionImage->m_image.m_indexUV);
            else
                generateImageUVCoordinates(vertexShader, fragmentShader, inKey, *occlusionImage, occlusionImage->m_image.m_indexUV);
            const auto &names = imageStringTable[int(QSSGRenderableImage::Type::Occlusion)];
            fragmentShader << "    float qt_ao = texture2D(" << names.imageSampler << ", "
                           << (hasIdentityMap ? imageFragCoords : names.imageFragCoords) << ")" << channelStr(channelProps, inKey) << ";\n";
            fragmentShader << "    global_diffuse_light.rgb = mix(global_diffuse_light.rgb, global_diffuse_light.rgb * qt_ao, qt_occlusionAmount);\n";
        }

        if (hasEmissiveMap)
            fragmentShader.append("    global_diffuse_light.rgb += qt_global_emission.rgb;");

        if (hasBaseColorMap && specularLightingEnabled) {
            fragmentShader.addInclude("luminance.glsllib");
            fragmentShader << "    float qt_lum = qt_luminance(qt_specularBase);\n"
                              "    global_specular_light.rgb *= (qt_lum > 0.0) ? qt_specularBase / qt_lum : vec3(1.0);\n";
        }

        Q_ASSERT(!isDepthPass && !isOrthoShadowPass && !isCubeShadowPass);
        fragmentShader.addInclude("tonemapping.glsllib");
        fragmentShader.append("    fragOutput = vec4(qt_tonemap(global_diffuse_light.rgb + global_specular_light.rgb), global_diffuse_light.a);");
    } else {
        if (isOrthoShadowPass) {
            fragmentShader.addUniform("qt_shadowDepthAdjust", "vec2");
            fragmentShader << "    // directional shadow pass\n"
                           << "    float qt_shadowDepth = (qt_varDepth + qt_shadowDepthAdjust.x) * qt_shadowDepthAdjust.y;\n"
                           << "    fragOutput = vec4(qt_shadowDepth);\n";
        } else if (isCubeShadowPass) {
            fragmentShader.addUniform("qt_cameraPosition", "vec3");
            fragmentShader.addUniform("qt_cameraProperties", "vec2");
            fragmentShader << "    // omnidirectional shadow pass\n"
                           << "    vec3 qt_shadowCamPos = vec3(qt_cameraPosition.x, qt_cameraPosition.y, -qt_cameraPosition.z);\n"
                           << "    float qt_shadowDist = length(qt_varShadowWorldPos - qt_shadowCamPos);\n"
                           << "    qt_shadowDist = (qt_shadowDist - qt_cameraProperties.x) / (qt_cameraProperties.y - qt_cameraProperties.x);\n"
                           << "    fragOutput = vec4(qt_shadowDist, qt_shadowDist, qt_shadowDist, 1.0);\n";
        } else {
            fragmentShader.addInclude("tonemapping.glsllib");
            fragmentShader.append("    fragOutput = vec4(qt_tonemap(qt_diffuseColor.rgb), qt_diffuseColor.a * qt_objectOpacity);");
        }
    }
}

QSSGRef<QSSGRhiShaderStages> QSSGMaterialShaderGenerator::generateMaterialRhiShader(const QByteArray &inShaderKeyPrefix,
                                                                                    QSSGMaterialVertexPipeline &vertexPipeline,
                                                                                    const QSSGShaderDefaultMaterialKey &key,
                                                                                    QSSGShaderDefaultMaterialKeyProperties &inProperties,
                                                                                    const ShaderFeatureSetList &inFeatureSet,
                                                                                    const QSSGRenderGraphObject &inMaterial,
                                                                                    const QSSGShaderLightList &inLights,
                                                                                    QSSGRenderableImage *inFirstImage,
                                                                                    const QSSGRef<QSSGShaderLibraryManager> &shaderLibraryManager,
                                                                                    const QSSGRef<QSSGShaderCache> &theCache)
{
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

    return vertexPipeline.programGenerator()->compileGeneratedRhiShader(materialInfoString, inFeatureSet, shaderLibraryManager, theCache);
}

void QSSGMaterialShaderGenerator::setRhiImageShaderVariables(const QSSGRef<QSSGRhiShaderStagesWithResources> &inShader, QSSGRenderableImage &inImage, quint32 idx)
{
    const QMatrix4x4 &textureTransform = inImage.m_image.m_textureTransform;
    // We separate rotational information from offset information so that just maybe the shader
    // will attempt to push less information to the card.
    const float *dataPtr(textureTransform.constData());
    // The third member of the offsets contains a flag indicating if the texture was
    // premultiplied or not.
    // We use this to mix the texture alpha.
    QVector3D offsets(dataPtr[12], dataPtr[13], inImage.m_image.m_textureData.m_textureFlags.isPreMultiplied() ? 1.0f : 0.0f);
    // Grab just the upper 2x2 rotation matrix from the larger matrix.
    QVector4D rotations(dataPtr[0], dataPtr[4], dataPtr[1], dataPtr[5]);

    // we need to map image to uniform name: "image0_rotations", "image0_offsets", etc...
    const auto &names = imageStringTable[int(inImage.m_mapType)];
    QSSGRhiShaderStagesWithResources::CommonUniformIndices &cui = inShader->commonUniformIndices;
    auto &indices = cui.imageIndices[idx];

    indices.imageRotationsUniformIndex = inShader->setUniform(names.imageRotations, &rotations, sizeof(rotations), indices.imageRotationsUniformIndex);
    indices.imageOffsetsUniformIndex = inShader->setUniform(names.imageOffsets, &offsets, sizeof(offsets), indices.imageOffsetsUniformIndex);
}

void QSSGMaterialShaderGenerator::setRhiMaterialProperties(const QSSGRenderContextInterface &renderContext,
                                                           QSSGRef<QSSGRhiShaderStagesWithResources> &shaders,
                                                           QSSGRhiGraphicsPipelineState *inPipelineState,
                                                           const QSSGRenderGraphObject &inMaterial,
                                                           QSSGRenderCamera &inCamera,
                                                           const QMatrix4x4 &inModelViewProjection,
                                                           const QMatrix3x3 &inNormalMatrix,
                                                           const QMatrix4x4 &inGlobalTransform,
                                                           const QMatrix4x4 &clipSpaceCorrMatrix,
                                                           const QSSGDataView<QMatrix4x4> &inBoneGlobals,
                                                           const QSSGDataView<QMatrix3x3> &inBoneNormals,
                                                           QSSGRenderableImage *inFirstImage,
                                                           float inOpacity,
                                                           const QSSGLayerGlobalRenderProperties &inRenderProperties,
                                                           const QSSGShaderLightList &inLights,
                                                           bool receivesShadows,
                                                           const QVector2D &shadowDepthAdjust)
{
    QSSGShaderMaterialAdapter *materialAdapter = getMaterialAdapter(inMaterial);
    QSSGRhiShaderStagesWithResources::CommonUniformIndices& cui = shaders->commonUniformIndices;

    materialAdapter->setCustomPropertyUniforms(shaders, renderContext);

    const QVector3D camGlobalPos = inCamera.getGlobalPos();
    const QVector2D camProperties(inCamera.clipNear, inCamera.clipFar);

    cui.cameraPositionIdx = shaders->setUniform(QByteArrayLiteral("qt_cameraPosition"), &camGlobalPos, 3 * sizeof(float), cui.cameraPositionIdx);
    cui.cameraDirectionIdx = shaders->setUniform(QByteArrayLiteral("qt_cameraDirection"), &inRenderProperties.cameraDirection, 3 * sizeof(float), cui.cameraDirectionIdx);
    cui.cameraPropertiesIdx = shaders->setUniform(QByteArrayLiteral("qt_cameraProperties"), &camProperties, 2 * sizeof(float), cui.cameraPropertiesIdx);

    QMatrix4x4 viewProj;
    inCamera.calculateViewProjectionMatrix(viewProj);
    viewProj = clipSpaceCorrMatrix * viewProj;
    cui.viewProjectionMatrixIdx = shaders->setUniform(QByteArrayLiteral("qt_viewProjectionMatrix"), viewProj.constData(), 16 * sizeof(float), cui.viewProjectionMatrixIdx);
    // Projection Matrices are only needed by CustomMaterial shaders
    if (inMaterial.type == QSSGRenderGraphObject::Type::CustomMaterial) {
        const auto *customMaterial = static_cast<const QSSGRenderCustomMaterial *>(&inMaterial);
        const bool usesProjectionMatrix = customMaterial->m_renderFlags.testFlag(QSSGRenderCustomMaterial::RenderFlag::ProjectionMatrix);
        const bool usesInvProjectionMatrix = customMaterial->m_renderFlags.testFlag(QSSGRenderCustomMaterial::RenderFlag::InverseProjectionMatrix);

        if (usesProjectionMatrix || usesInvProjectionMatrix) {
            const QMatrix4x4 projection = clipSpaceCorrMatrix * inCamera.projection;
            if (usesProjectionMatrix)
                cui.projectionMatrixIdx = shaders->setUniform(QByteArrayLiteral("qt_projectionMatrix"), projection.constData(), 16 * sizeof(float), cui.projectionMatrixIdx);
            if (usesInvProjectionMatrix)
                cui.inverseProjectionMatrixIdx = shaders->setUniform(QByteArrayLiteral("qt_inverseProjectionMatrix"), projection.inverted().constData(), 16 * sizeof (float), cui.inverseProjectionMatrixIdx);
        }
    }
    const QMatrix4x4 viewMatrix = inCamera.globalTransform.inverted();
    cui.viewMatrixIdx = shaders->setUniform(QByteArrayLiteral("qt_viewMatrix"), viewMatrix.constData(), 16 * sizeof(float), cui.viewMatrixIdx);

    // Skinning
    cui.boneTransformsIdx = shaders->setUniformArray(QByteArrayLiteral("qt_boneTransforms"), inBoneGlobals.mData, inBoneGlobals.mSize, QSSGRenderShaderDataType::Matrix4x4, cui.boneTransformsIdx);
    cui.boneNormalTransformsIdx = shaders->setUniformArray(QByteArrayLiteral("qt_boneNormalTransforms"), inBoneNormals.mData, inBoneNormals.mSize, QSSGRenderShaderDataType::Matrix3x3, cui.boneNormalTransformsIdx);

    // In D3D, Vulkan and Metal Y points down and the origin is
    // top-left in the viewport coordinate system. OpenGL is
    // bottom-left and Y up. This happens to match the framebuffer
    // coordinate system with all APIs so we rely on that query.
    // The winding order is calculated in window space so the
    // double-sided logic in the shader needs to take this into account.
    // (normally the correction matrix we multiply into the projection
    // takes care of getting identical behavior regardless of the
    // underlying API, but here it matters since we kind of take things
    // into our own hands)
    float normalVpFactor = inRenderProperties.isYUpInFramebuffer ? 1.0f : -1.0f;
    cui.normalAdjustViewportFactorIdx = shaders->setUniform(QByteArrayLiteral("qt_normalAdjustViewportFactor"), &normalVpFactor, sizeof(float), cui.normalAdjustViewportFactorIdx);

    const float isClipDepthZeroToOne = inRenderProperties.isClipDepthZeroToOne ? 0.0f : -1.0f;
    cui.isClipDepthZeroToOneIdx = shaders->setUniform(QByteArrayLiteral("qt_nearClipValue"), &isClipDepthZeroToOne, sizeof(float), cui.isClipDepthZeroToOneIdx);

    QVector3D theLightAmbientTotal = QVector3D(0, 0, 0);
    shaders->resetLights(QSSGRhiShaderStagesWithResources::LightBuffer0);
    shaders->resetShadowMaps();

    float zero[16];
    memset(zero, 0, sizeof(zero));

    for (quint32 lightIdx = 0, shadowMapIdx = 0, lightEnd = inLights.size();
         lightIdx < lightEnd && lightIdx < QSSG_MAX_NUM_LIGHTS; ++lightIdx)
    {
        QSSGRenderLight *theLight(inLights[lightIdx].light);
        QSSGShaderLightProperties &theLightProperties(shaders->addLight(QSSGRhiShaderStagesWithResources::LightBuffer0));
        float brightness = aux::translateBrightness(theLight->m_brightness);

        theLightProperties.lightColor = theLight->m_diffuseColor * brightness;
        theLightProperties.lightData.specular = QVector4D(theLight->m_specularColor * brightness, 1.0);
        theLightProperties.lightData.direction = QVector4D(inLights[lightIdx].direction, 1.0);

        // When it comes to receivesShadows, it is a bit tricky: to stay
        // compatible with the old, direct OpenGL rendering path (and the
        // generated shader code), we will need to ensure the texture
        // (shadowmap0, shadowmap1, ...) and sampler bindings are present.
        // So receivesShadows must not be included in the following
        // condition. Instead, it is the other shadow-related uniforms that
        // get an all-zero value, which then ensures no shadow contribution
        // for the object in question.

        if (theLight->m_castShadow && !theLight->m_scope && shadowMapIdx < (QSSG_MAX_NUM_SHADOWS_PER_TYPE * QSSG_SHADOW_MAP_TYPE_COUNT)) {
            QSSGRhiShadowMapProperties &theShadowMapProperties(shaders->addShadowMap());
            ++shadowMapIdx;

            QSSGShadowMapEntry *pEntry = inRenderProperties.shadowMapManager->getShadowMapEntry(lightIdx);
            Q_ASSERT(pEntry);

            const auto names = setupShadowMapVariableNames(lightIdx);

            if (theLight->m_lightType != QSSGRenderLight::Type::Directional) {
                theShadowMapProperties.shadowMapTexture = pEntry->m_rhiDepthCube;
                theShadowMapProperties.shadowMapTextureUniformName = names.shadowCubeStem;
                if (receivesShadows)
                    shaders->setUniform(names.shadowMatrixStem, pEntry->m_lightView.constData(), 16 * sizeof(float));
                else
                    shaders->setUniform(names.shadowMatrixStem, zero, 16 * sizeof(float));
            } else {
                theShadowMapProperties.shadowMapTexture = pEntry->m_rhiDepthMap;
                theShadowMapProperties.shadowMapTextureUniformName = names.shadowMapStem;
                if (receivesShadows) {
                    // add fixed scale bias matrix
                    const QMatrix4x4 bias = {
                        0.5, 0.0, 0.0, 0.5,
                        0.0, 0.5, 0.0, 0.5,
                        0.0, 0.0, 0.5, 0.5,
                        0.0, 0.0, 0.0, 1.0 };
                    const QMatrix4x4 m = bias * pEntry->m_lightVP;
                    shaders->setUniform(names.shadowMatrixStem, m.constData(), 16 * sizeof(float));
                } else {
                    shaders->setUniform(names.shadowMatrixStem, zero, 16 * sizeof(float));
                }
            }

            if (receivesShadows) {
                const QVector4D shadowControl(theLight->m_shadowBias,
                                              theLight->m_shadowFactor,
                                              theLight->m_shadowMapFar,
                                              inRenderProperties.isYUpInFramebuffer ? 0.0f : 1.0f);
                shaders->setUniform(names.shadowControlStem, &shadowControl, 4 * sizeof(float));
            } else {
                shaders->setUniform(names.shadowControlStem, zero, 4 * sizeof(float));
            }
        }

        if (theLight->m_lightType == QSSGRenderLight::Type::Point
                || theLight->m_lightType == QSSGRenderLight::Type::Spot) {
            theLightProperties.lightData.position = QVector4D(theLight->getGlobalPos(), 1.0);
            theLightProperties.lightData.constantAttenuation = aux::translateConstantAttenuation(theLight->m_constantFade);
            theLightProperties.lightData.linearAttenuation = aux::translateLinearAttenuation(theLight->m_linearFade);
            theLightProperties.lightData.quadraticAttenuation = aux::translateQuadraticAttenuation(theLight->m_quadraticFade);
            theLightProperties.lightData.coneAngle = 180.0f;
            if (theLight->m_lightType == QSSGRenderLight::Type::Spot) {
                theLightProperties.lightData.coneAngle
                        = qCos(qDegreesToRadians(theLight->m_coneAngle));
                float innerConeAngle = theLight->m_innerConeAngle;
                if (theLight->m_innerConeAngle < 0)
                    innerConeAngle = theLight->m_coneAngle * 0.7f;
                else if (theLight->m_innerConeAngle > theLight->m_coneAngle)
                    innerConeAngle = theLight->m_coneAngle;
                theLightProperties.lightData.innerConeAngle
                        = qCos(qDegreesToRadians(innerConeAngle));
            }
        } else if (theLight->m_lightType == QSSGRenderLight::Type::Area) {
            theLightProperties.lightData.position = QVector4D(theLight->getGlobalPos(), 1.0);

            QVector3D upDir = mat33::transform(mat44::getUpper3x3(theLight->globalTransform), QVector3D(0, 1, 0));
            QVector3D rtDir = mat33::transform(mat44::getUpper3x3(theLight->globalTransform), QVector3D(1, 0, 0));

            theLightProperties.lightData.up = QVector4D(upDir, theLight->m_areaHeight);
            theLightProperties.lightData.right = QVector4D(rtDir, theLight->m_areaWidth);
        }
        theLightAmbientTotal += theLight->m_ambientColor;
    }

    cui.shadowDepthAdjustIdx = shaders->setUniform(QByteArrayLiteral("qt_shadowDepthAdjust"), &shadowDepthAdjust, 2 * sizeof(float), cui.shadowDepthAdjustIdx);

    const QMatrix4x4 mvp = clipSpaceCorrMatrix * inModelViewProjection;
    cui.modelViewProjectionIdx = shaders->setUniform(QByteArrayLiteral("qt_modelViewProjection"), mvp.constData(), 16 * sizeof(float), cui.modelViewProjectionIdx);

    // mat3 is still 4 floats per column in the uniform buffer (but there
    // is no 4th column), so 48 bytes altogether, not 36 or 64.
    float normalMatrix[12];
    memcpy(normalMatrix, inNormalMatrix.constData(), 3 * sizeof(float));
    memcpy(normalMatrix + 4, inNormalMatrix.constData() + 3, 3 * sizeof(float));
    memcpy(normalMatrix + 8, inNormalMatrix.constData() + 6, 3 * sizeof(float));
    cui.normalMatrixIdx = shaders->setUniform(QByteArrayLiteral("qt_normalMatrix"), normalMatrix, 12 * sizeof(float), cui.normalMatrixIdx);
    cui.modelMatrixIdx = shaders->setUniform(QByteArrayLiteral("qt_modelMatrix"), inGlobalTransform.constData(), 16 * sizeof(float), cui.modelMatrixIdx);

    shaders->setDepthTexture(inRenderProperties.rhiDepthTexture);
    shaders->setSsaoTexture(inRenderProperties.rhiSsaoTexture);
    shaders->setScreenTexture(inRenderProperties.rhiScreenTexture);

    QSSGRenderImage *theLightProbe = inRenderProperties.lightProbe;

    // If the material has its own IBL Override, we should use that image instead.
    QSSGRenderImage *materialIblProbe = materialAdapter->iblProbe();
    const bool hasIblProbe = materialIblProbe != nullptr;
    const bool useMaterialIbl = hasIblProbe && materialIblProbe->m_textureData.m_rhiTexture;
    if (useMaterialIbl)
        theLightProbe = materialIblProbe;

    if (theLightProbe && theLightProbe->m_textureData.m_rhiTexture) {
        QSSGRenderTextureCoordOp theHorzLightProbeTilingMode = theLightProbe->m_horizontalTilingMode; //###??? was QSSGRenderTextureCoordOp::Repeat;
        QSSGRenderTextureCoordOp theVertLightProbeTilingMode = theLightProbe->m_verticalTilingMode;
        const QMatrix4x4 &textureTransform = theLightProbe->m_textureTransform;
        // We separate rotational information from offset information so that just maybe the
        // shader
        // will attempt to push less information to the card.
        const float *dataPtr(textureTransform.constData());
        // The third member of the offsets contains a flag indicating if the texture was
        // premultiplied or not.
        // We use this to mix the texture alpha.

        // The fourth member claims to be the number of mipmaps, but the directGL code path
        // actually uses the maximum mipmap level
        // TODO: clean up this!
        int maxMipLevel = theLightProbe->m_textureData.m_mipmaps - 1;

        QVector4D offsets(dataPtr[12],
                dataPtr[13],
                theLightProbe->m_textureData.m_textureFlags.isPreMultiplied() ? 1.0f : 0.0f,
                float(maxMipLevel));

        // Grab just the upper 2x2 rotation matrix from the larger matrix.
        QVector4D rotations(dataPtr[0], dataPtr[4], dataPtr[1], dataPtr[5]);
        cui.lightProbeRotationIdx = shaders->setUniform(QByteArrayLiteral("qt_lightProbeRotation"), &rotations, 4 * sizeof(float), cui.lightProbeRotationIdx);
        cui.lightProbeOffsetIdx = shaders->setUniform(QByteArrayLiteral("qt_lightProbeOffset"), &offsets, 4 * sizeof(float), cui.lightProbeOffsetIdx);

        if (!materialIblProbe && inRenderProperties.probeFOV < 180.f) {
            QVector4D opts(0.01745329251994329547f * inRenderProperties.probeFOV, 0.0f, 0.0f, 0.0f);
            cui.lightProbeOptionsIdx = shaders->setUniform(QByteArrayLiteral("qt_lightProbeOptions"), &opts, 4 * sizeof(float), cui.lightProbeOptionsIdx);
        }

        if (!materialIblProbe && !inRenderProperties.probeOrientation.isIdentity())
            cui.lightProbeOrientationIdx = shaders->setUniform(QByteArrayLiteral("qt_lightProbeOrientation"), inRenderProperties.probeOrientation.constData(), 16 * sizeof(float), cui.lightProbeOrientationIdx);

        QVector4D props(0.0f, 0.0f, inRenderProperties.probeHorizon, inRenderProperties.probeExposure);
        cui.lightProbePropertiesIdx = shaders->setUniform(QByteArrayLiteral("qt_lightProbeProperties"), &props, 4 * sizeof(float), cui.lightProbePropertiesIdx);
        shaders->setLightProbeTexture(theLightProbe->m_textureData.m_rhiTexture, theHorzLightProbeTilingMode, theVertLightProbeTilingMode);
    } else {
        // no lightprobe
        QVector4D emptyProps(0.0f, 0.0f, -1.0f, 0.0f);
        cui.lightProbePropertiesIdx = shaders->setUniform(QByteArrayLiteral("qt_lightProbeProperties"), &emptyProps, 4 * sizeof(float), cui.lightProbePropertiesIdx);

        shaders->setLightProbeTexture(nullptr);
    }

    const QVector3D emissiveColor = materialAdapter->emissiveColor();
    cui.material_emissiveColorIdx = shaders->setUniform(QByteArrayLiteral("qt_material_emissive_color"), &emissiveColor, 3 * sizeof(float), cui.material_emissiveColorIdx);

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
    cui.material_baseColorIdx = shaders->setUniform(QByteArrayLiteral("qt_material_base_color"), &color, 4 * sizeof(float), cui.material_baseColorIdx);

    const float ior = materialAdapter->ior();
    QVector4D specularColor(specularTint, ior);
    cui.material_specularIdx = shaders->setUniform(QByteArrayLiteral("qt_material_specular"), &specularColor, 4 * sizeof(float), cui.material_specularIdx);
    const float fresnelPower = materialAdapter->fresnelPower();
    cui.fresnelPowerIdx = shaders->setUniform(QByteArrayLiteral("qt_fresnelPower"), &fresnelPower, sizeof(float), cui.fresnelPowerIdx);

    const QVector3D diffuse = color.toVector3D(); // metalnessAmount cannot be multiplied in here yet due to custom materials
    const bool hasLighting = materialAdapter->hasLighting();
    shaders->setLightsEnabled(QSSGRhiShaderStagesWithResources::LightBuffer0, hasLighting);
    if (hasLighting) {
        for (int idx = 0, end = shaders->lightCount(QSSGRhiShaderStagesWithResources::LightBuffer0); idx < end; ++idx) {
            QSSGShaderLightProperties &lightProp(shaders->lightAt(QSSGRhiShaderStagesWithResources::LightBuffer0, idx));
            lightProp.lightData.diffuse = QVector4D(lightProp.lightColor * diffuse, 1.0);
        }
    }

    const QVector3D diffuseLightAmbientTotal = theLightAmbientTotal * diffuse;
    cui.light_ambient_totalIdx = shaders->setUniform(QByteArrayLiteral("qt_light_ambient_total"), &diffuseLightAmbientTotal, 3 * sizeof(float), cui.light_ambient_totalIdx);

    const QVector4D materialProperties(materialAdapter->specularAmount(),
                                       materialAdapter->specularRoughness(),
                                       materialAdapter->metalnessAmount(),
                                       inOpacity);
    cui.material_propertiesIdx = shaders->setUniform(QByteArrayLiteral("qt_material_properties"), &materialProperties, 4 * sizeof(float), cui.material_propertiesIdx);

    const float bumpAmount = materialAdapter->bumpAmount();
    cui.bumpAmountIdx = shaders->setUniform(QByteArrayLiteral("qt_bumpAmount"), &bumpAmount, sizeof(float), cui.bumpAmountIdx);
    const float translucentFallOff = materialAdapter->translucentFallOff();
    cui.translucentFalloffIdx = shaders->setUniform(QByteArrayLiteral("qt_translucentFalloff"), &translucentFallOff, sizeof(float), cui.translucentFalloffIdx);
    const float diffuseLightWrap = materialAdapter->diffuseLightWrap();
    cui.diffuseLightWrapIdx = shaders->setUniform(QByteArrayLiteral("qt_diffuseLightWrap"), &diffuseLightWrap, sizeof(float), cui.diffuseLightWrapIdx);
    const float occlusionAmount = materialAdapter->occlusionAmount();
    cui.occlusionAmountIdx = shaders->setUniform(QByteArrayLiteral("qt_occlusionAmount"), &occlusionAmount, sizeof(float), cui.occlusionAmountIdx);
    const float alphaCutOff = materialAdapter->alphaCutOff();
    cui.alphaCutoffIdx = shaders->setUniform(QByteArrayLiteral("qt_alphaCutoff"), &alphaCutOff, sizeof(float), cui.alphaCutoffIdx);

    quint32 imageIdx = 0;
    for (QSSGRenderableImage *theImage = inFirstImage; theImage; theImage = theImage->m_nextImage, ++imageIdx)
        setRhiImageShaderVariables(shaders, *theImage, imageIdx);

    inPipelineState->lineWidth = materialAdapter->lineWidth();

    const float pointSize = materialAdapter->pointSize();
    cui.pointSizeIdx = shaders->setUniform(QByteArrayLiteral("qt_materialPointSize"), &pointSize, sizeof(float), cui.pointSizeIdx);
}

QT_END_NAMESPACE
