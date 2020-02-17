/****************************************************************************
**
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

#ifndef QSSGRHICONTEXT_P_H
#define QSSGRHICONTEXT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qtquick3drenderglobal_p.h"
#include <QtCore/qstack.h>
#include <QtQuick3DRender/private/qssgrenderbasetypes_p.h>
#include <QtGui/private/qrhi_p.h>

QT_BEGIN_NAMESPACE

class QSSGRhiContext;
class QSSGRhiBuffer;
class QSSGRhiShaderStagesWithResources;
struct QSSGShaderLightProperties;

struct Q_QUICK3DRENDER_EXPORT QSSGRhiInputAssemblerState
{
    QRhiVertexInputLayout inputLayout;
    QVector<QByteArray> inputLayoutInputNames;
    QRhiGraphicsPipeline::Topology topology;
    QSSGRef<QSSGRhiBuffer> vertexBuffer;
    QSSGRef<QSSGRhiBuffer> indexBuffer;

    static QRhiVertexInputAttribute::Format toVertexInputFormat(QSSGRenderComponentType compType, quint32 numComps);
    static QRhiGraphicsPipeline::Topology toTopology(QSSGRenderDrawMode drawMode);

    // Fills out inputLayout.attributes[].location based on
    // inputLayoutInputNames and the provided shader reflection info (unless
    // already done for the same 'shaders')
    void bakeVertexInputLocations(const QSSGRhiShaderStagesWithResources &shaders);
    const void *lastBakeVertexInputKey = nullptr;
    QVector<QByteArray> lastBakeVertexInputNames;
};

class Q_QUICK3DRENDER_EXPORT QSSGRhiBuffer
{
    Q_DISABLE_COPY(QSSGRhiBuffer)
public:
    QAtomicInt ref;

    QSSGRhiBuffer(const QSSGRef<QSSGRhiContext> &context,
                  QRhiBuffer::Type type,
                  QRhiBuffer::UsageFlags usageMask,
                  quint32 stride,
                  int size,
                  QRhiCommandBuffer::IndexFormat indexFormat = QRhiCommandBuffer::IndexUInt16);

    virtual ~QSSGRhiBuffer();

    QRhiBuffer *buffer() const { return m_buffer; }
    quint32 stride() const { return m_stride; }
    quint32 numVertices() const {
        const quint32 sz = quint32(m_buffer->size());
        Q_ASSERT((sz % m_stride) == 0);
        return sz / m_stride;
    }
    QRhiCommandBuffer::IndexFormat indexFormat() const { return m_indexFormat; }

protected:
    QSSGRef<QSSGRhiContext> m_context;
    QRhiBuffer *m_buffer = nullptr;
    quint32 m_stride;
    QRhiCommandBuffer::IndexFormat m_indexFormat;
};

class Q_QUICK3DRENDER_EXPORT QSSGRhiShaderStages
{
    Q_DISABLE_COPY(QSSGRhiShaderStages)
public:
    QAtomicInt ref;

    QSSGRhiShaderStages(const QSSGRef<QSSGRhiContext> &context);

    QSSGRef<QSSGRhiContext> context() const { return m_context; }
    bool isNull() const { return m_stages.isEmpty(); }

    void addStage(const QRhiShaderStage &stage) { m_stages.append(stage); }
    const QVector<QRhiShaderStage> &stages() const { return m_stages; }

    const QRhiShaderStage *vertexStage() const {
        for (const QRhiShaderStage &s : m_stages) {
            if (s.type() == QRhiShaderStage::Vertex)
                return &s;
        }
        return nullptr;
    }
    const QRhiShaderStage *fragmentStage() const {
        for (const QRhiShaderStage &s : m_stages) {
            if (s.type() == QRhiShaderStage::Fragment)
                return &s;
        }
        return nullptr;
    }

private:
    QSSGRef<QSSGRhiContext> m_context;
    QVector<QRhiShaderStage> m_stages;
};

struct QSSGRhiShaderUniform
{
    QString name; // because QShaderDescription will have a QString, not QByteArray
    bool dirty = false;
    size_t size = 0;
    char data[256];

private:
    size_t offset = SIZE_MAX;
    friend class QSSGRhiShaderStagesWithResources;
};

// these are our current shader limits
#define QSSG_MAX_NUM_LIGHTS 15
#define QSSG_MAX_NUM_SHADOWS 8

// note this struct must exactly match the memory layout of the
// struct sampleLight.glsllib and sampleArea.glsllib. If you make changes here you need
// to adjust the code in sampleLight.glsllib and sampleArea.glsllib as well
struct QSSGLightSourceShader
{
    QVector4D position;
    QVector4D direction; // Specifies the light direction in world coordinates.
    QVector4D up;
    QVector4D right;
    QVector4D diffuse;
    QVector4D ambient;
    QVector4D specular;
    float coneAngle; // Specifies the outer cone angle of the spot light.
    float innerConeAngle; // Specifies the inner cone angle of the spot light.
    float constantAttenuation; // Specifies the constant light attenuation factor.
    float linearAttenuation; // Specifies the linear light attenuation factor.
    float quadraticAttenuation; // Specifies the quadratic light attenuation factor.
    float range; // Specifies the maximum distance of the light influence
    float width; // Specifies the width of the area light surface.
    float height; // Specifies the height of the area light surface;
    QVector4D shadowControls;
    float shadowView[16];
    qint32 shadowIdx;
    float padding1[3];
};

struct QSSGShaderLightProperties
{
    QVector3D lightColor;
    QSSGLightSourceShader lightData;
};

struct QSSGRhiShadowMapProperties
{
    QRhiTexture *shadowMapTexture = nullptr;
    QByteArray shadowMapTextureUniformName;
    int cachedBinding = -1; // -1 == invalid
};

class Q_QUICK3DRENDER_EXPORT QSSGRhiShaderStagesWithResources
{
    Q_DISABLE_COPY(QSSGRhiShaderStagesWithResources)
public:
    QAtomicInt ref;

    static QSSGRef<QSSGRhiShaderStagesWithResources> fromShaderStages(const QSSGRef<QSSGRhiShaderStages> &stages,
                                                                      const QByteArray &shaderKeyString);

    const QSSGRhiShaderStages *stages() const { return m_shaderStages.data(); }

    void setUniform(const QByteArray &name, const void *data, size_t size);
    void dumpUniforms();

    void resetLights() { m_lights.clear(); }
    QSSGShaderLightProperties &addLight() { m_lights.append(QSSGShaderLightProperties()); return m_lights.last(); }
    int lightCount() const { return m_lights.count(); }
    const QSSGShaderLightProperties &lightAt(int index) const { return m_lights[index]; }
    QSSGShaderLightProperties &lightAt(int index) { return m_lights[index]; }
    void setLightsEnabled(bool enable) { m_lightsEnabled = enable; }
    bool isLightingEnabled() const { return m_lightsEnabled; }

    void resetShadowMaps() { m_shadowMaps.clear(); }
    QSSGRhiShadowMapProperties &addShadowMap() { m_shadowMaps.append(QSSGRhiShadowMapProperties()); return m_shadowMaps.last(); }
    int shadowMapCount() const { return m_shadowMaps.count(); }
    const QSSGRhiShadowMapProperties &shadowMapAt(int index) const { return m_shadowMaps[index]; }
    QSSGRhiShadowMapProperties &shadowMapAt(int index) { return m_shadowMaps[index]; }

    void bakeMainUniformBuffer(QRhiBuffer **ubuf, QRhiResourceUpdateBatch *resourceUpdates);
    void bakeLightsUniformBuffer(QRhiBuffer **ubuf, QRhiResourceUpdateBatch *resourceUpdates);

    void setLightProbeTexture(QRhiTexture *texture,
                              QSSGRenderTextureCoordOp hTile = QSSGRenderTextureCoordOp::ClampToEdge,
                              QSSGRenderTextureCoordOp vTile = QSSGRenderTextureCoordOp::ClampToEdge)
    {
        m_lightProbeTexture = texture; m_lightProbeHorzTile = hTile; m_lightProbeVertTile = vTile;
    }
    QRhiTexture *lightProbeTexture() const { return m_lightProbeTexture; }
    QPair<QSSGRenderTextureCoordOp, QSSGRenderTextureCoordOp> lightProbeTiling() const
    {
        return {m_lightProbeHorzTile, m_lightProbeVertTile};
    }

    void setDepthTexture(QRhiTexture *texture) { m_depthTexture = texture; }
    QRhiTexture *depthTexture() const { return m_depthTexture; }

    void setSsaoTexture(QRhiTexture *texture) { m_ssaoTexture = texture; }
    QRhiTexture *ssaoTexture() const { return m_ssaoTexture; }

    QSSGRhiShaderStagesWithResources(QSSGRef<QSSGRhiShaderStages> shaderStages, const QByteArray &shaderKeyString)
        : m_context(shaderStages->context()),
          m_shaderKeyString(shaderKeyString),
          m_shaderStages(shaderStages)
    {
    }

protected:
    QSSGRef<QSSGRhiContext> m_context;
    QByteArray m_shaderKeyString;
    QSSGRef<QSSGRhiShaderStages> m_shaderStages;
    QHash<QByteArray, QSSGRhiShaderUniform> m_uniforms; // members of the main (binding 0) uniform buffer
    bool m_lightsEnabled = false;
    QVarLengthArray<QSSGShaderLightProperties, QSSG_MAX_NUM_LIGHTS> m_lights;
    QVarLengthArray<QSSGRhiShadowMapProperties, QSSG_MAX_NUM_SHADOWS> m_shadowMaps;
    QRhiTexture *m_lightProbeTexture = nullptr; // TODO: refcount (?)
    QSSGRenderTextureCoordOp m_lightProbeHorzTile = QSSGRenderTextureCoordOp::ClampToEdge;
    QSSGRenderTextureCoordOp m_lightProbeVertTile = QSSGRenderTextureCoordOp::ClampToEdge;
    QRhiTexture *m_depthTexture = nullptr; // not owned
    QRhiTexture *m_ssaoTexture = nullptr; // not owned
};

struct Q_QUICK3DRENDER_EXPORT QSSGRhiGraphicsPipelineState
{
    const QSSGRhiShaderStages *shaderStages;
    int samples = 1;

    bool depthTestEnable = false;
    bool depthWriteEnable = false;
    QRhiGraphicsPipeline::CompareOp depthFunc = QRhiGraphicsPipeline::LessOrEqual;
    QRhiGraphicsPipeline::CullMode cullMode = QRhiGraphicsPipeline::None;
    int depthBias = 0;
    float slopeScaledDepthBias = 0.0f;
    bool blendEnable = false;
    QRhiGraphicsPipeline::TargetBlend targetBlend;
    int colorAttachmentCount = 1;

    QRhiViewport viewport;
    bool scissorEnable = false;
    QRhiScissor scissor;

    QSSGRhiInputAssemblerState ia;

    static QRhiGraphicsPipeline::CullMode toCullMode(QSSGCullFaceMode cullFaceMode);
};

bool operator==(const QSSGRhiGraphicsPipelineState &a, const QSSGRhiGraphicsPipelineState &b) Q_DECL_NOTHROW;
bool operator!=(const QSSGRhiGraphicsPipelineState &a, const QSSGRhiGraphicsPipelineState &b) Q_DECL_NOTHROW;
uint qHash(const QSSGRhiGraphicsPipelineState &s, uint seed = 0) Q_DECL_NOTHROW;

struct QSSGGraphicsPipelineStateKey
{
    QSSGRhiGraphicsPipelineState state;
    QRhiRenderPassDescriptor *compatibleRpDesc;
    QRhiShaderResourceBindings *layoutCompatibleSrb;
};

bool operator==(const QSSGGraphicsPipelineStateKey &a, const QSSGGraphicsPipelineStateKey &b) Q_DECL_NOTHROW;
bool operator!=(const QSSGGraphicsPipelineStateKey &a, const QSSGGraphicsPipelineStateKey &b) Q_DECL_NOTHROW;
uint qHash(const QSSGGraphicsPipelineStateKey &k, uint seed = 0) Q_DECL_NOTHROW;

struct QSSGComputePipelineStateKey
{
    QShader shader;
    QRhiShaderResourceBindings *layoutCompatibleSrb;
};

inline bool operator==(const QSSGComputePipelineStateKey &a, const QSSGComputePipelineStateKey &b) Q_DECL_NOTHROW
{
    return a.shader == b.shader && a.layoutCompatibleSrb->isLayoutCompatible(b.layoutCompatibleSrb);
}

inline bool operator!=(const QSSGComputePipelineStateKey &a, const QSSGComputePipelineStateKey &b) Q_DECL_NOTHROW
{
    return !(a == b);
}

inline uint qHash(const QSSGComputePipelineStateKey &k, uint seed = 0) Q_DECL_NOTHROW
{
    return qHash(k.shader, seed);
}

// QSSGRhiContext acts as an owning container for various graphics resources,
// including uniform buffers.
//
// The lookup keys can be somewhat complicated due to having to handle cases
// like "render a model in a shared scene between multiple View3Ds" (here both
// the View3D ('layer') and the model ('model') act as the lookup key since
// while the model is the same, we still want different uniform buffers per
// View3D), or the case of shadow maps where the shadow map (there can be as
// many as lights) is taken into account too ('entry').
//
struct QSSGRhiUniformBufferSetKey
{
    enum Selector {
        Main,
        Shadow,
        ShadowBlurX,
        ShadowBlurY,
        ZPrePass,
        DepthTexture,
        AoTexture,
        ComputeMipmap,
        SkyBox
    };
    const void *layer;
    const void *model;
    const void *entry;
    Selector selector;
};

inline bool operator==(const QSSGRhiUniformBufferSetKey &a, const QSSGRhiUniformBufferSetKey &b) Q_DECL_NOTHROW
{
    return a.layer == b.layer && a.model == b.model && a.entry == b.entry && a.selector == b.selector;
}

inline bool operator!=(const QSSGRhiUniformBufferSetKey &a, const QSSGRhiUniformBufferSetKey &b) Q_DECL_NOTHROW
{
    return !(a == b);
}

inline uint qHash(const QSSGRhiUniformBufferSetKey &k, uint seed = 0) Q_DECL_NOTHROW
{
    return uint(k.selector) ^ qHash(k.layer, seed) ^ qHash(k.model, seed) ^ qHash(k.entry, seed);
}

struct QSSGRhiUniformBufferSet
{
    QRhiBuffer *ubuf = nullptr;
    QRhiBuffer *lightsUbuf = nullptr;

    void reset() {
        delete ubuf;
        delete lightsUbuf;
        *this = QSSGRhiUniformBufferSet();
    }
};

struct QSSGRhiSamplerDescription
{
    QRhiSampler::Filter minFilter;
    QRhiSampler::Filter magFilter;
    QRhiSampler::Filter mipmap;
    QRhiSampler::AddressMode hTiling;
    QRhiSampler::AddressMode vTiling;
};

inline bool operator==(const QSSGRhiSamplerDescription &a, const QSSGRhiSamplerDescription &b) Q_DECL_NOTHROW
{
   return a.hTiling == b.hTiling && a.vTiling == b.vTiling
           && a.minFilter == b.minFilter && a.magFilter == b.magFilter
           && a.mipmap == b.mipmap;
}

inline bool operator!=(const QSSGRhiSamplerDescription &a, const QSSGRhiSamplerDescription &b) Q_DECL_NOTHROW
{
    return !(a == b);
}

class Q_QUICK3DRENDER_EXPORT QSSGRhiContext
{
    Q_DISABLE_COPY(QSSGRhiContext)
public:
    QAtomicInt ref;
    QSSGRhiContext();
    ~QSSGRhiContext();

    void setRhi(QRhi *rhi);
    QRhi *rhi() const { return m_rhi; }
    bool isValid() const { return m_rhi != nullptr; }

    void setMainRenderPassDescriptor(QRhiRenderPassDescriptor *rpDesc) { m_mainRpDesc = rpDesc; }
    QRhiRenderPassDescriptor *mainRenderPassDescriptor() const { return m_mainRpDesc; }

    void setCommandBuffer(QRhiCommandBuffer *cb) { m_cb = cb; }
    QRhiCommandBuffer *commandBuffer() const { return m_cb; }

    void setMainPassSampleCount(int samples) { m_mainSamples = samples; }
    int mainPassSampleCount() const { return m_mainSamples; }

    QSSGRhiGraphicsPipelineState *graphicsPipelineState(const void *key)
    {
        return &m_gfxPs[key];
    }

    QSSGRhiGraphicsPipelineState *resetGraphicsPipelineState(const void *key)
    {
        m_gfxPs[key] = QSSGRhiGraphicsPipelineState();
        return &m_gfxPs[key];
    }

    using ShaderResourceBindingList = QVarLengthArray<QRhiShaderResourceBinding, 8>;
    QRhiShaderResourceBindings *srb(const ShaderResourceBindingList &bindings);
    QRhiGraphicsPipeline *pipeline(const QSSGGraphicsPipelineStateKey &key);
    QRhiComputePipeline *computePipeline(const QSSGComputePipelineStateKey &key);

    QSSGRhiUniformBufferSet &uniformBufferSet(const QSSGRhiUniformBufferSetKey &key)
    {
        return m_uniformBufferSets[key];
    }

    QRhiSampler *sampler(const QSSGRhiSamplerDescription &samplerDescription);

private:
    QRhi *m_rhi = nullptr;
    QRhiRenderPassDescriptor *m_mainRpDesc = nullptr;
    QRhiCommandBuffer *m_cb = nullptr;
    int m_mainSamples = 1;
    QHash<const void *, QSSGRhiGraphicsPipelineState> m_gfxPs;
    QHash<ShaderResourceBindingList, QRhiShaderResourceBindings *> m_srbCache;
    QHash<QSSGGraphicsPipelineStateKey, QRhiGraphicsPipeline *> m_pipelines;
    QHash<QSSGComputePipelineStateKey, QRhiComputePipeline *> m_computePipelines;
    QHash<QSSGRhiUniformBufferSetKey, QSSGRhiUniformBufferSet> m_uniformBufferSets;
    QVector<QPair<QSSGRhiSamplerDescription, QRhiSampler*>> m_samplers;
};

QT_END_NAMESPACE

#endif
