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
struct QSSGShaderLightProperties;
struct QSSGRenderModel;
class QSSGRhiShaderPipeline;

struct Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRhiInputAssemblerState
{
    enum InputSemantic {
        PositionSemantic,  // attr_pos
        NormalSemantic,    // attr_norm
        TexCoord0Semantic, // attr_uv0
        TexCoord1Semantic, // attr_uv1
        TangentSemantic,   // attr_textan
        BinormalSemantic,  // attr_binormal
        JointSemantic,     // attr_joints
        WeightSemantic,    // attr_weights
        ColorSemantic      // attr_color
    };

    QRhiVertexInputLayout inputLayout;
    QVarLengthArray<InputSemantic, 8> inputs;
    QRhiGraphicsPipeline::Topology topology;
    QSSGRef<QSSGRhiBuffer> vertexBuffer;
    QSSGRef<QSSGRhiBuffer> indexBuffer;

    static QRhiVertexInputAttribute::Format toVertexInputFormat(QSSGRenderComponentType compType, quint32 numComps);
    static QRhiGraphicsPipeline::Topology toTopology(QSSGRenderDrawMode drawMode);

    // Fills out inputLayout.attributes[].location based on
    // inputLayoutInputNames and the provided shader reflection info.
    void bakeVertexInputLocations(const QSSGRhiShaderPipeline &shaders);
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

private:
    QSSGRhiContext &m_context;
    QRhiBuffer *m_buffer = nullptr;
    quint32 m_stride;
    QRhiCommandBuffer::IndexFormat m_indexFormat;
};

struct QSSGRhiShaderUniform
{
    char name[64];
    size_t size = 0;

private:
    size_t offset = SIZE_MAX;
    bool maybeExists = true;
    friend class QSSGRhiShaderPipeline;
};

struct QSSGRhiShaderUniformArray
{
    char name[64];
    size_t typeSize = 0;
    size_t itemCount = 0;

private:
    size_t offset = SIZE_MAX;
    size_t size = 0;
    bool maybeExists = true;
    friend class QSSGRhiShaderPipeline;
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

enum class QSSGRhiSamplerBindingHints
{
    LightProbe = 64, // must be larger than the largest value in SSGRenderableImage::Type
    ScreenTexture,
    DepthTexture,
    AoTexture,

