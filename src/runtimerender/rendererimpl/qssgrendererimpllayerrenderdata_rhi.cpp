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

#include <QtQuick3DRuntimeRender/private/qssgrendererimpl_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderlight_p.h>

QT_BEGIN_NAMESPACE

static const QRhiShaderResourceBinding::StageFlags VISIBILITY_ALL =
        QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage;

static void fillTargetBlend(QRhiGraphicsPipeline::TargetBlend *targetBlend, QSSGRenderDefaultMaterial::MaterialBlendMode materialBlend)
{
    // Assuming default values in the other TargetBlend fields
    switch (materialBlend) {
    case QSSGRenderDefaultMaterial::MaterialBlendMode::Screen:
        targetBlend->srcColor = QRhiGraphicsPipeline::SrcAlpha;
        targetBlend->dstColor = QRhiGraphicsPipeline::One;
        targetBlend->srcAlpha = QRhiGraphicsPipeline::One;
        targetBlend->dstAlpha = QRhiGraphicsPipeline::One;
        break;
    case QSSGRenderDefaultMaterial::MaterialBlendMode::Multiply:
        targetBlend->srcColor = QRhiGraphicsPipeline::DstColor;
        targetBlend->dstColor = QRhiGraphicsPipeline::Zero;
        targetBlend->srcAlpha = QRhiGraphicsPipeline::One;
        targetBlend->dstAlpha = QRhiGraphicsPipeline::One;
        break;
    default:
        // Use SourceOver for everything else
        targetBlend->srcColor = QRhiGraphicsPipeline::SrcAlpha;
        targetBlend->dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
        targetBlend->srcAlpha = QRhiGraphicsPipeline::One;
        targetBlend->dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;
        break;
    }
}

