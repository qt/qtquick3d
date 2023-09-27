// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qssgrhicontext_p.h"
#include <QtQuick3DUtils/private/qssgmesh_p.h>
#include <QtQuick3DUtils/private/qssgassert_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderableimage_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendermesh_p.h>
#include <QtQuick3DUtils/private/qssgutils_p.h>
#include <QtQuick3DUtils/private/qssgassert_p.h>
#include <QtCore/QVariant>
#include <qtquick3d_tracepoints_p.h>

QT_BEGIN_NAMESPACE

Q_TRACE_POINT(qtquick3d, QSSG_renderPass_entry, const QString &renderPass);
Q_TRACE_POINT(qtquick3d, QSSG_renderPass_exit);
Q_TRACE_POINT(qtquick3d, QSSG_drawIndexed, int indexCount, int instanceCount);
Q_TRACE_POINT(qtquick3d, QSSG_draw, int vertexCount, int instanceCount);

QSSGRhiBuffer::QSSGRhiBuffer(QSSGRhiContext &context,
                             QRhiBuffer::Type type,
                             QRhiBuffer::UsageFlags usageMask,
                             quint32 stride,
                             qsizetype size,
                             QRhiCommandBuffer::IndexFormat indexFormat)
    : m_context(context),
      m_stride(stride),
      m_indexFormat(indexFormat)
{
    QSSG_ASSERT(size >= 0, size = 0);
    m_buffer = m_context.rhi()->newBuffer(type, usageMask, quint32(size));
    if (!m_buffer->create())
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
    } else if (compType == QSSGRenderComponentType::UnsignedInt32) {
        switch (numComps) {
        case 1:
            return QRhiVertexInputAttribute::UInt;
        case 2:
            return QRhiVertexInputAttribute::UInt2;
        case 3:
            return QRhiVertexInputAttribute::UInt3;
        case 4:
            return QRhiVertexInputAttribute::UInt4;
        default:
            break;
        }
    } else if (compType == QSSGRenderComponentType::Int32) {
        switch (numComps) {
        case 1:
            return QRhiVertexInputAttribute::SInt;
        case 2:
            return QRhiVertexInputAttribute::SInt2;
        case 3:
            return QRhiVertexInputAttribute::SInt3;
        case 4:
            return QRhiVertexInputAttribute::SInt4;
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
    case QSSGRenderDrawMode::TriangleFan:
        return QRhiGraphicsPipeline::TriangleFan;
    case QSSGRenderDrawMode::Triangles:
        return QRhiGraphicsPipeline::Triangles;
    case QSSGRenderDrawMode::LineLoop:
        QSSG_ASSERT_X(false, "LineLoop draw mode is not supported", return QRhiGraphicsPipeline::Triangles);
    }

    Q_UNREACHABLE_RETURN(QRhiGraphicsPipeline::Triangles);
}

void QSSGRhiInputAssemblerState::bakeVertexInputLocations(const QSSGRhiShaderPipeline &shaders, int instanceBufferBinding)
{
    if (!shaders.vertexStage())
        return;

    const auto &vertexInputs = shaders.vertexInputs();

    QVarLengthArray<QRhiVertexInputAttribute, 8> attrs;
    int inputIndex = 0;
    for (auto it = inputLayout.cbeginAttributes(), itEnd = inputLayout.cendAttributes(); it != itEnd; ++it) {
        const QSSGRhiInputAssemblerState::InputSemantic sem = inputs.at(inputIndex); // avoid detaching - submeshes share the same name list
        auto vertexInputVar = vertexInputs.constFind(sem);
        if (vertexInputVar != vertexInputs.constEnd()) {
            attrs.append(*it);
            attrs.last().setLocation(vertexInputVar->location);
        } // else the mesh has an input attribute that is not declared and used in the vertex shader - that's fine

        ++inputIndex;
    }

    // Add instance buffer input if necessary
    if (instanceBufferBinding > 0) {
        auto instanceBufferLocations = shaders.instanceBufferLocations();
        // transform0
        attrs.append(QRhiVertexInputAttribute(instanceBufferBinding,
                                             instanceBufferLocations.transform0,
                                             QRhiVertexInputAttribute::Float4,
                                             0));
        // transform1
        attrs.append(QRhiVertexInputAttribute(instanceBufferBinding,
                                             instanceBufferLocations.transform1,
                                             QRhiVertexInputAttribute::Float4,
                                             sizeof(float) * 4));
        // transform2
        attrs.append(QRhiVertexInputAttribute(instanceBufferBinding,
                                             instanceBufferLocations.transform2,
                                             QRhiVertexInputAttribute::Float4,
                                             sizeof(float) * 4 * 2));
        // color
        attrs.append(QRhiVertexInputAttribute(instanceBufferBinding,
                                             instanceBufferLocations.color,
                                             QRhiVertexInputAttribute::Float4,
                                             sizeof(float) * 4 * 3));
        // data
        attrs.append(QRhiVertexInputAttribute(instanceBufferBinding,
                                             instanceBufferLocations.data,
                                             QRhiVertexInputAttribute::Float4,
                                             sizeof(float) * 4 * 4));
    }

    inputLayout.setAttributes(attrs.cbegin(), attrs.cend());
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
    case QSSGCullFaceMode::Unknown:
        return QRhiGraphicsPipeline::None;
    }

    Q_UNREACHABLE_RETURN(QRhiGraphicsPipeline::None);
}

