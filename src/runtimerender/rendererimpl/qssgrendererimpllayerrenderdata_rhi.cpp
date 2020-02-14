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

static inline QRhiSampler::AddressMode toRhi(QSSGRenderTextureCoordOp tiling)
{
    switch (tiling) {
    case QSSGRenderTextureCoordOp::Repeat:
        return QRhiSampler::Repeat;
    case QSSGRenderTextureCoordOp::MirroredRepeat:
        return QRhiSampler::Mirror;
    default:
    case QSSGRenderTextureCoordOp::ClampToEdge:
        return QRhiSampler::ClampToEdge;
    }
}

static inline int bindingForTexture(const QString &name, const QSSGRhiShaderStages &shaderStages)
{
    QVector<QShaderDescription::InOutVariable> samplerVariables =
            shaderStages.fragmentStage()->shader().description().combinedImageSamplers();
    auto it = std::find_if(samplerVariables.cbegin(), samplerVariables.cend(),
                           [&name](const QShaderDescription::InOutVariable &s) { return s.name == name; });
    if (it != samplerVariables.cend())
        return it->binding;

    return -1;
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

            QRhiResourceUpdateBatch *resourceUpdates = rhiCtx->rhi()->nextResourceUpdateBatch();

            // Unlike the subsetRenderable (which is allocated per frame so is
            // not persistent in any way), the model reference is persistent in
            // the sense that it references the model node in the scene graph.
            // Combined with the layer node (multiple View3Ds may share the
            // same scene!), this is suitable as a key to get the uniform
            // buffers that were used with the rendering of the same model in
            // the previous frame.
            const void *layerNode = &inData.layer;
            const void *modelNode = &subsetRenderable.modelContext.model;

            QSSGRhiUniformBufferSet &uniformBuffers(rhiCtx->uniformBufferSet({ layerNode, modelNode,
                                                                               nullptr, QSSGRhiUniformBufferSetKey::Main }));
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
                while (renderableImage) {
                    // TODO: optimize this! We're looking for the sampler corresponding to imageNumber, and currently the
                    // only information we have is the name of the form "image0_sampler"
                    int samplerBinding = bindingForTexture(QStringLiteral("image%1_sampler").arg(imageNumber), *ps->shaderStages);
                    if (samplerBinding >= 0) {
                        QRhiTexture *texture = renderableImage->m_image.m_textureData.m_rhiTexture;
                        if (samplerBinding >= 0 && texture) {
                            QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                                                     toRhi(renderableImage->m_image.m_horizontalTilingMode),
                                                                     toRhi(renderableImage->m_image.m_verticalTilingMode) });
                            bindings.append(QRhiShaderResourceBinding::sampledTexture(samplerBinding,
                                                                                      QRhiShaderResourceBinding::FragmentStage,
                                                                                      texture, sampler));
                        }
                    } else {
                        qWarning("Could not find sampler for image");
                    }
                    renderableImage = renderableImage->m_nextImage;
                    imageNumber++;
                }
            }

            // Shadow map textures
            if (shaderPipeline->isLightingEnabled()) {
                const int shadowMapCount = shaderPipeline->shadowMapCount();
                for (int i = 0; i < shadowMapCount; ++i) {
                    QSSGRhiShadowMapProperties &shadowMapProperties(shaderPipeline->shadowMapAt(i));
                    QRhiTexture *texture = shadowMapProperties.shadowMapTexture;
                    QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                                             QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge });
                    Q_ASSERT(texture && sampler);
                    const QString name = QString::fromLatin1(shadowMapProperties.shadowMapTextureUniformName);
                    if (shadowMapProperties.cachedBinding < 0)
                        shadowMapProperties.cachedBinding = bindingForTexture(name, *ps->shaderStages);
                    if (shadowMapProperties.cachedBinding < 0) {
                        qWarning("No combined image sampler for shadow map texture '%s'", qPrintable(name));
                        continue;
                    }
                    bindings.append(QRhiShaderResourceBinding::sampledTexture(shadowMapProperties.cachedBinding,
                                                                              QRhiShaderResourceBinding::FragmentStage,
                                                                              texture,
                                                                              sampler));
                }
            }

            // Light probe texture
            if (shaderPipeline->lightProbeTexture()) {
                int binding = bindingForTexture(QLatin1String("lightProbe"), *ps->shaderStages);
                if (binding >= 0) {
                    auto tiling = shaderPipeline->lightProbeTiling();
                    QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::Linear, // enables mipmapping
                                                             toRhi(tiling.first), toRhi(tiling.second) });
                    bindings.append(QRhiShaderResourceBinding::sampledTexture(binding,
                                                                              QRhiShaderResourceBinding::FragmentStage,
                                                                              shaderPipeline->lightProbeTexture(), sampler));
                } else {
                    qWarning("Could not find sampler for lightprobe");
                }
            }

            // Depth texture
            if (shaderPipeline->depthTexture()) {
                int binding = bindingForTexture(QLatin1String("depthTexture"), *ps->shaderStages);
                if (binding >= 0) {
                     // nearest min/mag, no mipmap
                     QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Nearest, QRhiSampler::Nearest, QRhiSampler::None,
                                                              QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge });
                     bindings.append(QRhiShaderResourceBinding::sampledTexture(binding,
                                                                               QRhiShaderResourceBinding::FragmentStage,
                                                                               shaderPipeline->depthTexture(), sampler));
                 } // else ignore, not an error
            }

            // SSAO texture
            if (shaderPipeline->ssaoTexture()) {
                int binding = bindingForTexture(QLatin1String("aoTexture"), *ps->shaderStages);
                if (binding >= 0) {
                    // linear min/mag, no mipmap
                    QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                                             QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge });
                    bindings.append(QRhiShaderResourceBinding::sampledTexture(binding,
                                                                              QRhiShaderResourceBinding::FragmentStage,
                                                                              shaderPipeline->ssaoTexture(), sampler));
                } // else ignore, not an error
            }

            QRhiShaderResourceBindings *srb = rhiCtx->srb(bindings);

            const QSSGGraphicsPipelineStateKey pipelineKey { *ps, rhiCtx->mainRenderPassDesciptor(), srb };
            subsetRenderable.rhiRenderData.mainPass.pipeline = rhiCtx->pipeline(pipelineKey);
            subsetRenderable.rhiRenderData.mainPass.srb = srb;

        }
    } else if (inObject.renderableFlags.isCustomMaterialMeshSubset()) {
        // ### TODO custom materials
        Q_UNUSED(inData);
        Q_UNUSED(inCamera);
    } else {
        Q_ASSERT(false);
    }
}

