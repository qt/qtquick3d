// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "volumetricfogextension.h"

#include <memory>
#include <ssg/qssgrendercontextcore.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderer_p.h>
#include <QtQuick3DRuntimeRender/private/qssglayerrenderdata_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercamera_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrhicontext_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadowmap_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderimage_p.h>
#include <QtQuick3D/private/qquick3dobject_p.h>
#include <QtCore/QFile>
#include <QtGui/QImage>
#include <QtQuick/QSGTexture>
#include <QHash>

static constexpr int MAX_LIGHTS = 8;
static constexpr int MAX_VOLUMES = 32;

struct alignas(16) LightBufferData {
    float matrices[4][16];
    QVector4D lightPos[MAX_LIGHTS];
    QVector4D lightColor[MAX_LIGHTS];
    QVector4D lightDirection[MAX_LIGHTS];
    QVector4D coneAngles[MAX_LIGHTS];
    QVector4D dirLightColor;
    float csmSplits[4];
    float csmActive[4];
    float atlasLocations[4][4];
    float dimensionsInverted[4][4];
    float shadowBias;
    float shadowFactor;
    float padd2[2];
    int numLights;
    int padding[3];
    float lightShadowMat[MAX_LIGHTS][16];
    float lightShadowAtlas[MAX_LIGHTS][4];
    float lightShadowAtlas2[MAX_LIGHTS][4];
    QVector4D iesParams;
    QVector4D iesIndices[2];
};

struct alignas(16) FroxelUniforms {
    float invViewProjection[16];
    float viewMatrix[16];
    float gridDimensions[4];
    float depthParams[4];
    float cameraPos[4];
    float qt_rhi_properties[4];
};

struct alignas(16) FogVolumes {
    float matrices[MAX_VOLUMES][16];
    QVector4D extents[MAX_VOLUMES];
    QVector4D color[MAX_VOLUMES];
    QVector4D heightParams[MAX_VOLUMES];
    QVector4D noiseOffsetScale[MAX_VOLUMES];
    int numVolumes;
    int padding[3];
};

struct FogVolumeEntry {
    QMatrix4x4 invTransform;
    QVector3D extents;
    int type = 0;
    QColor color = Qt::white;
    float density = 1.0f;
    int heightEnabled = 0;
    float leastIntenseY = 10.0f;
    float mostIntenseY = 0.0f;
    float heightCurve = 1.0f;
    QVector3D noiseOffset;
    float noiseScale = 0.5f;
};

struct IESLightEntry {
    int index = -1;
    float intensity = 1.0f;
};

struct IESConfig {
    QSSGRenderImage *renderImage = nullptr;
    int count = 1;
    QHash<QSSGRenderLight *, IESLightEntry> lightMap;
};

struct FroxelConfig {
    int width = 160;
    int height = 90;
    int depth = 64;
    float nearPlane = 1.0f;
    float farPlane = 2000.0f;
    QList<FogVolumeEntry> fogVolumeData;
    IESConfig ies;
};

class FroxelQSGTexture : public QSGTexture
{
public:
    QRhiTexture *m_rhiTexture = nullptr;

    QSize textureSize() const override { return m_rhiTexture ? m_rhiTexture->pixelSize() : QSize{}; }
    bool hasAlphaChannel() const override { return true; }
    bool hasMipmaps() const override { return false; }
    qint64 comparisonKey() const override { return reinterpret_cast<quintptr>(m_rhiTexture); }
    QRhiTexture *rhiTexture() const override { return m_rhiTexture; }
};

class FroxelRenderer : public QSSGRenderExtension
{
public:
    FroxelConfig config;
    bool resourcesDirty = true;
    QQuick3DTexture *frontendTexture = nullptr;

    bool prepareData(QSSGFrameData &data) override;
    void prepareRender(QSSGFrameData &data) override;
    void render(QSSGFrameData &data) override;
    void resetForFrame() override;
    RenderMode mode() const override { return RenderMode::Standalone; }
    RenderStage stage() const override { return RenderStage::PostColor; }
    bool m_pipelineDirty = true;

private:
    void setupFroxelGrid(const QSSGRhiContext *rhiCtx);
    void setupComputePipeline(const QSSGRhiContext *rhiCtx, QRhiTexture *shadowAtlas);
    void updateUniforms(const QSSGRhiContext *rhiCtx, QSSGLayerRenderData *layer, QSSGRenderCamera *camera);

    bool m_enabled = false;