void QSSGRhiShaderPipeline::addStage(const QRhiShaderStage &stage, StageFlags flags)
{
    m_stages.append(stage);

    // Copy all member infos for the uniform block with binding 0 into m_ub0
    // for faster lookup.
    if (stage.type() == QRhiShaderStage::Vertex) {
        // Optimize by doing it only for the vertex shader. This code path is
        // only hit for pipelines with vertex+fragment stages and an in shaders
        // from materials an identical uniform block is present in both
        // shaders, so go through only one of them.
        const QVector<QShaderDescription::UniformBlock> uniformBlocks = stage.shader().description().uniformBlocks();
        for (const QShaderDescription::UniformBlock &blk : uniformBlocks) {
            if (blk.binding == 0) {
                m_ub0Size = blk.size;
                m_ub0NextUBufOffset = m_context.rhi()->ubufAligned(m_ub0Size);
                for (const QShaderDescription::BlockVariable &var : blk.members)
                    m_ub0[var.name] = var;
                break;
            }
        }
        // Now the same for vertex inputs.
        if (!flags.testFlag(UsedWithoutIa)) {
            const QVector<QShaderDescription::InOutVariable> inputs = stage.shader().description().inputVariables();
            for (const QShaderDescription::InOutVariable &var : inputs) {
                if (var.name == QSSGMesh::MeshInternal::getPositionAttrName()) {
                    m_vertexInputs[QSSGRhiInputAssemblerState::PositionSemantic] = var;
                } else if (var.name == QSSGMesh::MeshInternal::getNormalAttrName()) {
                    m_vertexInputs[QSSGRhiInputAssemblerState::NormalSemantic] = var;
                } else if (var.name == QSSGMesh::MeshInternal::getUV0AttrName()) {
                    m_vertexInputs[QSSGRhiInputAssemblerState::TexCoord0Semantic] = var;
                } else if (var.name == QSSGMesh::MeshInternal::getUV1AttrName()) {
                    m_vertexInputs[QSSGRhiInputAssemblerState::TexCoord1Semantic] = var;
                } else if (var.name == QSSGMesh::MeshInternal::getLightmapUVAttrName()) {
                    m_vertexInputs[QSSGRhiInputAssemblerState::TexCoordLightmapSemantic] = var;
                } else if (var.name == QSSGMesh::MeshInternal::getTexTanAttrName()) {
                    m_vertexInputs[QSSGRhiInputAssemblerState::TangentSemantic] = var;
                } else if (var.name == QSSGMesh::MeshInternal::getTexBinormalAttrName()) {
                    m_vertexInputs[QSSGRhiInputAssemblerState::BinormalSemantic] = var;
                } else if (var.name == QSSGMesh::MeshInternal::getColorAttrName()) {
                    m_vertexInputs[QSSGRhiInputAssemblerState::ColorSemantic] = var;
                } else if (var.name == QSSGMesh::MeshInternal::getJointAttrName()) {
                    m_vertexInputs[QSSGRhiInputAssemblerState::JointSemantic] = var;
                } else if (var.name == QSSGMesh::MeshInternal::getWeightAttrName()) {
                    m_vertexInputs[QSSGRhiInputAssemblerState::WeightSemantic] = var;
                } else if (var.name == "qt_instanceTransform0") {
                    instanceLocations.transform0 = var.location;
                } else if (var.name == "qt_instanceTransform1") {
                    instanceLocations.transform1 = var.location;
                } else if (var.name == "qt_instanceTransform2") {
                    instanceLocations.transform2 = var.location;
                } else if (var.name == "qt_instanceColor") {
                    instanceLocations.color = var.location;
                } else if (var.name == "qt_instanceData") {
                    instanceLocations.data = var.location;
                } else {
                    qWarning("Ignoring vertex input %s in shader", var.name.constData());
                }
            }
        }
    }

    const QVector<QShaderDescription::InOutVariable> combinedImageSamplers  = stage.shader().description().combinedImageSamplers();
    for (const QShaderDescription::InOutVariable &var : combinedImageSamplers)
        m_combinedImageSamplers[var.name] = var;

    std::fill(m_materialImageSamplerBindings,
              m_materialImageSamplerBindings + size_t(QSSGRhiSamplerBindingHints::BindingMapSize),
              -1);
}

