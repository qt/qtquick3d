/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "qssgrhiparticles_p.h"
#include "qssgrhicontext_p.h"

#include <QtQuick3DUtils/private/qssgutils_p.h>

#include <QtQuick3DRuntimeRender/private/qssgrenderer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercamera_p.h>

QT_BEGIN_NAMESPACE

static const QRhiShaderResourceBinding::StageFlags VISIBILITY_ALL =
        QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage;

void QSSGParticleRenderer::updateUniformsForParticles(QSSGRef<QSSGRhiShaderPipeline> &shaders,
                                                      QSSGRhiContext *rhiCtx,
                                                      char *ubufData,
                                                      QSSGParticlesRenderable &renderable,
                                                      QSSGRenderCamera &inCamera)
{
    const QSSGRef<QSSGRenderer> &generator(renderable.generator);
    const QMatrix4x4 clipSpaceCorrMatrix = rhiCtx->rhi()->clipSpaceCorrMatrix();

    auto renderProperties = generator->getLayerGlobalRenderProperties();
    QSSGRhiShaderPipeline::CommonUniformIndices &cui = shaders->commonUniformIndices;

    const QMatrix4x4 projection = clipSpaceCorrMatrix * inCamera.projection;
    shaders->setUniform(ubufData, "qt_projectionMatrix", projection.constData(), 16 * sizeof(float), &cui.projectionMatrixIdx);

    const QMatrix4x4 viewMatrix = inCamera.globalTransform.inverted();
    shaders->setUniform(ubufData, "qt_viewMatrix", viewMatrix.constData(), 16 * sizeof(float), &cui.viewMatrixIdx);

    const QMatrix4x4 &modelMatrix = renderable.globalTransform;
    shaders->setUniform(ubufData, "qt_modelMatrix", modelMatrix.constData(), 16 * sizeof(float), &cui.modelMatrixIdx);

    QVector2D oneOverSize = QVector2D(1.0f, 1.0f);
    auto &particleBuffer = renderable.particles.m_particleBuffer;
    const int particlesPerSlice = particleBuffer.particlesPerSlice();
    oneOverSize = QVector2D(1.0f / particleBuffer.size().width(), 1.0f / particleBuffer.size().height());
    shaders->setUniform(ubufData, "qt_oneOverParticleImageSize", &oneOverSize, 2 * sizeof(float));
    shaders->setUniform(ubufData, "qt_countPerSlice", &particlesPerSlice, 1 * sizeof(int));

    const QVector4D &color = renderable.particles.m_diffuseColor;
    shaders->setUniform(ubufData, "qt_material_base_color", &color, 4 * sizeof(float), &cui.material_baseColorIdx);

    float imageCount = 1.0f;
    float gradient = 0.0f;
    float blendImages = renderable.particles.m_blendImages ? 1.0f : 0.0f;
    if (renderable.particles.m_spriteImageCount)
        imageCount = float(renderable.particles.m_spriteImageCount);
    else
        gradient = 1.0f;

    float ooImageCount = 1.0f / imageCount;

    QVector4D spriteConfig(imageCount, ooImageCount, gradient, blendImages);
    shaders->setUniform(ubufData, "qt_spriteConfig", &spriteConfig, 4 * sizeof(float));

    const float opacity = renderable.opacity;
    shaders->setUniform(ubufData, "qt_opacity", &opacity, 1 * sizeof(float));
}


static void fillTargetBlend(QRhiGraphicsPipeline::TargetBlend &targetBlend, QSSGRenderParticles::BlendMode mode)
{
    switch (mode) {
    case QSSGRenderParticles::BlendMode::Screen:
        targetBlend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
        targetBlend.dstColor = QRhiGraphicsPipeline::One;
        targetBlend.srcAlpha = QRhiGraphicsPipeline::One;
        targetBlend.dstAlpha = QRhiGraphicsPipeline::One;
        break;
    case QSSGRenderParticles::BlendMode::Multiply:
        targetBlend.srcColor = QRhiGraphicsPipeline::DstColor;
        targetBlend.dstColor = QRhiGraphicsPipeline::Zero;
        targetBlend.srcAlpha = QRhiGraphicsPipeline::One;
        targetBlend.dstAlpha = QRhiGraphicsPipeline::One;
        break;
    default:
        // Source over as default
        targetBlend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
        targetBlend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
        targetBlend.srcAlpha = QRhiGraphicsPipeline::One;
        targetBlend.dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;
        break;

    }
}

