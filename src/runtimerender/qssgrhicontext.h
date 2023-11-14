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

struct QSSGRhiInputAssemblerState
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
};

struct QSSGRhiGraphicsPipelineState
{
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
    float lineWidth = 1.0f;
    QRhiGraphicsPipeline::PolygonMode polygonMode = QRhiGraphicsPipeline::Fill;

    // for internal use
    const QSSGRhiShaderPipeline *shaderPipeline = nullptr;
    QSSGRhiInputAssemblerState ia;
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

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRhiContext
{
    Q_DISABLE_COPY(QSSGRhiContext)
public:
    explicit QSSGRhiContext(QRhi *rhi);
    ~QSSGRhiContext();

    QRhi *rhi() const;
    bool isValid() const;

    QRhiRenderPassDescriptor *mainRenderPassDescriptor() const;
    QRhiCommandBuffer *commandBuffer() const;
    QRhiRenderTarget *renderTarget() const;
    int mainPassSampleCount() const;

    QRhiSampler *sampler(const QSSGRhiSamplerDescription &samplerDescription);
    void checkAndAdjustForNPoT(QRhiTexture *texture, QSSGRhiSamplerDescription *samplerDescription);
    QRhiTexture *dummyTexture(QRhiTexture::Flags flags, QRhiResourceUpdateBatch *rub,
                              const QSize &size = QSize(64, 64), const QColor &fillColor = Qt::black);

    QRhiCommandBuffer::BeginPassFlags commonPassFlags() const;

private:
    Q_DECLARE_PRIVATE(QSSGRhiContext)
    std::unique_ptr<QSSGRhiContextPrivate> d_ptr;
};

QT_END_NAMESPACE

#endif
