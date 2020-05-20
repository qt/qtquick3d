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

#include "qssgrendermaterialshadergenerator_p.h"
#include <QtQuick3DRuntimeRender/private/qssgrenderdefaultmaterialshadergenerator_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercontextcore_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercodegenerator_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderableimage_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderimage_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderlight_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercamera_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadowmap_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercustommaterial_p.h>
#include "qssgrendercustommaterialsystem_p.h"
#include <QtQuick3DRuntimeRender/private/qssgrendershaderkeys_p.h>
#include <QtQuick3DUtils/private/qssgutils_p.h>

QT_BEGIN_NAMESPACE

namespace {
struct QSSGShaderGenerator : public QSSGMaterialShaderGeneratorInterface
{
    const QSSGRenderCustomMaterial *m_currentMaterial;

    QByteArray m_imageSampler;
    QByteArray m_imageFragCoords;
    QByteArray m_imageRotScale;
    QByteArray m_imageOffset;

    explicit QSSGShaderGenerator(QSSGRenderContextInterface *inRc)
        : QSSGMaterialShaderGeneratorInterface (inRc)
        , m_currentMaterial(nullptr)
    {
    }

    const QSSGRef<QSSGShaderProgramGeneratorInterface> &programGenerator() { return m_programGenerator; }
    QSSGVertexPipelineBase &vertexGenerator() { return *m_currentPipeline; }
    QSSGStageGeneratorBase &fragmentGenerator()
    {
        return *m_programGenerator->getStage(QSSGShaderGeneratorStage::Fragment);
    }
    const QSSGRenderCustomMaterial &material() { return *m_currentMaterial; }
    bool hasTransparency() const { return m_hasTransparency; }

    quint32 convertTextureTypeValue(QSSGImageMapTypes inType)
    {
        QSSGRenderTextureTypeValue retVal = QSSGRenderTextureTypeValue::Unknown;

        switch (inType) {
        case QSSGImageMapTypes::LightmapIndirect:
            retVal = QSSGRenderTextureTypeValue::LightmapIndirect;
            break;
        case QSSGImageMapTypes::LightmapRadiosity:
            retVal = QSSGRenderTextureTypeValue::LightmapRadiosity;
            break;
        case QSSGImageMapTypes::LightmapShadow:
            retVal = QSSGRenderTextureTypeValue::LightmapShadow;
            break;
        case QSSGImageMapTypes::Bump:
            retVal = QSSGRenderTextureTypeValue::Bump;
            break;
        case QSSGImageMapTypes::Diffuse:
            retVal = QSSGRenderTextureTypeValue::Diffuse;
            break;
        default:
            retVal = QSSGRenderTextureTypeValue::Unknown;
            break;
        }

        Q_ASSERT(retVal != QSSGRenderTextureTypeValue::Unknown);

        return static_cast<quint32>(retVal);
    }

    ImageVariableNames getImageVariableNames(uint imageIdx) override
    {
        // convert to QSSGRenderTextureTypeValue
        QSSGRenderTextureTypeValue texType = QSSGRenderTextureTypeValue(imageIdx);
        QByteArray imageStem = toString(texType);
        imageStem.append("_");
        m_imageSampler = imageStem;
        m_imageSampler.append("sampler");
        m_imageFragCoords = imageStem;
        m_imageFragCoords.append("uv_coords");
        m_imageRotScale = imageStem;
        m_imageRotScale.append("rot_scale");
        m_imageOffset = imageStem;
        m_imageOffset.append("offset");

        ImageVariableNames retVal;
        retVal.m_imageSampler = m_imageSampler;
        retVal.m_imageFragCoords = m_imageFragCoords;
        return retVal;
    }

    void generateImageUVCoordinates(QSSGStageGeneratorBase &, quint32, quint32, QSSGRenderableImage &) override
    {
    }

