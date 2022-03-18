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

#include "qssgrendererimpllayerrenderdata_p.h"

#include <QtQuick3DRuntimeRender/private/qssgrenderer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderlight_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrhicustommaterialsystem_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrhiquadrenderer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrhiparticles_p.h>
#include <QtQuick/private/qsgtexture_p.h>
#include <QtQuick/private/qsgrenderer_p.h>

#include <QtCore/QBitArray>
#include <array>
using BoxPoints = std::array<QVector3D, 8>;

QT_BEGIN_NAMESPACE

static constexpr float QSSG_PI = float(M_PI);
static constexpr float QSSG_HALFPI = float(M_PI_2);

static const QRhiShaderResourceBinding::StageFlags VISIBILITY_ALL =
        QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage;

QSSGLayerRenderData::QSSGLayerRenderData(QSSGRenderLayer &inLayer, const QSSGRef<QSSGRenderer> &inRenderer)
    : QSSGLayerRenderPreparationData(inLayer, inRenderer)
    , m_depthBufferFormat(QSSGRenderTextureFormat::Unknown)
    , m_progressiveAAPassIndex(0)
    , m_temporalAAPassIndex(0)
    , m_globalZPrePassActive(false)
{
}

QSSGLayerRenderData::~QSSGLayerRenderData()
{
    m_rhiDepthTexture.reset();
    m_rhiAoTexture.reset();
    m_rhiScreenTexture.reset();
}

void QSSGLayerRenderData::prepareForRender()
{
    QSSGLayerRenderPreparationData::prepareForRender();
    QSSGLayerRenderPreparationResult &thePrepResult(*layerPrepResult);

    if (thePrepResult.flags.wasLayerDataDirty()) {
        m_progressiveAAPassIndex = 0;
    }

    renderer->layerNeedsFrameClear(*this);
}

void QSSGLayerRenderData::resetForFrame()
{
    QSSGLayerRenderPreparationData::resetForFrame();
}

static QSSGRef<QSSGRhiShaderPipeline> shadersForDefaultMaterial(QSSGRhiGraphicsPipelineState *ps,
                                                                QSSGSubsetRenderable &subsetRenderable,
                                                                const QSSGShaderFeatures &featureSet)
{
    const QSSGRef<QSSGRenderer> &generator(subsetRenderable.generator);
    QSSGRef<QSSGRhiShaderPipeline> shaderPipeline = generator->getRhiShaders(subsetRenderable, featureSet);
    if (shaderPipeline)
        ps->shaderPipeline = shaderPipeline.data();
    return shaderPipeline;
}

static QSSGRef<QSSGRhiShaderPipeline> shadersForParticleMaterial(QSSGRhiGraphicsPipelineState *ps,
                                                                 QSSGParticlesRenderable &particleRenderable)
{
    const QSSGRef<QSSGRenderer> &generator(particleRenderable.generator);
    auto featureLevel = particleRenderable.particles.m_featureLevel;
    QSSGRef<QSSGRhiShaderPipeline> shaderPipeline = generator->getRhiParticleShader(featureLevel);
    if (shaderPipeline)
        ps->shaderPipeline = shaderPipeline.data();
    return shaderPipeline;
}

static void updateUniformsForDefaultMaterial(QSSGRef<QSSGRhiShaderPipeline> &shaderPipeline,
                                             QSSGRhiContext *rhiCtx,
                                             char *ubufData,
                                             QSSGRhiGraphicsPipelineState *ps,
                                             QSSGSubsetRenderable &subsetRenderable,
                                             QSSGRenderCamera &camera,
                                             const QVector2D *depthAdjust,
                                             const QMatrix4x4 *alteredModelViewProjection)
{
    const QSSGRef<QSSGRenderer> &generator(subsetRenderable.generator);
    const QMatrix4x4 clipSpaceCorrMatrix = rhiCtx->rhi()->clipSpaceCorrMatrix();
    const QMatrix4x4 &mvp(alteredModelViewProjection ? *alteredModelViewProjection
                                                     : subsetRenderable.modelContext.modelViewProjection);


    const QMatrix4x4 &localInstanceTransform(subsetRenderable.modelContext.model.localInstanceTransform);
    const QMatrix4x4 &globalInstanceTransform(subsetRenderable.modelContext.model.globalInstanceTransform);
    const QMatrix4x4 &modelMatrix(subsetRenderable.modelContext.model.skin ? QMatrix4x4()
                                : !subsetRenderable.modelContext.model.skeleton ? subsetRenderable.globalTransform
                                                : subsetRenderable.modelContext.model.skeleton->globalTransform);

    QSSGMaterialShaderGenerator::setRhiMaterialProperties(*generator->contextInterface(),
                                                          shaderPipeline,
                                                          ubufData,
                                                          ps,
                                                          subsetRenderable.material,
                                                          subsetRenderable.shaderDescription,
                                                          generator->contextInterface()->renderer()->defaultMaterialShaderKeyProperties(),
                                                          camera,
                                                          mvp,
                                                          subsetRenderable.modelContext.normalMatrix,
                                                          modelMatrix,
                                                          clipSpaceCorrMatrix,
                                                          localInstanceTransform,
                                                          globalInstanceTransform,
                                                          subsetRenderable.boneGlobals,
                                                          subsetRenderable.boneNormals,
                                                          subsetRenderable.morphWeights,
                                                          subsetRenderable.firstImage,
                                                          subsetRenderable.opacity,
                                                          generator->getLayerGlobalRenderProperties(),
                                                          subsetRenderable.lights,
                                                          subsetRenderable.reflectionProbe,
                                                          subsetRenderable.renderableFlags.receivesShadows(),
                                                          subsetRenderable.renderableFlags.receivesReflections(),
                                                          depthAdjust);
}

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

static inline void addDepthTextureBindings(QSSGRhiContext *rhiCtx,
                                           QSSGRhiShaderPipeline *shaderPipeline,
                                           QSSGRhiShaderResourceBindingList &bindings)
{
    if (shaderPipeline->depthTexture()) {
        int binding = shaderPipeline->bindingForTexture("qt_depthTexture", int(QSSGRhiSamplerBindingHints::DepthTexture));
        if (binding >= 0) {
             // nearest min/mag, no mipmap
             QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Nearest, QRhiSampler::Nearest, QRhiSampler::None,
                                                      QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge, QRhiSampler::Repeat });
             bindings.addTexture(binding, QRhiShaderResourceBinding::FragmentStage, shaderPipeline->depthTexture(), sampler);
        } // else ignore, not an error
    }

    // SSAO texture
    if (shaderPipeline->ssaoTexture()) {
        int binding = shaderPipeline->bindingForTexture("qt_aoTexture", int(QSSGRhiSamplerBindingHints::AoTexture));
        if (binding >= 0) {
            // linear min/mag, no mipmap
            QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                                     QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge, QRhiSampler::Repeat });
            bindings.addTexture(binding, QRhiShaderResourceBinding::FragmentStage, shaderPipeline->ssaoTexture(), sampler);
        } // else ignore, not an error
    }
}

static void sortInstances(QByteArray &sortedData, QList<QSSGRhiSortData> &sortData, const void *instances,
                          int stride, int count, const QVector3D &cameraDirection)
{
    sortData.resize(count);
    Q_ASSERT(stride == sizeof(QSSGRenderInstanceTableEntry));
    // create sort data
    {
        const QSSGRenderInstanceTableEntry *instance = reinterpret_cast<const QSSGRenderInstanceTableEntry *>(instances);
        for (int i = 0; i < count; i++) {
            const QVector3D pos = QVector3D(instance->row0.w(), instance->row1.w(), instance->row2.w());
            sortData[i] = {QVector3D::dotProduct(pos, cameraDirection), i};
            instance++;
        }
    }

    // sort
    std::sort(sortData.begin(), sortData.end(), [](const QSSGRhiSortData &a, const QSSGRhiSortData &b){
        return a.d > b.d;
    });

    // copy instances
    {
        const QSSGRenderInstanceTableEntry *instance = reinterpret_cast<const QSSGRenderInstanceTableEntry *>(instances);
        QSSGRenderInstanceTableEntry *dest = reinterpret_cast<QSSGRenderInstanceTableEntry *>(sortedData.data());
        for (auto &s : sortData)
            *dest++ = instance[s.indexOrOffset];
    }
}

bool QSSGSubsetRenderable::prepareInstancing(QSSGRhiContext *rhiCtx, const QVector3D &cameraDirection)
{
    if (!modelContext.model.instancing() || instanceBuffer)
        return instanceBuffer;
    auto *table = modelContext.model.instanceTable;
    QSSGRhiInstanceBufferData &instanceData(rhiCtx->instanceBufferData(table));
    qsizetype instanceBufferSize = table->dataSize();
    // Create or resize the instance buffer ### if (instanceData.owned)
    bool sortingChanged = table->isDepthSortingEnabled() != instanceData.sorting;
    bool cameraDirectionChanged = !qFuzzyCompare(instanceData.sortedCameraDirection, cameraDirection);
    bool updateInstanceBuffer = table->serial() != instanceData.serial || sortingChanged || (cameraDirectionChanged && table->isDepthSortingEnabled());
    if (sortingChanged && !table->isDepthSortingEnabled()) {
        instanceData.sortedData.clear();
        instanceData.sortData.clear();
        instanceData.sortedCameraDirection = {};
    }
    instanceData.sorting = table->isDepthSortingEnabled();
    if (instanceData.buffer && instanceData.buffer->size() < instanceBufferSize) {
        updateInstanceBuffer = true;
        //                    qDebug() << "Resizing instance buffer";
        instanceData.buffer->setSize(instanceBufferSize);
        instanceData.buffer->create();
    }
    if (!instanceData.buffer) {
        //                    qDebug() << "Creating instance buffer";
        updateInstanceBuffer = true;
        instanceData.buffer = rhiCtx->rhi()->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::VertexBuffer, instanceBufferSize);
        instanceData.buffer->create();
    }
    if (updateInstanceBuffer) {
        const void *data = nullptr;
        if (table->isDepthSortingEnabled()) {
            QMatrix4x4 invGlobalTransform = modelContext.model.globalTransform.inverted();
            instanceData.sortedData.resize(table->dataSize());
            sortInstances(instanceData.sortedData, instanceData.sortData, table->constData(), table->stride(), table->count(),
                          (invGlobalTransform.mapVector(cameraDirection)).normalized());
            data = instanceData.sortedData.constData();
            instanceData.sortedCameraDirection = cameraDirection;
        } else {
            data = table->constData();
        }
        if (data) {
            QRhiResourceUpdateBatch *rub = rhiCtx->rhi()->nextResourceUpdateBatch();
            rub->updateDynamicBuffer(instanceData.buffer, 0, instanceBufferSize, data);
            rhiCtx->commandBuffer()->resourceUpdate(rub);
            //qDebug() << "****** UPDATING INST BUFFER. Size" << instanceBufferSize;
        } else {
            qWarning() << "NO DATA IN INSTANCE TABLE";
        }
        instanceData.serial = table->serial();
    }
    instanceBuffer = instanceData.buffer;
    return instanceBuffer;
}

static int setupInstancing(QSSGSubsetRenderable *renderable, QSSGRhiGraphicsPipelineState *ps, QSSGRhiContext *rhiCtx, const QVector3D &cameraDirection)
{
    // TODO: non-static so it can be used from QSSGCustomMaterialSystem::rhiPrepareRenderable()?
    const bool instancing = renderable->prepareInstancing(rhiCtx, cameraDirection);
    int instanceBufferBinding = 0;
    if (instancing) {
        // set up new bindings for instanced buffers
        const quint32 stride = renderable->modelContext.model.instanceTable->stride();
        QVarLengthArray<QRhiVertexInputBinding, 8> bindings;
        std::copy(ps->ia.inputLayout.cbeginBindings(), ps->ia.inputLayout.cendBindings(), std::back_inserter(bindings));
        bindings.append({ stride, QRhiVertexInputBinding::PerInstance });
        instanceBufferBinding = bindings.count() - 1;
        ps->ia.inputLayout.setBindings(bindings.cbegin(), bindings.cend());
    }
    return instanceBufferBinding;
}