    BindingMapSize
};

// these are our current shader limits
#define QSSG_MAX_NUM_LIGHTS 15
// directional light uses 2d shadow maps, other lights use cubemaps
#define QSSG_SHADOW_MAP_TYPE_COUNT 2
// this is the per-type (type as in 2d or cubemap) limit
#define QSSG_MAX_NUM_SHADOWS_PER_TYPE 4

// note this struct must exactly match the memory layout of the uniform block in
// funcSampleLightVars.glsllib
struct QSSGShaderLightData
{
    float position[4];
    float direction[4]; // Specifies the light direction in world coordinates.
    float diffuse[4];
    float specular[4];
    float coneAngle; // Specifies the outer cone angle of the spot light.
    float innerConeAngle; // Specifies the inner cone angle of the spot light.
    float constantAttenuation; // Specifies the constant light attenuation factor.
    float linearAttenuation; // Specifies the linear light attenuation factor.
    float quadraticAttenuation; // Specifies the quadratic light attenuation factor.
    float padding[3]; // the next light array element must start at a vec4-aligned offset
};

struct QSSGShaderLightsUniformData
{
    qint32 count = -1;
    float padding[3]; // first element must start at a vec4-aligned offset
    QSSGShaderLightData lightData[QSSG_MAX_NUM_LIGHTS];
};

// Default materials work with a regular combined image sampler for each shadowmap.
struct QSSGRhiShadowMapProperties
{
    QRhiTexture *shadowMapTexture = nullptr;
    QByteArray shadowMapTextureUniformName;
    int cachedBinding = -1; // -1 == invalid
};

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRhiShaderPipeline
{
    Q_DISABLE_COPY(QSSGRhiShaderPipeline)
public:
    QAtomicInt ref;

    QSSGRhiShaderPipeline(QSSGRhiContext &context) : m_context(context) { }

    QSSGRhiContext &context() const { return m_context; }
    bool isNull() const { return m_stages.isEmpty(); }

    enum StageFlag {
        // Indicates that this shaderpipeline object is not going to be used with
        // a QSSGRhiInputAssemblerState, i.e. bakeVertexInputLocations() will
        // not be called.
        UsedWithoutIa = 0x01
    };
    Q_DECLARE_FLAGS(StageFlags, StageFlag)

    void addStage(const QRhiShaderStage &stage, StageFlags flags = {});
    const QRhiShaderStage *cbeginStages() const { return m_stages.cbegin(); }
    const QRhiShaderStage *cendStages() const { return m_stages.cend(); }

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

    int ub0Size() const { return m_ub0Size; }
    int ub0LightDataOffset() const { return m_ub0NextUBufOffset; }
    int ub0LightDataSize() const
    {
        return int(4 * sizeof(qint32) + m_lightsUniformData.count * sizeof(QSSGShaderLightData));
    }

    const QHash<QSSGRhiInputAssemblerState::InputSemantic, QShaderDescription::InOutVariable> &vertexInputs() const { return m_vertexInputs; }

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
        int modelViewProjectionIdx = -1;
        int normalMatrixIdx = -1;
        int modelMatrixIdx = -1;
        int lightProbeOrientationIdx = -1;
        int lightProbePropertiesIdx = -1;
        int material_emissiveColorIdx = -1;
        int material_baseColorIdx = -1;
        int material_specularIdx = -1;
        int cameraPropertiesIdx = -1;
        int light_ambient_totalIdx = -1;
        int material_propertiesIdx = -1;
        int material_properties2Idx = -1;
        int material_properties3Idx = -1;
        int displaceAmountIdx = -1;
        int boneTransformsIdx = -1;
        int boneNormalTransformsIdx = -1;
        int shadowDepthAdjustIdx = -1;
        int pointSizeIdx = -1;

        struct ImageIndices
        {
            int imageRotationsUniformIndex = -1;
            int imageOffsetsUniformIndex = -1;
        };
        QVarLengthArray<ImageIndices, 16> imageIndices;
    } commonUniformIndices;

    enum class UniformFlag {
        Mat3 = 0x01
    };
    Q_DECLARE_FLAGS(UniformFlags, UniformFlag)

    void setUniformValue(char *ubufData, const char *name, const QVariant &value, QSSGRenderShaderDataType type);
    void setUniform(char *ubufData, const char *name, const void *data, size_t size, int *storeIndex = nullptr, UniformFlags flags = {});
    void setUniformArray(char *ubufData, const char *name, const void *data, size_t itemCount, QSSGRenderShaderDataType type, int *storeIndex = nullptr);
    int bindingForTexture(const char *name, int hint = -1);

    void setLightsEnabled(bool enable) { m_lightsEnabled = enable; }
    bool isLightingEnabled() const { return m_lightsEnabled; }

    void resetShadowMaps() { m_shadowMaps.clear(); }
    QSSGRhiShadowMapProperties &addShadowMap() { m_shadowMaps.append(QSSGRhiShadowMapProperties()); return m_shadowMaps.last(); }
    int shadowMapCount() const { return m_shadowMaps.count(); }
    const QSSGRhiShadowMapProperties &shadowMapAt(int index) const { return m_shadowMaps[index]; }
    QSSGRhiShadowMapProperties &shadowMapAt(int index) { return m_shadowMaps[index]; }

    void ensureCombinedMainLightsUniformBuffer(QRhiBuffer **ubuf);

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

    QSSGShaderLightsUniformData &lightsUniformData() { return m_lightsUniformData; }

private:
    QSSGRhiContext &m_context;
    QVarLengthArray<QRhiShaderStage, 2> m_stages;
    int m_ub0Size = 0;
    int m_ub0NextUBufOffset = 0;
    QHash<QByteArray, QShaderDescription::BlockVariable> m_ub0;
    QHash<QSSGRhiInputAssemblerState::InputSemantic, QShaderDescription::InOutVariable> m_vertexInputs;
    QHash<QByteArray, QShaderDescription::InOutVariable> m_combinedImageSamplers;
    int m_materialImageSamplerBindings[size_t(QSSGRhiSamplerBindingHints::BindingMapSize)];

