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

#include "qssgrhicustommaterialsystem_p.h"

#include <QtQuick3DUtils/private/qssgutils_p.h>

#include <QtQuick3DRuntimeRender/private/qssgrendercontextcore_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderbuffermanager_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderresourcemanager_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendermesh_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercamera_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderlight_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderlayer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderableimage_p.h>
#include <QtQuick3DRuntimeRender/private/qssgvertexpipelineimpl_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendererimpllayerrenderdata_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendermodel_p.h>
#include <QtQuick3DRuntimeRender/private/qssgruntimerenderlogging_p.h>

#include <QtCore/qbitarray.h>

QT_BEGIN_NAMESPACE

QSSGCustomMaterialRenderContext::QSSGCustomMaterialRenderContext(const QSSGRenderLayer &inLayer,
                                                                 const QSSGLayerRenderData &inData,
                                                                 const QSSGShaderLightList &inLights,
                                                                 QSSGRenderCamera &inCamera,
                                                                 const QSSGRenderModel &inModel,
                                                                 const QSSGRenderSubset &inSubset,
                                                                 const QMatrix4x4 &inMvp,
                                                                 const QMatrix4x4 &inWorld,
                                                                 const QMatrix3x3 &inNormal,
                                                                 const QSSGRenderCustomMaterial &inMaterial,
                                                                 QRhiTexture *inRhiDepthTex,
                                                                 QRhiTexture *inRhiAoTex,
                                                                 QRhiTexture *inRhiScreenTex,
                                                                 QSSGShaderDefaultMaterialKey inMaterialKey,
                                                                 QSSGRenderableImage *inFirstImage,
                                                                 float inOpacity)
    : layer(inLayer)
    , layerData(inData)
    , lights(inLights)
    , camera(inCamera)
    , model(inModel)
    , subset(inSubset)
    , modelViewProjection(inMvp)
    , modelMatrix(inWorld)
    , normalMatrix(inNormal)
    , material(inMaterial)
    , rhiDepthTexture(inRhiDepthTex)
    , rhiAoTexture(inRhiAoTex)
    , rhiScreenTexture(inRhiScreenTex)
    , materialKey(inMaterialKey)
    , firstImage(inFirstImage)
    , opacity(inOpacity)
{
}

QSSGCustomMaterialRenderContext::~QSSGCustomMaterialRenderContext() = default;

QSSGCustomMaterialSystem::QSSGCustomMaterialSystem() = default;

QSSGCustomMaterialSystem::~QSSGCustomMaterialSystem()
{
}

QSSGLayerGlobalRenderProperties QSSGCustomMaterialSystem::getLayerGlobalRenderProperties(QSSGCustomMaterialRenderContext &inRenderContext)
{
    const QSSGRenderLayer &theLayer = inRenderContext.layer;
    const QSSGLayerRenderData &theData = inRenderContext.layerData;

    const bool isYUpInFramebuffer = context->rhiContext()->isValid()
            ? context->rhiContext()->rhi()->isYUpInFramebuffer()
            : true;
    const bool isClipDepthZeroToOne = context->rhiContext()->isValid()
            ? context->rhiContext()->rhi()->isClipDepthZeroToOne()
            : true;

    return QSSGLayerGlobalRenderProperties{ theLayer,
                const_cast<QSSGRenderCamera &>(inRenderContext.camera),
                theData.cameraDirection,
                theData.shadowMapManager,
                inRenderContext.rhiDepthTexture,
                inRenderContext.rhiAoTexture,
                inRenderContext.rhiScreenTexture,
                theLayer.lightProbe,
                theLayer.probeHorizon,
                theLayer.probeExposure,
                theLayer.probeOrientation,
                isYUpInFramebuffer,
                isClipDepthZeroToOne
    };
}

bool QSSGCustomMaterialSystem::prepareForRender(const QSSGRenderModel &,
                                          const QSSGRenderSubset &,
                                          QSSGRenderCustomMaterial &inMaterial)
{
    return inMaterial.isDirty();
}

void QSSGCustomMaterialSystem::setRenderContextInterface(QSSGRenderContextInterface *inContext)
{
    context = inContext;
}

