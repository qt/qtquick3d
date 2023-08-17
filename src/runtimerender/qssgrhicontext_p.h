// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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
#include "private/qquick3dprofiler_p.h"

QT_BEGIN_NAMESPACE

class QSSGRhiContext;
class QSSGRhiBuffer;
struct QSSGShaderLightProperties;
struct QSSGRenderMesh;
struct QSSGRenderModel;
class QSSGRhiShaderPipeline;
struct QSSGRenderInstanceTable;
struct QSSGRenderLayer;

struct Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRhiInputAssemblerState
{
    enum InputSemantic {
        PositionSemantic,           // attr_pos
        NormalSemantic,             // attr_norm
        TexCoord0Semantic,          // attr_uv0
        TexCoord1Semantic,          // attr_uv1
        TangentSemantic,            // attr_textan
        BinormalSemantic,           // attr_binormal
        ColorSemantic,              // attr_color
        MaxTargetSemantic = ColorSemantic,
        JointSemantic,              // attr_joints
        WeightSemantic,             // attr_weights
        TexCoordLightmapSemantic    // attr_lightmapuv
    };

    QRhiVertexInputLayout inputLayout;
    QVarLengthArray<InputSemantic, 8> inputs;
    QRhiGraphicsPipeline::Topology topology;

    std::array<quint8, MaxTargetSemantic + 1> targetOffsets = { UINT8_MAX, UINT8_MAX, UINT8_MAX, UINT8_MAX,
                                                                     UINT8_MAX, UINT8_MAX, UINT8_MAX };
    quint8 targetCount = 0;

    static QRhiVertexInputAttribute::Format toVertexInputFormat(QSSGRenderComponentType compType, quint32 numComps);
    static QRhiGraphicsPipeline::Topology toTopology(QSSGRenderDrawMode drawMode);