    std::unique_ptr<QRhiTexture> m_froxelRhiTexture;
    std::unique_ptr<FroxelQSGTexture> m_qsgTexture;
    std::unique_ptr<QRhiBuffer> m_froxelUniformBuffer;
    std::unique_ptr<QRhiBuffer> m_lightDataBuffer;
    std::unique_ptr<QRhiBuffer> m_volumeDataBuffer;

    std::unique_ptr<QRhiComputePipeline> m_computePipeline;
    std::unique_ptr<QRhiShaderResourceBindings> m_computeBindings;
    std::unique_ptr<QRhiSampler> m_atlasSampler;
    std::unique_ptr<QRhiTexture> m_blueNoiseTexture;
    std::unique_ptr<QRhiSampler> m_blueNoiseSampler;
    QRhiTexture *m_shadowAtlas = nullptr;

    std::unique_ptr<QRhiTexture> m_ies1x1Fallback;
    std::unique_ptr<QRhiSampler> m_iesSampler;
    QRhiTexture *m_iesRhiTexture = nullptr;
};

void FroxelRenderer::setupFroxelGrid(const QSSGRhiContext *rhiCtx)
{
    if (!resourcesDirty)
        return;

    QRhi *rhi = rhiCtx->rhi();

    m_froxelRhiTexture.reset(rhi->newTexture(
        QRhiTexture::RGBA16F,
        config.width, config.height, config.depth,
        1,
        QRhiTexture::ThreeDimensional | QRhiTexture::UsedWithLoadStore | QRhiTexture::UsedAsTransferSource));
    m_froxelRhiTexture->setDepth(config.depth);
    m_froxelRhiTexture->create();

    if (!m_qsgTexture)
        m_qsgTexture = std::make_unique<FroxelQSGTexture>();
    m_qsgTexture->m_rhiTexture = m_froxelRhiTexture.get();

    if (frontendTexture) {
        auto *img = static_cast<QSSGRenderImage *>(
            QQuick3DObjectPrivate::get(frontendTexture)->spatialNode);
        if (img)
            img->m_qsgTexture = m_qsgTexture.get();
    }

    if (!m_froxelUniformBuffer) {
        m_froxelUniformBuffer.reset(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, sizeof(FroxelUniforms)));
        m_froxelUniformBuffer->create();
    }
    if (!m_lightDataBuffer) {
        m_lightDataBuffer.reset(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, sizeof(LightBufferData)));
        m_lightDataBuffer->create();
    }
    if (!m_volumeDataBuffer) {
        m_volumeDataBuffer.reset(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, sizeof(FogVolumes)));
        m_volumeDataBuffer->create();
    }
    if (!m_blueNoiseTexture) {
        QImage img(QStringLiteral(":/res/textures/blue_noise.png"));
        img = img.convertToFormat(QImage::Format_RGBA8888);
        m_blueNoiseTexture.reset(rhi->newTexture(QRhiTexture::RGBA8, img.size(), 1, QRhiTexture::Flags{}));
        m_blueNoiseTexture->create();
        QRhiResourceUpdateBatch *noiseRub = rhi->nextResourceUpdateBatch();
        QRhiTextureUploadEntry entry(0, 0, QRhiTextureSubresourceUploadDescription(img.constBits(), img.sizeInBytes()));
        noiseRub->uploadTexture(m_blueNoiseTexture.get(), QRhiTextureUploadDescription(entry));
        rhiCtx->commandBuffer()->resourceUpdate(noiseRub);
        m_blueNoiseSampler.reset(rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                                  QRhiSampler::Repeat, QRhiSampler::Repeat));
        m_blueNoiseSampler->create();
    }

    if (!m_ies1x1Fallback) {
        m_ies1x1Fallback.reset(rhi->newTexture(QRhiTexture::RGBA8, QSize(1, 1), 1, QRhiTexture::Flags{}));
        m_ies1x1Fallback->create();
        const quint8 white[4] = { 255, 255, 255, 255 };
        QRhiResourceUpdateBatch *rub = rhi->nextResourceUpdateBatch();
        QRhiTextureUploadEntry entry(0, 0, QRhiTextureSubresourceUploadDescription(white, 4));
        rub->uploadTexture(m_ies1x1Fallback.get(), QRhiTextureUploadDescription(entry));
        rhiCtx->commandBuffer()->resourceUpdate(rub);

        m_iesSampler.reset(rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                           QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
        m_iesSampler->create();
    }

    resourcesDirty  = false;
    m_pipelineDirty = true;
}

