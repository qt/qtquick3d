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

QSSGRhiBuffer::QSSGRhiBuffer(QSSGRhiContext &context,
                             QRhiBuffer::Type type,
                             QRhiBuffer::UsageFlags usageMask,
                             quint32 stride,
                             int size,
                             QRhiCommandBuffer::IndexFormat indexFormat)
    : m_context(context),
      m_stride(stride),
      m_indexFormat(indexFormat)
{
    m_buffer = m_context.rhi()->newBuffer(type, usageMask, size);
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
    if (lastBakeVertexInputKey == &shaders && lastBakeVertexInputNames == inputLayoutInputNames)
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
        } // else the mesh has an input attribute that is not declared and used in the vertex shader - that's fine
        ++inputIndex;
    }
    inputLayout.setAttributes(attrs.cbegin(), attrs.cend());

    lastBakeVertexInputKey = &shaders;
    lastBakeVertexInputNames = inputLayoutInputNames;
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
            && a.samples == b.samples
            && a.depthTestEnable == b.depthTestEnable
            && a.depthWriteEnable == b.depthWriteEnable
            && a.depthFunc == b.depthFunc
            && a.cullMode == b.cullMode
            && a.depthBias == b.depthBias
            && a.slopeScaledDepthBias == b.slopeScaledDepthBias
            && a.blendEnable == b.blendEnable
            && a.scissorEnable == b.scissorEnable
            && a.viewport == b.viewport
            && a.scissor == b.scissor
            && a.ia.topology == b.ia.topology
            && a.ia.inputLayout == b.ia.inputLayout
            && a.targetBlend.colorWrite == b.targetBlend.colorWrite
            && a.targetBlend.srcColor == b.targetBlend.srcColor
            && a.targetBlend.dstColor == b.targetBlend.dstColor
            && a.targetBlend.opColor == b.targetBlend.opColor
            && a.targetBlend.srcAlpha == b.targetBlend.srcAlpha
            && a.targetBlend.dstAlpha == b.targetBlend.dstAlpha
            && a.targetBlend.opAlpha == b.targetBlend.opAlpha
            && a.colorAttachmentCount == b.colorAttachmentCount;
}

bool operator!=(const QSSGRhiGraphicsPipelineState &a, const QSSGRhiGraphicsPipelineState &b) Q_DECL_NOTHROW
{
    return !(a == b);
}

