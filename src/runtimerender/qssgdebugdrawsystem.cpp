// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qssgdebugdrawsystem_p.h"

QT_BEGIN_NAMESPACE

QSSGDebugDrawSystem::QSSGDebugDrawSystem()
{

}

QSSGDebugDrawSystem::~QSSGDebugDrawSystem()
{

}

bool QSSGDebugDrawSystem::hasContent() const
{
    return !m_lines.isEmpty() || !m_persistentLines.isEmpty() || !m_bounds.isEmpty() || !m_persistentBounds.isEmpty() || !m_persistentPoints.isEmpty() || !m_points.isEmpty();
}

void QSSGDebugDrawSystem::drawLine(const QVector3D &startPoint,
                                   const QVector3D &endPoint,
                                   const QColor &color,
                                   bool isPersistent)
{
    LineData line = {startPoint, endPoint, color};
    if (isPersistent)
        m_persistentLines.append(line);
    else
        m_lines.append(line);
}

void QSSGDebugDrawSystem::drawBounds(const QSSGBounds3 &bounds,
                                     const QColor &color,
                                     bool isPersistent)
{
    BoundsData bound = {bounds, color};
    if (isPersistent)
        m_persistentBounds.append(bound);
    else
        m_bounds.append(bound);
}

void QSSGDebugDrawSystem::drawPoint(const QVector3D &vertex, const QColor &color, bool isPersistent)
{
    VertexData point = {vertex, {color.redF(), color.greenF(), color.blueF()}};
    if (isPersistent)
        m_persistentPoints.append(point);
    else
        m_points.append(point);
}


void QSSGDebugDrawSystem::prepareGeometry(QSSGRhiContext *rhiCtx, QRhiResourceUpdateBatch *rub)
{

    QVector<VertexData> vertexData;
    QVector<quint32> indexData;
    QVector<VertexData> pointsData;
    for (const auto &line : m_persistentLines)
        generateLine(line, vertexData, indexData);
    for (const auto &line : m_lines)
        generateLine(line, vertexData, indexData);
    for (const auto &bounds : m_persistentBounds)
        generateBox(bounds, vertexData, indexData);
    for (const auto &bounds : m_bounds)
        generateBox(bounds, vertexData, indexData);
    pointsData = m_persistentPoints + m_points;

    if (!vertexData.isEmpty()) {
        // Lines
        QByteArray vertexBufferData(reinterpret_cast<const char*>(vertexData.constData()), qsizetype(vertexData.count() * 6 * sizeof(float)));
        QByteArray indexBufferData(reinterpret_cast<const char*>(indexData.constData()), qsizetype(indexData.count() * sizeof(quint32)));

        if (m_lineVertexBuffer)
            m_lineVertexBuffer.reset();
        if (m_lineIndexBuffer)
            m_lineIndexBuffer.reset();

        m_lineVertexBuffer = std::make_shared<QSSGRhiBuffer>(*rhiCtx,
                                                             QRhiBuffer::Immutable,
                                                             QRhiBuffer::VertexBuffer,
                                                             quint32(6 * sizeof(float)),
                                                             6 * sizeof(float) * vertexData.count());
        m_lineVertexBuffer->buffer()->setName(QByteArrayLiteral("debug lines vertex buffer"));
        rub->uploadStaticBuffer(m_lineVertexBuffer->buffer(), vertexBufferData.constData());

        m_lineIndexBuffer = std::make_shared<QSSGRhiBuffer>(*rhiCtx,
                                                            QRhiBuffer::Immutable,
                                                            QRhiBuffer::IndexBuffer,
                                                            0,
                                                            indexBufferData.size(),
                                                            QRhiCommandBuffer::IndexUInt32);
        m_lineIndexBuffer->buffer()->setName(QByteArrayLiteral("debug lines index buffer"));
        rub->uploadStaticBuffer(m_lineIndexBuffer->buffer(), indexBufferData.constData());

        m_indexSize = indexData.count();
    }

    if (!pointsData.isEmpty()) {
        // Points
        QByteArray vertexBufferData(reinterpret_cast<const char*>(pointsData.constData()), qsizetype(pointsData.count() * 6 * sizeof(float)));

        if (m_pointVertexBuffer)
            m_pointVertexBuffer.reset();

        m_pointVertexBuffer = std::make_shared<QSSGRhiBuffer>(*rhiCtx,
                                                              QRhiBuffer::Immutable,
                                                              QRhiBuffer::VertexBuffer,
                                                              quint32(6 * sizeof(float)),
                                                              vertexBufferData.size());
        m_pointVertexBuffer->buffer()->setName(QByteArrayLiteral("debug points vertex buffer"));
        rub->uploadStaticBuffer(m_pointVertexBuffer->buffer(), vertexBufferData.constData());
        m_pointsSize = pointsData.count();
    }
}