    // Fills out inputLayout.attributes[].location based on
    // inputLayoutInputNames and the provided shader reflection info.
    void bakeVertexInputLocations(const QSSGRhiShaderPipeline &shaders, int instanceBufferBinding = 0);
};

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRhiBuffer
{
    Q_DISABLE_COPY(QSSGRhiBuffer)
public:
    QSSGRhiBuffer(QSSGRhiContext &context,
                  QRhiBuffer::Type type,
                  QRhiBuffer::UsageFlags usageMask,
                  quint32 stride,
                  qsizetype size,
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

using QSSGRhiBufferPtr = std::shared_ptr<QSSGRhiBuffer>;

QRhiSampler::Filter toRhi(QSSGRenderTextureFilterOp op);
QRhiSampler::AddressMode toRhi(QSSGRenderTextureCoordOp tiling);

struct QSSGRhiSamplerDescription
{
    QRhiSampler::Filter minFilter;
    QRhiSampler::Filter magFilter;
    QRhiSampler::Filter mipmap;
    QRhiSampler::AddressMode hTiling;
    QRhiSampler::AddressMode vTiling;
    QRhiSampler::AddressMode zTiling;
};

inline bool operator==(const QSSGRhiSamplerDescription &a, const QSSGRhiSamplerDescription &b) Q_DECL_NOTHROW
{
   return a.hTiling == b.hTiling && a.vTiling == b.vTiling && a.zTiling == b.zTiling
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
    LightmapTexture,

    BindingMapSize
};

// these are our current shader limits
#define QSSG_MAX_NUM_LIGHTS 15
#define QSSG_REDUCED_MAX_NUM_LIGHTS 5
#define QSSG_MAX_NUM_SHADOW_MAPS 8

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
    explicit QSSGRhiShaderPipeline(QSSGRhiContext &context) : m_context(context) { }

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
        int material_properties4Idx = -1;
        int material_properties5Idx = -1;
        int material_attenuationIdx = -1;
        int thicknessFactorIdx = -1;
        int rhiPropertiesIdx = -1;
        int displaceAmountIdx = -1;
        int boneTransformsIdx = -1;
        int boneNormalTransformsIdx = -1;
        int shadowDepthAdjustIdx = -1;
        int pointSizeIdx = -1;
        int morphWeightsIdx = -1;
        int reflectionProbeCubeMapCenter = -1;
        int reflectionProbeBoxMax = -1;
        int reflectionProbeBoxMin = -1;
        int reflectionProbeCorrection = -1;
        int specularAAIdx = -1;
        int fogColorIdx = -1;
        int fogSunColorIdx = -1;
        int fogDepthPropertiesIdx = -1;
        int fogHeightPropertiesIdx = -1;
        int fogTransmitPropertiesIdx = -1;

        struct ImageIndices
        {
            int imageRotationsUniformIndex = -1;
            int imageOffsetsUniformIndex = -1;
        };
        QVarLengthArray<ImageIndices, 16> imageIndices;
    } commonUniformIndices;

    struct InstanceLocations {
        int transform0 = -1;
        int transform1 = -1;
        int transform2 = -1;
        int color = -1;
        int data = -1;
    } instanceLocations;

    enum class UniformFlag {
        Mat3 = 0x01
    };
    Q_DECLARE_FLAGS(UniformFlags, UniformFlag)

    void setUniformValue(char *ubufData, const char *name, const QVariant &value, QSSGRenderShaderValue::Type type);
    void setUniform(char *ubufData, const char *name, const void *data, size_t size, int *storeIndex = nullptr, UniformFlags flags = {});
    void setUniformArray(char *ubufData, const char *name, const void *data, size_t itemCount, QSSGRenderShaderValue::Type type, int *storeIndex = nullptr);
    int bindingForTexture(const char *name, int hint = -1);

    void setLightsEnabled(bool enable) { m_lightsEnabled = enable; }
    bool isLightingEnabled() const { return m_lightsEnabled; }

    void resetShadowMaps() { m_shadowMaps.clear(); }
    QSSGRhiShadowMapProperties &addShadowMap() { m_shadowMaps.append(QSSGRhiShadowMapProperties()); return m_shadowMaps.last(); }
    int shadowMapCount() const { return m_shadowMaps.size(); }
    const QSSGRhiShadowMapProperties &shadowMapAt(int index) const { return m_shadowMaps[index]; }
    QSSGRhiShadowMapProperties &shadowMapAt(int index) { return m_shadowMaps[index]; }

    void ensureCombinedMainLightsUniformBuffer(QRhiBuffer **ubuf);
    void ensureUniformBuffer(QRhiBuffer **ubuf);

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

    void setLightmapTexture(QRhiTexture *texture) { m_lightmapTexture = texture; }
    QRhiTexture *lightmapTexture() const { return m_lightmapTexture; }

    void resetExtraTextures() { m_extraTextures.clear(); }
    void addExtraTexture(const QSSGRhiTexture &t) { m_extraTextures.append(t); }
    int extraTextureCount() const { return m_extraTextures.size(); }
    const QSSGRhiTexture &extraTextureAt(int index) const { return m_extraTextures[index]; }
    QSSGRhiTexture &extraTextureAt(int index) { return m_extraTextures[index]; }

    QSSGShaderLightsUniformData &lightsUniformData() { return m_lightsUniformData; }
    InstanceLocations instanceBufferLocations() const { return instanceLocations; }

    int offsetOfUniform(const QByteArray &name);

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
    QVarLengthArray<QSSGRhiShadowMapProperties, QSSG_MAX_NUM_SHADOW_MAPS> m_shadowMaps;
    QRhiTexture *m_lightProbeTexture = nullptr;
    QSSGRenderTextureCoordOp m_lightProbeHorzTile = QSSGRenderTextureCoordOp::ClampToEdge;
    QSSGRenderTextureCoordOp m_lightProbeVertTile = QSSGRenderTextureCoordOp::ClampToEdge;
    QRhiTexture *m_screenTexture = nullptr;
    QRhiTexture *m_depthTexture = nullptr;
    QRhiTexture *m_ssaoTexture = nullptr;
    QRhiTexture *m_lightmapTexture = nullptr;
    QVarLengthArray<QSSGRhiTexture, 8> m_extraTextures;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QSSGRhiShaderPipeline::StageFlags)
Q_DECLARE_OPERATORS_FOR_FLAGS(QSSGRhiShaderPipeline::UniformFlags)

using QSSGRhiShaderPipelinePtr = std::shared_ptr<QSSGRhiShaderPipeline>;

struct Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRhiGraphicsPipelineState
{
    const QSSGRhiShaderPipeline *shaderPipeline;
    int samples = 1;

    bool depthTestEnable = false;
    bool depthWriteEnable = false;
    bool usesStencilRef = false;
    QRhiGraphicsPipeline::CompareOp depthFunc = QRhiGraphicsPipeline::LessOrEqual;
    QRhiGraphicsPipeline::CullMode cullMode = QRhiGraphicsPipeline::None;
    QRhiGraphicsPipeline::StencilOpState stencilOpFrontState {};
    quint32 stencilWriteMask = 0xFF;
    quint32 stencilRef = 0;
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

