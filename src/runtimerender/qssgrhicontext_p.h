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

#include "qtquick3druntimerenderglobal_p.h"
#include <QtCore/qstack.h>
#include <QtQuick3DUtils/private/qssgrenderbasetypes_p.h>
#include <QtGui/private/qrhi_p.h>

QT_BEGIN_NAMESPACE

class QSSGRhiContext;
class QSSGRhiBuffer;
class QSSGRhiShaderStagesWithResources;
struct QSSGShaderLightProperties;
struct QSSGRenderModel;
struct Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRhiInputAssemblerState
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

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRhiBuffer
{
    Q_DISABLE_COPY(QSSGRhiBuffer)
public:
    QAtomicInt ref;

    QSSGRhiBuffer(QSSGRhiContext &context,
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
    QSSGRhiContext &m_context;
    QRhiBuffer *m_buffer = nullptr;
    quint32 m_stride;
    QRhiCommandBuffer::IndexFormat m_indexFormat;
};

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRhiShaderStages
{
    Q_DISABLE_COPY(QSSGRhiShaderStages)
public:
    QAtomicInt ref;

    QSSGRhiShaderStages(QSSGRhiContext &context) : m_context(context) { }

    QSSGRhiContext &context() const { return m_context; }
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
    QSSGRhiContext &m_context;
    QVector<QRhiShaderStage> m_stages;
};

struct QSSGRhiShaderUniform
{
    QByteArray name;
    bool dirty = false;
    size_t size = 0;
    char data[256];

private:
    size_t offset = SIZE_MAX;
    friend class QSSGRhiShaderStagesWithResources;
};

struct QSSGRhiShaderUniformArray
{
    QByteArray name;
    bool dirty = false;
    size_t typeSize = 0;
    size_t itemCount = 0;
    QByteArray data;

private:
    size_t offset = SIZE_MAX;
    friend class QSSGRhiShaderStagesWithResources;
};

// these are our current shader limits
#define QSSG_MAX_NUM_LIGHTS 15
// directional light uses 2d shadow maps, other lights use cubemaps
#define QSSG_SHADOW_MAP_TYPE_COUNT 2
// this is the per-type (type as in 2d or cubemap) limit
#define QSSG_MAX_NUM_SHADOWS_PER_TYPE 4

// note this struct must exactly match the memory layout of the uniform block in
// funcSampleLightVars.glsllib
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

// Default materials work with a regular combined image sampler for each shadowmap.
struct QSSGRhiShadowMapProperties
{
    QRhiTexture *shadowMapTexture = nullptr;
    QByteArray shadowMapTextureUniformName;
    int cachedBinding = -1; // -1 == invalid
};

// Custom materials have an array of combined image samplers, one array for 2D
// shadow maps, and one for cubemap ones.
struct QSSGRhiShadowMapArrayProperties
{
    QVarLengthArray<QRhiTexture *, 8> shadowMapTextures;
    QByteArray shadowMapArrayUniformName;
    bool isCubemap = false;
    int shaderArrayDim = 0;
    int cachedBinding = -1;
};

QRhiSampler::Filter toRhi(QSSGRenderTextureFilterOp op);
QRhiSampler::AddressMode toRhi(QSSGRenderTextureCoordOp tiling);

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

struct QSSGRhiTexture
{
    QByteArray name;
    QRhiTexture *texture;
    QSSGRhiSamplerDescription samplerDesc;
};

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRhiShaderStagesWithResources
{
    Q_DISABLE_COPY(QSSGRhiShaderStagesWithResources)
public:
    // This struct is used purely for performance. It is used to quickly store
    // and index common uniform names using the storeIndex argument in the
    // setUniform method.
    struct CommonUniformIndices
    {
        int cameraPositionIdx = -1;
        int cameraDirectionIdx = -1;
        int viewProjectionMatrixIdx = -1;
        int projectionMatrixIdx = -1;
        int inverseProjectionMatrixIdx = -1;
        int viewMatrixIdx = -1;
        int normalAdjustViewportFactorIdx = -1;
        int isClipDepthZeroToOneIdx = -1;
        int modelViewProjectionIdx = -1;
        int normalMatrixIdx = -1;
        int modelMatrixIdx = -1;
        int lightProbeRotationIdx = -1;
        int lightProbeOffsetIdx = -1;
        int lightProbeOptionsIdx = -1;
        int lightProbePropertiesIdx = -1;
        int material_emissiveColorIdx = -1;
        int material_baseColorIdx = -1;
        int material_specularIdx = -1;
        int cameraPropertiesIdx = -1;
        int fresnelPowerIdx = -1;
        int light_ambient_totalIdx = -1;
        int material_propertiesIdx = -1;
        int bumpAmountIdx = -1;
        int displaceAmountIdx = -1;
        int translucentFalloffIdx = -1;
        int diffuseLightWrapIdx = -1;
        int occlusionAmountIdx = -1;
        int alphaCutoffIdx = -1;
        int boneTransformsIdx = -1;
        int boneNormalTransformsIdx = -1;
        int shadowDepthAdjustIdx = -1;
        int pointSizeIdx = -1;

        struct ImageIndices
        {
            int imageRotationsUniformIndex = -1;
            int imageOffsetsUniformIndex = -1;
        };

        QHash<quint32, ImageIndices> imageIndices;
    } commonUniformIndices;

    QAtomicInt ref;

    static QSSGRef<QSSGRhiShaderStagesWithResources> fromShaderStages(const QSSGRef<QSSGRhiShaderStages> &stages);

    const QSSGRhiShaderStages *stages() const { return m_shaderStages.data(); }

    int setUniformValue(const QByteArray &name, const QVariant &value, QSSGRenderShaderDataType type);
    int setUniform(const QByteArray &name, const void *data, size_t size, int storeIndex = -1);
    int setUniformArray(const QByteArray &name, const void *data, size_t itemCount, QSSGRenderShaderDataType type, int storeIndex = -1);
    void dumpUniforms();
    int bindingForTexture(const QByteArray &name, const QVector<int> **arrayDims = nullptr) const;

    // Default materials put all lights into a single uniform buffer, whereas
    // custom material use two uniform buffers, one for area and one for
    // non-area lights.
    enum LightBufferSlot {
        LightBuffer0,
        LightBuffer1,

        LightBufferMax
    };

    void resetLights(LightBufferSlot slot) { m_lights[slot].clear(); }
    QSSGShaderLightProperties &addLight(LightBufferSlot slot) { m_lights[slot].append(QSSGShaderLightProperties()); return m_lights[slot].last(); }
    int lightCount(LightBufferSlot slot) const { return m_lights[slot].count(); }
    const QSSGShaderLightProperties &lightAt(LightBufferSlot slot, int index) const { return m_lights[slot][index]; }
    QSSGShaderLightProperties &lightAt(LightBufferSlot slot, int index) { return m_lights[slot][index]; }
    void setLightsEnabled(LightBufferSlot slot, bool enable) { m_lightsEnabled[slot] = enable; }
    bool isLightingEnabled(LightBufferSlot slot) const { return m_lightsEnabled[slot]; }

    void resetShadowMaps() { m_shadowMaps.clear(); }
    QSSGRhiShadowMapProperties &addShadowMap() { m_shadowMaps.append(QSSGRhiShadowMapProperties()); return m_shadowMaps.last(); }
    int shadowMapCount() const { return m_shadowMaps.count(); }
    const QSSGRhiShadowMapProperties &shadowMapAt(int index) const { return m_shadowMaps[index]; }
    QSSGRhiShadowMapProperties &shadowMapAt(int index) { return m_shadowMaps[index]; }

    void resetShadowMapArrays() { m_shadowMapArrays.clear(); }
    QSSGRhiShadowMapArrayProperties &addShadowMapArray() { m_shadowMapArrays.append(QSSGRhiShadowMapArrayProperties()); return m_shadowMapArrays.last(); }
    int shadowMapArrayCount() const { return m_shadowMapArrays.count(); }
    const QSSGRhiShadowMapArrayProperties &shadowMapArrayAt(int index) const { return m_shadowMapArrays[index]; }
    QSSGRhiShadowMapArrayProperties &shadowMapArrayAt(int index) { return m_shadowMapArrays[index]; }

    void bakeMainUniformBuffer(QRhiBuffer **ubuf, QRhiResourceUpdateBatch *resourceUpdates);
    void bakeLightsUniformBuffer(LightBufferSlot slot, QRhiBuffer **ubuf, QRhiResourceUpdateBatch *resourceUpdates);

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

    void setScreenTexture(QRhiTexture *texture) { m_screenTexture = texture; }
    QRhiTexture *screenTexture() const { return m_screenTexture; }

    void setDepthTexture(QRhiTexture *texture) { m_depthTexture = texture; }
    QRhiTexture *depthTexture() const { return m_depthTexture; }

    void setSsaoTexture(QRhiTexture *texture) { m_ssaoTexture = texture; }
    QRhiTexture *ssaoTexture() const { return m_ssaoTexture; }

    void resetExtraTextures() { m_extraTextures.clear(); }
    void addExtraTexture(const QSSGRhiTexture &t) { m_extraTextures.append(t); }
    int extraTextureCount() const { return m_extraTextures.count(); }
    const QSSGRhiTexture &extraTextureAt(int index) { return m_extraTextures[index]; }

    QSSGRhiShaderStagesWithResources(QSSGRef<QSSGRhiShaderStages> shaderStages)
        : m_context(shaderStages->context()),
          m_shaderStages(shaderStages)
    {
    }
    ~QSSGRhiShaderStagesWithResources();

protected:
    QSSGRhiContext &m_context;
    QSSGRef<QSSGRhiShaderStages> m_shaderStages;
    QVector<QSSGRhiShaderUniform> m_uniforms; // members of the main (binding 0) uniform buffer
    QHash<QByteArray, size_t> m_uniformIndex; // Maps uniform name to index in m_uniforms
    QVector<QSSGRhiShaderUniformArray *> m_uniformArrays;
    bool m_lightsEnabled[LightBufferMax] = {};
    QVarLengthArray<QSSGShaderLightProperties, QSSG_MAX_NUM_LIGHTS> m_lights[LightBufferMax];
    QVarLengthArray<QSSGRhiShadowMapProperties, QSSG_MAX_NUM_SHADOWS_PER_TYPE * QSSG_SHADOW_MAP_TYPE_COUNT> m_shadowMaps;
    QVarLengthArray<QSSGRhiShadowMapArrayProperties, 2> m_shadowMapArrays;
    QRhiTexture *m_lightProbeTexture = nullptr; // not owned
    QSSGRenderTextureCoordOp m_lightProbeHorzTile = QSSGRenderTextureCoordOp::ClampToEdge;
    QSSGRenderTextureCoordOp m_lightProbeVertTile = QSSGRenderTextureCoordOp::ClampToEdge;
    QRhiTexture *m_screenTexture = nullptr; // not owned
    QRhiTexture *m_depthTexture = nullptr; // not owned
    QRhiTexture *m_ssaoTexture = nullptr; // not owned
    QVarLengthArray<QSSGRhiTexture, 8> m_extraTextures; // does not own
};

struct Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRhiGraphicsPipelineState
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
    float lineWidth = 1.0f;

    static QRhiGraphicsPipeline::CullMode toCullMode(QSSGCullFaceMode cullFaceMode);
};