static void rhiPrepareRenderable(QSSGRhiContext *rhiCtx,
                                 QSSGLayerRenderData &inData,
                                 QSSGRenderableObject &inObject,
                                 const QVector2D &inCameraProps,
                                 const ShaderFeatureSetList &inFeatureSet,
                                 quint32 indexLight,
                                 const QSSGRenderCamera &inCamera)
{
    Q_UNUSED(indexLight);

    QSSGRhiGraphicsPipelineState *ps = rhiCtx->graphicsPipelineState(&inData);
    QRhiCommandBuffer *cb = rhiCtx->commandBuffer();

    if (inObject.renderableFlags.isDefaultMaterialMeshSubset()) {
        QSSGSubsetRenderable &subsetRenderable(static_cast<QSSGSubsetRenderable &>(inObject));
        const QSSGRef<QSSGRendererImpl> &generator(subsetRenderable.generator);

        QSSGRef<QSSGRhiShaderStagesWithResources> shaderPipeline = generator->getRhiShadersWithResources(subsetRenderable, inFeatureSet);
        if (shaderPipeline) {
            ps->shaderStages = shaderPipeline->stages();
            const QSSGRef<QSSGDefaultMaterialShaderGeneratorInterface> &defMatGen
                    = generator->contextInterface()->defaultMaterialShaderGenerator();
            defMatGen->setRhiMaterialProperties(shaderPipeline,
                                                ps,
                                                subsetRenderable.material,
                                                inCameraProps,
                                                subsetRenderable.modelContext.modelViewProjection,
                                                subsetRenderable.modelContext.normalMatrix,
                                                subsetRenderable.modelContext.model.globalTransform,
                                                subsetRenderable.firstImage,
                                                subsetRenderable.opacity,
                                                generator->getLayerGlobalRenderProperties(),
                                                subsetRenderable.renderableFlags.receivesShadows());

            // shaderPipeline->dumpUniforms();

            ps->cullMode = QSSGRhiGraphicsPipelineState::toCullMode(subsetRenderable.material.cullingMode);
            fillTargetBlend(&ps->targetBlend, subsetRenderable.material.blendMode);

            ps->ia = subsetRenderable.subset.rhi.ia;
            ps->ia.bakeVertexInputLocations(*shaderPipeline);

            QRhiResourceUpdateBatch *resourceUpdates;
            if (subsetRenderable.subset.rhi.bufferResourceUpdates) {
                resourceUpdates = subsetRenderable.subset.rhi.bufferResourceUpdates;
                subsetRenderable.subset.rhi.bufferResourceUpdates = nullptr;
            } else {
                resourceUpdates = rhiCtx->rhi()->nextResourceUpdateBatch();
            }

            // Unlike the subsetRenderable (which is allocated per frame so is
            // not persistent in any way), the model reference is persistent in
            // the sense that it references the model node in the scene graph.
            // Combined with the layer node (multiple View3Ds may share the
            // same scene!), this is suitable as a key to get the uniform
            // buffers that were used with the rendering of the same model in
            // the previous frame.
            const void *layerNode = &inData.layer;
            const void *modelNode = &subsetRenderable.modelContext.model;

            QSSGRhiUniformBufferSet &uniformBuffers(rhiCtx->uniformBufferSet({ layerNode, modelNode, nullptr }));
            shaderPipeline->bakeMainUniformBuffer(&uniformBuffers.ubuf, resourceUpdates);
            QRhiBuffer *ubuf = uniformBuffers.ubuf;

            QRhiBuffer *lightsUbuf = nullptr;
            if (shaderPipeline->isLightingEnabled()) {
                shaderPipeline->bakeLightsUniformBuffer(&uniformBuffers.lightsUbuf, resourceUpdates);
                lightsUbuf = uniformBuffers.lightsUbuf;
            }

            cb->resourceUpdate(resourceUpdates);

            QSSGRhiContext::ShaderResourceBindingList bindings;
            bindings.append(QRhiShaderResourceBinding::uniformBuffer(0, VISIBILITY_ALL, ubuf));

            if (lightsUbuf)
                bindings.append(QRhiShaderResourceBinding::uniformBuffer(1, VISIBILITY_ALL, lightsUbuf));

            // Texture maps
            auto *renderableImage = subsetRenderable.firstImage;
            if (renderableImage) {
                int imageNumber = 0;
                auto fragmentShader = ps->shaderStages->fragmentStage()->shader();
                Q_ASSERT(fragmentShader.isValid());
                auto desc =  fragmentShader.description();
                auto samplerVariables = desc.combinedImageSamplers();

                while (renderableImage) {
                    int samplerBinding = 0;

                    // TODO: optimize this! We're looking for the sampler corresponding to imageNumber, and currently the
                    // only information we have is the name of the form "image0_sampler"
                    QString samplerName = QStringLiteral("image%1_sampler").arg(imageNumber);
                    auto found = std::find_if(samplerVariables.cbegin(), samplerVariables.cend(),
                                              [&samplerName](const QShaderDescription::InOutVariable &s){ return s.name == samplerName; });
                    if (found != samplerVariables.cend())
                        samplerBinding = found->binding;
                    else
                        qWarning("Could not find sampler for image");

                    auto *rhiTex = renderableImage->m_image.m_textureData.m_rhiTexture;
                    if (samplerBinding >= 0 && rhiTex) {
                        auto *sampler = rhiCtx->sampler({renderableImage->m_image.m_horizontalTilingMode,
                                                         renderableImage->m_image.m_verticalTilingMode,
                                                        false});
                        bindings.append(QRhiShaderResourceBinding::sampledTexture(samplerBinding,QRhiShaderResourceBinding::FragmentStage, rhiTex, sampler));
                    }

                    renderableImage = renderableImage->m_nextImage;
                    imageNumber++;
                }
            }

            // Shadow map textures
            if (shaderPipeline->isLightingEnabled()) {
                const QVector<QShaderDescription::InOutVariable> imageSamplerVars =
                    ps->shaderStages->fragmentStage()->shader().description().combinedImageSamplers();
                const int shadowMapCount = shaderPipeline->shadowMapCount();
                for (int i = 0; i < shadowMapCount; ++i) {
                    QSSGRhiShadowMapProperties &shadowMapProperties(shaderPipeline->shadowMapAt(i));
                    QRhiTexture *texture = shadowMapProperties.shadowMapTexture;
                    QRhiSampler *sampler = rhiCtx->sampler({ QSSGRenderTextureCoordOp::ClampToEdge, QSSGRenderTextureCoordOp::ClampToEdge, false });
                    Q_ASSERT(texture && sampler);
                    const QByteArray name = shadowMapProperties.shadowMapTextureUniformName;

                    // ### this needs something more clever because the shadow
                    // map list on shaderPipeline is in fact regenerated on every
                    // setRhiMaterialProperties, meaning we will do the loop again and again
                    if (shadowMapProperties.cachedBinding < 0) {
                        for (const QShaderDescription::InOutVariable &var : imageSamplerVars) {
                            if (var.name.toLatin1() == name) {
                                shadowMapProperties.cachedBinding = var.binding;
                                break;
                            }
                        }
                    }
                    if (shadowMapProperties.cachedBinding < 0) {
                        qWarning("No combined image sampler for shadow map texture '%s'", name.constData());
                        continue;
                    }
                    bindings.append(QRhiShaderResourceBinding::sampledTexture(shadowMapProperties.cachedBinding,
                                                                              QRhiShaderResourceBinding::FragmentStage,
                                                                              texture,
                                                                              sampler));
                }
            }

            if (shaderPipeline->lightProbeTexture()) {
                //### TODO: reduce code duplication later

                static int debugCount;
                const bool doDebug = (debugCount++) < 5 && false;

                if (doDebug)
                    qDebug() << "____________ LIGHTPROBE___________________";

                auto fragmentShader = ps->shaderStages->fragmentStage()->shader();
                Q_ASSERT(fragmentShader.isValid());
                auto desc =  fragmentShader.description();
                auto samplerVariables = desc.combinedImageSamplers();
                const QString samplerName = QStringLiteral("lightProbe");
                int samplerBinding = -1;
                auto found = std::find_if(samplerVariables.cbegin(), samplerVariables.cend(), [&samplerName](const QShaderDescription::InOutVariable &s){ return s.name == samplerName; });
                if (found != samplerVariables.cend())
                    samplerBinding = found->binding;
                else
                    qWarning("Could not find sampler for lightprobe");
                auto *rhiTex = shaderPipeline->lightProbeTexture();
                if (samplerBinding >= 0 && rhiTex) {
                    auto *sampler = rhiCtx->sampler({ QSSGRenderTextureCoordOp::ClampToEdge,
                                                      QSSGRenderTextureCoordOp::ClampToEdge,
                                                      true
                                                    });
                    if (doDebug)
                        qDebug() << "binding lightprobe texture" << samplerBinding << rhiTex << "sampler" << sampler;
                    bindings.append(QRhiShaderResourceBinding::sampledTexture(samplerBinding,QRhiShaderResourceBinding::FragmentStage, rhiTex, sampler));
                }

            }

            QRhiShaderResourceBindings *srb = rhiCtx->srb(bindings);

            const QSSGGraphicsPipelineStateKey pipelineKey { *ps, rhiCtx->mainRenderPassDesciptor(), srb };
            subsetRenderable.rhiRenderData.pipeline = rhiCtx->pipeline(pipelineKey);
            subsetRenderable.rhiRenderData.srb = srb;

        }
    } else if (inObject.renderableFlags.isCustomMaterialMeshSubset()) {
        // ###
        Q_UNUSED(inData);
        Q_UNUSED(inCamera);
    } else {
        Q_ASSERT(false);
    }
}