static void rhiPrepareRenderable(QSSGRhiContext *rhiCtx,
                                 QSSGLayerRenderData &inData,
                                 QSSGRenderableObject &inObject,
                                 QRhiRenderPassDescriptor *renderPassDescriptor,
                                 int samples,
                                 QSSGRenderCamera *inCamera = nullptr,
                                 QMatrix4x4 *alteredModelViewProjection = nullptr,
                                 int cubeFace = -1,
                                 QSSGReflectionMapEntry *entry = nullptr,
                                 QSSGRhiGraphicsPipelineState *inPs = nullptr)
{
    QSSGRenderCamera *camera = inData.camera;
    if (inCamera)
        camera = inCamera;
    QSSGRhiGraphicsPipelineState *ps = rhiCtx->graphicsPipelineState(&inData);
    if (inPs)
        ps = inPs;
    if (inObject.renderableFlags.isDefaultMaterialMeshSubset()) {
        QSSGSubsetRenderable &subsetRenderable(static_cast<QSSGSubsetRenderable &>(inObject));

        QSSGShaderFeatures featureSet(inData.features);

        if (cubeFace < 0 && subsetRenderable.reflectionProbeIndex >= 0 && subsetRenderable.renderableFlags.testFlag(QSSGRenderableObjectFlag::ReceivesReflections))
            featureSet.set(QSSGShaderFeatures::Feature::ReflectionProbe, true);

        if (cubeFace >= 0) {
            // Disable tonemapping for the reflection pass
            featureSet.disableTonemapping();
        }

        QSSGRef<QSSGRhiShaderPipeline> shaderPipeline = shadersForDefaultMaterial(ps, subsetRenderable, featureSet);
        if (shaderPipeline) {
            // Unlike the subsetRenderable (which is allocated per frame so is
            // not persistent in any way), the model reference is persistent in
            // the sense that it references the model node in the scene graph.
            // Combined with the layer node (multiple View3Ds may share the
            // same scene!), this is suitable as a key to get the uniform
            // buffers that were used with the rendering of the same model in
            // the previous frame.
            QSSGRhiShaderResourceBindingList bindings;
            const void *layerNode = &inData.layer;
            const void *modelNode = &subsetRenderable.modelContext.model;
            const bool blendParticles = subsetRenderable.generator->contextInterface()->renderer()->defaultMaterialShaderKeyProperties().m_blendParticles.getValue(subsetRenderable.shaderDescription);

            QSSGRhiDrawCallData &dcd(cubeFace >= 0 ? rhiCtx->drawCallData({ layerNode, modelNode,
                                                                            entry, cubeFace + int(subsetRenderable.subset.offset << 3),
                                                                            QSSGRhiDrawCallDataKey::Reflection })
                                                   : rhiCtx->drawCallData({ layerNode, modelNode,
                                                                            &subsetRenderable.material, 0, QSSGRhiDrawCallDataKey::Main }));

            shaderPipeline->ensureCombinedMainLightsUniformBuffer(&dcd.ubuf);
            char *ubufData = dcd.ubuf->beginFullDynamicBufferUpdateForCurrentFrame();
            updateUniformsForDefaultMaterial(shaderPipeline, rhiCtx, ubufData, ps, subsetRenderable, *camera, nullptr, alteredModelViewProjection);
            if (blendParticles)
                QSSGParticleRenderer::updateUniformsForParticleModel(shaderPipeline, ubufData, &subsetRenderable.modelContext.model, subsetRenderable.subset.offset);
            dcd.ubuf->endFullDynamicBufferUpdateForCurrentFrame();

            if (blendParticles)
                QSSGParticleRenderer::prepareParticlesForModel(shaderPipeline, rhiCtx, bindings, &subsetRenderable.modelContext.model);

            ps->samples = samples;

            ps->cullMode = QSSGRhiGraphicsPipelineState::toCullMode(subsetRenderable.defaultMaterial().cullMode);
            fillTargetBlend(&ps->targetBlend, subsetRenderable.defaultMaterial().blendMode);

            ps->ia = subsetRenderable.subset.rhi.ia;
            QVector3D cameraDirection = inData.cameraDirection;
            if (inCamera)
                cameraDirection = inCamera->getScalingCorrectDirection();
            int instanceBufferBinding = setupInstancing(&subsetRenderable, ps, rhiCtx, cameraDirection);
            ps->ia.bakeVertexInputLocations(*shaderPipeline, instanceBufferBinding);

            bindings.addUniformBuffer(0, VISIBILITY_ALL, dcd.ubuf, 0, shaderPipeline->ub0Size());

            if (shaderPipeline->isLightingEnabled()) {
                bindings.addUniformBuffer(1, VISIBILITY_ALL, dcd.ubuf,
                                          shaderPipeline->ub0LightDataOffset(),
                                          shaderPipeline->ub0LightDataSize());
            }

            // Texture maps
            QSSGRenderableImage *renderableImage = subsetRenderable.firstImage;
            while (renderableImage) {
                const char *samplerName = QSSGMaterialShaderGenerator::getSamplerName(renderableImage->m_mapType);
                const int samplerHint = int(renderableImage->m_mapType);
                int samplerBinding = shaderPipeline->bindingForTexture(samplerName, samplerHint);
                if (samplerBinding >= 0) {
                    QRhiTexture *texture = renderableImage->m_texture.m_texture;
                    if (samplerBinding >= 0 && texture) {
                        const bool mipmapped = texture->flags().testFlag(QRhiTexture::MipMapped);
                        QRhiSampler *sampler = rhiCtx->sampler({ toRhi(renderableImage->m_imageNode.m_minFilterType),
                                                                 toRhi(renderableImage->m_imageNode.m_magFilterType),
                                                                 mipmapped ? toRhi(renderableImage->m_imageNode.m_mipFilterType) : QRhiSampler::None,
                                                                 toRhi(renderableImage->m_imageNode.m_horizontalTilingMode),
                                                                 toRhi(renderableImage->m_imageNode.m_verticalTilingMode),
                                                                 QRhiSampler::Repeat
                                                               });
                        bindings.addTexture(samplerBinding, VISIBILITY_ALL, texture, sampler);
                    }
                } // else this is not necessarily an error, e.g. having metalness/roughness maps with metalness disabled
                renderableImage = renderableImage->m_nextImage;
            }

            if (shaderPipeline->isLightingEnabled()) {
                // Shadow map textures
                const int shadowMapCount = shaderPipeline->shadowMapCount();
                for (int i = 0; i < shadowMapCount; ++i) {
                    QSSGRhiShadowMapProperties &shadowMapProperties(shaderPipeline->shadowMapAt(i));
                    QRhiTexture *texture = shadowMapProperties.shadowMapTexture;
                    QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                                             QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge, QRhiSampler::Repeat });
                    Q_ASSERT(texture && sampler);
                    const QByteArray &name(shadowMapProperties.shadowMapTextureUniformName);
                    if (shadowMapProperties.cachedBinding < 0)
                        shadowMapProperties.cachedBinding = shaderPipeline->bindingForTexture(name);
                    if (shadowMapProperties.cachedBinding < 0) {
                        qWarning("No combined image sampler for shadow map texture '%s'", name.data());
                        continue;
                    }
                    bindings.addTexture(shadowMapProperties.cachedBinding, QRhiShaderResourceBinding::FragmentStage,
                                               texture, sampler);
                }

                // Prioritize reflection texture over Light Probe texture because
                // reflection texture also contains the irradiance and pre filtered
                // values for the light probe.
                if (featureSet.isSet(QSSGShaderFeatures::Feature::ReflectionProbe)) {
                    int reflectionSampler = shaderPipeline->bindingForTexture("qt_reflectionMap");
                    QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::Linear,
                                                             QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge, QRhiSampler::Repeat });
                    QRhiTexture* reflectionTexture = inData.reflectionMapManager->reflectionMapEntry(subsetRenderable.reflectionProbeIndex)->m_rhiPrefilteredCube;
                    if (reflectionSampler >= 0 && reflectionTexture)
                        bindings.addTexture(reflectionSampler, QRhiShaderResourceBinding::FragmentStage, reflectionTexture, sampler);
                } else if (shaderPipeline->lightProbeTexture()) {
                    int binding = shaderPipeline->bindingForTexture("qt_lightProbe", int(QSSGRhiSamplerBindingHints::LightProbe));
                    if (binding >= 0) {
                        auto tiling = shaderPipeline->lightProbeTiling();
                        QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::Linear, // enables mipmapping
                                                                 toRhi(tiling.first), toRhi(tiling.second), QRhiSampler::Repeat });
                        bindings.addTexture(binding, QRhiShaderResourceBinding::FragmentStage,
                                                   shaderPipeline->lightProbeTexture(), sampler);
                    } else {
                        qWarning("Could not find sampler for lightprobe");
                    }
                }

                // Screen Texture
                if (shaderPipeline->screenTexture()) {
                    int binding = shaderPipeline->bindingForTexture("qt_screenTexture", int(QSSGRhiSamplerBindingHints::ScreenTexture));
                    if (binding >= 0) {
                        // linear min/mag, mipmap filtering depends on the
                        // texture, with SCREEN_TEXTURE there are no mipmaps, but
                        // once SCREEN_MIP_TEXTURE is seen the texture (the same
                        // one) has mipmaps generated.
                        QRhiSampler::Filter mipFilter = shaderPipeline->screenTexture()->flags().testFlag(QRhiTexture::MipMapped)
                                ? QRhiSampler::Linear : QRhiSampler::None;
                        QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Linear, QRhiSampler::Linear, mipFilter,
                                                                 QRhiSampler::Repeat, QRhiSampler::Repeat, QRhiSampler::Repeat });
                        bindings.addTexture(binding,
                                            QRhiShaderResourceBinding::FragmentStage,
                                            shaderPipeline->screenTexture(), sampler);
                    } // else ignore, not an error
                }
            }

            // Depth and SSAO textures
            addDepthTextureBindings(rhiCtx, shaderPipeline.data(), bindings);

            // Instead of always doing a QHash find in srb(), store the binding
            // list and the srb object in the per-model+material
            // QSSGRhiUniformBufferSet. While this still needs comparing the
            // binding list, to see if something has changed, it results in
            // significant gains with lots of models in the scene (because the
            // srb hash table becomes large then, so avoiding the lookup as
            // much as possible is helpful)
            QRhiShaderResourceBindings *&srb = dcd.srb;
            bool srbChanged = false;
            if (!srb || bindings != dcd.bindings) {
                srb = rhiCtx->srb(bindings);
                dcd.bindings = bindings;
                srbChanged = true;
            }

            if (cubeFace >= 0)
                subsetRenderable.rhiRenderData.reflectionPass.srb[cubeFace] = srb;
            else
                subsetRenderable.rhiRenderData.mainPass.srb = srb;

            const QSSGGraphicsPipelineStateKey pipelineKey = QSSGGraphicsPipelineStateKey::create(*ps, renderPassDescriptor, srb);
            if (dcd.pipeline
                    && !srbChanged
                    && dcd.renderTargetDescriptionHash == pipelineKey.extra.renderTargetDescriptionHash // we have the hash code anyway, use it to early out upon mismatch
                    && dcd.renderTargetDescription == pipelineKey.renderTargetDescription
                    && dcd.ps == *ps)
            {
                if (cubeFace >= 0)
                    subsetRenderable.rhiRenderData.reflectionPass.pipeline = dcd.pipeline;
                else
                    subsetRenderable.rhiRenderData.mainPass.pipeline = dcd.pipeline;
            } else {
                if (cubeFace >= 0) {
                    subsetRenderable.rhiRenderData.reflectionPass.pipeline = rhiCtx->pipeline(pipelineKey,
                                                                                              renderPassDescriptor,
                                                                                              srb);
                    dcd.pipeline = subsetRenderable.rhiRenderData.reflectionPass.pipeline;
                } else {
                    subsetRenderable.rhiRenderData.mainPass.pipeline = rhiCtx->pipeline(pipelineKey,
                                                                                        renderPassDescriptor,
                                                                                        srb);
                    dcd.pipeline = subsetRenderable.rhiRenderData.mainPass.pipeline;
                }
                dcd.renderTargetDescriptionHash = pipelineKey.extra.renderTargetDescriptionHash;
                dcd.renderTargetDescription = pipelineKey.renderTargetDescription;
                dcd.ps = *ps;
            }
        }
    } else if (inObject.renderableFlags.isCustomMaterialMeshSubset()) {
        QSSGSubsetRenderable &subsetRenderable(static_cast<QSSGSubsetRenderable &>(inObject));
        const QSSGRenderCustomMaterial &material(subsetRenderable.customMaterial());
        QSSGCustomMaterialSystem &customMaterialSystem(*subsetRenderable.generator->contextInterface()->customMaterialSystem().data());

        inData.features.set(QSSGShaderFeatures::Feature::LightProbe, inData.layer.lightProbe || material.m_iblProbe);

        QSSGShaderFeatures featureSet(inData.features);
        if (cubeFace < 0 && subsetRenderable.reflectionProbeIndex >= 0 && subsetRenderable.renderableFlags.testFlag(QSSGRenderableObjectFlag::ReceivesReflections))
            featureSet.set(QSSGShaderFeatures::Feature::ReflectionProbe, true);

        if (cubeFace >= 0) {
            // Disable tonemapping for the reflection pass
            featureSet.disableTonemapping();
        }

        customMaterialSystem.rhiPrepareRenderable(ps, subsetRenderable, featureSet,
                                                  material, inData, renderPassDescriptor, samples,
                                                  inCamera, cubeFace, alteredModelViewProjection, entry);
    } else if (inObject.renderableFlags.isParticlesRenderable()) {
        QSSGParticlesRenderable &particleRenderable(static_cast<QSSGParticlesRenderable &>(inObject));
        QSSGRef<QSSGRhiShaderPipeline> shaderPipeline = shadersForParticleMaterial(ps, particleRenderable);
        if (shaderPipeline) {
            QSSGParticleRenderer::rhiPrepareRenderable(shaderPipeline, rhiCtx, ps, particleRenderable, inData, renderPassDescriptor, samples,
                                                       inCamera, cubeFace, entry);
        }
    } else {
        Q_ASSERT(false);
    }
}

