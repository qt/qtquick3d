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
#include <QtQuick3DRuntimeRender/private/qssgrenderableimage_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderimage_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderlight_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercamera_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadowmap_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercustommaterial_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershaderlibrarymanager_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershaderkeys_p.h>

#include <QtCore/QByteArray>

QT_BEGIN_NAMESPACE

namespace {

struct QSSGShaderGenerator : public QSSGDefaultMaterialShaderGeneratorInterface
{
    const QSSGRenderDefaultMaterial *m_currentMaterial;

    QSSGRef<QSSGRenderShadowMap> m_shadowMapManager;
    bool m_lightsAsSeparateUniforms;

    QByteArray m_imageSampler;
    QByteArray m_imageFragCoords;
    QByteArray m_imageOffsets;
    QByteArray m_imageRotations;
    QByteArray m_imageTemp;
    QByteArray m_imageSamplerSize;

    QByteArray m_lightColor;
    QByteArray m_lightSpecularColor;
    QByteArray m_lightAttenuation;
    QByteArray m_lightConstantAttenuation;
    QByteArray m_lightLinearAttenuation;
    QByteArray m_lightQuadraticAttenuation;
    QByteArray m_normalizedDirection;
    QByteArray m_lightDirection;
    QByteArray m_lightPos;
    QByteArray m_lightUp;
    QByteArray m_lightRt;
    QByteArray m_lightConeAngle;
    QByteArray m_lightInnerConeAngle;
    QByteArray m_relativeDistance;
    QByteArray m_relativeDirection;
    QByteArray m_spotAngle;

    QByteArray m_shadowMapStem;
    QByteArray m_shadowCubeStem;
    QByteArray m_shadowMatrixStem;
    QByteArray m_shadowCoordStem;
    QByteArray m_shadowControlStem;

    QSSGShaderGenerator(QSSGRenderContextInterface *inRc)
        : QSSGDefaultMaterialShaderGeneratorInterface (inRc)
        , m_shadowMapManager(nullptr)
        , m_lightsAsSeparateUniforms(false)
    {
    }

    QSSGRef<QSSGProgramGenerator> programGenerator() { return m_programGenerator; }
    QSSGVertexPipelineBase &vertexGenerator() { return *m_currentPipeline; }
    QSSGStageGeneratorBase &fragmentGenerator()
    {
        return *m_programGenerator->getStage(QSSGShaderGeneratorStage::Fragment);
    }
    const QSSGRenderDefaultMaterial *material() { return m_currentMaterial; }
    bool hasTransparency() { return m_hasTransparency; }

    // TODO: !!! Remove
    void addFunction(QSSGStageGeneratorBase &generator, const QByteArray &functionName)
    {
        generator.addFunction(functionName);
    }

    void setupImageVariableNames(size_t imageIdx)
    {
        QByteArray imageStem = "image";
        char buf[16];
        qsnprintf(buf, 16, "%d", int(imageIdx));
        imageStem.append(buf);
        imageStem.append("_");

        m_imageSampler = imageStem;
        m_imageSampler.append("sampler");
        m_imageOffsets = imageStem;
        m_imageOffsets.append("offsets");
        m_imageRotations = imageStem;
        m_imageRotations.append("rotations");
        m_imageFragCoords = imageStem;
        m_imageFragCoords.append("uv_coords");
        m_imageSamplerSize = imageStem;
        m_imageSamplerSize.append("size");
    }

    QByteArray textureCoordVariableName(size_t uvSet)
    {
        QByteArray texCoordTemp = "varTexCoord";
        char buf[16];
        qsnprintf(buf, 16, "%d", int(uvSet));
        texCoordTemp.append(buf);
        return texCoordTemp;
    }

    ImageVariableNames getImageVariableNames(quint32 inIdx) override
    {
        setupImageVariableNames(inIdx);
        ImageVariableNames retval;
        retval.m_imageSampler = m_imageSampler;
        retval.m_imageFragCoords = m_imageFragCoords;
        return retval;
    }

    void addLocalVariable(QSSGStageGeneratorBase &inGenerator, const QByteArray &inName, const QByteArray &inType)
    {
        inGenerator << "    " << inType << " " << inName << ";\n";
    }

    QByteArray uvTransform()
    {
        QByteArray transform;
        transform = "    uTransform = vec3(" + m_imageRotations + ".x, " + m_imageRotations + ".y, " + m_imageOffsets + ".x);\n";
        transform += "    vTransform = vec3(" + m_imageRotations + ".z, " + m_imageRotations + ".w, " + m_imageOffsets + ".y);\n";
        return transform;
    }

    bool uvCoordsGenerated[32];
    void clearUVCoordsGen()
    {
        memset(uvCoordsGenerated, 0, sizeof(uvCoordsGenerated));
    }

    void generateImageUVCoordinates(QSSGVertexPipelineBase &vertexShader, quint32 idx, quint32 uvSet, QSSGRenderableImage &image) override
    {
        if (uvCoordsGenerated[idx])
            return;

        QSSGStageGeneratorBase &fragmentShader(fragmentGenerator());
        setupImageVariableNames(idx);
        QByteArray textureCoordName = textureCoordVariableName(uvSet);
        fragmentShader.addUniform(m_imageSampler, "sampler2D");
        vertexShader.addUniform(m_imageOffsets, "vec3");
        vertexShader.addUniform(m_imageRotations, "vec4");
        QByteArray uvTrans = uvTransform();
        if (image.m_image.m_mappingMode == QSSGRenderImage::MappingModes::Normal) {
            vertexShader << uvTrans;
            vertexShader.addOutgoing(m_imageFragCoords, "vec2");
            vertexShader.addFunction("getTransformedUVCoords");
            vertexShader.generateUVCoords(uvSet, key());
            m_imageTemp = m_imageFragCoords;
            m_imageTemp.append("temp");
            vertexShader << "    vec2 " << m_imageTemp << " = getTransformedUVCoords(vec3(" << textureCoordName << ", 1.0), uTransform, vTransform);\n";
            if (image.m_image.m_textureData.m_textureFlags.isInvertUVCoords())
                vertexShader << "    " << m_imageTemp << ".y = 1.0 - " << m_imageTemp << ".y;\n";

            vertexShader.assignOutput(m_imageFragCoords, m_imageTemp);
        } else {
            fragmentShader.addUniform(m_imageOffsets, "vec3");
            fragmentShader.addUniform(m_imageRotations, "vec4");
            fragmentShader << uvTrans;
            vertexShader.generateEnvMapReflection(key());
            addFunction(fragmentShader, "getTransformedUVCoords");
            fragmentShader << "    vec2 " << m_imageFragCoords << " = getTransformedUVCoords(environment_map_reflection, uTransform, vTransform);\n";
            if (image.m_image.m_textureData.m_textureFlags.isInvertUVCoords())
                fragmentShader << "    " << m_imageFragCoords << ".y = 1.0 - " << m_imageFragCoords << ".y;\n";
        }
        uvCoordsGenerated[idx] = true;
    }

    void generateImageUVSampler(quint32 idx, quint32 uvSet = 0)
    {
        QSSGStageGeneratorBase &fragmentShader(fragmentGenerator());
        setupImageVariableNames(idx);
        fragmentShader.addUniform(m_imageSampler, "sampler2D");
        m_imageFragCoords = textureCoordVariableName(uvSet);
        vertexGenerator().generateUVCoords(uvSet, key());
    }

    // TODO: !!! Remove
    void generateImageUVCoordinates(quint32 idx, QSSGRenderableImage &image, quint32 uvSet = 0)
    {
        generateImageUVCoordinates(vertexGenerator(), idx, uvSet, image);
    }

    void outputSpecularEquation(QSSGRenderDefaultMaterial::MaterialSpecularModel inSpecularModel,
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
            addFunction(fragmentShader, "specularBSDF");
            fragmentShader << "    global_specular_light.rgb += lightAttenuation * specularAmount"
                              " * specularBSDF(world_normal, -" << inLightDir << ".xyz, view_vector, " << inLightSpecColor << ".rgb, 2.56 / (roughnessAmount + 0.01)).rgb;\n";
            break;
        }
    }