namespace RendererImpl {
extern void setupCameraForShadowMap(const QRectF &inViewport,
                                    const QSSGRenderCamera &inCamera,
                                    const QSSGRenderLight *inLight,
                                    QSSGRenderCamera &theCamera);

extern void setupCubeShadowCameras(const QSSGRenderLight *inLight, QSSGRenderCamera inCameras[6]);
}

static const int ORTHO_SHADOW_UBUF_ENTRY_SIZE = 64 + 8;
static const int CUBE_SHADOW_UBUF_ENTRY_SIZE = 64 + 64 + 16 + 8;

static void rhiPrepareResourcesForShadowMap(QSSGRhiContext *rhiCtx,
                                            QSSGLayerRenderData &inData,
                                            QSSGShadowMapEntry *pEntry,
                                            float depthAdjust[2],
                                            const QVector<QSSGRenderableObjectHandle> &sortedOpaqueObjects,
                                            const QSSGRenderCamera &inCamera,
                                            bool orthographic,
                                            int cubeFace)
{
    QRhi *rhi = rhiCtx->rhi();
    QRhiCommandBuffer *cb = rhiCtx->commandBuffer();
    const QMatrix4x4 correctionMatrix = rhi->clipSpaceCorrMatrix();

    // For the orthographic case (just one 2D texture) a single set of uniform
    // values is enough, but the cubemap approach runs 6 renderpasses, one for
    // each face, and it's good if we can use the same uniform buffer, just
    // with a different dynamic offset. Hence 6 times the size in that case.
    const int UBUF_ENTRY_SIZE = orthographic ? ORTHO_SHADOW_UBUF_ENTRY_SIZE : CUBE_SHADOW_UBUF_ENTRY_SIZE;
    const int UBUF_SIZE = orthographic ? UBUF_ENTRY_SIZE : 6 * rhi->ubufAligned(UBUF_ENTRY_SIZE);
    const int ubufBaseOffset = cubeFace * rhi->ubufAligned(UBUF_ENTRY_SIZE);

    for (const auto &handle : sortedOpaqueObjects) {
        QSSGRenderableObject *theObject = handle.obj;
        if (!theObject->renderableFlags.castsShadows())
            continue;

        QRhiResourceUpdateBatch *rub = rhi->nextResourceUpdateBatch();
        if (theObject->renderableFlags.isDefaultMaterialMeshSubset() || theObject->renderableFlags.isCustomMaterialMeshSubset()) {
            QSSGSubsetRenderableBase *subsetRenderable(static_cast<QSSGSubsetRenderableBase *>(theObject));
            // if there are still pending updates on the mesh, take those too
            if (subsetRenderable->subset.rhi.bufferResourceUpdates) {
                rub->merge(subsetRenderable->subset.rhi.bufferResourceUpdates);
                subsetRenderable->subset.rhi.bufferResourceUpdates = nullptr;
            }

            const QSSGRhiUniformBufferSetKey ubufKey = { &inData.layer, &subsetRenderable->modelContext.model, pEntry };
            QSSGRhiUniformBufferSet &uniformBuffers(rhiCtx->uniformBufferSet(ubufKey));
            QRhiBuffer *&ubuf = uniformBuffers.ubuf;
            if (!ubuf) {
                ubuf = rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, UBUF_SIZE);
                ubuf->build();
            } else if (ubuf->size() < UBUF_SIZE) {
                ubuf->setSize(UBUF_SIZE);
                ubuf->build();
            }

            const QMatrix4x4 modelViewProjection = correctionMatrix * pEntry->m_lightVP * subsetRenderable->globalTransform;
            // mat4 modelViewProjection
            rub->updateDynamicBuffer(ubuf, ubufBaseOffset, 64, modelViewProjection.constData());

            if (orthographic) {
                // vec2 depthAdjust
                rub->updateDynamicBuffer(ubuf, ubufBaseOffset + 64, 8, depthAdjust);
            } else {
                // mat4 modelMatrix
                rub->updateDynamicBuffer(ubuf, ubufBaseOffset + 64, 64, subsetRenderable->globalTransform.constData());
                // vec3 cameraPosition
                rub->updateDynamicBuffer(ubuf, ubufBaseOffset + 128, 12, &inCamera.position);
                // vec2 cameraProperties
                float cameraProps[] = { inCamera.clipNear, inCamera.clipFar };
                rub->updateDynamicBuffer(ubuf, ubufBaseOffset + 144, 8, cameraProps); // vec2 is aligned to 8, hence the 144
            }
        }
        cb->resourceUpdate(rub);
    }

}