void QSSGRhiShaderPipeline::setUniformValue(char *ubufData, const char *name, const QVariant &inValue, QSSGRenderShaderValue::Type inType)
{
    using namespace QSSGRenderShaderValue;
    switch (inType) {
    case QSSGRenderShaderValue::Integer:
    {
        const qint32 v = inValue.toInt();
        setUniform(ubufData, name, &v, sizeof(qint32));
    }
        break;
    case QSSGRenderShaderValue::IntegerVec2:
    {
        const ivec2 v = inValue.value<ivec2>();
        setUniform(ubufData, name, &v, 2 * sizeof(qint32));
    }
        break;
    case QSSGRenderShaderValue::IntegerVec3:
    {
        const ivec3 v = inValue.value<ivec3>();
        setUniform(ubufData, name, &v, 3 * sizeof(qint32));
    }
        break;
    case QSSGRenderShaderValue::IntegerVec4:
    {
        const ivec4 v = inValue.value<ivec4>();
        setUniform(ubufData, name, &v, 4 * sizeof(qint32));
    }
        break;
    case QSSGRenderShaderValue::Boolean:
    {
        // whatever bool is does not matter, what matters is that the GLSL bool is 4 bytes
        const qint32 v = inValue.value<bool>();
        setUniform(ubufData, name, &v, sizeof(qint32));
    }
        break;
    case QSSGRenderShaderValue::BooleanVec2:
    {
        const bvec2 b = inValue.value<bvec2>();
        const ivec2 v(b.x, b.y);
        setUniform(ubufData, name, &v, 2 * sizeof(qint32));
    }
        break;
    case QSSGRenderShaderValue::BooleanVec3:
    {
        const bvec3 b = inValue.value<bvec3>();
        const ivec3 v(b.x, b.y, b.z);
        setUniform(ubufData, name, &v, 3 * sizeof(qint32));
    }
        break;
    case QSSGRenderShaderValue::BooleanVec4:
    {
        const bvec4 b = inValue.value<bvec4>();
        const ivec4 v(b.x, b.y, b.z, b.w);
        setUniform(ubufData, name, &v, 4 * sizeof(qint32));
    }
        break;
    case QSSGRenderShaderValue::Float:
    {
        const float v = inValue.value<float>();
        setUniform(ubufData, name, &v, sizeof(float));
    }
        break;
    case QSSGRenderShaderValue::Vec2:
    {
        const QVector2D v = inValue.value<QVector2D>();
        setUniform(ubufData, name, &v, 2 * sizeof(float));
    }
        break;
    case QSSGRenderShaderValue::Vec3:
    {
        const QVector3D v = inValue.value<QVector3D>();
        setUniform(ubufData, name, &v, 3 * sizeof(float));
    }
        break;
    case QSSGRenderShaderValue::Vec4:
    {
        const QVector4D v = inValue.value<QVector4D>();
        setUniform(ubufData, name, &v, 4 * sizeof(float));
    }
        break;
    case QSSGRenderShaderValue::Rgba:
    {
        const QVector4D v = QSSGUtils::color::sRGBToLinear(inValue.value<QColor>());
        setUniform(ubufData, name, &v, 4 * sizeof(float));
    }
        break;
    case QSSGRenderShaderValue::UnsignedInteger:
    {
        const quint32 v = inValue.value<quint32>();
        setUniform(ubufData, name, &v, sizeof(quint32));
    }
        break;
    case QSSGRenderShaderValue::UnsignedIntegerVec2:
    {
        const uvec2 v = inValue.value<uvec2>();
        setUniform(ubufData, name, &v, 2 * sizeof(quint32));
    }
        break;
    case QSSGRenderShaderValue::UnsignedIntegerVec3:
    {
        const uvec3 v = inValue.value<uvec3>();
        setUniform(ubufData, name, &v, 3 * sizeof(quint32));
    }
        break;
    case QSSGRenderShaderValue::UnsignedIntegerVec4:
    {
        const uvec4 v = inValue.value<uvec4>();
        setUniform(ubufData, name, &v, 4 * sizeof(quint32));
    }
        break;
    case QSSGRenderShaderValue::Matrix3x3:
    {
        const QMatrix3x3 m = inValue.value<QMatrix3x3>();
        setUniform(ubufData, name, m.constData(), 12 * sizeof(float), nullptr, QSSGRhiShaderPipeline::UniformFlag::Mat3);
    }
        break;
    case QSSGRenderShaderValue::Matrix4x4:
    {
        const QMatrix4x4 v = inValue.value<QMatrix4x4>();
        setUniform(ubufData, name, v.constData(), 16 * sizeof(float));
    }
        break;
    case QSSGRenderShaderValue::Size:
    {
        const QSize s = inValue.value<QSize>();
        float v[2] = { float(s.width()), float(s.height()) };
        setUniform(ubufData, name, v, 2 * sizeof(float));
    }
        break;
    case QSSGRenderShaderValue::SizeF:
    {
        const QSizeF s = inValue.value<QSizeF>();
        float v[2] = { float(s.width()), float(s.height()) };
        setUniform(ubufData, name, v, 2 * sizeof(float));
    }
        break;
    case QSSGRenderShaderValue::Point:
    {
        const QPoint p = inValue.value<QPoint>();
        float v[2] = { float(p.x()), float(p.y()) };
        setUniform(ubufData, name, v, 2 * sizeof(float));
    }
        break;
    case QSSGRenderShaderValue::PointF:
    {
        const QPointF p = inValue.value<QPointF>();
        float v[2] = { float(p.x()), float(p.y()) };
        setUniform(ubufData, name, v, 2 * sizeof(float));
    }
        break;
    case QSSGRenderShaderValue::Rect:
    {
        const QRect r = inValue.value<QRect>();
        float v[4] = { float(r.x()), float(r.y()), float(r.width()), float(r.height()) };
        setUniform(ubufData, name, v, 4 * sizeof(float));
    }
        break;
    case QSSGRenderShaderValue::RectF:
    {
        const QRectF r = inValue.value<QRectF>();
        float v[4] = { float(r.x()), float(r.y()), float(r.width()), float(r.height()) };
        setUniform(ubufData, name, v, 4 * sizeof(float));
    }
        break;
    case QSSGRenderShaderValue::Quaternion:
    {
        const QQuaternion q = inValue.value<QQuaternion>();
        float v[4] = { float(q.x()), float(q.y()), float(q.z()), float(q.scalar()) };
        setUniform(ubufData, name, v, 4 * sizeof(float));
    }
        break;
    default:
        qWarning("Attempted to set uniform %s value with unsupported data type %i",
                 name, int(inType));
        break;
    }
}

int QSSGRhiShaderPipeline::offsetOfUniform(const QByteArray &name)
{
    auto iter = m_ub0.constFind(name);
    if (iter != m_ub0.cend())
        return iter->offset;
    return -1;
}

static QString getUBMemberSizeWarning(QLatin1StringView name, qsizetype correctedSize, qsizetype requestedSize)
{
    return QStringLiteral("Uniform block member '%1' got %2 bytes whereas the true size is %3").arg(name, QString::number(correctedSize), QString::number(requestedSize));
}

void QSSGRhiShaderPipeline::setUniform(char *ubufData, const char *name, const void *data, size_t size, int *storeIndex, UniformFlags flags)
{
    int index = -1;
    if (!storeIndex || *storeIndex == -1) {
        const QByteArray ba = QByteArray::fromRawData(name, strlen(name));
        auto it = m_uniformIndex.constFind(ba);
        if (it != m_uniformIndex.cend()) {
            index = int(*it);
        } else if (ba.size() < qsizetype(sizeof(QSSGRhiShaderUniform::name))) {
            QSSGRhiShaderUniform u;
            memcpy(u.name, name, ba.size() + 1);
            u.size = size;

            const int new_idx = m_uniforms.size();
            m_uniformIndex[name] = new_idx; // key is name, not ba, this has to be a deep copy QByteArray
            m_uniforms.push_back(u);
            index = new_idx;
        } else {
            qWarning("Attempted to set uniform with too long name: %s", name);
            return;
        }
        if (storeIndex)
            *storeIndex = index;
    } else {
        index = *storeIndex;
    }

    Q_ASSERT(index >= 0);
    QSSGRhiShaderUniform &u = m_uniforms[index];
    if (size <= u.size) {
        if (u.offset == SIZE_MAX && u.maybeExists) {
            auto it = m_ub0.constFind(QByteArray::fromRawData(u.name, strlen(u.name)));
            if (it != m_ub0.constEnd()) {
                u.offset = it->offset;
                QSSG_ASSERT_X(QSSG_DEBUG_COND(int(u.size) == it->size), qPrintable(getUBMemberSizeWarning(QLatin1StringView(it->name), u.size, it->size)), return);
            }
        }
        if (u.offset == SIZE_MAX) {
            // must silently ignore uniforms that are not in the actual shader
            u.maybeExists = false; // but do not try again
            return;
        }

        char *dst = ubufData + u.offset;
        if (flags.testFlag(UniformFlag::Mat3)) {
            // mat3 is still 4 floats per column in the uniform buffer (but there
            // is no 4th column), so 48 bytes altogether, not 36 or 64.
            const float *src = static_cast<const float *>(data);
            memcpy(dst, src, 3 * sizeof(float));
            memcpy(dst + 4 * sizeof(float), src + 3, 3 * sizeof(float));
            memcpy(dst + 8 * sizeof(float), src + 6, 3 * sizeof(float));
        } else {
            memcpy(dst, data, size);
        }
    } else {
        qWarning("Attempted to set %u bytes to uniform %s with size %u", uint(size), name, uint(u.size));
    }
}