static void addOpaqueDepthPrePassBindings(QSSGRhiContext *rhiCtx,
                                          QSSGRhiShaderPipeline *shaderPipeline,
                                          QSSGRenderableImage *renderableImage,
                                          QSSGRhiShaderResourceBindingList &bindings,
                                          bool isCustomMaterialMeshSubset = false)
{
    static const auto imageAffectsAlpha = [](QSSGRenderableImage::Type mapType) {
        return mapType == QSSGRenderableImage::Type::BaseColor ||
               mapType == QSSGRenderableImage::Type::Diffuse ||
               mapType == QSSGRenderableImage::Type::Translucency ||
               mapType == QSSGRenderableImage::Type::Opacity;
    };

    while (renderableImage) {
        const auto mapType = renderableImage->m_mapType;
        if (imageAffectsAlpha(mapType)) {
            const char *samplerName = QSSGMaterialShaderGenerator::getSamplerName(mapType);
            const int samplerHint = int(mapType);
            int samplerBinding = shaderPipeline->bindingForTexture(samplerName, samplerHint);
            if (samplerBinding >= 0) {
                QRhiTexture *texture = renderableImage->m_texture.m_texture;
                if (samplerBinding >= 0 && texture) {
                    const bool mipmapped = texture->flags().testFlag(QRhiTexture::MipMapped);
                    QRhiSampler *sampler = rhiCtx->sampler({ toRhi(renderableImage->m_imageNode.m_minFilterType),
                                                             toRhi(renderableImage->m_imageNode.m_magFilterType),
                                                             mipmapped ? toRhi(renderableImage->m_imageNode.m_mipFilterType) : QRhiSampler::None,
                                                             toRhi(renderableImage->m_imageNode.m_horizontalTilingMode),
                                                             toRhi(renderableImage->m_imageNode.m_verticalTilingMode),
                                                             QRhiSampler::Repeat
                                                           });
                    bindings.addTexture(samplerBinding, VISIBILITY_ALL, texture, sampler);
                }
            } // else this is not necessarily an error, e.g. having metalness/roughness maps with metalness disabled
        }
        renderableImage = renderableImage->m_nextImage;
    }
    // For custom Materials we can't know which maps affect alpha, so map all
    if (isCustomMaterialMeshSubset) {
        QVector<QShaderDescription::InOutVariable> samplerVars =
                shaderPipeline->fragmentStage()->shader().description().combinedImageSamplers();
        for (const QShaderDescription::InOutVariable &var : shaderPipeline->vertexStage()->shader().description().combinedImageSamplers()) {
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

        if (maxSamplerBinding >= 0) {
            // custom property textures
            int customTexCount = shaderPipeline->extraTextureCount();
            for (int i = 0; i < customTexCount; ++i) {
                const QSSGRhiTexture &t(shaderPipeline->extraTextureAt(i));
                const int samplerBinding = shaderPipeline->bindingForTexture(t.name);
                if (samplerBinding >= 0) {
                    samplerBindingsSpecified.setBit(samplerBinding);
                    QRhiSampler *sampler = rhiCtx->sampler(t.samplerDesc);
                    bindings.addTexture(samplerBinding,
                                        VISIBILITY_ALL,
                                        t.texture,
                                        sampler);
                }
            }
        }

        // use a dummy texture for the unused samplers in the shader
        QRhiSampler *dummySampler = rhiCtx->sampler({ QRhiSampler::Nearest, QRhiSampler::Nearest, QRhiSampler::None,
                                                      QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge, QRhiSampler::Repeat });
        QRhiResourceUpdateBatch *resourceUpdates = rhiCtx->rhi()->nextResourceUpdateBatch();
        QRhiTexture *dummyTexture = rhiCtx->dummyTexture({}, resourceUpdates);
        QRhiTexture *dummyCubeTexture = rhiCtx->dummyTexture(QRhiTexture::CubeMap, resourceUpdates);
        rhiCtx->commandBuffer()->resourceUpdate(resourceUpdates);

        for (const QShaderDescription::InOutVariable &var : samplerVars) {
            if (!samplerBindingsSpecified.testBit(var.binding)) {
                QRhiTexture *t = var.type == QShaderDescription::SamplerCube ? dummyCubeTexture : dummyTexture;
                bindings.addTexture(var.binding, VISIBILITY_ALL, t, dummySampler);
            }
        }
    }
}

static bool rhiPrepareDepthPassForObject(QSSGRhiContext *rhiCtx,
                                         QSSGLayerRenderData &layerData,
                                         QSSGRenderableObject *obj,
                                         QRhiRenderPassDescriptor *rpDesc,
                                         QSSGRhiGraphicsPipelineState *ps,
                                         QSSGRhiDrawCallDataKey::Selector ubufSel)
{
    QSSGRef<QSSGRhiShaderPipeline> shaderPipeline;

    const bool isOpaqueDepthPrePass = obj->depthWriteMode == QSSGDepthDrawMode::OpaquePrePass;
    QSSGShaderFeatures featureSet;
    featureSet.set(QSSGShaderFeatures::Feature::DepthPass, true);
    if (isOpaqueDepthPrePass)
        featureSet.set(QSSGShaderFeatures::Feature::OpaqueDepthPrePass, true);

    QSSGRhiDrawCallData *dcd = nullptr;
    if (obj->renderableFlags.isDefaultMaterialMeshSubset() || obj->renderableFlags.isCustomMaterialMeshSubset()) {
        QSSGSubsetRenderable &subsetRenderable(static_cast<QSSGSubsetRenderable &>(*obj));
        const void *layerNode = &layerData.layer;
        const void *modelNode = &subsetRenderable.modelContext.model;
        dcd = &rhiCtx->drawCallData({ layerNode, modelNode, &subsetRenderable.material, 0, ubufSel });
    }

    if (obj->renderableFlags.isDefaultMaterialMeshSubset()) {
        QSSGSubsetRenderable &subsetRenderable(static_cast<QSSGSubsetRenderable &>(*obj));
        ps->cullMode = QSSGRhiGraphicsPipelineState::toCullMode(subsetRenderable.defaultMaterial().cullMode);

        shaderPipeline = shadersForDefaultMaterial(ps, subsetRenderable, featureSet);
        if (shaderPipeline) {
            shaderPipeline->ensureCombinedMainLightsUniformBuffer(&dcd->ubuf);
            char *ubufData = dcd->ubuf->beginFullDynamicBufferUpdateForCurrentFrame();
            updateUniformsForDefaultMaterial(shaderPipeline, rhiCtx, ubufData, ps, subsetRenderable, *layerData.camera, nullptr, nullptr);
            dcd->ubuf->endFullDynamicBufferUpdateForCurrentFrame();
        } else {
            return false;
        }
    } else if (obj->renderableFlags.isCustomMaterialMeshSubset()) {
        QSSGSubsetRenderable &subsetRenderable(static_cast<QSSGSubsetRenderable &>(*obj));
        ps->cullMode = QSSGRhiGraphicsPipelineState::toCullMode(subsetRenderable.customMaterial().m_cullMode);

        QSSGCustomMaterialSystem &customMaterialSystem(*subsetRenderable.generator->contextInterface()->customMaterialSystem().data());
        shaderPipeline = customMaterialSystem.shadersForCustomMaterial(ps, subsetRenderable.customMaterial(), subsetRenderable, featureSet);

        if (shaderPipeline) {
            shaderPipeline->ensureCombinedMainLightsUniformBuffer(&dcd->ubuf);
            char *ubufData = dcd->ubuf->beginFullDynamicBufferUpdateForCurrentFrame();
            customMaterialSystem.updateUniformsForCustomMaterial(shaderPipeline, rhiCtx, ubufData, ps, subsetRenderable.customMaterial(), subsetRenderable,
                                                                 layerData, *layerData.camera, nullptr, nullptr);
            dcd->ubuf->endFullDynamicBufferUpdateForCurrentFrame();
        } else {
            return false;
        }
    }

    // the rest is common, only relying on QSSGSubsetRenderableBase, not the subclasses
    if (obj->renderableFlags.isDefaultMaterialMeshSubset() || obj->renderableFlags.isCustomMaterialMeshSubset()) {
        QSSGSubsetRenderable &subsetRenderable(static_cast<QSSGSubsetRenderable &>(*obj));
        ps->ia = subsetRenderable.subset.rhi.ia;

        int instanceBufferBinding = setupInstancing(&subsetRenderable, ps, rhiCtx, layerData.cameraDirection);
        ps->ia.bakeVertexInputLocations(*shaderPipeline, instanceBufferBinding);

        QSSGRhiShaderResourceBindingList bindings;
        bindings.addUniformBuffer(0, VISIBILITY_ALL, dcd->ubuf);

        // Depth and SSAO textures, in case a custom material's shader code does something with them.
        addDepthTextureBindings(rhiCtx, shaderPipeline.data(), bindings);

        if (isOpaqueDepthPrePass) {
            addOpaqueDepthPrePassBindings(rhiCtx,
                                          shaderPipeline.data(),
                                          subsetRenderable.firstImage,
                                          bindings,
                                          obj->renderableFlags.isCustomMaterialMeshSubset());
        }

        QRhiShaderResourceBindings *srb = rhiCtx->srb(bindings);

        subsetRenderable.rhiRenderData.depthPrePass.pipeline = rhiCtx->pipeline(QSSGGraphicsPipelineStateKey::create(*ps, rpDesc, srb),
                                                                                rpDesc,
                                                                                srb);
        subsetRenderable.rhiRenderData.depthPrePass.srb = srb;
    }

    return true;
}

static bool rhiPrepareDepthPass(QSSGRhiContext *rhiCtx,
                                const QSSGRhiGraphicsPipelineState &basePipelineState,
                                QRhiRenderPassDescriptor *rpDesc,
                                QSSGLayerRenderData &inData,
                                const QVector<QSSGRenderableObjectHandle> &sortedOpaqueObjects,
                                const QVector<QSSGRenderableObjectHandle> &sortedTransparentObjects,
                                QSSGRhiDrawCallDataKey::Selector ubufSel,
                                int samples)
{
    // Phase 1 (prepare) for the Z prepass or the depth texture generation.
    // These renders opaque (Z prepass), or opaque and transparent (depth
    // texture), objects with depth test/write enabled, and color write
    // disabled, using a very simple set of shaders.

    QSSGRhiGraphicsPipelineState ps = basePipelineState; // viewport and others are filled out already
    // We took a copy of the pipeline state since we do not want to conflict
    // with what rhiPrepare() collects for its own use. So here just change
    // whatever we need.

    ps.samples = samples;
    ps.depthTestEnable = true;
    ps.depthWriteEnable = true;
    ps.targetBlend.colorWrite = {};

    for (const QSSGRenderableObjectHandle &handle : sortedOpaqueObjects) {
        if (!rhiPrepareDepthPassForObject(rhiCtx, inData, handle.obj, rpDesc, &ps, ubufSel))
            return false;
    }

    for (const QSSGRenderableObjectHandle &handle : sortedTransparentObjects) {
        if (!rhiPrepareDepthPassForObject(rhiCtx, inData, handle.obj, rpDesc, &ps, ubufSel))
            return false;
    }

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
        if (!renderableTex->texture->create()) {
            qWarning("Failed to build depth texture (size %dx%d, format %d)",
                     size.width(), size.height(), int(renderableTex->texture->format()));
            renderableTex->reset();
            return false;
        }
        renderableTex->resetRenderTarget();
        QRhiTextureRenderTargetDescription rtDesc;
        rtDesc.setDepthTexture(renderableTex->texture);
        renderableTex->rt = rhi->newTextureRenderTarget(rtDesc);
        renderableTex->rpDesc = renderableTex->rt->newCompatibleRenderPassDescriptor();
        renderableTex->rt->setRenderPassDescriptor(renderableTex->rpDesc);
        if (!renderableTex->rt->create()) {
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
        QSSGSubsetRenderable *subsetRenderable(static_cast<QSSGSubsetRenderable *>(obj));

        QRhiBuffer *vertexBuffer = subsetRenderable->subset.rhi.vertexBuffer->buffer();
        QRhiBuffer *indexBuffer = subsetRenderable->subset.rhi.indexBuffer
                ? subsetRenderable->subset.rhi.indexBuffer->buffer()
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

        QRhiCommandBuffer::VertexInput vertexBuffers[2];
        int vertexBufferCount = 1;
        vertexBuffers[0] = QRhiCommandBuffer::VertexInput(vertexBuffer, 0);
        quint32 instances = 1;
        if (subsetRenderable->modelContext.model.instancing()) {
            instances = subsetRenderable->modelContext.model.instanceCount();
            vertexBuffers[1] = QRhiCommandBuffer::VertexInput(subsetRenderable->instanceBuffer, 0);
            vertexBufferCount = 2;
        }

        if (indexBuffer) {
            cb->setVertexInput(0, vertexBufferCount, vertexBuffers, indexBuffer, 0, subsetRenderable->subset.rhi.indexBuffer->indexFormat());
            cb->drawIndexed(subsetRenderable->subset.count, instances, subsetRenderable->subset.offset);
            QSSGRHICTX_STAT(rhiCtx, drawIndexed(subsetRenderable->subset.count, instances));
        } else {
            cb->setVertexInput(0, vertexBufferCount, vertexBuffers);
            cb->draw(subsetRenderable->subset.count, instances, subsetRenderable->subset.offset);
            QSSGRHICTX_STAT(rhiCtx, draw(subsetRenderable->subset.count, instances));
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

static BoxPoints boundsToBoxPoints(const QSSGBounds3 &bounds)
{
    return BoxPoints { bounds.minimum,
                       QVector3D(bounds.maximum.x(), bounds.minimum.y(), bounds.minimum.z()),
                       QVector3D(bounds.minimum.x(), bounds.maximum.y(), bounds.minimum.z()),
                       QVector3D(bounds.maximum.x(), bounds.maximum.y(), bounds.minimum.z()),
                       QVector3D(bounds.minimum.x(), bounds.minimum.y(), bounds.maximum.z()),
                       QVector3D(bounds.maximum.x(), bounds.minimum.y(), bounds.maximum.z()),
                       QVector3D(bounds.minimum.x(), bounds.maximum.y(), bounds.maximum.z()),
                       bounds.maximum };
}
static std::pair<BoxPoints, BoxPoints> calculateSortedObjectBounds(const QVector<QSSGRenderableObjectHandle> &sortedOpaqueObjects,
                                                                   const QVector<QSSGRenderableObjectHandle> &sortedTransparentObjects)
{
    QSSGBounds3 boundsCasting;
    QSSGBounds3 boundsReceiving;
    for (const auto handles : { &sortedOpaqueObjects, &sortedTransparentObjects }) {
        // Since we may have nodes that are not a child of the camera parent we go through all
        // the opaque objects and include them in the bounds. Failing to do this can result in
        // too small bounds.
        for (const QSSGRenderableObjectHandle &handle : *handles) {
            const QSSGRenderableObject &obj = *handle.obj;

            // We skip objects not casting or receiving shadows since they don't influence or need to be covered by the shadow map
            if (obj.renderableFlags.castsShadows() || obj.renderableFlags.receivesShadows()) {
                QSSGBounds3 bounds;
                const QVector3D &max = obj.bounds.maximum;
                const QVector3D &min = obj.bounds.minimum;

                // Take all corners of the bounding box to make sure the transformed bounding box is big enough
                bounds.include(obj.globalTransform.map(min));
                bounds.include(obj.globalTransform.map(QVector3D(max.x(), min.y(), min.z())));
                bounds.include(obj.globalTransform.map(QVector3D(min.x(), max.y(), min.z())));
                bounds.include(obj.globalTransform.map(QVector3D(max.x(), max.y(), min.z())));
                bounds.include(obj.globalTransform.map(QVector3D(min.x(), min.y(), max.z())));
                bounds.include(obj.globalTransform.map(QVector3D(max.x(), min.y(), max.z())));
                bounds.include(obj.globalTransform.map(QVector3D(min.x(), max.y(), max.z())));
                bounds.include(obj.globalTransform.map(max));
                // Model particles are in world space
                bounds.include(obj.particleBounds);

                if (obj.renderableFlags.castsShadows()) {
                    boundsCasting.include(bounds);
                }
                if (obj.renderableFlags.receivesShadows()) {
                    boundsReceiving.include(bounds);
                }
            }
        }
    }

    return { boundsToBoxPoints(boundsCasting), boundsToBoxPoints(boundsReceiving) };
}

static QVector3D calcCenter(const BoxPoints &vertices)
{
    QVector3D center = vertices[0];
    for (int i = 1; i < 8; ++i) {
        center += vertices[i];
    }
    return center * 0.125f;
}

static QSSGBounds3 calculateShadowCameraBoundingBox(const BoxPoints &points, const QVector3D &forward, const QVector3D &up, const QVector3D &right)
{
    QSSGBounds3 bounds;
    for (int i = 0; i < 8; ++i) {
        const float distanceZ = QVector3D::dotProduct(points[i], forward);
        const float distanceY = QVector3D::dotProduct(points[i], up);
        const float distanceX = QVector3D::dotProduct(points[i], right);
        bounds.include(QVector3D(distanceX, distanceY, distanceZ));
    }
    return bounds;
}

static BoxPoints computeFrustumBounds(const QSSGRenderCamera &inCamera)
{
    QMatrix4x4 viewProjection;
    inCamera.calculateViewProjectionMatrix(viewProjection);

    bool invertible = false;
    QMatrix4x4 inv = viewProjection.inverted(&invertible);
    Q_ASSERT(invertible);

    return BoxPoints { inv.map(QVector3D(-1, -1, -1)), inv.map(QVector3D(+1, -1, -1)), inv.map(QVector3D(+1, +1, -1)),
                       inv.map(QVector3D(-1, +1, -1)), inv.map(QVector3D(-1, -1, +1)), inv.map(QVector3D(+1, -1, +1)),
                       inv.map(QVector3D(+1, +1, +1)), inv.map(QVector3D(-1, +1, +1)) };
}

static void setupCubeReflectionCameras(const QSSGRenderReflectionProbe *inProbe, QSSGRenderCamera inCameras[6])
{
    Q_ASSERT(inProbe != nullptr);

    // setup light matrix
    quint32 mapRes = 1 << inProbe->reflectionMapRes;
    QRectF theViewport(0.0f, 0.0f, (float)mapRes, (float)mapRes);
    static const QQuaternion rotOfs[6] {
        QQuaternion::fromEulerAngles(0.f, qRadiansToDegrees(-QSSG_HALFPI), qRadiansToDegrees(QSSG_PI)),
        QQuaternion::fromEulerAngles(0.f, qRadiansToDegrees(QSSG_HALFPI), qRadiansToDegrees(QSSG_PI)),
        QQuaternion::fromEulerAngles(qRadiansToDegrees(QSSG_HALFPI), 0.f, 0.f),
        QQuaternion::fromEulerAngles(qRadiansToDegrees(-QSSG_HALFPI), 0.f, 0.f),
        QQuaternion::fromEulerAngles(0.f, qRadiansToDegrees(QSSG_PI), qRadiansToDegrees(-QSSG_PI)),
        QQuaternion::fromEulerAngles(0.f, 0.f, qRadiansToDegrees(QSSG_PI)),
    };

    const QVector3D inProbePos = inProbe->getGlobalPos();

    for (int i = 0; i < 6; ++i) {
        inCameras[i].parent = nullptr;
        inCameras[i].pivot = inProbe->pivot;
        inCameras[i].clipNear = 1.0f;
        inCameras[i].clipFar = qMax<float>(2.0f, 10000.0f);
        inCameras[i].fov = qDegreesToRadians(90.f);

        inCameras[i].position = inProbePos;
        inCameras[i].rotation = rotOfs[i];
        inCameras[i].calculateGlobalVariables(theViewport);
    }
}

static void setupCameraForShadowMap(const QSSGRenderCamera &inCamera,
                                    const QSSGRenderLight *inLight,
                                    QSSGRenderCamera &theCamera,
                                    const BoxPoints &castingBox,
                                    const BoxPoints &receivingBox)
{
    // setup light matrix
    quint32 mapRes = 1 << inLight->m_shadowMapRes;
    QRectF theViewport(0.0f, 0.0f, (float)mapRes, (float)mapRes);
    theCamera.clipNear = 1.0f;
    theCamera.clipFar = inLight->m_shadowMapFar;
    // Setup camera projection
    QVector3D inLightPos = inLight->getGlobalPos();
    QVector3D inLightDir = inLight->getDirection();

    inLightPos -= inLightDir * inCamera.clipNear;
    theCamera.fov = qDegreesToRadians(90.f);
    theCamera.parent = nullptr;
    theCamera.pivot = inLight->pivot;

    if (inLight->type == QSSGRenderLight::Type::DirectionalLight) {
        Q_ASSERT(theCamera.type == QSSGRenderCamera::Type::OrthographicCamera);
        const QVector3D forward = inLightDir.normalized();
        const QVector3D right = qFuzzyCompare(qAbs(forward.y()), 1.0f)
                ? QVector3D::crossProduct(forward, QVector3D(1, 0, 0)).normalized()
                : QVector3D::crossProduct(forward, QVector3D(0, 1, 0)).normalized();
        const QVector3D up = QVector3D::crossProduct(right, forward).normalized();

        // Calculate bounding box of the scene camera frustum
        const BoxPoints frustumPoints = computeFrustumBounds(inCamera);
        const QSSGBounds3 frustumBounds = calculateShadowCameraBoundingBox(frustumPoints, forward, up, right);
        const QSSGBounds3 sceneCastingBounds = calculateShadowCameraBoundingBox(castingBox, forward, up, right);

        QVector3D finalDims;
        QVector3D center;
        // Select smallest bounds from either scene or camera frustum
        if (sceneCastingBounds.isFinite() // handle empty scene
            && sceneCastingBounds.extents().lengthSquared() < frustumBounds.extents().lengthSquared()) {
            center = calcCenter(castingBox);
            const QSSGBounds3 boundsReceiving = calculateShadowCameraBoundingBox(receivingBox, forward, up, right);
            const QVector3D centerReceiving = calcCenter(receivingBox);

            // Since we need to make sure every rendered geometry can get a valid depth value from the shadow map
            // we need to expand the scene bounding box along its z-axis so that it covers also receiving objects in the scene.
            //
            // We take the z dimensions of the casting bounds and expand it to include the z dimensions of the receiving objects.
            // We call the casting bounding box 'a' and the receiving bounding box 'b'.

            // length of boxes
            const float aLength = sceneCastingBounds.dimensions().z();
            const float bLength = boundsReceiving.dimensions().z();

            // center position of boxes
            const float aCenter = QVector3D::dotProduct(center, forward);
            const float bCenter = QVector3D::dotProduct(centerReceiving, forward);

            // distance between boxes
            const float d = bCenter - aCenter;

            // start/end positions
            const float a0 = 0.f;
            const float a1 = aLength;
            const float b0 = (aLength * 0.5f) + d - (bLength * 0.5f);
            const float b1 = (aLength * 0.5f) + d + (bLength * 0.5f);

            // goal start/end position
            const float ap0 = qMin(a0, b0);
            const float ap1 = qMax(a1, b1);
            // goal length
            const float length = ap1 - ap0;
            // goal center postion
            const float c = (ap1 + ap0) * 0.5f;

            // how much to move in forward direction
            const float move = c - aLength * 0.5f;

            center = center + forward * move;
            finalDims = sceneCastingBounds.dimensions();
            finalDims.setZ(length);
        } else {
            center = calcCenter(frustumPoints);
            finalDims = frustumBounds.dimensions();
        }

        // Expand dimensions a little bit to avoid precision problems
        finalDims *= 1.05f;

        // Apply bounding box parameters to shadow map camera projection matrix
        // so that the whole scene is fit inside the shadow map
        theViewport.setHeight(finalDims.y());
        theViewport.setWidth(finalDims.x());
        theCamera.clipNear = -0.5f * finalDims.z();
        theCamera.clipFar = 0.5f * finalDims.z();

        theCamera.position = center;
        theCamera.rotation = QQuaternion::fromDirection(forward, up);
    } else if (inLight->type == QSSGRenderLight::Type::PointLight) {
        theCamera.lookAt(inLightPos, QVector3D(0, 1.0, 0), QVector3D(0, 0, 0));
    }

    theCamera.calculateGlobalVariables(theViewport);
}

static void setupCubeShadowCameras(const QSSGRenderLight *inLight, QSSGRenderCamera inCameras[6])
{
    Q_ASSERT(inLight != nullptr);
    Q_ASSERT(inLight->type != QSSGRenderLight::Type::DirectionalLight);

    // setup light matrix
    quint32 mapRes = 1 << inLight->m_shadowMapRes;
    QRectF theViewport(0.0f, 0.0f, (float)mapRes, (float)mapRes);
    static const QQuaternion rotOfs[6] {
        QQuaternion::fromEulerAngles(0.f, qRadiansToDegrees(-QSSG_HALFPI), qRadiansToDegrees(QSSG_PI)),
        QQuaternion::fromEulerAngles(0.f, qRadiansToDegrees(QSSG_HALFPI), qRadiansToDegrees(QSSG_PI)),
        QQuaternion::fromEulerAngles(qRadiansToDegrees(QSSG_HALFPI), 0.f, 0.f),
        QQuaternion::fromEulerAngles(qRadiansToDegrees(-QSSG_HALFPI), 0.f, 0.f),
        QQuaternion::fromEulerAngles(0.f, qRadiansToDegrees(QSSG_PI), qRadiansToDegrees(-QSSG_PI)),
        QQuaternion::fromEulerAngles(0.f, 0.f, qRadiansToDegrees(QSSG_PI)),
    };

    const QVector3D inLightPos = inLight->getGlobalPos();

    for (int i = 0; i < 6; ++i) {
        inCameras[i].parent = nullptr;
        inCameras[i].pivot = inLight->pivot;
        inCameras[i].clipNear = 1.0f;
        inCameras[i].clipFar = qMax<float>(2.0f, inLight->m_shadowMapFar);
        inCameras[i].fov = qDegreesToRadians(90.f);

        inCameras[i].position = inLightPos;
        inCameras[i].rotation = rotOfs[i];
        inCameras[i].calculateGlobalVariables(theViewport);
    }

    /*
        if ( inLight->type == RenderLightTypes::Point ) return;

        QVector3D viewDirs[6];
        QVector3D viewUp[6];
        QMatrix3x3 theDirMatrix( inLight->m_GlobalTransform.getUpper3x3() );

        viewDirs[0] = theDirMatrix.transform( QVector3D( 1.f, 0.f, 0.f ) );
        viewDirs[2] = theDirMatrix.transform( QVector3D( 0.f, -1.f, 0.f ) );
        viewDirs[4] = theDirMatrix.transform( QVector3D( 0.f, 0.f, 1.f ) );
        viewDirs[0].normalize();  viewDirs[2].normalize();  viewDirs[4].normalize();
        viewDirs[1] = -viewDirs[0];
        viewDirs[3] = -viewDirs[2];
        viewDirs[5] = -viewDirs[4];

        viewUp[0] = viewDirs[2];
        viewUp[1] = viewDirs[2];
        viewUp[2] = viewDirs[5];
        viewUp[3] = viewDirs[4];
        viewUp[4] = viewDirs[2];
        viewUp[5] = viewDirs[2];

        for (int i = 0; i < 6; ++i)
        {
                inCameras[i].LookAt( inLightPos, viewUp[i], inLightPos + viewDirs[i] );
                inCameras[i].CalculateGlobalVariables( theViewport, QVector2D( theViewport.m_Width,
        theViewport.m_Height ) );
        }
        */
}

static void rhiPrepareSkyBox(QSSGRhiContext *rhiCtx,
                             QSSGRenderLayer &layer,
                             QSSGRenderCamera &inCamera,
                             const QSSGRef<QSSGRenderer> &renderer,
                             QSSGReflectionMapEntry *entry = nullptr,
                             int cubeFace = -1)
{
    QRhiCommandBuffer *cb = rhiCtx->commandBuffer();
    cb->debugMarkBegin(QByteArrayLiteral("Quick3D prepare skybox"));

    const QSSGRenderImageTexture lightProbeTexture = renderer->contextInterface()->bufferManager()->loadRenderImage(layer.lightProbe,
                                                                                                                    QSSGBufferManager::MipModeBsdf);
    const bool hasValidTexture = lightProbeTexture.m_texture != nullptr;
    if (hasValidTexture) {
        if (cubeFace < 0)
            layer.skyBoxIsRgbe8 = lightProbeTexture.m_flags.isRgbe8();

        QSSGRhiShaderResourceBindingList bindings;

        QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::Linear, //We have mipmaps
                                                 QRhiSampler::Repeat, QRhiSampler::ClampToEdge, QRhiSampler::Repeat });
        int samplerBinding = 1; //the shader code is hand-written, so we don't need to look that up
        const int ubufSize = 2 * 4 * 3 * sizeof(float) + 4 * 4 * sizeof(float) + 2 * sizeof(float); // 2x mat3 + mat4 + 2 floats
        bindings.addTexture(samplerBinding,
                            QRhiShaderResourceBinding::FragmentStage,
                            lightProbeTexture.m_texture, sampler);

        QSSGRhiDrawCallData &dcd(cubeFace >= 0 ? rhiCtx->drawCallData({ &layer, nullptr, entry, cubeFace, QSSGRhiDrawCallDataKey::Reflection })
                                               : rhiCtx->drawCallData({ &layer, nullptr, nullptr, 0, QSSGRhiDrawCallDataKey::SkyBox }));

        QRhi *rhi = rhiCtx->rhi();
        if (!dcd.ubuf) {
            dcd.ubuf = rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, ubufSize);
            dcd.ubuf->create();
        }

        const QMatrix4x4 &inverseProjection = inCamera.projection.inverted();
        const QMatrix4x4 &viewMatrix = inCamera.globalTransform;
        float adjustY = rhi->isYUpInNDC() ? 1.0f : -1.0f;
        const float exposure = layer.probeExposure;
        // orientation
        const QMatrix3x3 &rotationMatrix(layer.probeOrientation);
        const float blurAmount = layer.skyboxBlurAmount;
        const float maxMipLevel = float(lightProbeTexture.m_mipmapCount - 2);

        const QVector4D skyboxProperties = {
            adjustY,
            exposure,
            blurAmount,
            maxMipLevel
        };

        char *ubufData = dcd.ubuf->beginFullDynamicBufferUpdateForCurrentFrame();
        memcpy(ubufData, viewMatrix.constData(), 44);
        memcpy(ubufData + 48, inverseProjection.constData(), 64);
        memcpy(ubufData + 112, rotationMatrix.constData(), 12);
        memcpy(ubufData + 128, (char *)rotationMatrix.constData() + 12, 12);
        memcpy(ubufData + 144, (char *)rotationMatrix.constData() + 24, 12);
        memcpy(ubufData + 160, &skyboxProperties, 16);
        dcd.ubuf->endFullDynamicBufferUpdateForCurrentFrame();

        bindings.addUniformBuffer(0, VISIBILITY_ALL, dcd.ubuf);

        if (cubeFace >= 0)
            entry->m_skyBoxSrbs[cubeFace] = rhiCtx->srb(bindings);
        else
            layer.skyBoxSrb = rhiCtx->srb(bindings);

        renderer->rhiQuadRenderer()->prepareQuad(rhiCtx, nullptr);
    }

    cb->debugMarkEnd();
}

static void rhiPrepareResourcesForReflectionMap(QSSGRhiContext *rhiCtx,
                                                QSSGLayerRenderData &inData,
                                                QSSGReflectionMapEntry *pEntry,
                                                QSSGRhiGraphicsPipelineState *ps,
                                                const QVector<QSSGRenderableObjectHandle> &sortedOpaqueObjects,
                                                QSSGRenderCamera &inCamera,
                                                const QSSGRef<QSSGRenderer> &renderer,
                                                int cubeFace)
{
    if (inData.layer.background == QSSGRenderLayer::Background::SkyBox && inData.layer.lightProbe)
        rhiPrepareSkyBox(rhiCtx, inData.layer, inCamera, renderer, pEntry, cubeFace);

    for (const auto &handle : sortedOpaqueObjects) {
        QSSGRenderableObject &inObject = *handle.obj;

        QMatrix4x4 modelViewProjection;
        if (inObject.renderableFlags.isDefaultMaterialMeshSubset() || inObject.renderableFlags.isCustomMaterialMeshSubset()) {
            QSSGSubsetRenderable &renderable(static_cast<QSSGSubsetRenderable &>(inObject));
            modelViewProjection = pEntry->m_viewProjection * renderable.globalTransform;
        }

        rhiPrepareRenderable(rhiCtx, inData, inObject, pEntry->m_rhiRenderPassDesc, 1,
                             &inCamera, &modelViewProjection, cubeFace, pEntry, ps);
    }
}

static void rhiPrepareResourcesForShadowMap(QSSGRhiContext *rhiCtx,
                                            QSSGLayerRenderData &inData,
                                            QSSGShadowMapEntry *pEntry,
                                            QSSGRhiGraphicsPipelineState *ps,
                                            const QVector2D *depthAdjust,
                                            const QVector<QSSGRenderableObjectHandle> &sortedOpaqueObjects,
                                            QSSGRenderCamera &inCamera,
                                            bool orthographic,
                                            int cubeFace)
{
    QSSGShaderFeatures featureSet;
    if (orthographic)
        featureSet.set(QSSGShaderFeatures::Feature::OrthoShadowPass, true);
    else
        featureSet.set(QSSGShaderFeatures::Feature::CubeShadowPass, true);

    for (const auto &handle : sortedOpaqueObjects) {
        QSSGRenderableObject *theObject = handle.obj;
        if (!theObject->renderableFlags.castsShadows())
            continue;

        QSSGShaderFeatures objectFeatureSet = featureSet;
        const bool isOpaqueDepthPrePass = theObject->depthWriteMode == QSSGDepthDrawMode::OpaquePrePass;
        if (isOpaqueDepthPrePass)
            objectFeatureSet.set(QSSGShaderFeatures::Feature::OpaqueDepthPrePass, true);

        QSSGRhiDrawCallData *dcd = nullptr;
        QMatrix4x4 modelViewProjection;
        if (theObject->renderableFlags.isDefaultMaterialMeshSubset() || theObject->renderableFlags.isCustomMaterialMeshSubset()) {
            QSSGSubsetRenderable *renderable(static_cast<QSSGSubsetRenderable *>(theObject));
            modelViewProjection = pEntry->m_lightVP * renderable->globalTransform;
            dcd = &rhiCtx->drawCallData({ &inData.layer, &renderable->modelContext.model,
                                          pEntry, cubeFace + int(renderable->subset.offset << 3), QSSGRhiDrawCallDataKey::Shadow });
        }

        QSSGRhiShaderResourceBindingList bindings;
        QSSGRef<QSSGRhiShaderPipeline> shaderPipeline;
        QSSGSubsetRenderable &subsetRenderable(static_cast<QSSGSubsetRenderable &>(*theObject));
        if (theObject->renderableFlags.isDefaultMaterialMeshSubset()) {
            ps->cullMode = QSSGRhiGraphicsPipelineState::toCullMode(subsetRenderable.defaultMaterial().cullMode);
            const bool blendParticles = subsetRenderable.generator->contextInterface()->renderer()->defaultMaterialShaderKeyProperties().m_blendParticles.getValue(subsetRenderable.shaderDescription);

            shaderPipeline = shadersForDefaultMaterial(ps, subsetRenderable, objectFeatureSet);
            if (!shaderPipeline)
                continue;
            shaderPipeline->ensureCombinedMainLightsUniformBuffer(&dcd->ubuf);
            char *ubufData = dcd->ubuf->beginFullDynamicBufferUpdateForCurrentFrame();
            updateUniformsForDefaultMaterial(shaderPipeline, rhiCtx, ubufData, ps, subsetRenderable, inCamera, depthAdjust, &modelViewProjection);
            if (blendParticles)
                QSSGParticleRenderer::updateUniformsForParticleModel(shaderPipeline, ubufData, &subsetRenderable.modelContext.model, subsetRenderable.subset.offset);
            dcd->ubuf->endFullDynamicBufferUpdateForCurrentFrame();
            if (blendParticles)
                QSSGParticleRenderer::prepareParticlesForModel(shaderPipeline, rhiCtx, bindings, &subsetRenderable.modelContext.model);
        } else if (theObject->renderableFlags.isCustomMaterialMeshSubset()) {
            ps->cullMode = QSSGRhiGraphicsPipelineState::toCullMode(subsetRenderable.customMaterial().m_cullMode);

            QSSGCustomMaterialSystem &customMaterialSystem(*subsetRenderable.generator->contextInterface()->customMaterialSystem().data());
            shaderPipeline = customMaterialSystem.shadersForCustomMaterial(ps, subsetRenderable.customMaterial(), subsetRenderable, objectFeatureSet);
            if (!shaderPipeline)
                continue;
            shaderPipeline->ensureCombinedMainLightsUniformBuffer(&dcd->ubuf);
            char *ubufData = dcd->ubuf->beginFullDynamicBufferUpdateForCurrentFrame();
            // inCamera is the shadow camera, not the same as inData.camera
            customMaterialSystem.updateUniformsForCustomMaterial(shaderPipeline, rhiCtx, ubufData, ps, subsetRenderable.customMaterial(), subsetRenderable,
                                                                 inData, inCamera, depthAdjust, &modelViewProjection);
            dcd->ubuf->endFullDynamicBufferUpdateForCurrentFrame();
        }

        if (theObject->renderableFlags.isDefaultMaterialMeshSubset() || theObject->renderableFlags.isCustomMaterialMeshSubset()) {

            ps->shaderPipeline = shaderPipeline.data();
            ps->ia = subsetRenderable.subset.rhi.ia;
            int instanceBufferBinding = setupInstancing(&subsetRenderable, ps, rhiCtx, inData.cameraDirection);
            ps->ia.bakeVertexInputLocations(*shaderPipeline, instanceBufferBinding);


            bindings.addUniformBuffer(0, VISIBILITY_ALL, dcd->ubuf);

            // Depth and SSAO textures, in case a custom material's shader code does something with them.
            addDepthTextureBindings(rhiCtx, shaderPipeline.data(), bindings);

            if (isOpaqueDepthPrePass) {
                addOpaqueDepthPrePassBindings(rhiCtx,
                                              shaderPipeline.data(),
                                              subsetRenderable.firstImage,
                                              bindings,
                                              theObject->renderableFlags.isCustomMaterialMeshSubset());
            }

            // There is no screen texture at this stage. But the shader from a
            // custom material may rely on it, and an object with that material
            // can end up in the shadow map's object list. So bind a dummy
            // texture then due to the lack of other options.
            int binding = shaderPipeline->bindingForTexture("qt_screenTexture", int(QSSGRhiSamplerBindingHints::ScreenTexture));
            if (binding >= 0) {
                QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Nearest, QRhiSampler::Nearest, QRhiSampler::None,
                                                         QRhiSampler::Repeat, QRhiSampler::Repeat, QRhiSampler::Repeat });
                QRhiResourceUpdateBatch *resourceUpdates = rhiCtx->rhi()->nextResourceUpdateBatch();
                QRhiTexture *dummyTexture = rhiCtx->dummyTexture({}, resourceUpdates);
                rhiCtx->commandBuffer()->resourceUpdate(resourceUpdates);
                bindings.addTexture(binding,
                                    QRhiShaderResourceBinding::FragmentStage,
                                    dummyTexture, sampler);
            }

            QRhiShaderResourceBindings *srb = rhiCtx->srb(bindings);
            subsetRenderable.rhiRenderData.shadowPass.pipeline = rhiCtx->pipeline(QSSGGraphicsPipelineStateKey::create(*ps, pEntry->m_rhiRenderPassDesc, srb),
                                                                                  pEntry->m_rhiRenderPassDesc,
                                                                                  srb);
            subsetRenderable.rhiRenderData.shadowPass.srb[cubeFace] = srb;
        }
    }
}