QSSGRef<QSSGRhiShaderStagesWithResources> QSSGCustomMaterialSystem::shadersForCustomMaterial(QSSGRhiContext *rhiCtx,
                                                                                             QSSGRhiGraphicsPipelineState *ps,
                                                                                             const QSSGRenderCustomMaterial &material,
                                                                                             QSSGCustomMaterialRenderable &renderable,
                                                                                             const ShaderFeatureSetList &featureSet,
                                                                                             QSSGLayerRenderData &layerData,
                                                                                             QSSGRenderCamera &camera,
                                                                                             const QVector2D &depthAdjust,
                                                                                             const QMatrix4x4 *alteredModelViewProjection)
{
    const QMatrix4x4 &mvp(alteredModelViewProjection ? *alteredModelViewProjection
                                                     : renderable.modelContext.modelViewProjection);

    QSSGCustomMaterialRenderContext customMaterialContext(layerData.layer,
                                                          layerData,
                                                          renderable.lights,
                                                          camera,
                                                          renderable.modelContext.model,
                                                          renderable.subset,
                                                          mvp,
                                                          renderable.globalTransform,
                                                          renderable.modelContext.normalMatrix,
                                                          material,
                                                          layerData.m_rhiDepthTexture.texture,
                                                          layerData.m_rhiAoTexture.texture,
                                                          layerData.m_rhiScreenTexture.texture,
                                                          renderable.shaderDescription,
                                                          renderable.firstImage,
                                                          renderable.opacity);

    // This just references inFeatureSet and inRenderable.shaderDescription -
    // cheap to construct and is good enough for the find()
    QSSGShaderMapKey skey = QSSGShaderMapKey(material.m_shaderPathKey,
                                             featureSet,
                                             customMaterialContext.materialKey);

    QSSGRef<QSSGRhiShaderStagesWithResources> shaderPipeline;
    auto it = shaderMap.find(skey);
    if (it == shaderMap.end()) {
        // ### FIXME: this is null bones.
        // It should be replaced with custom material's boneTransforms
        QSSGDataView<QMatrix4x4> boneGlobals;
        QSSGDataView<QMatrix3x3> boneNormals;

        QSSGMaterialVertexPipeline pipeline(context->shaderProgramGenerator(),
                                            context->renderer()->defaultMaterialShaderKeyProperties(),
                                            material.adapter,
                                            boneGlobals,
                                            boneNormals);

        QSSGRef<QSSGRhiShaderStages> shaderStages = QSSGMaterialShaderGenerator::generateMaterialRhiShader(material.m_shaderPathKey,
                                                                                                           pipeline,
                                                                                                           renderable.shaderDescription,
                                                                                                           context->renderer()->defaultMaterialShaderKeyProperties(),
                                                                                                           featureSet,
                                                                                                           renderable.material,
                                                                                                           customMaterialContext.lights,
                                                                                                           customMaterialContext.firstImage,
                                                                                                           context->shaderLibraryManager(),
                                                                                                           context->shaderCache());
        if (shaderStages)
            shaderPipeline = QSSGRhiShaderStagesWithResources::fromShaderStages(shaderStages);
        // make skey useable as a key for the QHash (makes copies of materialKey and featureSet, instead of just referencing)
        skey.detach();
        // insert it no matter what, no point in trying over and over again
        shaderMap.insert(skey, shaderPipeline);
    } else {
        shaderPipeline = it.value();
    }

    if (shaderPipeline) {
        ps->shaderStages = shaderPipeline->stages();

        shaderPipeline->beginMainUniformBuffer();
        shaderPipeline->resetExtraTextures();

        const QMatrix4x4 clipSpaceCorrMatrix = rhiCtx->rhi()->clipSpaceCorrMatrix();

        // ### FIXME: this is null bones.
        // It should be replaced with custom material's boneTransforms
        QSSGDataView<QMatrix4x4> boneGlobals;
        QSSGDataView<QMatrix3x3> boneNormals;

        QSSGMaterialShaderGenerator::setRhiMaterialProperties(*context,
                                                              shaderPipeline,
                                                              ps,
                                                              material,
                                                              renderable.shaderDescription,
                                                              context->renderer()->defaultMaterialShaderKeyProperties(),
                                                              customMaterialContext.camera,
                                                              customMaterialContext.modelViewProjection,
                                                              customMaterialContext.normalMatrix,
                                                              customMaterialContext.modelMatrix,
                                                              clipSpaceCorrMatrix,
                                                              boneGlobals,
                                                              boneNormals,
                                                              customMaterialContext.firstImage,
                                                              customMaterialContext.opacity,
                                                              getLayerGlobalRenderProperties(customMaterialContext),
                                                              customMaterialContext.lights,
                                                              true,
                                                              depthAdjust);
    }

    return shaderPipeline;
}

