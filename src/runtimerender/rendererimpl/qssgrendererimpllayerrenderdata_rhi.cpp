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

QT_BEGIN_NAMESPACE

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

            //shaders->dumpUniforms();

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

            QSSGRhiUniformBufferSet &uniformBuffers(rhiCtx->uniformBufferSet({ layerNode, modelNode }));
            shaderPipeline->bakeMainUniformBuffer(&uniformBuffers.ubuf, resourceUpdates);
            QRhiBuffer *ubuf = uniformBuffers.ubuf;

            QRhiBuffer *lightsUbuf = nullptr;
            if (shaderPipeline->isLightingEnabled()) {
                shaderPipeline->bakeLightsUniformBuffer(&uniformBuffers.lightsUbuf, resourceUpdates);
                lightsUbuf = uniformBuffers.lightsUbuf;
            }

            // this is where vertex, index, and uniform buffer uploads/updates get committed
            cb->resourceUpdate(resourceUpdates);

            QSSGRhiContext::ShaderResourceBindingList bindings;
            const QRhiShaderResourceBinding::StageFlags visibilityAll =
                    QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage;

            bindings.append(QRhiShaderResourceBinding::uniformBuffer(0, visibilityAll, ubuf));

            if (lightsUbuf)
                bindings.append(QRhiShaderResourceBinding::uniformBuffer(1, visibilityAll, lightsUbuf));



            auto *renderableImage = subsetRenderable.firstImage;
            if (renderableImage) { // Textures:

                static int debugCount;
                const bool doDebug = (debugCount++) < 5 && false;

                if (doDebug)
                    qDebug() << "___firstImage___" << subsetRenderable.firstImage;

                int imageNumber = 0;
                auto fragmentShader = ps->shaderStages->fragmentStage()->shader();
                Q_ASSERT(fragmentShader.isValid());
                auto desc =  fragmentShader.description();
                auto samplerVariables = desc.combinedImageSamplers();
                int samplerCount = samplerVariables.count();
                Q_ASSERT(samplerCount > 0);

                while (renderableImage) {
                    if (doDebug)
                        qDebug() << imageNumber << "rhiTexture" << renderableImage->m_image.m_textureData.m_rhiTexture;

                    // ### assuming that samplers are in the same order as images
                    int bindingIndex = qMin(samplerCount, imageNumber);
                    int samplerBinding = samplerVariables[bindingIndex].binding;

                    static QRhiSampler *staticSampler; //### TODO: sampler management
                    if (!staticSampler) {
                        staticSampler = rhiCtx->rhi()->newSampler(QRhiSampler::Linear, QRhiSampler::Linear,
                                                                /*MIPMAP ? QRhiSampler::Linear : */QRhiSampler::None,
                                                                QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge);
                        staticSampler->build();
                    }

                    auto *rhiTex = renderableImage->m_image.m_textureData.m_rhiTexture;
                    if (samplerBinding >= 0 && rhiTex) {
                        if (doDebug)
                            qDebug() << "binding sampledTexture" << samplerBinding << rhiTex << staticSampler;
                        bindings.append(QRhiShaderResourceBinding::sampledTexture(samplerBinding,QRhiShaderResourceBinding::FragmentStage, rhiTex, staticSampler));
                    }

                    renderableImage = renderableImage->m_nextImage;
                    imageNumber++;
                    if (renderableImage && doDebug)
                        qDebug("-----------");
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

void QSSGLayerRenderData::rhiRunPreparePass(TRhiPrepareRenderableFunction inPrepareFn,
                                            bool inEnableBlending,
                                            bool inEnableDepthWrite,
                                            bool inEnableTransparentDepthWrite,
                                            bool inSortOpaqueRenderables,
                                            quint32 indexLight,
                                            const QSSGRenderCamera &inCamera)
{
    QSSGRhiContext *rhiCtx = renderer->context()->rhiContext().data();
    Q_ASSERT(rhiCtx->rhi()->isRecordingFrame());
    QSSGRhiGraphicsPipelineState *ps = rhiCtx->graphicsPipelineState(this);
    QRhiCommandBuffer *cb = rhiCtx->commandBuffer();

    // make the buffer copies and other stuff we put on the command buffer in
    // here show up within a named section in tools like RenderDoc when running
    // with QSG_RHI_PROFILE=1 (which enables debug markers)
    cb->debugMarkBegin(QByteArrayLiteral("Quick3D prepare renderables"));

    ps->depthFunc = QRhiGraphicsPipeline::LessOrEqual;
    ps->blendEnable = false;

    const QVector2D theCameraProps = QVector2D(camera->clipNear, camera->clipFar);
    const auto &theOpaqueObjects = getOpaqueRenderableObjects(inSortOpaqueRenderables);
    bool usingDepthBuffer = /* layer.flags.testFlag(QSSGRenderLayer::Flag::LayerEnableDepthTest) && */ !theOpaqueObjects.isEmpty();

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
                                bool needsSetViewport)
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

        if (needsSetViewport)
            cb->setViewport(rhiCtx->graphicsPipelineState(&inData)->viewport);

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
            rhiRenderRenderable(rhiCtx, *this, *theObject, needsSetViewport);
            needsSetViewport = false;
        }

        const auto &theTransparentObjects = getTransparentRenderableObjects();
        for (const auto &handle : theTransparentObjects) {
            QSSGRenderableObject *theObject = handle.obj;
            if (!theObject->renderableFlags.isCompletelyTransparent()) {
                rhiRenderRenderable(rhiCtx, *this, *theObject, needsSetViewport);
                needsSetViewport = false;
            }
        }

        rhiCtx->commandBuffer()->debugMarkEnd();

        renderer->endLayerRender();
    }
}

QT_END_NAMESPACE
