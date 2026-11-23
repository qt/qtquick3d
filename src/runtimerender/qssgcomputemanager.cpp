// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qssgcomputemanager_p.h"
#include "qssgrendercontextcore.h"
#include "extensionapi/qssgrenderextensions.h"
#include "qssgrendercommands_p.h"
#include "private/qssgrendercomputepass_p.h"
#include "qssgrendershadercodegenerator_p.h"
#include "qssgrendershadertokenizer_p.h"
#include "private/qssgrenderbuffer_p.h"
#include "private/qssgrenderer_p.h"
#include "private/qssglayerrenderdata_p.h"

QT_BEGIN_NAMESPACE

static constexpr int s_maxDirectionalLights = 16384u / sizeof(QSSGShaderDirectionalLightData);
static constexpr int s_maxPositionalLights = 16384u / sizeof(QSSGShaderLightData);

static const QByteArrayList cameraUniforms =
{
    QByteArrayLiteral("PROJECTION_MATRIX"), QByteArrayLiteral("qt_projectionMatrix"),
    QByteArrayLiteral("INVERSE_PROJECTION_MATRIX"), QByteArrayLiteral("qt_inverseProjectionMatrix"),
    QByteArrayLiteral("CAMERA_DIRECTION"), QByteArrayLiteral("qt_cameraDirection"),
    QByteArrayLiteral("CAMERA_PROPERTIES"), QByteArrayLiteral("qt_cameraProperties"),
    QByteArrayLiteral("CAMERA_POSITION"), QByteArrayLiteral("qt_cameraPosition"),
    QByteArrayLiteral("VIEW_MATRIX"), QByteArrayLiteral("qt_viewMatrix"),
    QByteArrayLiteral("INVERSE_VIEW_MATRIX"), QByteArrayLiteral("qt_inverseViewMatrix"),
    QByteArrayLiteral("VIEWPROJECTION_MATRIX"), QByteArrayLiteral("qt_viewProjectionMatrix"),
    QByteArrayLiteral("INVERSE_VIEWPROJECTION_MATRIX"), QByteArrayLiteral("qt_inverseViewProjectionMatrix")
};
static const QByteArrayList screenTextures =
{
    QByteArrayLiteral("SCREEN_TEXTURE"),
    QByteArrayLiteral("SCREEN_MIP_TEXTURE"),
    QByteArrayLiteral("DEPTH_TEXTURE"),
    QByteArrayLiteral("NORMAL_ROUGHNESS_TEXTURE"),
    QByteArrayLiteral("AO_TEXTURE")
};

static const QByteArrayList rhiProperties =
{
    QByteArrayLiteral("FRAMEBUFFER_Y_UP"),
    QByteArrayLiteral("NDC_Y_UP"),
    QByteArrayLiteral("NEAR_CLIP_VALUE")
};

static const QByteArrayList lightUniforms =
{
    QByteArrayLiteral("DIRECTIONAL_LIGHT_COUNT"),
    QByteArrayLiteral("DIRECTIONAL_LIGHT_DIFFUSE"),
    QByteArrayLiteral("DIRECTIONAL_LIGHT_DIRECTION"),
    QByteArrayLiteral("POINT_LIGHT_COUNT"),
    QByteArrayLiteral("POINT_LIGHT_DIFFUSE"),
    QByteArrayLiteral("POINT_LIGHT_POSITION"),
    QByteArrayLiteral("POINT_LIGHT_ATTENUATION"),
    QByteArrayLiteral("SPOT_LIGHT_COUNT"),
    QByteArrayLiteral("SPOT_LIGHT_DIFFUSE"),
    QByteArrayLiteral("SPOT_LIGHT_POSITION"),
    QByteArrayLiteral("SPOT_LIGHT_ATTENUATION"),
    QByteArrayLiteral("SPOT_LIGHT_DIRECTION"),
    QByteArrayLiteral("SPOT_LIGHT_CONES")
};

static const QByteArrayList positionalShadows =
{
    QByteArrayLiteral("SPOT_LIGHT_SHADOW"),
    QByteArrayLiteral("POINT_LIGHT_SHADOW"),
};

// The identifiers the shader code uses, outside comments.
static QSet<QByteArray> shaderIdentifiers(const QByteArray &shaderCode)
{
    QSet<QByteArray> identifiers;
    QSSGShaderTokenizer tok;
    tok.initialize(shaderCode.constData());
    for (auto t = tok.next(); t != QSSGShaderTokenizer::Token_EOF; t = tok.next()) {
        if (t == QSSGShaderTokenizer::Token_Identifier)
            identifiers.insert(QByteArray(tok.identifier, tok.pos - tok.identifier));
    }
    return identifiers;
}

static bool containsAny(const QSet<QByteArray> &identifiers, const QByteArrayList &list)
{
    for (const auto &a : list) {
        if (identifiers.contains(a))
            return true;
    }
    return false;
}

static quint32 softShadowQualityToInt(QSSGRenderLight::SoftShadowQuality quality)
{
    quint32 samplesCount = 0;
    switch (quality) {
    case QSSGRenderLight::SoftShadowQuality::Hard:
        samplesCount = 0;
        break;
    case QSSGRenderLight::SoftShadowQuality::PCF4:
        samplesCount = 4;
        break;
    case QSSGRenderLight::SoftShadowQuality::PCF8:
        samplesCount = 8;
        break;
    case QSSGRenderLight::SoftShadowQuality::PCF16:
    case QSSGRenderLight::SoftShadowQuality::PCF32:
    case QSSGRenderLight::SoftShadowQuality::PCF64:
        samplesCount = 16;
        break;
    }

    return samplesCount;
}