size_t qHash(const QSSGRhiGraphicsPipelineState &s, size_t seed) Q_DECL_NOTHROW
{
    // do not bother with all fields
    return qHash(s.shaderStages, seed)
            ^ qHash(s.samples)
            ^ qHash(s.targetBlend.dstColor)
            ^ qHash(s.depthFunc)
            ^ qHash(s.cullMode)
            ^ qHash(s.colorAttachmentCount)
            ^ (s.depthTestEnable << 1)
            ^ (s.depthWriteEnable << 2)
            ^ (s.blendEnable << 3)
            ^ (s.scissorEnable << 4);
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

size_t qHash(const QSSGGraphicsPipelineStateKey &k, size_t seed) Q_DECL_NOTHROW
{
    return qHash(k.state, seed); // rp and srb not included, intentionally (see ==, those are based on compatibility, not pointer equivalence)
}

QSSGRef<QSSGRhiShaderStagesWithResources> QSSGRhiShaderStagesWithResources::fromShaderStages(const QSSGRef<QSSGRhiShaderStages> &stages)
{
    return QSSGRef<QSSGRhiShaderStagesWithResources>(new QSSGRhiShaderStagesWithResources(stages));
}

int QSSGRhiShaderStagesWithResources::setUniformValue(const QByteArray &name, const QVariant &inValue, QSSGRenderShaderDataType inType)
{
    switch (inType) {
    case QSSGRenderShaderDataType::Integer:
    {
        const qint32 v = inValue.toInt();
        return setUniform(name, &v, sizeof(qint32));
    }
        break;
    case QSSGRenderShaderDataType::IntegerVec2:
    {
        const qint32_2 v = inValue.value<qint32_2>();
        return setUniform(name, &v, 2 * sizeof(qint32));
    }
        break;
    case QSSGRenderShaderDataType::IntegerVec3:
    {
        const qint32_3 v = inValue.value<qint32_3>();
        return setUniform(name, &v, 3 * sizeof(qint32));
    }
        break;
    case QSSGRenderShaderDataType::IntegerVec4:
    {
        const qint32_4 v = inValue.value<qint32_4>();
        return setUniform(name, &v, 4 * sizeof(qint32));
    }
        break;
    case QSSGRenderShaderDataType::Boolean:
    {
        // whatever bool is does not matter, what matters is that the GLSL bool is 4 bytes
        const qint32 v = inValue.value<bool>();
        return setUniform(name, &v, sizeof(qint32));
    }
        break;
    case QSSGRenderShaderDataType::BooleanVec2:
    {
        const bool_2 b = inValue.value<bool_2>();
        const qint32_2 v(b.x, b.y);
        return setUniform(name, &v, 2 * sizeof(qint32));
    }
        break;
    case QSSGRenderShaderDataType::BooleanVec3:
    {
        const bool_3 b = inValue.value<bool_3>();
        const qint32_3 v(b.x, b.y, b.z);
        return setUniform(name, &v, 3 * sizeof(qint32));
    }
        break;
    case QSSGRenderShaderDataType::BooleanVec4:
    {
        const bool_4 b = inValue.value<bool_4>();
        const qint32_4 v(b.x, b.y, b.z, b.w);
        return setUniform(name, &v, 4 * sizeof(qint32));
    }
        break;
    case QSSGRenderShaderDataType::Float:
    {
        const float v = inValue.value<float>();
        return setUniform(name, &v, sizeof(float));
    }
        break;
    case QSSGRenderShaderDataType::Vec2:
    {
        const QVector2D v = inValue.value<QVector2D>();
        return setUniform(name, &v, 2 * sizeof(float));
    }
        break;
    case QSSGRenderShaderDataType::Vec3:
    {
        const QVector3D v = inValue.value<QVector3D>();
        return setUniform(name, &v, 3 * sizeof(float));
    }
        break;
    case QSSGRenderShaderDataType::Vec4:
    {
        const QVector4D v = inValue.value<QVector4D>();
        return setUniform(name, &v, 4 * sizeof(float));
    }
        break;
    case QSSGRenderShaderDataType::Rgba:
    {
        const QColor c = inValue.value<QColor>();
        const float v[4] = { float(c.redF()), float(c.greenF()), float(c.blueF()), float(c.alphaF()) };
        return setUniform(name, &v, 4 * sizeof(float));
    }
        break;
    case QSSGRenderShaderDataType::UnsignedInteger:
    {
        const quint32 v = inValue.value<quint32>();
        return setUniform(name, &v, sizeof(quint32));
    }
        break;
    case QSSGRenderShaderDataType::UnsignedIntegerVec2:
    {
        const quint32_2 v = inValue.value<quint32_2>();
        return setUniform(name, &v, 2 * sizeof(quint32));
    }
        break;
    case QSSGRenderShaderDataType::UnsignedIntegerVec3:
    {
        const quint32_3 v = inValue.value<quint32_3>();
        return setUniform(name, &v, 3 * sizeof(quint32));
    }
        break;
    case QSSGRenderShaderDataType::UnsignedIntegerVec4:
    {
        const quint32_4 v = inValue.value<quint32_4>();
        return setUniform(name, &v, 4 * sizeof(quint32));
    }
        break;
    case QSSGRenderShaderDataType::Matrix3x3:
    {
        const QMatrix3x3 m = inValue.value<QMatrix3x3>();
        float v[12]; // 4 floats per column, last one is unused
        memcpy(v, m.constData(), 3 * sizeof(float));
        memcpy(v + 4, m.constData() + 3, 3 * sizeof(float));
        memcpy(v + 8, m.constData() + 6, 3 * sizeof(float));
        return setUniform(name, &v, 12 * sizeof(float));
    }
        break;
    case QSSGRenderShaderDataType::Matrix4x4:
    {
        const QMatrix4x4 v = inValue.value<QMatrix4x4>();
        return setUniform(name, &v, 16 * sizeof(float));
    }
        break;
    default:
        qWarning("Attempted to set uniform %s value with unsupported data type %i",
                 name.constData(), int(inType));
        break;
    }
    return -1;
}

int QSSGRhiShaderStagesWithResources::setUniform(const QByteArray &name, const void *data, size_t size, int storeIndex)
{
    int index = storeIndex;
    if (storeIndex == -1) {
        auto it = m_uniformIndex.constFind(name);
        if (it != m_uniformIndex.cend()) {
            index = *it;
        } else {
            QSSGRhiShaderUniform u;
            Q_ASSERT(size <= sizeof(u.data));
            u.name = QString::fromLatin1(name);
            u.size = size;
            memcpy(u.data, data, size);

            const int new_idx = m_uniforms.size();
            m_uniformIndex[name] = new_idx;
            m_uniforms.push_back(u);
            index = new_idx;
        }
    }

    QSSGRhiShaderUniform &u = m_uniforms[index];
    if (size <= u.size) {
        u.dirty = true;
        memcpy(u.data, data, size);
    } else {
        qWarning("Attempted to set %u bytes to uniform %s with size %u", uint(size), name.constData(), uint(u.size));
    }

    return index;
}

void QSSGRhiShaderStagesWithResources::dumpUniforms()
{
    for (const QSSGRhiShaderUniform &u : m_uniforms) {
        qDebug() << u.name << u.size << u.dirty << QByteArray(u.data, int(u.size));
    }
}

int QSSGRhiShaderStagesWithResources::bindingForTexture(const QLatin1String &name, const QVector<int> **arrayDims) const
{
    QVector<QShaderDescription::InOutVariable> samplers = m_shaderStages->fragmentStage()->shader().description().combinedImageSamplers();

    auto it = std::find_if(samplers.cbegin(), samplers.cend(), [&name](const QShaderDescription::InOutVariable &s) {
        return s.name == name;
    });
    if (it != samplers.cend()) {
        if (arrayDims)
            *arrayDims = &it->arrayDims;
        return it->binding;
    }
    return -1;
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
            const int size = blk.size;
            QVarLengthArray<char, 512> bufferData; // not ideal but will do for now
            bufferData.resize(size);

            if (!*ubuf) {
                *ubuf = m_context.rhi()->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, size);
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
                            if (int(u.size) != var.size) {
                                qWarning("Uniform block member '%s' got %d bytes whereas the true size is %d",
                                         qPrintable(var.name), int(u.size), var.size);
                                Q_ASSERT(false);
                            }
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

void QSSGRhiShaderStagesWithResources::bakeLightsUniformBuffer(LightBufferSlot slot,
                                                               QRhiBuffer **ubuf,
                                                               QRhiResourceUpdateBatch *resourceUpdates)
{
    Q_ASSERT(m_lightsEnabled);

    const int size = int(sizeof(QSSGLightSourceShader) * QSSG_MAX_NUM_LIGHTS + (4 * sizeof(qint32)));

    if (!*ubuf) {
        *ubuf = m_context.rhi()->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, size);
        (*ubuf)->build();
    }
    if ((*ubuf)->size() < size) {
        (*ubuf)->setSize(size);
        (*ubuf)->build();
    }

    const qint32 count = m_lights[slot].count();
    resourceUpdates->updateDynamicBuffer(*ubuf, 0, sizeof(qint32), &count);

    for (int idx = 0; idx < count; ++idx) {
        const int offset = idx * sizeof(QSSGLightSourceShader) + (4 * sizeof(qint32));
        resourceUpdates->updateDynamicBuffer(*ubuf, offset, sizeof(QSSGLightSourceShader), &m_lights[slot][idx].lightData);
    }
}

QSSGRhiContext::QSSGRhiContext()
{
}

QSSGRhiContext::~QSSGRhiContext()
{
    for (QSSGRhiUniformBufferSet &uniformBufferSet : m_uniformBufferSets)
        uniformBufferSet.reset();

    qDeleteAll(m_pipelines);
    qDeleteAll(m_computePipelines);
    qDeleteAll(m_srbCache);
    qDeleteAll(m_textures);
    for (const auto &samplerInfo : qAsConst(m_samplers))
        delete samplerInfo.second;

    qDeleteAll(m_dummyTextures);
}

void QSSGRhiContext::initialize(QRhi *rhi)
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

    QRhiGraphicsPipeline::TargetBlend blend = key.state.targetBlend;
    blend.enable = key.state.blendEnable;
    QVarLengthArray<QRhiGraphicsPipeline::TargetBlend, 8> targetBlends(key.state.colorAttachmentCount);
    for (int i = 0; i < key.state.colorAttachmentCount; ++i)
        targetBlends[i] = blend;
    ps->setTargetBlends(targetBlends.cbegin(), targetBlends.cend());

    ps->setSampleCount(key.state.samples);

    ps->setDepthTest(key.state.depthTestEnable);
    ps->setDepthWrite(key.state.depthWriteEnable);
    ps->setDepthOp(key.state.depthFunc);

    ps->setDepthBias(key.state.depthBias);
    ps->setSlopeScaledDepthBias(key.state.slopeScaledDepthBias);

    if (!ps->build()) {
        qWarning("Failed to build graphics pipeline state");
        delete ps;
        return nullptr;
    }

    m_pipelines.insert(key, ps);
    return ps;
}