static void rhiRenderRenderable(QSSGRhiContext *rhiCtx,
                                QSSGLayerRenderData &inData,
                                QSSGRenderableObject &object,
                                bool *needsSetViewport,
                                int cubeFace = -1,
                                QSSGRhiGraphicsPipelineState *state = nullptr);

static void rhiRenderOneShadowMap(QSSGRhiContext *rhiCtx,
                                  QSSGRhiGraphicsPipelineState *ps,
                                  const QVector<QSSGRenderableObjectHandle> &sortedOpaqueObjects,
                                  int cubeFace)
{
    QRhiCommandBuffer *cb = rhiCtx->commandBuffer();
    bool needsSetViewport = true;

    for (const auto &handle : sortedOpaqueObjects) {
        QSSGRenderableObject *theObject = handle.obj;
        if (!theObject->renderableFlags.castsShadows())
            continue;

        if (theObject->renderableFlags.isDefaultMaterialMeshSubset() || theObject->renderableFlags.isCustomMaterialMeshSubset()) {
            QSSGSubsetRenderable *renderable(static_cast<QSSGSubsetRenderable *>(theObject));

            QRhiBuffer *vertexBuffer = renderable->subset.rhi.vertexBuffer->buffer();
            QRhiBuffer *indexBuffer = renderable->subset.rhi.indexBuffer
                    ? renderable->subset.rhi.indexBuffer->buffer()
                    : nullptr;

            // Ideally we shouldn't need to deal with this, as only "valid" objects should be processed at this point.
            if (!renderable->rhiRenderData.shadowPass.pipeline)
                continue;

            cb->setGraphicsPipeline(renderable->rhiRenderData.shadowPass.pipeline);

            QRhiShaderResourceBindings *srb = renderable->rhiRenderData.shadowPass.srb[cubeFace];
            cb->setShaderResources(srb);

            if (needsSetViewport) {
                cb->setViewport(ps->viewport);
                needsSetViewport = false;
            }

            QRhiCommandBuffer::VertexInput vertexBuffers[2];
            int vertexBufferCount = 1;
            vertexBuffers[0] = QRhiCommandBuffer::VertexInput(vertexBuffer, 0);
            quint32 instances = 1;
            if (renderable->modelContext.model.instancing()) {
                instances = renderable->modelContext.model.instanceCount();
                vertexBuffers[1] = QRhiCommandBuffer::VertexInput(renderable->instanceBuffer, 0);
                vertexBufferCount = 2;
            }
            if (indexBuffer) {
                cb->setVertexInput(0, vertexBufferCount, vertexBuffers, indexBuffer, 0, renderable->subset.rhi.indexBuffer->indexFormat());
                cb->drawIndexed(renderable->subset.count, instances, renderable->subset.offset);
                QSSGRHICTX_STAT(rhiCtx, drawIndexed(renderable->subset.count, instances));
            } else {
                cb->setVertexInput(0, vertexBufferCount, vertexBuffers);
                cb->draw(renderable->subset.count, instances, renderable->subset.offset);
                QSSGRHICTX_STAT(rhiCtx, draw(renderable->subset.count, instances));
            }
        }
    }
}