static bool rhiPrepareDepthPassForObject(QSSGRhiContext *rhiCtx,
                                         QSSGLayerRenderData &inData,
                                         QSSGRenderableObject *obj,
                                         QRhiRenderPassDescriptor *rpDesc,
                                         QSSGRhiGraphicsPipelineState *ps,
                                         QRhiResourceUpdateBatch *rub,
                                         QSSGRhiUniformBufferSetKey::Selector ubufSel)
{
    QRhi *rhi = rhiCtx->rhi();
    const QMatrix4x4 correctionMatrix = rhi->clipSpaceCorrMatrix();

    // material specific have to be done differently for default and custom materials
    if (obj->renderableFlags.isDefaultMaterialMeshSubset()) {
        // We need to know the cull mode and if displacemant maps are used.
        QSSGSubsetRenderable &subsetRenderable(static_cast<QSSGSubsetRenderable &>(*obj));

        ps->cullMode = QSSGRhiGraphicsPipelineState::toCullMode(subsetRenderable.material.cullingMode);

        QSSGRenderableImage *displacementImage = nullptr;
        for (QSSGRenderableImage *theImage = subsetRenderable.firstImage; theImage; theImage = theImage->m_nextImage) {
            if (theImage->m_mapType == QSSGImageMapTypes::Displacement) {
                displacementImage = theImage;
                break;
            }
        }

        // Displacement maps are currently incompatible with the depth pass (it
        // would need a different set of shaders that actually samples the map
        // etc.). What we do now is just disable the Z prepass or depth texture
        // altogether whenever a displacement map is encountered.
        //
        // And in any case, displacement maps need tessellation shader
        // support which we will not have in the foreseeable future so that
        // feature is useless in the first place.
        //
        if (displacementImage) {
            static bool warned = false;
            if (!warned) {
                warned = true;
                qWarning("Displacement maps are not currently supported in combination with a depth pass.");
            }
            return false;
        }

    } else if (obj->renderableFlags.isCustomMaterialMeshSubset()) {
        // ### TODO custom materials
    }

    // the rest is common
    if (obj->renderableFlags.isDefaultMaterialMeshSubset() || obj->renderableFlags.isCustomMaterialMeshSubset()) {
        QSSGSubsetRenderableBase *subsetRenderable(static_cast<QSSGSubsetRenderableBase *>(obj));
        QSSGRef<QSSGRhiShaderStagesWithResources> shaderStages = subsetRenderable->generator->getRhiDepthPrepassShader();
        if (!shaderStages)
            return false;

        ps->shaderStages = shaderStages->stages();
        ps->ia = subsetRenderable->subset.rhi.iaDepth;

        // the depth prepass shader takes the mvp matrix, nothing else
        const int UBUF_SIZE = 64;
        const QSSGRhiUniformBufferSetKey ubufKey = { &inData.layer, &subsetRenderable->modelContext.model, nullptr, ubufSel };
        QSSGRhiUniformBufferSet &uniformBuffers(rhiCtx->uniformBufferSet(ubufKey));
        QRhiBuffer *&ubuf = uniformBuffers.ubuf;
        if (!ubuf) {
            ubuf = rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, UBUF_SIZE);
            ubuf->build();
        } else if (ubuf->size() < UBUF_SIZE) {
            ubuf->setSize(UBUF_SIZE);
            ubuf->build();
        }

        const QMatrix4x4 mvp = correctionMatrix * subsetRenderable->modelContext.modelViewProjection;
        rub->updateDynamicBuffer(ubuf, 0, 64, mvp.constData());

        QSSGRhiContext::ShaderResourceBindingList bindings;
        bindings.append(QRhiShaderResourceBinding::uniformBuffer(0, VISIBILITY_ALL, ubuf));
        QRhiShaderResourceBindings *srb = rhiCtx->srb(bindings);
        const QSSGGraphicsPipelineStateKey pipelineKey { *ps, rpDesc, srb };
        QRhiGraphicsPipeline *pipeline = rhiCtx->pipeline(pipelineKey);

        subsetRenderable->rhiRenderData.depthPrePass.pipeline = pipeline;
        subsetRenderable->rhiRenderData.depthPrePass.srb = srb;
    }

    return true;
}