// Quick3D uniform buffer is std140 type and all array data should be stored in this rule.
// You can check it in glspec45.core.pdf's 7.6.2.2.(4)
// https://www.khronos.org/registry/OpenGL/specs/gl/glspec45.core.pdf
void QSSGRhiShaderPipeline::setUniformArray(char *ubufData, const char *name, const void *data, size_t itemCount, QSSGRenderShaderValue::Type type, int *storeIndex)
{
    using namespace QSSGRenderShaderValue;

    QSSGRhiShaderUniformArray *ua = nullptr;
    constexpr size_t std140BaseTypeSize = 4 * sizeof(float);

    static const auto checkSize = [std140BaseTypeSize](QSSGRhiShaderUniformArray *ua) -> bool {
        Q_UNUSED(std140BaseTypeSize); // Silence clang warning about unneeded lambda capture (MSVC requires it be captrued).
        const size_t uniformSize = std140BaseTypeSize < ua->typeSize ? ua->typeSize * ua->itemCount : std140BaseTypeSize * ua->itemCount;
        QSSG_ASSERT_X(uniformSize == ua->size, qPrintable(getUBMemberSizeWarning(QLatin1StringView(ua->name), uniformSize, ua->size)), return false);
        return true;
    };

    if (!storeIndex || *storeIndex == -1) {
        int index = -1;
        const QByteArray ba = QByteArray::fromRawData(name, strlen(name));
        auto it = m_uniformIndex.constFind(ba);
        if (it != m_uniformIndex.cend()) {
            index = int(*it);
            ua = &m_uniformArrays[index];
        } else if (ba.size() < qsizetype(sizeof(QSSGRhiShaderUniformArray::name))) {
            index = m_uniformArrays.size();
            m_uniformArrays.push_back(QSSGRhiShaderUniformArray());
            m_uniformIndex[name] = index; // key needs deep copy
            ua = &m_uniformArrays.last();
            memcpy(ua->name, name, ba.size() + 1);
        } else {
            qWarning("Attempted to set uniform array with too long name: %s", name);
            return;
        }
        if (storeIndex)
            *storeIndex = index;
    } else {
        ua = &m_uniformArrays[*storeIndex];
    }

    if (!ua)
        return;

    if (ua->offset == SIZE_MAX && ua->maybeExists) {
        auto it = m_ub0.constFind(QByteArray::fromRawData(ua->name, strlen(ua->name)));
        if (it != m_ub0.constEnd()) {
            ua->offset = it->offset;
            ua->size = it->size;
        }
    }
    if (ua->offset == SIZE_MAX) {
        // must silently ignore uniforms that are not in the actual shader
        ua->maybeExists = false; // but do not try again
        return;
    }

    char *p = ubufData + ua->offset;

    switch (type) {
    case QSSGRenderShaderValue::Integer:
    {
        const qint32 *v = static_cast<const qint32 *>(data);
        if (sizeof(qint32) != ua->typeSize || itemCount != ua->itemCount) {
            ua->typeSize = sizeof(qint32);
            ua->itemCount = itemCount;
        }

        QSSG_ASSERT(QSSG_DEBUG_COND(checkSize(ua)), return);

        for (size_t i = 0; i < itemCount; ++i)
            memcpy(p + i * std140BaseTypeSize, &v[i], ua->typeSize);
    }
        break;
    case QSSGRenderShaderValue::IntegerVec2:
    {
        const ivec2 *v = static_cast<const ivec2 *>(data);
        if (2 * sizeof(qint32) != ua->typeSize || itemCount != ua->itemCount) {
            ua->typeSize = 2 * sizeof(qint32);
            ua->itemCount = itemCount;
        }

        QSSG_ASSERT(QSSG_DEBUG_COND(checkSize(ua)), return);

        for (size_t i = 0; i < itemCount; ++i)
            memcpy(p + i * std140BaseTypeSize, &v[i], ua->typeSize);
    }
        break;
    case QSSGRenderShaderValue::IntegerVec3:
    {
        const QSSGRenderShaderValue::ivec3 *v = static_cast<const QSSGRenderShaderValue::ivec3 *>(data);
        if (3 * sizeof(qint32) != ua->typeSize || itemCount != ua->itemCount) {
            ua->typeSize = 3 * sizeof(qint32);
            ua->itemCount = itemCount;
        }

        QSSG_ASSERT(QSSG_DEBUG_COND(checkSize(ua)), return);

        for (size_t i = 0; i < itemCount; ++i)
            memcpy(p + i * std140BaseTypeSize, &v[i], ua->typeSize);
    }
        break;
    case QSSGRenderShaderValue::IntegerVec4:
    {
        const ivec4 *v = static_cast<const ivec4 *>(data);
        if (4 * sizeof(qint32) != ua->typeSize || itemCount != ua->itemCount) {
            ua->typeSize = 4 * sizeof(qint32);
            ua->itemCount = itemCount;
        }

        QSSG_ASSERT(QSSG_DEBUG_COND(checkSize(ua)), return);

        memcpy(p, v, ua->typeSize * ua->itemCount);
    }
        break;
    case QSSGRenderShaderValue::Float:
    {
        const float *v = static_cast<const float *>(data);
        if (sizeof(float) != ua->typeSize || itemCount != ua->itemCount) {
            ua->typeSize = sizeof(float);
            ua->itemCount = itemCount;
        }

        QSSG_ASSERT(QSSG_DEBUG_COND(checkSize(ua)), return);

        for (size_t i = 0; i < itemCount; ++i)
            memcpy(p + i * std140BaseTypeSize, &v[i], ua->typeSize);
    }
        break;
    case QSSGRenderShaderValue::Vec2:
    {
        const QVector2D *v = static_cast<const QVector2D *>(data);
        if (2 * sizeof(float) != ua->typeSize || itemCount != ua->itemCount) {
            ua->typeSize = 2 * sizeof(float);
            ua->itemCount = itemCount;
        }

        QSSG_ASSERT(QSSG_DEBUG_COND(checkSize(ua)), return);

        for (size_t i = 0; i < itemCount; ++i)
            memcpy(p + i * std140BaseTypeSize, &v[i], ua->typeSize);
    }
        break;
    case QSSGRenderShaderValue::Vec3:
    {
        const QVector3D *v = static_cast<const QVector3D *>(data);
        if (3 * sizeof(float) != ua->typeSize || itemCount != ua->itemCount) {
            ua->typeSize = 3 * sizeof(float);
            ua->itemCount = itemCount;
        }

        QSSG_ASSERT(QSSG_DEBUG_COND(checkSize(ua)), return);

        for (size_t i = 0; i < itemCount; ++i)
            memcpy(p + i * std140BaseTypeSize, &v[i], ua->typeSize);
    }
        break;
    case QSSGRenderShaderValue::Vec4:
    {
        const QVector4D *v = static_cast<const QVector4D *>(data);
        if (4 * sizeof(float) != ua->typeSize || itemCount != ua->itemCount) {
            ua->typeSize = 4 * sizeof(float);
            ua->itemCount = itemCount;
        }

        QSSG_ASSERT(QSSG_DEBUG_COND(checkSize(ua)), return);

        memcpy(p, v, ua->typeSize * ua->itemCount);
    }
        break;
    case QSSGRenderShaderValue::Rgba:
    {
        const QColor *v = static_cast<const QColor *>(data);
        if (4 * sizeof(float) != ua->typeSize || itemCount != ua->itemCount) {
            ua->typeSize = 4 * sizeof(float);
            ua->itemCount = itemCount;
        }

        QSSG_ASSERT(QSSG_DEBUG_COND(checkSize(ua)), return);

        for (size_t i = 0; i < itemCount; ++i) {
            const QVector4D vi = QSSGUtils::color::sRGBToLinear(v[i]);
            memcpy(p + i * std140BaseTypeSize, &vi, ua->typeSize);
        }
    }
        break;
    case QSSGRenderShaderValue::UnsignedInteger:
    {
        const quint32 *v = static_cast<const quint32 *>(data);
        if (sizeof(quint32) != ua->typeSize || itemCount != ua->itemCount) {
            ua->typeSize = sizeof(quint32);
            ua->itemCount = itemCount;
        }

        QSSG_ASSERT(QSSG_DEBUG_COND(checkSize(ua)), return);

        for (size_t i = 0; i < itemCount; ++i)
            memcpy(p + i * std140BaseTypeSize, &v[i], ua->typeSize);
    }
        break;
    case QSSGRenderShaderValue::UnsignedIntegerVec2:
    {
        const uvec2 *v = static_cast<const uvec2 *>(data);
        if (2 * sizeof(quint32) != ua->typeSize || itemCount != ua->itemCount) {
            ua->typeSize = 2 * sizeof(quint32);
            ua->itemCount = itemCount;
        }

        QSSG_ASSERT(QSSG_DEBUG_COND(checkSize(ua)), return);

        for (size_t i = 0; i < itemCount; ++i)
            memcpy(p + i * std140BaseTypeSize, &v[i], ua->typeSize);
    }
        break;
    case QSSGRenderShaderValue::UnsignedIntegerVec3:
    {
        const uvec3 *v = static_cast<const uvec3 *>(data);
        if (3 * sizeof(quint32) != ua->typeSize || itemCount != ua->itemCount) {
            ua->typeSize = 3 * sizeof(quint32);
            ua->itemCount = itemCount;
        }

        QSSG_ASSERT(QSSG_DEBUG_COND(checkSize(ua)), return);

        for (size_t i = 0; i < itemCount; ++i)
            memcpy(p + i * std140BaseTypeSize, &v[i], ua->typeSize);
    }
        break;
    case QSSGRenderShaderValue::UnsignedIntegerVec4:
    {
        const uvec4 *v = static_cast<const uvec4 *>(data);
        if (4 * sizeof(quint32) != ua->typeSize || itemCount != ua->itemCount) {
            ua->typeSize = 4 * sizeof(quint32);
            ua->itemCount = itemCount;
        }

        QSSG_ASSERT(QSSG_DEBUG_COND(checkSize(ua)), return);

        memcpy(p, v, ua->typeSize * ua->itemCount);
    }
        break;
    case QSSGRenderShaderValue::Matrix3x3:
    {
        const QMatrix3x3 *v = static_cast<const QMatrix3x3 *>(data);
        if (12 * sizeof(float) != ua->typeSize || itemCount != ua->itemCount) {
            ua->typeSize = 12 * sizeof(float);
            ua->itemCount = itemCount;
        }

        QSSG_ASSERT(QSSG_DEBUG_COND(checkSize(ua)), return);

        for (uint i = 0; i < ua->itemCount; ++i) {
            memcpy(p + i * ua->typeSize, v[i].constData(), 3 * sizeof(float));
            memcpy(p + i * ua->typeSize + 4 * sizeof(float), v[i].constData() + 3, 3 * sizeof(float));
            memcpy(p + i * ua->typeSize + 8 * sizeof(float), v[i].constData() + 6, 3 * sizeof(float));
        }
    }
        break;
    case QSSGRenderShaderValue::Matrix4x4:
    {
        const QMatrix4x4 *v = static_cast<const QMatrix4x4 *>(data);
        if (16 * sizeof(float) != ua->typeSize || itemCount != ua->itemCount) {
            ua->typeSize = 16 * sizeof(float);
            ua->itemCount = itemCount;
        }

        QSSG_ASSERT(QSSG_DEBUG_COND(checkSize(ua)), return);

        for (uint i = 0; i < ua->itemCount; ++i)
            memcpy(p + i * ua->typeSize, &v[i] , ua->typeSize);
    }
        break;
    case QSSGRenderShaderValue::Boolean:
    case QSSGRenderShaderValue::BooleanVec2:
    case QSSGRenderShaderValue::BooleanVec3:
    case QSSGRenderShaderValue::BooleanVec4:
    case QSSGRenderShaderValue::Size:
    case QSSGRenderShaderValue::SizeF:
    case QSSGRenderShaderValue::Point:
    case QSSGRenderShaderValue::PointF:
    case QSSGRenderShaderValue::Rect:
    case QSSGRenderShaderValue::RectF:
    case QSSGRenderShaderValue::Quaternion:
    default:
        qWarning("Attempted to set uniform %s value with type %d that is unsupported for uniform arrays",
                 name, int(type));
        break;
    }
}