static void rhiRenderOneShadowMap(QSSGRhiContext *rhiCtx,
                                  QSSGLayerRenderData &inData,
                                  QSSGShadowMapEntry *pEntry,
                                  QSSGRhiGraphicsPipelineState *ps,
                                  const QVector<QSSGRenderableObjectHandle> &sortedOpaqueObjects,
                                  bool orthographic,
                                  int cubeFace)
{
    QRhiCommandBuffer *cb = rhiCtx->commandBuffer();
    const int UBUF_ENTRY_SIZE = orthographic ? ORTHO_SHADOW_UBUF_ENTRY_SIZE : CUBE_SHADOW_UBUF_ENTRY_SIZE;
    const int ubufBaseOffset = cubeFace * rhiCtx->rhi()->ubufAligned(UBUF_ENTRY_SIZE);
    bool needsSetViewport = true;

    for (const auto &handle : sortedOpaqueObjects) {
        QSSGRenderableObject *theObject = handle.obj;
        if (!theObject->renderableFlags.castsShadows())
            continue;

        if (theObject->renderableFlags.isDefaultMaterialMeshSubset() || theObject->renderableFlags.isCustomMaterialMeshSubset()) {
            QSSGSubsetRenderableBase *subsetRenderable(static_cast<QSSGSubsetRenderableBase *>(theObject));
            QSSGRef<QSSGRhiShaderStagesWithResources> shaderStages = orthographic
                    ? subsetRenderable->generator->getRhiOrthographicShadowDepthShader()
                    : subsetRenderable->generator->getRhiCubemapShadowDepthShader();
            if (!shaderStages)
                continue;

            ps->shaderStages = shaderStages->stages();
            ps->ia = subsetRenderable->subset.rhi.iaDepth;

            QRhiBuffer *vertexBuffer = subsetRenderable->subset.rhi.iaDepth.vertexBuffer->buffer();
            QRhiBuffer *indexBuffer = subsetRenderable->subset.rhi.iaDepth.indexBuffer
                    ? subsetRenderable->subset.rhi.iaDepth.indexBuffer->buffer()
                    : nullptr;

            const QSSGRhiUniformBufferSetKey ubufKey = { &inData.layer, &subsetRenderable->modelContext.model, pEntry };
            QRhiBuffer *ubuf = rhiCtx->uniformBufferSet(ubufKey).ubuf;

            QSSGRhiContext::ShaderResourceBindingList bindings;
            if (orthographic) {
                bindings.append(QRhiShaderResourceBinding::uniformBuffer(0, VISIBILITY_ALL, ubuf));
            } else {
                bindings.append(QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(
                                    0, VISIBILITY_ALL, ubuf, CUBE_SHADOW_UBUF_ENTRY_SIZE));
            }

            QRhiShaderResourceBindings *srb = rhiCtx->srb(bindings);
            const QSSGGraphicsPipelineStateKey pipelineKey { *ps, pEntry->m_rhiRenderPassDesc, srb };
            cb->setGraphicsPipeline(rhiCtx->pipeline(pipelineKey));
            if (orthographic) {
                cb->setShaderResources(srb);
            } else {
                const QRhiCommandBuffer::DynamicOffset dynOfs = { 0, quint32(ubufBaseOffset) };
                cb->setShaderResources(srb, 1, &dynOfs);
            }

            if (needsSetViewport) {
                cb->setViewport(ps->viewport);
                needsSetViewport = false;
            }

            QRhiCommandBuffer::VertexInput vb(vertexBuffer, 0);
            if (indexBuffer) {
                cb->setVertexInput(0, 1, &vb, indexBuffer, 0, subsetRenderable->subset.rhi.iaDepth.indexBuffer->indexFormat());
                cb->drawIndexed(subsetRenderable->subset.count, 1, subsetRenderable->subset.offset);
            } else {
                cb->setVertexInput(0, 1, &vb);
                cb->draw(subsetRenderable->subset.count, 1, subsetRenderable->subset.offset);
            }
        }
    }
}

