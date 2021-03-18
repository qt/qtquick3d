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

#include <QtQuick3DRender/private/qssgrendercontext_p.h>
#include <QtQuick3DRender/private/qssgrendershaderprogram_p.h>
#include <QtQuick3DRender/private/qssgrendershaderprogram_p.h>

#include <QtQuick3DRuntimeRender/private/qssgrenderdefaultmaterialshadergenerator_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercontextcore_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercodegeneratorv2_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderableimage_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderimage_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderlight_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercamera_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadowmap_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercustommaterial_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderdynamicobjectsystem_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderlightconstantproperties_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershaderkeys_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendererimplshaders_p.h>

#include <QtCore/QByteArray>

QT_BEGIN_NAMESPACE

namespace {
/**
 *	Cached light property lookups, used one per light so a shader generator for N
 *	lights will have an array of N of these lookup objects.
 */
struct QSSGShaderLightProperties
{
    // Color of the light
    QVector3D lightColor;
    QSSGLightSourceShader lightData;
};

struct QSSGShadowMapProperties
{
    QSSGRenderCachedShaderProperty<QSSGRenderTexture2D *> m_shadowmapTexture; ///< shadow texture
    QSSGRenderCachedShaderProperty<QSSGRenderTextureCube *> m_shadowCubeTexture; ///< shadow cubemap
    QSSGRenderCachedShaderProperty<QMatrix4x4> m_shadowmapMatrix; ///< world to ligh space transform matrix
    QSSGRenderCachedShaderProperty<QVector4D> m_shadowmapSettings; ///< shadow rendering settings

    QSSGShadowMapProperties() = default;
    QSSGShadowMapProperties(const QByteArray &shadowmapTextureName,
                              const QByteArray &shadowcubeTextureName,
                              const QByteArray &shadowmapMatrixName,
                              const QByteArray &shadowmapSettingsName,
                              const QSSGRef<QSSGRenderShaderProgram> &inShader)
        : m_shadowmapTexture(shadowmapTextureName, inShader)
        , m_shadowCubeTexture(shadowcubeTextureName, inShader)
        , m_shadowmapMatrix(shadowmapMatrixName, inShader)
        , m_shadowmapSettings(shadowmapSettingsName, inShader)
    {
    }
};

/**
 *	The results of generating a shader.  Caches all possible variable names into
 *	typesafe objects.
 */
struct QSSGShaderGeneratorGeneratedShader
{
    QAtomicInt ref;
    QSSGRef<QSSGRenderShaderProgram> m_shader;
    // Specific properties we know the shader has to have.
    QSSGRenderCachedShaderProperty<QMatrix4x4> m_mvp;
    QSSGRenderCachedShaderProperty<QMatrix3x3> m_normalMatrix;
    QSSGRenderCachedShaderProperty<QMatrix4x4> m_globalTransform;
    QSSGRenderCachedShaderProperty<QMatrix4x4> m_viewProj;
    QSSGRenderCachedShaderProperty<QMatrix4x4> m_viewMatrix;
    QSSGRenderCachedShaderProperty<QVector3D> m_materialDiffuse;
    QSSGRenderCachedShaderProperty<QVector4D> m_materialProperties;
    // tint, ior
    QSSGRenderCachedShaderProperty<QVector4D> m_materialSpecular;
    QSSGRenderCachedShaderProperty<float> m_bumpAmount;
    QSSGRenderCachedShaderProperty<float> m_displaceAmount;
    QSSGRenderCachedShaderProperty<float> m_translucentFalloff;
    QSSGRenderCachedShaderProperty<float> m_diffuseLightWrap;
    QSSGRenderCachedShaderProperty<float> m_fresnelPower;
    QSSGRenderCachedShaderProperty<float> m_occlusionAmount;
    QSSGRenderCachedShaderProperty<float> m_alphaCutoff;
    QSSGRenderCachedShaderProperty<QVector4D> m_baseColor;
    QSSGRenderCachedShaderProperty<QVector3D> m_cameraPosition;
    QSSGRenderCachedShaderProperty<QVector3D> m_cameraDirection;
    QVector3D m_lightAmbientTotal;
    QSSGRenderCachedShaderProperty<QVector3D> m_materialDiffuseLightAmbientTotal;
    QSSGRenderCachedShaderProperty<QVector2D> m_cameraProperties;

    QSSGRenderCachedShaderProperty<QSSGRenderTexture2D *> m_depthTexture;
    QSSGRenderCachedShaderProperty<QSSGRenderTexture2D *> m_aoTexture;
    QSSGRenderCachedShaderProperty<QSSGRenderTexture2D *> m_lightProbe;
    QSSGRenderCachedShaderProperty<QVector4D> m_lightProbeProps;
    QSSGRenderCachedShaderProperty<QVector4D> m_lightProbeOpts;
    QSSGRenderCachedShaderProperty<QVector4D> m_lightProbeRot;
    QSSGRenderCachedShaderProperty<QVector4D> m_lightProbeOfs;
    QSSGRenderCachedShaderProperty<QVector2D> m_lightProbeSize;
    QSSGRenderCachedShaderProperty<QSSGRenderTexture2D *> m_lightProbe2;
    QSSGRenderCachedShaderProperty<QVector4D> m_lightProbe2Props;
    QSSGRenderCachedShaderProperty<QVector2D> m_lightProbe2Size;

    QSSGRenderCachedShaderBuffer<QSSGRenderShaderConstantBuffer> m_aoShadowParams;
    QSSGRenderCachedShaderBuffer<QSSGRenderShaderConstantBuffer> m_lightsBuffer;

    QSSGLightConstantProperties<QSSGShaderGeneratorGeneratedShader> *m_lightConstantProperties = nullptr;

    // Cache the image property name lookups
    QVector<QSSGShaderTextureProperties> m_images;
    QVector<QSSGShaderLightProperties> m_lights;
    // Cache shadow map properties
    QVector<QSSGShadowMapProperties> m_shadowMaps;