void QSSGRhiShaderPipeline::ensureCombinedMainLightsUniformBuffer(QRhiBuffer **ubuf)
{
    const quint32 totalBufferSize = m_ub0NextUBufOffset + sizeof(QSSGShaderLightsUniformData);
    if (!*ubuf) {
        *ubuf = m_context.rhi()->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, totalBufferSize);
        (*ubuf)->create();
    }
    if ((*ubuf)->size() < totalBufferSize) {
        (*ubuf)->setSize(totalBufferSize);
        (*ubuf)->create();
    }
}

void QSSGRhiShaderPipeline::ensureUniformBuffer(QRhiBuffer **ubuf)
{
    if (!*ubuf) {
        *ubuf = m_context.rhi()->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, m_ub0Size);
        (*ubuf)->create();
    }
}

int QSSGRhiShaderPipeline::bindingForTexture(const char *name, int hint)
{
    if (hint >= 0) {
        const int binding = m_materialImageSamplerBindings[hint];
        if (binding >= 0)
            return binding;
    }

    auto it = m_combinedImageSamplers.constFind(QByteArray::fromRawData(name, strlen(name)));
    const int binding = it != m_combinedImageSamplers.cend() ? it->binding : -1;
    if (hint >= 0)
        m_materialImageSamplerBindings[hint] = binding;

    return binding;
}