    QRhiGraphicsPipeline::PolygonMode polygonMode = QRhiGraphicsPipeline::Fill;

    static QRhiGraphicsPipeline::CullMode toCullMode(QSSGCullFaceMode cullFaceMode);
};

inline bool operator==(const QSSGRhiGraphicsPipelineState &a, const QSSGRhiGraphicsPipelineState &b) Q_DECL_NOTHROW
{
    return a.shaderPipeline == b.shaderPipeline
            && a.samples == b.samples
            && a.depthTestEnable == b.depthTestEnable
            && a.depthWriteEnable == b.depthWriteEnable
            && a.usesStencilRef == b.usesStencilRef
            && a.stencilRef == b.stencilRef
            && (std::memcmp(&a.stencilOpFrontState, &b.stencilOpFrontState, sizeof(QRhiGraphicsPipeline::StencilOpState)) == 0)
            && a.stencilWriteMask == b.stencilWriteMask
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
            && a.lineWidth == b.lineWidth
            && a.polygonMode == b.polygonMode;
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
            ^ qHash(s.polygonMode)
            ^ qHashBits(&s.stencilOpFrontState, sizeof(QRhiGraphicsPipeline::StencilOpState))
            ^ (s.depthTestEnable << 1)
            ^ (s.depthWriteEnable << 2)
            ^ (s.blendEnable << 3)
            ^ (s.scissorEnable << 4)
            ^ (s.usesStencilRef << 5)
            ^ (s.stencilRef << 6)
            ^ (s.stencilWriteMask << 7);
}

struct QSSGGraphicsPipelineStateKey
{
    QSSGRhiGraphicsPipelineState state;
    QVector<quint32> renderTargetDescription;
    QVector<quint32> srbLayoutDescription;
    struct {
        size_t renderTargetDescriptionHash;
        size_t srbLayoutDescriptionHash;
    } extra;
    static QSSGGraphicsPipelineStateKey create(const QSSGRhiGraphicsPipelineState &state,
                                               const QRhiRenderPassDescriptor *rpDesc,
                                               const QRhiShaderResourceBindings *srb)
    {
        const QVector<quint32> rtDesc = rpDesc->serializedFormat();
        const QVector<quint32> srbDesc = srb->serializedLayoutDescription();
        return { state, rtDesc, srbDesc, { qHash(rtDesc), qHash(srbDesc) } };
    }
};

inline bool operator==(const QSSGGraphicsPipelineStateKey &a, const QSSGGraphicsPipelineStateKey &b) Q_DECL_NOTHROW
{
    return a.state == b.state
        && a.renderTargetDescription == b.renderTargetDescription
        && a.srbLayoutDescription == b.srbLayoutDescription;
}

inline bool operator!=(const QSSGGraphicsPipelineStateKey &a, const QSSGGraphicsPipelineStateKey &b) Q_DECL_NOTHROW
{
    return !(a == b);
}

inline size_t qHash(const QSSGGraphicsPipelineStateKey &k, size_t seed) Q_DECL_NOTHROW
{
    return qHash(k.state, seed)
        ^ k.extra.renderTargetDescriptionHash
        ^ k.extra.srbLayoutDescriptionHash;
}

struct QSSGComputePipelineStateKey
{
    QShader shader;
    QVector<quint32> srbLayoutDescription;
    struct {
        size_t srbLayoutDescriptionHash;
    } extra;
    static QSSGComputePipelineStateKey create(const QShader &shader,
                                              const QRhiShaderResourceBindings *srb)
    {
        const QVector<quint32> srbDesc = srb->serializedLayoutDescription();
        return { shader, srbDesc, { qHash(srbDesc) } };
    }
};

inline bool operator==(const QSSGComputePipelineStateKey &a, const QSSGComputePipelineStateKey &b) Q_DECL_NOTHROW
{
    return a.shader == b.shader && a.srbLayoutDescription == b.srbLayoutDescription;
}

inline bool operator!=(const QSSGComputePipelineStateKey &a, const QSSGComputePipelineStateKey &b) Q_DECL_NOTHROW
{
    return !(a == b);
}