static void rhiBlurShadowMap(QSSGRhiContext *rhiCtx,
                             QSSGShadowMapEntry *pEntry,
                             const QSSGRef<QSSGRendererImpl> &renderer,
                             float shadowFilter,
                             float shadowMapFar,
                             bool orthographic)
{
    // may not be able to do the blur pass if the number of max color
    // attachments is the gl/vk spec mandated minimum of 4, and we need 6.
    // (applicable only to !orthographic, whereas orthographic always works)
    if (!pEntry->m_rhiBlurRenderTarget0 || !pEntry->m_rhiBlurRenderTarget1)
        return;

    QRhi *rhi = rhiCtx->rhi();
    QSSGRhiGraphicsPipelineState ps;
    QRhiTexture *map = orthographic ? pEntry->m_rhiDepthMap : pEntry->m_rhiDepthCube;
    QRhiTexture *workMap = orthographic ? pEntry->m_rhiDepthCopy : pEntry->m_rhiCubeCopy;
    const QSize size = map->pixelSize();
    ps.viewport = QRhiViewport(0, 0, float(size.width()), float(size.height()));

    QSSGRef<QSSGRhiShaderStagesWithResources> shaderPipeline = orthographic ? renderer->getRhiOrthographicShadowBlurXShader()
                                                                            : renderer->getRhiCubemapShadowBlurXShader();
    if (!shaderPipeline)
        return;
    ps.shaderStages = shaderPipeline->stages();

    ps.colorAttachmentCount = orthographic ? 1 : 6;

    // construct a key that is unique for this frame (we use a dynamic buffer
    // so even if the same key gets used in the next frame, just doing
    // updateDynamicBuffer() on the same QRhiBuffer is ok due to QRhi's
    // internal double buffering)
    QSSGRhiUniformBufferSetKey ubufKey = {
        map,
        reinterpret_cast<const void *>(quintptr(1)), // blur X
        nullptr
    };

    QSSGRhiUniformBufferSet &ubufContainer = rhiCtx->uniformBufferSet(ubufKey);
    if (!ubufContainer.ubuf) {
        ubufContainer.ubuf = rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64 + 8);
        ubufContainer.ubuf->build();
    }
    QRhiResourceUpdateBatch *rub = rhi->nextResourceUpdateBatch();

    // the blur also needs Y reversed in order to get correct results (while
    // the second blur step would end up with the correct orientation without
    // this too, but we need to blur the correct fragments in the second step
    // hence the flip is important)
    QMatrix4x4 flipY;
    // correct for D3D and Metal but not for Vulkan because there the Y is down
    // in NDC so that kind of self-corrects...
    if (rhi->isYUpInFramebuffer() != rhi->isYUpInNDC())
        flipY.data()[5] = -1.0f;
    rub->updateDynamicBuffer(ubufContainer.ubuf, 0, 64, flipY.constData());
    float cameraProperties[2] = { shadowFilter, shadowMapFar };
    rub->updateDynamicBuffer(ubufContainer.ubuf, 64, 8, cameraProperties);

    QRhiSampler *sampler = rhiCtx->sampler({ QSSGRenderTextureCoordOp::ClampToEdge, QSSGRenderTextureCoordOp::ClampToEdge, false });
    Q_ASSERT(sampler);

    QSSGRhiContext::ShaderResourceBindingList bindings = {
        QRhiShaderResourceBinding::uniformBuffer(0, VISIBILITY_ALL, ubufContainer.ubuf),
        QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, map, sampler)
    };
    QRhiShaderResourceBindings *srb = rhiCtx->srb(bindings);

    const bool wantsUV = orthographic; // orthoshadowshadowblurx and y have attr_uv as well
    renderer->rhiQuadRenderer()->recordQuadRenderPass(rhiCtx, rub, &ps, srb, pEntry->m_rhiBlurRenderTarget0, wantsUV);

    // repeat for blur Y, now depthCopy -> depthMap or cubeCopy -> depthCube

    shaderPipeline = orthographic ? renderer->getRhiOrthographicShadowBlurYShader()
                                  : renderer->getRhiCubemapShadowBlurYShader();
    if (!shaderPipeline)
        return;
    ps.shaderStages = shaderPipeline->stages();

    ubufKey = {
        map,
        reinterpret_cast<const void *>(quintptr(2)), // blur Y
        nullptr
    };

    bindings = {
        QRhiShaderResourceBinding::uniformBuffer(0, VISIBILITY_ALL, ubufContainer.ubuf),
        QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, workMap, sampler)
    };
    srb = rhiCtx->srb(bindings);

    renderer->rhiQuadRenderer()->recordQuadRenderPass(rhiCtx, nullptr, &ps, srb, pEntry->m_rhiBlurRenderTarget1, wantsUV);
}