void FroxelRenderer::setupComputePipeline(const QSSGRhiContext *rhiCtx, QRhiTexture *shadowAtlas)
{
    if (shadowAtlas != m_shadowAtlas)
        m_pipelineDirty = true;
    if (!m_pipelineDirty)
        return;

    QRhi *rhi = rhiCtx->rhi();
    if (!rhi->isFeatureSupported(QRhi::Compute)) {
        qWarning("FroxelRenderer: compute shaders not supported on this backend");
        return;
    }

    m_computePipeline.reset();
    m_computeBindings.reset();
    m_atlasSampler.reset();
    m_shadowAtlas = nullptr;

    QShader computeShader;
    {
        QFile f(QStringLiteral(":/shaders/froxellightinjection.comp.qsb"));
        if (!f.open(QIODevice::ReadOnly)) {
            qWarning("FroxelRenderer: could not open froxellightinjection.comp.qsb");
            return;
        }
        computeShader = QShader::fromSerialized(f.readAll());
    }
    Q_ASSERT(computeShader.isValid());

    m_shadowAtlas = shadowAtlas;

    m_atlasSampler.reset(rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                         QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
    m_atlasSampler->create();

    QRhiTexture *iesForBinding = m_iesRhiTexture ? m_iesRhiTexture : m_ies1x1Fallback.get();

    m_computeBindings.reset(rhi->newShaderResourceBindings());
    m_computeBindings->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::ComputeStage, m_froxelUniformBuffer.get()),
        QRhiShaderResourceBinding::uniformBuffer(1, QRhiShaderResourceBinding::ComputeStage, m_lightDataBuffer.get()),
        QRhiShaderResourceBinding::uniformBuffer(2, QRhiShaderResourceBinding::ComputeStage, m_volumeDataBuffer.get()),
        QRhiShaderResourceBinding::imageStore(3, QRhiShaderResourceBinding::ComputeStage, m_froxelRhiTexture.get(), 0),
        QRhiShaderResourceBinding::sampledTexture(4, QRhiShaderResourceBinding::ComputeStage, m_shadowAtlas, m_atlasSampler.get()),
        QRhiShaderResourceBinding::sampledTexture(5, QRhiShaderResourceBinding::ComputeStage, m_blueNoiseTexture.get(), m_blueNoiseSampler.get()),
        QRhiShaderResourceBinding::sampledTexture(6, QRhiShaderResourceBinding::ComputeStage, iesForBinding, m_iesSampler.get()),
    });
    if (!m_computeBindings->create()) {
        qWarning("FroxelRenderer: failed to create compute shader resource bindings");
        return;
    }

    m_computePipeline.reset(rhi->newComputePipeline());
    m_computePipeline->setShaderResourceBindings(m_computeBindings.get());
    m_computePipeline->setShaderStage({ QRhiShaderStage::Compute, computeShader });
    if (!m_computePipeline->create()) {
        qWarning("FroxelRenderer: failed to create compute pipeline");
        return;
    }

    m_pipelineDirty = false;
}

