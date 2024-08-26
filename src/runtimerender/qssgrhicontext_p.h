// Copyright (C) 2023 The Qt Company Ltd.
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

#include <QtGui/rhi/qrhi.h>

#include <QtQuick3DRuntimeRender/qtquick3druntimerenderexports.h>
#include <QtQuick3DUtils/private/qssgrenderbasetypes_p.h>
#include <ssg/qssgrhicontext.h>

QT_BEGIN_NAMESPACE

struct QSSGRenderLayer;
struct QSSGRenderInstanceTable;
struct QSSGRenderModel;
struct QSSGRenderMesh;
class QSSGRenderGraphObject;

struct QSSGRhiInputAssemblerStatePrivate
{
    using InputAssemblerState = QSSGRhiGraphicsPipelineState::InputAssemblerState;
    static const InputAssemblerState &get(const QSSGRhiGraphicsPipelineState &ps) { return ps.ia; }
    static InputAssemblerState &get(QSSGRhiGraphicsPipelineState &ps) { return ps.ia; }
};

using QSSGRhiInputAssemblerState = QSSGRhiInputAssemblerStatePrivate::InputAssemblerState;

struct QSSGRhiGraphicsPipelineStatePrivate
{
    static void setShaderPipeline(QSSGRhiGraphicsPipelineState &ps, const QSSGRhiShaderPipeline *pipeline)
    {
        ps.shaderPipeline = pipeline;
    }

    static constexpr const QSSGRhiShaderPipeline *getShaderPipeline(const QSSGRhiGraphicsPipelineState &ps)
    {
        return ps.shaderPipeline;
    }
};

namespace QSSGRhiHelpers
{

inline QRhiSampler::Filter toRhi(QSSGRenderTextureFilterOp op)
{
    switch (op) {
    case QSSGRenderTextureFilterOp::Nearest:
        return QRhiSampler::Nearest;
    case QSSGRenderTextureFilterOp::Linear:
        return QRhiSampler::Linear;
    case QSSGRenderTextureFilterOp::None:
        return QRhiSampler::Linear;
    }

    Q_UNREACHABLE_RETURN(QRhiSampler::Linear);
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
    case QSSGRenderTextureCoordOp::Unknown:
        return QRhiSampler::ClampToEdge;
    }

    Q_UNREACHABLE_RETURN(QRhiSampler::ClampToEdge);
}

inline QRhiGraphicsPipeline::CullMode toCullMode(QSSGCullFaceMode cullFaceMode)
{
    switch (cullFaceMode) {
    case QSSGCullFaceMode::Back:
        return QRhiGraphicsPipeline::Back;
    case QSSGCullFaceMode::Front:
        return QRhiGraphicsPipeline::Front;
    case QSSGCullFaceMode::Disabled:
        return QRhiGraphicsPipeline::None;
    case QSSGCullFaceMode::FrontAndBack:
        qWarning("FrontAndBack cull mode not supported");
        return QRhiGraphicsPipeline::None;
    case QSSGCullFaceMode::Unknown:
        return QRhiGraphicsPipeline::None;
    }

    Q_UNREACHABLE_RETURN(QRhiGraphicsPipeline::None);
}

QRhiVertexInputAttribute::Format toVertexInputFormat(QSSGRenderComponentType compType, quint32 numComps);
QRhiGraphicsPipeline::Topology toTopology(QSSGRenderDrawMode drawMode);
// Fills out inputLayout.attributes[].location based on
// inputLayoutInputNames and the provided shader reflection info.
void bakeVertexInputLocations(QSSGRhiInputAssemblerState *ia, const QSSGRhiShaderPipeline &shaders, int instanceBufferBinding = 0);

} // namespace QSSGRhiHelpers

