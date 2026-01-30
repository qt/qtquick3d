// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default


#include "qquick3dsimplequadrenderpass_p.h"

#include <QtQuick3D/private/qquick3dobject_p.h>

#include <QtQuick3DRuntimeRender/private/qssglayerrenderdata_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrhiquadrenderer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercache_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendererimplshaders_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercommands_p.h>

#include <QtQuick3DRuntimeRender/private/qssgrenderbuffermanager_p.h>

#include <ssg/qssgrendercontextcore.h>
#include <ssg/qssgrenderextensions.h>

#include <rhi/qrhi.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype SimpleQuadRenderer
    \inqmlmodule QtQuick3D.Helpers
    \inherits RenderExtension
    \brief The SimpleQuadRenderer class renders a full-screen quad with a specified texture.

    The SimpleQuadRenderer is a convenient way to render a texture across the entire screen.

    \qml
    import QtQuick3D.Helpers

    SimpleQuadRenderer {
        texture: Texture {
            source: "myImage.png"
        }
    }
    \endqml

    \sa RenderPassTexture
*/

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
        bool wasDirty = true;

        if (image)
            renderImage = data.contextInterface()->bufferManager()->loadRenderImage(image);
        else
            renderImage = QSSGRenderImageTexture();

        wasDirty = (renderImage.m_texture != nullptr);

        return wasDirty;
    }

    virtual void prepareRender(QSSGFrameData &data) final
    {
        if (renderImage.m_texture) {
            const auto &rhiCtx = data.contextInterface()->rhiContext();
            const auto &renderer = data.contextInterface()->renderer();
            QRhiResourceUpdateBatch *rub = rhiCtx->rhi()->nextResourceUpdateBatch();
            renderer->rhiQuadRenderer()->prepareQuad(rhiCtx.get(), rub);
        }
    }

    virtual void render(QSSGFrameData &data) final
    {
        if (renderImage.m_texture) {
            const auto &rhiCtx = data.contextInterface()->rhiContext();
            const auto &renderer = data.contextInterface()->renderer();
            const auto &shaderCache = data.contextInterface()->shaderCache();

            QSSGRhiGraphicsPipelineState ps = data.getPipelineState();
            QRhiCommandBuffer *cb = rhiCtx->commandBuffer();
            cb->debugMarkBegin(QByteArrayLiteral("QtQuick3D Blit Pass"));

            const auto tonemapMode = QSSGLayerRenderData::getCurrent(data)->layer.tonemapMode;
            const auto &shaderPipeline = shaderCache->getBuiltInRhiShaders().getRhiSimpleQuadShader(ps.viewCount, tonemapMode);

            QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                                     QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge });
            QSSGRhiShaderResourceBindingList bindings;
            bindings.addTexture(0, QRhiShaderResourceBinding::FragmentStage, renderImage.m_texture, sampler);
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

    QSSGRenderImage *image = nullptr;
    QSSGRenderImageTexture renderImage;
    QRhiShaderResourceBindings *srb = nullptr;
    QSSGRhiShaderPipelinePtr quadShaderPipeline;
};


QQuick3DSimpleQuadRenderer::QQuick3DSimpleQuadRenderer() { }

QSSGRenderGraphObject *QQuick3DSimpleQuadRenderer::updateSpatialNode(QSSGRenderGraphObject *node)
{
    if (!node)
        node = new QSSGRenderSimpleQuadRenderer();

    QSSGRenderSimpleQuadRenderer *blitPass = static_cast<QSSGRenderSimpleQuadRenderer *>(node);
    if (m_source)
        blitPass->image = m_source->getRenderImage();
    else
        blitPass->image = nullptr;

    return node;
}

void QQuick3DSimpleQuadRenderer::itemChange(QQuick3DObject::ItemChange change, const QQuick3DObject::ItemChangeData &value)
{
    if (change == QQuick3DObject::ItemSceneChange)
        updateSceneManager(value.sceneManager);
}

QQuick3DTexture *QQuick3DSimpleQuadRenderer::texture() const
{
    return m_source;
}

void QQuick3DSimpleQuadRenderer::setTexture(QQuick3DTexture *newSource)
{
    if (m_source == newSource)
        return;

    QQuick3DObjectPrivate::attachWatcher(this, &QQuick3DSimpleQuadRenderer::setTexture, newSource, m_source);

    m_source = newSource;
    emit textureChanged();

    update();
}

void QQuick3DSimpleQuadRenderer::updateSceneManager(QQuick3DSceneManager *sceneManager)
{
    if (sceneManager)
        QQuick3DObjectPrivate::refSceneManager(m_source, *sceneManager);
    else
        QQuick3DObjectPrivate::derefSceneManager(m_source);
}

QT_END_NAMESPACE