static bool rhiPrepareDepthPass(QSSGRhiContext *rhiCtx,
                                const QSSGRhiGraphicsPipelineState &basePipelineState,
                                QRhiRenderPassDescriptor *rpDesc,
                                QSSGLayerRenderData &inData,
                                const QVector<QSSGRenderableObjectHandle> &sortedOpaqueObjects,
                                const QVector<QSSGRenderableObjectHandle> &sortedTransparentObjects,
                                QSSGRhiUniformBufferSetKey::Selector ubufSel)
{
    // Phase 1 (prepare) for the Z prepass or the depth texture generation.
    // These renders opaque (Z prepass), or opaque and transparent (depth
    // texture), objects with depth test/write enabled, and color write
    // disabled, using a very simple set of shaders.

    QRhi *rhi = rhiCtx->rhi();
    QRhiCommandBuffer *cb = rhiCtx->commandBuffer();
    QSSGRhiGraphicsPipelineState ps = basePipelineState; // viewport and others are filled out already
    // We took a copy of the pipeline state since we do not want to conflict
    // with what rhiPrepare() collects for its own use. So here just change
    // whatever we need.

    ps.depthTestEnable = true;
    ps.depthWriteEnable = true;
    ps.targetBlend.colorWrite = {};

    QRhiResourceUpdateBatch *rub = rhi->nextResourceUpdateBatch();

    for (const QSSGRenderableObjectHandle &handle : sortedOpaqueObjects) {
        if (!rhiPrepareDepthPassForObject(rhiCtx, inData, handle.obj, rpDesc, &ps, rub, ubufSel))
            return false;
    }

    for (const QSSGRenderableObjectHandle &handle : sortedTransparentObjects) {
        if (!rhiPrepareDepthPassForObject(rhiCtx, inData, handle.obj, rpDesc, &ps, rub, ubufSel))
            return false;
    }

    cb->resourceUpdate(rub);

    return true;
}

