/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "qssgrhicontext_p.h"

QT_BEGIN_NAMESPACE

QSSGRhiBuffer::QSSGRhiBuffer(const QSSGRef<QSSGRhiContext> &context,
                             QRhiBuffer::Type type,
                             QRhiBuffer::UsageFlags usageMask,
                             quint32 stride,
                             int size,
                             QRhiCommandBuffer::IndexFormat indexFormat)
    : m_context(context),
      m_stride(stride),
      m_indexFormat(indexFormat)
{
    m_buffer = m_context->rhi()->newBuffer(type, usageMask, size);
    if (!m_buffer->build())
        qWarning("Failed to build QRhiBuffer with size %d", m_buffer->size());
}

QSSGRhiBuffer::~QSSGRhiBuffer()
{
    delete m_buffer;
}

QRhiVertexInputAttribute::Format QSSGRhiInputAssemblerState::toVertexInputFormat(QSSGRenderComponentType compType, quint32 numComps)
{
    if (compType == QSSGRenderComponentType::Float32) {
        switch (numComps) {
        case 1:
            return QRhiVertexInputAttribute::Float;
        case 2:
            return QRhiVertexInputAttribute::Float2;
        case 3:
            return QRhiVertexInputAttribute::Float3;
        case 4:
            return QRhiVertexInputAttribute::Float4;
        default:
            break;
        }
    }
    Q_ASSERT(false);
    return QRhiVertexInputAttribute::Float4;
}

QRhiGraphicsPipeline::Topology QSSGRhiInputAssemblerState::toTopology(QSSGRenderDrawMode drawMode)
{
    switch (drawMode) {
    case QSSGRenderDrawMode::Points:
        return QRhiGraphicsPipeline::Points;
    case QSSGRenderDrawMode::LineStrip:
        return QRhiGraphicsPipeline::LineStrip;
    case QSSGRenderDrawMode::Lines:
        return QRhiGraphicsPipeline::Lines;
    case QSSGRenderDrawMode::TriangleStrip:
        return QRhiGraphicsPipeline::TriangleStrip;
    case QSSGRenderDrawMode::Triangles:
        return QRhiGraphicsPipeline::Triangles;
    default:
        break;
    }
    Q_ASSERT(false);
    return QRhiGraphicsPipeline::Triangles;
}

void QSSGRhiInputAssemblerState::bakeVertexInputLocations(const QSSGRhiShaderStagesWithResources &shaders)
{
    if (lastBakeVertexInputKey == &shaders)
        return;

    const QRhiShaderStage *vertexStage = shaders.stages()->vertexStage();
    if (!vertexStage)
        return;

    const QShaderDescription shaderDesc = vertexStage->shader().description();

    QHash<QByteArray, int> locationMap;
    for (const QShaderDescription::InOutVariable &var : shaderDesc.inputVariables())
        locationMap.insert(var.name.toLatin1(), var.location);

    QVarLengthArray<QRhiVertexInputAttribute, 4> attrs;
    int inputIndex = 0;
    for (auto it = inputLayout.cbeginAttributes(), itEnd = inputLayout.cendAttributes(); it != itEnd; ++it) {
        auto locIt = locationMap.constFind(inputLayoutInputNames[inputIndex]);
        if (locIt != locationMap.constEnd()) {
            attrs.append(*it);
            attrs.last().setLocation(locIt.value());
        }
        ++inputIndex;
    }
    inputLayout.setAttributes(attrs.cbegin(), attrs.cend());

    lastBakeVertexInputKey = &shaders;
}

QSSGRhiShaderStages::QSSGRhiShaderStages(const QSSGRef<QSSGRhiContext> &context)
    : m_context(context)
{
}

QRhiGraphicsPipeline::CullMode QSSGRhiGraphicsPipelineState::toCullMode(QSSGCullFaceMode cullFaceMode)
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
    default:
        break;
    }
    Q_ASSERT(false);
    return QRhiGraphicsPipeline::None;
}

bool operator==(const QSSGRhiGraphicsPipelineState &a, const QSSGRhiGraphicsPipelineState &b) Q_DECL_NOTHROW
{
    return a.shaderStages == b.shaderStages
            && a.depthTestEnable == b.depthTestEnable
            && a.depthWriteEnable == b.depthWriteEnable
            && a.depthFunc == b.depthFunc
            && a.cullMode == b.cullMode
            && a.blendEnable == b.blendEnable
            && a.scissorEnable == b.scissorEnable
            && a.viewport == b.viewport
            && a.scissor == b.scissor
            && a.ia.topology == b.ia.topology
            && a.ia.inputLayout == b.ia.inputLayout;
}

