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

QSSGRhiQuadRenderer::QSSGRhiQuadRenderer(const QSSGRef<QSSGRhiContext> &rhiCtx)
    : m_rhiContext(rhiCtx)
{
}

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
        m_vbuf = new QSSGRhiBuffer(rhiCtx,
                                   QRhiBuffer::Immutable,
                                   QRhiBuffer::VertexBuffer,
                                   5 * sizeof(float),
                                   5 * 4 * sizeof(float));
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
        m_ibuf = new QSSGRhiBuffer(rhiCtx,
                                   QRhiBuffer::Immutable,
                                   QRhiBuffer::IndexBuffer,
                                   0,
                                   6 * sizeof(float),
                                   QRhiCommandBuffer::IndexUInt16);
        const quint16 buf[] = { 0, 1, 2, 0, 2, 3 };
        rub->uploadStaticBuffer(m_ibuf->buffer(), buf);
    }
}

void QSSGRhiQuadRenderer::recordQuadRenderPass(QSSGRhiContext *rhiCtx, QRhiResourceUpdateBatch *maybeRub,
                                               QSSGRhiGraphicsPipelineState *ps, QRhiShaderResourceBindings *srb,
                                               QRhiTextureRenderTarget *rt, bool wantsUV)
{
    // ps must have viewport and shaderStages set already

    QRhi *rhi = rhiCtx->rhi();
    QRhiCommandBuffer *cb = rhiCtx->commandBuffer();
    QRhiResourceUpdateBatch *rub = maybeRub ? maybeRub : rhi->nextResourceUpdateBatch();

    ensureBuffers(rhiCtx, rub);

    ps->ia.vertexBuffer = m_vbuf;
    ps->ia.indexBuffer = m_ibuf;

    if (wantsUV) {
        ps->ia.inputLayout.setAttributes({
                                            { 0, 0, QRhiVertexInputAttribute::Float3, 0 },
                                            { 0, 1, QRhiVertexInputAttribute::Float2, 3 * sizeof(float) }
                                        });
        ps->ia.inputLayoutInputNames << QByteArrayLiteral("attr_pos") << QByteArrayLiteral("attr_uv");
    } else {
        ps->ia.inputLayout.setAttributes({ { 0, 0, QRhiVertexInputAttribute::Float3, 0 } });
        ps->ia.inputLayoutInputNames << QByteArrayLiteral("attr_pos");
    }
    ps->ia.inputLayout.setBindings({ 5 * sizeof(float) });
    ps->ia.topology = QRhiGraphicsPipeline::Triangles;

    ps->depthTestEnable = false;
    ps->depthWriteEnable = false;
    ps->cullMode = QRhiGraphicsPipeline::None;

    cb->beginPass(rt, Qt::black, { 1.0f, 0 }, rub);
    cb->setGraphicsPipeline(rhiCtx->pipeline({ *ps, rt->renderPassDescriptor(), srb }));
    cb->setShaderResources(srb);
    cb->setViewport(ps->viewport);
    QRhiCommandBuffer::VertexInput vb(ps->ia.vertexBuffer->buffer(), 0);
    cb->setVertexInput(0, 1, &vb, ps->ia.indexBuffer->buffer(), ps->ia.indexBuffer->indexFormat());
    cb->drawIndexed(6);
    cb->endPass();
}

QT_END_NAMESPACE