static bool rhiPrepareDepthTexture(QSSGRhiContext *rhiCtx, const QSize &size, QSSGRhiRenderableTexture *renderableTex)
{
    QRhi *rhi = rhiCtx->rhi();
    bool needsBuild = false;

    if (!renderableTex->texture) {
        QRhiTexture::Format format = QRhiTexture::D32F;
        if (!rhi->isTextureFormatSupported(format))
            format = QRhiTexture::D16;
        if (!rhi->isTextureFormatSupported(format))
            qWarning("Depth texture not supported");
        // the depth texture is always non-msaa, even if multisampling is used in the main pass
        renderableTex->texture = rhiCtx->rhi()->newTexture(format, size, 1, QRhiTexture::RenderTarget);
        needsBuild = true;
    } else if (renderableTex->texture->pixelSize() != size) {
        renderableTex->texture->setPixelSize(size);
        needsBuild = true;
    }

    if (needsBuild) {
        if (!renderableTex->texture->build()) {
            qWarning("Failed to build depth texture (size %dx%d, format %d)",
                     size.width(), size.height(), int(renderableTex->texture->format()));
            renderableTex->reset();
            return false;
        }

        delete renderableTex->rt;
        delete renderableTex->rpDesc;
        QRhiTextureRenderTargetDescription rtDesc;
        rtDesc.setDepthTexture(renderableTex->texture);
        renderableTex->rt = rhi->newTextureRenderTarget(rtDesc);
        renderableTex->rpDesc = renderableTex->rt->newCompatibleRenderPassDescriptor();
        renderableTex->rt->setRenderPassDescriptor(renderableTex->rpDesc);
        if (!renderableTex->rt->build()) {
            qWarning("Failed to build render target for depth texture");
            renderableTex->reset();
            return false;
        }
    }

    return true;
}

static void rhiRenderDepthPassForObject(QSSGRhiContext *rhiCtx,
                                        QSSGLayerRenderData &inData,
                                        QSSGRenderableObject *obj,
                                        bool *needsSetViewport)
{
    QRhiCommandBuffer *cb = rhiCtx->commandBuffer();

    // casts to SubsetRenderableBase so it works for both default and custom materials
    if (obj->renderableFlags.isDefaultMaterialMeshSubset() || obj->renderableFlags.isCustomMaterialMeshSubset()) {
        QSSGSubsetRenderableBase *subsetRenderable(static_cast<QSSGSubsetRenderableBase *>(obj));

        QRhiBuffer *vertexBuffer = subsetRenderable->subset.rhi.iaDepth.vertexBuffer->buffer();
        QRhiBuffer *indexBuffer = subsetRenderable->subset.rhi.iaDepth.indexBuffer
                ? subsetRenderable->subset.rhi.iaDepth.indexBuffer->buffer()
                : nullptr;

        QRhiGraphicsPipeline *ps = subsetRenderable->rhiRenderData.depthPrePass.pipeline;
        if (!ps)
            return;

        QRhiShaderResourceBindings *srb = subsetRenderable->rhiRenderData.depthPrePass.srb;
        if (!srb)
            return;

        cb->setGraphicsPipeline(ps);
        cb->setShaderResources(srb);

        if (*needsSetViewport) {
            cb->setViewport(rhiCtx->graphicsPipelineState(&inData)->viewport);
            *needsSetViewport = false;
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

static void rhiRenderDepthPass(QSSGRhiContext *rhiCtx,
                               QSSGLayerRenderData &inData,
                               const QVector<QSSGRenderableObjectHandle> &sortedOpaqueObjects,
                               const QVector<QSSGRenderableObjectHandle> &sortedTransparentObjects,
                               bool *needsSetViewport)
{
    for (const QSSGRenderableObjectHandle &handle : sortedOpaqueObjects)
        rhiRenderDepthPassForObject(rhiCtx, inData, handle.obj, needsSetViewport);

    for (const QSSGRenderableObjectHandle &handle : sortedTransparentObjects)
        rhiRenderDepthPassForObject(rhiCtx, inData, handle.obj, needsSetViewport);
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

            const QSSGRhiUniformBufferSetKey ubufKey = { &inData.layer, &subsetRenderable->modelContext.model,
                                                         pEntry, QSSGRhiUniformBufferSetKey::Shadow };
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

            const QSSGRhiUniformBufferSetKey ubufKey = { &inData.layer, &subsetRenderable->modelContext.model,
                                                         pEntry, QSSGRhiUniformBufferSetKey::Shadow };
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
    QSSGRhiUniformBufferSetKey ubufKey = { map, nullptr, nullptr, QSSGRhiUniformBufferSetKey::ShadowBlurX };

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

    QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                             QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge });
    Q_ASSERT(sampler);

    QSSGRhiContext::ShaderResourceBindingList bindings = {
        QRhiShaderResourceBinding::uniformBuffer(0, VISIBILITY_ALL, ubufContainer.ubuf),
        QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, map, sampler)
    };
    QRhiShaderResourceBindings *srb = rhiCtx->srb(bindings);

    const bool wantsUV = orthographic; // orthoshadowshadowblurx and y have attr_uv as well
    renderer->rhiQuadRenderer()->prepareQuad(rhiCtx, rub);
    renderer->rhiQuadRenderer()->recordRenderQuadPass(rhiCtx, &ps, srb, pEntry->m_rhiBlurRenderTarget0, wantsUV);

    // repeat for blur Y, now depthCopy -> depthMap or cubeCopy -> depthCube

    shaderPipeline = orthographic ? renderer->getRhiOrthographicShadowBlurYShader()
                                  : renderer->getRhiCubemapShadowBlurYShader();
    if (!shaderPipeline)
        return;
    ps.shaderStages = shaderPipeline->stages();

    ubufKey = { map, nullptr, nullptr, QSSGRhiUniformBufferSetKey::ShadowBlurY };

    bindings = {
        QRhiShaderResourceBinding::uniformBuffer(0, VISIBILITY_ALL, ubufContainer.ubuf),
        QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, workMap, sampler)
    };
    srb = rhiCtx->srb(bindings);

    renderer->rhiQuadRenderer()->prepareQuad(rhiCtx, nullptr);
    renderer->rhiQuadRenderer()->recordRenderQuadPass(rhiCtx, &ps, srb, pEntry->m_rhiBlurRenderTarget1, wantsUV);
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