    QSSGShaderGeneratorGeneratedShader(const QSSGRef<QSSGRenderShaderProgram> &inShader,
                                         const QSSGRef<QSSGRenderContext> &inContext)
        : m_shader(inShader)
        , m_mvp("modelViewProjection", inShader)
        , m_normalMatrix("normalMatrix", inShader)
        , m_globalTransform("modelMatrix", inShader)
        , m_viewProj("viewProjectionMatrix", inShader)
        , m_viewMatrix("viewMatrix", inShader)
        , m_materialDiffuse("material_diffuse", inShader)
        , m_materialProperties("material_properties", inShader)
        , m_materialSpecular("material_specular", inShader)
        , m_bumpAmount("bumpAmount", inShader)
        , m_displaceAmount("displaceAmount", inShader)
        , m_translucentFalloff("translucentFalloff", inShader)
        , m_diffuseLightWrap("diffuseLightWrap", inShader)
        , m_fresnelPower("fresnelPower", inShader)
        , m_occlusionAmount("occlusionAmount", inShader)
        , m_alphaCutoff("alphaCutoff", inShader)
        , m_baseColor("base_color", inShader)
        , m_cameraPosition("cameraPosition", inShader)
        , m_cameraDirection("cameraDirection", inShader)
        , m_materialDiffuseLightAmbientTotal("light_ambient_total", inShader)
        , m_cameraProperties("cameraProperties", inShader)
        , m_depthTexture("depthTexture", inShader)
        , m_aoTexture("aoTexture", inShader)
        , m_lightProbe("lightProbe", inShader)
        , m_lightProbeProps("lightProbeProperties", inShader)
        , m_lightProbeOpts("lightProbeOptions", inShader)
        , m_lightProbeRot("lightProbeRotation", inShader)
        , m_lightProbeOfs("lightProbeOffset", inShader)
        , m_lightProbeSize("lightProbeSize", inShader)
        , m_lightProbe2("lightProbe2", inShader)
        , m_lightProbe2Props("lightProbe2Properties", inShader)
        , m_lightProbe2Size("lightProbe2Size", inShader)
        , m_aoShadowParams("aoShadow", inShader)
        , m_lightsBuffer("lightsBuffer", inShader)
    {
        Q_UNUSED(inContext)
    }
    ~QSSGShaderGeneratorGeneratedShader() { delete m_lightConstantProperties; }
};

struct QSSGShaderGenerator : public QSSGDefaultMaterialShaderGeneratorInterface
{
    const QSSGRenderDefaultMaterial *m_currentMaterial;

    typedef QHash<QSSGRef<QSSGRenderShaderProgram>, QSSGRef<QSSGShaderGeneratorGeneratedShader>> ProgramToShaderMap;
    ProgramToShaderMap m_programToShaderMap;

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

    QSSGRef<QSSGShaderProgramGeneratorInterface> programGenerator() { return m_programGenerator; }
    QSSGDefaultMaterialVertexPipelineInterface &vertexGenerator() { return *m_currentPipeline; }
    QSSGShaderStageGeneratorInterface &fragmentGenerator()
    {
        return *m_programGenerator->getStage(QSSGShaderGeneratorStage::Fragment);
    }
    QSSGShaderDefaultMaterialKey &key() { return *m_currentKey; }
    const QSSGRenderDefaultMaterial *material() { return m_currentMaterial; }
    bool hasTransparency() { return m_hasTransparency; }

    void addFunction(QSSGShaderStageGeneratorInterface &generator, const QByteArray &functionName)
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

    void addLocalVariable(QSSGShaderStageGeneratorInterface &inGenerator, const QByteArray &inName, const QByteArray &inType)
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

