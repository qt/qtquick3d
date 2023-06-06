// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "outlinerenderextension.h"

#include <rhi/qrhi.h>

#include <QtQuick3D/qquick3dobject.h>
#include <QtQuick3D/private/qquick3dextensionhelpers_p.h>

#include <QtQuick3DRuntimeRender/private/qssgrenderhelpers_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderextensions_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercontextcore_p.h>

class OutlineRenderer : public QSSGRenderExtension
{
public:
    OutlineRenderer() = default;

    bool prepareData(QSSGFrameData &data) override;
    void prepareRender(const QSSGRenderer &renderer, QSSGFrameData &data) override;
    void render(const QSSGRenderer &renderer) override;
    void release() override;
    Type type() const override { return Type::Main; }
    RenderMode mode() const override { return RenderMode::Overlay; };

    QSSGNodeId targetId = 0;
    QSSGResourceId outlineMaterialId = 0;
    float outlineScale = 1.05f;

    QSSGRenderDefaultMaterial *outlineMaterial = nullptr;

    QList<QSSGRenderableNodeEntry> nodes;
    QList<QSSGRenderGraphObject *> outlineMaterials;
    QList<QSSGModelContext *> renderables;
    QSSGRhiGraphicsPipelineState pipelineStates[2] {};
};

bool OutlineRenderer::prepareData(QSSGFrameData &data)
{
    const auto &ctx = data.renderer()->contextInterface();

    // Make sure we have a tagetId.
    if (!targetId)
        return false;

    // We're just going to steal the node here as we're going to render it ourself.
    auto node = data.takeNode(targetId);

    if (node.isNull())
        return false;

    // We're only doing one object here, but by placing the object twice in the list we're going to
    // create renderables for both.
    nodes = { node, node };

    // This is the active camera for the scene (the camera used to render the final scene)
    auto *camera = data.camera();
    if (!camera)
        return false;

    float meshLodThreshold = 1.0f;

    // Ensure that meshes are loaded for our models.
    QSSGModelHelpers::ensureMeshes(*ctx, nodes);

    // Assuming our node has a mesh and a material we should have _two_ entries ready.
    // Before we prep the models we're going to change the material for the second model to
    // our outline material.
    if (nodes.size() < 2)
        return false;

    outlineMaterials.clear();
    outlineMaterials.push_back(data.getResource(outlineMaterialId));

    auto &outlineNode = nodes.at(1);
    outlineNode.materials = QSSGMaterialListView(outlineMaterials);

    Q_ASSERT(renderables.isEmpty());
    const QSSGModelHelpers::RenderableFilter filter = [this](QSSGModelContext *r) { renderables.push_back(r); return true; };

    bool ret = QSSGModelHelpers::createRenderables(*ctx, nodes, *camera, filter, meshLodThreshold);

    return ret; // wasDirty
}

void OutlineRenderer::prepareRender(const QSSGRenderer &renderer, QSSGFrameData &data)
{
    if (nodes.size() < 1)
        return;

    if (const auto &rhiCtx = renderer.contextInterface()->rhiContext()) {
        const auto basePs = data.getPipelineState();
        QRhiRenderPassDescriptor *rpDesc = rhiCtx->mainRenderPassDescriptor();
        const int samples = rhiCtx->mainPassSampleCount();

        { // original
            auto ps = basePs;
            ps.blendEnable = true;
            ps.depthWriteEnable = true;
            ps.usesStencilRef = true;
            ps.depthTestEnable = true;
            ps.stencilWriteMask = 0xff;
            ps.stencilRef = 1;
            ps.cullMode = QRhiGraphicsPipeline::Back;

            ps.stencilOpFrontState = { QRhiGraphicsPipeline::Keep,
                                       QRhiGraphicsPipeline::Keep,
                                       QRhiGraphicsPipeline::Replace,
                                       QRhiGraphicsPipeline::Always };

            const auto &model = renderables.at(0);
            for (auto &renderable : model->subsets)
                QSSGRenderHelpers::rhiPrepareRenderable(*rhiCtx, this, data, renderable, rpDesc, &ps, samples);
            pipelineStates[0] = ps;
        }

        { // Scaled and cut-out
            auto ps = basePs;
            ps.blendEnable = true;
            ps.depthWriteEnable = false;
            ps.depthTestEnable = true;
            ps.usesStencilRef = true;
            ps.stencilWriteMask = 0;
            ps.stencilRef = 1;
            ps.cullMode = QRhiGraphicsPipeline::Back;

            ps.stencilOpFrontState = { QRhiGraphicsPipeline::Keep,
                                       QRhiGraphicsPipeline::Keep,
                                       QRhiGraphicsPipeline::Replace,
                                       QRhiGraphicsPipeline::NotEqual };

            const auto &model = renderables.at(1);
            for (auto &renderable : model->subsets) {
                // We're going to change the scale before calling rhiPrepareRenderable()
                auto &o = static_cast<QSSGSubsetRenderable &>(renderable);
                const auto mvp { o.modelContext.modelViewProjection };
                const auto &modelCtx = o.modelContext;
                auto &mutableModelCtx = const_cast<QSSGModelContext &>(modelCtx);
                mutableModelCtx.modelViewProjection.scale(outlineScale);
                QSSGRenderHelpers::rhiPrepareRenderable(*rhiCtx, this, data, renderable, rpDesc, &ps, samples);
                // Restore.
                mutableModelCtx.modelViewProjection = mvp;
            }
            pipelineStates[1] = ps;
        }
    }
}

void OutlineRenderer::render(const QSSGRenderer &renderer)
{
    if (nodes.size() < 1)
        return;

    if (const auto &rhiCtx = renderer.contextInterface()->rhiContext()) {

        QRhiCommandBuffer *cb = rhiCtx->commandBuffer();
        cb->debugMarkBegin(QByteArrayLiteral("Stencil outline pass"));
        bool needsSetViewport = false;
        {
            const auto &model = renderables.at(0);
            for (auto &ro : model->subsets)
                QSSGRenderHelpers::rhiRenderRenderable(*rhiCtx, pipelineStates[0], ro, &needsSetViewport);
        }

        {
            const auto &model = renderables.at(1);
            for (auto &ro : model->subsets)
                QSSGRenderHelpers::rhiRenderRenderable(*rhiCtx, pipelineStates[1], ro, &needsSetViewport);
        }

        cb->debugMarkEnd();
    }
}

void OutlineRenderer::release()
{
    nodes.clear();
    renderables.clear();
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
        renderer->targetId = QQuick3DExtensionHelpers::getNodeId(*m_target);
    if (m_outlineMaterial)
        renderer->outlineMaterialId = QQuick3DExtensionHelpers::getResourceId(*m_outlineMaterial);

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