static bool rhiPrepareAoTexture(QSSGRhiContext *rhiCtx, const QSize &size, QSSGRhiRenderableTexture *renderableTex)
{
    QRhi *rhi = rhiCtx->rhi();
    bool needsBuild = false;

    if (!renderableTex->texture) {
        // the ambient occlusion texture is always non-msaa, even if multisampling is used in the main pass
        renderableTex->texture = rhiCtx->rhi()->newTexture(QRhiTexture::RGBA8, size, 1, QRhiTexture::RenderTarget);
        needsBuild = true;
    } else if (renderableTex->texture->pixelSize() != size) {
        renderableTex->texture->setPixelSize(size);
        needsBuild = true;
    }

    if (needsBuild) {
        if (!renderableTex->texture->build()) {
            qWarning("Failed to build ambient occlusion texture (size %dx%d)", size.width(), size.height());
            renderableTex->reset();
            return false;
        }

        delete renderableTex->rt;
        delete renderableTex->rpDesc;
        renderableTex->rt = rhi->newTextureRenderTarget({ renderableTex->texture });
        renderableTex->rpDesc = renderableTex->rt->newCompatibleRenderPassDescriptor();
        renderableTex->rt->setRenderPassDescriptor(renderableTex->rpDesc);
        if (!renderableTex->rt->build()) {
            qWarning("Failed to build render target for ambient occlusion texture");
            renderableTex->reset();
            return false;
        }
    }

    return true;
}

