// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "outlinerenderextension.h"

#include <rhi/qrhi.h>

#include <QtQuick3D/qquick3dobject.h>
#include <ssg/qquick3dextensionhelpers.h>

#include <ssg/qssgrenderhelpers.h>
#include <ssg/qssgrenderextensions.h>
#include <ssg/qssgrendercontextcore.h>

class OutlineRenderer : public QSSGRenderExtension
{
public:
    OutlineRenderer() = default;

    bool prepareData(QSSGFrameData &data) override;
    void prepareRender(QSSGFrameData &data) override;
    void render(QSSGFrameData &data) override;
    void resetForFrame() override;
    RenderMode mode() const override { return RenderMode::Main; }
    RenderStage stage() const override { return RenderStage::PostColor; };

    QSSGPrepContextId stencilPrepContext { QSSGPrepContextId::Invalid };
    QSSGPrepContextId outlinePrepContext { QSSGPrepContextId::Invalid };
    QSSGPrepResultId stencilPrepResult { QSSGPrepResultId::Invalid };
    QSSGPrepResultId outlinePrepResult { QSSGPrepResultId::Invalid };
    QSSGNodeId modelId { QSSGNodeId::Invalid }; // TODO: Request this each time
    QSSGResourceId outlineMaterialId {};
    float outlineScale = 1.05f;

    QSSGRenderablesId stencilRenderables;
    QSSGRenderablesId outlineRenderables;
};

bool OutlineRenderer::prepareData(QSSGFrameData &data)
{
    // Make sure we have a tagetId.
    if (modelId == QSSGNodeId::Invalid)
        return false;

    // This is the active camera for the scene (the camera used to render the final scene)
    auto camera = data.activeCamera();
    if (camera == QSSGNodeId::Invalid) // TODO: Make it easier
        return false;

    //
    stencilPrepContext = QSSGRenderHelpers::prepareForRender(data, *this, camera, 0);
    outlinePrepContext = QSSGRenderHelpers::prepareForRender(data, *this, camera, 1);
    // Create data set for this render
    stencilRenderables = QSSGRenderHelpers::createRenderables(data, stencilPrepContext, { modelId });
    outlineRenderables = QSSGRenderHelpers::createRenderables(data, outlinePrepContext, { modelId });

    // Now we can start setting data for our models
    QSSGModelHelpers::setModelMaterials(data, outlineRenderables, modelId, { outlineMaterialId });
    auto globalTransform = QSSGModelHelpers::getGlobalTransform(data, modelId);
    globalTransform.scale(outlineScale);
    QSSGModelHelpers::setGlobalTransform(data, outlineRenderables, modelId, globalTransform);

    // We're not drawing to the color output so set the opacity to 0.0f
    QSSGModelHelpers::setGlobalOpacity(data, stencilRenderables, modelId, 0.0f);

    // Commit the changes
    stencilPrepResult = QSSGRenderHelpers::commit(data, stencilPrepContext, stencilRenderables);
    outlinePrepResult = QSSGRenderHelpers::commit(data, outlinePrepContext, outlineRenderables);

    return true; // wasDirty
}