static QSSGRhiShaderPipelinePtr generateRhiShaderPipeline(const QByteArray &inKey,
                                                   QSSGRenderContextInterface *cif,
                                                   const QSSGRenderComputePass *pass,
                                                   const QSSGComputeLightContext &lightContext,
                                                   const bool multiview,
                                                   int viewCount)
{
    const auto &shaderCache = cif->shaderCache();
    const auto &shaderLibraryManager = cif->shaderLibraryManager();
    const auto &shaderProgramGenerator = cif->shaderProgramGenerator();
    QByteArray finalKey = inKey;
    finalKey += QByteArrayLiteral(";lights:");
    if (pass->computeShader.needsLights)
        finalKey += QByteArrayLiteral("true");
    else
        finalKey += QByteArrayLiteral("false");
    finalKey += QByteArrayLiteral(";shadows:");
    if (pass->computeShader.needsLights && (pass->computeShader.needsShadowsDirectional || pass->computeShader.needsShadowsPositional)) {
        finalKey += QByteArrayLiteral("true");
        finalKey += ";QSSG_SHADOW_SOFTNESS:"+QByteArray::number(softShadowQualityToInt(lightContext.maxSoftShadowQuality));
    } else {
        finalKey += QByteArrayLiteral("false");
    }

    // A shader that failed to compile is cached as null; return that instead of
    // compiling it again on every frame.
    if (shaderCache->hasRhiShaderPipeline(finalKey, {})) {
        auto pipeline = shaderCache->tryGetRhiShaderPipeline(finalKey, {});
        return (pipeline && !pipeline->isNull()) ? pipeline : nullptr;
    }

    // try persistent(disk based) cache
    const QByteArray qsbcKey = QQsbCollection::EntryDesc::generateSha(finalKey, {});
    auto pipeline = shaderCache->tryNewPipelineFromPersistentCache(qsbcKey, finalKey, {});
    if (pipeline && !pipeline->isNull())
        return pipeline;

    shaderProgramGenerator->beginProgram(QSSGShaderGeneratorStage::Compute);

    auto *cs = shaderProgramGenerator->getStage(QSSGShaderGeneratorStage::Compute);
    cs->addDefinition("QSSG_COMPUTE", "1");

    for (const auto &uniform : std::as_const(pass->computeShader.propertyUniforms)) {
        if (uniform.shaderDataType == QSSGRenderShaderValue::StorageBuffer) {
            const QSSGShaderBuffer buffer(uniform.value.value<QSSGShaderBuffer>());
            cs->addBuffer(uniform.name, buffer.dataType, buffer.qualifiers);
        } else if (uniform.shaderDataType == QSSGRenderShaderValue::Image) {
            const QSSGShaderImage image(uniform.value.value<QSSGShaderImage>());
            cs->addImage(uniform.name, image.imageType, image.format, image.qualifiers);
        } else {
            cs->addUniform(uniform.name, uniform.typeName);
        }
    }
    for (const auto &def : std::as_const(pass->computeShader.defines))
        cs->addDefinition(def.name, def.value);

    if (pass->computeShader.needsFrameIndex) {
        cs->addUniform("qt_frame_num", "float");
        cs->addDefinition("FRAME", "qt_frame_num");
    }
    if (pass->computeShader.needsCamera) {
        for (int i = 0; i < cameraUniforms.size() / 2; i++) {
            if (pass->computeShader.identifiers.contains(cameraUniforms[2 * i])) {
                if (multiview && cameraUniforms[2 * i] != "CAMERA_PROPERTIES") {
                    cs->addUniformArray(cameraUniforms[2 * i + 1], cameraUniforms[2 * i].contains("MATRIX") ? "mat4" : "vec3", viewCount);
                    cs->addDefinition(cameraUniforms[2 * i], cameraUniforms[2 * i + 1]);
                } else {
                    cs->addUniform(cameraUniforms[2 * i + 1], cameraUniforms[2 * i].contains("MATRIX") ? "mat4" : "vec3");
                    cs->addDefinition(cameraUniforms[2 * i], cameraUniforms[2 * i + 1]);
                }
            }
        }
    }
    if (pass->computeShader.needsRhiProperties || pass->computeShader.needsLights) {
        cs->addUniform("qt_rhi_properties", "vec3");
        cs->addDefinition("FRAMEBUFFER_Y_UP", "qt_rhi_properties.x");
        cs->addDefinition("NDC_Y_UP", "qt_rhi_properties.y");
        cs->addDefinition("NEAR_CLIP_VALUE", "qt_rhi_properties.z");
    }
    if (pass->computeShader.needsScreenTextures) {
        if (pass->computeShader.identifiers.contains(QByteArrayLiteral("SCREEN_TEXTURE"))
            || pass->computeShader.identifiers.contains(QByteArrayLiteral("SCREEN_MIP_TEXTURE"))) {
            cs->addUniform(multiview ? "qt_screenTextureArray" : "qt_screenTexture", multiview ? "sampler2DArray" : "sampler2D");
            cs->addDefinition("SCREEN_TEXTURE", multiview ? "qt_screenTextureArray" : "qt_screenTexture");
            cs->addDefinition("SCREEN_MIP_TEXTURE", multiview ? "qt_screenTextureArray" : "qt_screenTexture");
        }
        if (pass->computeShader.identifiers.contains(QByteArrayLiteral("DEPTH_TEXTURE"))) {
            cs->addUniform(multiview ? "qt_depthTextureArray" : "qt_depthTexture", multiview ? "sampler2DArray" : "sampler2D");
            cs->addDefinition("DEPTH_TEXTURE", multiview ? "qt_depthTextureArray" : "qt_depthTexture");
        }
        if (pass->computeShader.identifiers.contains(QByteArrayLiteral("AO_TEXTURE"))) {
            cs->addUniform(multiview ? "qt_aoTextureArray" : "qt_aoTexture", multiview ? "sampler2DArray" : "sampler2D");
            cs->addDefinition("AO_TEXTURE", multiview ? "qt_aoTextureArray" : "qt_aoTexture");
        }
    }
    if (pass->computeShader.needsNormalTexture) {
        cs->addUniform("qt_normalTexture", "sampler2D");
        cs->addDefinition("NORMAL_ROUGHNESS_TEXTURE", "qt_normalTexture");
    }
    if (pass->computeShader.needsLights) {
        cs->addUniform("qt_directionalLightCount", "uint");
        cs->addDefinition("DIRECTIONAL_LIGHT_COUNT", "qt_directionalLightCount");
        cs->addDefinition("DIRECTIONAL_LIGHT_DIFFUSE", "qt_directionalLightDiffuse");
        cs->addDefinition("DIRECTIONAL_LIGHT_DIRECTION", "qt_directionalLightDirection");

        cs->addUniform("qt_pointLightCount", "uint");
        cs->addDefinition("POINT_LIGHT_COUNT", "qt_pointLightCount");
        cs->addDefinition("POINT_LIGHT_DIFFUSE", "qt_pointLightDiffuse");
        cs->addDefinition("POINT_LIGHT_POSITION", "qt_pointLightPosition");
        cs->addDefinition("POINT_LIGHT_ATTENUATION", "qt_pointLightAttenuation");

        cs->addUniform("qt_spotLightCount", "uint");
        cs->addDefinition("SPOT_LIGHT_COUNT", "qt_spotLightCount");
        cs->addDefinition("SPOT_LIGHT_DIFFUSE", "qt_spotLightDiffuse");
        cs->addDefinition("SPOT_LIGHT_POSITION", "qt_spotLightPosition");
        cs->addDefinition("SPOT_LIGHT_ATTENUATION", "qt_spotLightAttenuation");
        cs->addDefinition("SPOT_LIGHT_DIRECTION", "qt_spotLightDirection");
        cs->addDefinition("SPOT_LIGHT_CONES", "qt_spotLightCones");

        cs->addDefinition("QSSG_MAX_POSITIONAL_LIGHTS", QByteArray::number(s_maxPositionalLights));
        cs->addDefinition("QSSG_MAX_DIRECTIONAL_LIGHTS", QByteArray::number(s_maxDirectionalLights));

        cs->addInclude("computeLights.glsllib");
        if (pass->computeShader.needsShadowsDirectional || pass->computeShader.needsShadowsPositional) {
            cs->addInclude("computeShadows.glsllib");
            cs->addDefinition("QSSG_SHADOW_SOFTNESS", QByteArray::number(softShadowQualityToInt(lightContext.maxSoftShadowQuality)));
            cs->addDefinition(QByteArrayLiteral("DIRECTIONAL_LIGHT_SHADOW"), QByteArrayLiteral("qt_dirLightShadowCompute"));
            cs->addDefinition(QByteArrayLiteral("POINT_LIGHT_SHADOW"), QByteArrayLiteral("qt_pointLightShadowCompute"));
            cs->addDefinition(QByteArrayLiteral("SPOT_LIGHT_SHADOW"), QByteArrayLiteral("qt_spotLightShadowCompute"));
        }
    }
    cs->append("#line 1\n");
    cs->append(pass->computeShader.body);
    return shaderProgramGenerator->compileGeneratedRhiShader(finalKey, {}, *shaderLibraryManager,
                                                             *shaderCache, {}, {}, multiview ? viewCount : 0, false);
}