static void rhiRenderShadowMap(QSSGRhiContext *rhiCtx,
                               QSSGLayerRenderData &inData,
                               QSSGRef<QSSGRenderShadowMap> &shadowMapManager,
                               const QRectF &viewViewport,
                               const QSSGRenderCamera &camera,
                               const QVector<QSSGRenderLight *> &globalLights,
                               const QVector<QSSGRenderableObjectHandle> &sortedOpaqueObjects,
                               const QSSGRef<QSSGRendererImpl> &renderer)
{
    QRhi *rhi = rhiCtx->rhi();
    QRhiCommandBuffer *cb = rhiCtx->commandBuffer();

    QSSGRhiGraphicsPipelineState ps;
    ps.depthTestEnable = true;
    ps.depthWriteEnable = true;
    ps.depthFunc = QRhiGraphicsPipeline::LessOrEqual;

    // We need to deal with a clip depth range of [0, 1] or
    // [-1, 1], depending on the graphics API underneath.
    float depthAdjust[2]; // (d + depthAdjust[0]) * depthAdjust[1] = d mapped to [0, 1]
    if (rhi->isClipDepthZeroToOne()) {
        // d is [0, 1] so no need for any mapping
        depthAdjust[0] = 0.0f;
        depthAdjust[1] = 1.0f;
    } else {
        // d is [-1, 1]
        depthAdjust[0] = 1.0f;
        depthAdjust[1] = 0.5f;
    }

    // Try reducing self-shadowing and artifacts.
    ps.depthBias = 2;
    ps.slopeScaledDepthBias = 1.5f;

    for (int i = 0, ie = globalLights.count(); i != ie; ++i) {
        if (!globalLights[i]->m_castShadow)
            continue;

        QSSGShadowMapEntry *pEntry = shadowMapManager->getShadowMapEntry(i);
        if (!pEntry)
            continue;

        Q_ASSERT(pEntry->m_rhiDepthStencil);
        const bool orthographic = pEntry->m_rhiDepthMap && pEntry->m_rhiDepthCopy;
        if (orthographic) {
            const QSize size = pEntry->m_rhiDepthMap->pixelSize();
            ps.viewport = QRhiViewport(0, 0, float(size.width()), float(size.height()));

            QSSGRenderCamera theCamera;
            RendererImpl::setupCameraForShadowMap(viewViewport, camera, globalLights[i], theCamera);
            theCamera.calculateViewProjectionMatrix(pEntry->m_lightVP);
            pEntry->m_lightView = theCamera.globalTransform.inverted(); // pre-calculate this for the material

            rhiPrepareResourcesForShadowMap(rhiCtx, inData, pEntry, depthAdjust,
                                            sortedOpaqueObjects, theCamera, true, 0);

            // Render into the 2D texture pEntry->m_rhiDepthMap, using
            // pEntry->m_rhiDepthStencil as the (throwaway) depth/stencil buffer.
            QRhiTextureRenderTarget *rt = pEntry->m_rhiRenderTargets[0];
            cb->beginPass(rt, Qt::white, { 1.0f, 0 });
            rhiRenderOneShadowMap(rhiCtx, inData, pEntry, &ps, sortedOpaqueObjects, true, 0);
            cb->endPass();

            rhiBlurShadowMap(rhiCtx, pEntry, renderer, globalLights[i]->m_shadowFilter, globalLights[i]->m_shadowMapFar, true);
        } else {
            Q_ASSERT(pEntry->m_rhiDepthCube && pEntry->m_rhiCubeCopy);
            const QSize size = pEntry->m_rhiDepthCube->pixelSize();
            ps.viewport = QRhiViewport(0, 0, float(size.width()), float(size.height()));

            QSSGRenderCamera theCameras[6];
            RendererImpl::setupCubeShadowCameras(globalLights[i], theCameras);
            pEntry->m_lightView = QMatrix4x4();

            const bool swapYFaces = !rhi->isYUpInFramebuffer();
            for (int face = 0; face < 6; ++face) {
                theCameras[face].calculateViewProjectionMatrix(pEntry->m_lightVP);
                pEntry->m_lightCubeView[face] = theCameras[face].globalTransform.inverted(); // pre-calculate this for the material

                rhiPrepareResourcesForShadowMap(rhiCtx, inData, pEntry, depthAdjust,
                                                sortedOpaqueObjects, theCameras[face], false, face);

                // Render into one face of the cubemap texture pEntry->m_rhiDephCube, using
                // pEntry->m_rhiDepthStencil as the (throwaway) depth/stencil buffer.

                int outFace = face;
                // FACE  S  T               GL
                // +x   -z, -y   right
                // -x   +z, -y   left
                // +y   +x, +z   top
                // -y   +x, -z   bottom
                // +z   +x, -y   front
                // -z   -x, -y   back
                // FACE  S  T               D3D
                // +x   -z, +y   right
                // -x   +z, +y   left
                // +y   +x, -z   bottom
                // -y   +x, +z   top
                // +z   +x, +y   front
                // -z   -x, +y   back
                if (swapYFaces) {
                    // +Y and -Y faces get swapped (D3D, Vulkan, Metal).
                    // See shadowMapping.glsllib. This is complemented there by reversing T as well.
                    if (outFace == 2)
                        outFace = 3;
                    else if (outFace == 3)
                        outFace = 2;
                }
                QRhiTextureRenderTarget *rt = pEntry->m_rhiRenderTargets[outFace];
                cb->beginPass(rt, Qt::white, { 1.0f, 0 });
                rhiRenderOneShadowMap(rhiCtx, inData, pEntry, &ps, sortedOpaqueObjects, false, face);
                cb->endPass();
            }

            rhiBlurShadowMap(rhiCtx, pEntry, renderer, globalLights[i]->m_shadowFilter, globalLights[i]->m_shadowMapFar, false);
        }
    }
}