    QVarLengthArray<QSSGRhiShaderUniform, 32> m_uniforms; // members of the main (binding 0) uniform buffer
    QVarLengthArray<QSSGRhiShaderUniformArray, 8> m_uniformArrays;
    QHash<QByteArray, size_t> m_uniformIndex; // Maps uniform name to index in m_uniforms and m_uniformArrays

    // transient (per-object) data; pointers are all non-owning
    bool m_lightsEnabled = false;
    QSSGShaderLightsUniformData m_lightsUniformData;
    QVarLengthArray<QSSGRhiShadowMapProperties, QSSG_MAX_NUM_SHADOWS_PER_TYPE * QSSG_SHADOW_MAP_TYPE_COUNT> m_shadowMaps;
    QRhiTexture *m_lightProbeTexture = nullptr;
    QSSGRenderTextureCoordOp m_lightProbeHorzTile = QSSGRenderTextureCoordOp::ClampToEdge;
    QSSGRenderTextureCoordOp m_lightProbeVertTile = QSSGRenderTextureCoordOp::ClampToEdge;
    QRhiTexture *m_screenTexture = nullptr;
    QRhiTexture *m_depthTexture = nullptr;
    QRhiTexture *m_ssaoTexture = nullptr;
    QVarLengthArray<QSSGRhiTexture, 8> m_extraTextures;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QSSGRhiShaderPipeline::StageFlags)
Q_DECLARE_OPERATORS_FOR_FLAGS(QSSGRhiShaderPipeline::UniformFlags)

struct Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRhiGraphicsPipelineState
{
    const QSSGRhiShaderPipeline *shaderPipeline;
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

inline bool operator==(const QSSGRhiGraphicsPipelineState &a, const QSSGRhiGraphicsPipelineState &b) Q_DECL_NOTHROW
{
    return a.shaderPipeline == b.shaderPipeline
            && a.samples == b.samples
            && a.depthTestEnable == b.depthTestEnable
            && a.depthWriteEnable == b.depthWriteEnable
            && a.depthFunc == b.depthFunc
            && a.cullMode == b.cullMode
            && a.depthBias == b.depthBias
            && a.slopeScaledDepthBias == b.slopeScaledDepthBias
            && a.blendEnable == b.blendEnable
            && a.scissorEnable == b.scissorEnable
            && a.viewport == b.viewport
            && a.scissor == b.scissor
            && a.ia.topology == b.ia.topology
            && a.ia.inputLayout == b.ia.inputLayout
            && a.targetBlend.colorWrite == b.targetBlend.colorWrite
            && a.targetBlend.srcColor == b.targetBlend.srcColor
            && a.targetBlend.dstColor == b.targetBlend.dstColor
            && a.targetBlend.opColor == b.targetBlend.opColor
            && a.targetBlend.srcAlpha == b.targetBlend.srcAlpha
            && a.targetBlend.dstAlpha == b.targetBlend.dstAlpha
            && a.targetBlend.opAlpha == b.targetBlend.opAlpha
            && a.colorAttachmentCount == b.colorAttachmentCount
            && a.lineWidth == b.lineWidth;
}

inline bool operator!=(const QSSGRhiGraphicsPipelineState &a, const QSSGRhiGraphicsPipelineState &b) Q_DECL_NOTHROW
{
    return !(a == b);
}

inline size_t qHash(const QSSGRhiGraphicsPipelineState &s, size_t seed) Q_DECL_NOTHROW
{
    // do not bother with all fields
    return qHash(s.shaderPipeline, seed)
            ^ qHash(s.samples)
            ^ qHash(s.targetBlend.dstColor)
            ^ qHash(s.depthFunc)
            ^ qHash(s.cullMode)
            ^ qHash(s.colorAttachmentCount)
            ^ qHash(s.lineWidth)
            ^ (s.depthTestEnable << 1)
            ^ (s.depthWriteEnable << 2)
            ^ (s.blendEnable << 3)
            ^ (s.scissorEnable << 4);
}

struct QSSGGraphicsPipelineStateKey
{
    QSSGRhiGraphicsPipelineState state;
    QRhiRenderPassDescriptor *compatibleRpDesc;
    QRhiShaderResourceBindings *layoutCompatibleSrb;
};

inline bool operator==(const QSSGGraphicsPipelineStateKey &a, const QSSGGraphicsPipelineStateKey &b) Q_DECL_NOTHROW
{
    return a.state == b.state
            && a.compatibleRpDesc->isCompatible(b.compatibleRpDesc)
            && a.layoutCompatibleSrb->isLayoutCompatible(b.layoutCompatibleSrb);
}

inline bool operator!=(const QSSGGraphicsPipelineStateKey &a, const QSSGGraphicsPipelineStateKey &b) Q_DECL_NOTHROW
{
    return !(a == b);
}

inline size_t qHash(const QSSGGraphicsPipelineStateKey &k, size_t seed) Q_DECL_NOTHROW
{
    return qHash(k.state, seed); // rp and srb not included, intentionally (see ==, those are based on compatibility, not pointer equivalence)
}

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

struct QSSGRhiShaderResourceBindingList
{
    static const int MAX_SIZE = 32;