bool operator==(const QSSGRhiGraphicsPipelineState &a, const QSSGRhiGraphicsPipelineState &b) Q_DECL_NOTHROW;
bool operator!=(const QSSGRhiGraphicsPipelineState &a, const QSSGRhiGraphicsPipelineState &b) Q_DECL_NOTHROW;
size_t qHash(const QSSGRhiGraphicsPipelineState &s, size_t seed = 0) Q_DECL_NOTHROW;

struct QSSGGraphicsPipelineStateKey
{
    QSSGRhiGraphicsPipelineState state;
    QRhiRenderPassDescriptor *compatibleRpDesc;
    QRhiShaderResourceBindings *layoutCompatibleSrb;
};

bool operator==(const QSSGGraphicsPipelineStateKey &a, const QSSGGraphicsPipelineStateKey &b) Q_DECL_NOTHROW;
bool operator!=(const QSSGGraphicsPipelineStateKey &a, const QSSGGraphicsPipelineStateKey &b) Q_DECL_NOTHROW;
size_t qHash(const QSSGGraphicsPipelineStateKey &k, size_t seed = 0) Q_DECL_NOTHROW;

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

inline size_t qHash(const QSSGComputePipelineStateKey &k, size_t seed = 0) Q_DECL_NOTHROW
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
        SkyBox,
        ProgressiveAA,
        Effects,
        Item2D
    };
    const void *layer;
    const void *model;
    const void *entry;
    int index;
    Selector selector;
};