    bool generateVertexShader(QSSGShaderDefaultMaterialKey &, const QByteArray &inShaderPathName)
    {
        const QSSGRef<QSSGShaderLibraryManger> &shaderLibraryManager(m_renderContext->shaderLibraryManger());
        QByteArray vertSource = shaderLibraryManager->getShaderSource(inShaderPathName);

        Q_ASSERT(!vertSource.isEmpty());

        // Check if the vertex shader portion already contains a main function
        // The same string contains both the vertex and the fragment shader
        // The last "#ifdef FRAGMENT_SHADER" should mark the start of the fragment shader
        int fragmentDefStart = vertSource.indexOf("#ifdef FRAGMENT_SHADER");
        int nextIndex = fragmentDefStart;
        while (nextIndex != -1) {
            nextIndex = vertSource.indexOf("#ifdef FRAGMENT_SHADER", nextIndex + 1);
            if (nextIndex != -1)
                fragmentDefStart = nextIndex;
        }
        const int mainStart = vertSource.indexOf("void main()");

        auto &vertGenerator = vertexGenerator();

        if (mainStart != -1 && (fragmentDefStart == -1 || mainStart < fragmentDefStart)) {
            programGenerator()->beginProgram();
            vertGenerator << "#define VERTEX_SHADER\n\n";
            vertGenerator << vertSource;
            return true;
        }

        // the pipeline opens/closes up the shaders stages
        vertexGenerator().beginVertexGeneration();
        return false;
    }