static const QRhiShaderResourceBinding::StageFlags VISIBILITY_ALL =
        QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage;

void QSSGCustomMaterialSystem::rhiPrepareRenderable(QSSGRhiGraphicsPipelineState *ps,
                                                    QSSGCustomMaterialRenderable &renderable,
                                                    const ShaderFeatureSetList &featureSet,
                                                    const QSSGRenderCustomMaterial &material,
                                                    QSSGLayerRenderData &layerData,
                                                    QRhiRenderPassDescriptor *renderPassDescriptor,
                                                    int samples)
{
    QSSGRhiContext *rhiCtx = context->rhiContext().data();

    QRhiGraphicsPipeline::TargetBlend blend; // no blending by default
    if (material.m_renderFlags.testFlag(QSSGRenderCustomMaterial::RenderFlag::Blending)) {
        blend.enable = true;
        blend.srcColor = material.m_srcBlend;
        blend.srcAlpha = material.m_srcBlend;
        blend.dstColor = material.m_dstBlend;
        blend.dstAlpha = material.m_dstBlend;
    }

    const QSSGCullFaceMode cullMode = material.m_cullMode;

    QSSGRef<QSSGRhiShaderStagesWithResources> shaderPipeline = shadersForCustomMaterial(rhiCtx,
                                                                                        ps,
                                                                                        material,
                                                                                        renderable,
                                                                                        featureSet,
                                                                                        layerData,
                                                                                        *layerData.camera,
                                                                                        QVector2D(),
                                                                                        nullptr);

    if (shaderPipeline) {
        QRhiCommandBuffer *cb = rhiCtx->commandBuffer();

        ps->samples = samples;

        ps->cullMode = QSSGRhiGraphicsPipelineState::toCullMode(cullMode);

        ps->targetBlend = blend;

        ps->ia = renderable.subset.rhi.ia;
        ps->ia.bakeVertexInputLocations(*shaderPipeline);

        QRhiResourceUpdateBatch *resourceUpdates = rhiCtx->rhi()->nextResourceUpdateBatch();
        QSSGRhiUniformBufferSet &uniformBuffers(rhiCtx->uniformBufferSet({ &layerData.layer,
                                                                           &renderable.modelContext.model,
                                                                           &material,
                                                                           0,
                                                                           QSSGRhiUniformBufferSetKey::Main }));
        shaderPipeline->bakeMainUniformBuffer(&uniformBuffers.ubuf, resourceUpdates);
        shaderPipeline->bakeLightsUniformBuffer(QSSGRhiShaderStagesWithResources::LightBuffer0,
                                                &uniformBuffers.lightsUbuf0,
                                                resourceUpdates);

        QRhiTexture *dummyTexture = rhiCtx->dummyTexture({}, resourceUpdates);
        QRhiTexture *dummyCubeTexture = rhiCtx->dummyTexture(QRhiTexture::CubeMap, resourceUpdates);

        cb->resourceUpdate(resourceUpdates);

        QSSGRhiContext::ShaderResourceBindingList bindings;

        bindings.append(QRhiShaderResourceBinding::uniformBuffer(0, VISIBILITY_ALL, uniformBuffers.ubuf));

        bindings.append(QRhiShaderResourceBinding::uniformBuffer(1, VISIBILITY_ALL, uniformBuffers.lightsUbuf0));

        QVector<QShaderDescription::InOutVariable> samplerVars =
                shaderPipeline->stages()->fragmentStage()->shader().description().combinedImageSamplers();;
        for (const QShaderDescription::InOutVariable &var : shaderPipeline->stages()->vertexStage()->shader().description().combinedImageSamplers()) {
            auto it = std::find_if(samplerVars.cbegin(), samplerVars.cend(),
                                   [&var](const QShaderDescription::InOutVariable &v) { return var.binding == v.binding; });
            if (it == samplerVars.cend())
                samplerVars.append(var);
        }

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
            int binding = shaderPipeline->bindingForTexture("qt_lightProbe", int(QSSGRhiSamplerBindingHints::LightProbe));
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

        if (shaderPipeline->screenTexture()) {
            int binding = shaderPipeline->bindingForTexture("qt_screenTexture", int(QSSGRhiSamplerBindingHints::ScreenTexture));
            if (binding >= 0) {
                samplerBindingsSpecified.setBit(binding);
                // linear min/mag, mipmap filtering depends on the
                // texture, with SCREEN_TEXTURE there are no mipmaps, but
                // once SCREEN_MIP_TEXTURE is seen the texture (the same
                // one) has mipmaps generated.
                QRhiSampler::Filter mipFilter = shaderPipeline->screenTexture()->flags().testFlag(QRhiTexture::MipMapped)
                        ? QRhiSampler::Linear : QRhiSampler::None;
                QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Linear, QRhiSampler::Linear, mipFilter,
                                                         QRhiSampler::Repeat, QRhiSampler::Repeat });
                bindings.append(QRhiShaderResourceBinding::sampledTexture(binding,
                                                                          QRhiShaderResourceBinding::FragmentStage,
                                                                          shaderPipeline->screenTexture(), sampler));
            } // else ignore, not an error
        }

        if (shaderPipeline->depthTexture()) {
            int binding = shaderPipeline->bindingForTexture("qt_depthTexture", int(QSSGRhiSamplerBindingHints::DepthTexture));
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
            int binding = shaderPipeline->bindingForTexture("qt_aoTexture", int(QSSGRhiSamplerBindingHints::AoTexture));
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

        const int shadowMapCount = shaderPipeline->shadowMapCount();
        for (int i = 0; i < shadowMapCount; ++i) {
            QSSGRhiShadowMapProperties &shadowMapProperties(shaderPipeline->shadowMapAt(i));
            QRhiTexture *texture = shadowMapProperties.shadowMapTexture;
            QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                                     QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge });
            const QByteArray &name(shadowMapProperties.shadowMapTextureUniformName);
            if (shadowMapProperties.cachedBinding < 0)
                shadowMapProperties.cachedBinding = shaderPipeline->bindingForTexture(name);
            if (shadowMapProperties.cachedBinding < 0) // may not be used in the shader with unshaded custom materials, that's normal
                continue;
            samplerBindingsSpecified.setBit(shadowMapProperties.cachedBinding);
            bindings.append(QRhiShaderResourceBinding::sampledTexture(shadowMapProperties.cachedBinding,
                                                                      QRhiShaderResourceBinding::FragmentStage,
                                                                      texture,
                                                                      sampler));
        }

        QSSGRenderableImage *renderableImage = renderable.firstImage;
        while (renderableImage) {
            const char *samplerName = QSSGMaterialShaderGenerator::getSamplerName(renderableImage->m_mapType);
            const int samplerHint = int(renderableImage->m_mapType);
            int samplerBinding = shaderPipeline->bindingForTexture(samplerName, samplerHint);
            if (samplerBinding >= 0) {
                QRhiTexture *texture = renderableImage->m_image.m_textureData.m_rhiTexture;
                if (samplerBinding >= 0 && texture) {
                    const bool mipmapped = texture->flags().testFlag(QRhiTexture::MipMapped);
                    QRhiSampler *sampler = rhiCtx->sampler({ toRhi(renderableImage->m_image.m_minFilterType),
                                                             toRhi(renderableImage->m_image.m_magFilterType),
                                                             mipmapped ? toRhi(renderableImage->m_image.m_mipFilterType) : QRhiSampler::None,
                                                             toRhi(renderableImage->m_image.m_horizontalTilingMode),
                                                             toRhi(renderableImage->m_image.m_verticalTilingMode) });
                    samplerBindingsSpecified.setBit(samplerBinding);
                    bindings.append(QRhiShaderResourceBinding::sampledTexture(samplerBinding,
                                                                              VISIBILITY_ALL,
                                                                              texture, sampler));
                }
            } // else this is not necessarily an error, e.g. having metalness/roughness maps with metalness disabled
            renderableImage = renderableImage->m_nextImage;
        }

        if (maxSamplerBinding >= 0) {
            // custom property textures
            int customTexCount = shaderPipeline->extraTextureCount();
            for (int i = 0; i < customTexCount; ++i) {
                const QSSGRhiTexture &t(shaderPipeline->extraTextureAt(i));
                const int samplerBinding = shaderPipeline->bindingForTexture(t.name);
                if (samplerBinding >= 0) {
                    samplerBindingsSpecified.setBit(samplerBinding);
                    QRhiSampler *sampler = rhiCtx->sampler(t.samplerDesc);
                    bindings.append(QRhiShaderResourceBinding::sampledTexture(samplerBinding,
                                                                              VISIBILITY_ALL,
                                                                              t.texture,
                                                                              sampler));
                }
            }

            // use a dummy texture for the unused samplers in the shader
            QVarLengthArray<QRhiShaderResourceBinding::TextureAndSampler, 16> texSamplers;
            QRhiSampler *dummySampler = rhiCtx->sampler({ QRhiSampler::Nearest, QRhiSampler::Nearest, QRhiSampler::None,
                                                          QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge });

            for (const QShaderDescription::InOutVariable &var : samplerVars) {
                if (!samplerBindingsSpecified.testBit(var.binding)) {
                    QRhiTexture *t = var.type == QShaderDescription::SamplerCube ? dummyCubeTexture : dummyTexture;
                    texSamplers.clear();
                    const int count = var.arrayDims.isEmpty() ? 1 : var.arrayDims.first();
                    for (int i = 0; i < count; ++i)
                        texSamplers.append({ t, dummySampler });
                    bindings.append(QRhiShaderResourceBinding::sampledTextures(var.binding,
                                                                               VISIBILITY_ALL,
                                                                               texSamplers.count(),
                                                                               texSamplers.constData()));
                }
            }
        }

        QRhiShaderResourceBindings *srb = rhiCtx->srb(bindings);

        const QSSGGraphicsPipelineStateKey pipelineKey { *ps, renderPassDescriptor, srb };
        renderable.rhiRenderData.mainPass.pipeline = rhiCtx->pipeline(pipelineKey);
        renderable.rhiRenderData.mainPass.srb = srb;
    }
}

