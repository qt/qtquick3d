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

#ifndef QSSGRHICONTEXT_P_H
#define QSSGRHICONTEXT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qtquick3drenderglobal_p.h"
#include <QtCore/qstack.h>
#include <QtQuick3DRender/private/qssgrenderbasetypes_p.h>
#include <QtGui/private/qrhi_p.h>

QT_BEGIN_NAMESPACE

class QSSGRhiContext;
class QSSGRhiBuffer;
class QSSGRhiShaderStagesWithResources;
struct QSSGShaderLightProperties;

struct Q_QUICK3DRENDER_EXPORT QSSGRhiInputAssemblerState
{
    QRhiVertexInputLayout inputLayout;
    QVector<QByteArray> inputLayoutInputNames;
    QRhiGraphicsPipeline::Topology topology;
    QSSGRef<QSSGRhiBuffer> vertexBuffer;
    QSSGRef<QSSGRhiBuffer> indexBuffer;

    static QRhiVertexInputAttribute::Format toVertexInputFormat(QSSGRenderComponentType compType, quint32 numComps);
    static QRhiGraphicsPipeline::Topology toTopology(QSSGRenderDrawMode drawMode);

    // Fills out inputLayout.attributes[].location based on
    // inputLayoutInputNames and the provided shader reflection info (unless
    // already done for the same 'shaders')
    void bakeVertexInputLocations(const QSSGRhiShaderStagesWithResources &shaders);
    const void *lastBakeVertexInputKey = nullptr;
};

class Q_QUICK3DRENDER_EXPORT QSSGRhiBuffer
{
    Q_DISABLE_COPY(QSSGRhiBuffer)
public:
    QAtomicInt ref;

    QSSGRhiBuffer(const QSSGRef<QSSGRhiContext> &context,
                  QRhiBuffer::Type type,
                  QRhiBuffer::UsageFlags usageMask,
                  quint32 stride,
                  int size,
                  QRhiCommandBuffer::IndexFormat indexFormat = QRhiCommandBuffer::IndexUInt16);

    virtual ~QSSGRhiBuffer();

    QRhiBuffer *buffer() const { return m_buffer; }
    quint32 stride() const { return m_stride; }
    quint32 numVertices() const {
        const quint32 sz = quint32(m_buffer->size());
        Q_ASSERT((sz % m_stride) == 0);
        return sz / m_stride;
    }
    QRhiCommandBuffer::IndexFormat indexFormat() const { return m_indexFormat; }

protected:
    QSSGRef<QSSGRhiContext> m_context;
    QRhiBuffer *m_buffer = nullptr;
    quint32 m_stride;
    QRhiCommandBuffer::IndexFormat m_indexFormat;
};

class Q_QUICK3DRENDER_EXPORT QSSGRhiShaderStages
{
    Q_DISABLE_COPY(QSSGRhiShaderStages)
public:
    QAtomicInt ref;

    QSSGRhiShaderStages(const QSSGRef<QSSGRhiContext> &context);

    QSSGRef<QSSGRhiContext> context() const { return m_context; }
    bool isNull() const { return m_stages.isEmpty(); }

    void addStage(const QRhiShaderStage &stage) { m_stages.append(stage); }
    const QVector<QRhiShaderStage> &stages() const { return m_stages; }

    const QRhiShaderStage *vertexStage() const {
        for (const QRhiShaderStage &s : m_stages) {
            if (s.type() == QRhiShaderStage::Vertex)
                return &s;
        }
        return nullptr;
    }

private:
    QSSGRef<QSSGRhiContext> m_context;
    QVector<QRhiShaderStage> m_stages;
};

struct QSSGRhiShaderUniform
{
    QString name; // because QShaderDescription will have a QString, not QByteArray
    bool dirty = false;
    size_t size = 0;
    char data[256];

private:
    size_t offset = SIZE_MAX;
    friend class QSSGRhiShaderStagesWithResources;
};

// these are our current shader limits
#define QSSG_MAX_NUM_LIGHTS 16
#define QSSG_MAX_NUM_SHADOWS 8