void FroxelRenderer::updateUniforms(const QSSGRhiContext *rhiCtx, QSSGLayerRenderData *layer, QSSGRenderCamera *camera)
{
    static_assert(alignof(FroxelUniforms)  == 16, "FroxelUniforms alignment mismatch");
    static_assert(alignof(LightBufferData) == 16, "LightBufferData alignment mismatch");

    QRhiResourceUpdateBatch *rub = rhiCtx->rhi()->nextResourceUpdateBatch();

    QMatrix4x4 cameraGlobalTransform = layer->getGlobalTransform(*camera);
    QMatrix4x4 viewProjection(Qt::Uninitialized);
    camera->calculateViewProjectionMatrix(cameraGlobalTransform, viewProjection);
    QMatrix4x4 invViewProjection = viewProjection.inverted();

    FroxelUniforms uniforms = {};
    memcpy(uniforms.invViewProjection, invViewProjection.constData(), 64);
    memcpy(uniforms.viewMatrix, cameraGlobalTransform.inverted().constData(), 64);

    const auto globalRenderData = QSSGLayerRenderData::globalRenderProperties(*layer->contextInterface());
    uniforms.qt_rhi_properties[0] = globalRenderData.isYUpInFramebuffer ? 1.0f : -1.0f;
    uniforms.qt_rhi_properties[1] = globalRenderData.isYUpInNDC ? 1.0f : -1.0f;

    uniforms.gridDimensions[0] = float(config.width);
    uniforms.gridDimensions[1] = float(config.height);
    uniforms.gridDimensions[2] = float(config.depth);

    uniforms.depthParams[0] = config.nearPlane;
    uniforms.depthParams[1] = config.farPlane;

    const QVector3D camPos = cameraGlobalTransform.column(3).toVector3D();
    uniforms.cameraPos[0] = camPos.x();
    uniforms.cameraPos[1] = camPos.y();
    uniforms.cameraPos[2] = camPos.z();

    rub->updateDynamicBuffer(m_froxelUniformBuffer.get(), 0, sizeof(FroxelUniforms), &uniforms);

    LightBufferData lightData = {};

    lightData.iesIndices[0] = QVector4D(-1.f, -1.f, -1.f, -1.f);
    lightData.iesIndices[1] = QVector4D(-1.f, -1.f, -1.f, -1.f);

    const bool iesEnabled = config.ies.renderImage != nullptr && config.ies.count > 0;
    lightData.iesParams = QVector4D(iesEnabled ? 1.0f / float(config.ies.count) : 0.0f,
                                    0.f, 0.f, 0.f);

    int lightCount = 0;

    for (quint32 i = 0, end = qMin(QSSG_MAX_NUM_LIGHTS, (int)layer->globalLights.size()); i < end; ++i) {
        QSSGRenderLight *light = layer->globalLights[i].light;
        const bool shadows = layer->globalLights[i].shadows;
        if (light->m_bakingEnabled)
            continue;

        if (light->type == QSSGRenderGraphObject::Type::DirectionalLight) {
            if (shadows) {
                auto *shadowMap = layer->getShadowMapManager()->shadowMapEntry(i);
                if (shadowMap) {
                    const bool noCascades = !(shadowMap->m_csmActive[0] || shadowMap->m_csmActive[1]
                                              || shadowMap->m_csmActive[2] || shadowMap->m_csmActive[3]);
                    if (!noCascades) {
                        const quint32 layerCount = shadowMap->m_csmNumSplits + 1;
                        for (quint32 k = 0; k < layerCount; ++k)
                            memcpy(lightData.matrices[k], shadowMap->m_fixedScaleBiasMatrix[k].constData(), 64);
                        lightData.shadowBias = light->m_shadowBias;
                        lightData.shadowFactor = qBound(0.0f, light->m_shadowFactor, 100.0f) * 0.02f;
                        memcpy(lightData.csmSplits, shadowMap->m_csmSplits, 4 * sizeof(float));
                        memcpy(lightData.csmActive, shadowMap->m_csmActive, 4 * sizeof(float));
                        for (quint32 k = 0; k < layerCount; ++k) {
                            const auto &a = shadowMap->m_atlasInfo[k];
                            lightData.atlasLocations[k][0] = a.uOffset;
                            lightData.atlasLocations[k][1] = a.vOffset;
                            lightData.atlasLocations[k][2] = a.uvScale;
                            lightData.atlasLocations[k][3] = a.layerIndex;
                            memcpy(lightData.dimensionsInverted[k], &shadowMap->m_dimensionsInverted[k], 4 * sizeof(float));
                        }
                    }
                }
            }
            lightData.dirLightColor = QVector4D(light->m_diffuseColor, light->m_brightness);

        } else if (light->type == QSSGRenderGraphObject::Type::PointLight) {
            const QMatrix4x4 lightTransform = layer->getGlobalTransform(*light);
            const QVector3D pos = QSSGRenderNode::getGlobalPos(lightTransform);
            const IESLightEntry iesEntry = iesEnabled ? config.ies.lightMap.value(light) : IESLightEntry{};
            lightData.lightPos[lightCount] = QVector4D(pos, 1.0f);
            lightData.lightColor[lightCount] = QVector4D(light->m_diffuseColor, light->m_brightness * iesEntry.intensity);
            lightData.lightDirection[lightCount] = QVector4D(-lightTransform.column(1).toVector3D().normalized(), 0.0f);
            if (shadows) {
                auto *shadowMap = layer->getShadowMapManager()->shadowMapEntry(i);
                if (shadowMap) {
                    lightData.shadowBias   = light->m_shadowBias;
                    lightData.shadowFactor = qBound(0.0f, light->m_shadowFactor, 100.0f) * 0.02f;
                    memcpy(lightData.lightShadowMat[lightCount], shadowMap->m_lightView.constData(), 64);
                    const auto &af = shadowMap->m_atlasInfo[0];
                    lightData.lightShadowAtlas[lightCount][0] = af.uOffset;
                    lightData.lightShadowAtlas[lightCount][1] = af.vOffset;
                    lightData.lightShadowAtlas[lightCount][2] = af.uvScale;
                    lightData.lightShadowAtlas[lightCount][3] = float(af.layerIndex);
                    const auto &ab = shadowMap->m_atlasInfo[1];
                    lightData.lightShadowAtlas2[lightCount][0] = ab.uOffset;
                    lightData.lightShadowAtlas2[lightCount][1] = ab.vOffset;
                    lightData.lightShadowAtlas2[lightCount][2] = ab.uvScale;
                    lightData.lightShadowAtlas2[lightCount][3] = float(ab.layerIndex);
                    lightData.coneAngles[lightCount].setZ(2.0f);
                    lightData.coneAngles[lightCount].setW(shadowMap->m_shadowMapFar);
                }
            }
            if (iesEnabled) {
                if (lightCount < 4) lightData.iesIndices[0][lightCount] = float(iesEntry.index);
                else lightData.iesIndices[1][lightCount - 4] = float(iesEntry.index);
            }
            ++lightCount;

        } else {
            const QVector3D direction = layer->globalLights[i].direction;
            const QVector3D pos = QSSGRenderNode::getGlobalPos(layer->getGlobalTransform(*light));
            const IESLightEntry iesEntry = iesEnabled ? config.ies.lightMap.value(light) : IESLightEntry{};
            lightData.lightPos[lightCount] = QVector4D(pos, 1.0f);
            lightData.lightColor[lightCount] = QVector4D(light->m_diffuseColor, light->m_brightness * iesEntry.intensity);
            lightData.lightDirection[lightCount] = QVector4D(direction, 1.0f);
            lightData.coneAngles[lightCount] = QVector4D(qCos(qDegreesToRadians(light->m_coneAngle)),
                                                             qCos(qDegreesToRadians(light->m_innerConeAngle)),
                                                             0.0f, 0.0f);
            if (shadows) {
                auto *shadowMap = layer->getShadowMapManager()->shadowMapEntry(i);
                if (shadowMap) {
                    memcpy(lightData.lightShadowMat[lightCount], shadowMap->m_fixedScaleBiasMatrix[0].constData(), 64);
                    const auto &a = shadowMap->m_atlasInfo[0];
                    lightData.shadowBias   = light->m_shadowBias;
                    lightData.shadowFactor = qBound(0.0f, light->m_shadowFactor, 100.0f) * 0.02f;
                    lightData.lightShadowAtlas[lightCount][0] = a.uOffset;
                    lightData.lightShadowAtlas[lightCount][1] = a.vOffset;
                    lightData.lightShadowAtlas[lightCount][2] = a.uvScale;
                    lightData.lightShadowAtlas[lightCount][3] = float(a.layerIndex);
                    lightData.coneAngles[lightCount].setZ(1.0f);
                    lightData.coneAngles[lightCount].setW(shadowMap->m_shadowMapFar);
                }
            }
            if (iesEnabled) {
                if (lightCount < 4) lightData.iesIndices[0][lightCount] = float(iesEntry.index);
                else lightData.iesIndices[1][lightCount - 4] = float(iesEntry.index);
            }
            ++lightCount;
        }
    }
    lightData.numLights = lightCount;
    rub->updateDynamicBuffer(m_lightDataBuffer.get(), 0, sizeof(LightBufferData), &lightData);

    FogVolumes volumes = {};
    int fvcount = 0;
    for (const auto &fv : std::as_const(config.fogVolumeData)) {
        if (fvcount >= MAX_VOLUMES) break;
        memcpy(volumes.matrices[fvcount], fv.invTransform.constData(), 64);
        volumes.extents[fvcount] = QVector4D(fv.extents, float(fv.type));
        volumes.color[fvcount] = QVector4D(fv.color.redF(), fv.color.greenF(), fv.color.blueF(), fv.density);
        volumes.heightParams[fvcount] = QVector4D(fv.leastIntenseY, fv.mostIntenseY, fv.heightCurve, fv.heightEnabled ? 1.0f : 0.0f);
        volumes.noiseOffsetScale[fvcount] = QVector4D(fv.noiseOffset, fv.noiseScale);
        ++fvcount;
    }
    volumes.numVolumes = fvcount;
    rub->updateDynamicBuffer(m_volumeDataBuffer.get(), 0, sizeof(FogVolumes), &volumes);

    rhiCtx->commandBuffer()->resourceUpdate(rub);
}