void QSSGParticleRenderer::rhiPrepareRenderable(QSSGRef<QSSGRhiShaderPipeline> &shaderPipeline,
                                                QSSGRhiContext *rhiCtx,
                                                QSSGRhiGraphicsPipelineState *ps,
                                                QSSGParticlesRenderable &renderable,
                                                QSSGLayerRenderData &inData,
                                                QRhiRenderPassDescriptor *renderPassDescriptor,
                                                int samples)
{
    const void *layerNode = &inData.layer;
    const void *node = &renderable.particles;

    QSSGRhiDrawCallData &dcd(rhiCtx->drawCallData({ layerNode, node,
                                                    nullptr, 0, QSSGRhiDrawCallDataKey::Main }));
    shaderPipeline->ensureUniformBuffer(&dcd.ubuf);

    char *ubufData = dcd.ubuf->beginFullDynamicBufferUpdateForCurrentFrame();
    updateUniformsForParticles(shaderPipeline, rhiCtx, ubufData, renderable, *inData.camera);
    dcd.ubuf->endFullDynamicBufferUpdateForCurrentFrame();

    QSSGRhiParticleData &particleData = rhiCtx->particleData(&renderable.particles);
    const QSSGParticleBuffer &particleBuffer = renderable.particles.m_particleBuffer;
    int particleCount = particleBuffer.particleCount();
    if (particleData.texture == nullptr || particleData.particleCount != particleCount) {
        QSize size(particleBuffer.size());
        if (!particleData.texture) {
            particleData.texture = rhiCtx->rhi()->newTexture(QRhiTexture::RGBA32F, size);
            particleData.texture->create();
        } else {
            particleData.texture->setPixelSize(size);
            particleData.texture->create();
        }
        particleData.particleCount = particleCount;
    }

    QRhiResourceUpdateBatch *rub = rhiCtx->rhi()->nextResourceUpdateBatch();
    QRhiTextureSubresourceUploadDescription upload;
    upload.setData(particleBuffer.data());
    QRhiTextureUploadDescription uploadDesc(QRhiTextureUploadEntry(0, 0, upload));
    rub->uploadTexture(particleData.texture, uploadDesc);
    rhiCtx->commandBuffer()->resourceUpdate(rub);

    ps->ia.topology = QRhiGraphicsPipeline::TriangleStrip;
    ps->ia.indexBuffer = nullptr;
    ps->ia.vertexBuffer = nullptr;
    ps->ia.inputLayout = QRhiVertexInputLayout();
    ps->ia.inputs.clear();

    ps->samples = samples;
    ps->cullMode = QRhiGraphicsPipeline::None;
    if (renderable.renderableFlags.hasTransparency())
        fillTargetBlend(ps->targetBlend, renderable.particles.m_blendMode);
    else
        ps->targetBlend = QRhiGraphicsPipeline::TargetBlend();

    QSSGRhiShaderResourceBindingList bindings;
    bindings.addUniformBuffer(0, VISIBILITY_ALL, dcd.ubuf, 0, shaderPipeline->ub0Size());

    // Texture maps
    // we only have one image
    QSSGRenderableImage *renderableImage = renderable.firstImage;

    int samplerBinding = shaderPipeline->bindingForTexture("qt_sprite");
    if (samplerBinding >= 0) {
        QRhiTexture *texture = renderableImage ? renderableImage->m_texture.m_texture : nullptr;
        if (samplerBinding >= 0 && texture) {
            const bool mipmapped = texture->flags().testFlag(QRhiTexture::MipMapped);
            QRhiSampler *sampler = rhiCtx->sampler({ toRhi(renderableImage->m_imageNode.m_minFilterType),
                                                     toRhi(renderableImage->m_imageNode.m_magFilterType),
                                                     mipmapped ? toRhi(renderableImage->m_imageNode.m_mipFilterType) : QRhiSampler::None,
                                                     toRhi(renderableImage->m_imageNode.m_horizontalTilingMode),
                                                     toRhi(renderableImage->m_imageNode.m_verticalTilingMode) });
            bindings.addTexture(samplerBinding, QRhiShaderResourceBinding::FragmentStage, texture, sampler);
        } else {
            QRhiResourceUpdateBatch *rub = rhiCtx->rhi()->nextResourceUpdateBatch();
            QRhiTexture *texture = rhiCtx->dummyTexture({}, rub, QSize(4, 4), Qt::white);
            rhiCtx->commandBuffer()->resourceUpdate(rub);
            QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::None,
                                                     QRhiSampler::None,
                                                     QRhiSampler::None,
                                                     QRhiSampler::ClampToEdge,
                                                     QRhiSampler::ClampToEdge });
            bindings.addTexture(samplerBinding, QRhiShaderResourceBinding::FragmentStage, texture, sampler);
        }
    }

    samplerBinding = shaderPipeline->bindingForTexture("qt_particleTexture");
    if (samplerBinding >= 0) {
        QRhiTexture *texture = particleData.texture;
        if (samplerBinding >= 0 && texture) {
            QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Nearest,
                                                     QRhiSampler::Nearest,
                                                     QRhiSampler::None,
                                                     QRhiSampler::ClampToEdge,
                                                     QRhiSampler::ClampToEdge });
            bindings.addTexture(samplerBinding, QRhiShaderResourceBinding::VertexStage, texture, sampler);
        }
    }

    // TODO: This is identical to other renderables. Make into a function?
    QRhiShaderResourceBindings *&srb = dcd.srb;
    bool srbChanged = false;
    if (!srb || bindings != dcd.bindings) {
        srb = rhiCtx->srb(bindings);
        dcd.bindings = bindings;
        srbChanged = true;
    }
    renderable.rhiRenderData.mainPass.srb = srb;

    if (dcd.pipeline
            && !srbChanged
            && dcd.pipelineRpDesc == renderPassDescriptor
            && dcd.ps == *ps)
    {
        renderable.rhiRenderData.mainPass.pipeline = dcd.pipeline;
    } else {
        const QSSGGraphicsPipelineStateKey pipelineKey { *ps, renderPassDescriptor, srb };
        renderable.rhiRenderData.mainPass.pipeline = rhiCtx->pipeline(pipelineKey);
        dcd.pipeline = renderable.rhiRenderData.mainPass.pipeline;
        dcd.pipelineRpDesc = renderPassDescriptor;
        dcd.ps = *ps;
    }
}

void QSSGParticleRenderer::rhiRenderRenderable(QSSGRhiContext *rhiCtx,
                                               QSSGParticlesRenderable &renderable,
                                               QSSGLayerRenderData &inData,
                                               bool *needsSetViewport)
{
    QRhiGraphicsPipeline *ps = renderable.rhiRenderData.mainPass.pipeline;
    QRhiShaderResourceBindings *srb = renderable.rhiRenderData.mainPass.srb;
    if (!ps || !srb)
        return;

    QRhiCommandBuffer *cb = rhiCtx->commandBuffer();
    // QRhi optimizes out unnecessary binding of the same pipline
    cb->setGraphicsPipeline(ps);
    cb->setVertexInput(0, 0, nullptr);
    cb->setShaderResources(srb);

    if (needsSetViewport && *needsSetViewport) {
        cb->setViewport(rhiCtx->graphicsPipelineState(&inData)->viewport);
        *needsSetViewport = false;
    }
    // draw triangle strip with 2 triangles N times
    cb->draw(4, renderable.particles.m_particleBuffer.particleCount());
}

QT_END_NAMESPACE