inline size_t qHash(const QSSGComputePipelineStateKey &k, size_t seed = 0) Q_DECL_NOTHROW
{
    return qHash(k.shader, seed) ^ k.extra.srbLayoutDescriptionHash;
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
    QRhiShaderResourceBinding::Data *d = QRhiImplementation::shaderResourceBindingData(v[p++]);
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
    QRhiShaderResourceBinding::Data *d = QRhiImplementation::shaderResourceBindingData(v[p++]);
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
// the View3D ('layer/cid') and the model ('model') act as the lookup key since
// while the model is the same, we still want different uniform buffers per
// View3D), or the case of shadow maps where the shadow map (there can be as
// many as lights) is taken into account too ('entry') together with an entry index
// where more resolution is needed (e.g., cube maps).
//
struct QSSGRhiDrawCallDataKey
{
    const void *cid = nullptr; // Usually the sub-pass (see usage of QSSGPassKey)
    const void *model = nullptr;
    const void *entry = nullptr;
    quintptr entryIdx = 0;
};

inline bool operator==(const QSSGRhiDrawCallDataKey &a, const QSSGRhiDrawCallDataKey &b) Q_DECL_NOTHROW
{
    return a.cid == b.cid && a.model == b.model && a.entry == b.entry && a.entryIdx == b.entryIdx;
}

inline bool operator!=(const QSSGRhiDrawCallDataKey &a, const QSSGRhiDrawCallDataKey &b) Q_DECL_NOTHROW
{
    return !(a == b);
}

inline size_t qHash(const QSSGRhiDrawCallDataKey &k, size_t seed = 0) Q_DECL_NOTHROW
{
    return qHash(quintptr(k.cid)
                 ^ quintptr(k.model)
                 ^ quintptr(k.entry)
                 ^ quintptr(k.entryIdx), seed);
}

struct QSSGRhiDrawCallData
{
    QRhiBuffer *ubuf = nullptr; // owned
    QRhiShaderResourceBindings *srb = nullptr; // not owned
    QSSGRhiShaderResourceBindingList bindings;
    QRhiGraphicsPipeline *pipeline = nullptr; // not owned
    size_t renderTargetDescriptionHash = 0;
    QVector<quint32> renderTargetDescription;
    QSSGRhiGraphicsPipelineState ps;

    void reset()
    {
        delete ubuf;
        ubuf = nullptr;
        srb = nullptr;
        pipeline = nullptr;
    }
};

struct QSSGRhiRenderableTexture
{
    QRhiTexture *texture = nullptr;
    QRhiRenderBuffer *depthStencil = nullptr;
    QRhiRenderPassDescriptor *rpDesc = nullptr;
    QRhiTextureRenderTarget *rt = nullptr;
    bool isValid() const { return texture && rpDesc && rt; }
    void resetRenderTarget() {
        delete rt;
        rt = nullptr;
        delete rpDesc;
        rpDesc = nullptr;
    }
    void reset() {
        resetRenderTarget();
        delete texture;
        delete depthStencil;
        *this = QSSGRhiRenderableTexture();
    }
};

struct QSSGRhiSortData
{
    float d = 0.0f;
    int indexOrOffset = -1;
};

struct QSSGRhiInstanceBufferData
{
    QRhiBuffer *buffer = nullptr;
    QByteArray sortedData;
    QList<QSSGRhiSortData> sortData;
    QVector3D sortedCameraDirection;
    QVector3D cameraPosition;
    QByteArray lodData;
    int serial = -1;
    bool owned = true;
    bool sorting = false;
};

struct QSSGRhiParticleData
{
    QRhiTexture *texture = nullptr;
    QByteArray sortedData;
    QByteArray convertData;
    QList<QSSGRhiSortData> sortData;
    int particleCount = 0;
    int serial = -1;
    bool sorting = false;
};

struct QSSGRhiDummyTextureKey
{
    QRhiTexture::Flags flags;
    QSize size;
    QColor color;
};

inline size_t qHash(const QSSGRhiDummyTextureKey &k, size_t seed) Q_DECL_NOTHROW
{
    return qHash(k.flags, seed)
            ^ qHash(k.size.width() ^ k.size.height() ^ k.color.red() ^ k.color.green()
                        ^ k.color.blue() ^ k.color.alpha());
}

inline bool operator==(const QSSGRhiDummyTextureKey &a, const QSSGRhiDummyTextureKey &b) Q_DECL_NOTHROW
{
    return a.flags == b.flags && a.size == b.size && a.color == b.color;
}

inline bool operator!=(const QSSGRhiDummyTextureKey &a, const QSSGRhiDummyTextureKey &b) Q_DECL_NOTHROW
{
    return !(a == b);
}

#define QSSGRHICTX_STAT(ctx, f) for (bool qssgrhictxlog_enabled = ctx->stats().isEnabled(); qssgrhictxlog_enabled; qssgrhictxlog_enabled = false) ctx->stats().f

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRhiContextStats
{
public:
    struct DrawInfo {
        quint64 callCount = 0;
        quint64 vertexOrIndexCount = 0;
    };
    struct InstancedDrawInfo {
        quint64 callCount = 0;
        quint64 vertexOrIndexCount = 0;
        quint64 instanceCount = 0;
    };
    struct RenderPassInfo {
        QByteArray rtName;
        QSize pixelSize;
        DrawInfo indexedDraws;
        DrawInfo draws;
        InstancedDrawInfo instancedIndexedDraws;
        InstancedDrawInfo instancedDraws;
    };
    struct PerLayerInfo {
        PerLayerInfo()
        {
            externalRenderPass.rtName = QByteArrayLiteral("Qt Quick");
        }

        // The main render pass if renderMode==Offscreen, plus render passes
        // for shadow maps, postprocessing effects, etc.
        QVector<RenderPassInfo> renderPasses;

        // An Underlay/Overlay/Inline renderMode will make the View3D add stuff
        // to a render pass managed by Qt Quick. (external == not under the
        // control of Qt Quick 3D)
        RenderPassInfo externalRenderPass;

        int currentRenderPassIndex = -1;
    };
    struct GlobalInfo { // global as in per QSSGRhiContext which is per-QQuickWindow
        quint64 meshDataSize = 0;
        quint64 imageDataSize = 0;
        qint64 materialGenerationTime = 0;
        qint64 effectGenerationTime = 0;
    };

    QHash<QSSGRenderLayer *, PerLayerInfo> perLayerInfo;
    GlobalInfo globalInfo;

    QSSGRhiContextStats(QSSGRhiContext &context)
        : context(context)
    {
    }

    // The data collected have four consumers:
    //
    // - Printed on debug output when QSG_RENDERER_DEBUG has the relevant key.
    //   (this way the debug output from the 2D scenegraph renderer and these 3D
    //   statistics appear nicely intermixed)
    // - Passed on to the QML profiler when profiling is enabled.
    // - DebugView via QQuick3DRenderStats.
    // - When tracing is enabled
    //
    // The first two are enabled globally, but DebugView needs a dynamic
    // enable/disable since we want to collect data when a DebugView item
    // becomes visible, but not otherwise.

    static bool profilingEnabled()
    {
        static bool enabled = Q_QUICK3D_PROFILING_ENABLED;
        return enabled;
    }

    static bool rendererDebugEnabled()
    {
        static bool enabled = qgetenv("QSG_RENDERER_DEBUG").contains(QByteArrayLiteral("render"));
        return enabled;
    }

    bool isEnabled() const;
    void drawIndexed(quint32 indexCount, quint32 instanceCount);
    void draw(quint32 vertexCount, quint32 instanceCount);

    void meshDataSizeChanges(quint64 newSize) // can be called outside start-stop
    {
        globalInfo.meshDataSize = newSize;
    }

    void imageDataSizeChanges(quint64 newSize) // can be called outside start-stop
    {
        globalInfo.imageDataSize = newSize;
    }

    void registerMaterialShaderGenerationTime(qint64 ms)
    {
        globalInfo.materialGenerationTime += ms;
    }

    void registerEffectShaderGenerationTime(qint64 ms)
    {
        globalInfo.effectGenerationTime += ms;
    }

    static quint64 totalDrawCallCountForPass(const QSSGRhiContextStats::RenderPassInfo &pass)
    {
        return pass.draws.callCount
                + pass.indexedDraws.callCount
                + pass.instancedDraws.callCount
                + pass.instancedIndexedDraws.callCount;
    }

    static quint64 totalVertexCountForPass(const QSSGRhiContextStats::RenderPassInfo &pass)
    {
        return pass.draws.vertexOrIndexCount
                + pass.indexedDraws.vertexOrIndexCount
                + pass.instancedDraws.vertexOrIndexCount
                + pass.instancedIndexedDraws.vertexOrIndexCount;
    }

    void start(QSSGRenderLayer *layer);
    void stop(QSSGRenderLayer *layer);
    void beginRenderPass(QRhiTextureRenderTarget *rt);
    void endRenderPass();
    void printRenderPass(const RenderPassInfo &rp);
    void cleanupLayerInfo(QSSGRenderLayer *layer);

    QSSGRhiContext &context;
    QSSGRenderLayer *layerKey = nullptr;
    QSet<QSSGRenderLayer *> dynamicDataSources;
};

class QSSGRenderGraphObject;

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRhiContext
{
    Q_DISABLE_COPY(QSSGRhiContext)
public:
    explicit QSSGRhiContext(QRhi *rhi = nullptr);
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

    QRhiShaderResourceBindings *srb(const QSSGRhiShaderResourceBindingList &bindings);
    void releaseDrawCallData(QSSGRhiDrawCallData &dcd);
    QRhiGraphicsPipeline *pipeline(const QSSGGraphicsPipelineStateKey &key,
                                   QRhiRenderPassDescriptor *rpDesc,
                                   QRhiShaderResourceBindings *srb);
    QRhiComputePipeline *computePipeline(const QSSGComputePipelineStateKey &key,
                                         QRhiShaderResourceBindings *srb);

    QSSGRhiDrawCallData &drawCallData(const QSSGRhiDrawCallDataKey &key)
    {
        return m_drawCallData[key];
    }

    QRhiSampler *sampler(const QSSGRhiSamplerDescription &samplerDescription);
    void checkAndAdjustForNPoT(QRhiTexture *texture, QSSGRhiSamplerDescription *samplerDescription);

    void registerTexture(QRhiTexture *texture);
    void releaseTexture(QRhiTexture *texture);
    QSet<QRhiTexture *> registeredTextures() const { return m_textures; }

    void registerMesh(QSSGRenderMesh *mesh);
    void releaseMesh(QSSGRenderMesh *mesh);
    QSet<QSSGRenderMesh *> registeredMeshes() const { return m_meshes; }

    QHash<QSSGGraphicsPipelineStateKey, QRhiGraphicsPipeline *> pipelines() const { return m_pipelines; }

    void cleanupDrawCallData(const QSSGRenderModel *model);

    QRhiTexture *dummyTexture(QRhiTexture::Flags flags, QRhiResourceUpdateBatch *rub,
                              const QSize &size = QSize(64, 64), const QColor &fillColor = Qt::black);

    static inline QRhiCommandBuffer::BeginPassFlags commonPassFlags()
    {
        // We do not use GPU compute at all at the moment, this means we can
        // get a small performance gain with OpenGL by declaring this.
        return QRhiCommandBuffer::DoNotTrackResourcesForCompute;
    }

    static bool shaderDebuggingEnabled();
    static bool editorMode();

    QSSGRhiInstanceBufferData &instanceBufferData(QSSGRenderInstanceTable *instanceTable)
    {
        return m_instanceBuffers[instanceTable];
    }

    QSSGRhiInstanceBufferData &instanceBufferData(const QSSGRenderModel *model)
    {
        return m_instanceBuffersLod[model];
    }

    QSSGRhiParticleData &particleData(const QSSGRenderGraphObject *particlesOrModel)
    {
        return m_particleData[particlesOrModel];
    }

    QSSGRhiContextStats &stats() { return m_stats; }

    int maxUniformBufferRange() const { return m_rhi->resourceLimit(QRhi::MaxUniformBufferRange); }

    void releaseCachedResources();

private:
    QRhi *m_rhi = nullptr;
    QRhiRenderPassDescriptor *m_mainRpDesc = nullptr;
    QRhiCommandBuffer *m_cb = nullptr;
    QRhiRenderTarget *m_rt = nullptr;
    int m_mainSamples = 1;
    QHash<QSSGRhiShaderResourceBindingList, QRhiShaderResourceBindings *> m_srbCache;
    QHash<QSSGGraphicsPipelineStateKey, QRhiGraphicsPipeline *> m_pipelines;
    QHash<QSSGComputePipelineStateKey, QRhiComputePipeline *> m_computePipelines;
    QHash<QSSGRhiDrawCallDataKey, QSSGRhiDrawCallData> m_drawCallData;
    QVector<QPair<QSSGRhiSamplerDescription, QRhiSampler*>> m_samplers;
    QSet<QRhiTexture *> m_textures;
    QSet<QSSGRenderMesh *> m_meshes;
    QHash<QSSGRhiDummyTextureKey, QRhiTexture *> m_dummyTextures;
    QHash<QSSGRenderInstanceTable *, QSSGRhiInstanceBufferData> m_instanceBuffers;
    QHash<const QSSGRenderModel *, QSSGRhiInstanceBufferData> m_instanceBuffersLod;
    QHash<const QSSGRenderGraphObject *, QSSGRhiParticleData> m_particleData;
    QSSGRhiContextStats m_stats;
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