void QSSGLayerRenderData::rhiRunPreparePass(TRhiPrepareRenderableFunction inPrepareFn,
                                            const QSSGLayerRenderPreparationResult &prepResult,
                                            bool inEnableBlending,
                                            bool inEnableDepthWrite,
                                            bool inEnableTransparentDepthWrite,
                                            bool inSortOpaqueRenderables,
                                            quint32 indexLight,
                                            const QSSGRenderCamera &inCamera)
{
    QSSGRhiContext *rhiCtx = renderer->context()->rhiContext().data();
    Q_ASSERT(rhiCtx->rhi()->isRecordingFrame());
    QRhiCommandBuffer *cb = rhiCtx->commandBuffer();

    const QVector2D theCameraProps = QVector2D(camera->clipNear, camera->clipFar);
    const auto &theOpaqueObjects = getOpaqueRenderableObjects(inSortOpaqueRenderables);

    if (prepResult.flags.requiresShadowMapPass() && m_progressiveAAPassIndex == 0) {
        if (!shadowMapManager)
            createShadowMapManager();

        if (!opaqueObjects.isEmpty() || !globalLights.isEmpty()) {
            setShaderFeature(QSSGShaderDefines::asString(QSSGShaderDefines::CgLighting), true);
            setShaderFeature(QSSGShaderDefines::asString(QSSGShaderDefines::Rhi), true);

            cb->debugMarkBegin(QByteArrayLiteral("Quick3D shadow map generation"));

            rhiRenderShadowMap(rhiCtx, *this, shadowMapManager, prepResult.viewport(), *camera,
                               globalLights, // scoped lights are not relevant here
                               theOpaqueObjects,
                               renderer);

            cb->debugMarkEnd();
        }
    }

    // make the buffer copies and other stuff we put on the command buffer in
    // here show up within a named section in tools like RenderDoc when running
    // with QSG_RHI_PROFILE=1 (which enables debug markers)
    cb->debugMarkBegin(QByteArrayLiteral("Quick3D prepare renderables"));

    QSSGRhiGraphicsPipelineState *ps = rhiCtx->graphicsPipelineState(this);

    ps->depthFunc = QRhiGraphicsPipeline::LessOrEqual;
    ps->blendEnable = false;

    const bool usingDepthBuffer = /* layer.flags.testFlag(QSSGRenderLayer::Flag::LayerEnableDepthTest) && */ !theOpaqueObjects.isEmpty();
    if (usingDepthBuffer) {
        ps->depthTestEnable = true;
        ps->depthWriteEnable = inEnableDepthWrite;
    } else {
        ps->depthTestEnable = false;
        ps->depthWriteEnable = false;
    }

    for (const auto &handle : theOpaqueObjects) {
        QSSGRenderableObject *theObject = handle.obj;
        QSSGScopedLightsListScope lightsScope(globalLights, lightDirections, sourceLightDirections, theObject->scopedLights);
        setShaderFeature(QSSGShaderDefines::asString(QSSGShaderDefines::CgLighting), globalLights.empty() == false);
        setShaderFeature(QSSGShaderDefines::asString(QSSGShaderDefines::Rhi), true);
        inPrepareFn(rhiCtx, *this, *theObject, theCameraProps, getShaderFeatureSet(), indexLight, inCamera);
    }

    // transparent objects
    if (inEnableBlending || !layer.flags.testFlag(QSSGRenderLayer::Flag::LayerEnableDepthTest)) {
        ps->blendEnable = inEnableBlending;
        ps->depthWriteEnable = inEnableTransparentDepthWrite;

        const auto &theTransparentObjects = getTransparentRenderableObjects();
        // "Assume all objects have transparency if the layer's depth test enabled flag is true." - ?!
        // The original code talks some nonsense about an "alternate route" with that code path containing the exact same code. Forget it.
        if (1 /*layer.flags.testFlag(QSSGRenderLayer::Flag::LayerEnableDepthTest)*/) {
            for (const auto &handle : theTransparentObjects) {
                QSSGRenderableObject *theObject = handle.obj;
                if (!(theObject->renderableFlags.isCompletelyTransparent())) {
                    QSSGScopedLightsListScope lightsScope(globalLights, lightDirections, sourceLightDirections, theObject->scopedLights);
                    setShaderFeature(QSSGShaderDefines::asString(QSSGShaderDefines::CgLighting), !globalLights.empty());
                    setShaderFeature(QSSGShaderDefines::asString(QSSGShaderDefines::Rhi), true);
                    inPrepareFn(rhiCtx, *this, *theObject, theCameraProps, getShaderFeatureSet(), indexLight, inCamera);
                }
            }
        }
    }

    cb->debugMarkEnd();
}