inline bool operator==(const QSSGRhiUniformBufferSetKey &a, const QSSGRhiUniformBufferSetKey &b) Q_DECL_NOTHROW
{
    return a.layer == b.layer && a.model == b.model && a.entry == b.entry && a.index == b.index && a.selector == b.selector;
}

inline bool operator!=(const QSSGRhiUniformBufferSetKey &a, const QSSGRhiUniformBufferSetKey &b) Q_DECL_NOTHROW
{
    return !(a == b);
}

inline size_t qHash(const QSSGRhiUniformBufferSetKey &k, size_t seed = 0) Q_DECL_NOTHROW
{
    return uint(k.selector) ^ uint(k.index) ^ qHash(k.layer, seed) ^ qHash(k.model, seed) ^ qHash(k.entry, seed);
}

struct QSSGRhiUniformBufferSet
{
    QRhiBuffer *ubuf = nullptr;
    QRhiBuffer *lightsUbuf0 = nullptr;
    QRhiBuffer *lightsUbuf1 = nullptr;

    void reset() {
        delete ubuf;
        delete lightsUbuf0;
        delete lightsUbuf1;
        *this = QSSGRhiUniformBufferSet();
    }
};

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRhiContext
{
    Q_DISABLE_COPY(QSSGRhiContext)
public:
    QAtomicInt ref;
    QSSGRhiContext();
    ~QSSGRhiContext();

    void initialize(QRhi *rhi);
    QRhi *rhi() const { return m_rhi; }
    bool isValid() const { return m_rhi != nullptr; }

    void setMainRenderPassDescriptor(QRhiRenderPassDescriptor *rpDesc) { m_mainRpDesc = rpDesc; }
    QRhiRenderPassDescriptor *mainRenderPassDescriptor() const { return m_mainRpDesc; }

    void setCommandBuffer(QRhiCommandBuffer *cb) { m_cb = cb; }
    QRhiCommandBuffer *commandBuffer() const { return m_cb; }

    void setRenderTarget(QRhiRenderTarget *rt) { m_rt = rt; }
    QRhiRenderTarget *renderTarget() const { return m_rt; }

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

    void invalidateCachedReferences(QRhiRenderPassDescriptor *rpDesc);

    QSSGRhiUniformBufferSet &uniformBufferSet(const QSSGRhiUniformBufferSetKey &key)
    {
        return m_uniformBufferSets[key];
    }

    QRhiSampler *sampler(const QSSGRhiSamplerDescription &samplerDescription);

    // ### this will become something more sophisticated later on, for now just hold on
    // to whatever texture we get, and make sure they get destroyed in the dtor
    void registerTexture(QRhiTexture *texture) { m_textures.insert(texture); }
    void releaseTexture(QRhiTexture *texture);

    void cleanupUniformBufferSets(const QSSGRenderModel *model);

    QRhiTexture *dummyTexture(QRhiTexture::Flags flags, QRhiResourceUpdateBatch *rub);

private:
    QRhi *m_rhi = nullptr;
    QRhiRenderPassDescriptor *m_mainRpDesc = nullptr;
    QRhiCommandBuffer *m_cb = nullptr;
    QRhiRenderTarget *m_rt = nullptr;
    int m_mainSamples = 1;
    QHash<const void *, QSSGRhiGraphicsPipelineState> m_gfxPs;
    QHash<ShaderResourceBindingList, QRhiShaderResourceBindings *> m_srbCache;
    QHash<QSSGGraphicsPipelineStateKey, QRhiGraphicsPipeline *> m_pipelines;
    QHash<QSSGComputePipelineStateKey, QRhiComputePipeline *> m_computePipelines;
    QHash<QSSGRhiUniformBufferSetKey, QSSGRhiUniformBufferSet> m_uniformBufferSets;
    QVector<QPair<QSSGRhiSamplerDescription, QRhiSampler*>> m_samplers;
    QSet<QRhiTexture *> m_textures;
    QHash<QRhiTexture::Flags, QRhiTexture *> m_dummyTextures;
};

inline QRhiSampler::Filter toRhi(QSSGRenderTextureFilterOp op)
{
    switch (op) {
    case QSSGRenderTextureFilterOp::Nearest:
        return QRhiSampler::Nearest;
    case QSSGRenderTextureFilterOp::Linear:
        return QRhiSampler::Linear;
    default:
        break;
    }
    return QRhiSampler::Linear;
}

inline QRhiSampler::AddressMode toRhi(QSSGRenderTextureCoordOp tiling)
{
    switch (tiling) {
    case QSSGRenderTextureCoordOp::Repeat:
        return QRhiSampler::Repeat;
    case QSSGRenderTextureCoordOp::MirroredRepeat:
        return QRhiSampler::Mirror;
    case QSSGRenderTextureCoordOp::ClampToEdge:
        return QRhiSampler::ClampToEdge;
    default:
        break;
    }
    return QRhiSampler::ClampToEdge;
}

QT_END_NAMESPACE

#endif