bool operator!=(const QSSGRhiGraphicsPipelineState &a, const QSSGRhiGraphicsPipelineState &b) Q_DECL_NOTHROW
{
    return !(a == b);
}

uint qHash(const QSSGRhiGraphicsPipelineState &s, uint seed) Q_DECL_NOTHROW
{
    // do not bother with all fields
    return seed
            + qHash(s.shaderStages, seed)
            + s.depthTestEnable * 1000
            + s.depthWriteEnable * 100
            + s.blendEnable * 10
            + s.depthFunc
            + s.cullMode
            + s.scissorEnable;
}

bool operator==(const QSSGGraphicsPipelineStateKey &a, const QSSGGraphicsPipelineStateKey &b) Q_DECL_NOTHROW
{
    return a.state == b.state
            && a.compatibleRpDesc->isCompatible(b.compatibleRpDesc)
            && a.layoutCompatibleSrb->isLayoutCompatible(b.layoutCompatibleSrb);
}

bool operator!=(const QSSGGraphicsPipelineStateKey &a, const QSSGGraphicsPipelineStateKey &b) Q_DECL_NOTHROW
{
    return !(a == b);
}

uint qHash(const QSSGGraphicsPipelineStateKey &k, uint seed) Q_DECL_NOTHROW
{
    return qHash(k.state, seed); // rp and srb not included, intentionally (see ==, those are based on compatibility, not pointer equivalence)
}

QSSGRef<QSSGRhiShaderStagesWithResources> QSSGRhiShaderStagesWithResources::fromShaderStages(const QSSGRef<QSSGRhiShaderStages> &stages,
                                                                                             const QByteArray &shaderKeyString)
{
    return QSSGRef<QSSGRhiShaderStagesWithResources>(new QSSGRhiShaderStagesWithResources(stages, shaderKeyString));
}

void QSSGRhiShaderStagesWithResources::setUniform(const QByteArray &name, const void *data, size_t size)
{
    auto it = m_uniforms.find(name);
    if (it != m_uniforms.end()) {
        if (size <= it.value().size) {
            it.value().dirty = true;
            memcpy(it.value().data, data, size);
        } else {
            qWarning("Attempted to set %u bytes to uniform %s with size %u",
                     uint(size), name.constData(), uint(it.value().size));
        }
    } else {
        QSSGRhiShaderUniform u;
        Q_ASSERT(size <= sizeof(u.data));
        u.name = QString::fromLatin1(name);
        u.size = size;
        memcpy(u.data, data, size);
        m_uniforms.insert(name, u);
    }
}

void QSSGRhiShaderStagesWithResources::dumpUniforms()
{
    for (const QSSGRhiShaderUniform &u : m_uniforms) {
        qDebug() << u.name << u.size << u.dirty << QByteArray(u.data, int(u.size));
    }
}

void QSSGRhiShaderStagesWithResources::bakeMainUniformBuffer(QRhiBuffer **ubuf, QRhiResourceUpdateBatch *resourceUpdates)
{
    /* pointless to look for dirty flags, they are always true for now
    bool hasDirty = false;
    for (const QSSGRhiShaderUniform &u : uniforms) {
        if (u.dirty) {
            hasDirty = true;
            break;
        }
    }
    if (!hasDirty)
        return;
        */

    // We will assume that the main uniform buffer has the same layout in all
    // stages (the generator should ensure that), meaning it includes all
    // members in all shaders, even if a member is not used in that particular
    // shader.
    const QRhiShaderStage *vertexStage = m_shaderStages->vertexStage();
    if (!vertexStage)
        return;

    const QShaderDescription shaderDesc = vertexStage->shader().description();
    const QVector<QShaderDescription::UniformBlock> uniformBlocks = shaderDesc.uniformBlocks();
    if (uniformBlocks.isEmpty())
        return;

    for (const QShaderDescription::UniformBlock &blk : uniformBlocks) {
        if (blk.binding == 0) {
            if (!resourceUpdates)
                resourceUpdates = m_context->rhi()->nextResourceUpdateBatch();

            const int size = blk.size;
            QVarLengthArray<char, 512> bufferData; // not ideal but will do for now
            bufferData.resize(size);

            if (!*ubuf) {
                *ubuf = m_context->rhi()->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, size);
                (*ubuf)->build();
            }
            if ((*ubuf)->size() < size) {
                (*ubuf)->setSize(size);
                (*ubuf)->build();
            }

            for (QSSGRhiShaderUniform &u : m_uniforms) {
                if (u.offset == SIZE_MAX) {
                    for (const QShaderDescription::BlockVariable &var : blk.members) {
                        if (var.name == u.name) {
                            u.offset = var.offset;
                            break;
                        }
                    }
                }
                if (u.offset == SIZE_MAX) // must silently ignore uniforms that are not in the actual shader
                    continue;

                memcpy(bufferData.data() + u.offset, u.data, u.size);
            }

            resourceUpdates->updateDynamicBuffer(*ubuf, 0, size, bufferData.constData());

            break;
        }
    }
}