// note this struct must exactly match the memory layout of the
// struct sampleLight.glsllib and sampleArea.glsllib. If you make changes here you need
// to adjust the code in sampleLight.glsllib and sampleArea.glsllib as well
struct QSSGLightSourceShader
{
    QVector4D position;
    QVector4D direction; // Specifies the light direction in world coordinates.
    QVector4D up;
    QVector4D right;
    QVector4D diffuse;
    QVector4D ambient;
    QVector4D specular;
    float spotExponent; // Specifies the intensity distribution of the light.
    float spotCutoff; // Specifies the maximum spread angle of the light.
    float constantAttenuation; // Specifies the constant light attenuation factor.
    float linearAttenuation; // Specifies the linear light attenuation factor.
    float quadraticAttenuation; // Specifies the quadratic light attenuation factor.
    float range; // Specifies the maximum distance of the light influence
    float width; // Specifies the width of the area light surface.
    float height; // Specifies the height of the area light surface;
    QVector4D shadowControls;
    float shadowView[16];
    qint32 shadowIdx;
    float padding1[3];
};

struct QSSGShaderLightProperties
{
    // Color of the light
    QVector3D lightColor;
    QSSGLightSourceShader lightData;
};

class Q_QUICK3DRENDER_EXPORT QSSGRhiShaderStagesWithResources
{
    Q_DISABLE_COPY(QSSGRhiShaderStagesWithResources)
public:
    QAtomicInt ref;

    static QSSGRef<QSSGRhiShaderStagesWithResources> fromShaderStages(const QSSGRef<QSSGRhiShaderStages> &stages,
                                                                      const QByteArray &shaderKeyString);

    const QSSGRhiShaderStages *stages() const { return m_shaderStages.data(); }

    void setUniform(const QByteArray &name, const void *data, size_t size);
    void dumpUniforms();

    void resetLights() { m_lights.clear(); }
    QSSGShaderLightProperties &addLight() { m_lights.append(QSSGShaderLightProperties()); return m_lights.last(); }
    int lightCount() const { return m_lights.count(); }
    const QSSGShaderLightProperties &lightAt(int index) const { return m_lights[index]; }
    QSSGShaderLightProperties &lightAt(int index) { return m_lights[index]; }
    void setLightsEnabled(bool enable) { m_lightsEnabled = enable; }
    bool isLightingEnabled() const { return m_lightsEnabled; }

    void bakeMainUniformBuffer(QRhiBuffer **ubuf, QRhiResourceUpdateBatch *resourceUpdates);
    void bakeLightsUniformBuffer(QRhiBuffer **ubuf, QRhiResourceUpdateBatch *resourceUpdates);

    // other uniform buffers (aoshadow)
    // images
    // ... ?

    QSSGRhiShaderStagesWithResources(QSSGRef<QSSGRhiShaderStages> shaderStages, const QByteArray &shaderKeyString)
        : m_context(shaderStages->context()),
          m_shaderKeyString(shaderKeyString),
          m_shaderStages(shaderStages)
    {
    }

protected:
    QSSGRef<QSSGRhiContext> m_context;
    QByteArray m_shaderKeyString;
    QSSGRef<QSSGRhiShaderStages> m_shaderStages;
    QHash<QByteArray, QSSGRhiShaderUniform> m_uniforms; // members of the main (binding 0) uniform buffer
    bool m_lightsEnabled = false;
    QVarLengthArray<QSSGShaderLightProperties, QSSG_MAX_NUM_LIGHTS> m_lights;
};

struct Q_QUICK3DRENDER_EXPORT QSSGRhiGraphicsPipelineState
{
    QRhiViewport viewport;
    bool scissorEnable = false;
    QRhiScissor scissor;

    bool depthTestEnable = false;
    bool depthWriteEnable = false;
    QRhiGraphicsPipeline::CompareOp depthFunc = QRhiGraphicsPipeline::LessOrEqual;
    QRhiGraphicsPipeline::CullMode cullMode = QRhiGraphicsPipeline::None;
    bool blendEnable = false;
    QRhiGraphicsPipeline::TargetBlend targetBlend;

    const QSSGRhiShaderStages *shaderStages;

    QSSGRhiInputAssemblerState ia;

