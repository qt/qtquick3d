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
    static constexpr const char* sampler() { return "u"#V"Map_sampler"; }\
    static constexpr const char* offsets() { return "u"#V"Map_offsets"; }\
    static constexpr const char* rotations() { return "u"#V"Map_rotations"; }\
    static constexpr const char* fragCoords1() { return "u"#V"Map_uv_coords1"; }\
    static constexpr const char* fragCoords2() { return "u"#V"Map_uv_coords2"; }\
    static constexpr const char* samplerSize() { return "u"#V"Map_size"; }\
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

void textureCoordVariableName(char (&outString)[13], quint8 uvSet)
{
    // For now, uvSet will be less than 2.
    // But this value will be verified in the setProperty function.
    Q_ASSERT(uvSet < 9);
    qstrncpy(outString, "varTexCoordX", 13);
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
    transform = "    uTransform = vec3(" + imageRotations + ".x, " + imageRotations + ".y, " + imageOffsets + ".x);\n";
    transform += "    vTransform = vec3(" + imageRotations + ".z, " + imageRotations + ".w, " + imageOffsets + ".y);\n";
    return transform;
}

static void generateImageUVCoordinates(QSSGVertexPipelineBase &vertexShader,
                                       QSSGStageGeneratorBase &fragmentShader,
                                       const QSSGShaderDefaultMaterialKey &key,
                                       QSSGRenderableImage &image,
                                       quint32 uvSet = 0)
{
    if (image.uvCoordsGenerated)
        return;

    const auto &names = imageStringTable[int(image.m_mapType)];
    char textureCoordName[13];
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
        vertexShader << "    vec2 " << names.imageFragCoordsTemp << " = getTransformedUVCoords(vec3(" << textureCoordName << ", 1.0), uTransform, vTransform);\n";
        if (image.m_image.m_textureData.m_textureFlags.isInvertUVCoords())
            vertexShader << "    " << names.imageFragCoordsTemp << ".y = 1.0 - " << names.imageFragCoordsTemp << ".y;\n";

        vertexShader.assignOutput(names.imageFragCoords, names.imageFragCoordsTemp);
    } else {
        fragmentShader.addUniform(names.imageOffsets, "vec3");
        fragmentShader.addUniform(names.imageRotations, "vec4");
        fragmentShader << uvTrans;
        vertexShader.generateEnvMapReflection(key);
        fragmentShader.addFunction("getTransformedUVCoords");
        fragmentShader << "    vec2 " << names.imageFragCoords << " = getTransformedUVCoords(environment_map_reflection, uTransform, vTransform);\n";
        if (image.m_image.m_textureData.m_textureFlags.isInvertUVCoords())
            fragmentShader << "    " << names.imageFragCoords << ".y = 1.0 - " << names.imageFragCoords << ".y;\n";
    }
    image.uvCoordsGenerated = true;
}

static void generateImageUVSampler(QSSGVertexPipelineBase &vertexGenerator,
                                   QSSGStageGeneratorBase &fragmentShader,
                                   const QSSGShaderDefaultMaterialKey &key,
                                   const QSSGRenderableImage &image,
                                   char (&outString)[13],
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
        fragmentShader.addInclude("defaultMaterialPhysGlossyBSDF.glsllib");
        fragmentShader.addUniform("material_specular", "vec4");
        fragmentShader << "    global_specular_light.rgb += lightAttenuation * specularAmount"
                          " * kggxGlossyDefaultMtl(world_normal, tangent, -" << inLightDir << ".xyz, view_vector, " << inLightSpecColor << ".rgb, vec3(material_specular.rgb), roughnessAmount).rgb;\n";
    } break;
    case QSSGRenderDefaultMaterial::MaterialSpecularModel::KWard: {
        fragmentShader.addInclude("defaultMaterialPhysGlossyBSDF.glsllib");
        fragmentShader.addUniform("material_specular", "vec4");
        fragmentShader << "    global_specular_light.rgb += lightAttenuation * specularAmount"
                          " * wardGlossyDefaultMtl(world_normal, tangent, -" << inLightDir << ".xyz, view_vector, " << inLightSpecColor << ".rgb, vec3(material_specular.rgb), roughnessAmount).rgb;\n";
    } break;
    default:
        fragmentShader.addFunction("specularBSDF");
        fragmentShader << "    global_specular_light.rgb += lightAttenuation * specularAmount"
                          " * specularBSDF(world_normal, -" << inLightDir << ".xyz, view_vector, " << inLightSpecColor << ".rgb, 2.56 / (roughnessAmount + 0.01)).rgb;\n";
        break;
    }
}

static void outputDiffuseAreaLighting(QSSGStageGeneratorBase &infragmentShader,
                                      const QByteArray &inPos,
                                      const QByteArray &inLightPrefix,
                                      QSSGMaterialShaderGenerator::LightVariableNames &lightVarNames)
{
    lightVarNames.normalizedDirection = inLightPrefix + "_areaDir";
    addLocalVariable(infragmentShader, lightVarNames.normalizedDirection, "vec3");
    infragmentShader << "    lightAttenuation = calculateDiffuseAreaOld(" << lightVarNames.lightDirection << ".xyz, "
                     << lightVarNames.lightPos << ".xyz, "
                     << lightVarNames.lightUp << ", "
                     << lightVarNames.lightRt << ", "
                     << inPos << ", "
                     << lightVarNames.normalizedDirection << ");\n";
}

static void outputSpecularAreaLighting(QSSGStageGeneratorBase &infragmentShader,
                                       const QByteArray &inPos,
                                       const QByteArray &inView,
                                       const QByteArray &inLightSpecColor,
                                       const QSSGMaterialShaderGenerator::LightVariableNames &lightVarNames)
{
    infragmentShader.addFunction("sampleAreaGlossyDefault");
    infragmentShader.addUniform("material_specular", "vec4");
    infragmentShader << "global_specular_light.rgb += " << inLightSpecColor << ".rgb * lightAttenuation * shadowFac * material_specular.rgb * specularAmount * sampleAreaGlossyDefault(tanFrame, "
                     << inPos << ", "
                     << lightVarNames.normalizedDirection << ", "
                     << lightVarNames.lightPos << ".xyz, "
                     << lightVarNames.lightRt << ".w, "
                     << lightVarNames.lightUp << ".w, "
                     << inView << ", roughnessAmount).rgb;\n";
}

static void addTranslucencyIrradiance(QSSGStageGeneratorBase &infragmentShader,
                                      QSSGRenderableImage *image,
                                      bool areaLight,
                                      const QSSGMaterialShaderGenerator::LightVariableNames &lightVarNames)
{
    if (image == nullptr)
        return;

    infragmentShader.addFunction("diffuseReflectionWrapBSDF");
    if (areaLight) {
        infragmentShader << "    global_diffuse_light.rgb += lightAttenuation * translucent_thickness_exp * diffuseReflectionWrapBSDF(-world_normal, "
                         << lightVarNames.normalizedDirection << ", "
                         << lightVarNames.lightColor << ".rgb, diffuseLightWrap).rgb;\n";
    } else {
        infragmentShader << "    global_diffuse_light.rgb += lightAttenuation * translucent_thickness_exp * diffuseReflectionWrapBSDF(-world_normal, -"
                         << lightVarNames.normalizedDirection << ", "
                         << lightVarNames.lightColor << ".rgb, diffuseLightWrap).rgb;\n";
    }
}

static QSSGMaterialShaderGenerator::ShadowVariableNames setupShadowMapVariableNames(size_t lightIdx)
{
    QSSGMaterialShaderGenerator::ShadowVariableNames names;
    names.shadowMapStem = QByteArrayLiteral("shadowmap");
    names.shadowCubeStem = QByteArrayLiteral("shadowcube");
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

static void addShadowMapContribution(QSSGStageGeneratorBase &inLightShader,
                                     quint32 lightIndex,
                                     QSSGRenderLight::Type inType,
                                     const QSSGMaterialShaderGenerator::LightVariableNames &lightVarNames)
{
    const auto names = setupShadowMapVariableNames(lightIndex);

    inLightShader.addInclude("shadowMapping.glsllib");
    if (inType == QSSGRenderLight::Type::Directional) {
        inLightShader.addUniform(names.shadowMapStem, "sampler2D");
    } else {
        inLightShader.addUniform(names.shadowCubeStem, "samplerCube");
    }
    inLightShader.addUniform(names.shadowControlStem, "vec4");
    inLightShader.addUniform(names.shadowMatrixStem, "mat4");

    if (inType != QSSGRenderLight::Type::Directional) {
        inLightShader << "    shadow_map_occl = sampleCubemap(" << names.shadowCubeStem << ", " << names.shadowControlStem << ", " << names.shadowMatrixStem << ", " << lightVarNames.lightPos << ".xyz, varWorldPos, vec2(1.0, " << names.shadowControlStem << ".z));\n";
    } else {
        inLightShader << "    shadow_map_occl = sampleOrthographic(" << names.shadowMapStem << ", " << names.shadowControlStem << ", " << names.shadowMatrixStem << ", varWorldPos, vec2(1.0, " << names.shadowControlStem << ".z));\n";
    }
}

static void addSpecularAmount(QSSGStageGeneratorBase &fragmentShader, bool &fragmentHasSpecularAmount, bool reapply = false)
{
    if (!fragmentHasSpecularAmount)
        fragmentShader << "    vec3 specularAmount = specularBase * vec3(material_properties.z + material_properties.x * (1.0 - material_properties.z));\n";
    else if (reapply)
        fragmentShader << "    specularAmount = specularBase * vec3(material_properties.z + material_properties.x * (1.0 - material_properties.z));\n";
    fragmentHasSpecularAmount = true;
}

static void maybeAddMaterialFresnel(QSSGStageGeneratorBase &fragmentShader,
                                    const QSSGShaderDefaultMaterialKeyProperties &keyProps,
                                    QSSGDataView<quint32> inKey,
                                    bool &fragmentHasSpecularAmount,
                                    bool hasMetalness)
{
    if (keyProps.m_fresnelEnabled.getValue(inKey)) {
        addSpecularAmount(fragmentShader, fragmentHasSpecularAmount);
        fragmentShader.addInclude("defaultMaterialFresnel.glsllib");
        fragmentShader.addUniform("fresnelPower", "float");
        fragmentShader.addUniform("material_specular", "vec4");
        if (hasMetalness) {
            fragmentShader << "    // Add fresnel ratio\n"
                              "    specularAmount *= defaultMaterialSimpleFresnel(specularBase, metalnessAmount, world_normal, view_vector, dielectricSpecular(material_properties.w), fresnelPower);\n";
        } else {
            fragmentShader << "    // Add fresnel ratio\n"
                              "    specularAmount *= defaultMaterialSimpleFresnelNoMetalness(world_normal, view_vector, dielectricSpecular(material_properties.w), fresnelPower);\n";
        }
    }
}

static QSSGMaterialShaderGenerator::LightVariableNames setupLightVariableNames(qint32 lightIdx, QSSGRenderLight &inLight, bool lightsAsSeparateUniforms)
{
    Q_ASSERT(lightIdx > -1);
    QSSGMaterialShaderGenerator::LightVariableNames names;
    if (lightsAsSeparateUniforms) {
        char buf[16];
        qsnprintf(buf, 16, "light_%d", int(lightIdx));
        QByteArray lightStem = buf;
        names.lightColor = lightStem;
        names.lightColor.append("_diffuse");
        names.lightDirection = lightStem;
        names.lightDirection.append("_direction");
        names.lightSpecularColor = lightStem;
        names.lightSpecularColor.append("_specular");
        if (inLight.m_lightType == QSSGRenderLight::Type::Point) {
            names.lightPos = lightStem;
            names.lightPos.append("_position");
            names.lightAttenuation = lightStem;
            names.lightAttenuation.append("_attenuation");
        } else if (inLight.m_lightType == QSSGRenderLight::Type::Area) {
            names.lightPos = lightStem;
            names.lightPos.append("_position");
            names.lightUp = lightStem;
            names.lightUp.append("_up");
            names.lightRt = lightStem;
            names.lightRt.append("_right");
        } else if (inLight.m_lightType == QSSGRenderLight::Type::Spot) {
            names.lightPos = lightStem;
            names.lightPos.append("_position");
            names.lightAttenuation = lightStem;
            names.lightAttenuation.append("_attenuation");
            names.lightConeAngle = lightStem;
            names.lightConeAngle.append("_coneAngle");
            names.lightInnerConeAngle = lightStem;
            names.lightInnerConeAngle.append("_innerConeAngle");
        }
    } else {
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
    }

    return names;
}

static void generateShadowMapOcclusion(QSSGStageGeneratorBase &fragmentShader,
                                       QSSGVertexPipelineBase &vertexShader,
                                       quint32 lightIdx,
                                       bool inShadowEnabled,
                                       QSSGRenderLight::Type inType,
                                       const QSSGMaterialShaderGenerator::LightVariableNames &lightVarNames)
{
    if (inShadowEnabled) {
        vertexShader.generateWorldPosition();
        addShadowMapContribution(fragmentShader, lightIdx, inType, lightVarNames);
        /*
            VertexGenerator().AddUniform( m_ShadowMatrixStem, "mat4" );
            VertexGenerator().AddOutgoing( m_ShadowCoordStem, "vec4" );
            VertexGenerator() << "    vec4 local_" << m_ShadowCoordStem << " = " << m_ShadowMatrixStem
            << " * vec4(local_model_world_position, 1.0);" << "\n";
            m_TempStr.assign( "local_" );
            m_TempStr.append( m_ShadowCoordStem );
            VertexGenerator().AssignOutput( m_ShadowCoordStem, m_TempStr );
            */
    } else {
        fragmentShader << "    shadow_map_occl = 1.0;\n";
    }
}

static void generateFragmentShader(QSSGStageGeneratorBase &fragmentShader,
                                   QSSGVertexPipelineBase &vertexShader,
                                   const QSSGShaderDefaultMaterialKey &inKey,
                                   const QSSGShaderDefaultMaterialKeyProperties &keyProps,
                                   const ShaderFeatureSetList &featureSet,
                                   const QSSGRenderDefaultMaterial &material,
                                   const QSSGShaderLightList &lights,
                                   QSSGRenderableImage *firstImage,
                                   bool lightsAsSeparateUniforms)
{
    const bool metalnessEnabled = material.isMetalnessEnabled();
    const bool specularEnabled = material.isSpecularEnabled();
    bool vertexColorsEnabled = material.isVertexColorsEnabled();
    bool specularLightingEnabled = metalnessEnabled || specularEnabled;

    bool hasLighting = material.hasLighting();
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
    char imageFragCoords[13];

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
    bool enableSSDO = false;
    bool enableShadowMaps = false;
    bool enableBumpNormal = normalImage || bumpImage;
    specularLightingEnabled |= specularAmountImage != nullptr;

    for (qint32 idx = 0; idx < featureSet.size(); ++idx) {
        const auto &name = featureSet.at(idx).name;
        if (name == QSSGShaderDefines::asString(QSSGShaderDefines::Ssao))
            enableSSAO = featureSet.at(idx).enabled;
        else if (name == QSSGShaderDefines::asString(QSSGShaderDefines::Ssdo))
            enableSSDO = featureSet.at(idx).enabled;
        else if (name == QSSGShaderDefines::asString(QSSGShaderDefines::Ssm))
            enableShadowMaps = featureSet.at(idx).enabled;
    }

    bool includeSSAOSSDOVars = enableSSAO || enableSSDO || enableShadowMaps;

    vertexShader.beginFragmentGeneration();

    // The fragment or vertex shaders may not use the material_properties or diffuse
    // uniforms in all cases but it is simpler to just add them and let the linker strip them.
    fragmentShader.addUniform("material_diffuse", "vec3");
    fragmentShader.addUniform("base_color", "vec4");
    fragmentShader.addUniform("material_properties", "vec4");

    // !hasLighting does not mean 'no light source'
    // it should be KHR_materials_unlit
    // https://github.com/KhronosGroup/glTF/tree/master/extensions/2.0/Khronos/KHR_materials_unlit
    if (hasLighting) {
        // All these are needed for SSAO
        if (includeSSAOSSDOVars) {
            fragmentShader.addInclude("SSAOCustomMaterial.glsllib");
            // fragmentShader.AddUniform( "aoTexture", "sampler2D" );
        }

        if (hasIblProbe) {
            fragmentShader.addInclude("sampleProbe.glsllib");
        }

        if (!lightsAsSeparateUniforms)
            fragmentShader.addFunction("sampleLightVars");
        fragmentShader.addFunction("diffuseReflectionBSDF");

        if (hasLightmaps) {
            fragmentShader.addInclude("evalLightmaps.glsllib");
        }

        // view_vector, varWorldPos, world_normal are all used if there is a specular map
        // in addition to if there is specular lighting.  So they are lifted up here, always
        // generated.
        // we rely on the linker to strip out what isn't necessary instead of explicitly stripping
        // it for code simplicity.
        if (hasImage) {
            fragmentShader.append("    vec3 uTransform;");
            fragmentShader.append("    vec3 vTransform;");
        }

        vertexShader.generateViewVector();
        vertexShader.generateWorldNormal(inKey);
        vertexShader.generateWorldPosition();

        if (includeSSAOSSDOVars || specularEnabled || metalnessEnabled || hasIblProbe || enableBumpNormal)
            vertexShader.generateVarTangentAndBinormal(inKey);

        fragmentShader.append("    vec3 org_normal = world_normal;\n");
        fragmentShader.append("    float facing = step(0.0, dot(view_vector, org_normal)) * 2.0 - 1.0;\n");

        if (bumpImage != nullptr) {
            generateImageUVCoordinates(vertexShader, fragmentShader, inKey, *bumpImage, bumpImage->m_image.m_indexUV);
            const auto &names = imageStringTable[int(QSSGRenderableImage::Type::Bump)];

            fragmentShader << "    if (tangent == vec3(0.0)) {\n"
                           << "        vec2 dUVdx = dFdx(" << names.imageFragCoords << ");\n"
                           << "        vec2 dUVdy = dFdy(" << names.imageFragCoords << ");\n";
        } else if (normalImage != nullptr) {
            generateImageUVCoordinates(vertexShader, fragmentShader, inKey, *normalImage, normalImage->m_image.m_indexUV);
            const auto &names = imageStringTable[int(QSSGRenderableImage::Type::Normal)];
            fragmentShader << "    if (tangent == vec3(0.0)) {\n"
                           << "        vec2 dUVdx = dFdx(" << names.imageFragCoords << ");\n"
                           << "        vec2 dUVdy = dFdy(" << names.imageFragCoords << ");\n";
        }

        if (enableBumpNormal) {
            fragmentShader.addUniform("bumpAmount", "float");
            fragmentShader << "        tangent = (dUVdy.y * dFdx(varWorldPos) - dUVdx.y * dFdy(varWorldPos)) / (dUVdx.x * dUVdy.y - dUVdx.y * dUVdy.x);\n"
                           << "        tangent = tangent - dot(org_normal, tangent) * org_normal;\n"
                           << "        tangent = normalize(tangent);\n"
                           << "    }\n";
            fragmentShader << "    if (binormal == vec3(0.0))\n"
                           << "        binormal = cross(org_normal, tangent);\n";
        }

        // apply facing factor before fetching texture
        fragmentShader.append("    org_normal *= facing;");
        if (includeSSAOSSDOVars || specularEnabled || metalnessEnabled || hasIblProbe || enableBumpNormal) {
            fragmentShader.append("    tangent *= facing;");
            fragmentShader.append("    binormal *= facing;");
        }

        if (bumpImage != nullptr) {
            const auto &names = imageStringTable[int(QSSGRenderableImage::Type::Bump)];
            fragmentShader.addUniform(names.imageSamplerSize, "vec2");
            fragmentShader.addInclude("defaultMaterialBumpNoLod.glsllib");
            fragmentShader << "    world_normal = defaultMaterialBumpNoLod(" << names.imageSampler << ", bumpAmount, " << names.imageFragCoords << ", tangent, binormal, org_normal, " << names.imageSamplerSize << ");\n";
        } else if (normalImage != nullptr) {
            const auto &names = imageStringTable[int(QSSGRenderableImage::Type::Normal)];
            fragmentShader.addFunction("sampleNormalTexture");
            fragmentShader << "    world_normal = sampleNormalTexture(" << names.imageSampler << ", bumpAmount, " << names.imageFragCoords << ", tangent, binormal, org_normal);\n";
        }

        fragmentShader.addUniform("normalAdjustViewportFactor", "float");
        if (isDoubleSided) {
            fragmentShader.addInclude("doubleSided.glsllib");
            fragmentShader.append("    world_normal = adjustNormalForFace(world_normal, varWorldPos, normalAdjustViewportFactor);\n");
        }

        if (includeSSAOSSDOVars || specularEnabled || metalnessEnabled || hasIblProbe || enableBumpNormal)
            fragmentShader << "    mat3 tanFrame = mat3(tangent, binormal, world_normal);\n";

        if (hasEmissiveMap)
            fragmentShader.append("    vec3 global_emission = material_diffuse.rgb;");

        if (specularLightingEnabled)
            fragmentShader.append("    vec3 specularBase;");

    }

    if (vertexColorsEnabled)
        vertexShader.generateVertexColor(inKey);
    else
        fragmentShader.append("    vec4 vertColor = vec4(1.0);");

    bool fragmentHasSpecularAmount = false;

    fragmentShader << "    vec4 diffuseColor = base_color * vertColor;\n";

    if (baseImage) {
        QByteArray texSwizzle;
        QByteArray lookupSwizzle;

        // NoLighting also needs to fetch baseImage
        if (!hasLighting) {
            fragmentShader.append("    vec3 uTransform;");
            fragmentShader.append("    vec3 vTransform;");
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

        fragmentShader << "    vec4 base_texture_color" << texSwizzle << " = texture2D(" << names.imageSampler << ", " << (hasIdentityMap ? imageFragCoords : names.imageFragCoords) << ")" << lookupSwizzle << ";\n";
        fragmentShader << "    diffuseColor *= base_texture_color;\n";
    }

    if (hasLighting) {
        fragmentShader.addUniform("light_ambient_total", "vec3");

        fragmentShader.append("    vec4 global_diffuse_light = vec4(light_ambient_total.rgb * diffuseColor.rgb, diffuseColor.a);");
        fragmentShader.append("    vec3 global_specular_light = vec3(0.0, 0.0, 0.0);");
        fragmentShader.append("    float shadow_map_occl = 1.0;");

        if (specularLightingEnabled) {
            fragmentShader << "    specularBase = diffuseColor.rgb;\n";
            vertexShader.generateViewVector();
            fragmentShader.addUniform("material_properties", "vec4");
            addSpecularAmount(fragmentShader, fragmentHasSpecularAmount);
        }

        if (lightmapIndirectImage != nullptr) {
            const bool hasIdentityMap = identityImages.contains(lightmapIndirectImage);
            if (hasIdentityMap)
                generateImageUVSampler(vertexShader, fragmentShader, inKey, *lightmapIndirectImage, imageFragCoords, 1);
            else
                generateImageUVCoordinates(vertexShader, fragmentShader, inKey, *lightmapIndirectImage, 1);

            const auto &names = imageStringTable[int(QSSGRenderableImage::Type::LightmapIndirect)];
            fragmentShader << "    vec4 indirect_light = texture2D(" << names.imageSampler << ", " << (hasIdentityMap ? imageFragCoords : names.imageFragCoords) << ");\n";
            fragmentShader << "    global_diffuse_light += indirect_light;\n";
            if (specularLightingEnabled)
                fragmentShader << "    global_specular_light += indirect_light.rgb * specularAmount;\n";
        }

        if (lightmapRadiosityImage != nullptr) {
            const bool hasIdentityMap = identityImages.contains(lightmapRadiosityImage);
            if (identityImages.contains(lightmapRadiosityImage))
                generateImageUVSampler(vertexShader, fragmentShader, inKey, *lightmapRadiosityImage, imageFragCoords, 1);
            else
                generateImageUVCoordinates(vertexShader, fragmentShader, inKey, *lightmapRadiosityImage, 1);

            const auto &names = imageStringTable[int(QSSGRenderableImage::Type::LightmapRadiosity)];
            fragmentShader << "    vec4 direct_light = texture2D(" << names.imageSampler << ", " << (hasIdentityMap ? imageFragCoords : names.imageFragCoords) << ");\n";
            fragmentShader << "    global_diffuse_light += direct_light;\n";
            if (specularLightingEnabled)
                fragmentShader << "    global_specular_light += direct_light.rgb * specularAmount;\n";
        }

        if (translucencyImage != nullptr) {
            fragmentShader.addUniform("translucentFalloff", "float");
            fragmentShader.addUniform("diffuseLightWrap", "float");

            const bool hasIdentityMap = identityImages.contains(translucencyImage);
            if (hasIdentityMap)
                generateImageUVSampler(vertexShader, fragmentShader, inKey, *translucencyImage, imageFragCoords);
            else
                generateImageUVCoordinates(vertexShader, fragmentShader, inKey, *translucencyImage);

            const auto &names = imageStringTable[int(QSSGRenderableImage::Type::Translucency)];
            const auto &channelProps = keyProps.m_textureChannels[QSSGShaderDefaultMaterialKeyProperties::TranslucencyChannel];
            fragmentShader << "    float translucent_depth_range = texture2D(" << names.imageSampler
                           << ", " << (hasIdentityMap ? imageFragCoords : names.imageFragCoords) << ")" << channelStr(channelProps, inKey) << ";\n";
            fragmentShader << "    float translucent_thickness = translucent_depth_range * translucent_depth_range;\n";
            fragmentShader << "    float translucent_thickness_exp = exp(translucent_thickness * translucentFalloff);\n";
        }

        fragmentShader.append("    float lightAttenuation = 1.0;");

        addLocalVariable(fragmentShader, "aoFactor", "float");

        if (enableSSAO)
            fragmentShader.append("    aoFactor = customMaterialAO();");
        else
            fragmentShader.append("    aoFactor = 1.0;");

        addLocalVariable(fragmentShader, "shadowFac", "float");

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
            fragmentShader << "    specularBase *= texture2D(" << names.imageSampler << ", " << (hasIdentityMap ? imageFragCoords : names.imageFragCoords) << ").rgb;\n";
        }

        fragmentShader << "    float roughnessAmount = material_properties.y;\n";
        if (specularLightingEnabled && roughnessImage) {
            const auto &channelProps = keyProps.m_textureChannels[QSSGShaderDefaultMaterialKeyProperties::RoughnessChannel];
            const bool hasIdentityMap = identityImages.contains(roughnessImage);
            if (hasIdentityMap)
                generateImageUVSampler(vertexShader, fragmentShader, inKey, *roughnessImage, imageFragCoords, roughnessImage->m_image.m_indexUV);
            else
                generateImageUVCoordinates(vertexShader, fragmentShader, inKey, *roughnessImage, roughnessImage->m_image.m_indexUV);

            const auto &names = imageStringTable[int(QSSGRenderableImage::Type::Roughness)];
            fragmentShader << "    roughnessAmount *= texture2D(" << names.imageSampler << ", "
                           << (hasIdentityMap ? imageFragCoords : names.imageFragCoords) << ")" << channelStr(channelProps, inKey) << ";\n";
        }

        fragmentShader << "    float metalnessAmount = material_properties.z;\n";
        if (specularLightingEnabled && metalnessImage) {
            const auto &channelProps = keyProps.m_textureChannels[QSSGShaderDefaultMaterialKeyProperties::MetalnessChannel];
            const bool hasIdentityMap = identityImages.contains(metalnessImage);
            if (hasIdentityMap)
                generateImageUVSampler(vertexShader, fragmentShader, inKey, *metalnessImage, imageFragCoords, metalnessImage->m_image.m_indexUV);
            else
                generateImageUVCoordinates(vertexShader, fragmentShader, inKey, *metalnessImage, metalnessImage->m_image.m_indexUV);

            const auto &names = imageStringTable[int(QSSGRenderableImage::Type::Metalness)];
            fragmentShader << "    float sampledMetalness = texture2D(" << names.imageSampler << ", "
                           << (hasIdentityMap ? imageFragCoords : names.imageFragCoords) << ")" << channelStr(channelProps, inKey) << ";\n";
            fragmentShader << "    metalnessAmount = clamp(metalnessAmount * sampledMetalness, 0.0, 1.0);\n";
            addSpecularAmount(fragmentShader, fragmentHasSpecularAmount, true);
        }
        fragmentShader.addInclude("defaultMaterialFresnel.glsllib");
        fragmentShader << "    float ds = dielectricSpecular(material_properties.w);\n";
        fragmentShader << "    diffuseColor.rgb *= (1.0 - ds) * (1.0 - metalnessAmount);\n";
        if (specularLightingEnabled) {
            if (!hasBaseColorMap && material.type == QSSGRenderGraphObject::Type::PrincipledMaterial) {
                fragmentShader << "    float lum = dot(base_color.rgb, vec3(0.21, 0.72, 0.07));\n"
                                  "    specularBase += (lum > 0.0) ? (base_color.rgb) / lum : vec3(1.0);\n";
            }

            maybeAddMaterialFresnel(fragmentShader, keyProps, inKey, fragmentHasSpecularAmount, metalnessEnabled);
        }

        // Iterate through all lights
        Q_ASSERT(lights.size() < INT32_MAX);
        for (qint32 lightIdx = 0; lightIdx < lights.size(); ++lightIdx) {
            auto &shaderLight = lights[lightIdx];
            if (!shaderLight.enabled)
                continue;
            QSSGRenderLight *lightNode = shaderLight.light;
            auto lightVarNames = setupLightVariableNames(lightIdx, *lightNode, lightsAsSeparateUniforms);

            bool isDirectional = lightNode->m_lightType == QSSGRenderLight::Type::Directional;
            bool isArea = lightNode->m_lightType == QSSGRenderLight::Type::Area;
            bool isSpot = lightNode->m_lightType == QSSGRenderLight::Type::Spot;
            bool isShadow = enableShadowMaps && lightNode->m_castShadow;

            fragmentShader.append("");
            char buf[11];
            snprintf(buf, 11, "%d", lightIdx);

            QByteArray tempStr = "light";
            tempStr.append(buf);

            fragmentShader << "    //Light " << buf << "\n"
                                                       "    lightAttenuation = 1.0;\n";
            if (isDirectional) {
                if (lightsAsSeparateUniforms) {
                    fragmentShader.addUniform(lightVarNames.lightDirection, "vec4");
                    fragmentShader.addUniform(lightVarNames.lightColor, "vec4");
                }

                if (enableSSDO)
                    fragmentShader << "    shadowFac = customMaterialShadow(" << lightVarNames.lightDirection << ".xyz, varWorldPos);\n";
                else
                    fragmentShader << "    shadowFac = 1.0;\n";

                generateShadowMapOcclusion(fragmentShader, vertexShader, lightIdx, enableShadowMaps && isShadow, lightNode->m_lightType, lightVarNames);

                if (specularLightingEnabled && enableShadowMaps && isShadow)
                    fragmentShader << "    lightAttenuation *= shadow_map_occl;\n";

                fragmentShader << "    global_diffuse_light.rgb += diffuseColor.rgb * shadowFac * shadow_map_occl * diffuseReflectionBSDF(world_normal, -" << lightVarNames.lightDirection << ".xyz, " << lightVarNames.lightColor << ".rgb).rgb;\n";

                if (specularLightingEnabled) {
                    if (lightsAsSeparateUniforms)
                        fragmentShader.addUniform(lightVarNames.lightSpecularColor, "vec4");
                    outputSpecularEquation(material.specularModel, fragmentShader, lightVarNames.lightDirection, lightVarNames.lightSpecularColor);
                }
            } else if (isArea) {
                if (lightsAsSeparateUniforms) {
                    fragmentShader.addUniform(lightVarNames.lightColor, "vec4");
                    fragmentShader.addUniform(lightVarNames.lightPos, "vec4");
                    fragmentShader.addUniform(lightVarNames.lightDirection, "vec4");
                    fragmentShader.addUniform(lightVarNames.lightUp, "vec4");
                    fragmentShader.addUniform(lightVarNames.lightRt, "vec4");
                } else {
                    fragmentShader.addFunction("areaLightVars");
                }
                fragmentShader.addFunction("calculateDiffuseAreaOld");
                vertexShader.generateWorldPosition();
                generateShadowMapOcclusion(fragmentShader, vertexShader, lightIdx, enableShadowMaps && isShadow, lightNode->m_lightType, lightVarNames);

                lightVarNames.normalizedDirection = tempStr;
                lightVarNames.normalizedDirection.append("_Frame");

                addLocalVariable(fragmentShader, lightVarNames.normalizedDirection, "mat3");
                fragmentShader << "    " << lightVarNames.normalizedDirection << " = mat3(" << lightVarNames.lightRt << ".xyz, " << lightVarNames.lightUp << ".xyz, -" << lightVarNames.lightDirection << ".xyz);\n";

                if (enableSSDO)
                    fragmentShader << "    shadowFac = shadow_map_occl * customMaterialShadow(" << lightVarNames.lightDirection << ".xyz, varWorldPos);\n";
                else
                    fragmentShader << "    shadowFac = shadow_map_occl;\n";

                if (specularLightingEnabled) {
                    vertexShader.generateViewVector();
                    if (lightsAsSeparateUniforms)
                        fragmentShader.addUniform(lightVarNames.lightSpecularColor, "vec4");
                    outputSpecularAreaLighting(fragmentShader, "varWorldPos", "view_vector", lightVarNames.lightSpecularColor, lightVarNames);
                }

                outputDiffuseAreaLighting(fragmentShader, "varWorldPos", tempStr, lightVarNames);
                fragmentShader << "    lightAttenuation *= shadowFac;\n";

                addTranslucencyIrradiance(fragmentShader, translucencyImage, true, lightVarNames);

                fragmentShader << "    global_diffuse_light.rgb += diffuseColor.rgb * lightAttenuation * diffuseReflectionBSDF(world_normal, " << lightVarNames.normalizedDirection << ", " << lightVarNames.lightColor << ".rgb).rgb;\n";
            } else {
                vertexShader.generateWorldPosition();

                if (lightsAsSeparateUniforms) {
                    fragmentShader.addUniform(lightVarNames.lightColor, "vec4");
                    fragmentShader.addUniform(lightVarNames.lightPos, "vec4");
                    if (isSpot)
                        fragmentShader.addUniform(lightVarNames.lightDirection, "vec4");
                }

                lightVarNames.relativeDirection = tempStr;
                lightVarNames.relativeDirection.append("_relativeDirection");

                lightVarNames.normalizedDirection = lightVarNames.relativeDirection;
                lightVarNames.normalizedDirection.append("_normalized");

                lightVarNames.relativeDistance = tempStr;
                lightVarNames.relativeDistance.append("_distance");

                fragmentShader << "    vec3 " << lightVarNames.relativeDirection << " = varWorldPos - " << lightVarNames.lightPos << ".xyz;\n"
                               << "    float " << lightVarNames.relativeDistance << " = length(" << lightVarNames.relativeDirection << ");\n"
                               << "    vec3 " << lightVarNames.normalizedDirection << " = " << lightVarNames.relativeDirection << " / " << lightVarNames.relativeDistance << ";\n";

                if (isSpot) {
                    lightVarNames.spotAngle = tempStr;
                    lightVarNames.spotAngle.append("_spotAngle");

                    if (lightsAsSeparateUniforms) {
                        fragmentShader.addUniform(lightVarNames.lightConeAngle, "float");
                        fragmentShader.addUniform(lightVarNames.lightInnerConeAngle, "float");
                    }
                    fragmentShader << "    float " << lightVarNames.spotAngle << " = dot(" << lightVarNames.normalizedDirection
                                   << ", normalize(vec3(" << lightVarNames.lightDirection << ")));\n";
                    fragmentShader << "    if (" << lightVarNames.spotAngle << " > " << lightVarNames.lightConeAngle << ") {\n";
                }

                generateShadowMapOcclusion(fragmentShader, vertexShader, lightIdx, enableShadowMaps && isShadow, lightNode->m_lightType, lightVarNames);

                if (enableSSDO) {
                    fragmentShader << "    shadowFac = shadow_map_occl * customMaterialShadow(" << lightVarNames.normalizedDirection << ", varWorldPos);\n";
                } else {
                    fragmentShader << "    shadowFac = shadow_map_occl;\n";
                }

                fragmentShader.addFunction("calculatePointLightAttenuation");

                if (lightsAsSeparateUniforms) {
                    fragmentShader.addUniform(lightVarNames.lightAttenuation, "vec3");
                    fragmentShader << "    lightAttenuation = shadowFac * calculatePointLightAttenuation(vec3(" << lightVarNames.lightAttenuation << ".x, " << lightVarNames.lightAttenuation << ".y, " << lightVarNames.lightAttenuation << ".z), " << lightVarNames.relativeDistance << ");\n";
                } else {
                    fragmentShader << "    lightAttenuation = shadowFac * calculatePointLightAttenuation(vec3(" << lightVarNames.lightConstantAttenuation << ", " << lightVarNames.lightLinearAttenuation << ", " << lightVarNames.lightQuadraticAttenuation << "), " << lightVarNames.relativeDistance << ");\n";
                }

                addTranslucencyIrradiance(fragmentShader, translucencyImage, false, lightVarNames);

                if (isSpot) {
                    fragmentShader << "    float spotFactor = smoothstep(" << lightVarNames.lightConeAngle
                                   << ", " << lightVarNames.lightInnerConeAngle << ", " << lightVarNames.spotAngle
                                   << ");\n";
                    fragmentShader << "    global_diffuse_light.rgb += diffuseColor.rgb * spotFactor * ";
                } else {
                    fragmentShader << "    global_diffuse_light.rgb += diffuseColor.rgb * ";
                }
                fragmentShader << "lightAttenuation * diffuseReflectionBSDF(world_normal, -"
                               << lightVarNames.normalizedDirection << ", "
                               << lightVarNames.lightColor << ".rgb).rgb;\n";

                if (specularLightingEnabled) {
                    if (lightsAsSeparateUniforms)
                        fragmentShader.addUniform(lightVarNames.lightSpecularColor, "vec4");
                    outputSpecularEquation(material.specularModel, fragmentShader, lightVarNames.normalizedDirection, lightVarNames.lightSpecularColor);
                }

                if (isSpot)
                    fragmentShader << "    }\n";
            }
        }

        // This may be confusing but the light colors are already modulated by the base
        // material color.
        // Thus material color is the base material color * material emissive.
        // Except material_color.a *is* the actual opacity factor.
        // Furthermore objectOpacity is something that may come from the vertex pipeline or
        // somewhere else.
        // We leave it up to the vertex pipeline to figure it out.
        fragmentShader << "    global_diffuse_light = vec4(global_diffuse_light.rgb * aoFactor, objectOpacity * diffuseColor.a);\n"
                          "    global_specular_light = vec3(global_specular_light.rgb);\n";
        if (!hasEmissiveMap)
            fragmentShader << "    global_diffuse_light.rgb += diffuseColor.rgb * material_diffuse.rgb;\n";

        // since we already modulate our material diffuse color
        // into the light color we will miss it entirely if no IBL
        // or light is used
        if (hasLightmaps && !(lights.size() || hasIblProbe))
            fragmentShader << "    global_diffuse_light.rgb *= diffuseColor.rgb;\n";

        if (hasIblProbe) {
            vertexShader.generateWorldNormal(inKey);

            fragmentShader << "    global_diffuse_light.rgb += diffuseColor.rgb * aoFactor * sampleDiffuse(tanFrame).rgb;\n";

            if (specularLightingEnabled) {
                fragmentShader.addUniform("material_specular", "vec4");
                fragmentShader << "    global_specular_light.rgb += specularAmount * vec3(material_specular.rgb) * sampleGlossy(tanFrame, view_vector, roughnessAmount).rgb;\n";
            }
        }

        if (hasImage) {
            fragmentShader.append("    vec4 texture_color;");
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
                fragmentShader << "    texture_color" << texSwizzle << " = texture2D(" << names.imageSampler
                               << ", " << (hasIdentityMap ? imageFragCoords : names.imageFragCoords) << ")" << lookupSwizzle << ";\n";

                if (image->m_image.m_textureData.m_textureFlags.isPreMultiplied())
                    fragmentShader << "    texture_color.rgb = texture_color.a > 0.0 ? texture_color.rgb / texture_color.a : vec3(0.0);\n";

                // These mapping types honestly don't make a whole ton of sense to me.
                switch (image->m_mapType) {
                case QSSGRenderableImage::Type::BaseColor:
                    // color already taken care of
                    if (material.alphaMode == QSSGRenderDefaultMaterial::MaterialAlphaMode::Mask) {
                        // The rendered output is either fully opaque or fully transparent depending on the alpha
                        // value and the specified alpha cutoff value.
                        fragmentShader.addUniform("alphaCutoff", "float");
                        fragmentShader << "    if ((texture_color.a * base_color.a) < alphaCutoff) {\n"
                                          "        fragOutput = vec4(0);\n"
                                          "        return;\n"
                                          "    }\n";
                    }
                    break;
                case QSSGRenderableImage::Type::Diffuse: // assume images are premultiplied.
                    // color already taken care of
                    fragmentShader.append("    global_diffuse_light.a *= base_color.a * texture_color.a;");
                    break;
                case QSSGRenderableImage::Type::LightmapShadow:
                    // We use image offsets.z to switch between incoming premultiplied textures or
                    // not premultiplied textures.
                    // If Z is 1, then we assume the incoming texture is already premultiplied, else
                    // we just read the rgb value.
                    fragmentShader.append("    global_diffuse_light *= texture_color;");
                    break;
                case QSSGRenderableImage::Type::Specular:
                    fragmentShader.addUniform("material_specular", "vec4");
                    if (fragmentHasSpecularAmount) {
                        fragmentShader.append("    global_specular_light.rgb += specularAmount * texture_color.rgb * material_specular.rgb;");
                    } else {
                        fragmentShader.append("    global_specular_light.rgb += texture_color.rgb * material_specular.rgb;");
                    }
                    fragmentShader.append("    global_diffuse_light.a *= texture_color.a;");
                    break;
                case QSSGRenderableImage::Type::Opacity:
                {
                    const auto &channelProps = keyProps.m_textureChannels[QSSGShaderDefaultMaterialKeyProperties::OpacityChannel];
                    fragmentShader << "    global_diffuse_light.a *= texture_color" << channelStr(channelProps, inKey) << ";\n";
                    break;
                }
                case QSSGRenderableImage::Type::Emissive:
                    fragmentShader.append("    global_emission *= texture_color.rgb * texture_color.a;");
                    break;
                default:
                    Q_ASSERT(false); // fallthrough intentional
                }
            }
        }

        // Occlusion Map
        if (occlusionImage) {
            fragmentShader.addUniform("occlusionAmount", "float");
            const auto &channelProps = keyProps.m_textureChannels[QSSGShaderDefaultMaterialKeyProperties::OcclusionChannel];
            const bool hasIdentityMap = identityImages.contains(occlusionImage);
            if (hasIdentityMap)
                generateImageUVSampler(vertexShader, fragmentShader, inKey, *occlusionImage, imageFragCoords, occlusionImage->m_image.m_indexUV);
            else
                generateImageUVCoordinates(vertexShader, fragmentShader, inKey, *occlusionImage, occlusionImage->m_image.m_indexUV);
            const auto &names = imageStringTable[int(QSSGRenderableImage::Type::Occlusion)];
            fragmentShader << "    float ao = texture2D(" << names.imageSampler << ", "
                           << (hasIdentityMap ? imageFragCoords : names.imageFragCoords) << ")" << channelStr(channelProps, inKey) << ";\n";
            fragmentShader << "    global_diffuse_light.rgb = mix(global_diffuse_light.rgb, global_diffuse_light.rgb * ao, occlusionAmount);\n";
        }

        if (hasEmissiveMap)
            fragmentShader.append("    global_diffuse_light.rgb += global_emission.rgb;");

        if (hasBaseColorMap && specularLightingEnabled) {
            fragmentShader.addInclude("luminance.glsllib");
            fragmentShader << "    float lum = luminance(specularBase);\n"
                              "    global_specular_light.rgb *= (lum > 0.0) ? specularBase / lum : vec3(1.0);\n";
        }

        // Ensure the rgb colors are in range.
        fragmentShader.append("    fragOutput = vec4(clamp(global_diffuse_light.rgb + global_specular_light.rgb, 0.0, 1.0), global_diffuse_light.a);");
    } else {
        fragmentShader.append("    fragOutput = diffuseColor;");
    }
}

QSSGRef<QSSGRhiShaderStages> QSSGMaterialShaderGenerator::generateMaterialRhiShader(const QByteArray &inShaderPrefix,
                                                                                    QSSGVertexPipelineBase &vertexPipeline,
                                                                                    const QSSGShaderDefaultMaterialKey &key,
                                                                                    QSSGShaderDefaultMaterialKeyProperties &inProperties,
                                                                                    const ShaderFeatureSetList &inFeatureSet,
                                                                                    const QSSGRenderDefaultMaterial &material,
                                                                                    const QSSGShaderLightList &inLights,
                                                                                    QSSGRenderableImage *inFirstImage)
{
    // build a string that allows us to print out the shader we are generating to the log.
    // This is time consuming but I feel like it doesn't happen all that often and is very
    // useful to users
    // looking at the log file.

    QByteArray generatedShaderString;
    generatedShaderString = inShaderPrefix;

    key.toString(generatedShaderString, inProperties);

    auto &vertexGenerator = vertexPipeline;
    const auto &programGenerator = vertexPipeline.programGenerator();

    // the pipeline opens/closes up the shaders stages
    vertexGenerator.beginVertexGeneration();
    auto &fragmentGenerator = vertexPipeline.fragment();

    generateFragmentShader(fragmentGenerator, vertexGenerator, key, inProperties, inFeatureSet, material, inLights, inFirstImage, false /*lightsAsSeparateUniforms*/);

    vertexGenerator.endVertexGeneration(false);
    vertexGenerator.endFragmentGeneration(false);

    return programGenerator->compileGeneratedRhiShader(generatedShaderString, QSSGShaderCacheProgramFlags(), inFeatureSet);
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

void QSSGMaterialShaderGenerator::setRhiMaterialProperties(const QSSGRenderContextInterface &/*renderContext*/,
                                                           QSSGRef<QSSGRhiShaderStagesWithResources> &shaders,
                                                           QSSGRhiGraphicsPipelineState *inPipelineState,
                                                           const QSSGRenderGraphObject &inMaterial,
                                                           const QVector2D &inCameraVec,
                                                           const QMatrix4x4 &inModelViewProjection,
                                                           const QMatrix3x3 &inNormalMatrix,
                                                           const QMatrix4x4 &inGlobalTransform,
                                                           const QMatrix4x4 &clipSpaceCorrMatrix,
                                                           const QSSGDataView<QMatrix4x4> &inBones,
                                                           QSSGRenderableImage *inFirstImage,
                                                           float inOpacity,
                                                           const QSSGLayerGlobalRenderProperties &inRenderProperties,
                                                           const QSSGShaderLightList &inLights,
                                                           bool receivesShadows)
{
    Q_UNUSED(inPipelineState);
    Q_UNUSED(receivesShadows);

    const QSSGRenderDefaultMaterial &theMaterial(static_cast<const QSSGRenderDefaultMaterial &>(inMaterial));
    Q_ASSERT(inMaterial.type == QSSGRenderGraphObject::Type::DefaultMaterial || inMaterial.type == QSSGRenderGraphObject::Type::PrincipledMaterial);

    QSSGRhiShaderStagesWithResources::CommonUniformIndices& cui = shaders->commonUniformIndices;

    QSSGRenderCamera &theCamera(inRenderProperties.camera);

    const QVector3D camGlobalPos = theCamera.getGlobalPos();
    cui.cameraPositionIdx = shaders->setUniform(QByteArrayLiteral("cameraPosition"), &camGlobalPos, 3 * sizeof(float), cui.cameraPositionIdx);
    cui.cameraDirectionIdx = shaders->setUniform(QByteArrayLiteral("cameraDirection"), &inRenderProperties.cameraDirection, 3 * sizeof(float), cui.cameraDirectionIdx);

    QMatrix4x4 viewProj;
    theCamera.calculateViewProjectionMatrix(viewProj);
    viewProj = clipSpaceCorrMatrix * viewProj;
    cui.viewProjectionMatrixIdx = shaders->setUniform(QByteArrayLiteral("viewProjectionMatrix"), viewProj.constData(), 16 * sizeof(float), cui.viewProjectionMatrixIdx);

    const QMatrix4x4 viewMatrix = theCamera.globalTransform.inverted();
    cui.viewMatrixIdx = shaders->setUniform(QByteArrayLiteral("viewMatrix"), viewMatrix.constData(), 16 * sizeof(float), cui.viewMatrixIdx);

    // Skinning
    cui.boneTransformsIdx = shaders->setUniformArray(QByteArrayLiteral("boneTransforms"), inBones.mData, inBones.mSize, QSSGRenderShaderDataType::Matrix4x4, cui.boneTransformsIdx);

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
    cui.normalAdjustViewportFactorIdx = shaders->setUniform(QByteArrayLiteral("normalAdjustViewportFactor"), &normalVpFactor, sizeof(float), cui.normalAdjustViewportFactorIdx);

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

    const QMatrix4x4 mvp = clipSpaceCorrMatrix * inModelViewProjection;
    cui.modelViewProjectionIdx = shaders->setUniform(QByteArrayLiteral("modelViewProjection"), mvp.constData(), 16 * sizeof(float), cui.modelViewProjectionIdx);

    // mat3 is still 4 floats per column in the uniform buffer (but there
    // is no 4th column), so 48 bytes altogether, not 36 or 64.
    float normalMatrix[12];
    memcpy(normalMatrix, inNormalMatrix.constData(), 3 * sizeof(float));
    memcpy(normalMatrix + 4, inNormalMatrix.constData() + 3, 3 * sizeof(float));
    memcpy(normalMatrix + 8, inNormalMatrix.constData() + 6, 3 * sizeof(float));
    cui.normalMatrixIdx = shaders->setUniform(QByteArrayLiteral("normalMatrix"), normalMatrix, 12 * sizeof(float), cui.normalMatrixIdx);
    cui.modelMatrixIdx = shaders->setUniform(QByteArrayLiteral("modelMatrix"), inGlobalTransform.constData(), 16 * sizeof(float), cui.modelMatrixIdx);

    shaders->setDepthTexture(inRenderProperties.rhiDepthTexture);
    shaders->setSsaoTexture(inRenderProperties.rhiSsaoTexture);

    QSSGRenderImage *theLightProbe = inRenderProperties.lightProbe;
    //QSSGRenderImage *theLightProbe2 = inRenderProperties.lightProbe2; ??? LightProbe2 not used in tooling

    // If the material has its own IBL Override, we should use that image instead.
    const bool hasIblProbe = theMaterial.iblProbe != nullptr;
    const bool useMaterialIbl = hasIblProbe && theMaterial.iblProbe->m_textureData.m_rhiTexture;
    if (useMaterialIbl)
        theLightProbe = theMaterial.iblProbe;

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
        cui.lightProbeRotationIdx = shaders->setUniform(QByteArrayLiteral("lightProbeRotation"), &rotations, 4 * sizeof(float), cui.lightProbeRotationIdx);
        cui.lightProbeOffsetIdx = shaders->setUniform(QByteArrayLiteral("lightProbeOffset"), &offsets, 4 * sizeof(float), cui.lightProbeOffsetIdx);

        if ((!theMaterial.iblProbe) && (inRenderProperties.probeFOV < 180.f)) {
            QVector4D opts(0.01745329251994329547f * inRenderProperties.probeFOV, 0.0f, 0.0f, 0.0f);
            cui.lightProbeOptionsIdx = shaders->setUniform(QByteArrayLiteral("lightProbeOptions"), &opts, 4 * sizeof(float), cui.lightProbeOptionsIdx);
        }

        QVector4D emptyProps2(0.0f, 0.0f, 0.0f, 0.0f);
        cui.lightProbe2PropertiesIdx = shaders->setUniform(QByteArrayLiteral("lightProbe2Properties"), &emptyProps2, 4 * sizeof(float), cui.lightProbe2PropertiesIdx);

        QVector4D props(0.0f, 0.0f, inRenderProperties.probeHorizon, inRenderProperties.probeBright * 0.01f);
        cui.lightProbePropertiesIdx = shaders->setUniform(QByteArrayLiteral("lightProbeProperties"), &props, 4 * sizeof(float), cui.lightProbePropertiesIdx);
        shaders->setLightProbeTexture(theLightProbe->m_textureData.m_rhiTexture, theHorzLightProbeTilingMode, theVertLightProbeTilingMode);
    } else {
        // no lightprobe
        QVector4D emptyProps(0.0f, 0.0f, -1.0f, 0.0f);
        cui.lightProbePropertiesIdx = shaders->setUniform(QByteArrayLiteral("lightProbeProperties"), &emptyProps, 4 * sizeof(float), cui.lightProbePropertiesIdx);

        QVector4D emptyProps2(0.0f, 0.0f, 0.0f, 0.0f);
        cui.lightProbe2PropertiesIdx = shaders->setUniform(QByteArrayLiteral("lightProbe2Properties"), &emptyProps2, 4 * sizeof(float), cui.lightProbe2PropertiesIdx);

        shaders->setLightProbeTexture(nullptr);
    }

    cui.material_diffuseIdx = shaders->setUniform(QByteArrayLiteral("material_diffuse"), &theMaterial.emissiveColor, 3 * sizeof(float), cui.material_diffuseIdx);

    const auto qMix = [](float x, float y, float a) {
        return (x * (1.0f - a) + (y * a));
    };

    const auto qMix3 = [&qMix](const QVector3D &x, const QVector3D &y, float a) {
        return QVector3D{qMix(x.x(), y.x(), a), qMix(x.y(), y.y(), a), qMix(x.z(), y.z(), a)};
    };

    const QVector4D &color = theMaterial.color;
    const auto &specularTint = (theMaterial.type == QSSGRenderGraphObject::Type::PrincipledMaterial) ? qMix3(QVector3D(1.0f, 1.0f, 1.0f), color.toVector3D(), theMaterial.specularTint.x())
                                                                                                     : theMaterial.specularTint;
    cui.base_colorIdx = shaders->setUniform(QByteArrayLiteral("base_color"), &color, 4 * sizeof(float), cui.base_colorIdx);

    QVector4D specularColor(specularTint, theMaterial.ior);
    cui.material_specularIdx = shaders->setUniform(QByteArrayLiteral("material_specular"), &specularColor, 4 * sizeof(float), cui.material_specularIdx);
    cui.cameraPropertiesIdx = shaders->setUniform(QByteArrayLiteral("cameraProperties"), &inCameraVec, 2 * sizeof(float), cui.cameraPropertiesIdx);
    cui.fresnelPowerIdx = shaders->setUniform(QByteArrayLiteral("fresnelPower"), &theMaterial.fresnelPower, sizeof(float), cui.fresnelPowerIdx);

    const auto diffuse = color.toVector3D() * (1.0f - theMaterial.metalnessAmount);
    const bool hasLighting = theMaterial.lighting != QSSGRenderDefaultMaterial::MaterialLighting::NoLighting;
    shaders->setLightsEnabled(QSSGRhiShaderStagesWithResources::LightBuffer0, hasLighting);
    if (hasLighting) {
        for (int idx = 0, end = shaders->lightCount(QSSGRhiShaderStagesWithResources::LightBuffer0); idx < end; ++idx) {
            QSSGShaderLightProperties &lightProp(shaders->lightAt(QSSGRhiShaderStagesWithResources::LightBuffer0, idx));
            lightProp.lightData.diffuse = QVector4D(lightProp.lightColor * diffuse, 1.0);
        }
    }

    const QVector3D diffuseLightAmbientTotal = theLightAmbientTotal * diffuse;
    cui.light_ambient_totalIdx = shaders->setUniform(QByteArrayLiteral("light_ambient_total"), &diffuseLightAmbientTotal, 3 * sizeof(float), cui.light_ambient_totalIdx);

    const QVector4D materialProperties(theMaterial.specularAmount, theMaterial.specularRoughness, theMaterial.metalnessAmount, inOpacity);
    cui.material_propertiesIdx = shaders->setUniform(QByteArrayLiteral("material_properties"), &materialProperties, 4 * sizeof(float), cui.material_propertiesIdx);

    cui.bumpAmountIdx = shaders->setUniform(QByteArrayLiteral("bumpAmount"), &theMaterial.bumpAmount, sizeof(float), cui.bumpAmountIdx);
    cui.translucentFalloffIdx = shaders->setUniform(QByteArrayLiteral("translucentFalloff"), &theMaterial.translucentFalloff, sizeof(float), cui.translucentFalloffIdx);
    cui.diffuseLightWrapIdx = shaders->setUniform(QByteArrayLiteral("diffuseLightWrap"), &theMaterial.diffuseLightWrap, sizeof(float), cui.diffuseLightWrapIdx);
    cui.occlusionAmountIdx = shaders->setUniform(QByteArrayLiteral("occlusionAmount"), &theMaterial.occlusionAmount, sizeof(float), cui.occlusionAmountIdx);
    cui.alphaCutoffIdx = shaders->setUniform(QByteArrayLiteral("alphaCutoff"), &theMaterial.alphaCutoff, sizeof(float), cui.alphaCutoffIdx);

    quint32 imageIdx = 0;
    for (QSSGRenderableImage *theImage = inFirstImage; theImage; theImage = theImage->m_nextImage, ++imageIdx)
        setRhiImageShaderVariables(shaders, *theImage, imageIdx);
}

QT_END_NAMESPACE