bool FroxelRenderer::prepareData(QSSGFrameData &data)
{
    const auto ctx = data.contextInterface();
    auto *layer = QSSGLayerRenderData::getCurrent(*ctx->renderer());

    if (!layer || layer->renderedCameras.isEmpty()) {
        m_enabled = false;
        return false;
    }

    m_enabled = true;

    const auto &rhiCtx = ctx->rhiContext();
    setupFroxelGrid(rhiCtx.get());

    {
        QRhiTexture *iesRhiTex = m_ies1x1Fallback.get();
        if (config.ies.renderImage) {
            QSSGRenderImageTexture texData =
                ctx->bufferManager()->loadRenderImage(config.ies.renderImage);
            if (texData.m_texture)
                iesRhiTex = texData.m_texture;
        }
        if (iesRhiTex != m_iesRhiTexture) {
            m_iesRhiTexture = iesRhiTex;
            m_pipelineDirty = true;
        }
    }

    auto mgr = layer->getShadowMapManager();
    setupComputePipeline(rhiCtx.get(), mgr ? mgr->shadowMapAtlasTexture() : nullptr);
    updateUniforms(rhiCtx.get(), layer, layer->renderedCameras[0]);

    return true;
}

void FroxelRenderer::prepareRender(QSSGFrameData &data)
{
    Q_UNUSED(data)
}