    int p = 0;
    size_t h = 0;
    QRhiShaderResourceBinding v[MAX_SIZE];

    void clear() { p = 0; h = 0; }

    QSSGRhiShaderResourceBindingList() { }

    QSSGRhiShaderResourceBindingList(const QSSGRhiShaderResourceBindingList &other)
        : p(other.p),
          h(other.h)
    {
        for (int i = 0; i < p; ++i)
            v[i] = other.v[i];
    }

    QSSGRhiShaderResourceBindingList &operator=(const QSSGRhiShaderResourceBindingList &other) Q_DECL_NOTHROW
    {
        if (this != &other) {
            p = other.p;
            h = other.h;
            for (int i = 0; i < p; ++i)
                v[i] = other.v[i];
        }
        return *this;
    }

    void addUniformBuffer(int binding, QRhiShaderResourceBinding::StageFlags stage, QRhiBuffer *buf, int offset, int size);
    void addTexture(int binding, QRhiShaderResourceBinding::StageFlags stage, QRhiTexture *tex, QRhiSampler *sampler);
};

inline bool operator==(const QSSGRhiShaderResourceBindingList &a, const QSSGRhiShaderResourceBindingList &b) Q_DECL_NOTHROW
{
    if (a.h != b.h)
        return false;
    if (a.p != b.p)
        return false;
    for (int i = 0; i < a.p; ++i) {
        if (a.v[i] != b.v[i])
            return false;
    }
    return true;
}

inline bool operator!=(const QSSGRhiShaderResourceBindingList &a, const QSSGRhiShaderResourceBindingList &b) Q_DECL_NOTHROW
{
    return !(a == b);
}

inline size_t qHash(const QSSGRhiShaderResourceBindingList &bl, size_t seed) Q_DECL_NOTHROW
{
    return bl.h ^ seed;
}

inline void QSSGRhiShaderResourceBindingList::addUniformBuffer(int binding, QRhiShaderResourceBinding::StageFlags stage,
                                                               QRhiBuffer *buf, int offset = 0, int size = 0)
{
#ifdef QT_DEBUG
    if (p == MAX_SIZE) {
        qWarning("Out of shader resource bindings slots (max is %d)", MAX_SIZE);
        return;
    }
#endif
    QRhiShaderResourceBinding::Data *d = v[p++].data();
    h ^= qintptr(buf);
    d->binding = binding;
    d->stage = stage;
    d->type = QRhiShaderResourceBinding::UniformBuffer;
    d->u.ubuf.buf = buf;
    d->u.ubuf.offset = offset;
    d->u.ubuf.maybeSize = size; // 0 = all
    d->u.ubuf.hasDynamicOffset = false;
}

inline void QSSGRhiShaderResourceBindingList::addTexture(int binding, QRhiShaderResourceBinding::StageFlags stage,
                                                         QRhiTexture *tex, QRhiSampler *sampler)
{
#ifdef QT_DEBUG
    if (p == QSSGRhiShaderResourceBindingList::MAX_SIZE) {
        qWarning("Out of shader resource bindings slots (max is %d)", MAX_SIZE);
        return;
    }
#endif
    QRhiShaderResourceBinding::Data *d = v[p++].data();
    h ^= qintptr(tex) ^ qintptr(sampler);
    d->binding = binding;
    d->stage = stage;
    d->type = QRhiShaderResourceBinding::SampledTexture;
    d->u.stex.count = 1;
    d->u.stex.texSamplers[0].tex = tex;
    d->u.stex.texSamplers[0].sampler = sampler;
}

// The lookup keys can be somewhat complicated due to having to handle cases
// like "render a model in a shared scene between multiple View3Ds" (here both
// the View3D ('layer') and the model ('model') act as the lookup key since
// while the model is the same, we still want different uniform buffers per
// View3D), or the case of shadow maps where the shadow map (there can be as
// many as lights) is taken into account too ('entry').
//
struct QSSGRhiDrawCallDataKey
{
    enum Selector {
        Main,
        Shadow,
        ShadowBlur,
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

inline bool operator==(const QSSGRhiDrawCallDataKey &a, const QSSGRhiDrawCallDataKey &b) Q_DECL_NOTHROW
{
    return a.selector == b.selector && a.layer == b.layer && a.model == b.model && a.entry == b.entry && a.index == b.index;
}

inline bool operator!=(const QSSGRhiDrawCallDataKey &a, const QSSGRhiDrawCallDataKey &b) Q_DECL_NOTHROW
{
    return !(a == b);
}

inline size_t qHash(const QSSGRhiDrawCallDataKey &k, size_t seed = 0) Q_DECL_NOTHROW
{
    return uint(k.selector) ^ uint(k.index) ^ qHash(k.layer, seed) ^ qHash(k.model, seed) ^ qHash(k.entry, seed);
}

struct QSSGRhiDrawCallData
{
    QRhiBuffer *ubuf = nullptr; // owned
    QRhiShaderResourceBindings *srb = nullptr; // not owned
    QSSGRhiShaderResourceBindingList bindings;
    QRhiGraphicsPipeline *pipeline = nullptr; // not owned
    QRhiRenderPassDescriptor *pipelineRpDesc = nullptr; // not owned
    QSSGRhiGraphicsPipelineState ps;