void QSSGLayerRenderData::rhiPrepare()
{
    QSSGRhiContext *rhiCtx = renderer->context()->rhiContext().data();
    Q_ASSERT(rhiCtx->isValid());

    QSSGRhiGraphicsPipelineState *ps = rhiCtx->resetGraphicsPipelineState(this);

    const QRectF vp = layerPrepResult->viewport();
    ps->viewport = { float(vp.x()), float(vp.y()), float(vp.width()), float(vp.height()), 0.0f, 1.0f };
    ps->scissorEnable = true;
    const QRect sc = layerPrepResult->scissor().toRect();
    ps->scissor = { sc.x(), sc.y(), sc.width(), sc.height() };

    QSSGStackPerfTimer ___timer(renderer->contextInterface()->performanceTimer(), Q_FUNC_INFO);
    if (camera) {
        renderer->beginLayerRender(*this);
        rhiRunPreparePass(rhiPrepareRenderable,
                          *layerPrepResult,
                          true, // blending
                          true /* !layer.flags.testFlag(QSSGRenderLayer::Flag::LayerEnableDepthPrePass) */, // depth write
                          false, // transparent depth write
                          true, // sort opaque renderables
                          0, // indexLight
                          *camera);
        renderer->endLayerRender();
    }
}

static void rhiRenderRenderable(QSSGRhiContext *rhiCtx,
                                QSSGLayerRenderData &inData,
                                QSSGRenderableObject &object,
                                bool *needsSetViewport)
{
    QRhiCommandBuffer *cb = rhiCtx->commandBuffer();
    if (object.renderableFlags.isDefaultMaterialMeshSubset()) {
        QSSGSubsetRenderable &subsetRenderable(static_cast<QSSGSubsetRenderable &>(object));

        QRhiGraphicsPipeline *ps = subsetRenderable.rhiRenderData.pipeline;
        if (!ps)
            return;

        QRhiShaderResourceBindings *srb = subsetRenderable.rhiRenderData.srb;
        if (!srb)
            return;

        QRhiBuffer *vertexBuffer = subsetRenderable.subset.rhi.ia.vertexBuffer->buffer();
        QRhiBuffer *indexBuffer = subsetRenderable.subset.rhi.ia.indexBuffer ? subsetRenderable.subset.rhi.ia.indexBuffer->buffer() : nullptr;

        // QRhi optimizes out unnecessary binding of the same pipline
        cb->setGraphicsPipeline(ps);
        cb->setShaderResources(srb);

        if (*needsSetViewport) {
            cb->setViewport(rhiCtx->graphicsPipelineState(&inData)->viewport);
            *needsSetViewport = false;
        }

        QRhiCommandBuffer::VertexInput vb(vertexBuffer, 0);
        if (indexBuffer) {
            cb->setVertexInput(0, 1, &vb, indexBuffer, 0, subsetRenderable.subset.rhi.ia.indexBuffer->indexFormat());
            cb->drawIndexed(subsetRenderable.subset.count, 1, subsetRenderable.subset.offset);
        } else {
            cb->setVertexInput(0, 1, &vb);
            cb->draw(subsetRenderable.subset.count, 1, subsetRenderable.subset.offset);
        }

        // context->draw(subset.gl.primitiveType, subset.count, subset.offset);

    } else if (object.renderableFlags.isCustomMaterialMeshSubset()) {
        // ###
    } else {
        Q_ASSERT(false);
    }
}

void QSSGLayerRenderData::rhiRender()
{
    QSSGRhiContext *rhiCtx = renderer->context()->rhiContext().data();

    QSSGStackPerfTimer ___timer(renderer->contextInterface()->performanceTimer(), Q_FUNC_INFO);
    if (camera) {
        renderer->beginLayerRender(*this);

        rhiCtx->commandBuffer()->debugMarkBegin(QByteArrayLiteral("Quick3D render renderables"));

        bool needsSetViewport = true;
        const auto &theOpaqueObjects = getOpaqueRenderableObjects(true);
        for (const auto &handle : theOpaqueObjects) {
            QSSGRenderableObject *theObject = handle.obj;
            rhiRenderRenderable(rhiCtx, *this, *theObject, &needsSetViewport);
        }

        const auto &theTransparentObjects = getTransparentRenderableObjects();
        for (const auto &handle : theTransparentObjects) {
            QSSGRenderableObject *theObject = handle.obj;
            if (!theObject->renderableFlags.isCompletelyTransparent())
                rhiRenderRenderable(rhiCtx, *this, *theObject, &needsSetViewport);
        }

        rhiCtx->commandBuffer()->debugMarkEnd();

        renderer->endLayerRender();
    }
}

QT_END_NAMESPACE