    void outputDiffuseAreaLighting(QSSGStageGeneratorBase &infragmentShader, const QByteArray &inPos, const QByteArray &inLightPrefix)
    {
        m_normalizedDirection = inLightPrefix + "_areaDir";
        addLocalVariable(infragmentShader, m_normalizedDirection, "vec3");
        infragmentShader << "    lightAttenuation = calculateDiffuseAreaOld(" << m_lightDirection << ".xyz, " << m_lightPos << ".xyz, " << m_lightUp << ", " << m_lightRt << ", " << inPos << ", " << m_normalizedDirection << ");\n";
    }

    void outputSpecularAreaLighting(QSSGStageGeneratorBase &infragmentShader,
                                    const QByteArray &inPos,
                                    const QByteArray &inView,
                                    const QByteArray &inLightSpecColor)
    {
        addFunction(infragmentShader, "sampleAreaGlossyDefault");
        infragmentShader.addUniform("material_specular", "vec4");
        infragmentShader << "global_specular_light.rgb += " << inLightSpecColor << ".rgb * lightAttenuation * shadowFac * material_specular.rgb * specularAmount"
                            " * sampleAreaGlossyDefault(tanFrame, " << inPos << ", " << m_normalizedDirection << ", " << m_lightPos << ".xyz, " << m_lightRt << ".w, " << m_lightUp << ".w, " << inView << ", roughnessAmount).rgb;\n";
    }

    void addTranslucencyIrradiance(QSSGStageGeneratorBase &infragmentShader,
                                   QSSGRenderableImage *image,
                                   bool areaLight)
    {
        if (image == nullptr)
            return;

        addFunction(infragmentShader, "diffuseReflectionWrapBSDF");
        if (areaLight) {
            infragmentShader << "    global_diffuse_light.rgb += lightAttenuation * translucent_thickness_exp * diffuseReflectionWrapBSDF(-world_normal, " << m_normalizedDirection << ", " << m_lightColor << ".rgb, diffuseLightWrap).rgb;\n";
        } else {
            infragmentShader << "    global_diffuse_light.rgb += lightAttenuation * translucent_thickness_exp * diffuseReflectionWrapBSDF(-world_normal, -" << m_normalizedDirection << ", " << m_lightColor << ".rgb, diffuseLightWrap).rgb;\n";
        }
    }

    void setupShadowMapVariableNames(size_t lightIdx)
    {
        m_shadowMapStem = "shadowmap";
        m_shadowCubeStem = "shadowcube";
        char buf[16];
        qsnprintf(buf, 16, "%d", int(lightIdx));
        m_shadowMapStem.append(buf);
        m_shadowCubeStem.append(buf);
        m_shadowMatrixStem = m_shadowMapStem;
        m_shadowMatrixStem.append("_matrix");
        m_shadowCoordStem = m_shadowMapStem;
        m_shadowCoordStem.append("_coord");
        m_shadowControlStem = m_shadowMapStem;
        m_shadowControlStem.append("_control");
    }

    void addShadowMapContribution(QSSGStageGeneratorBase &inLightShader, quint32 lightIndex, QSSGRenderLight::Type inType)
    {
        setupShadowMapVariableNames(lightIndex);

        inLightShader.addInclude("shadowMapping.glsllib");
        if (inType == QSSGRenderLight::Type::Directional) {
            inLightShader.addUniform(m_shadowMapStem, "sampler2D");
        } else {
            inLightShader.addUniform(m_shadowCubeStem, "samplerCube");
        }
        inLightShader.addUniform(m_shadowControlStem, "vec4");
        inLightShader.addUniform(m_shadowMatrixStem, "mat4");

        if (inType != QSSGRenderLight::Type::Directional) {
            inLightShader << "    shadow_map_occl = sampleCubemap(" << m_shadowCubeStem << ", " << m_shadowControlStem << ", " << m_shadowMatrixStem << ", " << m_lightPos << ".xyz, varWorldPos, vec2(1.0, " << m_shadowControlStem << ".z));\n";
        } else {
            inLightShader << "    shadow_map_occl = sampleOrthographic(" << m_shadowMapStem << ", " << m_shadowControlStem << ", " << m_shadowMatrixStem << ", varWorldPos, vec2(1.0, " << m_shadowControlStem << ".z));\n";
        }
    }