bool finalizeComputeShader(QSSGUserComputeShader &computeShader)
{
    if (computeShader.body.isEmpty())
        return false;
    const auto shaderData = computeShader.body;
    QCryptographicHash hashData(QCryptographicHash::Algorithm::Sha1);
    hashData.addData(shaderData);
    // The declarations of images, buffers and plain uniforms are generated
    // from the metadata and the QML property types, not from the body, so
    // they all have to be part of the key.
    for (const auto &prop : std::as_const(computeShader.propertyUniforms)) {
        QByteArray u(prop.name + ":" + prop.typeName);
        if (prop.shaderDataType == QSSGRenderShaderValue::StorageBuffer) {
            const QSSGShaderBuffer buffer(prop.value.value<QSSGShaderBuffer>());
            u += ":" + buffer.dataType + ":" + QSSGBaseTypeHelpers::toString(buffer.qualifiers);
        } else if (prop.shaderDataType == QSSGRenderShaderValue::Image) {
            const QSSGShaderImage image(prop.value.value<QSSGShaderImage>());
            u += ":" + image.imageType + ":" + image.format + ":" + QSSGBaseTypeHelpers::toString(image.qualifiers);
        }
        u += ";";
        hashData.addData(u);
    }
    computeShader.hash = hashData.result().toHex();
    computeShader.identifiers = shaderIdentifiers(shaderData);
    const auto &ids = computeShader.identifiers;
    computeShader.needsCamera = containsAny(ids, cameraUniforms);
    computeShader.needsRhiProperties = containsAny(ids, rhiProperties);
    computeShader.needsScreenTextures = containsAny(ids, screenTextures);
    computeShader.needsNormalTexture = ids.contains(QByteArrayLiteral("NORMAL_ROUGHNESS_TEXTURE"));
    computeShader.needsFrameIndex = ids.contains(QByteArrayLiteral("FRAME"));
    computeShader.needsShadowsPositional = containsAny(ids, positionalShadows);
    computeShader.needsShadowsDirectional = ids.contains(QByteArrayLiteral("DIRECTIONAL_LIGHT_SHADOW"));
    computeShader.needsLights = containsAny(ids, lightUniforms) || computeShader.needsShadowsPositional || computeShader.needsShadowsDirectional;
    computeShader.supportsMultiview = ids.contains(QByteArrayLiteral("QSHADER_VIEW_COUNT"));
    return true;
}