void QSSGDebugDrawSystem::recordRenderDebugObjects(QSSGRhiContext *rhiCtx,
                                                   QSSGRhiGraphicsPipelineState *ps,
                                                   QRhiShaderResourceBindings *srb,
                                                   QRhiRenderPassDescriptor *rpDesc)
{
    ps->ia.inputLayout.setAttributes({
                                         { 0, 0, QRhiVertexInputAttribute::Float3, 0 },
                                         { 0, 1, QRhiVertexInputAttribute::Float3, 3 * sizeof(float) }
                                     });
    ps->ia.inputs << QSSGRhiInputAssemblerState::PositionSemantic
                  << QSSGRhiInputAssemblerState::ColorSemantic;
    ps->ia.inputLayout.setBindings({6 * sizeof(float)});
    ps->ia.topology = QRhiGraphicsPipeline::Lines;
    ps->depthWriteEnable = true;
    ps->depthTestEnable = true;
    ps->cullMode = QRhiGraphicsPipeline::None;

    QRhiCommandBuffer *cb = rhiCtx->commandBuffer();
    if (m_indexSize > 0) {
        auto graphicsPipeline = rhiCtx->pipeline(QSSGGraphicsPipelineStateKey::create(*ps, rpDesc, srb), rpDesc, srb);
        cb->setGraphicsPipeline(graphicsPipeline);
        cb->setShaderResources(srb);
        cb->setViewport(ps->viewport);

        // Lines
        QRhiCommandBuffer::VertexInput vb(m_lineVertexBuffer->buffer(), 0);
        cb->setVertexInput(0, 1, &vb, m_lineIndexBuffer->buffer(), 0, m_lineIndexBuffer->indexFormat());
        cb->drawIndexed(m_indexSize);
    }

    // Points
    if (m_pointsSize > 0) {
        ps->ia.topology = QRhiGraphicsPipeline::Points;
        auto graphicsPipeline = rhiCtx->pipeline(QSSGGraphicsPipelineStateKey::create(*ps, rpDesc, srb), rpDesc, srb);
        cb->setGraphicsPipeline(graphicsPipeline);
        cb->setShaderResources(srb);
        cb->setViewport(ps->viewport);

        QRhiCommandBuffer::VertexInput vb(m_pointVertexBuffer->buffer(), 0);
        cb->setVertexInput(0, 1, &vb);
        cb->draw(m_pointsSize);
    }

    m_lines.clear();
    m_bounds.clear();
    m_points.clear();
    m_indexSize = 0;
    m_pointsSize = 0;
}

void QSSGDebugDrawSystem::generateLine(const LineData &line, QVector<VertexData> &vertexArray, QVector<quint32> &indexArray)
{
    const QVector3D color = {line.color.redF(), line.color.greenF(), line.color.blueF()};
    indexArray.append(vertexArray.count());
    vertexArray.append({line.startPoint, color});
    indexArray.append(vertexArray.count());
    vertexArray.append({line.endPoint, color});
}

void QSSGDebugDrawSystem::generateBox(const BoundsData &bounds, QVector<VertexData> &vertexArray, QVector<quint32> &indexArray)
{
    const QVector3D color = {bounds.color.redF(), bounds.color.greenF(), bounds.color.blueF()};

    quint32 offset = vertexArray.count();
    for (const QVector3D point : bounds.bounds.toQSSGBoxPoints())
        vertexArray.append({point, color});

    indexArray.append(offset + 0);
    indexArray.append(offset + 3);

    indexArray.append(offset + 3);
    indexArray.append(offset + 6);

    indexArray.append(offset + 6);
    indexArray.append(offset + 1);

    indexArray.append(offset + 1);
    indexArray.append(offset + 0);

    indexArray.append(offset + 2);
    indexArray.append(offset + 5);

    indexArray.append(offset + 5);
    indexArray.append(offset + 4);

    indexArray.append(offset + 4);
    indexArray.append(offset + 7);

    indexArray.append(offset + 7);
    indexArray.append(offset + 2);

    indexArray.append(offset + 0);
    indexArray.append(offset + 2);

    indexArray.append(offset + 3);
    indexArray.append(offset + 5);

    indexArray.append(offset + 6);
    indexArray.append(offset + 4);

    indexArray.append(offset + 1);
    indexArray.append(offset + 7);

}

QT_END_NAMESPACE