static void rhiBlurShadowMap(QSSGRhiContext *rhiCtx,
                             QSSGShadowMapEntry *pEntry,
                             const QSSGRef<QSSGRenderer> &renderer,
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

    QSSGRef<QSSGRhiShaderPipeline> shaderPipeline = orthographic ? renderer->getRhiOrthographicShadowBlurXShader()
                                                               : renderer->getRhiCubemapShadowBlurXShader();
    if (!shaderPipeline)
        return;
    ps.shaderPipeline = shaderPipeline.data();

    ps.colorAttachmentCount = orthographic ? 1 : 6;

    // construct a key that is unique for this frame (we use a dynamic buffer
    // so even if the same key gets used in the next frame, just updating the
    // contents on the same QRhiBuffer is ok due to QRhi's internal double buffering)
    QSSGRhiDrawCallData &dcd = rhiCtx->drawCallData({ map, nullptr, nullptr, 0, QSSGRhiDrawCallDataKey::ShadowBlur });
    if (!dcd.ubuf) {
        dcd.ubuf = rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64 + 8);
        dcd.ubuf->create();
    }

    // the blur also needs Y reversed in order to get correct results (while
    // the second blur step would end up with the correct orientation without
    // this too, but we need to blur the correct fragments in the second step
    // hence the flip is important)
    QMatrix4x4 flipY;
    // correct for D3D and Metal but not for Vulkan because there the Y is down
    // in NDC so that kind of self-corrects...
    if (rhi->isYUpInFramebuffer() != rhi->isYUpInNDC())
        flipY.data()[5] = -1.0f;
    float cameraProperties[2] = { shadowFilter, shadowMapFar };
    char *ubufData = dcd.ubuf->beginFullDynamicBufferUpdateForCurrentFrame();
    memcpy(ubufData, flipY.constData(), 64);
    memcpy(ubufData + 64, cameraProperties, 8);
    dcd.ubuf->endFullDynamicBufferUpdateForCurrentFrame();

    QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                             QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge, QRhiSampler::Repeat });
    Q_ASSERT(sampler);

    QSSGRhiShaderResourceBindingList bindings;
    bindings.addUniformBuffer(0, VISIBILITY_ALL, dcd.ubuf);
    bindings.addTexture(1, QRhiShaderResourceBinding::FragmentStage, map, sampler);
    QRhiShaderResourceBindings *srb = rhiCtx->srb(bindings);

    QSSGRhiQuadRenderer::Flags quadFlags;
    if (orthographic) // orthoshadowshadowblurx and y have attr_uv as well
        quadFlags |= QSSGRhiQuadRenderer::UvCoords;
    renderer->rhiQuadRenderer()->prepareQuad(rhiCtx, nullptr);
    renderer->rhiQuadRenderer()->recordRenderQuadPass(rhiCtx, &ps, srb, pEntry->m_rhiBlurRenderTarget0, quadFlags);

    // repeat for blur Y, now depthCopy -> depthMap or cubeCopy -> depthCube

    shaderPipeline = orthographic ? renderer->getRhiOrthographicShadowBlurYShader()
                                  : renderer->getRhiCubemapShadowBlurYShader();
    if (!shaderPipeline)
        return;
    ps.shaderPipeline = shaderPipeline.data();

    bindings.clear();
    bindings.addUniformBuffer(0, VISIBILITY_ALL, dcd.ubuf);
    bindings.addTexture(1, QRhiShaderResourceBinding::FragmentStage, workMap, sampler);
    srb = rhiCtx->srb(bindings);

    renderer->rhiQuadRenderer()->prepareQuad(rhiCtx, nullptr);
    renderer->rhiQuadRenderer()->recordRenderQuadPass(rhiCtx, &ps, srb, pEntry->m_rhiBlurRenderTarget1, quadFlags);
}