void prepareComputePass(QSSGRenderer &renderer, QSSGLayerRenderData &data, QSSGRenderComputePass *passNode, ComputeUserPassData &userData, QSSGPassKey key)
{
    const auto &rhiCtx = renderer.contextInterface()->rhiContext();
    QSSGRhiContextPrivate *rhiCtxD = QSSGRhiContextPrivate::get(rhiCtx.get());
    auto &bufferManager(renderer.contextInterface()->bufferManager());
    if (!rhiCtx->rhi()->isFeatureSupported(QRhi::Compute)) {
        if (!data.computeUnsupportedWarningShown) {
            qWarning() << "Compute is not supported by the current RHI backend, ComputePass will be skipped.";
            data.computeUnsupportedWarningShown = true;
        }
        return;
    }
    if (!passNode->isReady())
        return;
    const bool multiview = data.layer.viewCount > 1;
    const bool needsMultiview = passNode->computeShader.needsScreenTextures || passNode->computeShader.needsCamera;

    if (!passNode->multiviewWarningShown && multiview && needsMultiview && !passNode->computeShader.supportsMultiview) {
        qWarning() << "Multiview enabled, but compute shader doesn't use QSHADER_VIEW_COUNT";
        passNode->multiviewWarningShown = true;
    }

    const int maxGroups = rhiCtx->rhi()->resourceLimit(QRhi::MaxThreadGroupsPerDimension);
    bool dispatchSet = false;
    bool noGroups = false;
    for (const auto &command : std::as_const(passNode->commands)) {
        switch (command->m_type) {
        case CommandType::Dispatch: {
            QSSGDispatch *disp = static_cast<QSSGDispatch*>(command);
            if (dispatchSet) {
                if (!passNode->dispatchMultiWarningShown) {
                    qWarning() << "Multiple dispatches in ComputePass. Using the first.";
                    passNode->dispatchMultiWarningShown = true;
                }
                continue;
            }
            if (!passNode->dispatchGroupsWarningShown
                && (disp->m_groupCount.x > maxGroups || disp->m_groupCount.y > maxGroups || disp->m_groupCount.z > maxGroups)) {
                qWarning() << "Dispatch exceeds maximum thread groups limit. " << maxGroups;
                passNode->dispatchGroupsWarningShown = true;
            }
            userData.m_groupCount = {qMin(maxGroups, disp->m_groupCount.x),
                                     qMin(maxGroups, disp->m_groupCount.y),
                                     qMin(maxGroups, disp->m_groupCount.z)};
            noGroups = userData.m_groupCount.x == 0 || userData.m_groupCount.y == 0 || userData.m_groupCount.z == 0;
            dispatchSet = true;
        } break;
        case CommandType::AddShaderDefine:
        {
            const auto *define = static_cast<const QSSGAddShaderDefine *>(command);
            const auto &defineName = define->m_name;
            if (defineName.size() > 0) {
                QByteArray value = QByteArray::number(define->m_value);
                userData.m_shaderDefines.push_back({ defineName, value });
            }
        } break;
        default:
            break;
        }
    }

    if (!dispatchSet || noGroups) {
        if (!passNode->dispatchMissingWarningShown && !dispatchSet) {
            qWarning() << "ComputePass has no Dispatch. ComputePass will be skipped";
            passNode->dispatchMissingWarningShown = true;
        }
        return;
    }

    QRhiResourceUpdateBatch *rub = rhiCtx->rhi()->nextResourceUpdateBatch();
    /* Lights are the same for all compute passes so we need do this only once per frame. */
    if (passNode->computeShader.needsLights && !data.computeLightsUpdated) {
        QList<QSSGShaderLight> dirs, spots, points;
        const auto &globalLights = data.globalLights;
        auto &lightContext = data.getComputeLightContext();
        lightContext.maxSoftShadowQuality = QSSGRenderLight::SoftShadowQuality::Hard;

        const auto lightIndex = [](const QSSGShaderLightList &lights, const QSSGShaderLight &light) -> int {
            for (int i = 0; i < lights.size(); i++)
                if (lights[i].light == light.light)
                    return i;
            return -1;
        };

        bool dirWarn = false;
        bool posWarn = false;

        for (const auto &l : globalLights) {
            if (l.shadows && l.light->m_softShadowQuality > lightContext.maxSoftShadowQuality)
                lightContext.maxSoftShadowQuality = l.light->m_softShadowQuality;
            if (l.light->type == QSSGRenderGraphObject::Type::DirectionalLight) {
                if (dirs.size() < s_maxDirectionalLights) {
                    dirs.append(l);
                } else if (!dirWarn) {
                    qWarning() << "ComputePass: Max directional lights reached.";
                    dirWarn = true;
                }
            } else if (l.light->type == QSSGRenderGraphObject::Type::SpotLight) {
                if ((spots.size() + points.size()) < s_maxPositionalLights) {
                    spots.append(l);
                } else if (!posWarn) {
                    qWarning() << "ComputePass: Max positional lights reached.";
                    posWarn = true;
                }
            } else if (l.light->type == QSSGRenderGraphObject::Type::PointLight) {
                if ((spots.size() + points.size()) < s_maxPositionalLights) {
                    points.append(l);
                } else if (!posWarn) {
                    qWarning() << "ComputePass: Max positional lights reached.";
                    posWarn = true;
                }
            }
        }

        if (!lightContext.dirLightBuffer) {
            lightContext.dirLightBuffer = rhiCtx->rhi()->newBuffer(QRhiBuffer::Static,
                                                                   QRhiBuffer::StorageBuffer,
                                                                   quint32(sizeof(QSSGShaderDirectionalLightData) * s_maxDirectionalLights));
            lightContext.dirLightBuffer->create();
            lightContext.dirLightData.resize(sizeof(QSSGShaderDirectionalLightData) * s_maxDirectionalLights);
            lightContext.dirLightData.fill(0);
        }
        if (!lightContext.lightBuffer) {
            lightContext.lightBuffer = rhiCtx->rhi()->newBuffer(QRhiBuffer::Static,
                                                                QRhiBuffer::StorageBuffer,
                                                                quint32(sizeof(QSSGShaderLightData) * s_maxPositionalLights));
            lightContext.lightBuffer->create();
            lightContext.lightData.resize(sizeof(QSSGShaderLightData) * s_maxPositionalLights);
            lightContext.lightData.fill(0);
        }
        lightContext.spotLights = spots.size();
        lightContext.pointLights = points.size();
        lightContext.dirLights = dirs.size();

        QSSGShaderDirectionalLightData *dirLightDataPtr = reinterpret_cast<QSSGShaderDirectionalLightData*>(lightContext.dirLightData.data());
        for (int idx = 0; idx < dirs.size(); idx++) {
            auto light = dirs[idx].light;
            auto &lightData = dirLightDataPtr[idx];
            const bool lightShadows = dirs[idx].shadows;
            const float brightness = light->m_brightness;
            const QVector3D diffuseColor(light->m_diffuseColor.x() * brightness,
                                         light->m_diffuseColor.y() * brightness,
                                         light->m_diffuseColor.z() * brightness);
            const QVector3D specularColor(light->m_specularColor.x() * brightness,
                                          light->m_specularColor.y() * brightness,
                                          light->m_specularColor.z() * brightness);
            lightData.lightmapState = 0;
            lightData.direction[0] = dirs[idx].direction.x();
            lightData.direction[1] = dirs[idx].direction.y();
            lightData.direction[2] = dirs[idx].direction.z();
            lightData.shadowBias = 0.0f;
            lightData.diffuseColor[0] = diffuseColor.x();
            lightData.diffuseColor[1] = diffuseColor.y();
            lightData.diffuseColor[2] = diffuseColor.z();
            lightData.shadowFactor = 0.0f;
            lightData.specularColor[0] = specularColor.x();
            lightData.specularColor[1] = specularColor.y();
            lightData.specularColor[2] = specularColor.z();
            lightData.enableShadows = 0.0f;
            if (lightShadows && data.getShadowMapManager()) {
                QSSGShadowMapEntry *pEntry = data.getShadowMapManager()->shadowMapEntry(lightIndex(data.globalLights, dirs[idx]));
                if (pEntry) {
                    const quint32 layerCount = pEntry->m_csmNumSplits + 1;

                    for (quint32 i = 0; i < layerCount; ++i)
                        memcpy(lightData.matrices[i], pEntry->m_fixedScaleBiasMatrix[i].constData(), 16 * sizeof(float));

                    lightData.shadowBias = light->m_shadowBias;
                    lightData.shadowFactor = light->m_shadowFactor;
                    lightData.shadowMapFar = light->m_shadowMapFar;
                    lightData.shadowPcfSamples = softShadowQualityToInt(light->m_softShadowQuality);
                    lightData.shadowPcfFactor = light->m_pcfFactor;

                    for (quint32 i = 0; i < layerCount; ++i) {
                        const auto &atlasInfo = pEntry->m_atlasInfo[i];
                        lightData.atlasLocations[i][0] = atlasInfo.uOffset;
                        lightData.atlasLocations[i][1] = atlasInfo.vOffset;
                        lightData.atlasLocations[i][2] = atlasInfo.uvScale;
                        lightData.atlasLocations[i][3] = atlasInfo.layerIndex;
                    }

                    lightData.csmNumSplits = pEntry->m_csmNumSplits;
                    memcpy(lightData.csmSplits, pEntry->m_csmSplits, 4 * sizeof(float));
                    memcpy(lightData.csmActive, pEntry->m_csmActive, 4 * sizeof(float));
                    lightData.csmBlendRatio = light->m_csmBlendRatio;
                    for (quint32 i = 0; i < layerCount; ++i)
                        memcpy(lightData.dimensionsInverted[i], &pEntry->m_dimensionsInverted[i], 4 * sizeof(float));
                    if (pEntry->hasActiveCascades())
                        lightData.enableShadows = 1.0f;
                }
            }
        }

        const auto handlePositionalLight = [&data, lightIndex](const QSSGShaderLight &shaderLight, QSSGShaderLightData &lightData) {
            auto light = shaderLight.light;
            const bool lightShadows = shaderLight.shadows;
            const float brightness = light->m_brightness;
            const QVector3D diffuseColor(light->m_diffuseColor.x() * brightness,
                                         light->m_diffuseColor.y() * brightness,
                                         light->m_diffuseColor.z() * brightness);
            const QVector3D specularColor(light->m_specularColor.x() * brightness,
                                          light->m_specularColor.y() * brightness,
                                          light->m_specularColor.z() * brightness);
            lightData.lightmapState = 0;
            lightData.direction[0] = shaderLight.direction.x();
            lightData.direction[1] = shaderLight.direction.y();
            lightData.direction[2] = shaderLight.direction.z();
            lightData.shadowBias = 0.0f;
            lightData.diffuseColor[0] = diffuseColor.x();
            lightData.diffuseColor[1] = diffuseColor.y();
            lightData.diffuseColor[2] = diffuseColor.z();
            lightData.shadowFactor = 0.0f;
            lightData.specularColor[0] = specularColor.x();
            lightData.specularColor[1] = specularColor.y();
            lightData.specularColor[2] = specularColor.z();
            const auto gt = data.getGlobalTransform(*light);
            const QVector3D globalPos = QSSGRenderNode::getGlobalPos(gt);
            lightData.position[0] = globalPos.x();
            lightData.position[1] = globalPos.y();
            lightData.position[2] = globalPos.z();
            lightData.constantAttenuation = QSSGUtils::aux::translateConstantAttenuation(light->m_constantFade);
            lightData.linearAttenuation = QSSGUtils::aux::translateLinearAttenuation(light->m_linearFade);
            lightData.quadraticAttenuation = QSSGUtils::aux::translateQuadraticAttenuation(light->m_quadraticFade);
            lightData.coneAngle = 360.0f;
            if (light->type == QSSGRenderLight::Type::SpotLight) {
                // NB: This is how we tell in the shader that this is a spot light
                // PointLights have a coneAngle of 360.0f which is greater than 1.0
                // Since cos(any value) will be between -1 and 1, we can use this.
                const float coneAngle = light->m_coneAngle;
                const float innerConeAngle = (light->m_innerConeAngle > coneAngle) ? coneAngle : light->m_innerConeAngle;
                lightData.coneAngle = qCos(qDegreesToRadians(coneAngle));
                lightData.innerConeAngle = qCos(qDegreesToRadians(innerConeAngle));
            }
            lightData.enableShadows = 0.0f;
            if (lightShadows && data.getShadowMapManager()) {
                QSSGShadowMapEntry *pEntry = data.getShadowMapManager()->shadowMapEntry(lightIndex(data.globalLights, shaderLight));
                if (pEntry) {
                    lightData.enableShadows = 1.0f;
                    lightData.shadowPcfFactor = light->m_pcfFactor;
                    lightData.shadowPcfSamples = softShadowQualityToInt(light->m_softShadowQuality);
                    lightData.shadowFactor = light->m_shadowFactor;
                    lightData.shadowBias = light->m_shadowBias;
                    lightData.shadowClipNear = 1.0f;
                    lightData.shadowMapFar = pEntry->m_shadowMapFar;
                    lightData.shadowTextureSize = pEntry->m_atlasInfo[0].uvScale;

                    if (light->type == QSSGRenderLight::Type::SpotLight) {
                        // add fixed scale bias matrix
                        static const QMatrix4x4 bias = {
                                                         0.5, 0.0, 0.0, 0.5,
                                                         0.0, 0.5, 0.0, 0.5,
                                                         0.0, 0.0, 0.5, 0.5,
                                                         0.0, 0.0, 0.0, 1.0 };
                        const QMatrix4x4 m = bias * pEntry->m_lightViewProjection[0];
                        memcpy(lightData.shadowMatrix, m.constData(), 16 * sizeof(float));
                        lightData.shadowAtlasUV0[0] = pEntry->m_atlasInfo[0].uOffset;
                        lightData.shadowAtlasUV0[1] = pEntry->m_atlasInfo[0].vOffset;
                        lightData.shadowAtlasLayer0 = pEntry->m_atlasInfo[0].layerIndex;

                    } else {
                        Q_ASSERT(light->type == QSSGRenderLight::Type::PointLight);
                        memcpy(lightData.shadowMatrix, pEntry->m_lightView.constData(), 16 * sizeof(float));
                        lightData.shadowAtlasUV0[0] = pEntry->m_atlasInfo[0].uOffset;
                        lightData.shadowAtlasUV0[1] = pEntry->m_atlasInfo[0].vOffset;
                        lightData.shadowAtlasLayer0 = pEntry->m_atlasInfo[0].layerIndex;
                        lightData.shadowAtlasUV1[0] = pEntry->m_atlasInfo[1].uOffset;
                        lightData.shadowAtlasUV1[1] = pEntry->m_atlasInfo[1].vOffset;
                        lightData.shadowAtlasLayer1 = pEntry->m_atlasInfo[1].layerIndex;
                    }
                }
            }
        };
        QSSGShaderLightData *lightDataPtr = reinterpret_cast<QSSGShaderLightData*>(lightContext.lightData.data());
        for (int i = 0; i < points.size(); i++)
            handlePositionalLight(points[i], lightDataPtr[i]);
        for (int i = 0; i < spots.size(); i++)
            handlePositionalLight(spots[i], lightDataPtr[i + points.size()]);

        if (lightContext.dirLightBuffer)
            rub->uploadStaticBuffer(lightContext.dirLightBuffer, 0, lightContext.dirLightData);
        if (lightContext.lightBuffer)
            rub->uploadStaticBuffer(lightContext.lightBuffer, 0, lightContext.lightData);
        data.computeLightsUpdated = true;
    }

    passNode->computeShader.defines = userData.m_shaderDefines;
    QByteArray defineKey;
    for (const auto &def : std::as_const(userData.m_shaderDefines))
        defineKey.append(def.name + "=" + def.value + ";");

    // Add viewCount
    if (multiview)
        defineKey.append("viewCount=" + QByteArray::number(data.layer.viewCount) + ";");

    userData.m_computePipeline = generateRhiShaderPipeline("compute - " + passNode->computeShader.hash + " - " + defineKey,
                                                           renderer.contextInterface(), passNode, data.getComputeLightContextConst(),
                                                           multiview && needsMultiview,
                                                           data.layer.viewCount);
    if (userData.m_computePipeline == nullptr) {
        if (!std::exchange(passNode->pipelineWarningShown, true))
            qWarning() << "Unable to initialize compute pass.";
        rhiCtx->commandBuffer()->resourceUpdate(rub);
        return;
    }
    passNode->pipelineWarningShown = false;

    QSSGRhiDrawCallData &dcd(rhiCtxD->drawCallData({ key, passNode, nullptr, nullptr, 0 }));
    userData.m_computePipeline->ensureUniformBuffer(&dcd.ubuf);
    {
        QSSGRhiShaderResourceBindingList bindings;
        bindings.addUniformBuffer(0, QRhiShaderResourceBinding::ComputeStage, dcd.ubuf);

        for (const auto &prop : std::as_const(passNode->computeShader.propertyUniforms)) {
            switch (prop.shaderDataType) {
            case QSSGRenderShaderValue::Texture: {
                QSSGRenderImage *image = prop.value.value<QSSGRenderImage*>();
                int binding = userData.m_computePipeline->bindingForTexture(prop.name);
                if (binding == -1)
                    continue;
                bool dummyRequired = true;
                if (image) {
                    const QSSGRenderImageTexture texture = bufferManager->loadRenderImage(image);
                    if (texture.m_texture) {
                        dummyRequired = false;
                        QSSGRhiSamplerDescription desc{
                                                         QSSGRhiHelpers::toRhi(image->m_minFilterType),
                                                         QSSGRhiHelpers::toRhi(image->m_magFilterType),
                                                         image->m_mipFilterType != QSSGRenderTextureFilterOp::None ? QSSGRhiHelpers::toRhi(image->m_mipFilterType) : QRhiSampler::None,
                                                         QSSGRhiHelpers::toRhi(image->m_horizontalTilingMode),
                                                         QSSGRhiHelpers::toRhi(image->m_verticalTilingMode),
                                                         QSSGRhiHelpers::toRhi(image->m_depthTilingMode)};
                        QRhiSampler *sampler = rhiCtx->sampler(desc);
                        bindings.addTexture(binding, QRhiShaderResourceBinding::ComputeStage, texture.m_texture, sampler);
                    }
                }
                if (dummyRequired) {
                    QRhiTexture *texture = rhiCtx->dummyTexture({}, rub, QSize(4, 4), Qt::white);
                    QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Nearest,
                            QRhiSampler::Nearest,
                            QRhiSampler::None,
                            QRhiSampler::ClampToEdge,
                            QRhiSampler::ClampToEdge,
                            QRhiSampler::Repeat
                    });
                    bindings.addTexture(binding, QRhiShaderResourceBinding::ComputeStage, texture, sampler);
                }
            } break;
            case QSSGRenderShaderValue::Image:
            {
                const QSSGShaderImage image(prop.value.value<QSSGShaderImage>());
                int binding = userData.m_computePipeline->bindingForStorageImage(prop.name);
                if (binding == -1)
                    continue;
                QRhiTexture *rhiTex = nullptr;
                if (image.image)
                    rhiTex = bufferManager->loadRenderImage(image.image).m_texture;
                if (rhiTex) {
                    if (image.imageType.contains("Cube") && !(rhiTex->flags() & QRhiTexture::CubeMap)) {
                        // We should bail out as running the shader is not safe with incorrect texture type
                        userData.m_computePipeline = nullptr;
                        if (!std::exchange(passNode->bindingWarningShown, true))
                            qWarning() << "Unable to run compute shader: bound image is not CubeMap: " << prop.name;
                        rhiCtx->commandBuffer()->resourceUpdate(rub);
                        return;
                    }
                    if (image.qualifiers.testFlag(QSSGMemoryQualifier::Readonly))
                        bindings.addImageLoad(binding, QRhiShaderResourceBinding::ComputeStage, rhiTex, 0);
                    else if (image.qualifiers.testFlag(QSSGMemoryQualifier::Writeonly))
                        bindings.addImageStore(binding, QRhiShaderResourceBinding::ComputeStage, rhiTex, 0);
                    else
                        bindings.addImageLoadStore(binding, QRhiShaderResourceBinding::ComputeStage, rhiTex, 0);
                } else {
                    // We should bail out as running the shader is not safe with incorrect sized image
                    userData.m_computePipeline = nullptr;
                    if (!std::exchange(passNode->bindingWarningShown, true))
                        qWarning() << "Unable to run compute shader: image not bound: " << prop.name;
                    rhiCtx->commandBuffer()->resourceUpdate(rub);
                    return;
                }
            } break;
            case QSSGRenderShaderValue::StorageBuffer:
            {
                const QSSGShaderBuffer buffer(prop.value.value<QSSGShaderBuffer>());
                int binding = userData.m_computePipeline->bindingForStorageBuffer(prop.name);
                if (binding == -1)
                    continue;
                QRhiBuffer *rhiBuf = nullptr;
                if (buffer.buffer)
                    rhiBuf = bufferManager->loadBuffer(*buffer.buffer);
                if (rhiBuf) {
                    if (buffer.qualifiers.testFlag(QSSGMemoryQualifier::Readonly))
                        bindings.addStorageBufferLoad(binding, QRhiShaderResourceBinding::ComputeStage, rhiBuf);
                    else if (buffer.qualifiers.testFlag(QSSGMemoryQualifier::Writeonly))
                        bindings.addStorageBufferStore(binding, QRhiShaderResourceBinding::ComputeStage, rhiBuf);
                    else
                        bindings.addStorageBufferLoadStore(binding, QRhiShaderResourceBinding::ComputeStage, rhiBuf);
                } else {
                    // We should bail out as running the shader is not safe with incorrect sized buffer
                    userData.m_computePipeline = nullptr;
                    if (!std::exchange(passNode->bindingWarningShown, true))
                        qWarning() << "Unable to run compute shader: buffer not bound: " << prop.name;
                    rhiCtx->commandBuffer()->resourceUpdate(rub);
                    return;
                }
            } break;
            default:
                break;
            }
        }
        bufferManager->commitBufferResourceUpdates();
        // Everything got bound, so warn again if binding fails later on.
        passNode->bindingWarningShown = false;

        if (passNode->computeShader.needsLights) {
            bindings.addStorageBufferLoad(1, QRhiShaderResourceBinding::ComputeStage, data.getComputeLightContextConst().lightBuffer);
            bindings.addStorageBufferLoad(2, QRhiShaderResourceBinding::ComputeStage, data.getComputeLightContextConst().dirLightBuffer);
        }
        if (passNode->computeShader.needsShadowsDirectional || passNode->computeShader.needsShadowsPositional) {
            auto smMgr = data.getShadowMapManager();
            auto atlasTexture = smMgr ? data.getShadowMapManager()->shadowMapAtlasTexture() : nullptr;
            auto noiseTexture = smMgr ? data.getShadowMapManager()->shadowMapBlueNoiseTexture() : nullptr;
            int binding = userData.m_computePipeline->bindingForTexture("qt_shadowmap_texture");
            if (binding >= 0) {
                QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Linear,
                                                         QRhiSampler::Linear,
                                                         QRhiSampler::None,
                                                         QRhiSampler::ClampToEdge,
                                                         QRhiSampler::ClampToEdge,
                                                         QRhiSampler::Repeat });
                if (!atlasTexture)
                    atlasTexture = rhiCtx->dummyTexture({ }, rub, QSize(1, 1), Qt::black, 2);
                bindings.addTexture(binding, QRhiShaderResourceBinding::ComputeStage, atlasTexture, sampler);
            }
            binding = userData.m_computePipeline->bindingForTexture("qt_shadowmap_blue_noise_texture");
            if (binding >= 0) {
                QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Linear,
                                                         QRhiSampler::Linear,
                                                         QRhiSampler::None,
                                                         QRhiSampler::Repeat,
                                                         QRhiSampler::Repeat,
                                                         QRhiSampler::Repeat });
                if (!noiseTexture)
                    noiseTexture = rhiCtx->dummyTexture({}, rub);
                bindings.addTexture(binding, QRhiShaderResourceBinding::ComputeStage, noiseTexture, sampler);
            }
        }
        if (passNode->computeShader.needsScreenTextures) {
            const QSSGRhiRenderableTexture *depthTexture = data.getRenderResult(QSSGRenderResult::Key::DepthTexture);
            const QSSGRhiRenderableTexture *normalTexture = data.getRenderResult(QSSGRenderResult::Key::NormalTexture);
            const QSSGRhiRenderableTexture *ssaoTexture = data.getRenderResult(QSSGRenderResult::Key::AoTexture);
            const QSSGRhiRenderableTexture *screenTexture = data.getRenderResult(QSSGRenderResult::Key::ScreenTexture);
            const int screenTextureBinding = userData.m_computePipeline->bindingForTexture("qt_screenTexture", int(QSSGRhiSamplerBindingHints::ScreenTexture));
            const int screenTextureArrayBinding = userData.m_computePipeline->bindingForTexture("qt_screenTextureArray", int(QSSGRhiSamplerBindingHints::ScreenTextureArray));
            if (screenTextureBinding >= 0 || screenTextureArrayBinding >= 0) {
                QRhiSampler::Filter mipFilter = QRhiSampler::None;
                if (screenTexture->texture) {
                    mipFilter = screenTexture->texture->flags().testFlag(QRhiTexture::MipMapped)
                                ? QRhiSampler::Linear : QRhiSampler::None;
                }
                QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Linear, QRhiSampler::Linear, mipFilter,
                                                         QRhiSampler::Repeat, QRhiSampler::Repeat, QRhiSampler::Repeat });
                QRhiTexture *texture = screenTexture->texture;
                if (!texture)
                    texture = rhiCtx->dummyTexture({}, rub, QSize(4, 4), Qt::white);
                if (screenTextureBinding >= 0) {
                    bindings.addTexture(screenTextureBinding, QRhiShaderResourceBinding::ComputeStage, texture, sampler);
                } else if (screenTextureArrayBinding >= 0) {
                    bindings.addTexture(screenTextureArrayBinding, QRhiShaderResourceBinding::ComputeStage, texture, sampler);
                }
            }
            const int depthTextureBinding = userData.m_computePipeline->bindingForTexture("qt_depthTexture", int(QSSGRhiSamplerBindingHints::DepthTexture));
            const int depthTextureArrayBinding = userData.m_computePipeline->bindingForTexture("qt_depthTextureArray", int(QSSGRhiSamplerBindingHints::DepthTextureArray));
            if (depthTextureBinding >= 0 || depthTextureArrayBinding >= 0) {
                QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Nearest, QRhiSampler::Nearest, QRhiSampler::None,
                                                         QRhiSampler::Repeat, QRhiSampler::Repeat, QRhiSampler::Repeat });
                QRhiTexture *texture = depthTexture->texture;
                if (!texture)
                    texture = rhiCtx->dummyTexture({}, rub, QSize(4, 4), Qt::white);
                if (depthTextureBinding >= 0) {
                    bindings.addTexture(depthTextureBinding, QRhiShaderResourceBinding::ComputeStage, texture, sampler);
                } else if (depthTextureArrayBinding >= 0) {
                    bindings.addTexture(depthTextureArrayBinding, QRhiShaderResourceBinding::ComputeStage, texture, sampler);
                }
            }
            const int aoTextureBinding = userData.m_computePipeline->bindingForTexture("qt_aoTexture", int(QSSGRhiSamplerBindingHints::AoTexture));
            const int aoTextureArrayBinding = userData.m_computePipeline->bindingForTexture("qt_aoTextureArray", int(QSSGRhiSamplerBindingHints::AoTextureArray));
            if (aoTextureBinding >= 0 || aoTextureArrayBinding >= 0) {
                QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                                         QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge, QRhiSampler::Repeat });
                QRhiTexture *texture = ssaoTexture->texture;
                if (!texture)
                    texture = rhiCtx->dummyTexture({}, rub, QSize(4, 4), Qt::white);
                if (aoTextureBinding >= 0) {
                    bindings.addTexture(aoTextureBinding, QRhiShaderResourceBinding::ComputeStage, texture, sampler);
                } else if (aoTextureArrayBinding >= 0) {
                    bindings.addTexture(aoTextureArrayBinding, QRhiShaderResourceBinding::ComputeStage, texture, sampler);
                }
            }
            const int normalTextureBinding = userData.m_computePipeline->bindingForTexture("qt_normalTexture", int(QSSGRhiSamplerBindingHints::NormalTexture));
            if (normalTextureBinding >= 0) {
                QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Nearest, QRhiSampler::Nearest, QRhiSampler::None,
                                                         QRhiSampler::Repeat, QRhiSampler::Repeat, QRhiSampler::Repeat });
                QRhiTexture *texture = normalTexture->texture;
                if (!texture)
                    texture = rhiCtx->dummyTexture({}, rub, QSize(4, 4), Qt::white);
                bindings.addTexture(normalTextureBinding, QRhiShaderResourceBinding::ComputeStage, texture, sampler);
            }
        }
        if (!dcd.srb || bindings != dcd.bindings) {
            dcd.srb = rhiCtxD->srb(bindings);
            rhiCtxD->releaseCachedSrb(dcd.bindings);
            dcd.bindings = bindings;
        }
    }

    userData.m_computeBindings = dcd.srb;

    char *ubufData = dcd.ubuf->beginFullDynamicBufferUpdateForCurrentFrame();

    auto *p = userData.m_computePipeline.get();
    for (const auto &prop : std::as_const(passNode->computeShader.propertyUniforms)) {
        switch (prop.shaderDataType) {
        case QSSGRenderShaderValue::Texture:
        case QSSGRenderShaderValue::StorageBuffer:
        case QSSGRenderShaderValue::Unknown:
        case QSSGRenderShaderValue::Image:
            break;
        default:
            userData.m_computePipeline->setUniformValue(ubufData, prop.name, prop.value, prop.shaderDataType);
            break;
        }
    }

    if (passNode->computeShader.needsFrameIndex) {
        float fc = float(renderer.frameCount());
        userData.m_computePipeline->setUniformValue(ubufData, "qt_frame_num", fc, QSSGRenderShaderValue::Float);
    }

    if (passNode->computeShader.needsRhiProperties || passNode->computeShader.needsLights) {
        QVector3D qt_rhi_properties;
        const auto globalRenderData = QSSGLayerRenderData::globalRenderProperties(*renderer.contextInterface());
        qt_rhi_properties.setX(globalRenderData.isYUpInFramebuffer ? 1.0f : -1.0f);
        qt_rhi_properties.setY(globalRenderData.isYUpInNDC ? 1.0f : -1.0f);
        qt_rhi_properties.setZ(globalRenderData.isClipDepthZeroToOne ? 0.0f : -1.0f);
        userData.m_computePipeline->setUniformValue(ubufData, "qt_rhi_properties", qt_rhi_properties, QSSGRenderShaderValue::Vec3);
    }
    if (passNode->computeShader.needsCamera) {
        QSSG_ASSERT(!data.renderedCameras.isEmpty(),
                    dcd.ubuf->endFullDynamicBufferUpdateForCurrentFrame();
                    rhiCtx->commandBuffer()->resourceUpdate(rub);
                    return);
        const QVector3D camProperties(data.renderedCameras[0]->clipPlanes[0], data.renderedCameras[0]->clipPlanes[1], 0.0f);
        p->setUniformValue(ubufData, "qt_cameraProperties", camProperties, QSSGRenderShaderValue::Vec3);
        const auto clipCorrMatrix = rhiCtx->rhi()->clipSpaceCorrMatrix();

        if (multiview) {
            int viewCount = data.layer.viewCount;
            QVarLengthArray<QMatrix4x4> cameraGlobalMatrix(viewCount);
            QVarLengthArray<QMatrix4x4> projection(viewCount);
            QVarLengthArray<QMatrix4x4> invProjection(viewCount);
            QVarLengthArray<QVector3D>  dir(viewCount);
            QVarLengthArray<QMatrix4x4> viewMatrix(viewCount);
            QVarLengthArray<QMatrix4x4> viewProjection(viewCount);
            QVarLengthArray<QMatrix4x4> invViewMatrix(viewCount);
            QVarLengthArray<QMatrix4x4> invViewProjection(viewCount);
            QVarLengthArray<QVector3D> camGlobalPos(viewCount);

            for (int i = 0; i < viewCount; i++) {
                QSSGRenderCamera *camera = data.renderedCameras[i];
                cameraGlobalMatrix[i] = data.getGlobalTransform(*camera);
                projection[i] = clipCorrMatrix * camera->projection;
                invProjection[i] = projection[i].inverted();
                dir[i] = data.renderedCameraData.value()[i].direction;
                viewMatrix[i] = cameraGlobalMatrix[i].inverted();
                camera->calculateViewProjectionMatrix(cameraGlobalMatrix[i], viewProjection[i]);
                viewProjection[i] = clipCorrMatrix * viewProjection[i];
                invViewMatrix[i] = cameraGlobalMatrix[i];
                invViewProjection[i] = viewProjection[i].inverted();
                camGlobalPos[i] = QSSGRenderNode::getGlobalPos(cameraGlobalMatrix[i]);
            }
            p->setUniformArray(ubufData, "qt_viewMatrix", viewMatrix.constData(), viewCount, QSSGRenderShaderValue::Matrix4x4);
            p->setUniformArray(ubufData, "qt_viewProjectionMatrix", viewProjection.constData(), viewCount, QSSGRenderShaderValue::Matrix4x4);
            p->setUniformArray(ubufData, "qt_inverseViewMatrix", invViewMatrix.constData(), viewCount, QSSGRenderShaderValue::Matrix4x4);
            p->setUniformArray(ubufData, "qt_inverseViewProjectionMatrix", invViewProjection.constData(), viewCount, QSSGRenderShaderValue::Matrix4x4);
            p->setUniformArray(ubufData, "qt_projectionMatrix", projection.constData(), viewCount, QSSGRenderShaderValue::Matrix4x4);
            p->setUniformArray(ubufData, "qt_inverseProjectionMatrix", invProjection.constData(), viewCount, QSSGRenderShaderValue::Matrix4x4);
            p->setUniformArray(ubufData, "qt_cameraPosition", camGlobalPos.constData(), viewCount, QSSGRenderShaderValue::Vec3);
            p->setUniformArray(ubufData, "qt_cameraDirection", dir.constData(), viewCount, QSSGRenderShaderValue::Vec3);
        } else {
            QSSGRenderCamera *camera = data.renderedCameras[0];
            QMatrix4x4 cameraGlobalMatrix = data.getGlobalTransform(*camera);
            QMatrix4x4 projection = clipCorrMatrix * camera->projection;
            QMatrix4x4 invProjection = projection.inverted();
            auto &dir = data.renderedCameraData.value()[0].direction;
            QMatrix4x4 viewMatrix = cameraGlobalMatrix.inverted();
            QMatrix4x4 viewProjection(Qt::Uninitialized);
            camera->calculateViewProjectionMatrix(cameraGlobalMatrix, viewProjection);
            viewProjection = clipCorrMatrix * viewProjection;
            QMatrix4x4 invViewMatrix = cameraGlobalMatrix;
            QMatrix4x4 invViewProjection = viewProjection.inverted();
            p->setUniformValue(ubufData, "qt_viewMatrix", viewMatrix, QSSGRenderShaderValue::Matrix4x4);
            p->setUniformValue(ubufData, "qt_viewProjectionMatrix", viewProjection, QSSGRenderShaderValue::Matrix4x4);
            p->setUniformValue(ubufData, "qt_inverseViewMatrix", invViewMatrix, QSSGRenderShaderValue::Matrix4x4);
            p->setUniformValue(ubufData, "qt_inverseViewProjectionMatrix", invViewProjection, QSSGRenderShaderValue::Matrix4x4);
            p->setUniformValue(ubufData, "qt_projectionMatrix", projection, QSSGRenderShaderValue::Matrix4x4);
            p->setUniformValue(ubufData, "qt_inverseProjectionMatrix", invProjection, QSSGRenderShaderValue::Matrix4x4);
            const QVector3D camGlobalPos = QSSGRenderNode::getGlobalPos(cameraGlobalMatrix);
            p->setUniformValue(ubufData, "qt_cameraPosition", camGlobalPos, QSSGRenderShaderValue::Vec3);
            p->setUniformValue(ubufData, "qt_cameraDirection", dir, QSSGRenderShaderValue::Vec3);
        }
    }
    if (passNode->computeShader.needsLights) {
        const auto &lc = data.getComputeLightContextConst();
        p->setUniformValue(ubufData, "qt_directionalLightCount", lc.dirLights, QSSGRenderShaderValue::UnsignedInteger);
        p->setUniformValue(ubufData, "qt_spotLightCount", lc.spotLights, QSSGRenderShaderValue::UnsignedInteger);
        p->setUniformValue(ubufData, "qt_pointLightCount", lc.pointLights, QSSGRenderShaderValue::UnsignedInteger);
    }

    dcd.ubuf->endFullDynamicBufferUpdateForCurrentFrame();
    rhiCtx->commandBuffer()->resourceUpdate(rub);
}

QT_END_NAMESPACE