    void setRhiLightBufferData(QSSGLightSourceShader *lightData, QSSGRenderLight *light, float clipFar, int shadowIdx)
    {
        QVector3D dir(0, 0, 1);
        if (light->m_lightType == QSSGRenderLight::Type::Directional) {
            dir = light->getScalingCorrectDirection();
            // we lit in world sapce
            dir *= -1;
            lightData->position = QVector4D(dir, 0.0);
        } else if (light->m_lightType == QSSGRenderLight::Type::Area
                   || light->m_lightType == QSSGRenderLight::Type::Spot) {
            dir = light->getScalingCorrectDirection();
            lightData->position = QVector4D(light->getGlobalPos(), 1.0);
        } else {
            dir = light->getGlobalPos();
            lightData->position = QVector4D(dir, 1.0);
        }

        lightData->direction = QVector4D(dir, 0.0);

        float normalizedBrightness = aux::translateBrightness(light->m_brightness);
        lightData->diffuse = QVector4D(light->m_diffuseColor * normalizedBrightness, 1.0);
        lightData->specular = QVector4D(light->m_specularColor * normalizedBrightness, 1.0);

        if (light->m_lightType == QSSGRenderLight::Type::Area) {
            lightData->width = light->m_areaWidth;
            lightData->height = light->m_areaWidth;

            QMatrix3x3 theDirMatrix(mat44::getUpper3x3(light->globalTransform));
            lightData->right = QVector4D(mat33::transform(theDirMatrix, QVector3D(1, 0, 0)), light->m_areaWidth);
            lightData->up = QVector4D(mat33::transform(theDirMatrix, QVector3D(0, 1, 0)), light->m_areaHeight);
        } else {
            lightData->width = 0.0;
            lightData->height = 0.0;
            lightData->right = QVector4D();
            lightData->up = QVector4D();

            // These components only apply to CG lights
            lightData->ambient = QVector4D(light->m_ambientColor, 1.0);

            lightData->constantAttenuation
                    = aux::translateConstantAttenuation(light->m_constantFade);
            lightData->linearAttenuation = aux::translateLinearAttenuation(light->m_linearFade);
            lightData->quadraticAttenuation
                    = aux::translateQuadraticAttenuation(light->m_quadraticFade);
            lightData->coneAngle = 180.0f;
            if (light->m_lightType == QSSGRenderLight::Type::Spot) {
                lightData->coneAngle = qCos(qDegreesToRadians(light->m_coneAngle));
                float innerConeAngle = light->m_innerConeAngle;
                if (light->m_innerConeAngle < 0)
                    innerConeAngle = light->m_coneAngle * 0.7f;
                else if (light->m_innerConeAngle > light->m_coneAngle)
                    innerConeAngle = light->m_coneAngle;
                lightData->innerConeAngle = qCos(qDegreesToRadians(innerConeAngle));
            }
        }

        if (light->m_lightType == QSSGRenderLight::Type::Point) {
            QMatrix4x4 ident;
            memcpy(lightData->shadowView, ident.constData(), 16 * sizeof(float));
        } else {
            memcpy(lightData->shadowView, light->globalTransform.constData(), 16 * sizeof(float));
        }

        lightData->shadowControls = QVector4D(light->m_shadowBias, light->m_shadowFactor, clipFar, 0.0);
        lightData->shadowIdx = shadowIdx;
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
        Q_UNUSED(inCameraVec);
        Q_UNUSED(inFirstImage);
        Q_UNUSED(inOpacity);
        Q_UNUSED(inRenderProperties);
        Q_UNUSED(receivesShadows);

        const QSSGRenderCustomMaterial &material(static_cast<const QSSGRenderCustomMaterial &>(inMaterial));
        QSSGMaterialSystem *materialSystem = m_renderContext->customMaterialSystem().data();

        materialSystem->applyRhiShaderPropertyValues(material, shaders);

        QSSGRenderCamera &theCamera(inRenderProperties.camera);

        const QVector3D camGlobalPos = theCamera.getGlobalPos();
        shaders->setUniform(QByteArrayLiteral("cameraPosition"), &camGlobalPos, 3 * sizeof(float));
        shaders->setUniform(QByteArrayLiteral("cameraDirection"), &inRenderProperties.cameraDirection, 3 * sizeof(float));
        QVector2D camProps(theCamera.clipNear, theCamera.clipFar);
        shaders->setUniform(QByteArrayLiteral("cameraProperties"), &camProps, 2 * sizeof(float));

        const QMatrix4x4 clipSpaceCorrMatrix = m_renderContext->rhiContext()->rhi()->clipSpaceCorrMatrix();
        QMatrix4x4 viewProj;
        theCamera.calculateViewProjectionMatrix(viewProj);
        viewProj = clipSpaceCorrMatrix * viewProj;
        shaders->setUniform(QByteArrayLiteral("viewProjectionMatrix"), viewProj.constData(), 16 * sizeof(float));

        const QMatrix4x4 viewMatrix = theCamera.globalTransform.inverted();
        shaders->setUniform(QByteArrayLiteral("viewMatrix"), viewMatrix.constData(), 16 * sizeof(float));

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

        shaders->setUniform(QByteArrayLiteral("objectOpacity"), &inOpacity, sizeof(float));

        shaders->setDepthTexture(inRenderProperties.rhiDepthTexture);
        shaders->setSsaoTexture(inRenderProperties.rhiSsaoTexture);

        const QSSGRhiShaderStagesWithResources::LightBufferSlot nonAreaLightBuf = QSSGRhiShaderStagesWithResources::LightBuffer0;
        const QSSGRhiShaderStagesWithResources::LightBufferSlot areaLightBuf = QSSGRhiShaderStagesWithResources::LightBuffer1;
        shaders->resetLights(nonAreaLightBuf);
        shaders->resetLights(areaLightBuf);

        qint32 nonAreaLightCount = 0;
        qint32 areaLightCount = 0;

        qint32 shadowMapCount = 0;
        qint32 shadowCubeCount = 0;

        QVarLengthArray<QRhiTexture *, QSSG_MAX_NUM_SHADOWS_PER_TYPE> shadowMapTextures;
        QVarLengthArray<QRhiTexture *, QSSG_MAX_NUM_SHADOWS_PER_TYPE> shadowCubeTextures;

        for (quint32 lightIdx = 0, lightEnd = inRenderProperties.lights.size();
             lightIdx < lightEnd && lightIdx < QSSG_MAX_NUM_LIGHTS; ++lightIdx)
        {
            QSSGRenderLight *light(inRenderProperties.lights[lightIdx]);
            int shadowIdx = -1;
            if (receivesShadows) {
                QSSGShadowMapEntry *shadowMapEntry = nullptr;
                if (inRenderProperties.shadowMapManager && light->m_castShadow)
                    shadowMapEntry = inRenderProperties.shadowMapManager->getShadowMapEntry(lightIdx);
                const bool isDirectional = light->m_lightType == QSSGRenderLight::Type::Directional;
                if (shadowMapEntry) {
                    if (!isDirectional && shadowMapEntry->m_rhiDepthCube && shadowCubeCount < QSSG_MAX_NUM_SHADOWS_PER_TYPE) {
                        shadowCubeTextures.append(shadowMapEntry->m_rhiDepthCube);
                        shadowIdx = shadowCubeCount++;
                    } else if (isDirectional && shadowMapEntry->m_rhiDepthMap && shadowMapCount < QSSG_MAX_NUM_SHADOWS_PER_TYPE) {
                        shadowMapTextures.append(shadowMapEntry->m_rhiDepthMap);
                        shadowIdx = shadowMapCount++;
                    }
                }
            }
            if (light->m_lightType == QSSGRenderLight::Type::Area) {
                ::QSSGShaderLightProperties &lightProperties(shaders->addLight(areaLightBuf));
                setRhiLightBufferData(&lightProperties.lightData, light, theCamera.clipFar, shadowIdx);
                ++areaLightCount;
            } else {
                ::QSSGShaderLightProperties &lightProperties(shaders->addLight(nonAreaLightBuf));
                setRhiLightBufferData(&lightProperties.lightData, light, theCamera.clipFar, shadowIdx);
                ++nonAreaLightCount;
            }
        }

        shaders->setUniform(QByteArrayLiteral("lightCount"), &nonAreaLightCount, sizeof(qint32));
        shaders->setUniform(QByteArrayLiteral("areaLightCount"), &areaLightCount, sizeof(qint32));

        shaders->setUniform(QByteArrayLiteral("shadowMapCount"), &shadowMapCount, sizeof(qint32));
        shaders->setUniform(QByteArrayLiteral("shadowCubeCount"), &shadowCubeCount, sizeof(qint32));

        shaders->resetShadowMapArrays();
        if (!shadowMapTextures.isEmpty()) {
            QSSGRhiShadowMapArrayProperties &p(shaders->addShadowMapArray());
            p.shadowMapArrayUniformName = QByteArrayLiteral("shadowMaps");
            p.isCubemap = false;
            for (QRhiTexture *texture : shadowMapTextures)
                p.shadowMapTextures.append(texture);
        }
        if (!shadowCubeTextures.isEmpty()) {
            QSSGRhiShadowMapArrayProperties &p(shaders->addShadowMapArray());
            p.shadowMapArrayUniformName = QByteArrayLiteral("shadowCubes");
            p.isCubemap = true;
            for (QRhiTexture *texture : shadowCubeTextures)
                p.shadowMapTextures.append(texture);
        }

        QSSGRenderImage *theLightProbe = inRenderProperties.lightProbe;
        //QSSGRenderImage *theLightProbe2 = inRenderProperties.lightProbe2;

        if (material.m_iblProbe && material.m_iblProbe->m_textureData.m_rhiTexture)
            theLightProbe = material.m_iblProbe;

        if (theLightProbe && theLightProbe->m_textureData.m_rhiTexture) {
            QSSGRenderTextureCoordOp theHorzLightProbeTilingMode = theLightProbe->m_horizontalTilingMode;
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

            if (!material.m_iblProbe && inRenderProperties.probeFOV < 180.f) {
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
    }

    void generateLightmapIndirectFunc(QSSGStageGeneratorBase &inFragmentShader, QSSGRenderImage *pEmissiveLightmap)
    {
        inFragmentShader << "\n"
                            "vec3 computeMaterialLightmapIndirect()\n{\n"
                            "  vec4 indirect = vec4( 0.0, 0.0, 0.0, 0.0 );\n";
        if (pEmissiveLightmap) {
            ImageVariableNames names = getImageVariableNames(convertTextureTypeValue(QSSGImageMapTypes::LightmapIndirect));
            inFragmentShader.addUniform(names.m_imageSampler, "sampler2D");
            inFragmentShader.addUniform(m_imageOffset, "vec3");
            inFragmentShader.addUniform(m_imageRotScale, "vec4");

            inFragmentShader << "\n  indirect = evalIndirectLightmap( " << m_imageSampler << ", varTexCoord1, "
                             << m_imageRotScale << ", "
                             << m_imageOffset << " );\n\n";
        }

        inFragmentShader << "  return indirect.rgb;\n"
                            "}\n\n";
    }

    void generateLightmapRadiosityFunc(QSSGStageGeneratorBase &inFragmentShader, QSSGRenderImage *pRadiosityLightmap)
    {
        inFragmentShader << "\n"
                            "vec3 computeMaterialLightmapRadiosity()\n{\n"
                            "  vec4 radiosity = vec4( 1.0, 1.0, 1.0, 1.0 );\n";
        if (pRadiosityLightmap) {
            ImageVariableNames names = getImageVariableNames(convertTextureTypeValue(QSSGImageMapTypes::LightmapRadiosity));
            inFragmentShader.addUniform(names.m_imageSampler, "sampler2D");
            inFragmentShader.addUniform(m_imageOffset, "vec3");
            inFragmentShader.addUniform(m_imageRotScale, "vec4");

            inFragmentShader << "\n  radiosity = evalRadiosityLightmap( " << m_imageSampler << ", varTexCoord1, "
                             << m_imageRotScale << ", "
                             << m_imageOffset << " );\n\n";
        }

        inFragmentShader << "  return radiosity.rgb;\n"
                            "}\n\n";
    }

    void generateLightmapShadowFunc(QSSGStageGeneratorBase &inFragmentShader, QSSGRenderImage *pBakedShadowMap)
    {
        inFragmentShader << "\n"
                            "vec4 computeMaterialLightmapShadow()\n{\n"
                            "  vec4 shadowMask = vec4( 1.0, 1.0, 1.0, 1.0 );\n";
        if (pBakedShadowMap) {
            ImageVariableNames names = getImageVariableNames(static_cast<quint32>(QSSGRenderTextureTypeValue::LightmapShadow));
            // Add uniforms
            inFragmentShader.addUniform(names.m_imageSampler, "sampler2D");
            inFragmentShader.addUniform(m_imageOffset, "vec3");
            inFragmentShader.addUniform(m_imageRotScale, "vec4");

            inFragmentShader << "\n  shadowMask = evalShadowLightmap( " << m_imageSampler << ", texCoord0, "
                             << m_imageRotScale << ", "
                             << m_imageOffset << " );\n\n";
        }

        inFragmentShader << "  return shadowMask;\n"
                            "}\n\n";
    }

    void generateLightmapIndirectSetupCode(QSSGStageGeneratorBase &inFragmentShader,
                                           QSSGRenderableImage *pIndirectLightmap,
                                           QSSGRenderableImage *pRadiosityLightmap)
    {
        if (!pIndirectLightmap && !pRadiosityLightmap)
            return;

        QByteArray finalValue;

        inFragmentShader << "\n"
                            "void initializeLayerVariablesWithLightmap(void)\n{\n";
        if (pIndirectLightmap) {
            inFragmentShader << "  vec3 lightmapIndirectValue = computeMaterialLightmapIndirect( );\n";
            finalValue.append("vec4(lightmapIndirectValue, 1.0)");
        }
        if (pRadiosityLightmap) {
            inFragmentShader << "  vec3 lightmapRadisoityValue = computeMaterialLightmapRadiosity( );\n";
            if (finalValue.isEmpty())
                finalValue.append("vec4(lightmapRadisoityValue, 1.0)");
            else
                finalValue.append(" + vec4(lightmapRadisoityValue, 1.0)");
        }

        finalValue.append(";\n");

        inFragmentShader << "  layer.base += " << finalValue;
        inFragmentShader << "  layer.layer += " << finalValue;

        inFragmentShader << "}\n\n";
    }

    void generateLightmapShadowCode(QSSGStageGeneratorBase &inFragmentShader, QSSGRenderableImage *pBakedShadowMap)
    {
        if (pBakedShadowMap) {
            inFragmentShader << " tmpShadowTerm *= computeMaterialLightmapShadow( );\n\n";
        }
    }

    void applyEmissiveMask(QSSGStageGeneratorBase &inFragmentShader, QSSGRenderImage *pEmissiveMaskMap)
    {
        inFragmentShader << "\n"
                            "vec3 computeMaterialEmissiveMask()\n{\n"
                            "  vec3 emissiveMask = vec3( 1.0, 1.0, 1.0 );\n";
        if (pEmissiveMaskMap) {
            inFragmentShader << "  texture_coordinate_info tci;\n"
                                "  texture_coordinate_info transformed_tci;\n"
                                "  tci = textureCoordinateInfo( texCoord0, tangent, binormal );\n"
                                "  transformed_tci = transformCoordinate( "
                                "rotationTranslationScale( vec3( 0.000000, 0.000000, 0.000000 ), "
                                "vec3( 0.000000, 0.000000, 0.000000 ), vec3( 1.000000, 1.000000, "
                                "1.000000 ) ), tci );\n"
                                "  emissiveMask = fileTexture( " << pEmissiveMaskMap->m_imageShaderName.toUtf8()
                             << ", vec3( 0, 0, 0 ), vec3( 1, 1, 1 ), mono_alpha, transformed_tci, "
                             << "vec2( 0.000000, 1.000000 ), vec2( 0.000000, 1.000000 ), "
                                "wrap_repeat, wrap_repeat, gamma_default ).tint;\n";
        }

        inFragmentShader << "  return emissiveMask;\n"
                            "}\n\n";
    }

    void registerNonSnippetUnconditionalUniforms(QSSGStageGeneratorBase &fs)
    {
        fs.addUniform("modelMatrix", "mat4");
        fs.addUniform("modelViewProjection", "mat4");
        fs.addUniform("viewMatrix", "mat4");
        fs.addUniform("normalMatrix", "mat3");
        fs.addUniform("cameraPosition", "vec3");
        fs.addUniform("viewProjectionMatrix", "mat4");
        fs.addUniform("viewportMatrix", "mat4");
        fs.addUniform("cameraProperties", "vec2");
        fs.addUniform("lightCount", "int");
        fs.addUniform("areaLightCount", "int");
        fs.addUniform("objectOpacity", "float");
    }

    bool generateFragmentShader(QSSGShaderDefaultMaterialKey &inKey,
                                const QByteArray &inShaderPathName,
                                bool hasCustomVertShader)
    {
        const QSSGRef<QSSGShaderLibraryManger> &shaderLibraryManager(m_renderContext->shaderLibraryManger());
        const QByteArray fragSource = shaderLibraryManager->getShaderSource(inShaderPathName);

        Q_ASSERT(!fragSource.isEmpty());

        // light maps
        bool hasLightmaps = false;
        QSSGRenderableImage *lightmapShadowImage = nullptr;
        QSSGRenderableImage *lightmapIndirectImage = nullptr;
        QSSGRenderableImage *lightmapRadisoityImage = nullptr;

        for (QSSGRenderableImage *img = m_firstImage; img != nullptr; img = img->m_nextImage) {
            if (img->m_mapType == QSSGImageMapTypes::LightmapIndirect) {
                lightmapIndirectImage = img;
                hasLightmaps = true;
            } else if (img->m_mapType == QSSGImageMapTypes::LightmapRadiosity) {
                lightmapRadisoityImage = img;
                hasLightmaps = true;
            } else if (img->m_mapType == QSSGImageMapTypes::LightmapShadow) {
                lightmapShadowImage = img;
            }
        }

        if (!hasCustomVertShader) {
            vertexGenerator().generateUVCoords(0, key());
            // for lightmaps we expect a second set of uv coordinates
            if (hasLightmaps)
                vertexGenerator().generateUVCoords(1, key());
        }

        QSSGVertexPipelineBase &vertexShader(vertexGenerator());
        QSSGStageGeneratorBase &fragmentShader(fragmentGenerator());

        fragmentShader << "#define FRAGMENT_SHADER\n\n";

        const bool hasCustomFragShader = fragSource.contains("void main()");

        if (!hasCustomFragShader)
            fragmentShader.addInclude("evalLightmaps.glsllib");

        // check dielectric materials
        if (!material().isDielectric())
            fragmentShader << "#define MATERIAL_IS_NON_DIELECTRIC 1\n\n";
        else
            fragmentShader << "#define MATERIAL_IS_NON_DIELECTRIC 0\n\n";

        fragmentShader << "#define QSSG_ENABLE_RNM 0\n\n";

        fragmentShader << fragSource << "\n";

        // If a "main()" is already
        // written, we'll assume that the
        // shader
        // pass is already written out and we don't need to add anything.
        // Nothing beyond the basics, anyway
        if (hasCustomFragShader) {
            fragmentShader << "#define FRAGMENT_SHADER\n\n";
            if (!hasCustomVertShader) {
                vertexShader.generateWorldNormal(inKey);
                vertexShader.generateVarTangentAndBinormal(inKey);
                vertexShader.generateWorldPosition();

                vertexShader.generateViewVector();
            }
            return true;
        }

        if (material().hasLighting() && lightmapIndirectImage) {
            generateLightmapIndirectFunc(fragmentShader, &lightmapIndirectImage->m_image);
        }
        if (material().hasLighting() && lightmapRadisoityImage) {
            generateLightmapRadiosityFunc(fragmentShader, &lightmapRadisoityImage->m_image);
        }
        if (material().hasLighting() && lightmapShadowImage) {
            generateLightmapShadowFunc(fragmentShader, &lightmapShadowImage->m_image);
        }

        if (material().hasLighting() && (lightmapIndirectImage || lightmapRadisoityImage))
            generateLightmapIndirectSetupCode(fragmentShader, lightmapIndirectImage, lightmapRadisoityImage);

        if (material().hasLighting()) {
            applyEmissiveMask(fragmentShader, material().m_emissiveMap);
        }

        if (m_renderContext->rhiContext()->isValid()) {
            // Unlike the direct OpenGL path, with RHI "built-in" uniforms that are
            // not declared in snippets (or handled by the above branches) must
            // also go through the addUniform() mechanism.
            registerNonSnippetUnconditionalUniforms(fragmentShader);
        }

        // setup main
        vertexGenerator().beginFragmentGeneration();

        // since we do pixel lighting we always need this if lighting is enabled
        // We write this here because the functions below may also write to
        // the fragment shader
        if (material().hasLighting()) {
            vertexShader.generateWorldNormal(inKey);
            vertexShader.generateVarTangentAndBinormal(inKey);
            vertexShader.generateWorldPosition();

            if (material().isSpecularEnabled()) {
                vertexShader.generateViewVector();
            }
        }

        fragmentShader << "  initializeBaseFragmentVariables();\n"
                          "  computeTemporaries();\n"
                          "  normal = normalize( computeNormal() );\n"
                          "  initializeLayerVariables();\n"
                          "  float alpha = clamp( evalCutout(), 0.0, 1.0 );\n";

        if (material().isCutOutEnabled()) {
            fragmentShader << "  if ( alpha <= 0.0f )\n"
                              "    discard;\n";
        }

        // indirect / direct lightmap init
        if (material().hasLighting() && (lightmapIndirectImage || lightmapRadisoityImage))
            fragmentShader << "  initializeLayerVariablesWithLightmap();\n";

        // shadow map
        generateLightmapShadowCode(fragmentShader, lightmapShadowImage);

        // main Body
        fragmentShader << "#include \"customMaterialFragBodyAO.glsllib\"\n";

        // for us right now transparency means we render a glass style material
        if (m_hasTransparency && !material().isTransmissive())
            fragmentShader << " rgba = computeGlass( normal, materialIOR, alpha, rgba );\n";
        if (material().isTransmissive())
            fragmentShader << " rgba = computeOpacity( rgba );\n";

        fragmentShader << "  rgba.a *= objectOpacity;\n";
        fragmentShader << "  fragColor = rgba;\n";
        return false;
    }

    QSSGRef<QSSGRhiShaderStages> generateCustomMaterialRhiShader(const QByteArray &inShaderPrefix,
                                                                 const QByteArray &inCustomMaterialName)
    {
        // build a string that allows us to print out the shader we are generating to the log.
        // This is time consuming but I feel like it doesn't happen all that often and is very
        // useful to users
        // looking at the log file.
        QByteArray generatedShaderString;
        generatedShaderString = inShaderPrefix;
        generatedShaderString.append(inCustomMaterialName);
        QSSGShaderDefaultMaterialKey theKey(key());
        theKey.toString(generatedShaderString, m_defaultMaterialShaderKeyProperties);

        const bool hasCustomVertShader = generateVertexShader(theKey, inCustomMaterialName);
        const bool hasCustomFragShader = generateFragmentShader(theKey, inCustomMaterialName, hasCustomVertShader);

        vertexGenerator().endVertexGeneration(hasCustomVertShader);
        vertexGenerator().endFragmentGeneration(hasCustomFragShader);

        return programGenerator()->compileGeneratedRhiShader(generatedShaderString, QSSGShaderCacheProgramFlags(), m_currentFeatureSet);
    }

    QSSGRef<QSSGRhiShaderStages> generateRhiShaderStages(const QSSGRenderGraphObject &inMaterial,
                                                         QSSGShaderDefaultMaterialKey inShaderDescription,
                                                         QSSGStageGeneratorBase &inVertexPipeline,
                                                         const ShaderFeatureSetList &inFeatureSet,
                                                         const QVector<QSSGRenderLight *> &inLights,
                                                         QSSGRenderableImage *inFirstImage,
                                                         bool inHasTransparency,
                                                         const QByteArray &inShaderPrefix,
                                                         const QByteArray &inCustomMaterialName) override
    {
        Q_ASSERT(inMaterial.type == QSSGRenderGraphObject::Type::CustomMaterial);
        m_currentMaterial = static_cast<const QSSGRenderCustomMaterial *>(&inMaterial);
        m_currentKey = &inShaderDescription;
        m_currentPipeline = static_cast<QSSGVertexPipelineBase *>(&inVertexPipeline);
        m_currentFeatureSet = inFeatureSet;
        m_lights = inLights;
        m_firstImage = inFirstImage;
        m_hasTransparency = inHasTransparency;

        return generateCustomMaterialRhiShader(inShaderPrefix, inCustomMaterialName);
    }
};
} // namespace

QSSGRef<QSSGMaterialShaderGeneratorInterface> QSSGMaterialShaderGeneratorInterface::createCustomMaterialShaderGenerator(QSSGRenderContextInterface *inRc)
{
    return QSSGRef<QSSGMaterialShaderGeneratorInterface>(new QSSGShaderGenerator(inRc));
}

QT_END_NAMESPACE