QSSGRhiContext::QSSGRhiContext(QRhi *rhi)
    : m_rhi(rhi)
    , m_stats(*this)
{
    Q_STATIC_ASSERT(int(QSSGRhiSamplerBindingHints::LightProbe) > int(QSSGRenderableImage::Type::Occlusion));
}


QSSGRhiContext::~QSSGRhiContext()
{
    releaseCachedResources();

    qDeleteAll(m_textures);
    qDeleteAll(m_meshes);
}

void QSSGRhiContext::releaseCachedResources()
{
    for (QSSGRhiDrawCallData &dcd : m_drawCallData) {
        // We don't call releaseDrawCallData() here, since we're anyways
        // are going to delete the non-owned resources further down and
        // there's no point in removing each of those one-by-one, as is
        // the case with releaseDrawCallData().
        // This speeds up the release of cached resources greatly when there
        // are many entries in the map, also at application shutdown.
        dcd.reset();
    }

    m_drawCallData.clear();

    qDeleteAll(m_pipelines);
    qDeleteAll(m_computePipelines);
    qDeleteAll(m_srbCache);
    qDeleteAll(m_dummyTextures);

    m_pipelines.clear();
    m_computePipelines.clear();
    m_srbCache.clear();
    m_dummyTextures.clear();

    for (const auto &samplerInfo : std::as_const(m_samplers))
        delete samplerInfo.second;

    m_samplers.clear();

    for (const auto &particleData : std::as_const(m_particleData))
        delete particleData.texture;

    m_particleData.clear();

    for (const auto &instanceData : std::as_const(m_instanceBuffers)) {
        if (instanceData.owned)
            delete instanceData.buffer;
    }

    m_instanceBuffers.clear();

    for (const auto &instanceData : std::as_const(m_instanceBuffersLod)) {
        if (instanceData.owned)
            delete instanceData.buffer;
    }

    m_instanceBuffersLod.clear();
}

void QSSGRhiContext::initialize(QRhi *rhi)
{
    Q_ASSERT(rhi && !m_rhi);
    m_rhi = rhi;
}

QRhiShaderResourceBindings *QSSGRhiContext::srb(const QSSGRhiShaderResourceBindingList &bindings)
{
    auto it = m_srbCache.constFind(bindings);
    if (it != m_srbCache.constEnd())
        return *it;

    QRhiShaderResourceBindings *srb = m_rhi->newShaderResourceBindings();
    srb->setBindings(bindings.v, bindings.v + bindings.p);
    if (srb->create()) {
        m_srbCache.insert(bindings, srb);
    } else {
        qWarning("Failed to build srb");
        delete srb;
        srb = nullptr;
    }
    return srb;
}

void QSSGRhiContext::releaseDrawCallData(QSSGRhiDrawCallData &dcd)
{
    delete dcd.ubuf;
    dcd.ubuf = nullptr;
    auto srb = m_srbCache.take(dcd.bindings);
    QSSG_CHECK(srb == dcd.srb);
    delete srb;
    dcd.srb = nullptr;
    dcd.pipeline = nullptr;
}