void FroxelRenderer::render(QSSGFrameData &data)
{
    if (!m_enabled || !m_computePipeline || !m_computeBindings)
        return;

    const auto &rhiCtx = data.contextInterface()->rhiContext();
    QRhiCommandBuffer *cb = rhiCtx->commandBuffer();

    cb->debugMarkBegin(QByteArrayLiteral("Quick3D volumetric light (froxel injection)"));
    cb->beginComputePass();
    cb->setComputePipeline(m_computePipeline.get());
    cb->setShaderResources(m_computeBindings.get());
    cb->dispatch((config.width  + 7) / 8,
                 (config.height + 7) / 8,
                  config.depth);
    cb->endComputePass();
    cb->debugMarkEnd();
}

void FroxelRenderer::resetForFrame()
{
    m_enabled = false;
}

VolumetricFogExtension::VolumetricFogExtension(QQuick3DObject *parent)
    : QQuick3DRenderExtension(parent)
{
    m_froxelTexture = new QQuick3DTexture(this);
    m_froxelTexture->setHorizontalTiling(QQuick3DTexture::ClampToEdge);
    m_froxelTexture->setVerticalTiling(QQuick3DTexture::ClampToEdge);
    m_froxelTexture->setDepthTiling(QQuick3DTexture::ClampToEdge);
    m_froxelTexture->setTextureData(new QQuick3DTextureData(m_froxelTexture));
    m_froxelTexture->textureData()->setDepth(1);
}