static void rhiRenderAoTexture(QSSGRhiContext *rhiCtx,
                               const QSSGRhiGraphicsPipelineState &basePipelineState,
                               const QSSGLayerRenderData &inData,
                               const QSSGRenderCamera &camera)
{
    // ### enable this when the qtbase patch has merged
#if 0
    // no texelFetch in GLSL <= 120 and GLSL ES 100
    if (!rhiCtx->rhi()->isFeatureSupported(QRhi::TexelFetch)) {
        QRhiCommandBuffer *cb = rhiCtx->commandBuffer();
        // just clear and stop there
        cb->beginPass(inData.m_rhiAoTexture.rt, Qt::white, { 1.0f, 0 });
        cb->endPass();
        return;
    }
#endif

    QSSGRef<QSSGRhiShaderStagesWithResources> shaderPipeline = inData.renderer->getRhiSsaoShader();
    if (!shaderPipeline)
        return;

    QSSGRhiGraphicsPipelineState ps = basePipelineState;
    ps.shaderStages = shaderPipeline->stages();

    const float R2 = inData.layer.aoDistance * inData.layer.aoDistance * 0.16f;
    const QSize textureSize = inData.m_rhiAoTexture.texture->pixelSize();
    const float rw = float(textureSize.width());
    const float rh = float(textureSize.height());
    const float fov = camera.verticalFov(rw / rh);
    const float tanHalfFovY = tanf(0.5f * fov * (rh / rw));
    const float invFocalLenX = tanHalfFovY * (rw / rh);

    const QVector4D aoProps(inData.layer.aoStrength * 0.01f, inData.layer.aoDistance * 0.4f, inData.layer.aoSoftness * 0.02f, inData.layer.aoBias);
    const QVector4D aoProps2(float(inData.layer.aoSamplerate), (inData.layer.aoDither) ? 1.0f : 0.0f, 0.0f, 0.0f);
    const QVector4D aoScreenConst(1.0f / R2, rh / (2.0f * tanHalfFovY), 1.0f / rw, 1.0f / rh);
    const QVector4D uvToEyeConst(2.0f * invFocalLenX, -2.0f * tanHalfFovY, -invFocalLenX, tanHalfFovY);
    const QVector2D cameraProps(camera.clipNear, camera.clipFar);

//    layout(std140, binding = 0) uniform buf {
//        vec4 aoProperties;
//        vec4 aoProperties2;
//        vec4 aoScreenConst;
//        vec4 uvToEyeConst;
//        vec2 cameraProperties;

    const int UBUF_SIZE = 72;
    const QSSGRhiUniformBufferSetKey ubufKey = { &inData.layer, nullptr, nullptr, QSSGRhiUniformBufferSetKey::AoTexture };
    QSSGRhiUniformBufferSet &uniformBuffers(rhiCtx->uniformBufferSet(ubufKey));
    QRhiBuffer *&ubuf = uniformBuffers.ubuf;
    if (!ubuf) {
        ubuf = rhiCtx->rhi()->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, UBUF_SIZE);
        ubuf->build();
    } else if (ubuf->size() < UBUF_SIZE) {
        ubuf->setSize(UBUF_SIZE);
        ubuf->build();
    }

    QRhiResourceUpdateBatch *rub = rhiCtx->rhi()->nextResourceUpdateBatch();
    rub->updateDynamicBuffer(ubuf, 0, 16, &aoProps);
    rub->updateDynamicBuffer(ubuf, 16, 16, &aoProps2);
    rub->updateDynamicBuffer(ubuf, 32, 16, &aoScreenConst);
    rub->updateDynamicBuffer(ubuf, 48, 16, &uvToEyeConst);
    rub->updateDynamicBuffer(ubuf, 64, 8, &cameraProps);

    QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Nearest, QRhiSampler::Nearest, QRhiSampler::None,
                                             QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge });
    QSSGRhiContext::ShaderResourceBindingList bindings = {
        QRhiShaderResourceBinding::uniformBuffer(0, VISIBILITY_ALL, ubuf),
        QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, inData.m_rhiDepthTexture.texture, sampler)
    };
    QRhiShaderResourceBindings *srb = rhiCtx->srb(bindings);

    inData.renderer->rhiQuadRenderer()->prepareQuad(rhiCtx, rub);
    inData.renderer->rhiQuadRenderer()->recordRenderQuadPass(rhiCtx, &ps, srb, inData.m_rhiAoTexture.rt, false);
}