    void generateImageUVCoordinates(QSSGShaderStageGeneratorInterface &inVertexPipeline, quint32 idx, quint32 uvSet, QSSGRenderableImage &image) override
    {
        if (uvCoordsGenerated[idx])
            return;
        QSSGDefaultMaterialVertexPipelineInterface &vertexShader(
                static_cast<QSSGDefaultMaterialVertexPipelineInterface &>(inVertexPipeline));
        QSSGShaderStageGeneratorInterface &fragmentShader(fragmentGenerator());
        setupImageVariableNames(idx);
        QByteArray textureCoordName = textureCoordVariableName(uvSet);
        fragmentShader.addUniform(m_imageSampler, "sampler2D");
        vertexShader.addUniform(m_imageOffsets, "vec3");
        vertexShader.addUniform(m_imageRotations, "vec4");
        QByteArray uvTrans = uvTransform();
        if (image.m_image.m_mappingMode == QSSGRenderImage::MappingModes::Normal) {
            vertexShader << uvTrans;
            vertexShader.addOutgoing(m_imageFragCoords, "vec2");
            addFunction(vertexShader, "getTransformedUVCoords");
            vertexShader.generateUVCoords(key(), uvSet);
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
        QSSGShaderStageGeneratorInterface &fragmentShader(fragmentGenerator());
        setupImageVariableNames(idx);
        fragmentShader.addUniform(m_imageSampler, "sampler2D");
        m_imageFragCoords = textureCoordVariableName(uvSet);
        vertexGenerator().generateUVCoords(key(), uvSet);
    }

    void generateImageUVCoordinates(quint32 idx, QSSGRenderableImage &image, quint32 uvSet = 0)
    {
        generateImageUVCoordinates(vertexGenerator(), idx, uvSet, image);
    }

    void outputSpecularEquation(QSSGRenderDefaultMaterial::MaterialSpecularModel inSpecularModel,
                                QSSGShaderStageGeneratorInterface &fragmentShader,
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

    void outputDiffuseAreaLighting(QSSGShaderStageGeneratorInterface &infragmentShader, const QByteArray &inPos, const QByteArray &inLightPrefix)
    {
        m_normalizedDirection = inLightPrefix + "_areaDir";
        addLocalVariable(infragmentShader, m_normalizedDirection, "vec3");
        infragmentShader << "    lightAttenuation = calculateDiffuseAreaOld(" << m_lightDirection << ".xyz, " << m_lightPos << ".xyz, " << m_lightUp << ", " << m_lightRt << ", " << inPos << ", " << m_normalizedDirection << ");\n";
    }

    void outputSpecularAreaLighting(QSSGShaderStageGeneratorInterface &infragmentShader,
                                    const QByteArray &inPos,
                                    const QByteArray &inView,
                                    const QByteArray &inLightSpecColor)
    {
        addFunction(infragmentShader, "sampleAreaGlossyDefault");
        infragmentShader.addUniform("material_specular", "vec4");
        infragmentShader << "global_specular_light.rgb += " << inLightSpecColor << ".rgb * lightAttenuation * shadowFac * material_specular.rgb * specularAmount"
                            " * sampleAreaGlossyDefault(tanFrame, " << inPos << ", " << m_normalizedDirection << ", " << m_lightPos << ".xyz, " << m_lightRt << ".w, " << m_lightUp << ".w, " << inView << ", roughnessAmount).rgb;\n";
    }

    void addTranslucencyIrradiance(QSSGShaderStageGeneratorInterface &infragmentShader,
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

    void addShadowMapContribution(QSSGShaderStageGeneratorInterface &inLightShader, quint32 lightIndex, QSSGRenderLight::Type inType)
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

        /*
        if ( inType == RenderLightTypes::Area )
        {
                inLightShader << "vec2 " << m_shadowCoordStem << ";" << "\n";
                inLightShader << "    shadow_map_occl = sampleParaboloid( " << m_shadowMapStem << ", "
        << m_shadowControlStem << ", "
                                                                                <<
        m_shadowMatrixStem << ", varWorldPos, vec2(1.0, " << m_shadowControlStem << ".z), "
                                                                                << m_shadowCoordStem
        << " );" << "\n";
        }
        else */
        if (inType != QSSGRenderLight::Type::Directional) {
            inLightShader << "    shadow_map_occl = sampleCubemap(" << m_shadowCubeStem << ", " << m_shadowControlStem << ", " << m_shadowMatrixStem << ", " << m_lightPos << ".xyz, varWorldPos, vec2(1.0, " << m_shadowControlStem << ".z));\n";
        } else {
            inLightShader << "    shadow_map_occl = sampleOrthographic(" << m_shadowMapStem << ", " << m_shadowControlStem << ", " << m_shadowMatrixStem << ", varWorldPos, vec2(1.0, " << m_shadowControlStem << ".z));\n";
        }
    }

    void addDisplacementMappingForDepthPass(QSSGShaderStageGeneratorInterface &inShader) override
    {
        inShader.addIncoming("attr_uv0", "vec2");
        inShader.addIncoming("attr_norm", "vec3");
        inShader.addUniform("displacementSampler", "sampler2D");
        inShader.addUniform("displaceAmount", "float");
        inShader.addUniform("displacementMap_rot", "vec4");
        inShader.addUniform("displacementMap_offset", "vec3");
        inShader.addInclude("defaultMaterialFileDisplacementTexture.glsllib");

        inShader << "    vec3 uTransform = vec3(displacementMap_rot.x, displacementMap_rot.y, displacementMap_offset.x);\n"
                    "    vec3 vTransform = vec3(displacementMap_rot.z, displacementMap_rot.w, displacementMap_offset.y);\n";
        addFunction(inShader, "getTransformedUVCoords");
        inShader << "    vec2 uv_coords = attr_uv0;\n"
                    "    uv_coords = getTransformedUVCoords(vec3(uv_coords, 1.0), uTransform, vTransform);\n"
                    "    vec3 displacedPos = defaultMaterialFileDisplacementTexture(displacementSampler , displaceAmount, uv_coords , attr_norm, attr_pos);\n"
                    "    gl_Position = modelViewProjection * vec4(displacedPos, 1.0);\n";
    }

    void addDisplacementImageUniforms(QSSGShaderStageGeneratorInterface &inGenerator,
                                      quint32 displacementImageIdx,
                                      QSSGRenderableImage *displacementImage) override
    {
        if (displacementImage) {
            setupImageVariableNames(displacementImageIdx);
            inGenerator.addInclude("defaultMaterialFileDisplacementTexture.glsllib");
            inGenerator.addUniform("modelMatrix", "mat4");
            inGenerator.addUniform("cameraPosition", "vec3");
            inGenerator.addUniform("displaceAmount", "float");
            inGenerator.addUniform(m_imageSampler, "sampler2D");
        }
    }

    void maybeAddMaterialFresnel(QSSGShaderStageGeneratorInterface &fragmentShader, QSSGDataView<quint32> inKey, bool &fragmentHasSpecularAmount, bool hasMetalness)
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

    void addDisplacementMapping(QSSGDefaultMaterialVertexPipelineInterface &inShader)
    {
        inShader.addIncoming("attr_uv0", "vec2");
        inShader.addIncoming("attr_norm", "vec3");
        inShader.addUniform("displacementSampler", "sampler2D");
        inShader.addUniform("displaceAmount", "float");
        inShader.addUniform("displacementMap_rot", "vec4");
        inShader.addUniform("displacementMap_offset", "vec3");
        inShader.addInclude("defaultMaterialFileDisplacementTexture.glsllib");

        inShader << "    vec3 uTransform = vec3(displacementMap_rot.x, displacementMap_rot.y, displacementMap_offset.x);\n"
                    "    vec3 vTransform = vec3(displacementMap_rot.z, displacementMap_rot.w, displacementMap_offset.y);\n";
        addFunction(inShader, "getTransformedUVCoords");
        inShader.generateUVCoords(key());
        inShader << "    varTexCoord0 = getTransformedUVCoords(vec3(varTexCoord0, 1.0), uTransform, vTransform);\n"
                    "    vec3 displacedPos = defaultMaterialFileDisplacementTexture(displacementSampler , displaceAmount, varTexCoord0 , attr_norm, attr_pos);\n"
                    "    gl_Position = modelViewProjection * vec4(displacedPos, 1.0);\n";
    }

    void generateTextureSwizzle(QSSGRenderTextureSwizzleMode swizzleMode, QByteArray &texSwizzle, QByteArray &lookupSwizzle)
    {
        QSSGRenderContextTypes deprecatedContextFlags(QSSGRenderContextType::GL2 | QSSGRenderContextType::GLES2);

        if (!(deprecatedContextFlags & m_renderContext->renderContext()->renderContextType())) {
            switch (swizzleMode) {
            case QSSGRenderTextureSwizzleMode::L8toR8:
            case QSSGRenderTextureSwizzleMode::L16toR16:
                texSwizzle.append(".rgb");
                lookupSwizzle.append(".rrr");
                break;
            case QSSGRenderTextureSwizzleMode::L8A8toRG8:
                texSwizzle.append(".rgba");
                lookupSwizzle.append(".rrrg");
                break;
            case QSSGRenderTextureSwizzleMode::A8toR8:
                texSwizzle.append(".a");
                lookupSwizzle.append(".r");
                break;
            default:
                break;
            }
        }
    }

    ///< get the light constant buffer and generate if necessary
    QSSGRef<QSSGRenderConstantBuffer> getLightConstantBuffer(qint32 inLightCount)
    {
        Q_ASSERT(inLightCount >= 0);
        const QSSGRef<QSSGRenderContext> &theContext = m_renderContext->renderContext();

        // we assume constant buffer support
        Q_ASSERT(theContext->supportsConstantBuffer());

        // we only create if if we have lights
        if (!inLightCount || !theContext->supportsConstantBuffer())
            return nullptr;

        static const QByteArray theName = QByteArrayLiteral("lightsBuffer");
        QSSGRef<QSSGRenderConstantBuffer> pCB = theContext->getConstantBuffer(theName);
        if (pCB)
            return pCB;

        // create
        const size_t size = sizeof(QSSGLightSourceShader) * QSSG_MAX_NUM_LIGHTS + (4 * sizeof(qint32));
        quint8 stackData[size];
        memset(stackData, 0, 4 * sizeof(qint32));
        // QSSGLightSourceShader *s = new (stackData + 4*sizeof(qint32)) QSSGLightSourceShader[QSSG_MAX_NUM_LIGHTS];
        QSSGByteView cBuffer(stackData, size);
        pCB = *m_constantBuffers.insert(theName, new QSSGRenderConstantBuffer(theContext, theName, QSSGRenderBufferUsageType::Static, cBuffer));
        if (Q_UNLIKELY(!pCB)) {
            Q_ASSERT(false);
            return nullptr;
        }

        return pCB;

    }

    void setImageShaderVariables(const QSSGRef<QSSGShaderGeneratorGeneratedShader> &inShader, QSSGRenderableImage &inImage, quint32 idx)
    {
        size_t numImageVariables = inShader->m_images.size();
        for (size_t namesIdx = numImageVariables; namesIdx <= idx; ++namesIdx) {
            setupImageVariableNames(idx);
            inShader->m_images.push_back(
                    QSSGShaderTextureProperties(inShader->m_shader, m_imageSampler, m_imageOffsets, m_imageRotations, m_imageSamplerSize));
        }
        QSSGShaderTextureProperties &theShaderProps = inShader->m_images[idx];
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

        // The image horizontal and vertical tiling modes need to be set here, before we set texture
        // on the shader.
        // because setting the image on the texture forces the textue to bind and immediately apply
        // any tex params.
        const QSSGRef<QSSGRenderTexture2D> &imageTexture = inImage.m_image.m_textureData.m_texture;
        imageTexture->setTextureWrapS(inImage.m_image.m_horizontalTilingMode);
        imageTexture->setTextureWrapT(inImage.m_image.m_verticalTilingMode);
        theShaderProps.sampler.set(imageTexture.data());

        // If item size has changed, force sampler parameters update.
        bool forceParamsUpdate = inImage.m_image.m_flags.testFlag(QSSGRenderImage::Flag::ItemSizeDirty);
        if (forceParamsUpdate) {
            imageTexture->setsamplerParamsDirty();
            inImage.m_image.m_flags.setFlag(QSSGRenderImage::Flag::ItemSizeDirty, false);
        }

        theShaderProps.offsets.set(offsets);
        theShaderProps.rotations.set(rotations);
        theShaderProps.size.set(QVector2D(imageTexture->textureDetails().width, imageTexture->textureDetails().height));
    }

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

    void generateVertexShader(const QSSGShaderDefaultMaterialKey &inKey)
    {
        // vertex displacement
        quint32 imageIdx = 0;
        QSSGRenderableImage *displacementImage = nullptr;
        quint32 displacementImageIdx = 0;

        for (QSSGRenderableImage *img = m_firstImage; img != nullptr; img = img->m_nextImage, ++imageIdx) {
            if (img->m_mapType == QSSGImageMapTypes::Displacement) {
                displacementImage = img;
                displacementImageIdx = imageIdx;
                break;
            }
        }

        // the pipeline opens/closes up the shaders stages
        vertexGenerator().beginVertexGeneration(inKey, displacementImageIdx, displacementImage);
    }

    void addSpecularAmount(QSSGShaderStageGeneratorInterface &fragmentShader, bool &fragmentHasSpecularAmount, bool reapply = false)
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
        const bool supportStandardDerivatives = m_renderContext->renderContext()->supportsStandardDerivatives();

        QSSGRenderableImage *baseImage = nullptr;
        quint32 baseImageIdx = 0;

        // Use shared texcoord when transforms are identity
        QVector<QSSGRenderableImage *> identityImages;

        Q_UNUSED(lightmapShadowImage)
        Q_UNUSED(lightmapShadowImageIdx)
        Q_UNUSED(supportStandardDerivatives)

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
            if (img->m_mapType == QSSGImageMapTypes::BaseColor || img->m_mapType == QSSGImageMapTypes::Diffuse) {
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
            } else if (img->m_mapType == QSSGImageMapTypes::Occlusion) {
                occlusionImage = img;
                occlusionImageIdx = imageIdx;
            } else if (img->m_mapType == QSSGImageMapTypes::Normal) {
                normalImage = img;
                normalImageIdx = imageIdx;
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
        QSSGShaderStageGeneratorInterface &fragmentShader(fragmentGenerator());
        QSSGDefaultMaterialVertexPipelineInterface &vertexShader(vertexGenerator());

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
            if (includeSSAOSSDOVars)
                fragmentShader.addInclude("SSAOCustomMaterial.glsllib");

            if (hasIblProbe)
                fragmentShader.addInclude("sampleProbe.glsllib");

            if (!m_lightsAsSeparateUniforms)
                addFunction(fragmentShader, "sampleLightVars");
            addFunction(fragmentShader, "diffuseReflectionBSDF");

            if (hasLightmaps)
                fragmentShader.addInclude("evalLightmaps.glsllib");

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

            if (isDoubleSided) {
                fragmentShader.addInclude("doubleSided.glsllib");
                fragmentShader.append("    world_normal = adjustNormalForFace(world_normal, varWorldPos);\n");
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

            if (identityImages.contains(baseImage))
                generateImageUVSampler(baseImageIdx);
            else
                generateImageUVCoordinates(baseImageIdx, *baseImage);
            generateTextureSwizzle(baseImage->m_image.m_textureData.m_texture->textureSwizzleMode(), texSwizzle, lookupSwizzle);

            fragmentShader << "    vec4 base_texture_color" << texSwizzle << " = texture2D(" << m_imageSampler << ", " << m_imageFragCoords << ")" << lookupSwizzle << ";\n";
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

            if (enableSSAO)
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
            fragmentShader << "    diffuseColor.rgb *= (1.0 - ds) * (1.0 - metalnessAmount);\n";
            if (specularLightingEnabled) {
                if (!hasBaseColorMap && material()->type == QSSGRenderGraphObject::Type::PrincipledMaterial) {
                    fragmentShader << "    float lum = dot(base_color.rgb, vec3(0.21, 0.72, 0.07));\n"
                                      "    specularBase += (lum > 0.0) ? (base_color.rgb) / lum : vec3(1.0);\n";
                }

                maybeAddMaterialFresnel(fragmentShader, inKey, fragmentHasSpecularAmount, metalnessEnabled);
            }


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

                    fragmentShader << "    global_diffuse_light.rgb += diffuseColor.rgb * shadowFac * shadow_map_occl * diffuseReflectionBSDF(world_normal, -" << m_lightDirection << ".xyz, " << m_lightColor << ".rgb).rgb;\n";

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

                    // Debug measure to make sure paraboloid sampling was projecting to the right
                    // location
                    // fragmentShader << "    global_diffuse_light.rg += " << m_ShadowCoordStem << ";"
                    // << "\n";
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

                    fragmentShader << "    global_diffuse_light.rgb += diffuseColor.rgb * lightAttenuation * diffuseReflectionBSDF(world_normal, " << m_normalizedDirection << ", " << m_lightColor << ".rgb).rgb;\n";
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
                        fragmentShader << "    global_diffuse_light.rgb += diffuseColor.rgb * spotFactor * ";
                    } else {
                        fragmentShader << "    global_diffuse_light.rgb += diffuseColor.rgb * ";
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
            fragmentShader << "    global_diffuse_light = vec4(global_diffuse_light.rgb * aoFactor, objectOpacity * diffuseColor.a);\n"
                              "    global_specular_light = vec3(global_specular_light.rgb);\n";
            if (!hasEmissiveMap)
                fragmentShader << "    global_diffuse_light.rgb += diffuseColor.rgb * material_diffuse.rgb;\n";

            // since we already modulate our material diffuse color
            // into the light color we will miss it entirely if no IBL
            // or light is used
            if (hasLightmaps && !(m_lights.size() || hasIblProbe))
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
                quint32 idx = 0;
                for (QSSGRenderableImage *image = m_firstImage; image; image = image->m_nextImage, ++idx) {
                    // Various maps are handled on a different locations
                    if (image->m_mapType == QSSGImageMapTypes::Bump || image->m_mapType == QSSGImageMapTypes::Normal
                        || image->m_mapType == QSSGImageMapTypes::Displacement || image->m_mapType == QSSGImageMapTypes::SpecularAmountMap
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
                    generateTextureSwizzle(image->m_image.m_textureData.m_texture->textureSwizzleMode(), texSwizzle, lookupSwizzle);

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
            fragmentShader.append("    fragOutput = vec4(clamp(global_diffuse_light.rgb + global_specular_light.rgb, 0.0, 1.0), global_diffuse_light.a);");
        } else {
            fragmentShader.append("    fragOutput = vec4(diffuseColor.rgb, diffuseColor.a * objectOpacity);");
        }

        if (vertexGenerator().hasActiveWireframe()) {
            fragmentShader << "    vec3 edgeDistance = varEdgeDistance * gl_FragCoord.w;\n"
                              "    float d = min(min(edgeDistance.x, edgeDistance.y), edgeDistance.z);\n"
                              "    float mixVal = smoothstep(0.0, 1.0, d);\n" // line width 1.0
                              "    fragOutput = mix(vec4(0.0, 1.0, 0.0, 1.0), fragOutput, mixVal);\n";
        }
    }

    QSSGRef<QSSGRenderShaderProgram> generateMaterialShader(const QByteArray &inShaderPrefix)
    {
        // build a string that allows us to print out the shader we are generating to the log.
        // This is time consuming but I feel like it doesn't happen all that often and is very
        // useful to users
        // looking at the log file.

        QByteArray generatedShaderString;
        generatedShaderString = inShaderPrefix;

        QSSGShaderDefaultMaterialKey theKey(key());
        theKey.toString(generatedShaderString, m_defaultMaterialShaderKeyProperties);

        m_lightsAsSeparateUniforms = !m_renderContext->renderContext()->supportsConstantBuffer();

        generateVertexShader(theKey);
        generateFragmentShader(theKey);

        vertexGenerator().endVertexGeneration(false);
        vertexGenerator().endFragmentGeneration(false);

        return programGenerator()->compileGeneratedShader(generatedShaderString, QSSGShaderCacheProgramFlags(), m_currentFeatureSet);
    }

    QSSGRef<QSSGRenderShaderProgram> generateShader(const QSSGRenderGraphObject &inMaterial,
                                                        QSSGShaderDefaultMaterialKey inShaderDescription,
                                                        QSSGShaderStageGeneratorInterface &inVertexPipeline,
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
        m_currentPipeline = static_cast<QSSGDefaultMaterialVertexPipelineInterface *>(&inVertexPipeline);
        m_currentFeatureSet = inFeatureSet;
        m_lights = inLights;
        m_firstImage = inFirstImage;
        m_hasTransparency = inHasTransparency;

        return generateMaterialShader(inVertexPipelineName);
    }

    const QSSGRef<QSSGShaderGeneratorGeneratedShader> &getShaderForProgram(const QSSGRef<QSSGRenderShaderProgram> &inProgram)
    {
        auto inserter = m_programToShaderMap.constFind(inProgram);
        if (inserter == m_programToShaderMap.constEnd())
            inserter = m_programToShaderMap.insert(inProgram,
                                                   QSSGRef<QSSGShaderGeneratorGeneratedShader>(
                                                           new QSSGShaderGeneratorGeneratedShader(inProgram,
                                                                                                    m_renderContext->renderContext())));

        return inserter.value();
    }

    void setGlobalProperties(const QSSGRef<QSSGRenderShaderProgram> &inProgram,
                             const QSSGRenderLayer & /*inLayer*/,
                             QSSGRenderCamera &inCamera,
                             const QVector3D &inCameraDirection,
                             const QVector<QSSGRenderLight *> &inLights,
                             const QVector<QVector3D> &inLightDirections,
                             const QSSGRef<QSSGRenderShadowMap> &inShadowMapManager,
                             bool receivesShadows = true)
    {
        const QSSGRef<QSSGShaderGeneratorGeneratedShader> &shader(getShaderForProgram(inProgram));
        m_renderContext->renderContext()->setActiveShader(inProgram);

        m_shadowMapManager = inShadowMapManager;

        QSSGRenderCamera &theCamera(inCamera);
        shader->m_cameraPosition.set(theCamera.getGlobalPos());
        shader->m_cameraDirection.set(inCameraDirection);

        QMatrix4x4 viewProj;
        if (shader->m_viewProj.isValid()) {
            theCamera.calculateViewProjectionMatrix(viewProj);
            shader->m_viewProj.set(viewProj);
        }

        if (shader->m_viewMatrix.isValid()) {
            viewProj = theCamera.globalTransform.inverted();
            shader->m_viewMatrix.set(viewProj);
        }

        // update the constant buffer
        shader->m_aoShadowParams.set();
        // We can't cache light properties because they can change per object.
        QVector3D theLightAmbientTotal = QVector3D(0, 0, 0);
        size_t numShaderLights = shader->m_lights.size();
        size_t numShadowLights = shader->m_shadowMaps.size();
        for (quint32 lightIdx = 0, shadowMapIdx = 0, lightEnd = inLights.size(); lightIdx < lightEnd && lightIdx < QSSG_MAX_NUM_LIGHTS;
             ++lightIdx) {
            QSSGRenderLight *theLight(inLights[lightIdx]);
            if (lightIdx >= numShaderLights) {
                shader->m_lights.push_back(QSSGShaderLightProperties());
                ++numShaderLights;
            }
            if (shadowMapIdx >= numShadowLights && numShadowLights < QSSG_MAX_NUM_SHADOWS && receivesShadows) {
                if (theLight->m_scope == nullptr && theLight->m_castShadow) {
                    // PKC TODO : Fix multiple shadow issues.
                    // Need to know when the list of lights changes order, and clear shadow maps
                    // when that happens.
                    setupShadowMapVariableNames(lightIdx);
                    shader->m_shadowMaps.push_back(
                            QSSGShadowMapProperties(m_shadowMapStem, m_shadowCubeStem, m_shadowMatrixStem, m_shadowControlStem, inProgram));
                }
            }
            Q_ASSERT(lightIdx < numShaderLights);
            QSSGShaderLightProperties &theLightProperties(shader->m_lights[lightIdx]);
            float brightness = aux::translateBrightness(theLight->m_brightness);

            // setup light data
            theLightProperties.lightColor = theLight->m_diffuseColor * brightness;
            theLightProperties.lightData.specular = QVector4D(theLight->m_specularColor * brightness, 1.0);
            theLightProperties.lightData.direction = QVector4D(inLightDirections[lightIdx], 1.0);

            // TODO : This does potentially mean that we can create more shadow map entries than
            // we can actually use at once.
            if ((theLight->m_scope == nullptr) && (theLight->m_castShadow && receivesShadows && inShadowMapManager)) {
                QSSGShadowMapProperties &theShadowMapProperties(shader->m_shadowMaps[shadowMapIdx++]);
                QSSGShadowMapEntry *pEntry = inShadowMapManager->getShadowMapEntry(lightIdx);
                if (pEntry) {
                    // add fixed scale bias matrix
                    QMatrix4x4 bias = { 0.5, 0.0, 0.0, 0.5,
                                        0.0, 0.5, 0.0, 0.5,
                                        0.0, 0.0, 0.5, 0.5,
                                        0.0, 0.0, 0.0, 1.0 };

                    if (theLight->m_lightType != QSSGRenderLight::Type::Directional) {
                        theShadowMapProperties.m_shadowCubeTexture.set(pEntry->m_depthCube.data());
                        theShadowMapProperties.m_shadowmapMatrix.set(pEntry->m_lightView);
                    } else {
                        theShadowMapProperties.m_shadowmapTexture.set(pEntry->m_depthMap.data());
                        theShadowMapProperties.m_shadowmapMatrix.set(bias * pEntry->m_lightVP);
                    }

                    theShadowMapProperties.m_shadowmapSettings.set(
                            QVector4D(theLight->m_shadowBias, theLight->m_shadowFactor, theLight->m_shadowMapFar, 0.0f));
                } else {
                    // if we have a light casting shadow we should find an entry
                    Q_ASSERT(false);
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
                    if (theLight->m_innerConeAngle > theLight->m_coneAngle)
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
        shader->m_lightAmbientTotal = theLightAmbientTotal;
    }

    // Also sets the blend function on the render context.
    void setMaterialProperties(const QSSGRef<QSSGRenderShaderProgram> &inProgram,
                               const QSSGRenderDefaultMaterial &inMaterial,
                               const QVector2D &inCameraVec,
                               const QMatrix4x4 &inModelViewProjection,
                               const QMatrix3x3 &inNormalMatrix,
                               const QMatrix4x4 &inGlobalTransform,
                               QSSGRenderableImage *inFirstImage,
                               float inOpacity,
                               const QSSGRef<QSSGRenderTexture2D> &inDepthTexture,
                               const QSSGRef<QSSGRenderTexture2D> &inSSaoTexture,
                               QSSGRenderImage *inLightProbe,
                               QSSGRenderImage *inLightProbe2,
                               float inProbeHorizon,
                               float inProbeBright,
                               float inProbe2Window,
                               float inProbe2Pos,
                               float inProbe2Fade,
                               float inProbeFOV)
    {

        const QSSGRef<QSSGRenderContext> &context = m_renderContext->renderContext();
        const QSSGRef<QSSGShaderGeneratorGeneratedShader> &shader = getShaderForProgram(inProgram);
        shader->m_mvp.set(inModelViewProjection);
        shader->m_normalMatrix.set(inNormalMatrix);
        shader->m_globalTransform.set(inGlobalTransform);
        shader->m_depthTexture.set(inDepthTexture.data());

        shader->m_aoTexture.set(inSSaoTexture.data());

        QSSGRenderImage *theLightProbe = inLightProbe;
        QSSGRenderImage *theLightProbe2 = inLightProbe2;

        // If the material has its own IBL Override, we should use that image instead.
        const bool hasIblProbe = inMaterial.iblProbe != nullptr;
        const bool useMaterialIbl = hasIblProbe ? (inMaterial.iblProbe->m_textureData.m_texture != nullptr) : false;
        if (useMaterialIbl)
            theLightProbe = inMaterial.iblProbe;

        if (theLightProbe) {
            if (theLightProbe->m_textureData.m_texture) {
                QSSGRenderTextureCoordOp theHorzLightProbeTilingMode = QSSGRenderTextureCoordOp::Repeat;
                QSSGRenderTextureCoordOp theVertLightProbeTilingMode = theLightProbe->m_verticalTilingMode;
                theLightProbe->m_textureData.m_texture->setTextureWrapS(theHorzLightProbeTilingMode);
                theLightProbe->m_textureData.m_texture->setTextureWrapT(theVertLightProbeTilingMode);
                const QMatrix4x4 &textureTransform = theLightProbe->m_textureTransform;
                // We separate rotational information from offset information so that just maybe the
                // shader
                // will attempt to push less information to the card.
                const float *dataPtr(textureTransform.constData());
                // The third member of the offsets contains a flag indicating if the texture was
                // premultiplied or not.
                // We use this to mix the texture alpha.
                QVector4D offsets(dataPtr[12],
                                  dataPtr[13],
                                  theLightProbe->m_textureData.m_textureFlags.isPreMultiplied() ? 1.0f : 0.0f,
                                  (float)theLightProbe->m_textureData.m_texture->numMipmaps());

                // Grab just the upper 2x2 rotation matrix from the larger matrix.
                QVector4D rotations(dataPtr[0], dataPtr[4], dataPtr[1], dataPtr[5]);

                shader->m_lightProbeRot.set(rotations);
                shader->m_lightProbeOfs.set(offsets);

                if ((!inMaterial.iblProbe) && (inProbeFOV < 180.f)) {
                    shader->m_lightProbeOpts.set(QVector4D(0.01745329251994329547f * inProbeFOV, 0.0f, 0.0f, 0.0f));
                }

                // Also make sure to add the secondary texture, but it should only be added if the
                // primary
                // (i.e. background) texture is also there.
                if (theLightProbe2 && theLightProbe2->m_textureData.m_texture) {
                    theLightProbe2->m_textureData.m_texture->setTextureWrapS(theHorzLightProbeTilingMode);
                    theLightProbe2->m_textureData.m_texture->setTextureWrapT(theVertLightProbeTilingMode);
                    shader->m_lightProbe2.set(theLightProbe2->m_textureData.m_texture.data());
                    shader->m_lightProbe2Props.set(QVector4D(inProbe2Window, inProbe2Pos, inProbe2Fade, 1.0f));

                    const QMatrix4x4 &xform2 = theLightProbe2->m_textureTransform;
                    const float *dataPtr(xform2.constData());
                    shader->m_lightProbeProps.set(QVector4D(dataPtr[12], dataPtr[13], inProbeHorizon, inProbeBright * 0.01f));
                } else {
                    shader->m_lightProbe2Props.set(QVector4D(0.0f, 0.0f, 0.0f, 0.0f));
                    shader->m_lightProbeProps.set(QVector4D(0.0f, 0.0f, inProbeHorizon, inProbeBright * 0.01f));
                }
                const QSSGRef<QSSGRenderTexture2D> &textureImage = theLightProbe->m_textureData.m_texture;
                shader->m_lightProbe.set(textureImage.data());
                shader->m_lightProbeSize.set(
                        QVector2D(textureImage->textureDetails().width, textureImage->textureDetails().height));
            } else {
                shader->m_lightProbeProps.set(QVector4D(0.0f, 0.0f, -1.0f, 0.0f));
                shader->m_lightProbe2Props.set(QVector4D(0.0f, 0.0f, 0.0f, 0.0f));
            }
        } else {
            shader->m_lightProbeProps.set(QVector4D(0.0f, 0.0f, -1.0f, 0.0f));
            shader->m_lightProbe2Props.set(QVector4D(0.0f, 0.0f, 0.0f, 0.0f));
        }

        const auto &color = inMaterial.color;
        shader->m_materialDiffuse.set(inMaterial.emissiveColor);

        const auto qMix = [](float x, float y, float a) {
            return (x * (1.0f - a) + (y * a));
        };

        const auto qMix3 = [&qMix](const QVector3D &x, const QVector3D &y, float a) {
            return QVector3D{qMix(x.x(), y.x(), a), qMix(x.y(), y.y(), a), qMix(x.z(), y.z(), a)};
        };

        const auto &specularTint = (inMaterial.type == QSSGRenderGraphObject::Type::PrincipledMaterial) ? qMix3(QVector3D(1.0f, 1.0f, 1.0f), color.toVector3D(), inMaterial.specularTint.x())
                                                                                                        : inMaterial.specularTint;

        shader->m_baseColor.set(color);
        shader->m_materialSpecular.set(QVector4D(specularTint, inMaterial.ior));
        shader->m_cameraProperties.set(inCameraVec);
        shader->m_fresnelPower.set(inMaterial.fresnelPower);

        const bool hasLighting = inMaterial.lighting != QSSGRenderDefaultMaterial::MaterialLighting::NoLighting;
        if (hasLighting) {
            if (context->supportsConstantBuffer()) {
                const QSSGRef<QSSGRenderConstantBuffer> &pLightCb = getLightConstantBuffer(shader->m_lights.size());
                // if we have lights we need a light buffer
                Q_ASSERT(shader->m_lights.size() == 0 || pLightCb);

                for (qint32 idx = 0, end = shader->m_lights.size(); idx < end && pLightCb; ++idx) {
                    auto &lightProp = shader->m_lights[idx];
                    lightProp.lightData.diffuse = QVector4D(lightProp.lightColor, 1.0);

                    // this is our final change update memory
                    pLightCb->updateRaw(quint32(idx) * sizeof(QSSGLightSourceShader) + (4 * sizeof(qint32)), toByteView(lightProp.lightData));
                }
                // update light buffer to hardware
                if (pLightCb) {
                    qint32 cgLights = shader->m_lights.size();
                    pLightCb->updateRaw(0, toByteView(cgLights));
                    shader->m_lightsBuffer.set();
                }
            } else {
                QSSGLightConstantProperties<QSSGShaderGeneratorGeneratedShader> *pLightConstants = getLightConstantProperties(shader);

                // if we have lights we need a light buffer
                Q_ASSERT(shader->m_lights.size() == 0 || pLightConstants);

                for (qint32 idx = 0, end = shader->m_lights.size(); idx < end && pLightConstants; ++idx) {
                    auto &lightProp = shader->m_lights[idx];
                    lightProp.lightData.diffuse = QVector4D(lightProp.lightColor, 1.0);
                }
                // update light buffer to hardware
                if (pLightConstants)
                    pLightConstants->updateLights(shader);
            }
        }

        shader->m_materialDiffuseLightAmbientTotal.set(shader->m_lightAmbientTotal);
        shader->m_materialProperties.set(QVector4D(inMaterial.specularAmount, inMaterial.specularRoughness, inMaterial.metalnessAmount, inOpacity));
        shader->m_bumpAmount.set(inMaterial.bumpAmount);
        shader->m_displaceAmount.set(inMaterial.displaceAmount);
        shader->m_translucentFalloff.set(inMaterial.translucentFalloff);
        shader->m_diffuseLightWrap.set(inMaterial.diffuseLightWrap);
        shader->m_occlusionAmount.set(inMaterial.occlusionAmount);
        shader->m_alphaCutoff.set(inMaterial.alphaCutoff);

        quint32 imageIdx = 0;
        for (QSSGRenderableImage *theImage = inFirstImage; theImage; theImage = theImage->m_nextImage, ++imageIdx)
            setImageShaderVariables(shader, *theImage, imageIdx);

        QSSGRenderBlendFunctionArgument blendFunc;
        QSSGRenderBlendEquationArgument blendEqua(QSSGRenderBlendEquation::Add, QSSGRenderBlendEquation::Add);
        // The blend function goes:
        // src op
        // dst op
        // src alpha op
        // dst alpha op
        // All of our shaders produce non-premultiplied values.
        switch (inMaterial.blendMode) {
        case QSSGRenderDefaultMaterial::MaterialBlendMode::Screen:
            blendFunc = QSSGRenderBlendFunctionArgument(QSSGRenderSrcBlendFunc::One,
                                                          QSSGRenderDstBlendFunc::OneMinusSrcColor,
                                                          QSSGRenderSrcBlendFunc::One,
                                                          QSSGRenderDstBlendFunc::OneMinusSrcColor);
            break;
        case QSSGRenderDefaultMaterial::MaterialBlendMode::Multiply:
            blendFunc = QSSGRenderBlendFunctionArgument(QSSGRenderSrcBlendFunc::DstColor,
                                                          QSSGRenderDstBlendFunc::Zero,
                                                          QSSGRenderSrcBlendFunc::One,
                                                          QSSGRenderDstBlendFunc::One);
            break;
        case QSSGRenderDefaultMaterial::MaterialBlendMode::Overlay:
            // SW fallback is not using blend equation
            // note blend func is not used here anymore
            if (context->supportsAdvancedBlendHW() || context->supportsAdvancedBlendHwKHR())
                blendEqua = QSSGRenderBlendEquationArgument(QSSGRenderBlendEquation::Overlay, QSSGRenderBlendEquation::Overlay);
            break;
        case QSSGRenderDefaultMaterial::MaterialBlendMode::ColorBurn:
            // SW fallback is not using blend equation
            // note blend func is not used here anymore
            if (context->supportsAdvancedBlendHW() || context->supportsAdvancedBlendHwKHR())
                blendEqua = QSSGRenderBlendEquationArgument(QSSGRenderBlendEquation::ColorBurn,
                                                              QSSGRenderBlendEquation::ColorBurn);
            break;
        case QSSGRenderDefaultMaterial::MaterialBlendMode::ColorDodge:
            // SW fallback is not using blend equation
            // note blend func is not used here anymore
            if (context->supportsAdvancedBlendHW() || context->supportsAdvancedBlendHwKHR())
                blendEqua = QSSGRenderBlendEquationArgument(QSSGRenderBlendEquation::ColorDodge,
                                                              QSSGRenderBlendEquation::ColorDodge);
            break;
        default:
            blendFunc = QSSGRenderBlendFunctionArgument(QSSGRenderSrcBlendFunc::SrcAlpha,
                                                          QSSGRenderDstBlendFunc::OneMinusSrcAlpha,
                                                          QSSGRenderSrcBlendFunc::One,
                                                          QSSGRenderDstBlendFunc::OneMinusSrcAlpha);
            break;
        }
        context->setBlendFunction(blendFunc);
        context->setBlendEquation(blendEqua);
    }
    void setMaterialProperties(const QSSGRef<QSSGRenderShaderProgram> &inProgram,
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
        const QSSGRenderDefaultMaterial &theMaterial(static_cast<const QSSGRenderDefaultMaterial &>(inMaterial));
        Q_ASSERT(inMaterial.type == QSSGRenderGraphObject::Type::DefaultMaterial || inMaterial.type == QSSGRenderGraphObject::Type::PrincipledMaterial);


        setGlobalProperties(inProgram,
                            inRenderProperties.layer,
                            inRenderProperties.camera,
                            inRenderProperties.cameraDirection,
                            inRenderProperties.lights,
                            inRenderProperties.lightDirections,
                            inRenderProperties.shadowMapManager,
                            receivesShadows);
        setMaterialProperties(inProgram,
                              theMaterial,
                              inCameraVec,
                              inModelViewProjection,
                              inNormalMatrix,
                              inGlobalTransform,
                              inFirstImage,
                              inOpacity,
                              inRenderProperties.depthTexture,
                              inRenderProperties.ssaoTexture,
                              inRenderProperties.lightProbe,
                              inRenderProperties.lightProbe2,
                              inRenderProperties.probeHorizon,
                              inRenderProperties.probeBright,
                              inRenderProperties.probe2Window,
                              inRenderProperties.probe2Pos,
                              inRenderProperties.probe2Fade,
                              inRenderProperties.probeFOV);
    }

    QSSGLightConstantProperties<QSSGShaderGeneratorGeneratedShader> *getLightConstantProperties(
            const QSSGRef<QSSGShaderGeneratorGeneratedShader> &shader)
    {
        if (!shader->m_lightConstantProperties
            || int(shader->m_lights.size()) > shader->m_lightConstantProperties->m_constants.size()) {
            if (shader->m_lightConstantProperties)
                delete shader->m_lightConstantProperties;
            shader->m_lightConstantProperties = new QSSGLightConstantProperties<QSSGShaderGeneratorGeneratedShader>(shader.data(), m_lightsAsSeparateUniforms);
        }
        return shader->m_lightConstantProperties;
    }
};
}

QSSGRef<QSSGDefaultMaterialShaderGeneratorInterface> QSSGDefaultMaterialShaderGeneratorInterface::createDefaultMaterialShaderGenerator(
        QSSGRenderContextInterface *inRc)
{
    return QSSGRef<QSSGDefaultMaterialShaderGeneratorInterface>(new QSSGShaderGenerator(inRc));
}

QSSGDefaultMaterialVertexPipelineInterface::~QSSGDefaultMaterialVertexPipelineInterface() = default;

QT_END_NAMESPACE