static void rhiRenderShadowMap(QSSGRhiContext *rhiCtx,
                               QSSGLayerRenderData &inData,
                               QSSGRenderShadowMap *shadowMapManager,
                               const QSSGRenderCamera &camera,
                               const QSSGShaderLightList &globalLights,
                               const QVector<QSSGRenderableObjectHandle> &sortedOpaqueObjects,
                               const QSSGRef<QSSGRenderer> &renderer,
                               const BoxPoints &castingObjectsBox,
                               const BoxPoints &receivingObjectsBox)
{
    QRhi *rhi = rhiCtx->rhi();
    QRhiCommandBuffer *cb = rhiCtx->commandBuffer();

    QSSGRhiGraphicsPipelineState ps;
    ps.depthTestEnable = true;
    ps.depthWriteEnable = true;

    // We need to deal with a clip depth range of [0, 1] or
    // [-1, 1], depending on the graphics API underneath.
    QVector2D depthAdjust; // (d + depthAdjust[0]) * depthAdjust[1] = d mapped to [0, 1]
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
        if (!globalLights[i].shadows)
            continue;

        QSSGShadowMapEntry *pEntry = shadowMapManager->shadowMapEntry(i);
        if (!pEntry)
            continue;

        Q_ASSERT(pEntry->m_rhiDepthStencil);
        const bool orthographic = pEntry->m_rhiDepthMap && pEntry->m_rhiDepthCopy;
        if (orthographic) {
            const QSize size = pEntry->m_rhiDepthMap->pixelSize();
            ps.viewport = QRhiViewport(0, 0, float(size.width()), float(size.height()));

            const auto &light = globalLights[i].light;
            const auto cameraType = (light->type == QSSGRenderLight::Type::DirectionalLight) ? QSSGRenderCamera::Type::OrthographicCamera : QSSGRenderCamera::Type::CustomCamera;
            QSSGRenderCamera theCamera(cameraType);
            setupCameraForShadowMap(camera, light, theCamera, castingObjectsBox, receivingObjectsBox);
            theCamera.calculateViewProjectionMatrix(pEntry->m_lightVP);
            pEntry->m_lightView = theCamera.globalTransform.inverted(); // pre-calculate this for the material

            rhiPrepareResourcesForShadowMap(rhiCtx, inData, pEntry, &ps, &depthAdjust,
                                            sortedOpaqueObjects, theCamera, true, 0);

            // Render into the 2D texture pEntry->m_rhiDepthMap, using
            // pEntry->m_rhiDepthStencil as the (throwaway) depth/stencil buffer.
            QRhiTextureRenderTarget *rt = pEntry->m_rhiRenderTargets[0];
            cb->beginPass(rt, Qt::white, { 1.0f, 0 }, nullptr, QSSGRhiContext::commonPassFlags());
            QSSGRHICTX_STAT(rhiCtx, beginRenderPass(rt));
            rhiRenderOneShadowMap(rhiCtx, &ps, sortedOpaqueObjects, 0);
            cb->endPass();
            QSSGRHICTX_STAT(rhiCtx, endRenderPass());

            rhiBlurShadowMap(rhiCtx, pEntry, renderer, globalLights[i].light->m_shadowFilter, globalLights[i].light->m_shadowMapFar, true);
        } else {
            Q_ASSERT(pEntry->m_rhiDepthCube && pEntry->m_rhiCubeCopy);
            const QSize size = pEntry->m_rhiDepthCube->pixelSize();
            ps.viewport = QRhiViewport(0, 0, float(size.width()), float(size.height()));

            QSSGRenderCamera theCameras[6] { QSSGRenderCamera{QSSGRenderCamera::Type::PerspectiveCamera},
                                             QSSGRenderCamera{QSSGRenderCamera::Type::PerspectiveCamera},
                                             QSSGRenderCamera{QSSGRenderCamera::Type::PerspectiveCamera},
                                             QSSGRenderCamera{QSSGRenderCamera::Type::PerspectiveCamera},
                                             QSSGRenderCamera{QSSGRenderCamera::Type::PerspectiveCamera},
                                             QSSGRenderCamera{QSSGRenderCamera::Type::PerspectiveCamera} };
            setupCubeShadowCameras(globalLights[i].light, theCameras);
            pEntry->m_lightView = QMatrix4x4();

            const bool swapYFaces = !rhi->isYUpInFramebuffer();
            for (int face = 0; face < 6; ++face) {
                theCameras[face].calculateViewProjectionMatrix(pEntry->m_lightVP);
                pEntry->m_lightCubeView[face] = theCameras[face].globalTransform.inverted(); // pre-calculate this for the material

                rhiPrepareResourcesForShadowMap(rhiCtx, inData, pEntry, &ps, &depthAdjust,
                                                sortedOpaqueObjects, theCameras[face], false, face);
            }

            for (int face = 0; face < 6; ++face) {
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
                cb->beginPass(rt, Qt::white, { 1.0f, 0 }, nullptr, QSSGRhiContext::commonPassFlags());
                QSSGRHICTX_STAT(rhiCtx, beginRenderPass(rt));
                rhiRenderOneShadowMap(rhiCtx, &ps, sortedOpaqueObjects, face);
                cb->endPass();
                QSSGRHICTX_STAT(rhiCtx, endRenderPass());
            }

            rhiBlurShadowMap(rhiCtx, pEntry, renderer, globalLights[i].light->m_shadowFilter, globalLights[i].light->m_shadowMapFar, false);
        }
    }
}

static void rhiRenderReflectionMap(QSSGRhiContext *rhiCtx,
                               QSSGLayerRenderData &inData,
                               QSSGRenderReflectionMap *reflectionMapManager,
                               const QVector<QSSGRenderReflectionProbe *> &reflectionProbes,
                               const QVector<QSSGRenderableObjectHandle> &reflectionPassObjects,
                               const QSSGRef<QSSGRenderer> &renderer)
{
    QRhi *rhi = rhiCtx->rhi();
    QRhiCommandBuffer *cb = rhiCtx->commandBuffer();

    QSSGRhiGraphicsPipelineState ps;
    ps.depthTestEnable = true;
    ps.depthWriteEnable = true;
    ps.blendEnable = true;

    for (int i = 0, ie = reflectionProbes.count(); i != ie; ++i) {
        QSSGReflectionMapEntry *pEntry = reflectionMapManager->reflectionMapEntry(i);
        if (!pEntry)
            continue;

        if (!pEntry->m_needsRender)
            continue;

        if (reflectionProbes[i]->refreshMode == QSSGRenderReflectionProbe::ReflectionRefreshMode::FirstFrame && pEntry->m_rendered)
            continue;

        Q_ASSERT(pEntry->m_rhiDepthStencil);
        Q_ASSERT(pEntry->m_rhiCube);

        const QSize size = pEntry->m_rhiCube->pixelSize();
        ps.viewport = QRhiViewport(0, 0, float(size.width()), float(size.height()));

        QSSGRenderCamera theCameras[6] { QSSGRenderCamera{QSSGRenderCamera::Type::PerspectiveCamera},
                                         QSSGRenderCamera{QSSGRenderCamera::Type::PerspectiveCamera},
                                         QSSGRenderCamera{QSSGRenderCamera::Type::PerspectiveCamera},
                                         QSSGRenderCamera{QSSGRenderCamera::Type::PerspectiveCamera},
                                         QSSGRenderCamera{QSSGRenderCamera::Type::PerspectiveCamera},
                                         QSSGRenderCamera{QSSGRenderCamera::Type::PerspectiveCamera} };
        setupCubeReflectionCameras(reflectionProbes[i], theCameras);
        const bool swapYFaces = !rhi->isYUpInFramebuffer();
        for (int face = 0; face < 6; ++face) {
            theCameras[face].calculateViewProjectionMatrix(pEntry->m_viewProjection);

            rhiPrepareResourcesForReflectionMap(rhiCtx, inData, pEntry, &ps,
                                                reflectionPassObjects, theCameras[face], renderer, face);
        }
        QRhiRenderPassDescriptor *renderPassDesc = nullptr;
        for (int face = 0; face < 6; ++face) {
            if (pEntry->m_timeSlicing == QSSGRenderReflectionProbe::ReflectionTimeSlicing::IndividualFaces)
                face = pEntry->m_timeSliceFace;

            int outFace = face;
            // Faces are swapped similarly to shadow maps due to differences in backends
            // Prefilter step handles correcting orientation differences in the final render
            if (swapYFaces) {
                if (outFace == 2)
                    outFace = 3;
                else if (outFace == 3)
                    outFace = 2;
            }
            QRhiTextureRenderTarget *rt = pEntry->m_rhiRenderTargets[outFace];
            cb->beginPass(rt, reflectionProbes[i]->clearColor, { 1.0f, 0 }, nullptr, QSSGRhiContext::commonPassFlags());
            QSSGRHICTX_STAT(rhiCtx, beginRenderPass(rt));

            if (inData.layer.background == QSSGRenderLayer::Background::SkyBox
                    && rhiCtx->rhi()->isFeatureSupported(QRhi::TexelFetch)
                    && pEntry->m_skyBoxSrbs[face])
            {
                auto shaderPipeline = renderer->getRhiSkyBoxShader(QSSGRenderLayer::TonemapMode::None, inData.layer.skyBoxIsRgbe8);
                Q_ASSERT(shaderPipeline);
                ps.shaderPipeline = shaderPipeline.data();
                QRhiShaderResourceBindings *srb = pEntry->m_skyBoxSrbs[face];
                if (!renderPassDesc)
                    renderPassDesc = rt->newCompatibleRenderPassDescriptor();
                rt->setRenderPassDescriptor(renderPassDesc);
                renderer->rhiQuadRenderer()->recordRenderQuad(rhiCtx, &ps, srb, renderPassDesc, {});
            }

            bool needsSetViewport = true;
            for (const auto &handle : reflectionPassObjects)
                rhiRenderRenderable(rhiCtx, inData, *handle.obj, &needsSetViewport, face, &ps);

            cb->endPass();
            QSSGRHICTX_STAT(rhiCtx, endRenderPass());

            if (pEntry->m_timeSlicing == QSSGRenderReflectionProbe::ReflectionTimeSlicing::IndividualFaces)
                break;
        }
        if (renderPassDesc)
            renderPassDesc->deleteLater();

        pEntry->renderMips(rhiCtx);

        if (pEntry->m_timeSlicing == QSSGRenderReflectionProbe::ReflectionTimeSlicing::IndividualFaces) {
            pEntry->m_timeSliceFace++;
            if (pEntry->m_timeSliceFace >= 6)
                pEntry->m_timeSliceFace = 0;
        }

        if (reflectionProbes[i]->refreshMode == QSSGRenderReflectionProbe::ReflectionRefreshMode::FirstFrame)
            pEntry->m_rendered = true;

        pEntry->m_needsRender = false;
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
        if (!renderableTex->texture->create()) {
            qWarning("Failed to build ambient occlusion texture (size %dx%d)", size.width(), size.height());
            renderableTex->reset();
            return false;
        }
        renderableTex->resetRenderTarget();
        renderableTex->rt = rhi->newTextureRenderTarget({ renderableTex->texture });
        renderableTex->rpDesc = renderableTex->rt->newCompatibleRenderPassDescriptor();
        renderableTex->rt->setRenderPassDescriptor(renderableTex->rpDesc);
        if (!renderableTex->rt->create()) {
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
    // no texelFetch in GLSL <= 120 and GLSL ES 100
    if (!rhiCtx->rhi()->isFeatureSupported(QRhi::TexelFetch)) {
        QRhiCommandBuffer *cb = rhiCtx->commandBuffer();
        // just clear and stop there
        cb->beginPass(inData.m_rhiAoTexture.rt, Qt::white, { 1.0f, 0 });
        QSSGRHICTX_STAT(rhiCtx, beginRenderPass(inData.m_rhiAoTexture.rt));
        cb->endPass();
        QSSGRHICTX_STAT(rhiCtx, endRenderPass());
        return;
    }

    QSSGRef<QSSGRhiShaderPipeline> shaderPipeline = inData.renderer->getRhiSsaoShader();
    if (!shaderPipeline)
        return;

    QSSGRhiGraphicsPipelineState ps = basePipelineState;
    ps.shaderPipeline = shaderPipeline.data();

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
    QSSGRhiDrawCallData &dcd(rhiCtx->drawCallData({ &inData.layer, nullptr, nullptr, 0, QSSGRhiDrawCallDataKey::AoTexture }));
    if (!dcd.ubuf) {
        dcd.ubuf = rhiCtx->rhi()->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, UBUF_SIZE);
        dcd.ubuf->create();
    }

    char *ubufData = dcd.ubuf->beginFullDynamicBufferUpdateForCurrentFrame();
    memcpy(ubufData, &aoProps, 16);
    memcpy(ubufData + 16, &aoProps2, 16);
    memcpy(ubufData + 32, &aoScreenConst, 16);
    memcpy(ubufData + 48, &uvToEyeConst, 16);
    memcpy(ubufData + 64, &cameraProps, 8);
    dcd.ubuf->endFullDynamicBufferUpdateForCurrentFrame();

    QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Nearest, QRhiSampler::Nearest, QRhiSampler::None,
                                             QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge, QRhiSampler::Repeat });
    QSSGRhiShaderResourceBindingList bindings;
    bindings.addUniformBuffer(0, VISIBILITY_ALL, dcd.ubuf);
    bindings.addTexture(1, QRhiShaderResourceBinding::FragmentStage, inData.m_rhiDepthTexture.texture, sampler);
    QRhiShaderResourceBindings *srb = rhiCtx->srb(bindings);

    inData.renderer->rhiQuadRenderer()->prepareQuad(rhiCtx, nullptr);
    inData.renderer->rhiQuadRenderer()->recordRenderQuadPass(rhiCtx, &ps, srb, inData.m_rhiAoTexture.rt, {});
}