void QSSGCustomMaterialSystem::setShaderResources(const QSSGRenderCustomMaterial &inMaterial,
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
            const QSSGRef<QSSGBufferManager> &theBufferManager(context->bufferManager());

            QSSGBufferManager::MipMode mipMode = QSSGBufferManager::MipModeNone;
            // the mipFilterType here is only non-None when generateMipmaps was true on the Texture
            if (textureProperty->mipFilterType != QSSGRenderTextureFilterOp::None)
                mipMode = QSSGBufferManager::MipModeGenerated;
            // ### would we want MipModeBsdf in some cases?

            QSSGRenderImageTextureData theTextureData = theBufferManager->loadRenderImage(image, false, mipMode);
            if (theTextureData.m_rhiTexture) {
                const QSSGRhiTexture t = {
                    inPropertyName,
                    theTextureData.m_rhiTexture,
                    { toRhi(textureProperty->minFilterType),
                      toRhi(textureProperty->magFilterType),
                      textureProperty->mipFilterType != QSSGRenderTextureFilterOp::None ? toRhi(textureProperty->mipFilterType) : QRhiSampler::None,
                      toRhi(textureProperty->clampType),
                      toRhi(textureProperty->clampType) }
                };
                shaderPipeline->addExtraTexture(t);
            }
            image->m_textureData = theTextureData;
        }
    } else {
        shaderPipeline->setUniformValue(inPropertyName, propertyValue, inPropertyType);
    }
}

void QSSGCustomMaterialSystem::applyRhiShaderPropertyValues(const QSSGRenderCustomMaterial &material,
                                                            const QSSGRef<QSSGRhiShaderStagesWithResources> &shaderPipeline)
{
    const auto &properties = material.m_properties;
    for (const auto &prop : properties)
        setShaderResources(material, prop.name, prop.value, prop.shaderDataType, shaderPipeline);

    const auto textProps = material.m_textureProperties;
    for (const auto &prop : textProps)
        setShaderResources(material, prop.name, QVariant::fromValue((void *)&prop), prop.shaderDataType, shaderPipeline);
}

void QSSGCustomMaterialSystem::rhiRenderRenderable(QSSGRhiContext *rhiCtx,
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

QT_END_NAMESPACE