QSSGRenderGraphObject *VolumetricFogExtension::updateSpatialNode(QSSGRenderGraphObject *node)
{
    if (!node)
        node = new FroxelRenderer;

    auto *renderer = static_cast<FroxelRenderer *>(node);
    renderer->frontendTexture = m_froxelTexture;

    if (m_dirtyFlag & Config) {
        const bool gridChanged = renderer->config.width != m_froxelWidth
                              || renderer->config.height != m_froxelHeight
                              || renderer->config.depth != m_froxelDepth;
        renderer->config.width = m_froxelWidth;
        renderer->config.height = m_froxelHeight;
        renderer->config.depth = m_froxelDepth;
        renderer->config.nearPlane = m_nearPlane;
        renderer->config.farPlane = m_farPlane;

        if (gridChanged)
            renderer->resourcesDirty = true;
        m_dirtyFlag &= ~static_cast<DirtyT>(Config);
    }
    if (m_dirtyFlag & Volumes) {
        renderer->config.fogVolumeData.clear();
        for (auto *fv : std::as_const(m_fogVolumes)) {
            FogVolumeEntry entry;
            entry.invTransform = fv->sceneTransform().inverted();
            entry.extents = fv->extents();
            entry.type = int(fv->type());
            entry.color = fv->color();
            entry.density = fv->density();
            entry.heightEnabled = fv->heightEnabled();
            entry.leastIntenseY = fv->leastIntenseY();
            entry.mostIntenseY = fv->mostIntenseY();
            entry.heightCurve = fv->heightCurve();
            entry.noiseOffset = fv->noiseOffset();
            entry.noiseScale = fv->noiseScale();
            renderer->config.fogVolumeData.append(entry);
        }
        m_dirtyFlag &= ~static_cast<DirtyT>(Volumes);
    }
    if (m_dirtyFlag & IES) {
        bool nodeReady = true;
        QSSGRenderImage *img = nullptr;
        if (m_iesQmlTexture) {
            auto *priv = QQuick3DObjectPrivate::get(m_iesQmlTexture);
            if (priv && priv->spatialNode) {
                img = static_cast<QSSGRenderImage *>(priv->spatialNode);
            } else {
                nodeReady = false;
                update();
            }
        }
        if (nodeReady) {
            QHash<QSSGRenderLight *, IESLightEntry> lightMap;
            for (auto *mapping : std::as_const(m_iesLightProfiles)) {
                if (!mapping->light())
                    continue;
                auto *quick3dObj = qobject_cast<QQuick3DObject *>(mapping->light());
                if (!quick3dObj)
                    continue;
                auto *lpriv = QQuick3DObjectPrivate::get(quick3dObj);
                if (!lpriv || !lpriv->spatialNode) {
                    nodeReady = false;
                    update();
                    break;
                }
                lightMap.insert(static_cast<QSSGRenderLight *>(lpriv->spatialNode),
                                IESLightEntry{ mapping->index(), mapping->intensity() });
            }
            if (nodeReady) {
                renderer->config.ies.renderImage = img;
                renderer->config.ies.count = m_iesCount;
                renderer->config.ies.lightMap = std::move(lightMap);
                renderer->m_pipelineDirty = true;
                m_dirtyFlag &= ~static_cast<DirtyT>(IES);
            }
        }
    }

    return node;
}

void VolumetricFogExtension::markDirty(Dirty v)
{
    m_dirtyFlag |= v;
    update();
}

QQuick3DTexture *VolumetricFogExtension::froxelTexture() const { return m_froxelTexture; }
int VolumetricFogExtension::froxelWidth() const { return m_froxelWidth; }
int VolumetricFogExtension::froxelHeight() const { return m_froxelHeight; }
int VolumetricFogExtension::froxelDepth() const { return m_froxelDepth; }
float VolumetricFogExtension::nearPlane() const { return m_nearPlane; }
float VolumetricFogExtension::farPlane() const { return m_farPlane; }

void VolumetricFogExtension::setFroxelWidth(int v)
{
    if (m_froxelWidth == v) return;
    m_froxelWidth = v; markDirty(Config); emit froxelWidthChanged();
}
void VolumetricFogExtension::setFroxelHeight(int v)
{
    if (m_froxelHeight == v) return;
    m_froxelHeight = v; markDirty(Config); emit froxelHeightChanged();
}
void VolumetricFogExtension::setFroxelDepth(int v)
{
    if (m_froxelDepth == v) return;
    m_froxelDepth = v; markDirty(Config); emit froxelDepthChanged();
}
void VolumetricFogExtension::setNearPlane(float v)
{
    if (qFuzzyCompare(m_nearPlane, v)) return;
    m_nearPlane = v; markDirty(Config); emit nearPlaneChanged();
}
void VolumetricFogExtension::setFarPlane(float v)
{
    if (qFuzzyCompare(m_farPlane, v)) return;
    m_farPlane = v; markDirty(Config); emit farPlaneChanged();
}

QQuick3DTexture *VolumetricFogExtension::iesTexture() const { return m_iesQmlTexture; }
void VolumetricFogExtension::setIesTexture(QQuick3DTexture *texture)
{
    if (m_iesQmlTexture == texture) return;
    m_iesQmlTexture = texture;
    markDirty(IES);
    emit iesTextureChanged();
}

int VolumetricFogExtension::iesCount() const { return m_iesCount; }
void VolumetricFogExtension::setIesCount(int count)
{
    if (m_iesCount == count) return;
    m_iesCount = qMax(1, count);
    markDirty(IES);
    emit iesCountChanged();
}

QQmlListProperty<IESLightProfileIndex> VolumetricFogExtension::iesLightProfiles()
{
    return QQmlListProperty<IESLightProfileIndex>(this, nullptr,
                                                  appendIESLightProfile, iesLightProfileCount,
                                                  iesLightProfileAt, clearIESLightProfiles);
}