QRhiComputePipeline *QSSGRhiContext::computePipeline(const QSSGComputePipelineStateKey &key)
{
    auto it = m_computePipelines.constFind(key);
    if (it != m_computePipelines.constEnd())
        return it.value();

    QRhiComputePipeline *computePipeline = m_rhi->newComputePipeline();
    computePipeline->setShaderResourceBindings(key.layoutCompatibleSrb);
    computePipeline->setShaderStage({ QRhiShaderStage::Compute, key.shader });
    if (!computePipeline->build()) {
        qWarning("Failed to build compute pipeline");
        delete computePipeline;
        return nullptr;
    }
    m_computePipelines.insert(key, computePipeline);
    return computePipeline;
}

void QSSGRhiContext::invalidateCachedReferences(QRhiRenderPassDescriptor *rpDesc)
{
    if (!rpDesc)
        return;

    for (auto it = m_pipelines.begin(); it != m_pipelines.end(); ) {
        if (it.key().compatibleRpDesc == rpDesc) {
            // The QRhiGraphicsPipeline object is kept alive until the current
            // frame is submitted (by QRhi::endFrame()) The underlying native
            // graphics object(s) may live even longer in fact, but QRhi takes
            // care of that so that's no concern for us here.
            it.value()->releaseAndDestroyLater();
            it = m_pipelines.erase(it);
        } else {
            ++it;
        }
    }
}