inline bool operator==(const QSSGRhiGraphicsPipelineState &a, const QSSGRhiGraphicsPipelineState &b) Q_DECL_NOTHROW
{
    const auto &ia_a = QSSGRhiInputAssemblerStatePrivate::get(a);
    const auto &ia_b = QSSGRhiInputAssemblerStatePrivate::get(b);
    return QSSGRhiGraphicsPipelineStatePrivate::getShaderPipeline(a) == QSSGRhiGraphicsPipelineStatePrivate::getShaderPipeline(b)
            && a.samples == b.samples
            && a.flags == b.flags
            && a.stencilRef == b.stencilRef
            && (std::memcmp(&a.stencilOpFrontState, &b.stencilOpFrontState, sizeof(QRhiGraphicsPipeline::StencilOpState)) == 0)
            && a.stencilWriteMask == b.stencilWriteMask
            && a.depthFunc == b.depthFunc
            && a.cullMode == b.cullMode
            && a.depthBias == b.depthBias
            && a.slopeScaledDepthBias == b.slopeScaledDepthBias
            && a.viewport == b.viewport
            && a.scissor == b.scissor
            && ia_a.topology == ia_b.topology
            && ia_a.inputLayout == ia_b.inputLayout
            && a.targetBlend.colorWrite == b.targetBlend.colorWrite
            && a.targetBlend.srcColor == b.targetBlend.srcColor
            && a.targetBlend.dstColor == b.targetBlend.dstColor
            && a.targetBlend.opColor == b.targetBlend.opColor
            && a.targetBlend.srcAlpha == b.targetBlend.srcAlpha
            && a.targetBlend.dstAlpha == b.targetBlend.dstAlpha
            && a.targetBlend.opAlpha == b.targetBlend.opAlpha
            && a.colorAttachmentCount == b.colorAttachmentCount
            && a.lineWidth == b.lineWidth
            && a.polygonMode == b.polygonMode
            && a.viewCount == b.viewCount;
}

inline bool operator!=(const QSSGRhiGraphicsPipelineState &a, const QSSGRhiGraphicsPipelineState &b) Q_DECL_NOTHROW
{
    return !(a == b);
}

inline size_t qHash(const QSSGRhiGraphicsPipelineState &s, size_t seed) Q_DECL_NOTHROW
{
    // do not bother with all fields
    return qHash(QSSGRhiGraphicsPipelineStatePrivate::getShaderPipeline(s), seed)
            ^ qHash(s.samples)
            ^ qHash(s.viewCount)
            ^ qHash(s.targetBlend.dstColor)
            ^ qHash(s.depthFunc)
            ^ qHash(s.cullMode)
            ^ qHash(s.colorAttachmentCount)
            ^ qHash(s.lineWidth)
            ^ qHash(s.polygonMode)
            ^ qHashBits(&s.stencilOpFrontState, sizeof(QRhiGraphicsPipeline::StencilOpState))
            ^ (s.flags)
            ^ (s.stencilRef << 6)
            ^ (s.stencilWriteMask << 7);
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
    QRhiTexture *texture = nullptr;
    QSSGRhiSamplerDescription samplerDesc;
};

enum class QSSGRhiSamplerBindingHints
{
    LightProbe = 64, // must be larger than the largest value in SSGRenderableImage::Type
    ScreenTexture,
    DepthTexture,
    AoTexture,
    LightmapTexture,
    DepthTextureArray,
    ScreenTextureArray,
    AoTextureArray,

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

// note this struct must exactly match the memory layout of the uniform block in
// funcSampleLightVars.glsllib
struct QSSGShaderShadowData {
    float matrices[4][16];
    float dimensionsInverted[4][4];
    float csmSplits[4];
    float csmActive[4];

    float bias;
    float factor;
    float isYUp;
    float clipNear;

    float shadowMapFar;
    qint32 layerIndex;
    qint32 csmNumSplits;
    float csmBlendRatio;