    void reset() {
        delete ubuf;
        ubuf = nullptr;
        srb = nullptr;
        pipeline = nullptr;
        pipelineRpDesc = nullptr;
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

    QRhiShaderResourceBindings *srb(const QSSGRhiShaderResourceBindingList &bindings);
    QRhiGraphicsPipeline *pipeline(const QSSGGraphicsPipelineStateKey &key);
    QRhiComputePipeline *computePipeline(const QSSGComputePipelineStateKey &key);

    void invalidateCachedReferences(QRhiRenderPassDescriptor *rpDesc);

    QSSGRhiDrawCallData &drawCallData(const QSSGRhiDrawCallDataKey &key)
    {
        return m_drawCallData[key];
    }

    QRhiSampler *sampler(const QSSGRhiSamplerDescription &samplerDescription);

    // ### this will become something more sophisticated later on, for now just hold on
    // to whatever texture we get, and make sure they get destroyed in the dtor
    void registerTexture(QRhiTexture *texture) { m_textures.insert(texture); }
    void releaseTexture(QRhiTexture *texture);

    void cleanupDrawCallData(const QSSGRenderModel *model);

    QRhiTexture *dummyTexture(QRhiTexture::Flags flags, QRhiResourceUpdateBatch *rub);

    static inline QRhiCommandBuffer::BeginPassFlags commonPassFlags()
    {
        // We do not use GPU compute at all at the moment, this means we can
        // get a small performance gain with OpenGL by declaring this.
        return QRhiCommandBuffer::DoNotTrackResourcesForCompute;
    }

private:
    QRhi *m_rhi = nullptr;
    QRhiRenderPassDescriptor *m_mainRpDesc = nullptr;
    QRhiCommandBuffer *m_cb = nullptr;
    QRhiRenderTarget *m_rt = nullptr;
    int m_mainSamples = 1;
    QHash<const void *, QSSGRhiGraphicsPipelineState> m_gfxPs;
    QHash<QSSGRhiShaderResourceBindingList, QRhiShaderResourceBindings *> m_srbCache;
    QHash<QSSGGraphicsPipelineStateKey, QRhiGraphicsPipeline *> m_pipelines;
    QHash<QSSGComputePipelineStateKey, QRhiComputePipeline *> m_computePipelines;
    QHash<QSSGRhiDrawCallDataKey, QSSGRhiDrawCallData> m_drawCallData;
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
