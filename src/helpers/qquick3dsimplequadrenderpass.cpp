// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qquick3dsimplequadrenderpass_p.h"

#include <QtQuick3D/private/qquick3dobject_p.h>

#include <QtQuick3DRuntimeRender/private/qssglayerrenderdata_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrhiquadrenderer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercache_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendererimplshaders_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercommands_p.h>

#include <ssg/qssgrendercontextcore.h>
#include <ssg/qssgrenderextensions.h>

#include <rhi/qrhi.h>

QT_BEGIN_NAMESPACE

class QSSGRenderSimpleQuadRenderer : public QSSGRenderExtension
{
    static constexpr FlagT flags = FlagT(Flags::HasGraphicsResources) | FlagT(QSSGRenderGraphObjectUtils::InternalFlags::AutoRegisterExtension);
public:
    QSSGRenderSimpleQuadRenderer()
        : QSSGRenderExtension(QSSGRenderGraphObject::Type::RenderExtension, flags)
    {
    }

    virtual bool prepareData(QSSGFrameData &data) final
    {
        Q_UNUSED(data);
        bool wasDirty = true;

        wasDirty = (sourceTexture != nullptr && sourceTexture->texture());

        return wasDirty;
    }

    virtual void prepareRender(QSSGFrameData &data) final
    {
        Q_UNUSED(data);
    }

    virtual void render(QSSGFrameData &data) final
    {
        if (sourceTexture && sourceTexture->texture()) {
            const auto &rhiCtx = data.contextInterface()->rhiContext();
            const auto &renderer = data.contextInterface()->renderer();
            const auto &shaderCache = data.contextInterface()->shaderCache();

            QSSGRhiGraphicsPipelineState ps = data.getPipelineState();
            QRhiCommandBuffer *cb = rhiCtx->commandBuffer();
            cb->debugMarkBegin(QByteArrayLiteral("QtQuick3D Blit Pass"));

            const auto &shaderPipeline = shaderCache->getBuiltInRhiShaders().getRhiSimpleQuadShader(ps.viewCount);

            QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                                     QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge });
            QSSGRhiShaderResourceBindingList bindings;
            bindings.addTexture(0, QRhiShaderResourceBinding::FragmentStage, sourceTexture->texture().get(), sampler);
            QSSGRhiContextPrivate *rhiCtxD = QSSGRhiContextPrivate::get(rhiCtx.get());
            QRhiShaderResourceBindings *srb = rhiCtxD->srb(bindings);

            QSSGRhiGraphicsPipelineStatePrivate::setShaderPipeline(ps, shaderPipeline.get());
            renderer->rhiQuadRenderer()->recordRenderQuad(rhiCtx.get(), &ps, srb, rhiCtx->mainRenderPassDescriptor(), QSSGRhiQuadRenderer::UvCoords | QSSGRhiQuadRenderer::PremulBlend);
            cb->debugMarkEnd();
        }
    }
    virtual void resetForFrame() final {}
    virtual RenderMode mode() const final { return RenderMode::Main; }
    virtual RenderStage stage() const final { return RenderStage::PostColor; }

    QSSGSharedRhiTextureWrapperPtr sourceTexture;
    QRhiShaderResourceBindings *srb = nullptr;
    QSSGRhiShaderPipelinePtr quadShaderPipeline;
};


QQuick3DSimpleQuadRenderer::QQuick3DSimpleQuadRenderer() { }

QQuick3DShaderUtilsRenderPassTexture *QQuick3DSimpleQuadRenderer::texture() const
{
    return m_sourcePassTexture;
}

void QQuick3DSimpleQuadRenderer::setTexture(QQuick3DShaderUtilsRenderPassTexture *newTexture)
{
    if (m_sourcePassTexture == newTexture)
        return;

    QQuick3DObjectPrivate::attachWatcher(this, &QQuick3DSimpleQuadRenderer::setTexture, newTexture, m_sourcePassTexture);

    m_sourcePassTexture = newTexture;
    emit textureChanged();

    update();
}

QSSGRenderGraphObject *QQuick3DSimpleQuadRenderer::updateSpatialNode(QSSGRenderGraphObject *node)
{
    if (!node)
        node = new QSSGRenderSimpleQuadRenderer();

    QSSGRenderSimpleQuadRenderer *blitPass = static_cast<QSSGRenderSimpleQuadRenderer *>(node);
    if (m_sourcePassTexture && m_sourcePassTexture->command) {
        blitPass->sourceTexture = m_sourcePassTexture->command->texture();
    } else {
        blitPass->sourceTexture = {};
    }

    // TODO/FIXME: We need to get notified when the source texture gets updated/content changes.
    QMetaObject::invokeMethod(this, &QQuick3DObject::update, Qt::QueuedConnection);

    return node;
}

QT_END_NAMESPACE