    static QRhiGraphicsPipeline::CullMode toCullMode(QSSGCullFaceMode cullFaceMode);
};

bool operator==(const QSSGRhiGraphicsPipelineState &a, const QSSGRhiGraphicsPipelineState &b) Q_DECL_NOTHROW;
bool operator!=(const QSSGRhiGraphicsPipelineState &a, const QSSGRhiGraphicsPipelineState &b) Q_DECL_NOTHROW;
uint qHash(const QSSGRhiGraphicsPipelineState &s, uint seed = 0) Q_DECL_NOTHROW;

struct QSSGGraphicsPipelineStateKey
{
    QSSGRhiGraphicsPipelineState state;
    QRhiRenderPassDescriptor *compatibleRpDesc;
    QRhiShaderResourceBindings *layoutCompatibleSrb;
};

bool operator==(const QSSGGraphicsPipelineStateKey &a, const QSSGGraphicsPipelineStateKey &b) Q_DECL_NOTHROW;
bool operator!=(const QSSGGraphicsPipelineStateKey &a, const QSSGGraphicsPipelineStateKey &b) Q_DECL_NOTHROW;
uint qHash(const QSSGGraphicsPipelineStateKey &k, uint seed = 0) Q_DECL_NOTHROW;

struct QSSGRhiUniformBufferSet
{
    QRhiBuffer *ubuf = nullptr;
    QRhiBuffer *lightsUbuf = nullptr;

    void reset() {
        delete ubuf;
        delete lightsUbuf;
        *this = QSSGRhiUniformBufferSet();
    }
};

class Q_QUICK3DRENDER_EXPORT QSSGRhiContext
{
    Q_DISABLE_COPY(QSSGRhiContext)
public:
    QAtomicInt ref;
    QSSGRhiContext();
    ~QSSGRhiContext();

    void setRhi(QRhi *rhi);
    QRhi *rhi() const { return m_rhi; }
    bool isValid() const { return m_rhi != nullptr; }

    void setMainRenderPassDescriptor(QRhiRenderPassDescriptor *rpDesc) { m_mainRpDesc = rpDesc; }
    QRhiRenderPassDescriptor *mainRenderPassDesciptor() const { return m_mainRpDesc; }

    void setCommandBuffer(QRhiCommandBuffer *cb) { m_cb = cb; }
    QRhiCommandBuffer *commandBuffer() const { return m_cb; }

    QSSGRhiGraphicsPipelineState *currentGraphicsPipelineState() { return &m_gfxPs; }
    const QSSGRhiGraphicsPipelineState *currentGraphicsPipelineState() const { return &m_gfxPs; }
    void resetGraphicsPipelineState() { m_gfxPs = QSSGRhiGraphicsPipelineState(); }

    void pushGraphicsPipelineState() { m_gfxPsStack.push(m_gfxPs); }
    void popGraphicsPipelineState(bool copyToCurrent) {
        if (copyToCurrent)
            m_gfxPs = m_gfxPsStack.pop();
        else
            m_gfxPsStack.pop();
    }

    using ShaderResourceBindingList = QVarLengthArray<QRhiShaderResourceBinding, 8>;
    QRhiShaderResourceBindings *srb(const ShaderResourceBindingList &bindings);
    QRhiGraphicsPipeline *pipeline(const QSSGGraphicsPipelineStateKey &key);

    QSSGRhiUniformBufferSet &uniformBufferSet(const void *key)
    {
        return m_uniformBufferSets[key];
    }

private:
    QRhi *m_rhi = nullptr;
    QRhiRenderPassDescriptor *m_mainRpDesc = nullptr;
    QRhiCommandBuffer *m_cb = nullptr;
    QSSGRhiGraphicsPipelineState m_gfxPs;
    QStack<QSSGRhiGraphicsPipelineState> m_gfxPsStack;
    QHash<ShaderResourceBindingList, QRhiShaderResourceBindings *> m_srbCache;
    QHash<QSSGGraphicsPipelineStateKey, QRhiGraphicsPipeline *> m_pipelines;
    QHash<const void *, QSSGRhiUniformBufferSet> m_uniformBufferSets;
};

QT_END_NAMESPACE

#endif