void QSSGRhiShaderStagesWithResources::bakeLightsUniformBuffer(QRhiBuffer **ubuf, QRhiResourceUpdateBatch *resourceUpdates)
{
    Q_ASSERT(m_lightsEnabled);

    const int size = int(sizeof(QSSGLightSourceShader) * QSSG_MAX_NUM_LIGHTS + (4 * sizeof(qint32)));

    if (!*ubuf) {
        *ubuf = m_context->rhi()->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, size);
        (*ubuf)->build();
    }
    if ((*ubuf)->size() < size) {
        (*ubuf)->setSize(size);
        (*ubuf)->build();
    }

    const qint32 count = m_lights.count();
    resourceUpdates->updateDynamicBuffer(*ubuf, 0, sizeof(qint32), &count);

    for (int idx = 0; idx < m_lights.count(); ++idx) {
        const int offset = idx * sizeof(QSSGLightSourceShader) + (4 * sizeof(qint32));
        resourceUpdates->updateDynamicBuffer(*ubuf, offset, sizeof(QSSGLightSourceShader), &m_lights[idx].lightData);
    }
}

QSSGRhiContext::QSSGRhiContext()
{
}

QSSGRhiContext::~QSSGRhiContext()
{
    qDeleteAll(m_ownBuffers);
    qDeleteAll(m_pipelines);
    qDeleteAll(m_srbCache);
}

void QSSGRhiContext::setRhi(QRhi *rhi)
{
    Q_ASSERT(rhi && !m_rhi);
    m_rhi = rhi;
}

QRhiShaderResourceBindings *QSSGRhiContext::srb(const ShaderResourceBindingList &bindings)
{
    auto it = m_srbCache.constFind(bindings);
    if (it != m_srbCache.constEnd())
        return *it;

    QRhiShaderResourceBindings *srb = m_rhi->newShaderResourceBindings();
    srb->setBindings(bindings.cbegin(), bindings.cend());
    if (srb->build()) {
        m_srbCache.insert(bindings, srb);
    } else {
        qWarning("Failed to build srb");
        delete srb;
        srb = nullptr;
    }
    return srb;
}

QRhiGraphicsPipeline *QSSGRhiContext::pipeline(const QSSGGraphicsPipelineStateKey &key)
{
    auto it = m_pipelines.constFind(key);
    if (it != m_pipelines.constEnd())
        return it.value();

    // Build a new one. This is potentially expensive.
    QRhiGraphicsPipeline *ps = m_rhi->newGraphicsPipeline();

    const QVector<QRhiShaderStage> &stages(key.state.shaderStages->stages());
    ps->setShaderStages(stages.cbegin(), stages.cend());
    ps->setVertexInputLayout(key.state.ia.inputLayout);
    ps->setShaderResourceBindings(key.layoutCompatibleSrb);
    ps->setRenderPassDescriptor(key.compatibleRpDesc);

    QRhiGraphicsPipeline::Flags flags; // ### QRhiGraphicsPipeline::UsesScissor -> we will need setScissor once this flag is set
    ps->setFlags(flags);

    ps->setTopology(key.state.ia.topology);
    ps->setCullMode(key.state.cullMode);

    QRhiGraphicsPipeline::TargetBlend blend;
    blend.enable = key.state.blendEnable;
//    blend.srcColor = m_gstate.srcColor;
//    blend.dstColor = m_gstate.dstColor;
    ps->setTargetBlends({ blend });

    ps->setDepthTest(key.state.depthTestEnable);
    ps->setDepthWrite(key.state.depthWriteEnable);
    ps->setDepthOp(key.state.depthFunc);

    if (!ps->build()) {
        qWarning("Failed to build graphics pipeline state");
        delete ps;
        return nullptr;
    }

    m_pipelines.insert(key, ps);
    return ps;
}

QT_END_NAMESPACE