using SamplerInfo = QPair<QSSGRhiSamplerDescription, QRhiSampler*>;

QRhiSampler *QSSGRhiContext::sampler(const QSSGRhiSamplerDescription &samplerDescription)
{
    auto compareSampler = [samplerDescription](const SamplerInfo &info){ return info.first == samplerDescription; };
    const auto found = std::find_if(m_samplers.cbegin(), m_samplers.cend(), compareSampler);
    if (found != m_samplers.cend())
        return found->second;

    QRhiSampler *newSampler = m_rhi->newSampler(samplerDescription.minFilter, samplerDescription.magFilter,
                                                samplerDescription.mipmap,
                                                samplerDescription.hTiling, samplerDescription.vTiling);
    if (!newSampler->build()) {
        qWarning("Failed to build image sampler");
        delete newSampler;
        return nullptr;
    }
    m_samplers << SamplerInfo{samplerDescription, newSampler};
    return newSampler;
}

QRhiTexture *QSSGRhiContext::dummyTexture(QRhiTexture::Flags flags, QRhiResourceUpdateBatch *rub)
{
    auto it = m_dummyTextures.constFind(flags);
    if (it != m_dummyTextures.constEnd())
        return *it;

    QRhiTexture *t = m_rhi->newTexture(QRhiTexture::RGBA8, QSize(64, 64), 1, flags);
    if (t->build()) {
        QImage image(t->pixelSize(), QImage::Format_RGBA8888);
        image.fill(Qt::black);
        rub->uploadTexture(t, image);
    } else {
        qWarning("Failed to build dummy texture");
    }

    m_dummyTextures.insert(flags, t);
    return t;
}

QT_END_NAMESPACE