// Phase 1: prepare. Called when the renderpass is not yet started on the command buffer.
void QSSGLayerRenderData::rhiPrepare()
{
    QSSGRhiContext *rhiCtx = renderer->context()->rhiContext().data();
    Q_ASSERT(rhiCtx->isValid());

    setShaderFeature(QSSGShaderDefines::asString(QSSGShaderDefines::Rhi), true);

    QSSGRhiGraphicsPipelineState *ps = rhiCtx->resetGraphicsPipelineState(this);

    const QRectF vp = layerPrepResult->viewport();
    ps->viewport = { float(vp.x()), float(vp.y()), float(vp.width()), float(vp.height()), 0.0f, 1.0f };
    ps->scissorEnable = true;
    const QRect sc = layerPrepResult->scissor().toRect();
    ps->scissor = { sc.x(), sc.y(), sc.width(), sc.height() };

    QSSGStackPerfTimer ___timer(renderer->contextInterface()->performanceTimer(), Q_FUNC_INFO);
    if (camera) {
        renderer->beginLayerRender(*this);

        QSSGRhiContext *rhiCtx = renderer->context()->rhiContext().data();
        Q_ASSERT(rhiCtx->rhi()->isRecordingFrame());
        QRhiCommandBuffer *cb = rhiCtx->commandBuffer();

        const QVector2D theCameraProps = QVector2D(camera->clipNear, camera->clipFar);
        const auto &sortedOpaqueObjects = getOpaqueRenderableObjects(true); // front to back
        const auto &sortedTransparentObjects = getTransparentRenderableObjects(); // back to front

        // If needed, generate a depth texture, containing both opaque and alpha objects.
        if (layerPrepResult->flags.requiresDepthTexture() && m_progressiveAAPassIndex == 0) {
            cb->debugMarkBegin(QByteArrayLiteral("Quick3D depth texture"));

            if (rhiPrepareDepthTexture(rhiCtx, layerPrepResult->textureDimensions(), &m_rhiDepthTexture)) {
                Q_ASSERT(m_rhiDepthTexture.isValid());
                if (rhiPrepareDepthPass(rhiCtx, *ps, m_rhiDepthTexture.rpDesc, *this,
                                        sortedOpaqueObjects, sortedTransparentObjects,
                                        QSSGRhiUniformBufferSetKey::DepthTexture))
                {
                    bool needsSetVieport = true;
                    cb->beginPass(m_rhiDepthTexture.rt, Qt::transparent, { 1.0f, 0 });
                    rhiRenderDepthPass(rhiCtx, *this, sortedOpaqueObjects, sortedTransparentObjects, &needsSetVieport);
                    cb->endPass();
                } else {
                    m_rhiDepthTexture.reset();
                }
            }

            cb->debugMarkEnd();
        } else {
            // Do not keep it around when no longer needed. Internally QRhi
            // takes care of keeping the native texture resource around as long
            // as it is in use by an in-flight frame we do not have to worry
            // about that here.
            m_rhiDepthTexture.reset();
        }

        // Screen space ambient occlusion. Relies on the depth texture and generates an AO map.
        if (layerPrepResult->flags.requiresSsaoPass() && m_progressiveAAPassIndex == 0 && camera) {
           cb->debugMarkBegin(QByteArrayLiteral("Quick3D SSAO map"));

           if (rhiPrepareAoTexture(rhiCtx, layerPrepResult->textureDimensions(), &m_rhiAoTexture)) {
               Q_ASSERT(m_rhiAoTexture.isValid());
               rhiRenderAoTexture(rhiCtx, *ps, *this, *camera);
           }

           cb->debugMarkEnd();
        } else {
            m_rhiAoTexture.reset();
        }

        // Shadows. Generates a 2D or cube shadow map. (opaque objects only)
        if (layerPrepResult->flags.requiresShadowMapPass() && m_progressiveAAPassIndex == 0) {
            if (!shadowMapManager)
                createShadowMapManager();

            if (!opaqueObjects.isEmpty() || !globalLights.isEmpty()) {
                cb->debugMarkBegin(QByteArrayLiteral("Quick3D shadow map"));

                rhiRenderShadowMap(rhiCtx, *this, shadowMapManager, layerPrepResult->viewport(), *camera,
                                   globalLights, // scoped lights are not relevant here
                                   sortedOpaqueObjects,
                                   renderer);

                cb->debugMarkEnd();
            }
        }

        // Z (depth) pre-pass, if enabled, is part of the main render pass. (opaque objects only)
        // Prepare the data for it.
        bool zPrePass = layer.flags.testFlag(QSSGRenderLayer::Flag::LayerEnableDepthPrePass)
                && layer.flags.testFlag(QSSGRenderLayer::Flag::LayerEnableDepthTest)
                && !sortedOpaqueObjects.isEmpty();
        if (zPrePass) {
            cb->debugMarkBegin(QByteArrayLiteral("Quick3D prepare Z prepass"));
            if (!rhiPrepareDepthPass(rhiCtx, *ps, rhiCtx->mainRenderPassDesciptor(), *this, sortedOpaqueObjects, {},
                                     QSSGRhiUniformBufferSetKey::ZPrePass))
            {
                // alas, no Z prepass for you
                m_zPrePassPossible = false;
            }
            cb->debugMarkEnd();
        }

        // Now onto preparing the data for the main pass.

        // make the buffer copies and other stuff we put on the command buffer in
        // here show up within a named section in tools like RenderDoc when running
        // with QSG_RHI_PROFILE=1 (which enables debug markers)
        cb->debugMarkBegin(QByteArrayLiteral("Quick3D prepare renderables"));

        QSSGRhiGraphicsPipelineState *ps = rhiCtx->graphicsPipelineState(this);

        ps->depthFunc = QRhiGraphicsPipeline::LessOrEqual;
        ps->blendEnable = false;

        if (layer.flags.testFlag(QSSGRenderLayer::Flag::LayerEnableDepthTest) && !sortedOpaqueObjects.isEmpty()) {
            ps->depthTestEnable = true;
            // enable depth write for opaque objects when there was no Z prepass
            ps->depthWriteEnable = !layer.flags.testFlag(QSSGRenderLayer::Flag::LayerEnableDepthPrePass);
        } else {
            ps->depthTestEnable = false;
            ps->depthWriteEnable = false;
        }

        // opaque objects (or, this list is empty when LayerEnableDepthTest is disabled)
        for (const auto &handle : sortedOpaqueObjects) {
            QSSGRenderableObject *theObject = handle.obj;
            QSSGScopedLightsListScope lightsScope(globalLights, lightDirections, sourceLightDirections, theObject->scopedLights);
            setShaderFeature(QSSGShaderDefines::asString(QSSGShaderDefines::CgLighting), !globalLights.empty());
            rhiPrepareRenderable(rhiCtx, *this, *theObject, theCameraProps, getShaderFeatureSet(), 0, *camera);
        }

        // transparent objects (or, without LayerEnableDepthTest, all objects)
        ps->blendEnable = true;
        ps->depthWriteEnable = false;

        for (const auto &handle : sortedTransparentObjects) {
            QSSGRenderableObject *theObject = handle.obj;
            if (!(theObject->renderableFlags.isCompletelyTransparent())) {
                QSSGScopedLightsListScope lightsScope(globalLights, lightDirections, sourceLightDirections, theObject->scopedLights);
                setShaderFeature(QSSGShaderDefines::asString(QSSGShaderDefines::CgLighting), !globalLights.empty());
                rhiPrepareRenderable(rhiCtx, *this, *theObject, theCameraProps, getShaderFeatureSet(), 0, *camera);
            }
        }

        cb->debugMarkEnd();

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

        QRhiGraphicsPipeline *ps = subsetRenderable.rhiRenderData.mainPass.pipeline;
        if (!ps)
            return;

        QRhiShaderResourceBindings *srb = subsetRenderable.rhiRenderData.mainPass.srb;
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
    } else if (object.renderableFlags.isCustomMaterialMeshSubset()) {
        // ### TODO custom materials

        // May be identical to the above due to rhiRenderData being in
        // SubsetRenderableBase (so it is there for both type of materials), it
        // is likely that it is the prepare step that will diverge for default
        // and custom materials, not the draw call recording here.

    } else {
        Q_ASSERT(false);
    }
}