static bool rhiPrepareScreenTexture(QSSGRhiContext *rhiCtx, const QSize &size, bool wantsMips, QSSGRhiRenderableTexture *renderableTex)
{
    QRhi *rhi = rhiCtx->rhi();
    bool needsBuild = false;
    QRhiTexture::Flags flags = QRhiTexture::RenderTarget;
    if (wantsMips)
        flags |= QRhiTexture::MipMapped | QRhiTexture::UsedWithGenerateMips;

    if (!renderableTex->texture) {
        // always non-msaa, even if multisampling is used in the main pass
        renderableTex->texture = rhi->newTexture(QRhiTexture::RGBA8, size, 1, flags);
        needsBuild = true;
    } else if (renderableTex->texture->pixelSize() != size) {
        renderableTex->texture->setPixelSize(size);
        needsBuild = true;
    }

    if (!renderableTex->depthStencil) {
        renderableTex->depthStencil = rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, size);
        needsBuild = true;
    } else if (renderableTex->depthStencil->pixelSize() != size) {
        renderableTex->depthStencil->setPixelSize(size);
        needsBuild = true;
    }

    if (needsBuild) {
        if (!renderableTex->texture->create()) {
            qWarning("Failed to build screen texture (size %dx%d)", size.width(), size.height());
            renderableTex->reset();
            return false;
        }
        if (!renderableTex->depthStencil->create()) {
            qWarning("Failed to build depth-stencil buffer for screen texture (size %dx%d)",
                     size.width(), size.height());
            renderableTex->reset();
            return false;
        }
        renderableTex->resetRenderTarget();
        QRhiTextureRenderTargetDescription desc;
        desc.setColorAttachments({ QRhiColorAttachment(renderableTex->texture) });
        desc.setDepthStencilBuffer(renderableTex->depthStencil);
        renderableTex->rt = rhi->newTextureRenderTarget(desc);
        renderableTex->rpDesc = renderableTex->rt->newCompatibleRenderPassDescriptor();
        renderableTex->rt->setRenderPassDescriptor(renderableTex->rpDesc);
        if (!renderableTex->rt->create()) {
            qWarning("Failed to build render target for screen texture");
            renderableTex->reset();
            return false;
        }
    }

    return true;
}

// These are meant to be pixel offsets, so you need to divide them by the width/height
// of the layer respectively.
static const QVector2D s_ProgressiveAAVertexOffsets[QSSGLayerRenderPreparationData::MAX_AA_LEVELS] = {
    QVector2D(-0.170840f, -0.553840f), // 1x
    QVector2D(0.162960f, -0.319340f), // 2x
    QVector2D(0.360260f, -0.245840f), // 3x
    QVector2D(-0.561340f, -0.149540f), // 4x
    QVector2D(0.249460f, 0.453460f), // 5x
    QVector2D(-0.336340f, 0.378260f), // 6x
    QVector2D(0.340000f, 0.166260f), // 7x
    QVector2D(0.235760f, 0.527760f), // 8x
};

static inline QRect correctViewportCoordinates(const QRectF &layerViewport, const QRect &deviceRect)
{
    const int y = deviceRect.bottom() - layerViewport.bottom() + 1;
    return QRect(layerViewport.x(), y, layerViewport.width(), layerViewport.height());
}

