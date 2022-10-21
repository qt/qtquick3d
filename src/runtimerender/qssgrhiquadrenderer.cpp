/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
** Copyright (C) 2020 The Qt Company Ltd.
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

#include <QtQuick3DRuntimeRender/private/qssgrhiquadrenderer_p.h>

QT_BEGIN_NAMESPACE

static const QVector3D g_fullScreenRectFace[] = {
    QVector3D(-1, -1, 0),
    QVector3D(-1, 1, 0),
    QVector3D(1, 1, 0),
    QVector3D(1, -1, 0),
};

static const QVector2D g_fullScreenRectUVs[] = {
    QVector2D(0, 0),
    QVector2D(0, 1),
    QVector2D(1, 1),
    QVector2D(1, 0)
};

void QSSGRhiQuadRenderer::ensureBuffers(QSSGRhiContext *rhiCtx, QRhiResourceUpdateBatch *rub)
{
    if (!m_vbuf) {
        m_vbuf = new QSSGRhiBuffer(*rhiCtx,
                                   QRhiBuffer::Immutable,
                                   QRhiBuffer::VertexBuffer,
                                   5 * sizeof(float),
                                   5 * 4 * sizeof(float));
        m_vbuf->buffer()->setName(QByteArrayLiteral("quad vertex buffer"));
        float buf[20];
        float *p = buf;
        for (int i = 0; i < 4; ++i) {
            *p++ = g_fullScreenRectFace[i].x();
            *p++ = g_fullScreenRectFace[i].y();
            *p++ = g_fullScreenRectFace[i].z();
            *p++ = g_fullScreenRectUVs[i].x();
            *p++ = g_fullScreenRectUVs[i].y();
        }
        rub->uploadStaticBuffer(m_vbuf->buffer(), buf);
    }
    if (!m_ibuf) {
        m_ibuf = new QSSGRhiBuffer(*rhiCtx,
                                   QRhiBuffer::Immutable,
                                   QRhiBuffer::IndexBuffer,
                                   0,
                                   6 * sizeof(quint16),
                                   QRhiCommandBuffer::IndexUInt16);
        m_ibuf->buffer()->setName(QByteArrayLiteral("quad index buffer"));
        const quint16 buf[] = { 0, 1, 2, 0, 2, 3 };
        rub->uploadStaticBuffer(m_ibuf->buffer(), buf);
    }
}

void QSSGRhiQuadRenderer::prepareQuad(QSSGRhiContext *rhiCtx, QRhiResourceUpdateBatch *maybeRub)
{
    QRhiResourceUpdateBatch *rub = maybeRub ? maybeRub : rhiCtx->rhi()->nextResourceUpdateBatch();
    ensureBuffers(rhiCtx, rub);
    rhiCtx->commandBuffer()->resourceUpdate(rub);
}

void QSSGRhiQuadRenderer::recordRenderQuad(QSSGRhiContext *rhiCtx,
                                           QSSGRhiGraphicsPipelineState *ps, QRhiShaderResourceBindings *srb,
                                           QRhiRenderPassDescriptor *rpDesc, Flags flags)
{
    // ps must have viewport and shaderPipeline set already
    if (flags.testFlag(UvCoords)) {
        ps->ia.inputLayout.setAttributes({
                                            { 0, 0, QRhiVertexInputAttribute::Float3, 0 },
                                            { 0, 1, QRhiVertexInputAttribute::Float2, 3 * sizeof(float) }
                                        });
        ps->ia.inputs << QSSGRhiInputAssemblerState::PositionSemantic << QSSGRhiInputAssemblerState::TexCoord0Semantic;
    } else {
        ps->ia.inputLayout.setAttributes({ { 0, 0, QRhiVertexInputAttribute::Float3, 0 } });
        ps->ia.inputs << QSSGRhiInputAssemblerState::PositionSemantic;
    }
    ps->ia.inputLayout.setBindings({ 5 * sizeof(float) });
    ps->ia.topology = QRhiGraphicsPipeline::Triangles;

    ps->depthTestEnable = flags.testFlag(DepthTest);
    ps->depthWriteEnable = flags.testFlag(DepthWrite);
    ps->cullMode = QRhiGraphicsPipeline::None;
    if (flags.testFlag(PremulBlend)) {
        ps->blendEnable = true;
        ps->targetBlend.srcColor = QRhiGraphicsPipeline::One;
        ps->targetBlend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
        ps->targetBlend.srcAlpha = QRhiGraphicsPipeline::One;
        ps->targetBlend.dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    }

    QRhiCommandBuffer *cb = rhiCtx->commandBuffer();
    cb->setGraphicsPipeline(rhiCtx->pipeline(QSSGGraphicsPipelineStateKey::create(*ps, rpDesc, srb), rpDesc, srb));
    cb->setShaderResources(srb);
    cb->setViewport(ps->viewport);
    QRhiCommandBuffer::VertexInput vb(m_vbuf->buffer(), 0);
    cb->setVertexInput(0, 1, &vb, m_ibuf->buffer(), m_ibuf->indexFormat());
    cb->drawIndexed(6);
    QSSGRHICTX_STAT(rhiCtx, drawIndexed(6, 1));
}

void QSSGRhiQuadRenderer::recordRenderQuadPass(QSSGRhiContext *rhiCtx,
                                               QSSGRhiGraphicsPipelineState *ps, QRhiShaderResourceBindings *srb,
                                               QRhiTextureRenderTarget *rt, Flags flags)
{
    QRhiCommandBuffer *cb = rhiCtx->commandBuffer();
    cb->beginPass(rt, Qt::black, { 1.0f, 0 }, nullptr, QSSGRhiContext::commonPassFlags());
    QSSGRHICTX_STAT(rhiCtx, beginRenderPass(rt));
    recordRenderQuad(rhiCtx, ps, srb, rt->renderPassDescriptor(), flags);
    cb->endPass();
    QSSGRHICTX_STAT(rhiCtx, endRenderPass());
}

QT_END_NAMESPACE