// Phase 2: render. Called within an active renderpass on the command buffer.
void QSSGLayerRenderData::rhiRender()
{
    QSSGRhiContext *rhiCtx = renderer->context()->rhiContext().data();

    QSSGStackPerfTimer ___timer(renderer->contextInterface()->performanceTimer(), Q_FUNC_INFO);
    if (camera) {
        renderer->beginLayerRender(*this);

        QRhiCommandBuffer *cb = rhiCtx->commandBuffer();
        const auto &theOpaqueObjects = getOpaqueRenderableObjects(true);
        bool needsSetViewport = true;

        bool zPrePass = layer.flags.testFlag(QSSGRenderLayer::Flag::LayerEnableDepthPrePass)
                && layer.flags.testFlag(QSSGRenderLayer::Flag::LayerEnableDepthTest)
                && !theOpaqueObjects.isEmpty();
        if (zPrePass && m_zPrePassPossible) {
            cb->debugMarkBegin(QByteArrayLiteral("Quick3D render Z prepass"));
            rhiRenderDepthPass(rhiCtx, *this, theOpaqueObjects, {}, &needsSetViewport);
            cb->debugMarkEnd();
        }

        cb->debugMarkBegin(QByteArrayLiteral("Quick3D render opaque"));
        for (const auto &handle : theOpaqueObjects) {
            QSSGRenderableObject *theObject = handle.obj;
            rhiRenderRenderable(rhiCtx, *this, *theObject, &needsSetViewport);
        }
        cb->debugMarkEnd();

        cb->debugMarkBegin(QByteArrayLiteral("Quick3D render alpha"));
        const auto &theTransparentObjects = getTransparentRenderableObjects();
        for (const auto &handle : theTransparentObjects) {
            QSSGRenderableObject *theObject = handle.obj;
            if (!theObject->renderableFlags.isCompletelyTransparent())
                rhiRenderRenderable(rhiCtx, *this, *theObject, &needsSetViewport);
        }
        cb->debugMarkEnd();

        renderer->endLayerRender();
    }
}

QT_END_NAMESPACE