void VolumetricFogExtension::appendIESLightProfile(QQmlListProperty<IESLightProfileIndex> *list, IESLightProfileIndex *v)
{
    auto *self = static_cast<VolumetricFogExtension *>(list->object);
    self->m_iesLightProfiles.append(v);
    connect(v, &IESLightProfileIndex::lightChanged, self, [self]{ self->markDirty(IES); });
    connect(v, &IESLightProfileIndex::indexChanged, self, [self]{ self->markDirty(IES); });
    connect(v, &IESLightProfileIndex::intensityChanged, self, [self]{ self->markDirty(IES); });
    self->markDirty(IES);
    emit self->iesLightProfilesChanged();
}

qsizetype VolumetricFogExtension::iesLightProfileCount(QQmlListProperty<IESLightProfileIndex> *list)
{
    return static_cast<VolumetricFogExtension *>(list->object)->m_iesLightProfiles.size();
}

IESLightProfileIndex *VolumetricFogExtension::iesLightProfileAt(QQmlListProperty<IESLightProfileIndex> *list, qsizetype i)
{
    return static_cast<VolumetricFogExtension *>(list->object)->m_iesLightProfiles.at(i);
}

void VolumetricFogExtension::clearIESLightProfiles(QQmlListProperty<IESLightProfileIndex> *list)
{
    auto *self = static_cast<VolumetricFogExtension *>(list->object);
    for (auto *v : std::as_const(self->m_iesLightProfiles))
        disconnect(v, nullptr, self, nullptr);
    self->m_iesLightProfiles.clear();
    self->markDirty(IES);
    emit self->iesLightProfilesChanged();
}

QQmlListProperty<Fog3DVolume> VolumetricFogExtension::fogVolumes()
{
    return QQmlListProperty<Fog3DVolume>(this, nullptr,
                                        appendFogVolume, fogVolumeCount,
                                        fogVolumeAt, clearFogVolumes);
}

void VolumetricFogExtension::appendFogVolume(QQmlListProperty<Fog3DVolume> *list, Fog3DVolume *v)
{
    auto *self = static_cast<VolumetricFogExtension *>(list->object);
    self->m_fogVolumes.append(v);
    connect(v, &Fog3DVolume::typeChanged, self, [self]{ self->markDirty(Volumes); });
    connect(v, &Fog3DVolume::extentsChanged, self, [self]{ self->markDirty(Volumes); });
    connect(v, &Fog3DVolume::colorChanged, self, [self]{ self->markDirty(Volumes); });
    connect(v, &Fog3DVolume::densityChanged, self, [self]{ self->markDirty(Volumes); });
    connect(v, &Fog3DVolume::heightEnabledChanged, self, [self]{ self->markDirty(Volumes); });
    connect(v, &Fog3DVolume::leastIntenseYChanged, self, [self]{ self->markDirty(Volumes); });
    connect(v, &Fog3DVolume::mostIntenseYChanged, self, [self]{ self->markDirty(Volumes); });
    connect(v, &Fog3DVolume::heightCurveChanged, self, [self]{ self->markDirty(Volumes); });
    connect(v, &Fog3DVolume::noiseOffsetChanged, self, [self]{ self->markDirty(Volumes); });
    connect(v, &Fog3DVolume::noiseScaleChanged, self, [self]{ self->markDirty(Volumes); });
    connect(v, &QQuick3DNode::sceneTransformChanged, self, [self]{ self->markDirty(Volumes); });
    self->markDirty(Volumes);
    emit self->fogVolumesChanged();
}

qsizetype VolumetricFogExtension::fogVolumeCount(QQmlListProperty<Fog3DVolume> *list)
{
    return static_cast<VolumetricFogExtension *>(list->object)->m_fogVolumes.size();
}

Fog3DVolume *VolumetricFogExtension::fogVolumeAt(QQmlListProperty<Fog3DVolume> *list, qsizetype i)
{
    return static_cast<VolumetricFogExtension *>(list->object)->m_fogVolumes.at(i);
}

void VolumetricFogExtension::clearFogVolumes(QQmlListProperty<Fog3DVolume> *list)
{
    auto *self = static_cast<VolumetricFogExtension *>(list->object);
    for (auto *v : std::as_const(self->m_fogVolumes))
        disconnect(v, nullptr, self, nullptr);
    self->m_fogVolumes.clear();
    self->markDirty(Volumes);
    emit self->fogVolumesChanged();
}
