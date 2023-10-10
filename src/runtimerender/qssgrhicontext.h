// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QSSGRHICONTEXT_H
#define QSSGRHICONTEXT_H

//
//  W A R N I N G
//  -------------
//
// This file is part of the QtQuick3D API, with limited compatibility guarantees.
// Usage of this API may make your code source and binary incompatible with
// future versions of Qt.
//

#include <QtCore/qstack.h>
#include <rhi/qrhi.h>

#include <QtQuick3DRuntimeRender/qtquick3druntimerenderexports.h>
#include <ssg/qssgrenderbasetypes.h>

QT_BEGIN_NAMESPACE

class QSSGRhiContextPrivate;
class QSSGRhiShaderPipeline;
struct QSSGRenderMesh;

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

struct Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRhiGraphicsPipelineState
{
    const QSSGRhiShaderPipeline *shaderPipeline = nullptr;
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

struct QSSGRhiSamplerDescription
{
    QRhiSampler::Filter minFilter;
    QRhiSampler::Filter magFilter;
    QRhiSampler::Filter mipmap;
    QRhiSampler::AddressMode hTiling;
    QRhiSampler::AddressMode vTiling;
    QRhiSampler::AddressMode zTiling;
};

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

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRhiContext
{
    Q_DISABLE_COPY(QSSGRhiContext)
public:
    explicit QSSGRhiContext(QRhi *rhi);
    ~QSSGRhiContext();

    QRhi *rhi() const;
    bool isValid() const;

    void setMainRenderPassDescriptor(QRhiRenderPassDescriptor *rpDesc);
    QRhiRenderPassDescriptor *mainRenderPassDescriptor() const;

    void setCommandBuffer(QRhiCommandBuffer *cb);
    QRhiCommandBuffer *commandBuffer() const;

    void setRenderTarget(QRhiRenderTarget *rt);
    QRhiRenderTarget *renderTarget() const;

    void setMainPassSampleCount(int samples);
    int mainPassSampleCount() const;

    QRhiShaderResourceBindings *srb(const QSSGRhiShaderResourceBindingList &bindings);
    QRhiGraphicsPipeline *pipeline(const QSSGRhiGraphicsPipelineState &ps,
                                   QRhiRenderPassDescriptor *rpDesc,
                                   QRhiShaderResourceBindings *srb);
    QRhiComputePipeline *computePipeline(const QShader &shader,
                                         QRhiShaderResourceBindings *srb);

    QRhiSampler *sampler(const QSSGRhiSamplerDescription &samplerDescription);
    void checkAndAdjustForNPoT(QRhiTexture *texture, QSSGRhiSamplerDescription *samplerDescription);

    void registerTexture(QRhiTexture *texture);
    void releaseTexture(QRhiTexture *texture);

    void registerMesh(QSSGRenderMesh *mesh);
    void releaseMesh(QSSGRenderMesh *mesh);

    QRhiTexture *dummyTexture(QRhiTexture::Flags flags, QRhiResourceUpdateBatch *rub,
                              const QSize &size = QSize(64, 64), const QColor &fillColor = Qt::black);

    static constexpr QRhiCommandBuffer::BeginPassFlags commonPassFlags()
    {
        // We do not use GPU compute at all at the moment, this means we can
        // get a small performance gain with OpenGL by declaring this.
        return QRhiCommandBuffer::DoNotTrackResourcesForCompute;
    }

    void releaseCachedResources();

private:
    friend class QSSGRhiContextStats;

    Q_DECLARE_PRIVATE(QSSGRhiContext)
    std::unique_ptr<QSSGRhiContextPrivate> d_ptr;
};

QT_END_NAMESPACE

#endif