    float pcfFactor;
    float padding[3];
};

struct QSSGShaderShadowsUniformData
{
    qint32 count = -1;
    float padding[3]; // first element must start at a vec4-aligned offset
    QSSGShaderShadowData shadowData[QSSG_MAX_NUM_SHADOW_MAPS];
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
    int ub0ShadowDataOffset() const;
    int ub0ShadowDataSize() const
    {
        return int(4 * sizeof(qint32) + m_shadowsUniformData.count * sizeof(QSSGShaderShadowData));
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
        int clearcoatNormalStrengthIdx = -1;
        int clearcoatFresnelPowerIdx = -1;
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

    void ensureCombinedUniformBuffer(QRhiBuffer **ubuf);
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
    QSSGShaderShadowsUniformData &shadowsUniformData() { return m_shadowsUniformData; }
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
    QSSGShaderShadowsUniformData m_shadowsUniformData;
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

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRhiShaderResourceBindingList
{
public:
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

    void addUniformBuffer(int binding, QRhiShaderResourceBinding::StageFlags stage, QRhiBuffer *buf, int offset = 0 , int size = 0);
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
    QRhiTexture *depthTexture = nullptr; // either depthStencil or depthTexture are valid, never both
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
        delete depthTexture;
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

class QSSGComputePipelineStateKey
{
public:
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

struct QSSGRhiDummyTextureKey
{
    QRhiTexture::Flags flags;
    QSize size;
    QColor color;
    int arraySize;
};

inline size_t qHash(const QSSGRhiDummyTextureKey &k, size_t seed) Q_DECL_NOTHROW
{
    return qHash(k.flags, seed)
            ^ qHash(k.size.width() ^ k.size.height() ^ k.color.red() ^ k.color.green()
                        ^ k.color.blue() ^ k.color.alpha() ^ k.arraySize);
}

inline bool operator==(const QSSGRhiDummyTextureKey &a, const QSSGRhiDummyTextureKey &b) Q_DECL_NOTHROW
{
    return a.flags == b.flags && a.size == b.size && a.color == b.color && a.arraySize == b.arraySize;
}

inline bool operator!=(const QSSGRhiDummyTextureKey &a, const QSSGRhiDummyTextureKey &b) Q_DECL_NOTHROW
{
    return !(a == b);
}

class QSSGGraphicsPipelineStateKey
{
public:
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

#define QSSGRHICTX_STAT(ctx, f) \
    for (bool qssgrhictxlog_enabled = QSSGRhiContextStats::get(*ctx).isEnabled(); qssgrhictxlog_enabled; qssgrhictxlog_enabled = false) \
        QSSGRhiContextStats::get(*ctx).f

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRhiContextStats
{
public:
    [[nodiscard]] static QSSGRhiContextStats &get(QSSGRhiContext &rhiCtx);
    [[nodiscard]] static const QSSGRhiContextStats &get(const QSSGRhiContext &rhiCtx);

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
        : rhiCtx(&context)
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

    static bool profilingEnabled();
    static bool rendererDebugEnabled();

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

    QSSGRhiContext *rhiCtx;
    QSSGRenderLayer *layerKey = nullptr;
    QSet<QSSGRenderLayer *> dynamicDataSources;
};

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRhiContextPrivate
{
    Q_DECLARE_PUBLIC(QSSGRhiContext)

    explicit QSSGRhiContextPrivate(QSSGRhiContext &rhiCtx, QRhi *rhi_)
        : q_ptr(&rhiCtx)
        , m_rhi(rhi_)
        , m_stats(rhiCtx)
    {}

public:
    using Textures = QSet<QRhiTexture *>;
    using Meshes = QSet<QSSGRenderMesh *>;

    [[nodiscard]] static QSSGRhiContextPrivate *get(QSSGRhiContext *q) { return q->d_ptr.get(); }
    [[nodiscard]] static const QSSGRhiContextPrivate *get(const QSSGRhiContext *q) { return q->d_ptr.get(); }

    [[nodiscard]] static bool shaderDebuggingEnabled();
    [[nodiscard]] static bool editorMode();

    void setMainRenderPassDescriptor(QRhiRenderPassDescriptor *rpDesc);
    void setCommandBuffer(QRhiCommandBuffer *cb);
    void setRenderTarget(QRhiRenderTarget *rt);
    void setMainPassSampleCount(int samples);
    void setMainPassViewCount(int viewCount);

    void releaseCachedResources();

    void registerTexture(QRhiTexture *texture);
    void releaseTexture(QRhiTexture *texture);

    void registerMesh(QSSGRenderMesh *mesh);
    void releaseMesh(QSSGRenderMesh *mesh);

    QRhiShaderResourceBindings *srb(const QSSGRhiShaderResourceBindingList &bindings);
    void releaseCachedSrb(QSSGRhiShaderResourceBindingList &bindings);

    QRhiGraphicsPipeline *pipeline(const QSSGRhiGraphicsPipelineState &ps,
                                   QRhiRenderPassDescriptor *rpDesc,
                                   QRhiShaderResourceBindings *srb);

    QRhiGraphicsPipeline *pipeline(const QSSGGraphicsPipelineStateKey &key,
                                   QRhiRenderPassDescriptor *rpDesc,
                                   QRhiShaderResourceBindings *srb);

    QRhiComputePipeline *computePipeline(const QShader &shader,
                                         QRhiShaderResourceBindings *srb);

    QRhiComputePipeline *computePipeline(const QSSGComputePipelineStateKey &key,
                                         QRhiShaderResourceBindings *srb);

    QSSGRhiDrawCallData &drawCallData(const QSSGRhiDrawCallDataKey &key);
    void releaseDrawCallData(QSSGRhiDrawCallData &dcd);
    void cleanupDrawCallData(const QSSGRenderModel *model);

    QSSGRhiInstanceBufferData &instanceBufferData(QSSGRenderInstanceTable *instanceTable);
    void releaseInstanceBuffer(QSSGRenderInstanceTable *instanceTable);

    QSSGRhiInstanceBufferData &instanceBufferData(const QSSGRenderModel *model);

    QSSGRhiParticleData &particleData(const QSSGRenderGraphObject *particlesOrModel);

    QSSGRhiContext *q_ptr = nullptr;
    QRhi *m_rhi = nullptr;

    QRhiRenderPassDescriptor *m_mainRpDesc = nullptr;
    QRhiCommandBuffer *m_cb = nullptr;
    QRhiRenderTarget *m_rt = nullptr;
    Textures m_textures;
    Meshes m_meshes;
    int m_mainSamples = 1;
    int m_mainViewCount = 1;

    QVector<QPair<QSSGRhiSamplerDescription, QRhiSampler*>> m_samplers;

    QHash<QSSGRhiDrawCallDataKey, QSSGRhiDrawCallData> m_drawCallData;
    QHash<QSSGRhiShaderResourceBindingList, QRhiShaderResourceBindings *> m_srbCache;
    QHash<QSSGGraphicsPipelineStateKey, QRhiGraphicsPipeline *> m_pipelines;
    QHash<QSSGComputePipelineStateKey, QRhiComputePipeline *> m_computePipelines;
    QHash<QSSGRhiDummyTextureKey, QRhiTexture *> m_dummyTextures;
    QHash<QSSGRenderInstanceTable *, QSSGRhiInstanceBufferData> m_instanceBuffers;
    QHash<const QSSGRenderModel *, QSSGRhiInstanceBufferData> m_instanceBuffersLod;
    QHash<const QSSGRenderGraphObject *, QSSGRhiParticleData> m_particleData;
    QSSGRhiContextStats m_stats;
};

inline bool operator==(const QSSGRhiDrawCallDataKey &a, const QSSGRhiDrawCallDataKey &b) noexcept
{
    return a.cid == b.cid && a.model == b.model && a.entry == b.entry && a.entryIdx == b.entryIdx;
}

inline bool operator!=(const QSSGRhiDrawCallDataKey &a, const QSSGRhiDrawCallDataKey &b) noexcept
{
    return !(a == b);
}

inline size_t qHash(const QSSGRhiDrawCallDataKey &k, size_t seed = 0) noexcept
{
    return qHash(quintptr(k.cid)
                 ^ quintptr(k.model)
                 ^ quintptr(k.entry)
                 ^ quintptr(k.entryIdx), seed);
}

QT_END_NAMESPACE

#endif // QSSGRHICONTEXT_P_H
