// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "qssggltfaccessorreader_p.h"

#include <cstring>

QT_BEGIN_NAMESPACE

using namespace QSSGGltf;

/*!
    \namespace QSSGGltfAccessorReader
    \internal

    Typed access to glTF accessor data. All functions compose the buffer view
    byte stride (interleaved data), the accessor byte offset, and sparse
    substitution, and return tightly packed copies. An accessor without a
    buffer view yields zeros, as the specification requires.
*/
namespace QSSGGltfAccessorReader {

namespace {

// Pointer to the first element and the stride between elements for data in
// a buffer view; null when the accessor has no buffer view.
struct DataView {
    const char *data = nullptr;
    qsizetype stride = 0;
};

DataView viewForAccessor(const QSSGGltfDocument &document, const Accessor &accessor)
{
    if (accessor.bufferView < 0)
        return {};
    const BufferView &view = document.bufferViews.at(accessor.bufferView);
    const Buffer &buffer = document.buffers.at(view.buffer);
    DataView result;
    result.stride = view.byteStride > 0 ? view.byteStride : accessor.elementByteSize();
    result.data = buffer.data.constData() + view.byteOffset + accessor.byteOffset;
    return result;
}

quint32 readIndexValue(const char *data, Accessor::ComponentType componentType)
{
    switch (componentType) {
    case Accessor::ComponentType::UnsignedByte:
        return *reinterpret_cast<const quint8 *>(data);
    case Accessor::ComponentType::UnsignedShort: {
        quint16 value;
        std::memcpy(&value, data, sizeof(value));
        return value;
    }
    case Accessor::ComponentType::UnsignedInt: {
        quint32 value;
        std::memcpy(&value, data, sizeof(value));
        return value;
    }
    default:
        break;
    }
    return 0;
}

void applySparse(const QSSGGltfDocument &document, const Accessor &accessor, QByteArray &packed)
{
    const Accessor::Sparse &sparse = *accessor.sparse;
    const qsizetype elementSize = accessor.elementByteSize();

    const BufferView &indicesView = document.bufferViews.at(sparse.indicesBufferView);
    const Buffer &indicesBuffer = document.buffers.at(indicesView.buffer);
    const char *indices = indicesBuffer.data.constData() + indicesView.byteOffset + sparse.indicesByteOffset;
    const qsizetype indexSize = Accessor::componentByteSize(sparse.indicesComponentType);

    const BufferView &valuesView = document.bufferViews.at(sparse.valuesBufferView);
    const Buffer &valuesBuffer = document.buffers.at(valuesView.buffer);
    const char *values = valuesBuffer.data.constData() + valuesView.byteOffset + sparse.valuesByteOffset;

    for (qint64 i = 0; i < sparse.count; ++i) {
        const quint32 targetElement = readIndexValue(indices + i * indexSize, sparse.indicesComponentType);
        if (qint64(targetElement) >= accessor.count)
            continue; // structurally invalid substitution index; skip
        std::memcpy(packed.data() + qsizetype(targetElement) * elementSize, values + i * elementSize, elementSize);
    }
}

float componentToFloat(const char *data, Accessor::ComponentType componentType, bool normalized)
{
    switch (componentType) {
    case Accessor::ComponentType::Byte: {
        const qint8 value = *reinterpret_cast<const qint8 *>(data);
        return normalized ? std::max(value / 127.0f, -1.0f) : float(value);
    }
    case Accessor::ComponentType::UnsignedByte: {
        const quint8 value = *reinterpret_cast<const quint8 *>(data);
        return normalized ? value / 255.0f : float(value);
    }
    case Accessor::ComponentType::Short: {
        qint16 value;
        std::memcpy(&value, data, sizeof(value));
        return normalized ? std::max(value / 32767.0f, -1.0f) : float(value);
    }
    case Accessor::ComponentType::UnsignedShort: {
        quint16 value;
        std::memcpy(&value, data, sizeof(value));
        return normalized ? value / 65535.0f : float(value);
    }
    case Accessor::ComponentType::UnsignedInt: {
        quint32 value;
        std::memcpy(&value, data, sizeof(value));
        return float(value);
    }
    case Accessor::ComponentType::Float: {
        float value;
        std::memcpy(&value, data, sizeof(value));
        return value;
    }
    }
    return 0.0f;
}

} // namespace

/*!
    \internal

    Returns the element data of accessor \a accessorIndex in \a document,
    packed to accessor.elementByteSize() per element.
*/
QByteArray readPacked(const QSSGGltfDocument &document, int accessorIndex)
{
    if (accessorIndex < 0 || accessorIndex >= document.accessors.size())
        return {};
    const Accessor &accessor = document.accessors.at(accessorIndex);
    const qsizetype elementSize = accessor.elementByteSize();

    QByteArray packed(accessor.count * elementSize, '\0');
    const DataView view = viewForAccessor(document, accessor);
    if (view.data) {
        if (view.stride == elementSize) {
            std::memcpy(packed.data(), view.data, packed.size());
        } else {
            // Bound the loop by what was actually allocated rather than by
            // accessor.count, so that this stays inside the destination even
            // if the count and the allocation ever disagree
            const qint64 count = packed.size() / elementSize;
            for (qint64 i = 0; i < count; ++i)
                std::memcpy(packed.data() + i * elementSize, view.data + i * view.stride, elementSize);
        }
    }

    if (accessor.sparse.has_value())
        applySparse(document, accessor, packed);

    return packed;
}

/*!
    \internal

    Returns the element data of accessor \a accessorIndex in \a document,
    converted to floats, accessor.componentCount() per element. Normalized
    integer component types are dequantized as the specification describes
    (c/255, max(c/127, -1), ...), which also covers accessors quantized per
    KHR_mesh_quantization; non-normalized integers are cast.
*/
QList<float> readAsFloats(const QSSGGltfDocument &document, int accessorIndex)
{
    if (accessorIndex < 0 || accessorIndex >= document.accessors.size())
        return {};
    const Accessor &accessor = document.accessors.at(accessorIndex);
    const QByteArray packed = readPacked(document, accessorIndex);
    const int components = Accessor::componentCount(accessor.type);
    const int componentSize = Accessor::componentByteSize(accessor.componentType);

    QList<float> result(accessor.count * components);
    const char *element = packed.constData();
    for (qsizetype i = 0; i < result.size(); ++i)
        result[i] = componentToFloat(element + i * componentSize, accessor.componentType, accessor.normalized);
    return result;
}

/*!
    \internal

    Returns the scalar index data of accessor \a accessorIndex in \a document,
    widened to 32-bit unsigned integers.
*/
QList<quint32> readIndices(const QSSGGltfDocument &document, int accessorIndex)
{
    if (accessorIndex < 0 || accessorIndex >= document.accessors.size())
        return {};
    const Accessor &accessor = document.accessors.at(accessorIndex);
    const QByteArray packed = readPacked(document, accessorIndex);
    const qsizetype componentSize = Accessor::componentByteSize(accessor.componentType);

    QList<quint32> result(accessor.count);
    for (qint64 i = 0; i < accessor.count; ++i)
        result[i] = readIndexValue(packed.constData() + i * componentSize, accessor.componentType);
    return result;
}

} // namespace QSSGGltfAccessorReader

QT_END_NAMESPACE