void OutlineRenderer::prepareRender(QSSGFrameData &data)
{
    if (modelId == QSSGNodeId::Invalid)
        return;

    if (stencilPrepResult == QSSGPrepResultId::Invalid || outlinePrepResult == QSSGPrepResultId::Invalid)
        return;

    const auto &ctx = data.contextInterface();

    if (const auto &rhiCtx = ctx->rhiContext()) {
        const auto basePs = data.getPipelineState();
        QRhiRenderPassDescriptor *rpDesc = rhiCtx->mainRenderPassDescriptor();
        const int samples = rhiCtx->mainPassSampleCount();

        auto ps = basePs;

        { // Original
            ps.flags |= { QSSGRhiGraphicsPipelineState::Flag::BlendEnabled,
                          QSSGRhiGraphicsPipelineState::Flag::DepthWriteEnabled,
                          QSSGRhiGraphicsPipelineState::Flag::UsesStencilRef,
                          QSSGRhiGraphicsPipelineState::Flag::DepthTestEnabled };
            ps.stencilWriteMask = 0xff;
            ps.stencilRef = 1;
            ps.samples = samples;
            ps.cullMode = QRhiGraphicsPipeline::Back;

            ps.stencilOpFrontState = { QRhiGraphicsPipeline::Keep,
                                       QRhiGraphicsPipeline::Keep,
                                       QRhiGraphicsPipeline::Replace,
                                       QRhiGraphicsPipeline::Always };

            QSSGRenderHelpers::prepareRenderables(data, stencilPrepResult, rpDesc, ps);
        }

        { // Scaled and cut-out
            auto ps = basePs;
            ps.flags |= { QSSGRhiGraphicsPipelineState::Flag::BlendEnabled,
                          QSSGRhiGraphicsPipelineState::Flag::UsesStencilRef,
                          QSSGRhiGraphicsPipelineState::Flag::DepthTestEnabled };
            ps.flags.setFlag(QSSGRhiGraphicsPipelineState::Flag::DepthWriteEnabled, false);
            ps.stencilWriteMask = 0;
            ps.stencilRef = 1;
            ps.cullMode = QRhiGraphicsPipeline::Back;

            ps.stencilOpFrontState = { QRhiGraphicsPipeline::Keep,
                                       QRhiGraphicsPipeline::Keep,
                                       QRhiGraphicsPipeline::Replace,
                                       QRhiGraphicsPipeline::NotEqual };

            QSSGRenderHelpers::prepareRenderables(data, outlinePrepResult, rpDesc, ps);
        }
    }
}

void OutlineRenderer::render(QSSGFrameData &data)
{
    if (stencilPrepResult == QSSGPrepResultId::Invalid)
        return;

    const auto &ctx = data.contextInterface();
    if (const auto &rhiCtx = ctx->rhiContext()) {
        QRhiCommandBuffer *cb = rhiCtx->commandBuffer();
        cb->debugMarkBegin(QByteArrayLiteral("Stencil outline pass"));
        QSSGRenderHelpers::renderRenderables(*ctx, stencilPrepResult);
        QSSGRenderHelpers::renderRenderables(*ctx, outlinePrepResult);
        cb->debugMarkEnd();
    }
}

void OutlineRenderer::resetForFrame()
{
    stencilPrepContext = { QSSGPrepContextId::Invalid };
    stencilPrepResult = { QSSGPrepResultId::Invalid };
}

OutlineRenderExtension::~OutlineRenderExtension() {}

float OutlineRenderExtension::outlineScale() const
{
    return m_outlineScale;
}

void OutlineRenderExtension::setOutlineScale(float newOutlineScale)
{
    auto &outlineScale = m_outlineScale;
    if (qFuzzyCompare(outlineScale, newOutlineScale))
        return;
    outlineScale = newOutlineScale;

    markDirty(Dirty::OutlineScale);

    emit outlineScaleChanged();
}

QQuick3DObject *OutlineRenderExtension::target() const
{
    return m_target;
}

void OutlineRenderExtension::setTarget(QQuick3DObject *newTarget)
{
    auto &target = m_target;
    if (target == newTarget)
        return;
    target = newTarget;

    markDirty(Dirty::Target);

    emit targetChanged();
}

// This is our sync point
QSSGRenderGraphObject *OutlineRenderExtension::updateSpatialNode(QSSGRenderGraphObject *node)
{
    if (!node)
        node = new OutlineRenderer;

    OutlineRenderer *renderer = static_cast<OutlineRenderer *>(node);
    renderer->outlineScale = m_outlineScale;
    if (m_target)
        renderer->modelId = QQuick3DExtensionHelpers::getNodeId(*m_target);
    if (m_outlineMaterial)
        renderer->outlineMaterialId = QQuick3DExtensionHelpers::getResourceId(*m_outlineMaterial);

    m_dirtyFlag = {};

    return node;
}

void OutlineRenderExtension::markDirty(Dirty v)
{
    m_dirtyFlag |= v;
    update();
}

QQuick3DObject *OutlineRenderExtension::outlineMaterial() const
{
    return m_outlineMaterial;
}

void OutlineRenderExtension::setOutlineMaterial(QQuick3DObject *newOutlineMaterial)
{
    if (m_outlineMaterial == newOutlineMaterial)
        return;

    m_outlineMaterial = newOutlineMaterial;

    markDirty(Dirty::OutlineMaterial);
    emit outlineMaterialChanged();
}