QRhiGraphicsPipeline *QSSGRhiContext::pipeline(const QSSGGraphicsPipelineStateKey &key,
                                               QRhiRenderPassDescriptor *rpDesc,
                                               QRhiShaderResourceBindings *srb)
{
    auto it = m_pipelines.constFind(key);
    if (it != m_pipelines.constEnd())
        return it.value();

    // Build a new one. This is potentially expensive.
    QRhiGraphicsPipeline *ps = m_rhi->newGraphicsPipeline();

    ps->setShaderStages(key.state.shaderPipeline->cbeginStages(), key.state.shaderPipeline->cendStages());
    ps->setVertexInputLayout(key.state.ia.inputLayout);
    ps->setShaderResourceBindings(srb);
    ps->setRenderPassDescriptor(rpDesc);

    QRhiGraphicsPipeline::Flags flags;
    if (key.state.scissorEnable)
        flags |= QRhiGraphicsPipeline::UsesScissor;

    static const bool shaderDebugInfo = qEnvironmentVariableIntValue("QT_QUICK3D_SHADER_DEBUG_INFO");
    if (shaderDebugInfo)
        flags |= QRhiGraphicsPipeline::CompileShadersWithDebugInfo;
    ps->setFlags(flags);

    ps->setTopology(key.state.ia.topology);
    ps->setCullMode(key.state.cullMode);
    if (key.state.ia.topology == QRhiGraphicsPipeline::Lines || key.state.ia.topology == QRhiGraphicsPipeline::LineStrip)
        ps->setLineWidth(key.state.lineWidth);

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
    ps->setPolygonMode(key.state.polygonMode);

    if (key.state.usesStencilRef)
        flags |= QRhiGraphicsPipeline::UsesStencilRef;
    ps->setFlags(flags);
    ps->setStencilFront(key.state.stencilOpFrontState);
    ps->setStencilTest(key.state.usesStencilRef);
    ps->setStencilWriteMask(key.state.stencilWriteMask);

    if (!ps->create()) {
        qWarning("Failed to build graphics pipeline state");
        delete ps;
        return nullptr;
    }

    m_pipelines.insert(key, ps);
    return ps;
}

QRhiComputePipeline *QSSGRhiContext::computePipeline(const QSSGComputePipelineStateKey &key,
                                                     QRhiShaderResourceBindings *srb)
{
    auto it = m_computePipelines.constFind(key);
    if (it != m_computePipelines.constEnd())
        return it.value();

    QRhiComputePipeline *computePipeline = m_rhi->newComputePipeline();
    computePipeline->setShaderResourceBindings(srb);
    computePipeline->setShaderStage({ QRhiShaderStage::Compute, key.shader });
    if (!computePipeline->create()) {
        qWarning("Failed to build compute pipeline");
        delete computePipeline;
        return nullptr;
    }
    m_computePipelines.insert(key, computePipeline);
    return computePipeline;
}

using SamplerInfo = QPair<QSSGRhiSamplerDescription, QRhiSampler*>;

QRhiSampler *QSSGRhiContext::sampler(const QSSGRhiSamplerDescription &samplerDescription)
{
    auto compareSampler = [samplerDescription](const SamplerInfo &info){ return info.first == samplerDescription; };
    const auto found = std::find_if(m_samplers.cbegin(), m_samplers.cend(), compareSampler);
    if (found != m_samplers.cend())
        return found->second;

    QRhiSampler *newSampler = m_rhi->newSampler(samplerDescription.magFilter,
                                                samplerDescription.minFilter,
                                                samplerDescription.mipmap,
                                                samplerDescription.hTiling,
                                                samplerDescription.vTiling,
                                                samplerDescription.zTiling);
    if (!newSampler->create()) {
        qWarning("Failed to build image sampler");
        delete newSampler;
        return nullptr;
    }
    m_samplers << SamplerInfo{samplerDescription, newSampler};
    return newSampler;
}

void QSSGRhiContext::checkAndAdjustForNPoT(QRhiTexture *texture, QSSGRhiSamplerDescription *samplerDescription)
{
    if (samplerDescription->mipmap != QRhiSampler::None
            || samplerDescription->hTiling != QRhiSampler::ClampToEdge
            || samplerDescription->vTiling != QRhiSampler::ClampToEdge
            || samplerDescription->zTiling != QRhiSampler::ClampToEdge)
    {
        if (m_rhi->isFeatureSupported(QRhi::NPOTTextureRepeat))
            return;

        const QSize pixelSize = texture->pixelSize();
        const int w = qNextPowerOfTwo(pixelSize.width() - 1);
        const int h = qNextPowerOfTwo(pixelSize.height() - 1);
        if (w != pixelSize.width() || h != pixelSize.height()) {
            static bool warnShown = false;
            if (!warnShown) {
                warnShown = true;
                qWarning("Attempted to use an unsupported filtering or wrap mode, "
                         "this is likely due to lacking proper support for non-power-of-two textures on this platform.\n"
                         "If this is with WebGL, try updating the application to use QQuick3D::idealSurfaceFormat() in main() "
                         "in order to ensure WebGL 2 is used.");
            }
            samplerDescription->mipmap = QRhiSampler::None;
            samplerDescription->hTiling = QRhiSampler::ClampToEdge;
            samplerDescription->vTiling = QRhiSampler::ClampToEdge;
            samplerDescription->zTiling = QRhiSampler::ClampToEdge;
        }
    }
}

void QSSGRhiContext::registerTexture(QRhiTexture *texture)
{
    m_textures.insert(texture);
}

void QSSGRhiContext::releaseTexture(QRhiTexture *texture)
{
    m_textures.remove(texture);
    delete texture;
}

void QSSGRhiContext::registerMesh(QSSGRenderMesh *mesh)
{
    m_meshes.insert(mesh);
}

void QSSGRhiContext::releaseMesh(QSSGRenderMesh *mesh)
{
    if (mesh) {
        for (const auto &subset : std::as_const(mesh->subsets)) {
            if (subset.rhi.targetsTexture)
                releaseTexture(subset.rhi.targetsTexture);
        }
    }
    m_meshes.remove(mesh);
    delete mesh;
}

void QSSGRhiContext::cleanupDrawCallData(const QSSGRenderModel *model)
{
    // Find all QSSGRhiUniformBufferSet that reference model
    // and delete them
    const void *modelNode = model;
    auto it = m_drawCallData.begin();
    while (it != m_drawCallData.end()) {
        if (it.key().model == modelNode) {
            releaseDrawCallData(*it);
            it = m_drawCallData.erase(it);
        } else {
            ++it;
        }
    }
}