// Phase 1: prepare. Called when the renderpass is not yet started on the command buffer.
void QSSGLayerRenderData::rhiPrepare()
{
    QSSGRhiContext *rhiCtx = renderer->contextInterface()->rhiContext().data();
    Q_ASSERT(rhiCtx->isValid());

    QSSGRhiGraphicsPipelineState *ps = rhiCtx->resetGraphicsPipelineState(this);

    const QRectF vp = layerPrepResult->viewport();
    ps->viewport = { float(vp.x()), float(vp.y()), float(vp.width()), float(vp.height()), 0.0f, 1.0f };
    ps->scissorEnable = true;
    const QRect sc = layerPrepResult->scissor().toRect();
    ps->scissor = { sc.x(), sc.y(), sc.width(), sc.height() };


    const bool animating = layerPrepResult->flags.wasLayerDataDirty();
    if (animating)
        layer.progAAPassIndex = 0;

    const bool progressiveAA = layer.antialiasingMode == QSSGRenderLayer::AAMode::ProgressiveAA && !animating;
    layer.progressiveAAIsActive = progressiveAA;

    const bool temporalAA = layer.temporalAAEnabled && !progressiveAA &&  layer.antialiasingMode != QSSGRenderLayer::AAMode::MSAA;

    layer.temporalAAIsActive = temporalAA;

    QVector2D vertexOffsetsAA;

    if (progressiveAA && layer.progAAPassIndex > 0) {
        int idx = layer.progAAPassIndex - 1;
        vertexOffsetsAA = s_ProgressiveAAVertexOffsets[idx] / QVector2D{ float(vp.width()/2.0), float(vp.height()/2.0) };
    }

    if (temporalAA) {
        const int t = 1 - 2 * (layer.tempAAPassIndex % 2);
        const float f = t * layer.temporalAAStrength;
        vertexOffsetsAA = { f / float(vp.width()/2.0), f / float(vp.height()/2.0) };
    }

    if (camera) {
        if (temporalAA || progressiveAA /*&& !vertexOffsetsAA.isNull()*/) {
            // TODO - optimize this exact matrix operation.
            for (qint32 idx = 0, end = modelContexts.size(); idx < end; ++idx) {
                QMatrix4x4 offsetProjection = camera->projection;
                offsetProjection(0, 2) = vertexOffsetsAA.x();
                offsetProjection(1, 2) = vertexOffsetsAA.y();
                modelContexts[idx]->modelViewProjection = offsetProjection * camera->projection.inverted() * modelContexts[idx]->modelViewProjection;
            }
        }

        camera->dpr = renderer->contextInterface()->dpr();
        renderer->beginLayerRender(*this);

        QSSGRhiContext *rhiCtx = renderer->contextInterface()->rhiContext().data();
        Q_ASSERT(rhiCtx->rhi()->isRecordingFrame());
        QRhiCommandBuffer *cb = rhiCtx->commandBuffer();

        const auto &sortedOpaqueObjects = getOpaqueRenderableObjects(true); // front to back
        const auto &sortedTransparentObjects = getTransparentRenderableObjects(); // back to front
        const auto &sortedScreenTextureObjects = getScreenTextureRenderableObjects(); // back to front
        const auto &item2Ds = getRenderableItem2Ds();

        // Verify that the depth write list(s) were cleared between frames
        Q_ASSERT(renderedDepthWriteObjects.isEmpty());
        Q_ASSERT(renderedOpaqueDepthPrepassObjects.isEmpty());

        // Depth Write List
        if (layer.flags.testFlag(QSSGRenderLayer::Flag::LayerEnableDepthTest)) {
            for (const auto &opaqueObject : sortedOpaqueObjects) {
                const auto depthMode = opaqueObject.obj->depthWriteMode;
                if (depthMode == QSSGDepthDrawMode::Always || depthMode == QSSGDepthDrawMode::OpaqueOnly)
                    renderedDepthWriteObjects.append(opaqueObject);
                else if (depthMode == QSSGDepthDrawMode::OpaquePrePass)
                    renderedOpaqueDepthPrepassObjects.append(opaqueObject);
            }
            for (const auto &transparentObject : sortedTransparentObjects) {
                const auto depthMode = transparentObject.obj->depthWriteMode;
                if (depthMode == QSSGDepthDrawMode::Always)
                    renderedDepthWriteObjects.append(transparentObject);
                else if (depthMode == QSSGDepthDrawMode::OpaquePrePass)
                    renderedOpaqueDepthPrepassObjects.append(transparentObject);
            }
            for (const auto &screenTextureObject : sortedScreenTextureObjects) {
                const auto depthMode = screenTextureObject.obj->depthWriteMode;
                if (depthMode == QSSGDepthDrawMode::Always || depthMode == QSSGDepthDrawMode::OpaqueOnly)
                    renderedDepthWriteObjects.append(screenTextureObject);
                else if (depthMode == QSSGDepthDrawMode::OpaquePrePass)
                    renderedOpaqueDepthPrepassObjects.append(screenTextureObject);
            }
        }

        // If needed, generate a depth texture with the opaque objects. This
        // and the SSAO texture must come first since other passes may want to
        // expose these textures to their shaders.
        if (layerPrepResult->flags.requiresDepthTexture() && m_progressiveAAPassIndex == 0) {
            cb->debugMarkBegin(QByteArrayLiteral("Quick3D depth texture"));

            if (rhiPrepareDepthTexture(rhiCtx, layerPrepResult->textureDimensions(), &m_rhiDepthTexture)) {
                Q_ASSERT(m_rhiDepthTexture.isValid());
                if (rhiPrepareDepthPass(rhiCtx, *ps, m_rhiDepthTexture.rpDesc, *this,
                                        sortedOpaqueObjects, sortedTransparentObjects,
                                        QSSGRhiDrawCallDataKey::DepthTexture,
                                        1))
                {
                    bool needsSetVieport = true;
                    cb->beginPass(m_rhiDepthTexture.rt, Qt::transparent, { 1.0f, 0 }, nullptr, QSSGRhiContext::commonPassFlags());
                    QSSGRHICTX_STAT(rhiCtx, beginRenderPass(m_rhiDepthTexture.rt));
                    // NB! We do not pass sortedTransparentObjects in the 4th
                    // argument to stay compatible with the 5.15 code base,
                    // which also does not include semi-transparent objects in
                    // the depth texture. In addition, capturing after the
                    // opaque pass, not including transparent objects, is part
                    // of the contract for screen reading custom materials,
                    // both for depth and color.
                    rhiRenderDepthPass(rhiCtx, *this, sortedOpaqueObjects, {}, &needsSetVieport);
                    cb->endPass();
                    QSSGRHICTX_STAT(rhiCtx, endRenderPass());
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

        // Shadows. Generates a 2D or cube shadow map. (opaque + pre-pass transparent objects)
        if (layerPrepResult->flags.requiresShadowMapPass() && m_progressiveAAPassIndex == 0) {
            if (!shadowMapManager)
                shadowMapManager = new QSSGRenderShadowMap(*renderer->contextInterface());

            const TRenderableObjectList shadowPassObjects = renderedDepthWriteObjects + renderedOpaqueDepthPrepassObjects;

            if (!shadowPassObjects.isEmpty() || !globalLights.isEmpty()) {
                cb->debugMarkBegin(QByteArrayLiteral("Quick3D shadow map"));

                const auto [castingObjectsBox, receivingObjectsBox] = calculateSortedObjectBounds(sortedOpaqueObjects,
                                                                                                  sortedTransparentObjects);

                rhiRenderShadowMap(rhiCtx,
                                   *this,
                                   shadowMapManager,
                                   *camera,
                                   globalLights, // scoped lights are not relevant here
                                   shadowPassObjects,
                                   renderer,
                                   castingObjectsBox,
                                   receivingObjectsBox);

                cb->debugMarkEnd();
            }
        }

        // Reflections.
        {
            if (!reflectionMapManager)
                reflectionMapManager = new QSSGRenderReflectionMap(*renderer->contextInterface());

            const TRenderableObjectList reflectionPassObjects = sortedOpaqueObjects + sortedTransparentObjects;

            if (!reflectionPassObjects.isEmpty() || !reflectionProbes.isEmpty()) {
                cb->debugMarkBegin(QByteArrayLiteral("Quick3D reflection map"));

                rhiRenderReflectionMap(rhiCtx,
                                   *this,
                                   reflectionMapManager,
                                   reflectionProbes,
                                   reflectionPassObjects,
                                   renderer);

                cb->debugMarkEnd();
            }
        }

        // Z (depth) pre-pass, if enabled, is part of the main render pass. (opaque + pre-pass transparent objects)
        // Prepare the data for it.
        bool zPrePass = layer.flags.testFlag(QSSGRenderLayer::Flag::LayerEnableDepthPrePass)
                && layer.flags.testFlag(QSSGRenderLayer::Flag::LayerEnableDepthTest)
                && (!renderedDepthWriteObjects.isEmpty() || !item2Ds.isEmpty());
        if (zPrePass || !renderedOpaqueDepthPrepassObjects.isEmpty()) {
            cb->debugMarkBegin(QByteArrayLiteral("Quick3D prepare Z prepass"));
            m_globalZPrePassActive = false;
            if (!zPrePass) {
                rhiPrepareDepthPass(rhiCtx, *ps, rhiCtx->mainRenderPassDescriptor(), *this,
                                    {}, renderedOpaqueDepthPrepassObjects,
                                    QSSGRhiDrawCallDataKey::ZPrePass,
                                    rhiCtx->mainPassSampleCount());

            } else {
                m_globalZPrePassActive = rhiPrepareDepthPass(rhiCtx, *ps, rhiCtx->mainRenderPassDescriptor(), *this,
                                                         renderedDepthWriteObjects, renderedOpaqueDepthPrepassObjects,
                                                         QSSGRhiDrawCallDataKey::ZPrePass,
                                                         rhiCtx->mainPassSampleCount());
            }
            cb->debugMarkEnd();
        }

        // Now onto preparing the data for the main pass.

        QSSGRhiGraphicsPipelineState *ps = rhiCtx->graphicsPipelineState(this);

        ps->depthFunc = QRhiGraphicsPipeline::LessOrEqual;
        ps->blendEnable = false;

        if (layer.background == QSSGRenderLayer::Background::SkyBox && layer.lightProbe)
            rhiPrepareSkyBox(rhiCtx, layer, *camera, renderer);

        const bool layerEnableDepthTest = layer.flags.testFlag(QSSGRenderLayer::Flag::LayerEnableDepthTest);
        bool depthTestEnableDefault = false;
        bool depthWriteEnableDefault = false;
        if (layerEnableDepthTest && (!sortedOpaqueObjects.isEmpty() || !renderedOpaqueDepthPrepassObjects.isEmpty() || !renderedDepthWriteObjects.isEmpty())) {
            depthTestEnableDefault = true;
            // enable depth write for opaque objects when there was no Z prepass
            depthWriteEnableDefault = !layer.flags.testFlag(QSSGRenderLayer::Flag::LayerEnableDepthPrePass) || !m_globalZPrePassActive;
        }

        ps->depthTestEnable = depthTestEnableDefault;
        ps->depthWriteEnable = depthWriteEnableDefault;

        // Screen texture with opaque objects.
        if (layerPrepResult->flags.requiresScreenTexture() && m_progressiveAAPassIndex == 0) {
            const bool wantsMips = layerPrepResult->flags.requiresMipmapsForScreenTexture();
            cb->debugMarkBegin(QByteArrayLiteral("Quick3D screen texture"));
            if (rhiPrepareScreenTexture(rhiCtx, layerPrepResult->textureDimensions(), wantsMips, &m_rhiScreenTexture)) {
                Q_ASSERT(m_rhiScreenTexture.isValid());
                // NB: not compatible with disabling LayerEnableDepthTest
                // because there are effectively no "opaque" objects then.
                // Disable Tonemapping for all materials in the screen pass texture
                QSSGShaderFeatures featuresBackup = this->features;
                this->features.disableTonemapping();
                for (const auto &handle : sortedOpaqueObjects)
                    rhiPrepareRenderable(rhiCtx, *this, *handle.obj, m_rhiScreenTexture.rpDesc, 1);
                QColor clearColor(Qt::transparent);
                if (layer.background == QSSGRenderLayer::Background::Color)
                    clearColor = QColor::fromRgbF(layer.clearColor.x(), layer.clearColor.y(), layer.clearColor.z());
                cb->beginPass(m_rhiScreenTexture.rt, clearColor, { 1.0f, 0 }, nullptr, QSSGRhiContext::commonPassFlags());
                QSSGRHICTX_STAT(rhiCtx, beginRenderPass(m_rhiScreenTexture.rt));
                if (layer.background == QSSGRenderLayer::Background::SkyBox
                        && rhiCtx->rhi()->isFeatureSupported(QRhi::TexelFetch) && layer.skyBoxSrb) {
                    // This is offscreen, so rendered untonemapped
                    auto shaderPipeline = renderer->getRhiSkyBoxShader(QSSGRenderLayer::TonemapMode::None, layer.skyBoxIsRgbe8);
                    Q_ASSERT(shaderPipeline);
                    QSSGRhiGraphicsPipelineState *ps = rhiCtx->graphicsPipelineState(this);
                    ps->shaderPipeline = shaderPipeline.data();
                    QRhiShaderResourceBindings *srb = layer.skyBoxSrb;
                    QRhiRenderPassDescriptor *rpDesc = rhiCtx->mainRenderPassDescriptor();
                    renderer->rhiQuadRenderer()->recordRenderQuad(rhiCtx, ps, srb, rpDesc, {});
                }
                bool needsSetViewport = true;
                for (const auto &handle : sortedOpaqueObjects)
                    rhiRenderRenderable(rhiCtx, *this, *handle.obj, &needsSetViewport);

                QRhiResourceUpdateBatch *rub = nullptr;
                if (wantsMips) {
                    rub = rhiCtx->rhi()->nextResourceUpdateBatch();
                    rub->generateMips(m_rhiScreenTexture.texture);
                }
                cb->endPass(rub);
                QSSGRHICTX_STAT(rhiCtx, endRenderPass());
                // Re-enable all tonemapping
                this->features = featuresBackup;
            }
            cb->debugMarkEnd();
        } else {
            m_rhiScreenTexture.reset();
        }

        // make the buffer copies and other stuff we put on the command buffer in
        // here show up within a named section in tools like RenderDoc when running
        // with QSG_RHI_PROFILE=1 (which enables debug markers)
        cb->debugMarkBegin(QByteArrayLiteral("Quick3D prepare renderables"));

        QRhiRenderPassDescriptor *mainRpDesc = rhiCtx->mainRenderPassDescriptor();
        const int samples = rhiCtx->mainPassSampleCount();

        ps->depthTestEnable = depthTestEnableDefault;
        ps->depthWriteEnable = depthWriteEnableDefault;

        // opaque objects (or, this list is empty when LayerEnableDepthTest is disabled)
        for (const auto &handle : sortedOpaqueObjects) {
            QSSGRenderableObject *theObject = handle.obj;
            const auto depthWriteMode = theObject->depthWriteMode;
            ps->depthWriteEnable = !(depthWriteMode == QSSGDepthDrawMode::Never ||
                                    depthWriteMode == QSSGDepthDrawMode::OpaquePrePass ||
                                    m_globalZPrePassActive || !layerEnableDepthTest);
            rhiPrepareRenderable(rhiCtx, *this, *theObject, mainRpDesc, samples);
        }

        // objects that requires the screen texture
        ps->depthTestEnable = depthTestEnableDefault;
        ps->depthWriteEnable = depthWriteEnableDefault;
        for (const auto &handle : sortedScreenTextureObjects) {
            QSSGRenderableObject *theObject = handle.obj;
            const auto depthWriteMode = theObject->depthWriteMode;
            ps->blendEnable = theObject->renderableFlags.hasTransparency();
            ps->depthWriteEnable = !(depthWriteMode == QSSGDepthDrawMode::Never || depthWriteMode == QSSGDepthDrawMode::OpaquePrePass
                                     || m_globalZPrePassActive || !layerEnableDepthTest);
            rhiPrepareRenderable(rhiCtx, *this, *theObject, mainRpDesc, samples);
        }

        // objects rendered by Qt Quick 2D
        ps->depthTestEnable = depthTestEnableDefault;
        ps->depthWriteEnable = depthWriteEnableDefault;
        ps->blendEnable = false;

        for (const auto &item: item2Ds) {
            QSSGRenderItem2D *item2D = static_cast<QSSGRenderItem2D *>(item.node);
            // Set the projection matrix
            if (!item2D->m_renderer)
                continue;
            if (item2D->m_rci != renderer->contextInterface()) {
                if (!item2D->m_contextWarningShown) {
                    qWarning () << "Scene with embedded 2D content can only be rendered in one window.";
                    item2D->m_contextWarningShown = true;
                }
                continue;
            }

            item2D->m_renderer->setProjectionMatrix(item2D->MVP);
            const auto &renderTarget = rhiCtx->renderTarget();
            item2D->m_renderer->setDevicePixelRatio(renderTarget->devicePixelRatio());
            const QRect deviceRect(QPoint(0, 0), renderTarget->pixelSize());
            item2D->m_renderer->setViewportRect(correctViewportCoordinates(layerPrepResult->viewport(), deviceRect));
            item2D->m_renderer->setDeviceRect(deviceRect);
            item2D->m_renderer->setRenderTarget(renderTarget);
            item2D->m_renderer->setCommandBuffer(rhiCtx->commandBuffer());
            QRhiRenderPassDescriptor *oldRp = nullptr;
            if (item2D->m_rp) {
                // Changing render target, and so incompatible renderpass
                // descriptors should be uncommon, but possible.
                if (!item2D->m_rp->isCompatible(rhiCtx->mainRenderPassDescriptor()))
                    std::swap(item2D->m_rp, oldRp);
            }
            if (!item2D->m_rp) {
                // Do not pass our object to the Qt Quick scenegraph. It may
                // hold on to it, leading to lifetime and ownership issues.
                // Rather, create a dedicated, compatible object.
                item2D->m_rp = rhiCtx->mainRenderPassDescriptor()->newCompatibleRenderPassDescriptor();
                Q_ASSERT(item2D->m_rp);
            }
            item2D->m_renderer->setRenderPassDescriptor(item2D->m_rp);
            delete oldRp;
            item2D->m_renderer->prepareSceneInline();
        }

        // transparent objects (or, without LayerEnableDepthTest, all objects)
        ps->blendEnable = true;
        ps->depthWriteEnable = false;

        for (const auto &handle : sortedTransparentObjects) {
            QSSGRenderableObject *theObject = handle.obj;
            const auto depthWriteMode = theObject->depthWriteMode;
            if (depthWriteMode == QSSGDepthDrawMode::Always && !m_globalZPrePassActive)
                ps->depthWriteEnable = true;
            else
                ps->depthWriteEnable = false;
            if (!(theObject->renderableFlags.isCompletelyTransparent()))
                rhiPrepareRenderable(rhiCtx, *this, *theObject, mainRpDesc, samples);
        }

        cb->debugMarkEnd();

        renderer->endLayerRender();
    }
}

static void rhiRenderRenderable(QSSGRhiContext *rhiCtx,
                                QSSGLayerRenderData &inData,
                                QSSGRenderableObject &object,
                                bool *needsSetViewport,
                                int cubeFace,
                                QSSGRhiGraphicsPipelineState *state)
{
    if (object.renderableFlags.isDefaultMaterialMeshSubset()) {
        QSSGSubsetRenderable &subsetRenderable(static_cast<QSSGSubsetRenderable &>(object));

        QRhiGraphicsPipeline *ps = subsetRenderable.rhiRenderData.mainPass.pipeline;
        QRhiShaderResourceBindings *srb = subsetRenderable.rhiRenderData.mainPass.srb;

        if (cubeFace >= 0) {
            ps = subsetRenderable.rhiRenderData.reflectionPass.pipeline;
            srb = subsetRenderable.rhiRenderData.reflectionPass.srb[cubeFace];
        }

        if (!ps || !srb)
            return;

        QRhiBuffer *vertexBuffer = subsetRenderable.subset.rhi.vertexBuffer->buffer();
        QRhiBuffer *indexBuffer = subsetRenderable.subset.rhi.indexBuffer ? subsetRenderable.subset.rhi.indexBuffer->buffer() : nullptr;

        QRhiCommandBuffer *cb = rhiCtx->commandBuffer();
        // QRhi optimizes out unnecessary binding of the same pipline
        cb->setGraphicsPipeline(ps);
        cb->setShaderResources(srb);

        if (*needsSetViewport) {
            if (!state)
                cb->setViewport(rhiCtx->graphicsPipelineState(&inData)->viewport);
            else
                cb->setViewport(state->viewport);
            *needsSetViewport = false;
        }

        QRhiCommandBuffer::VertexInput vertexBuffers[2];
        int vertexBufferCount = 1;
        vertexBuffers[0] = QRhiCommandBuffer::VertexInput(vertexBuffer, 0);
        quint32 instances = 1;
        if ( subsetRenderable.modelContext.model.instancing()) {
            instances = subsetRenderable.modelContext.model.instanceCount();
            vertexBuffers[1] = QRhiCommandBuffer::VertexInput(subsetRenderable.instanceBuffer, 0);
            vertexBufferCount = 2;
        }
        if (indexBuffer) {
            cb->setVertexInput(0, vertexBufferCount, vertexBuffers, indexBuffer, 0, subsetRenderable.subset.rhi.indexBuffer->indexFormat());
            cb->drawIndexed(subsetRenderable.subset.count, instances, subsetRenderable.subset.offset);
            QSSGRHICTX_STAT(rhiCtx, drawIndexed(subsetRenderable.subset.count, instances));
        } else {
            cb->setVertexInput(0, vertexBufferCount, vertexBuffers);
            cb->draw(subsetRenderable.subset.count, instances, subsetRenderable.subset.offset);
            QSSGRHICTX_STAT(rhiCtx, draw(subsetRenderable.subset.count, instances));
        }
    } else if (object.renderableFlags.isCustomMaterialMeshSubset()) {
        QSSGSubsetRenderable &subsetRenderable(static_cast<QSSGSubsetRenderable &>(object));
        QSSGCustomMaterialSystem &customMaterialSystem(*subsetRenderable.generator->contextInterface()->customMaterialSystem().data());
        customMaterialSystem.rhiRenderRenderable(rhiCtx, subsetRenderable, inData, needsSetViewport, cubeFace, state);
    } else if (object.renderableFlags.isParticlesRenderable()) {
        QSSGParticlesRenderable &renderable(static_cast<QSSGParticlesRenderable &>(object));
        QSSGParticleRenderer::rhiRenderRenderable(rhiCtx, renderable, inData, needsSetViewport, cubeFace, state);
    } else {
        Q_ASSERT(false);
    }
}

// Phase 2: render. Called within an active renderpass on the command buffer.
void QSSGLayerRenderData::rhiRender()
{
    QSSGRhiContext *rhiCtx = renderer->contextInterface()->rhiContext().data();

    if (camera) {
        renderer->beginLayerRender(*this);

        QRhiCommandBuffer *cb = rhiCtx->commandBuffer();
        const auto &theOpaqueObjects = getOpaqueRenderableObjects(true);
        const auto &item2Ds = getRenderableItem2Ds();
        bool needsSetViewport = true;

        bool zPrePass = layer.flags.testFlag(QSSGRenderLayer::Flag::LayerEnableDepthPrePass)
                && layer.flags.testFlag(QSSGRenderLayer::Flag::LayerEnableDepthTest)
                && (!renderedDepthWriteObjects.isEmpty() || !item2Ds.isEmpty());
        if (zPrePass && m_globalZPrePassActive) {
            cb->debugMarkBegin(QByteArrayLiteral("Quick3D render Z prepass"));
            rhiRenderDepthPass(rhiCtx, *this, renderedDepthWriteObjects, renderedOpaqueDepthPrepassObjects, &needsSetViewport);
            cb->debugMarkEnd();
        } else if (!renderedOpaqueDepthPrepassObjects.isEmpty() &&
                   layer.flags.testFlag(QSSGRenderLayer::Flag::LayerEnableDepthTest)) {
            cb->debugMarkBegin(QByteArrayLiteral("Quick3D render Z forced prepass"));
            rhiRenderDepthPass(rhiCtx, *this, {}, renderedOpaqueDepthPrepassObjects, &needsSetViewport);
            cb->debugMarkEnd();
        }

        if (layer.background == QSSGRenderLayer::Background::SkyBox
                && rhiCtx->rhi()->isFeatureSupported(QRhi::TexelFetch)
                && layer.skyBoxSrb)
        {
            auto shaderPipeline = renderer->getRhiSkyBoxShader(layer.tonemapMode, layer.skyBoxIsRgbe8);
            Q_ASSERT(shaderPipeline);
            QSSGRhiGraphicsPipelineState *ps = rhiCtx->graphicsPipelineState(this);
            ps->shaderPipeline = shaderPipeline.data();
            QRhiShaderResourceBindings *srb = layer.skyBoxSrb;
            QRhiRenderPassDescriptor *rpDesc = rhiCtx->mainRenderPassDescriptor();
            renderer->rhiQuadRenderer()->recordRenderQuad(rhiCtx, ps, srb, rpDesc, {});
        }

        cb->debugMarkBegin(QByteArrayLiteral("Quick3D render opaque"));
        for (const auto &handle : theOpaqueObjects) {
            QSSGRenderableObject *theObject = handle.obj;
            rhiRenderRenderable(rhiCtx, *this, *theObject, &needsSetViewport);
        }
        cb->debugMarkEnd();

        cb->debugMarkBegin(QByteArrayLiteral("Quick3D render screen texture dependent"));
        const auto &theScreenTextureObjects = getScreenTextureRenderableObjects();
        for (const auto &handle : theScreenTextureObjects) {
            QSSGRenderableObject *theObject = handle.obj;
            rhiRenderRenderable(rhiCtx, *this, *theObject, &needsSetViewport);
        }
        cb->debugMarkEnd();

        if (!item2Ds.isEmpty()) {
            cb->debugMarkBegin(QByteArrayLiteral("Quick3D render 2D sub-scene"));
            for (const auto &item : item2Ds) {
                QSSGRenderItem2D *item2D = static_cast<QSSGRenderItem2D *>(item.node);
                if (item2D->m_rci == renderer->contextInterface())
                    item2D->m_renderer->renderSceneInline();
            }
            cb->debugMarkEnd();
        }

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