    void maybeAddMaterialFresnel(QSSGStageGeneratorBase &fragmentShader, QSSGDataView<quint32> inKey, bool &fragmentHasSpecularAmount, bool hasMetalness)
    {
        if (m_defaultMaterialShaderKeyProperties.m_fresnelEnabled.getValue(inKey)) {
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
    void setupLightVariableNames(qint32 lightIdx, QSSGRenderLight &inLight)
    {
        Q_ASSERT(lightIdx > -1);
        if (m_lightsAsSeparateUniforms) {
            char buf[16];
            qsnprintf(buf, 16, "light_%d", int(lightIdx));
            QByteArray lightStem = buf;
            m_lightColor = lightStem;
            m_lightColor.append("_diffuse");
            m_lightDirection = lightStem;
            m_lightDirection.append("_direction");
            m_lightSpecularColor = lightStem;
            m_lightSpecularColor.append("_specular");
            if (inLight.m_lightType == QSSGRenderLight::Type::Point) {
                m_lightPos = lightStem;
                m_lightPos.append("_position");
                m_lightAttenuation = lightStem;
                m_lightAttenuation.append("_attenuation");
            } else if (inLight.m_lightType == QSSGRenderLight::Type::Area) {
                m_lightPos = lightStem;
                m_lightPos.append("_position");
                m_lightUp = lightStem;
                m_lightUp.append("_up");
                m_lightRt = lightStem;
                m_lightRt.append("_right");
            } else if (inLight.m_lightType == QSSGRenderLight::Type::Spot) {
                m_lightPos = lightStem;
                m_lightPos.append("_position");
                m_lightAttenuation = lightStem;
                m_lightAttenuation.append("_attenuation");
                m_lightConeAngle = lightStem;
                m_lightConeAngle.append("_coneAngle");
                m_lightInnerConeAngle = lightStem;
                m_lightInnerConeAngle.append("_innerConeAngle");
            }
        } else {
            QByteArray lightStem = "lights";
            char buf[16];
            qsnprintf(buf, 16, "[%d].", int(lightIdx));
            lightStem.append(buf);

            m_lightColor = lightStem;
            m_lightColor.append("diffuse");
            m_lightDirection = lightStem;
            m_lightDirection.append("direction");
            m_lightSpecularColor = lightStem;
            m_lightSpecularColor.append("specular");
            if (inLight.m_lightType == QSSGRenderLight::Type::Point) {
                m_lightPos = lightStem;
                m_lightPos.append("position");
                m_lightConstantAttenuation = lightStem;
                m_lightConstantAttenuation.append("constantAttenuation");
                m_lightLinearAttenuation = lightStem;
                m_lightLinearAttenuation.append("linearAttenuation");
                m_lightQuadraticAttenuation = lightStem;
                m_lightQuadraticAttenuation.append("quadraticAttenuation");
            } else if (inLight.m_lightType == QSSGRenderLight::Type::Area) {
                m_lightPos = lightStem;
                m_lightPos.append("position");
                m_lightUp = lightStem;
                m_lightUp.append("up");
                m_lightRt = lightStem;
                m_lightRt.append("right");
            } else if (inLight.m_lightType == QSSGRenderLight::Type::Spot) {
                m_lightPos = lightStem;
                m_lightPos.append("position");
                m_lightConstantAttenuation = lightStem;
                m_lightConstantAttenuation.append("constantAttenuation");
                m_lightLinearAttenuation = lightStem;
                m_lightLinearAttenuation.append("linearAttenuation");
                m_lightQuadraticAttenuation = lightStem;
                m_lightQuadraticAttenuation.append("quadraticAttenuation");
                m_lightConeAngle = lightStem;
                m_lightConeAngle.append("coneAngle");
                m_lightInnerConeAngle = lightStem;
                m_lightInnerConeAngle.append("innerConeAngle");
            }
        }
    }

//    void generateTextureSwizzle(QSSGRenderTextureSwizzleMode swizzleMode, QByteArray &texSwizzle, QByteArray &lookupSwizzle)
//    {
//        QSSGRenderContextTypes deprecatedContextFlags(QSSGRenderContextType::GL2 | QSSGRenderContextType::GLES2);

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

    void generateShadowMapOcclusion(quint32 lightIdx, bool inShadowEnabled, QSSGRenderLight::Type inType)
    {
        if (inShadowEnabled) {
            vertexGenerator().generateWorldPosition();
            addShadowMapContribution(fragmentGenerator(), lightIdx, inType);
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
            fragmentGenerator() << "    shadow_map_occl = 1.0;\n";
        }
    }

    // TODO: !!! Call directly
    void generateVertexShader()
    {
        // the pipeline opens/closes up the shaders stages
        vertexGenerator().beginVertexGeneration();
    }

    void addSpecularAmount(QSSGStageGeneratorBase &fragmentShader, bool &fragmentHasSpecularAmount, bool reapply = false)
    {
        if (!fragmentHasSpecularAmount)
            fragmentShader << "    vec3 specularAmount = specularBase * vec3(material_properties.z + material_properties.x * (1.0 - material_properties.z));\n";
        else if (reapply)
            fragmentShader << "    specularAmount = specularBase * vec3(material_properties.z + material_properties.x * (1.0 - material_properties.z));\n";
        fragmentHasSpecularAmount = true;
    }

    void generateFragmentShader(QSSGShaderDefaultMaterialKey &inKey)
    {
        const bool metalnessEnabled = material()->isMetalnessEnabled();
        const bool specularEnabled = material()->isSpecularEnabled();
        bool vertexColorsEnabled = material()->isVertexColorsEnabled();
        bool specularLightingEnabled = metalnessEnabled || specularEnabled;

        const auto &keyProps = m_defaultMaterialShaderKeyProperties;

        bool hasLighting = material()->hasLighting();
        bool isDoubleSided = keyProps.m_isDoubleSided.getValue(inKey);
        bool hasImage = m_firstImage != nullptr;

        bool hasIblProbe = keyProps.m_hasIbl.getValue(inKey);
        bool hasSpecMap = false;
        bool hasMetalMap = false;
        bool hasEnvMap = false;
        bool hasEmissiveMap = false;
        bool hasLightmaps = false;
        bool hasBaseColorMap = false;
        // Pull the bump out as
        QSSGRenderableImage *bumpImage = nullptr;
        quint32 imageIdx = 0;
        quint32 bumpImageIdx = 0;
        QSSGRenderableImage *specularAmountImage = nullptr;
        quint32 specularAmountImageIdx = 0;
        QSSGRenderableImage *roughnessImage = nullptr;
        quint32 roughnessImageIdx = 0;
        QSSGRenderableImage *metalnessImage = nullptr;
        quint32 metalnessImageIdx = 0;
        QSSGRenderableImage *occlusionImage = nullptr;
        quint32 occlusionImageIdx = 0;
        // normal mapping
        QSSGRenderableImage *normalImage = nullptr;
        quint32 normalImageIdx = 0;
        // translucency map
        QSSGRenderableImage *translucencyImage = nullptr;
        quint32 translucencyImageIdx = 0;
        // lightmaps
        QSSGRenderableImage *lightmapIndirectImage = nullptr;
        quint32 lightmapIndirectImageIdx = 0;
        QSSGRenderableImage *lightmapRadiosityImage = nullptr;
        quint32 lightmapRadiosityImageIdx = 0;
        QSSGRenderableImage *lightmapShadowImage = nullptr;
        quint32 lightmapShadowImageIdx = 0;

        QSSGRenderableImage *baseImage = nullptr;
        quint32 baseImageIdx = 0;

        // Use shared texcoord when transforms are identity
        QVector<QSSGRenderableImage *> identityImages;

        Q_UNUSED(lightmapShadowImage)
        Q_UNUSED(lightmapShadowImageIdx)

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

        // Reset uv cooordinate generation
        clearUVCoordsGen();

        for (QSSGRenderableImage *img = m_firstImage; img != nullptr; img = img->m_nextImage, ++imageIdx) {
            if (img->m_image.isImageTransformIdentity())
                identityImages.push_back(img);
            if (img->m_mapType == QSSGImageMapTypes::Specular) {
                hasSpecMap = true;
            } else if (img->m_mapType == QSSGImageMapTypes::BaseColor || img->m_mapType == QSSGImageMapTypes::Diffuse) {
                hasBaseColorMap = img->m_mapType == QSSGImageMapTypes::BaseColor;
                baseImage = img;
                baseImageIdx = imageIdx;
            } else if (img->m_mapType == QSSGImageMapTypes::Bump) {
                bumpImage = img;
                bumpImageIdx = imageIdx;
            } else if (img->m_mapType == QSSGImageMapTypes::SpecularAmountMap) {
                specularAmountImage = img;
                specularAmountImageIdx = imageIdx;
            } else if (img->m_mapType == QSSGImageMapTypes::Roughness) {
                roughnessImage = img;
                roughnessImageIdx = imageIdx;
            } else if (img->m_mapType == QSSGImageMapTypes::Metalness) {
                metalnessImage = img;
                metalnessImageIdx = imageIdx;
                hasMetalMap = true;
            } else if (img->m_mapType == QSSGImageMapTypes::Occlusion) {
                occlusionImage = img;
                occlusionImageIdx = imageIdx;
            } else if (img->m_mapType == QSSGImageMapTypes::Normal) {
                normalImage = img;
                normalImageIdx = imageIdx;
            } else if (img->m_image.m_mappingMode == QSSGRenderImage::MappingModes::Environment) {
                hasEnvMap = true;
            } else if (img->m_mapType == QSSGImageMapTypes::Translucency) {
                translucencyImage = img;
                translucencyImageIdx = imageIdx;
            } else if (img->m_mapType == QSSGImageMapTypes::Emissive) {
                hasEmissiveMap = true;
            } else if (img->m_mapType == QSSGImageMapTypes::LightmapIndirect) {
                lightmapIndirectImage = img;
                lightmapIndirectImageIdx = imageIdx;
                hasLightmaps = true;
            } else if (img->m_mapType == QSSGImageMapTypes::LightmapRadiosity) {
                lightmapRadiosityImage = img;
                lightmapRadiosityImageIdx = imageIdx;
                hasLightmaps = true;
            } else if (img->m_mapType == QSSGImageMapTypes::LightmapShadow) {
                lightmapShadowImage = img;
                lightmapShadowImageIdx = imageIdx;
                hasLightmaps = true;
            }
        }

        bool enableFresnel = keyProps.m_fresnelEnabled.getValue(inKey);
        bool enableSSAO = false;
        bool enableSSDO = false;
        bool enableShadowMaps = false;
        bool enableBumpNormal = normalImage || bumpImage;
        specularLightingEnabled |= specularAmountImage != nullptr;

        for (qint32 idx = 0; idx < m_currentFeatureSet.size(); ++idx) {
            const auto &name = m_currentFeatureSet.at(idx).name;
            if (name == QSSGShaderDefines::asString(QSSGShaderDefines::Ssao))
                enableSSAO = m_currentFeatureSet.at(idx).enabled;
            else if (name == QSSGShaderDefines::asString(QSSGShaderDefines::Ssdo))
                enableSSDO = m_currentFeatureSet.at(idx).enabled;
            else if (name == QSSGShaderDefines::asString(QSSGShaderDefines::Ssm))
                enableShadowMaps = m_currentFeatureSet.at(idx).enabled;
        }

        bool includeSSAOSSDOVars = enableSSAO || enableSSDO || enableShadowMaps;

        vertexGenerator().beginFragmentGeneration();
        QSSGStageGeneratorBase &fragmentShader(fragmentGenerator());
        QSSGVertexPipelineBase &vertexShader(vertexGenerator());

        // The fragment or vertex shaders may not use the material_properties or diffuse
        // uniforms in all cases but it is simpler to just add them and let the linker strip them.
        fragmentShader.addUniform("material_diffuse", "vec3");
        fragmentShader.addUniform("base_color", "vec4");
        fragmentShader.addUniform("material_properties", "vec4");

        // All these are needed for SSAO
        if (includeSSAOSSDOVars) {
            fragmentShader.addInclude("SSAOCustomMaterial.glsllib");
            // fragmentShader.AddUniform( "aoTexture", "sampler2D" );
        }

        if (hasIblProbe && hasLighting) {
            fragmentShader.addInclude("sampleProbe.glsllib");
        }

        if (hasLighting) {
            if (!m_lightsAsSeparateUniforms)
                addFunction(fragmentShader, "sampleLightVars");
            addFunction(fragmentShader, "diffuseReflectionBSDF");
        }

        if (hasLighting && hasLightmaps) {
            fragmentShader.addInclude("evalLightmaps.glsllib");
        }

        // view_vector, varWorldPos, world_normal are all used if there is a specular map
        // in addition to if there is specular lighting.  So they are lifted up here, always
        // generated.
        // we rely on the linker to strip out what isn't necessary instead of explicitly stripping
        // it for code simplicity.
        if (hasImage && hasLighting) {
            fragmentShader.append("    vec3 uTransform;");
            fragmentShader.append("    vec3 vTransform;");
        }

        if (includeSSAOSSDOVars || hasSpecMap || hasMetalMap || hasLighting || hasEnvMap || enableFresnel || hasIblProbe || enableBumpNormal) {
            vertexShader.generateViewVector();
            vertexShader.generateWorldNormal(inKey);
            vertexShader.generateWorldPosition();
        }
        if (includeSSAOSSDOVars || specularEnabled || metalnessEnabled || hasIblProbe || enableBumpNormal)
            vertexShader.generateVarTangentAndBinormal(inKey);

        if (vertexColorsEnabled)
            vertexShader.generateVertexColor(inKey);
        else
            fragmentShader.append("    vec3 vertColor = vec3(1.0);");

        // You do bump or normal mapping but not both
        if (bumpImage != nullptr) {
            generateImageUVCoordinates(bumpImageIdx, *bumpImage);
            fragmentShader.addUniform("bumpAmount", "float");

            fragmentShader.addUniform(m_imageSamplerSize, "vec2");
            fragmentShader.addInclude("defaultMaterialBumpNoLod.glsllib");
            fragmentShader << "    world_normal = defaultMaterialBumpNoLod(" << m_imageSampler << ", bumpAmount, " << m_imageFragCoords << ", tangent, binormal, world_normal, " << m_imageSamplerSize << ");\n";
            // Do gram schmidt
            fragmentShader << "    binormal = normalize(cross(world_normal, tangent));\n";
            fragmentShader << "    tangent = normalize(cross(binormal, world_normal));\n";

        } else if (normalImage != nullptr) {
            generateImageUVCoordinates(normalImageIdx, *normalImage);

            fragmentShader.addFunction("sampleNormalTexture");
            fragmentShader.addUniform("bumpAmount", "float");

            fragmentShader << "    world_normal = sampleNormalTexture(" << m_imageSampler << ", bumpAmount, " << m_imageFragCoords << ", tangent, binormal, world_normal);\n";
            // Do gram schmidt
            fragmentShader << "    binormal = normalize(cross(world_normal, tangent));\n";
            fragmentShader << "    tangent = normalize(cross(binormal, world_normal));\n";
        }

        fragmentShader.addUniform("normalAdjustViewportFactor", "float");
        if (hasLighting && isDoubleSided) {
            fragmentShader.addInclude("doubleSided.glsllib");
            fragmentShader.append("    world_normal = adjustNormalForFace(world_normal, varWorldPos, normalAdjustViewportFactor);\n");
        }

        if (includeSSAOSSDOVars || specularEnabled || metalnessEnabled || hasIblProbe || enableBumpNormal)
            fragmentShader << "    mat3 tanFrame = mat3(tangent, binormal, world_normal);\n";

        bool fragmentHasSpecularAmount = false;

        if (hasEmissiveMap)
            fragmentShader.append("    vec3 global_emission = material_diffuse.rgb;");

        if (specularLightingEnabled)
            fragmentShader.append("    vec3 specularBase;");

        fragmentShader << "    vec3 diffuseColor = base_color.rgb;\n";
        if (baseImage) {
            QByteArray texSwizzle;
            QByteArray lookupSwizzle;

            if (identityImages.contains(baseImage))
                generateImageUVSampler(baseImageIdx);
            else
                generateImageUVCoordinates(baseImageIdx, *baseImage);

//            if (baseImage->m_image.m_textureData.m_texture) {
//                // not supported for rhi
//                generateTextureSwizzle(baseImage->m_image.m_textureData.m_texture->textureSwizzleMode(), texSwizzle, lookupSwizzle);
//            }

            fragmentShader << "    vec4 base_texture_color" << texSwizzle << " = texture2D(" << m_imageSampler << ", " << m_imageFragCoords << ")" << lookupSwizzle << ";\n";
            fragmentShader << "    diffuseColor *= base_texture_color.rgb;\n";
            // we use base color with specular
            if (specularLightingEnabled)
                fragmentShader << "    specularBase = base_texture_color.rgb * base_color.rgb;\n";
        } else if (specularLightingEnabled) {
            fragmentShader << "    specularBase = base_color.rgb;\n";
        }

        if (hasLighting) {
            fragmentShader.addUniform("light_ambient_total", "vec3");

            fragmentShader.append("    vec4 global_diffuse_light = vec4(light_ambient_total.rgb * diffuseColor, 1.0);");
            fragmentShader.append("    vec3 global_specular_light = vec3(0.0, 0.0, 0.0);");
            fragmentShader.append("    float shadow_map_occl = 1.0;");

            if (specularLightingEnabled) {
                vertexShader.generateViewVector();
                fragmentShader.addUniform("material_properties", "vec4");
                addSpecularAmount(fragmentShader, fragmentHasSpecularAmount);
            }

            if (lightmapIndirectImage != nullptr) {
                if (identityImages.contains(lightmapIndirectImage))
                    generateImageUVSampler(lightmapIndirectImageIdx, 1);
                else
                    generateImageUVCoordinates(lightmapIndirectImageIdx, *lightmapIndirectImage, 1);


                fragmentShader << "    vec4 indirect_light = texture2D(" << m_imageSampler << ", " << m_imageFragCoords << ");\n";
                fragmentShader << "    global_diffuse_light += indirect_light;\n";
                if (specularLightingEnabled)
                    fragmentShader << "    global_specular_light += indirect_light.rgb * specularAmount;\n";
            }

            if (lightmapRadiosityImage != nullptr) {
                if (identityImages.contains(lightmapRadiosityImage))
                    generateImageUVSampler(lightmapRadiosityImageIdx, 1);
                else
                    generateImageUVCoordinates(lightmapRadiosityImageIdx, *lightmapRadiosityImage, 1);

                fragmentShader << "    vec4 direct_light = texture2D(" << m_imageSampler << ", " << m_imageFragCoords << ");\n";
                fragmentShader << "    global_diffuse_light += direct_light;\n";
                if (specularLightingEnabled)
                    fragmentShader << "    global_specular_light += direct_light.rgb * specularAmount;\n";
            }

            if (translucencyImage != nullptr) {
                fragmentShader.addUniform("translucentFalloff", "float");
                fragmentShader.addUniform("diffuseLightWrap", "float");

                if (identityImages.contains(translucencyImage))
                    generateImageUVSampler(translucencyImageIdx);
                else
                    generateImageUVCoordinates(translucencyImageIdx, *translucencyImage);

                const auto &channelProps = keyProps.m_textureChannels[QSSGShaderDefaultMaterialKeyProperties::TranslucencyChannel];
                fragmentShader << "    float translucent_depth_range = texture2D(" << m_imageSampler
                               << ", " << m_imageFragCoords << ")" << channelStr(channelProps, inKey) << ";\n";
                fragmentShader << "    float translucent_thickness = translucent_depth_range * translucent_depth_range;\n";
                fragmentShader << "    float translucent_thickness_exp = exp(translucent_thickness * translucentFalloff);\n";
            }

            fragmentShader.append("    float lightAttenuation = 1.0;");

            addLocalVariable(fragmentShader, "aoFactor", "float");

            if (hasLighting && enableSSAO)
                fragmentShader.append("    aoFactor = customMaterialAO();");
            else
                fragmentShader.append("    aoFactor = 1.0;");

            addLocalVariable(fragmentShader, "shadowFac", "float");

            // Fragment lighting means we can perhaps attenuate the specular amount by a texture
            // lookup.
            if (specularAmountImage) {
                addSpecularAmount(fragmentShader, fragmentHasSpecularAmount);

                if (identityImages.contains(specularAmountImage))
                    generateImageUVSampler(specularAmountImageIdx);
                else
                    generateImageUVCoordinates(specularAmountImageIdx, *specularAmountImage);
                fragmentShader << "    specularBase *= texture2D(" << m_imageSampler << ", " << m_imageFragCoords << ").rgb;\n";
            }

            fragmentShader << "    float roughnessAmount = material_properties.y;\n";
            if (specularLightingEnabled && roughnessImage) {
                const auto &channelProps = keyProps.m_textureChannels[QSSGShaderDefaultMaterialKeyProperties::RoughnessChannel];
                if (identityImages.contains(roughnessImage))
                    generateImageUVSampler(roughnessImageIdx);
                else
                    generateImageUVCoordinates(roughnessImageIdx, *roughnessImage);
                fragmentShader << "    roughnessAmount *= texture2D(" << m_imageSampler << ", "
                               << m_imageFragCoords << ")" << channelStr(channelProps, inKey) << ";\n";
            }

            fragmentShader << "    float metalnessAmount = material_properties.z;\n";
            if (specularLightingEnabled && metalnessImage) {
                const auto &channelProps = keyProps.m_textureChannels[QSSGShaderDefaultMaterialKeyProperties::MetalnessChannel];
                if (identityImages.contains(metalnessImage))
                    generateImageUVSampler(metalnessImageIdx);
                else
                    generateImageUVCoordinates(metalnessImageIdx, *metalnessImage);
                fragmentShader << "    float sampledMetalness = texture2D(" << m_imageSampler << ", "
                               << m_imageFragCoords << ")" << channelStr(channelProps, inKey) << ";\n";
                fragmentShader << "    metalnessAmount = clamp(metalnessAmount * sampledMetalness, 0.0, 1.0);\n";
                addSpecularAmount(fragmentShader, fragmentHasSpecularAmount, true);
            }
            fragmentShader.addInclude("defaultMaterialFresnel.glsllib");
            fragmentShader << "    float ds = dielectricSpecular(material_properties.w);\n";
            fragmentShader << "    diffuseColor *= (1.0 - ds) * (1.0 - metalnessAmount);\n";
            if (!hasBaseColorMap && material()->type == QSSGRenderGraphObject::Type::PrincipledMaterial && specularLightingEnabled) {
                fragmentShader << "    float lum = dot(base_color.rgb, vec3(0.21, 0.72, 0.07));\n"
                                  "    specularBase += (lum > 0.0) ? (base_color.rgb) / lum : vec3(1.0);\n";
            }

            if (specularLightingEnabled)
                maybeAddMaterialFresnel(fragmentShader, inKey, fragmentHasSpecularAmount, metalnessEnabled);

            // Iterate through all lights
            Q_ASSERT(m_lights.size() < INT32_MAX);
            for (qint32 lightIdx = 0; lightIdx < m_lights.size(); ++lightIdx) {
                QSSGRenderLight *lightNode = m_lights[lightIdx];
                setupLightVariableNames(lightIdx, *lightNode);
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
                    if (m_lightsAsSeparateUniforms) {
                        fragmentShader.addUniform(m_lightDirection, "vec4");
                        fragmentShader.addUniform(m_lightColor, "vec4");
                    }

                    if (enableSSDO)
                        fragmentShader << "    shadowFac = customMaterialShadow(" << m_lightDirection << ".xyz, varWorldPos);\n";
                    else
                        fragmentShader << "    shadowFac = 1.0;\n";

                    generateShadowMapOcclusion(lightIdx, enableShadowMaps && isShadow, lightNode->m_lightType);

                    if (specularLightingEnabled && enableShadowMaps && isShadow)
                        fragmentShader << "    lightAttenuation *= shadow_map_occl;\n";

                    fragmentShader << "    global_diffuse_light.rgb += diffuseColor * shadowFac * shadow_map_occl * diffuseReflectionBSDF(world_normal, -" << m_lightDirection << ".xyz, " << m_lightColor << ".rgb).rgb;\n";

                    if (specularLightingEnabled) {
                        if (m_lightsAsSeparateUniforms)
                            fragmentShader.addUniform(m_lightSpecularColor, "vec4");
                        outputSpecularEquation(material()->specularModel, fragmentShader, m_lightDirection, m_lightSpecularColor);
                    }
                } else if (isArea) {
                    if (m_lightsAsSeparateUniforms) {
                        fragmentShader.addUniform(m_lightColor, "vec4");
                        fragmentShader.addUniform(m_lightPos, "vec4");
                        fragmentShader.addUniform(m_lightDirection, "vec4");
                        fragmentShader.addUniform(m_lightUp, "vec4");
                        fragmentShader.addUniform(m_lightRt, "vec4");
                    } else {
                        addFunction(fragmentShader, "areaLightVars");
                    }
                    addFunction(fragmentShader, "calculateDiffuseAreaOld");
                    vertexShader.generateWorldPosition();
                    generateShadowMapOcclusion(lightIdx, enableShadowMaps && isShadow, lightNode->m_lightType);

                    m_normalizedDirection = tempStr;
                    m_normalizedDirection.append("_Frame");

                    addLocalVariable(fragmentShader, m_normalizedDirection, "mat3");
                    fragmentShader << "    " << m_normalizedDirection << " = mat3(" << m_lightRt << ".xyz, " << m_lightUp << ".xyz, -" << m_lightDirection << ".xyz);\n";

                    if (enableSSDO)
                        fragmentShader << "    shadowFac = shadow_map_occl * customMaterialShadow(" << m_lightDirection << ".xyz, varWorldPos);\n";
                    else
                        fragmentShader << "    shadowFac = shadow_map_occl;\n";

                    if (specularLightingEnabled) {
                        vertexShader.generateViewVector();
                        if (m_lightsAsSeparateUniforms)
                            fragmentShader.addUniform(m_lightSpecularColor, "vec4");
                        outputSpecularAreaLighting(fragmentShader, "varWorldPos", "view_vector", m_lightSpecularColor);
                    }

                    outputDiffuseAreaLighting(fragmentShader, "varWorldPos", tempStr);
                    fragmentShader << "    lightAttenuation *= shadowFac;\n";

                    addTranslucencyIrradiance(fragmentShader, translucencyImage, true);

                    fragmentShader << "    global_diffuse_light.rgb += diffuseColor * lightAttenuation * diffuseReflectionBSDF(world_normal, " << m_normalizedDirection << ", " << m_lightColor << ".rgb).rgb;\n";
                } else {
                    vertexShader.generateWorldPosition();

                    if (m_lightsAsSeparateUniforms) {
                        fragmentShader.addUniform(m_lightColor, "vec4");
                        fragmentShader.addUniform(m_lightPos, "vec4");
                        if (isSpot)
                            fragmentShader.addUniform(m_lightDirection, "vec4");
                    }

                    m_relativeDirection = tempStr;
                    m_relativeDirection.append("_relativeDirection");

                    m_normalizedDirection = m_relativeDirection;
                    m_normalizedDirection.append("_normalized");

                    m_relativeDistance = tempStr;
                    m_relativeDistance.append("_distance");

                    fragmentShader << "    vec3 " << m_relativeDirection << " = varWorldPos - " << m_lightPos << ".xyz;\n"
                                      "    float " << m_relativeDistance << " = length(" << m_relativeDirection << ");\n"
                                      "    vec3 " << m_normalizedDirection << " = " << m_relativeDirection << " / " << m_relativeDistance << ";\n";

                    if (isSpot) {
                        m_spotAngle = tempStr;
                        m_spotAngle.append("_spotAngle");

                        if (m_lightsAsSeparateUniforms) {
                            fragmentShader.addUniform(m_lightConeAngle, "float");
                            fragmentShader.addUniform(m_lightInnerConeAngle, "float");
                        }
                        fragmentShader << "    float " << m_spotAngle << " = dot(" << m_normalizedDirection
                                       << ", normalize(vec3(" << m_lightDirection << ")));\n";
                        fragmentShader << "    if (" << m_spotAngle << " > " << m_lightConeAngle << ") {\n";
                    }

                    generateShadowMapOcclusion(lightIdx, enableShadowMaps && isShadow, lightNode->m_lightType);

                    if (enableSSDO) {
                        fragmentShader << "    shadowFac = shadow_map_occl * customMaterialShadow(" << m_normalizedDirection << ", varWorldPos);\n";
                    } else {
                        fragmentShader << "    shadowFac = shadow_map_occl;\n";
                    }

                    addFunction(fragmentShader, "calculatePointLightAttenuation");

                    if (m_lightsAsSeparateUniforms) {
                        fragmentShader.addUniform(m_lightAttenuation, "vec3");
                        fragmentShader << "    lightAttenuation = shadowFac * calculatePointLightAttenuation(vec3(" << m_lightAttenuation << ".x, " << m_lightAttenuation << ".y, " << m_lightAttenuation << ".z), " << m_relativeDistance << ");\n";
                    } else {
                        fragmentShader << "    lightAttenuation = shadowFac * calculatePointLightAttenuation(vec3(" << m_lightConstantAttenuation << ", " << m_lightLinearAttenuation << ", " << m_lightQuadraticAttenuation << "), " << m_relativeDistance << ");\n";
                    }

                    addTranslucencyIrradiance(fragmentShader, translucencyImage, false);

                    if (isSpot) {
                        fragmentShader << "    float spotFactor = smoothstep(" << m_lightConeAngle
                                       << ", " << m_lightInnerConeAngle << ", " << m_spotAngle
                                       << ");\n";
                        fragmentShader << "    global_diffuse_light.rgb += diffuseColor * spotFactor * ";
                    } else {
                        fragmentShader << "    global_diffuse_light.rgb += diffuseColor * ";
                    }
                    fragmentShader << "lightAttenuation * diffuseReflectionBSDF(world_normal, -"
                                   << m_normalizedDirection << ", "
                                   << m_lightColor << ".rgb).rgb;\n";

                    if (specularLightingEnabled) {
                        if (m_lightsAsSeparateUniforms)
                            fragmentShader.addUniform(m_lightSpecularColor, "vec4");
                        outputSpecularEquation(material()->specularModel, fragmentShader, m_normalizedDirection, m_lightSpecularColor);
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
            fragmentShader << "    global_diffuse_light = vec4(global_diffuse_light.rgb * aoFactor, objectOpacity * base_color.a);\n"
                              "    global_specular_light = vec3(global_specular_light.rgb);\n";
        } else { // no lighting.
            fragmentShader << "    vec4 global_diffuse_light = vec4(0.0, 0.0, 0.0, objectOpacity * base_color.a);\n"
                              "    vec3 global_specular_light = vec3(0.0, 0.0, 0.0);\n";

            // We still have specular maps and such that could potentially use the fresnel variable.
            maybeAddMaterialFresnel(fragmentShader, inKey, fragmentHasSpecularAmount, false);
        }

        if (!hasEmissiveMap)
            fragmentShader << "    global_diffuse_light.rgb += diffuseColor.rgb * material_diffuse.rgb;\n";

        // since we already modulate our material diffuse color
        // into the light color we will miss it entirely if no IBL
        // or light is used
        if (hasLightmaps && !(m_lights.size() || hasIblProbe))
            fragmentShader << "    global_diffuse_light.rgb *= diffuseColor.rgb;\n";

        if (hasLighting && hasIblProbe) {
            vertexShader.generateWorldNormal(inKey);

            fragmentShader << "    global_diffuse_light.rgb += diffuseColor.rgb * aoFactor * sampleDiffuse(tanFrame).rgb;\n";

            if (specularLightingEnabled) {
                fragmentShader.addUniform("material_specular", "vec4");
                fragmentShader << "    global_specular_light.rgb += specularAmount * vec3(material_specular.rgb) * sampleGlossy(tanFrame, view_vector, roughnessAmount).rgb;\n";
            }
        }

        if (hasImage) {
            fragmentShader.append("    vec4 texture_color;");
            quint32 idx = 0;
            for (QSSGRenderableImage *image = m_firstImage; image; image = image->m_nextImage, ++idx) {
                // Various maps are handled on a different locations
                if (image->m_mapType == QSSGImageMapTypes::Bump || image->m_mapType == QSSGImageMapTypes::Normal
                    || image->m_mapType == QSSGImageMapTypes::SpecularAmountMap
                    || image->m_mapType == QSSGImageMapTypes::Roughness || image->m_mapType == QSSGImageMapTypes::Translucency
                    || image->m_mapType == QSSGImageMapTypes::Metalness || image->m_mapType == QSSGImageMapTypes::Occlusion
                    || image->m_mapType == QSSGImageMapTypes::LightmapIndirect
                    || image->m_mapType == QSSGImageMapTypes::LightmapRadiosity) {
                    continue;
                }

                QByteArray texSwizzle;
                QByteArray lookupSwizzle;

                if (identityImages.contains(image))
                    generateImageUVSampler(idx);
                else
                    generateImageUVCoordinates(idx, *image);

//                if (image->m_image.m_textureData.m_texture) {
//                    // not supported for rhi
//                    generateTextureSwizzle(image->m_image.m_textureData.m_texture->textureSwizzleMode(), texSwizzle, lookupSwizzle);
//                }

                fragmentShader << "    texture_color" << texSwizzle << " = texture2D(" << m_imageSampler << ", " << m_imageFragCoords << ")" << lookupSwizzle << ";\n";

                if (image->m_image.m_textureData.m_textureFlags.isPreMultiplied())
                    fragmentShader << "    texture_color.rgb = texture_color.a > 0.0 ? texture_color.rgb / texture_color.a : vec3(0.0);\n";

                // These mapping types honestly don't make a whole ton of sense to me.
                switch (image->m_mapType) {
                case QSSGImageMapTypes::BaseColor:
                    // color already taken care of
                    if (material()->alphaMode == QSSGRenderDefaultMaterial::MaterialAlphaMode::Mask) {
                        // The rendered output is either fully opaque or fully transparent depending on the alpha
                        // value and the specified alpha cutoff value.
                        fragmentShader.addUniform("alphaCutoff", "float");
                        fragmentShader << "    if ((texture_color.a * base_color.a) < alphaCutoff) {\n"
                                          "        fragOutput = vec4(0);\n"
                                          "        return;\n"
                                          "    }\n";
                    } else if (material()->alphaMode != QSSGRenderDefaultMaterial::MaterialAlphaMode::Opaque) {
                        // Blend && Default
                        // Use the alpha channel of base color
                        fragmentShader << "    global_diffuse_light.a *= texture_color.a * base_color.a;\n";
                    }

                    break;
                case QSSGImageMapTypes::Diffuse: // assume images are premultiplied.
                    // color already taken care of
                    fragmentShader.append("    global_diffuse_light.a *= base_color.a * texture_color.a;");
                    break;
                case QSSGImageMapTypes::LightmapShadow:
                    // We use image offsets.z to switch between incoming premultiplied textures or
                    // not premultiplied textures.
                    // If Z is 1, then we assume the incoming texture is already premultiplied, else
                    // we just read the rgb value.
                    fragmentShader.append("    global_diffuse_light *= texture_color;");
                    break;
                case QSSGImageMapTypes::Specular:
                    fragmentShader.addUniform("material_specular", "vec4");
                    if (fragmentHasSpecularAmount) {
                        fragmentShader.append("    global_specular_light.rgb += specularAmount * texture_color.rgb * material_specular.rgb;");
                    } else {
                        fragmentShader.append("    global_specular_light.rgb += texture_color.rgb * material_specular.rgb;");
                    }
                    fragmentShader.append("    global_diffuse_light.a *= texture_color.a;");
                    break;
                case QSSGImageMapTypes::Opacity:
                {
                    const auto &channelProps = keyProps.m_textureChannels[QSSGShaderDefaultMaterialKeyProperties::OpacityChannel];
                    fragmentShader << "    global_diffuse_light.a *= texture_color" << channelStr(channelProps, inKey) << ";\n";
                    break;
                }
                case QSSGImageMapTypes::Emissive:
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
            if (identityImages.contains(occlusionImage))
                generateImageUVSampler(occlusionImageIdx);
            else
                generateImageUVCoordinates(occlusionImageIdx, *occlusionImage);
            fragmentShader << "    float ao = texture2D(" << m_imageSampler << ", "
                           << m_imageFragCoords << ")" << channelStr(channelProps, inKey) << ";\n";
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
        fragmentShader.append("    fragOutput = vec4(clamp(vertColor * global_diffuse_light.rgb + global_specular_light.rgb, 0.0, 65519.0), global_diffuse_light.a);");

    }

    QSSGRef<QSSGRhiShaderStages> generateMaterialRhiShader(const QByteArray &inShaderPrefix)
    {
        // build a string that allows us to print out the shader we are generating to the log.
        // This is time consuming but I feel like it doesn't happen all that often and is very
        // useful to users
        // looking at the log file.

        QByteArray generatedShaderString;
        generatedShaderString = inShaderPrefix;

        QSSGShaderDefaultMaterialKey theKey(key());
        theKey.toString(generatedShaderString, m_defaultMaterialShaderKeyProperties);

        m_lightsAsSeparateUniforms = false;

        generateVertexShader();
        generateFragmentShader(theKey);

        vertexGenerator().endVertexGeneration(false);
        vertexGenerator().endFragmentGeneration(false);

        return programGenerator()->compileGeneratedRhiShader(generatedShaderString, QSSGShaderCacheProgramFlags(), m_currentFeatureSet);
    }

    QSSGRef<QSSGRhiShaderStages> generateRhiShaderStages(const QSSGRenderGraphObject &inMaterial,
                                                         QSSGShaderDefaultMaterialKey inShaderDescription,
                                                         QSSGVertexPipelineBase &inVertexPipeline,
                                                         const ShaderFeatureSetList &inFeatureSet,
                                                         const QVector<QSSGRenderLight *> &inLights,
                                                         QSSGRenderableImage *inFirstImage,
                                                         bool inHasTransparency,
                                                         const QByteArray &inVertexPipelineName,
                                                         const QByteArray &) override
    {
        Q_ASSERT(inMaterial.type == QSSGRenderGraphObject::Type::DefaultMaterial || inMaterial.type == QSSGRenderGraphObject::Type::PrincipledMaterial);
        m_currentMaterial = static_cast<const QSSGRenderDefaultMaterial *>(&inMaterial);
        m_currentKey = &inShaderDescription;
        m_currentPipeline = &inVertexPipeline;
        m_currentFeatureSet = inFeatureSet;
        m_lights = inLights;
        m_firstImage = inFirstImage;
        m_hasTransparency = inHasTransparency;

        return generateMaterialRhiShader(inVertexPipelineName);
    }

    void setRhiImageShaderVariables(const QSSGRef<QSSGRhiShaderStagesWithResources> &inShader, QSSGRenderableImage &inImage, quint32 idx)
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
        setupImageVariableNames(idx);

        inShader->setUniform(m_imageRotations, &rotations, sizeof(rotations));
        inShader->setUniform(m_imageOffsets, &offsets, sizeof(offsets));
    }

    void setRhiMaterialProperties(QSSGRef<QSSGRhiShaderStagesWithResources> &shaders,
                                  QSSGRhiGraphicsPipelineState *inPipelineState,
                                  const QSSGRenderGraphObject &inMaterial,
                                  const QVector2D &inCameraVec,
                                  const QMatrix4x4 &inModelViewProjection,
                                  const QMatrix3x3 &inNormalMatrix,
                                  const QMatrix4x4 &inGlobalTransform,
                                  QSSGRenderableImage *inFirstImage,
                                  float inOpacity,
                                  const QSSGLayerGlobalRenderProperties &inRenderProperties,
                                  bool receivesShadows) override
    {
        Q_UNUSED(inPipelineState);
        Q_UNUSED(receivesShadows);

        const QSSGRenderDefaultMaterial &theMaterial(static_cast<const QSSGRenderDefaultMaterial &>(inMaterial));
        Q_ASSERT(inMaterial.type == QSSGRenderGraphObject::Type::DefaultMaterial || inMaterial.type == QSSGRenderGraphObject::Type::PrincipledMaterial);

        m_shadowMapManager = inRenderProperties.shadowMapManager;

        QSSGRenderCamera &theCamera(inRenderProperties.camera);

        const QVector3D camGlobalPos = theCamera.getGlobalPos();
        shaders->setUniform(QByteArrayLiteral("cameraPosition"), &camGlobalPos, 3 * sizeof(float));
        shaders->setUniform(QByteArrayLiteral("cameraDirection"), &inRenderProperties.cameraDirection, 3 * sizeof(float));

        const QMatrix4x4 clipSpaceCorrMatrix = m_renderContext->rhiContext()->rhi()->clipSpaceCorrMatrix();
        QMatrix4x4 viewProj;
        theCamera.calculateViewProjectionMatrix(viewProj);
        viewProj = clipSpaceCorrMatrix * viewProj;
        shaders->setUniform(QByteArrayLiteral("viewProjectionMatrix"), viewProj.constData(), 16 * sizeof(float));

        const QMatrix4x4 viewMatrix = theCamera.globalTransform.inverted();
        shaders->setUniform(QByteArrayLiteral("viewMatrix"), viewMatrix.constData(), 16 * sizeof(float));

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
        shaders->setUniform(QByteArrayLiteral("normalAdjustViewportFactor"), &normalVpFactor, sizeof(float));

        QVector3D theLightAmbientTotal = QVector3D(0, 0, 0);
        shaders->resetLights(QSSGRhiShaderStagesWithResources::LightBuffer0);
        shaders->resetShadowMaps();

        float zero[16];
        memset(zero, 0, sizeof(zero));

        for (quint32 lightIdx = 0, shadowMapIdx = 0, lightEnd = inRenderProperties.lights.size();
             lightIdx < lightEnd && lightIdx < QSSG_MAX_NUM_LIGHTS; ++lightIdx)
        {
            QSSGRenderLight *theLight(inRenderProperties.lights[lightIdx]);
            QSSGShaderLightProperties &theLightProperties(shaders->addLight(QSSGRhiShaderStagesWithResources::LightBuffer0));
            float brightness = aux::translateBrightness(theLight->m_brightness);

            theLightProperties.lightColor = theLight->m_diffuseColor * brightness;
            theLightProperties.lightData.specular = QVector4D(theLight->m_specularColor * brightness, 1.0);
            theLightProperties.lightData.direction = QVector4D(inRenderProperties.lightDirections[lightIdx], 1.0);

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

                setupShadowMapVariableNames(lightIdx);

                if (theLight->m_lightType != QSSGRenderLight::Type::Directional) {
                    theShadowMapProperties.shadowMapTexture = pEntry->m_rhiDepthCube;
                    theShadowMapProperties.shadowMapTextureUniformName = m_shadowCubeStem;
                    if (receivesShadows)
                        shaders->setUniform(m_shadowMatrixStem, pEntry->m_lightView.constData(), 16 * sizeof(float));
                    else
                        shaders->setUniform(m_shadowMatrixStem, zero, 16 * sizeof(float));
                } else {
                    theShadowMapProperties.shadowMapTexture = pEntry->m_rhiDepthMap;
                    theShadowMapProperties.shadowMapTextureUniformName = m_shadowMapStem;
                    if (receivesShadows) {
                        // add fixed scale bias matrix
                        const QMatrix4x4 bias = {
                            0.5, 0.0, 0.0, 0.5,
                            0.0, 0.5, 0.0, 0.5,
                            0.0, 0.0, 0.5, 0.5,
                            0.0, 0.0, 0.0, 1.0 };
                        const QMatrix4x4 m = bias * pEntry->m_lightVP;
                        shaders->setUniform(m_shadowMatrixStem, m.constData(), 16 * sizeof(float));
                    } else {
                        shaders->setUniform(m_shadowMatrixStem, zero, 16 * sizeof(float));
                    }
                }

                if (receivesShadows) {
                    const QVector4D shadowControl(theLight->m_shadowBias,
                                                  theLight->m_shadowFactor,
                                                  theLight->m_shadowMapFar,
                                                  inRenderProperties.isYUpInFramebuffer ? 0.0f : 1.0f);
                    shaders->setUniform(m_shadowControlStem, &shadowControl, 4 * sizeof(float));
                } else {
                    shaders->setUniform(m_shadowControlStem, zero, 4 * sizeof(float));
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
        shaders->setUniform(QByteArrayLiteral("modelViewProjection"), mvp.constData(), 16 * sizeof(float));

        // mat3 is still 4 floats per column in the uniform buffer (but there
        // is no 4th column), so 48 bytes altogether, not 36 or 64.
        float normalMatrix[12];
        memcpy(normalMatrix, inNormalMatrix.constData(), 3 * sizeof(float));
        memcpy(normalMatrix + 4, inNormalMatrix.constData() + 3, 3 * sizeof(float));
        memcpy(normalMatrix + 8, inNormalMatrix.constData() + 6, 3 * sizeof(float));
        shaders->setUniform(QByteArrayLiteral("normalMatrix"), normalMatrix, 12 * sizeof(float));

        shaders->setUniform(QByteArrayLiteral("modelMatrix"), inGlobalTransform.constData(), 16 * sizeof(float));

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

                shaders->setUniform(QByteArrayLiteral("lightProbeRotation"), &rotations, 4 * sizeof(float));
                shaders->setUniform(QByteArrayLiteral("lightProbeOffset"), &offsets, 4 * sizeof(float));

                if ((!theMaterial.iblProbe) && (inRenderProperties.probeFOV < 180.f)) {
                    QVector4D opts(0.01745329251994329547f * inRenderProperties.probeFOV, 0.0f, 0.0f, 0.0f);
                    shaders->setUniform(QByteArrayLiteral("lightProbeOptions"), &opts, 4 * sizeof(float));
                }

                QVector4D emptyProps2(0.0f, 0.0f, 0.0f, 0.0f);
                shaders->setUniform(QByteArrayLiteral("lightProbe2Properties"), &emptyProps2, 4 * sizeof(float));

                QVector4D props(0.0f, 0.0f, inRenderProperties.probeHorizon, inRenderProperties.probeBright * 0.01f);
                shaders->setUniform(QByteArrayLiteral("lightProbeProperties"), &props, 4 * sizeof(float));
                shaders->setLightProbeTexture(theLightProbe->m_textureData.m_rhiTexture, theHorzLightProbeTilingMode, theVertLightProbeTilingMode);
        } else {
            // no lightprobe
            QVector4D emptyProps(0.0f, 0.0f, -1.0f, 0.0f);
            shaders->setUniform(QByteArrayLiteral("lightProbeProperties"), &emptyProps, 4 * sizeof(float));

            QVector4D emptyProps2(0.0f, 0.0f, 0.0f, 0.0f);
            shaders->setUniform(QByteArrayLiteral("lightProbe2Properties"), &emptyProps2, 4 * sizeof(float));

            shaders->setLightProbeTexture(nullptr);
        }

        shaders->setUniform(QByteArrayLiteral("material_diffuse"), &theMaterial.emissiveColor, 3 * sizeof(float));

        const auto qMix = [](float x, float y, float a) {
            return (x * (1.0f - a) + (y * a));
        };

        const auto qMix3 = [&qMix](const QVector3D &x, const QVector3D &y, float a) {
            return QVector3D{qMix(x.x(), y.x(), a), qMix(x.y(), y.y(), a), qMix(x.z(), y.z(), a)};
        };

        const QVector4D &color = theMaterial.color;
        const auto &specularTint = (theMaterial.type == QSSGRenderGraphObject::Type::PrincipledMaterial) ? qMix3(QVector3D(1.0f, 1.0f, 1.0f), color.toVector3D(), theMaterial.specularTint.x())
                                                                                                         : theMaterial.specularTint;

        shaders->setUniform(QByteArrayLiteral("base_color"), &color, 4 * sizeof(float));

        QVector4D specularColor(specularTint, theMaterial.ior);
        shaders->setUniform(QByteArrayLiteral("material_specular"), &specularColor, 4 * sizeof(float));

        shaders->setUniform(QByteArrayLiteral("cameraProperties"), &inCameraVec, 2 * sizeof(float));

        shaders->setUniform(QByteArrayLiteral("fresnelPower"), &theMaterial.fresnelPower, sizeof(float));

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
        shaders->setUniform(QByteArrayLiteral("light_ambient_total"), &diffuseLightAmbientTotal, 3 * sizeof(float));

        const QVector4D materialProperties(theMaterial.specularAmount, theMaterial.specularRoughness, theMaterial.metalnessAmount, inOpacity);
        shaders->setUniform(QByteArrayLiteral("material_properties"), &materialProperties, 4 * sizeof(float));

        shaders->setUniform(QByteArrayLiteral("bumpAmount"), &theMaterial.bumpAmount, sizeof(float));
        shaders->setUniform(QByteArrayLiteral("translucentFalloff"), &theMaterial.translucentFalloff, sizeof(float));
        shaders->setUniform(QByteArrayLiteral("diffuseLightWrap"), &theMaterial.diffuseLightWrap, sizeof(float));
        shaders->setUniform(QByteArrayLiteral("occlusionAmount"), &theMaterial.occlusionAmount, sizeof(float));
        shaders->setUniform(QByteArrayLiteral("alphaCutoff"), &theMaterial.alphaCutoff, sizeof(float));

        quint32 imageIdx = 0;
        for (QSSGRenderableImage *theImage = inFirstImage; theImage; theImage = theImage->m_nextImage, ++imageIdx)
            setRhiImageShaderVariables(shaders, *theImage, imageIdx);
    }
};
}

QSSGRef<QSSGDefaultMaterialShaderGeneratorInterface> QSSGDefaultMaterialShaderGeneratorInterface::createDefaultMaterialShaderGenerator(
        QSSGRenderContextInterface *inRc)
{
    return QSSGRef<QSSGDefaultMaterialShaderGeneratorInterface>(new QSSGShaderGenerator(inRc));
}

QT_END_NAMESPACE