QRhiTexture *QSSGRhiContext::dummyTexture(QRhiTexture::Flags flags, QRhiResourceUpdateBatch *rub,
                                          const QSize &size, const QColor &fillColor)
{
    auto it = m_dummyTextures.constFind({flags, size, fillColor});
    if (it != m_dummyTextures.constEnd())
        return *it;

    QRhiTexture *t = m_rhi->newTexture(QRhiTexture::RGBA8, size, 1, flags);
    if (t->create()) {
        QImage image(t->pixelSize(), QImage::Format_RGBA8888);
        image.fill(fillColor);
        rub->uploadTexture(t, image);
    } else {
        qWarning("Failed to build dummy texture");
    }

    m_dummyTextures.insert({flags, size, fillColor}, t);
    return t;
}

bool QSSGRhiContext::shaderDebuggingEnabled()
{
    static const bool isSet = (qEnvironmentVariableIntValue("QT_RHI_SHADER_DEBUG") != 0);
    return isSet;
}

bool QSSGRhiContext::editorMode()
{
    static const bool isSet = (qEnvironmentVariableIntValue("QT_QUICK3D_EDITORMODE") != 0);
    return isSet;
}

void QSSGRhiContextStats::start(QSSGRenderLayer *layer)
{
    layerKey = layer;
    PerLayerInfo &info(perLayerInfo[layerKey]);
    info.renderPasses.clear();
    info.externalRenderPass = {};
    info.currentRenderPassIndex = -1;
}

void QSSGRhiContextStats::stop(QSSGRenderLayer *layer)
{
    if (rendererDebugEnabled()) {
        PerLayerInfo &info(perLayerInfo[layer]);
        const int rpCount = info.renderPasses.size();
        qDebug("%d render passes in 3D renderer %p", rpCount, layer);
        for (int i = 0; i < rpCount; ++i) {
            const RenderPassInfo &rp(info.renderPasses[i]);
            qDebug("Render pass %d: rt name='%s' target size %dx%d pixels",
                   i, rp.rtName.constData(), rp.pixelSize.width(), rp.pixelSize.height());
            printRenderPass(rp);
        }
        if (info.externalRenderPass.indexedDraws.callCount || info.externalRenderPass.instancedIndexedDraws.callCount
                || info.externalRenderPass.draws.callCount || info.externalRenderPass.instancedDraws.callCount)
        {
            qDebug("Within external render passes:");
            printRenderPass(info.externalRenderPass);
        }
    }

    // a new start() may preceed stop() for the previous View3D, must handle this gracefully
    if (layerKey == layer)
        layerKey = nullptr;

    // The data must stay valid until the next start() with the same key, the
    // QQuick3DRenderStats and DebugView may read it.
}

void QSSGRhiContextStats::cleanupLayerInfo(QSSGRenderLayer *layer)
{
    perLayerInfo.remove(layer);
    dynamicDataSources.remove(layer);
}

void QSSGRhiContextStats::beginRenderPass(QRhiTextureRenderTarget *rt)
{
    PerLayerInfo &info(perLayerInfo[layerKey]);
    Q_TRACE(QSSG_renderPass_entry, QString::fromUtf8(rt->name()));
    info.renderPasses.append({ rt->name(), rt->pixelSize(), {}, {}, {}, {} });
    info.currentRenderPassIndex = info.renderPasses.size() - 1;
}

void QSSGRhiContextStats::endRenderPass()
{
    Q_TRACE(QSSG_renderPass_exit);
    PerLayerInfo &info(perLayerInfo[layerKey]);
    info.currentRenderPassIndex = -1;
}

bool QSSGRhiContextStats::isEnabled() const
{
    return !dynamicDataSources.isEmpty() || profilingEnabled() || rendererDebugEnabled()
            || Q_TRACE_ENABLED(QSSG_draw);
}

void QSSGRhiContextStats::drawIndexed(quint32 indexCount, quint32 instanceCount)
{
    Q_TRACE(QSSG_drawIndexed, indexCount, instanceCount);
    PerLayerInfo &info(perLayerInfo[layerKey]);
    RenderPassInfo &rp(info.currentRenderPassIndex >= 0 ? info.renderPasses[info.currentRenderPassIndex] : info.externalRenderPass);
    if (instanceCount > 1) {
        rp.instancedIndexedDraws.callCount += 1;
        rp.instancedIndexedDraws.vertexOrIndexCount += indexCount;
        rp.instancedIndexedDraws.instanceCount += instanceCount;
    } else {
        rp.indexedDraws.callCount += 1;
        rp.indexedDraws.vertexOrIndexCount += indexCount;
    }
}

void QSSGRhiContextStats::draw(quint32 vertexCount, quint32 instanceCount)
{
    Q_TRACE(QSSG_draw, vertexCount, instanceCount);
    PerLayerInfo &info(perLayerInfo[layerKey]);
    RenderPassInfo &rp(info.currentRenderPassIndex >= 0 ? info.renderPasses[info.currentRenderPassIndex] : info.externalRenderPass);
    if (instanceCount > 1) {
        rp.instancedDraws.callCount += 1;
        rp.instancedDraws.vertexOrIndexCount += vertexCount;
        rp.instancedDraws.instanceCount += instanceCount;
    } else {
        rp.draws.callCount += 1;
        rp.draws.vertexOrIndexCount += vertexCount;
    }
}

void QSSGRhiContextStats::printRenderPass(const QSSGRhiContextStats::RenderPassInfo &rp)
{
    qDebug("%llu indexed draw calls with %llu indices in total, "
           "%llu non-indexed draw calls with %llu vertices in total",
           rp.indexedDraws.callCount, rp.indexedDraws.vertexOrIndexCount,
           rp.draws.callCount, rp.draws.vertexOrIndexCount);
    if (rp.instancedIndexedDraws.callCount || rp.instancedDraws.callCount) {
        qDebug("%llu instanced indexed draw calls with %llu indices and %llu instances in total, "
               "%llu instanced non-indexed draw calls with %llu indices and %llu instances in total",
               rp.instancedIndexedDraws.callCount, rp.instancedIndexedDraws.vertexOrIndexCount, rp.instancedIndexedDraws.instanceCount,
               rp.instancedDraws.callCount, rp.instancedDraws.vertexOrIndexCount, rp.instancedDraws.instanceCount);
    }
}

QT_END_NAMESPACE
