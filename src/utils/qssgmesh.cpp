// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser


#include "qssgmesh_p.h"

#include <QtCore/QVector>
#include <QtCore/qhash.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/qmath.h>
#include <QtQuick3DUtils/private/qssgdataref_p.h>
#include <QtQuick3DUtils/private/qssglightmapuvgenerator_p.h>
#include <QtQuick3DUtils/private/qssgutils_p.h>

#include "meshoptimizer.h"

#include <algorithm>

QT_BEGIN_NAMESPACE

namespace QSSGMesh {

// fileId, fileVersion, offset, count
static const size_t MULTI_HEADER_STRUCT_SIZE = 16;

// meshOffset, meshId, padding
static const size_t MULTI_ENTRY_STRUCT_SIZE = 16;

// fileId, fileVersion, flags, size
static const size_t MESH_HEADER_STRUCT_SIZE = 12;

// vertexBuffer, indexBuffer, subsets, joints, drawMode, winding
static const size_t MESH_STRUCT_SIZE = 56;

// vertex buffer entry list: nameOffset, componentType, componentCount, offset
static const size_t VERTEX_BUFFER_ENTRY_STRUCT_SIZE = 16;

// subset list: count, offset, minXYZ, maxXYZ, nameOffset, nameLength
static const size_t SUBSET_STRUCT_SIZE_V3_V4 = 40;
// subset list: count, offset, minXYZ, maxXYZ, nameOffset, nameLength, lightmapSizeWidth, lightmapSizeHeight
static const size_t SUBSET_STRUCT_SIZE_V5 = 48;
// subset list: count, offset, minXYZ, maxXYZ, nameOffset, nameLength, lightmapSizeWidth, lightmapSizeHeight, lodCount
static const size_t SUBSET_STRUCT_SIZE_V6 = 52;

//lod entry: count, offset, distance
static const size_t LOD_STRUCT_SIZE = 12;

// joint list: jointID, parentID, invBindPose, localToGlobalBoneSpace
static const size_t JOINT_STRUCT_SIZE = 136;

// getSizeOfType() is Q_UNREACHABLE outside its enumeration, so a component type
// from the file has to be checked before it gets there.
// Spelled out so that a new component type fails to compile until handled.
static bool isKnownComponentType(quint32 value)
{
    switch (Mesh::ComponentType(value)) {
    case Mesh::ComponentType::UnsignedInt8:
    case Mesh::ComponentType::Int8:
    case Mesh::ComponentType::UnsignedInt16:
    case Mesh::ComponentType::Int16:
    case Mesh::ComponentType::UnsignedInt32:
    case Mesh::ComponentType::Int32:
    case Mesh::ComponentType::UnsignedInt64:
    case Mesh::ComponentType::Int64:
    case Mesh::ComponentType::Float16:
    case Mesh::ComponentType::Float32:
    case Mesh::ComponentType::Float64:
        return true;
    }
    return false;
}

// Bounds a count by the file rather than by a chosen cap, so the limit scales
// with the file and cannot be set wrong.
static bool canRead(QIODevice *device, quint64 count, quint64 itemSize)
{
    quint64 bytes = 0;
    if (qMulOverflow(count, itemSize, &bytes))
        return false;
    const qint64 remaining = device->size() - device->pos();
    return remaining >= 0 && bytes <= quint64(remaining);
}

MeshInternal::MultiMeshInfo MeshInternal::readFileHeader(QIODevice *device)
{
    const qint64 multiHeaderStartOffset = device->size() - qint64(MULTI_HEADER_STRUCT_SIZE);
    if (multiHeaderStartOffset < 0) {
        qWarning("Mesh file is too small to hold a header");
        return {};
    }

    device->seek(multiHeaderStartOffset);
    QDataStream inputStream(device);
    inputStream.setByteOrder(QDataStream::LittleEndian);
    inputStream.setFloatingPointPrecision(QDataStream::SinglePrecision);

    MultiMeshInfo meshFileInfo;
    inputStream >> meshFileInfo.fileId >> meshFileInfo.fileVersion;

    if (!meshFileInfo.isValid()) {
        qWarning("Mesh file invalid");
        return {};
    }

    quint32 multiEntriesOffset; // unused, the entry list is right before the header
    quint32 meshCount;
    inputStream >> multiEntriesOffset >> meshCount;

    // The entry list sits immediately before the header, so it bounds the count.
    if (quint64(meshCount) * MULTI_ENTRY_STRUCT_SIZE > quint64(multiHeaderStartOffset)) {
        qWarning("Mesh file declares %u meshes, which do not fit in it", meshCount);
        return {};
    }

    for (quint32 i = 0; i < meshCount; ++i) {
        device->seek(multiHeaderStartOffset
                     - (qint64(MULTI_ENTRY_STRUCT_SIZE) * meshCount)
                     + (qint64(MULTI_ENTRY_STRUCT_SIZE) * i));
        quint64 offset;
        quint32 id;
        inputStream >> offset >> id;
        meshFileInfo.meshEntries.insert(id, offset);
    }

    return meshFileInfo;
}

void MeshInternal::writeFileHeader(QIODevice *device, const MeshInternal::MultiMeshInfo &meshFileInfo)
{
    QDataStream outputStream(device);
    outputStream.setByteOrder(QDataStream::LittleEndian);
    outputStream.setFloatingPointPrecision(QDataStream::SinglePrecision);

    const quint32 multiEntriesOffset = device->pos();
    for (auto it = meshFileInfo.meshEntries.cbegin(), end = meshFileInfo.meshEntries.cend(); it != end; ++it) {
        const quint32 id = it.key();
        const quint64 offset = it.value();
        const quint32 padding = 0;
        outputStream << offset << id << padding;
    }

    const quint32 meshCount = meshFileInfo.meshEntries.size();
    outputStream << meshFileInfo.fileId << meshFileInfo.fileVersion << multiEntriesOffset << meshCount;
}

quint64 MeshInternal::readMeshData(QIODevice *device, quint64 offset, Mesh *mesh, MeshDataHeader *header)
{
    static char alignPadding[4] = {};

    device->seek(offset);
    QDataStream inputStream(device);
    inputStream.setByteOrder(QDataStream::LittleEndian);
    inputStream.setFloatingPointPrecision(QDataStream::SinglePrecision);

    inputStream >> header->fileId >> header->fileVersion >> header->flags >> header->sizeInBytes;
    if (!header->isValid()) {
        qWarning() << "Mesh data invalid";
        if (header->fileId == MeshDataHeader::FILE_ID) {
            if (header->fileVersion > MeshDataHeader::FILE_VERSION)
                qWarning() << "File version " << header->fileVersion << " newer than " << MeshDataHeader::FILE_VERSION;
            if (header->fileVersion < MeshDataHeader::LEGACY_MESH_FILE_VERSION)
                qWarning() << "File version " << header->fileVersion << " older than " << MeshDataHeader::LEGACY_MESH_FILE_VERSION;
        } else {
            qWarning() << "Invalid file ID" << header->fileId;
        }
        return 0;
    }

    MeshInternal::MeshOffsetTracker offsetTracker(offset + MESH_HEADER_STRUCT_SIZE);
    Q_ASSERT(offsetTracker.offset() == device->pos());

    quint32 targetBufferEntriesCount;
    quint32 vertexBufferEntriesCount;
    quint32 targetBufferDataSize;
    quint32 vertexBufferDataSize;
    inputStream >> targetBufferEntriesCount
                >> vertexBufferEntriesCount
                >> mesh->m_vertexBuffer.stride
                >> targetBufferDataSize
                >> vertexBufferDataSize;

    if (!header->hasSeparateTargetBuffer()) {
        targetBufferEntriesCount = 0;
        targetBufferDataSize = 0;
    }

    quint32 indexBufferComponentType;
    quint32 indexBufferDataOffset;
    quint32 indexBufferDataSize;
    inputStream >> indexBufferComponentType
                >> indexBufferDataOffset
                >> indexBufferDataSize;
    if (!isKnownComponentType(indexBufferComponentType)) {
        qWarning("Mesh index buffer has unknown component type %u", indexBufferComponentType);
        return 0;
    }
    mesh->m_indexBuffer.componentType = Mesh::ComponentType(indexBufferComponentType);

    quint32 targetCount;
    quint32 subsetsCount;
    inputStream >> targetCount >> subsetsCount;
    mesh->m_targetBuffer.numTargets = targetCount;

    quint32 jointsOffsets; // unused, see the format documentation
    quint32 jointsCount;
    inputStream >> jointsOffsets >> jointsCount;
    quint32 drawMode;
    quint32 winding;
    inputStream >> drawMode >> winding;
    mesh->m_drawMode = Mesh::DrawMode(drawMode);
    mesh->m_winding = Mesh::Winding(winding);

    offsetTracker.advance(MESH_STRUCT_SIZE);

    if (!canRead(device, vertexBufferEntriesCount, VERTEX_BUFFER_ENTRY_STRUCT_SIZE)) {
        qWarning("Mesh declares %u vertex buffer entries, which do not fit in the file",
                 vertexBufferEntriesCount);
        return 0;
    }

    quint32 entriesByteSize = 0;
    for (quint32 i = 0; i < vertexBufferEntriesCount; ++i) {
        Mesh::VertexBufferEntry vertexBufferEntry;
        quint32 componentType;
        quint32 nameOffset; // unused
        inputStream >> nameOffset
                    >> componentType
                    >> vertexBufferEntry.componentCount
                    >> vertexBufferEntry.offset;
        if (!isKnownComponentType(componentType)) {
            qWarning("Mesh attribute %u has unknown component type %u", i, componentType);
            return 0;
        }
        vertexBufferEntry.componentType = Mesh::ComponentType(componentType);
        mesh->m_vertexBuffer.entries.append(vertexBufferEntry);
        entriesByteSize += VERTEX_BUFFER_ENTRY_STRUCT_SIZE;
    }
    quint32 alignAmount = offsetTracker.alignedAdvance(entriesByteSize);
    if (alignAmount)
        device->read(alignPadding, alignAmount);

    // vertex buffer entry names
    quint32 numTargets = 0;
    // used for recording the target attributes supported by the mesh
    // and re-construting it when meeting attr_unsupported
    QList<QByteArray> attrNames;
    for (auto &entry : mesh->m_vertexBuffer.entries) {
        quint32 nameLength;
        inputStream >> nameLength;
        offsetTracker.advance(sizeof(quint32));
        if (!canRead(device, nameLength, 1)) {
            qWarning("Mesh declares a %u byte attribute name, which does not fit in the file",
                     nameLength);
            return 0;
        }
        const QByteArray nameWithZeroTerminator = device->read(nameLength);
        entry.name = QByteArray(nameWithZeroTerminator.constData(), qMax(0, nameWithZeroTerminator.size() - 1));
        alignAmount = offsetTracker.alignedAdvance(nameLength);
        if (alignAmount)
            device->read(alignPadding, alignAmount);
        // Old morph meshes' target attributes were appended sequentially
        // behind vertex attributes. However, since the number of targets are restricted by 8
        // the other attributes were named by "attr_unsupported"
        // So just checking numTargets is safe with the above assumption and
        // it will try to reconstruct the unsupported attributes.
        if (numTargets > 0 || (!header->hasSeparateTargetBuffer() && entry.name.startsWith("attr_t"))) {
            // Any later name lands here once a target has been seen, including
            // short ones, and sliced() asserts where mid() would not.
            const QByteArray suffix = entry.name.size() > 6 ? entry.name.sliced(6) : QByteArray();
            if (suffix.startsWith("pos")) {
                const quint32 targetId = entry.name.mid(9).toUInt();
                // All the attributes of the first target should be recorded correctly.
                if (targetId == 0)
                    attrNames.append(MeshInternal::getPositionAttrName());
                numTargets = qMax(numTargets, targetId + 1);
                entry.name = MeshInternal::getPositionAttrName();
                mesh->m_targetBuffer.entries.append(entry);
                targetBufferEntriesCount++;
            } else if (suffix.startsWith("norm")) {
                const quint32 targetId = entry.name.mid(10).toUInt();
                if (targetId == 0)
                    attrNames.append(MeshInternal::getNormalAttrName());
                numTargets = qMax(numTargets, targetId + 1);
                entry.name = MeshInternal::getNormalAttrName();
                mesh->m_targetBuffer.entries.append(entry);
                targetBufferEntriesCount++;
            } else if (suffix.startsWith("tan")) {
                const quint32 targetId = entry.name.mid(9).toUInt();
                if (targetId == 0)
                    attrNames.append(MeshInternal::getTexTanAttrName());
                numTargets = qMax(numTargets, targetId + 1);
                entry.name = MeshInternal::getTexTanAttrName();
                mesh->m_targetBuffer.entries.append(entry);
                targetBufferEntriesCount++;
            } else if (suffix.startsWith("binorm")) {
                const quint32 targetId = entry.name.mid(12).toUInt();
                if (targetId == 0)
                    attrNames.append(MeshInternal::getTexBinormalAttrName());
                numTargets = qMax(numTargets, targetId + 1);
                entry.name = MeshInternal::getTexBinormalAttrName();
                mesh->m_targetBuffer.entries.append(entry);
                targetBufferEntriesCount++;
            } else if (entry.name.startsWith("attr_unsupported")) {
                // Reconstruct, if target 0 recorded a layout to do it from.
                if (attrNames.isEmpty()) {
                    qWarning("Mesh has an unsupported morph target attribute with no target 0 to "
                             "take the layout from");
                    return 0;
                }
                entry.name = attrNames[targetBufferEntriesCount % attrNames.size()];
                mesh->m_targetBuffer.entries.append(entry);
                targetBufferEntriesCount++;
            }
        }
    }

    if (!canRead(device, vertexBufferDataSize, 1)) {
        qWarning("Mesh declares %u bytes of vertex data, which do not fit in the file",
                 vertexBufferDataSize);
        return 0;
    }
    mesh->m_vertexBuffer.data = device->read(vertexBufferDataSize);
    alignAmount = offsetTracker.alignedAdvance(vertexBufferDataSize);
    if (alignAmount)
        device->read(alignPadding, alignAmount);

    if (!canRead(device, indexBufferDataSize, 1)) {
        qWarning("Mesh declares %u bytes of index data, which do not fit in the file",
                 indexBufferDataSize);
        return 0;
    }
    mesh->m_indexBuffer.data = device->read(indexBufferDataSize);
    alignAmount = offsetTracker.alignedAdvance(indexBufferDataSize);
    if (alignAmount)
        device->read(alignPadding, alignAmount);

    const size_t subsetStructSize = header->hasLodDataHint() ? SUBSET_STRUCT_SIZE_V6
            : header->hasLightmapSizeHint()                   ? SUBSET_STRUCT_SIZE_V5
                                                              : SUBSET_STRUCT_SIZE_V3_V4;
    if (!canRead(device, subsetsCount, subsetStructSize)) {
        qWarning("Mesh declares %u subsets, which do not fit in the file", subsetsCount);
        return 0;
    }

    quint32 subsetByteSize = 0;
    QVector<MeshInternal::Subset> internalSubsets;
    for (quint32 i = 0; i < subsetsCount; ++i) {
        MeshInternal::Subset subset;
        float minX;
        float minY;
        float minZ;
        float maxX;
        float maxY;
        float maxZ;
        quint32 nameOffset; // unused
        inputStream >> subset.count
                    >> subset.offset
                    >> minX
                    >> minY
                    >> minZ
                    >> maxX
                    >> maxY
                    >> maxZ
                    >> nameOffset
                    >> subset.nameLength;
        subset.bounds.min = QVector3D(minX, minY, minZ);
        subset.bounds.max = QVector3D(maxX, maxY, maxZ);
        if (header->hasLightmapSizeHint()) {
            quint32 width = 0;
            quint32 height = 0;
            inputStream >> width >> height;
            subset.lightmapSizeHint = QSize(width, height);
            if (header->hasLodDataHint()) {
                quint32 lodCount = 0;
                inputStream >> lodCount;
                subset.lodCount = lodCount;
                subsetByteSize += SUBSET_STRUCT_SIZE_V6;
            } else {
                subsetByteSize += SUBSET_STRUCT_SIZE_V5;
            }
        } else {
            subset.lightmapSizeHint = QSize(0, 0);
            subsetByteSize += SUBSET_STRUCT_SIZE_V3_V4;
        }
        internalSubsets.append(subset);

    }
    alignAmount = offsetTracker.alignedAdvance(subsetByteSize);
    if (alignAmount)
        device->read(alignPadding, alignAmount);

    for (MeshInternal::Subset &internalSubset : internalSubsets) {
        if (!canRead(device, internalSubset.nameLength, 2)) {
            qWarning("Mesh subset declares a %u character name, which does not fit in the file",
                     internalSubset.nameLength);
            return 0;
        }
        internalSubset.rawNameUtf16 = device->read(qint64(internalSubset.nameLength) * 2);
        alignAmount = offsetTracker.alignedAdvance(internalSubset.nameLength * 2);
        if (alignAmount)
            device->read(alignPadding, alignAmount);
    }

    quint32 lodByteSize = 0;
    for (const MeshInternal::Subset &internalSubset : internalSubsets) {
        // The subsets share these bytes, so check them as they are consumed.
        if (!canRead(device, internalSubset.lodCount, LOD_STRUCT_SIZE)) {
            qWarning("Mesh subset declares %u levels of detail, which do not fit in the file",
                     internalSubset.lodCount);
            return 0;
        }
        auto meshSubset = internalSubset.toMeshSubset();
        // Read Level of Detail data here
        for (auto &lod : meshSubset.lods) {
            quint32 count = 0;
            quint32 offset = 0;
            float distance = 0.0;
            inputStream >> count >> offset >> distance;
            lod.count = count;
            lod.offset = offset;
            lod.distance = distance;
            lodByteSize += LOD_STRUCT_SIZE;
        }

        mesh->m_subsets.append(meshSubset);
    }
    alignAmount = offsetTracker.alignedAdvance(lodByteSize);
    if (alignAmount)
        device->read(alignPadding, alignAmount);

    // Unused, but the morph target entries come after them.
    if (jointsCount) {
        if (!canRead(device, jointsCount, JOINT_STRUCT_SIZE)) {
            qWarning("Mesh declares %u joints, which do not fit in the file", jointsCount);
            return 0;
        }
        const quint64 jointsByteSize = quint64(jointsCount) * JOINT_STRUCT_SIZE;
        device->seek(device->pos() + qint64(jointsByteSize));
        offsetTracker.advance(qint32(jointsByteSize));
    }

    // Data for morphTargets
    if (targetBufferEntriesCount > 0) {
        if (header->hasSeparateTargetBuffer()) {
            if (!canRead(device, targetBufferEntriesCount, VERTEX_BUFFER_ENTRY_STRUCT_SIZE)) {
                qWarning("Mesh declares %u morph target entries, which do not fit in the file",
                         targetBufferEntriesCount);
                return 0;
            }
            entriesByteSize = 0;
            for (quint32 i = 0; i < targetBufferEntriesCount; ++i) {
                Mesh::VertexBufferEntry targetBufferEntry;
                quint32 componentType;
                quint32 nameOffset; // unused
                inputStream >> nameOffset
                            >> componentType
                            >> targetBufferEntry.componentCount
                            >> targetBufferEntry.offset;
                if (!isKnownComponentType(componentType)) {
                    qWarning("Mesh morph target attribute %u has unknown component type %u", i,
                             componentType);
                    return 0;
                }
                targetBufferEntry.componentType = Mesh::ComponentType(componentType);
                mesh->m_targetBuffer.entries.append(targetBufferEntry);
                entriesByteSize += VERTEX_BUFFER_ENTRY_STRUCT_SIZE;
            }
            alignAmount = offsetTracker.alignedAdvance(entriesByteSize);
            if (alignAmount)
                device->read(alignPadding, alignAmount);

            for (auto &entry : mesh->m_targetBuffer.entries) {
                quint32 nameLength;
                inputStream >> nameLength;
                offsetTracker.advance(sizeof(quint32));
                if (!canRead(device, nameLength, 1)) {
                    qWarning("Mesh declares a %u byte morph target name, which does not fit in "
                             "the file", nameLength);
                    return 0;
                }
                const QByteArray nameWithZeroTerminator = device->read(nameLength);
                entry.name = QByteArray(nameWithZeroTerminator.constData(), qMax(0, nameWithZeroTerminator.size() - 1));
                alignAmount = offsetTracker.alignedAdvance(nameLength);
                if (alignAmount)
                    device->read(alignPadding, alignAmount);
            }

            if (!canRead(device, targetBufferDataSize, 1)) {
                qWarning("Mesh declares %u bytes of morph target data, which do not fit in the "
                         "file", targetBufferDataSize);
                return 0;
            }
            mesh->m_targetBuffer.data = device->read(targetBufferDataSize);
        } else {
            // The target buffer is sized by the product of these two, and the
            // layout has only position, normal, tangent and binormal per target.
            constexpr quint32 maxTargets = 8;
            constexpr quint32 maxComponentsPerTarget = 4;
            if (mesh->m_vertexBuffer.stride == 0 || numTargets == 0
                || numTargets > maxTargets
                || vertexBufferDataSize < mesh->m_vertexBuffer.stride
                || targetBufferEntriesCount > vertexBufferEntriesCount
                || targetBufferEntriesCount < numTargets
                || targetBufferEntriesCount > numTargets * maxComponentsPerTarget) {
                qWarning("Mesh morph target layout is inconsistent: stride %u, %u targets, %u "
                         "entries of %u",
                         mesh->m_vertexBuffer.stride, numTargets, targetBufferEntriesCount,
                         vertexBufferEntriesCount);
                return 0;
            }
            // remove target entries from vertexbuffer entries
            mesh->m_vertexBuffer.entries.remove(vertexBufferEntriesCount - targetBufferEntriesCount,
                                                targetBufferEntriesCount);
            // At least one vertex, so vertexCount - 1 below does not wrap.
            const quint32 vertexCount = vertexBufferDataSize / mesh->m_vertexBuffer.stride;
            // The least an entry can need, checked before sizing from vertexCount.
            if (qsizetype(vertexCount - 1) * qsizetype(mesh->m_vertexBuffer.stride)
                        + 3 * qsizetype(sizeof(float))
                > qsizetype(vertexBufferDataSize)) {
                qWarning("Mesh morph target source does not fit a %u byte vertex buffer of stride "
                         "%u", vertexBufferDataSize, mesh->m_vertexBuffer.stride);
                return 0;
            }
            const quint32 targetEntryTexWidth = qCeil(qSqrt(vertexCount));
            qsizetype targetCompStride = 0;
            qsizetype targetBufferSize = 0;
            if (qMulOverflow(qsizetype(targetEntryTexWidth), qsizetype(targetEntryTexWidth),
                             &targetCompStride)
                || qMulOverflow(targetCompStride, qsizetype(4 * sizeof(float)), &targetCompStride)
                || qMulOverflow(targetCompStride, qsizetype(targetBufferEntriesCount),
                                &targetBufferSize)) {
                qWarning("Mesh morph target buffer for %u vertices is too large", vertexCount);
                return 0;
            }
            mesh->m_targetBuffer.data.resize(targetBufferSize);
            const quint32 numComps = targetBufferEntriesCount / numTargets;
            for (quint32 i = 0; i < targetBufferEntriesCount; ++i) {
                auto &entry = mesh->m_targetBuffer.entries[i];
                const qsizetype dstOffset = qsizetype(i / numComps) * targetCompStride
                        + qsizetype(i % numComps) * targetCompStride * numTargets;
                // Three floats per vertex, so it is the last twelve byte copy
                // that has to fit.
                const qsizetype copyBytes = 3 * qsizetype(sizeof(float));
                const qsizetype dstNeeded = dstOffset
                        + qsizetype(vertexCount - 1) * qsizetype(4 * sizeof(float)) + copyBytes;
                const qsizetype srcNeeded = qsizetype(entry.offset)
                        + qsizetype(vertexCount - 1) * qsizetype(mesh->m_vertexBuffer.stride)
                        + copyBytes;
                if (dstOffset < 0 || dstNeeded > targetBufferSize
                    || srcNeeded > mesh->m_vertexBuffer.data.size()) {
                    qWarning("Mesh morph target entry %u does not fit its buffer", i);
                    return 0;
                }
                char *dstBuf = mesh->m_targetBuffer.data.data() + dstOffset;
                const char *srcBuf = mesh->m_vertexBuffer.data.constData() + entry.offset;
                for (quint32 j = 0; j < vertexCount; ++j) {
                    memcpy(dstBuf + j * 4 * sizeof(float),
                           srcBuf + j * mesh->m_vertexBuffer.stride,
                           3 * sizeof(float));
                }
                entry.offset = i * targetCompStride;
            }
            // now we don't need to have redundant targetbuffer entries
            mesh->m_targetBuffer.entries.remove(numComps, targetBufferEntriesCount - numComps);
            mesh->m_targetBuffer.numTargets = numTargets;
        }
    }

    // These go straight to drawIndexed() or draw(), so a range outside the
    // buffer it indexes has to be caught here rather than by the driver.
    const quint32 indexSize = byteSizeForComponentType(mesh->m_indexBuffer.componentType);
    quint64 drawableCount = 0;
    if (!mesh->m_indexBuffer.data.isEmpty())
        drawableCount = indexSize ? quint64(mesh->m_indexBuffer.data.size()) / indexSize : 0;
    else if (mesh->m_vertexBuffer.stride)
        drawableCount = quint64(mesh->m_vertexBuffer.data.size()) / mesh->m_vertexBuffer.stride;

    for (const Mesh::Subset &subset : std::as_const(mesh->m_subsets)) {
        if (quint64(subset.offset) + subset.count > drawableCount) {
            qWarning("Mesh subset draws %u from %u, but only %llu are available", subset.count,
                     subset.offset, qulonglong(drawableCount));
            return 0;
        }
        for (const Mesh::Lod &lod : subset.lods) {
            if (quint64(lod.offset) + lod.count > drawableCount) {
                qWarning("Mesh level of detail draws %u from %u, but only %llu are available",
                         lod.count, lod.offset, qulonglong(drawableCount));
                return 0;
            }
        }
    }

    return header->sizeInBytes;
}

void MeshInternal::writeMeshHeader(QIODevice *device, const MeshDataHeader &header)
{
    QDataStream outputStream(device);
    outputStream.setByteOrder(QDataStream::LittleEndian);
    outputStream.setFloatingPointPrecision(QDataStream::SinglePrecision);

    outputStream << header.fileId << header.fileVersion << header.flags << header.sizeInBytes;
}

// The legacy, now-removed, insane mesh code used to use a "serialization"
// strategy with dumping memory, yet combined with with an in-memory layout
// that is different from what's in the file. In version 4 we no longer write
// out valid offset values (see the // legacy offset comments), because the new
// loader does not need them, and calculating them is not sensible, especially
// due to the different layouts. We still do the alignment padding, even though
// that's also legacy nonsense, but having that allows the reader not have to
// branch based on the version.

quint64 MeshInternal::writeMeshData(QIODevice *device, const Mesh &mesh)
{
    static const char alignPadding[4] = {};

    QDataStream outputStream(device);
    outputStream.setByteOrder(QDataStream::LittleEndian);
    outputStream.setFloatingPointPrecision(QDataStream::SinglePrecision);

    const qint64 startPos = device->pos();
    MeshInternal::MeshOffsetTracker offsetTracker(startPos);
    Q_ASSERT(offsetTracker.offset() == device->pos());

    const quint32 vertexBufferEntriesCount = mesh.m_vertexBuffer.entries.size();
    const quint32 vertexBufferDataSize = mesh.m_vertexBuffer.data.size();
    const quint32 vertexBufferStride = mesh.m_vertexBuffer.stride;
    const quint32 targetBufferEntriesCount = mesh.m_targetBuffer.entries.count();
    const quint32 targetBufferDataSize = mesh.m_targetBuffer.data.size();
    outputStream << targetBufferEntriesCount
                 << vertexBufferEntriesCount
                 << vertexBufferStride;
    outputStream << targetBufferDataSize
                 << vertexBufferDataSize;

    const quint32 indexBufferDataSize = mesh.m_indexBuffer.data.size();
    const quint32 indexComponentType = quint32(mesh.m_indexBuffer.componentType);
    outputStream << indexComponentType;
    outputStream << quint32(0) // legacy offset
                 << indexBufferDataSize;

    const quint32 targetCount = mesh.m_targetBuffer.numTargets;
    const quint32 subsetsCount = mesh.m_subsets.size();
    outputStream << targetCount
                 << subsetsCount;

    outputStream << quint32(0) // legacy offset
                 << quint32(0); // legacy jointsCount

    const quint32 drawMode = quint32(mesh.m_drawMode);
    const quint32 winding = quint32(mesh.m_winding);
    outputStream << drawMode
                 << winding;

    offsetTracker.advance(MESH_STRUCT_SIZE);

    quint32 entriesByteSize = 0;
    for (quint32 i = 0; i < vertexBufferEntriesCount; ++i) {
        const Mesh::VertexBufferEntry &entry(mesh.m_vertexBuffer.entries[i]);
        const quint32 componentType = quint32(entry.componentType);
        const quint32 componentCount = entry.componentCount;
        const quint32 offset = entry.offset;
        outputStream << quint32(0) // legacy offset
                     << componentType
                     << componentCount
                     << offset;
        entriesByteSize += VERTEX_BUFFER_ENTRY_STRUCT_SIZE;
    }
    quint32 alignAmount = offsetTracker.alignedAdvance(entriesByteSize);
    if (alignAmount)
        device->write(alignPadding, alignAmount);

    for (quint32 i = 0; i < vertexBufferEntriesCount; ++i) {
        const Mesh::VertexBufferEntry &entry(mesh.m_vertexBuffer.entries[i]);
        const quint32 nameLength = entry.name.size() + 1;
        outputStream << nameLength;
        device->write(entry.name.constData(), nameLength); // with zero terminator included
        alignAmount = offsetTracker.alignedAdvance(sizeof(quint32) + nameLength);
        if (alignAmount)
            device->write(alignPadding, alignAmount);
    }

    device->write(mesh.m_vertexBuffer.data.constData(), vertexBufferDataSize);
    alignAmount = offsetTracker.alignedAdvance(vertexBufferDataSize);
    if (alignAmount)
        device->write(alignPadding, alignAmount);

    device->write(mesh.m_indexBuffer.data.constData(), indexBufferDataSize);
    alignAmount = offsetTracker.alignedAdvance(indexBufferDataSize);
    if (alignAmount)
        device->write(alignPadding, alignAmount);

    quint32 subsetByteSize = 0;
    for (quint32 i = 0; i < subsetsCount; ++i) {
        const Mesh::Subset &subset(mesh.m_subsets[i]);
        const quint32 subsetCount = subset.count;
        const quint32 subsetOffset = subset.offset;
        const float minX = subset.bounds.min.x();
        const float minY = subset.bounds.min.y();
        const float minZ = subset.bounds.min.z();
        const float maxX = subset.bounds.max.x();
        const float maxY = subset.bounds.max.y();
        const float maxZ = subset.bounds.max.z();
        const quint32 nameLength = subset.name.size() + 1;
        const quint32 lightmapSizeHintWidth = qMax(0, subset.lightmapSizeHint.width());
        const quint32 lightmapSizeHintHeight = qMax(0, subset.lightmapSizeHint.height());
        const quint32 lodCount = subset.lods.size();
        outputStream << subsetCount
                     << subsetOffset
                     << minX
                     << minY
                     << minZ
                     << maxX
                     << maxY
                     << maxZ;
        outputStream << quint32(0) // legacy offset
                     << nameLength;
        outputStream << lightmapSizeHintWidth
                     << lightmapSizeHintHeight;
        outputStream << lodCount;
        subsetByteSize += SUBSET_STRUCT_SIZE_V6;
    }
    alignAmount = offsetTracker.alignedAdvance(subsetByteSize);
    if (alignAmount)
        device->write(alignPadding, alignAmount);

    for (quint32 i = 0; i < subsetsCount; ++i) {
        const Mesh::Subset &subset(mesh.m_subsets[i]);
        const char *utf16Name = reinterpret_cast<const char *>(subset.name.utf16());
        const quint32 nameByteSize = (subset.name.size() + 1) * 2;
        device->write(utf16Name, nameByteSize);
        alignAmount = offsetTracker.alignedAdvance(nameByteSize);
        if (alignAmount)
            device->write(alignPadding, alignAmount);
    }

    // LOD data
    quint32 lodDataByteSize = 0;
    for (quint32 i = 0; i < subsetsCount; ++i) {
        const Mesh::Subset &subset(mesh.m_subsets[i]);
        for (auto lod : subset.lods) {
            const quint32 count = lod.count;
            const quint32 offset = lod.offset;
            const float distance = lod.distance;
            outputStream << count << offset << distance;
            lodDataByteSize += LOD_STRUCT_SIZE;
        }
    }
    alignAmount = offsetTracker.alignedAdvance(lodDataByteSize);
    if (alignAmount)
        device->write(alignPadding, alignAmount);

    // Data for morphTargets
    for (quint32 i = 0; i < targetBufferEntriesCount; ++i) {
        const Mesh::VertexBufferEntry &entry(mesh.m_targetBuffer.entries[i]);
        const quint32 componentType = quint32(entry.componentType);
        const quint32 componentCount = entry.componentCount;
        const quint32 offset = entry.offset;
        outputStream << quint32(0) // legacy offset
                     << componentType
                     << componentCount
                     << offset;
        entriesByteSize += VERTEX_BUFFER_ENTRY_STRUCT_SIZE;
    }
    alignAmount = offsetTracker.alignedAdvance(entriesByteSize);
    if (alignAmount)
        device->write(alignPadding, alignAmount);

    for (quint32 i = 0; i < targetBufferEntriesCount; ++i) {
        const Mesh::VertexBufferEntry &entry(mesh.m_targetBuffer.entries[i]);
        const quint32 nameLength = entry.name.size() + 1;
        outputStream << nameLength;
        device->write(entry.name.constData(), nameLength); // with zero terminator included
        alignAmount = offsetTracker.alignedAdvance(sizeof(quint32) + nameLength);
        if (alignAmount)
            device->write(alignPadding, alignAmount);
    }

    device->write(mesh.m_targetBuffer.data.constData(), targetBufferDataSize);

    const quint32 endPos = device->pos();
    const quint32 sizeInBytes = endPos - startPos;
    device->seek(endPos);
    return sizeInBytes;
}

Mesh Mesh::loadMesh(QIODevice *device, quint32 id)
{
    MeshInternal::MeshDataHeader header;
    const MeshInternal::MultiMeshInfo meshFileInfo = MeshInternal::readFileHeader(device);
    auto it = meshFileInfo.meshEntries.constFind(id);
    if (it != meshFileInfo.meshEntries.constEnd()) {
        Mesh mesh;
        quint64 size = MeshInternal::readMeshData(device, *it, &mesh, &header);
        if (size)
            return mesh;
    } else if (id == 0 && !meshFileInfo.meshEntries.isEmpty()) {
        Mesh mesh;
        quint64 size = MeshInternal::readMeshData(device, *meshFileInfo.meshEntries.cbegin(), &mesh, &header);
        if (size)
            return mesh;
    }
    return Mesh();
}

QMap<quint32, Mesh> Mesh::loadAll(QIODevice *device)
{
    MeshInternal::MeshDataHeader header;
    const MeshInternal::MultiMeshInfo meshFileInfo = MeshInternal::readFileHeader(device);
    QMap<quint32, Mesh> meshes;
    for (auto it = meshFileInfo.meshEntries.cbegin(), end = meshFileInfo.meshEntries.cend(); it != end; ++it) {
        Mesh mesh;
        quint64 size = MeshInternal::readMeshData(device, *it, &mesh, &header);
        if (size)
            meshes.insert(it.key(), mesh);
        else
            qWarning("Failed to find mesh #%u", it.key());
    }
    return meshes;
}

static inline quint32 getAlignedOffset(quint32 offset, quint32 align)
{
    Q_ASSERT(align > 0);
    const quint32 leftover = (align > 0) ? offset % align : 0;
    if (leftover)
        return offset + (align - leftover);
    return offset;
}

Mesh Mesh::fromAssetData(const QVector<AssetVertexEntry> &vbufEntries,
                         const QByteArray &indexBufferData,
                         ComponentType indexComponentType,
                         const QVector<AssetMeshSubset> &subsets,
                         quint32 numTargets,
                         quint32 numTargetComps)
{
    Mesh mesh;
    quint32 currentOffset = 0;
    quint32 bufferAlignment = 0;
    quint32 numItems = 0;
    bool ok = true;

    mesh.m_targetBuffer.numTargets = numTargets;
    quint32 targetCurrentComp = 0;
    quint32 targetCompStride = 0;

    QVector<AssetVertexEntry> vEntries;
    for (const AssetVertexEntry &entry : vbufEntries) {
        // Ignore entries with no data.
        if (entry.data.isEmpty())
            continue;

        VertexBufferEntry meshEntry;
        meshEntry.componentType = entry.componentType;
        meshEntry.componentCount = entry.componentCount;
        meshEntry.name = entry.name;

        if (entry.morphTargetId < 0) {
            const quint32 alignment = MeshInternal::byteSizeForComponentType(entry.componentType);
            const quint32 byteSize = alignment * entry.componentCount;

            if (entry.data.size() % alignment != 0) {
                Q_ASSERT(false);
                ok = false;
            }

            quint32 localNumItems = entry.data.size() / byteSize;
            if (numItems == 0) {
                numItems = localNumItems;
            } else if (numItems != localNumItems) {
                Q_ASSERT(false);
                ok = false;
                numItems = qMin(numItems, localNumItems);
            }

            currentOffset = getAlignedOffset(currentOffset, alignment);
            meshEntry.offset = currentOffset;

            mesh.m_vertexBuffer.entries.append(meshEntry);
            currentOffset += byteSize;
            bufferAlignment = qMax(bufferAlignment, alignment);
            vEntries.append(entry);
        } else {
            if (!targetCompStride) {
                const quint32 targetEntrySize = entry.data.size();
                quint32 targetEntryTexWidth = qCeil(qSqrt(((targetEntrySize + 15) >> 4)));
                targetCompStride = targetEntryTexWidth * targetEntryTexWidth * 4 * sizeof(float);
                mesh.m_targetBuffer.data.resize(targetCompStride * numTargets * numTargetComps);
            }

            // At assets, these entries are appended sequentially from target 0 to target N - 1
            // It is safe to calculate the offset by the data size
            meshEntry.offset = (targetCurrentComp * numTargets + entry.morphTargetId)
                                    * targetCompStride;
            memcpy(mesh.m_targetBuffer.data.data() + meshEntry.offset,
                   entry.data.constData(), entry.data.size());

            // Note: the targetBuffer will not be interleaved,
            // data will be just appended in order and used for a texture array.
            if (entry.morphTargetId == 0)
                mesh.m_targetBuffer.entries.append(meshEntry);

            targetCurrentComp = (targetCurrentComp + 1 < numTargetComps) ? targetCurrentComp + 1 : 0;
        }
    }

    if (!ok)
        return Mesh();

    mesh.m_vertexBuffer.stride = getAlignedOffset(currentOffset, bufferAlignment);

    // Packed interleave the data.
    for (quint32 idx = 0; idx < numItems; ++idx) {
        quint32 dataOffset = 0;
        for (const AssetVertexEntry &entry : vEntries) {
            if (entry.data.isEmpty())
                continue;

            const quint32 alignment = MeshInternal::byteSizeForComponentType(entry.componentType);
            const quint32 byteSize = alignment * entry.componentCount;
            const quint32 offset = byteSize * idx;
            const quint32 newOffset = getAlignedOffset(dataOffset, alignment);
            if (newOffset != dataOffset) {
                QByteArray filler(newOffset - dataOffset, '\0');
                mesh.m_vertexBuffer.data.append(filler);
            }

            mesh.m_vertexBuffer.data.append(entry.data.constData() + offset, byteSize);
            dataOffset = newOffset + byteSize;
        }
        Q_ASSERT(dataOffset == mesh.m_vertexBuffer.stride);
    }

    mesh.m_indexBuffer.componentType = indexComponentType;
    mesh.m_indexBuffer.data = indexBufferData;

    for (const AssetMeshSubset &subset : subsets) {
        Mesh::Subset meshSubset;
        meshSubset.name = subset.name;
        meshSubset.count = subset.count;
        meshSubset.offset = subset.offset;

        // TODO: QTBUG-102026
        if (subset.boundsPositionEntryIndex != std::numeric_limits<quint32>::max()) {
            const QSSGBounds3 bounds = MeshInternal::calculateSubsetBounds(
                    mesh.m_vertexBuffer.entries[subset.boundsPositionEntryIndex],
                    mesh.m_vertexBuffer.data,
                    mesh.m_vertexBuffer.stride,
                    mesh.m_indexBuffer.data,
                    mesh.m_indexBuffer.componentType,
                    subset.count,
                    subset.offset);
            meshSubset.bounds.min = bounds.minimum;
            meshSubset.bounds.max = bounds.maximum;
        }

        meshSubset.lightmapSizeHint = QSize(subset.lightmapWidth, subset.lightmapHeight);
        meshSubset.lods = subset.lods;

        mesh.m_subsets.append(meshSubset);
    }

    mesh.m_drawMode = DrawMode::Triangles;
    mesh.m_winding = Winding::CounterClockwise;

    return mesh;
}

Mesh Mesh::fromRuntimeData(const RuntimeMeshData &data, QString *error)
{
    if (data.m_vertexBuffer.size() == 0) {
        *error = QObject::tr("Vertex buffer empty");
        return Mesh();
    }
    if (data.m_attributeCount == 0) {
        *error = QObject::tr("No attributes defined");
        return Mesh();
    }

    Mesh mesh;
    mesh.m_drawMode = data.m_primitiveType;
    mesh.m_winding = Winding::CounterClockwise;

    for (int i = 0; i < data.m_attributeCount; ++i) {
        const RuntimeMeshData::Attribute &att = data.m_attributes[i];
        if (att.semantic == RuntimeMeshData::Attribute::IndexSemantic) {
            mesh.m_indexBuffer.componentType = att.componentType;
        } else {
            const char *name = nullptr;
            switch (att.semantic) {
            case RuntimeMeshData::Attribute::PositionSemantic:
                name = MeshInternal::getPositionAttrName();
                break;
            case RuntimeMeshData::Attribute::NormalSemantic:
                name = MeshInternal::getNormalAttrName();
                break;
            case RuntimeMeshData::Attribute::TexCoord0Semantic:
                name = MeshInternal::getUV0AttrName();
                break;
            case RuntimeMeshData::Attribute::TexCoord1Semantic:
                name = MeshInternal::getUV1AttrName();
                break;
            case RuntimeMeshData::Attribute::TangentSemantic:
                name = MeshInternal::getTexTanAttrName();
                break;
            case RuntimeMeshData::Attribute::BinormalSemantic:
                name = MeshInternal::getTexBinormalAttrName();
                break;
            case RuntimeMeshData::Attribute::JointSemantic:
                name = MeshInternal::getJointAttrName();
                break;
            case RuntimeMeshData::Attribute::WeightSemantic:
                name = MeshInternal::getWeightAttrName();
                break;
            case RuntimeMeshData::Attribute::ColorSemantic:
                name = MeshInternal::getColorAttrName();
                break;
            default:
                *error = QObject::tr("Warning: Invalid attribute semantic: %1")
                        .arg(att.semantic);
                return Mesh();
            }

            VertexBufferEntry entry;
            entry.componentType = att.componentType;
            entry.componentCount = att.componentCount();
            entry.offset = att.offset;
            entry.name = name;
            mesh.m_vertexBuffer.entries.append(entry);
        }
    }

    mesh.m_vertexBuffer.data = data.m_vertexBuffer;
    // Only interleaved vertex attribute packing is supported, both internally
    // and in the QQuick3DGeometry API, hence the per-vertex buffer stride.
    mesh.m_vertexBuffer.stride = data.m_stride;
    mesh.m_subsets = data.m_subsets;
    mesh.m_indexBuffer.data = data.m_indexBuffer;

    if (!data.m_targetBuffer.isEmpty()) {
        const quint32 vertexCount = data.m_vertexBuffer.size() / data.m_stride;
        const quint32 targetEntryTexWidth = qCeil(qSqrt(vertexCount));
        const quint32 targetCompStride = targetEntryTexWidth * targetEntryTexWidth * 4 * sizeof(float);
        mesh.m_targetBuffer.data.resize(targetCompStride * data.m_targetAttributeCount);

        QVarLengthArray<RuntimeMeshData::TargetAttribute> sortedAttribs(
                                            data.m_targetAttributes,
                                            data.m_targetAttributes + data.m_targetAttributeCount);
        std::sort(sortedAttribs.begin(), sortedAttribs.end(),
                  [] (RuntimeMeshData::TargetAttribute a, RuntimeMeshData::TargetAttribute b) {
                  return (a.targetId == b.targetId) ? a.attr.semantic < b.attr.semantic :
                                                      a.targetId < b.targetId; });
        for (int i = 0; i < data.m_targetAttributeCount; ++i) {
            const RuntimeMeshData::Attribute &att = sortedAttribs[i].attr;
            const int stride = (sortedAttribs[i].stride < 1) ? att.componentCount() * sizeof(float)
                                                             : sortedAttribs[i].stride;
            const char *name = nullptr;
            switch (att.semantic) {
            case RuntimeMeshData::Attribute::PositionSemantic:
                name = MeshInternal::getPositionAttrName();
                break;
            case RuntimeMeshData::Attribute::NormalSemantic:
                name = MeshInternal::getNormalAttrName();
                break;
            case RuntimeMeshData::Attribute::TexCoord0Semantic:
                name = MeshInternal::getUV0AttrName();
                break;
            case RuntimeMeshData::Attribute::TexCoord1Semantic:
                name = MeshInternal::getUV1AttrName();
                break;
            case RuntimeMeshData::Attribute::TangentSemantic:
                name = MeshInternal::getTexTanAttrName();
                break;
            case RuntimeMeshData::Attribute::BinormalSemantic:
                name = MeshInternal::getTexBinormalAttrName();
                break;
            case RuntimeMeshData::Attribute::IndexSemantic:
            case RuntimeMeshData::Attribute::JointSemantic:
            case RuntimeMeshData::Attribute::WeightSemantic:
                *error = QObject::tr("Warning: Invalid target attribute semantic: %1")
                        .arg(att.semantic);
                continue;
            case RuntimeMeshData::Attribute::ColorSemantic:
                name = MeshInternal::getColorAttrName();
                break;
            default:
                *error = QObject::tr("Warning: Invalid target attribute semantic: %1")
                        .arg(att.semantic);
                return Mesh();
            }
            char *dstBuf = mesh.m_targetBuffer.data.data() + i * targetCompStride;
            const char *srcBuf = data.m_targetBuffer.constData() + att.offset;
            Q_ASSERT(att.componentType == Mesh::ComponentType::Float32);
            if (stride == 4 * sizeof(float)) {
                memcpy(dstBuf, srcBuf, vertexCount * stride);
            } else {
                for (quint32 j = 0; j < vertexCount; ++j) {
                    memcpy(dstBuf + j * 4 * sizeof(float),
                           srcBuf + j * stride,
                           att.componentCount() * sizeof(float));
                }
            }

            if (sortedAttribs[i].targetId == 0) {
                VertexBufferEntry entry;
                entry.componentType = att.componentType;
                entry.componentCount = att.componentCount();
                entry.offset = i * targetCompStride;
                entry.name = name;
                mesh.m_targetBuffer.entries.append(entry);
            }
        }
        mesh.m_targetBuffer.numTargets = data.m_targetAttributeCount / mesh.m_targetBuffer.entries.size();
    }
    return mesh;
}

quint32 Mesh::save(QIODevice *device, quint32 id) const
{
    qint64 newMeshStartPosFromEnd = 0;
    quint32 newId = 1;
    MeshInternal::MultiMeshInfo header;

    if (device->size() > 0) {
        header = MeshInternal::readFileHeader(device);
        if (!header.isValid()) {
            qWarning("There is existing data, but mesh file header is invalid; cannot save");
            return 0;
        }
        for (auto it = header.meshEntries.cbegin(), end = header.meshEntries.cend(); it != end; ++it) {
            if (id) {
                Q_ASSERT(id != it.key());
                newId = id;
            } else {
                newId = qMax(newId, it.key() + 1);
            }
        }
        newMeshStartPosFromEnd = MULTI_HEADER_STRUCT_SIZE + header.meshEntries.size() * MULTI_ENTRY_STRUCT_SIZE;
    } else {
        header = MeshInternal::MultiMeshInfo::withDefaults();
    }

    // the new mesh data overwrites the entry list and file header
    device->seek(device->size() - newMeshStartPosFromEnd);
    const qint64 meshOffset = device->pos();
    header.meshEntries.insert(newId, meshOffset);

    MeshInternal::MeshDataHeader meshHeader = MeshInternal::MeshDataHeader::withDefaults();
    // skip the space for the mesh header for now
    device->seek(device->pos() + MESH_HEADER_STRUCT_SIZE);
    meshHeader.sizeInBytes = MeshInternal::writeMeshData(device, *this);
    // now the mesh header is ready to be written out
    device->seek(meshOffset);
    MeshInternal::writeMeshHeader(device, meshHeader);
    device->seek(meshOffset + MESH_HEADER_STRUCT_SIZE + meshHeader.sizeInBytes);
    // write out new entry list and file header
    MeshInternal::writeFileHeader(device, header);

    return newId;
}

QSSGBounds3 MeshInternal::calculateSubsetBounds(const Mesh::VertexBufferEntry &entry,
                                                const QByteArray &vertexBufferData,
                                                quint32 vertexBufferStride,
                                                const QByteArray &indexBufferData,
                                                Mesh::ComponentType indexComponentType,
                                                quint32 subsetCount,
                                                quint32 subsetOffset)
{
    QSSGBounds3 result;
    if (entry.componentType != Mesh::ComponentType::Float32 || entry.componentCount != 3) {
        Q_ASSERT(false);
        return result;
    }

    const int indexComponentByteSize = byteSizeForComponentType(indexComponentType);
    if (indexComponentByteSize != 2 && indexComponentByteSize != 4) {
        Q_ASSERT(false);
        return result;
    }

    const quint32 indexBufferCount = indexBufferData.size() / indexComponentByteSize;
    const quint32 vertexBufferByteSize = vertexBufferData.size();
    const char *vertexSrcPtr = vertexBufferData.constData();
    const char *indexSrcPtr = indexBufferData.constData();

    for (quint32 idx = 0, numItems = subsetCount; idx < numItems; ++idx) {
        if (idx + subsetOffset >= indexBufferCount)
            continue;

        quint32 vertexIdx = 0;
        switch (indexComponentByteSize) {
        case 2:
            vertexIdx = reinterpret_cast<const quint16 *>(indexSrcPtr)[idx + subsetOffset];
            break;
        case 4:
            vertexIdx = reinterpret_cast<const quint32 *>(indexSrcPtr)[idx + subsetOffset];
            break;
        default:
            Q_UNREACHABLE();
            break;
        }

        const quint32 finalOffset = entry.offset + (vertexIdx * vertexBufferStride);
        float v[3];
        if (finalOffset + sizeof(v) <= vertexBufferByteSize) {
            memcpy(v, vertexSrcPtr + finalOffset, sizeof(v));
            result.include(QVector3D(v[0], v[1], v[2]));
        } else {
            Q_ASSERT(false);
        }
    }

    return result;
}

bool Mesh::hasLightmapUVChannel() const
{
    const char *lightmapAttrName = MeshInternal::getLightmapUVAttrName();
    for (const VertexBufferEntry &vbe : std::as_const(m_vertexBuffer.entries)) {
        if (vbe.name == lightmapAttrName)
            return true;
    }
    return false;
}

bool Mesh::createLightmapUVChannel(float texelsPerUnit, const QMatrix4x4 &scale)
{
    const char *posAttrName = MeshInternal::getPositionAttrName();
    const char *normalAttrName = MeshInternal::getNormalAttrName();
    const char *uvAttrName = MeshInternal::getUV0AttrName();
    const char *lightmapAttrName = MeshInternal::getLightmapUVAttrName();

    // this function should do nothing if there is already an attr_lightmapuv
    if (hasLightmapUVChannel())
        return true;

    const char *srcVertexData = m_vertexBuffer.data.constData();
    const quint32 srcVertexStride = m_vertexBuffer.stride;
    if (!srcVertexStride) {
        qWarning("Lightmap UV unwrapping encountered a Mesh with 0 vertex stride, this cannot happen");
        return false;
    }
    if (m_indexBuffer.data.isEmpty()) {
        qWarning("Lightmap UV unwrapping encountered a Mesh without index data, this cannot happen");
        return false;
    }

    quint32 positionOffset = UINT32_MAX;
    quint32 normalOffset = UINT32_MAX;
    quint32 uvOffset = UINT32_MAX;

    for (const VertexBufferEntry &vbe : std::as_const(m_vertexBuffer.entries)) {
        if (vbe.name == posAttrName) {
            if (vbe.componentCount != 3) {
                qWarning("Lightmap UV unwrapping encountered a Mesh non-float3 position data, this cannot happen");
                return false;
            }
            positionOffset = vbe.offset;
        } else if (vbe.name == normalAttrName) {
            if (vbe.componentCount != 3) {
                qWarning("Lightmap UV unwrapping encountered a Mesh non-float3 normal data, this cannot happen");
                return false;
            }
            normalOffset = vbe.offset;
        } else if (vbe.name == uvAttrName) {
            if (vbe.componentCount != 2) {
                qWarning("Lightmap UV unwrapping encountered a Mesh non-float2 UV0 data, this cannot happen");
                return false;
            }
            uvOffset = vbe.offset;
        }
    }

    if (positionOffset == UINT32_MAX) {
        qWarning("Lightmap UV unwrapping encountered a Mesh without vertex positions, this cannot happen");
        return false;
    }
    // normal and uv0 are optional

    const qsizetype vertexCount = m_vertexBuffer.data.size() / srcVertexStride;
    QByteArray positionData(vertexCount * 3 * sizeof(float), Qt::Uninitialized);
    float *posPtr = reinterpret_cast<float *>(positionData.data());
    for (qsizetype i = 0; i < vertexCount; ++i) {
        const char *vertexBasePtr = srcVertexData + i * srcVertexStride;
        const float *srcPos = reinterpret_cast<const float *>(vertexBasePtr + positionOffset);
        QVector3D srcV;
        srcV.setX(*srcPos++);
        srcV.setY(*srcPos++);
        srcV.setZ(*srcPos++);
        // We scale the positions here, but not on the source mesh, so that the uv unwrapper works on
        // the positions that the model will have in the scene after its scaling has been applied. This
        // way the texels-per-unit will be correct.
        srcV = scale.map(srcV);
        *posPtr++ = srcV.x();
        *posPtr++ = srcV.y();
        *posPtr++ = srcV.z();
    }

    QByteArray normalData;
    if (normalOffset != UINT32_MAX) {
        normalData.resize(vertexCount * 3 * sizeof(float));
        float *normPtr = reinterpret_cast<float *>(normalData.data());
        for (qsizetype i = 0; i < vertexCount; ++i) {
            const char *vertexBasePtr = srcVertexData + i * srcVertexStride;
            const float *srcNormal = reinterpret_cast<const float *>(vertexBasePtr + normalOffset);
            *normPtr++ = *srcNormal++;
            *normPtr++ = *srcNormal++;
            *normPtr++ = *srcNormal++;
        }
    }

    QByteArray uvData;
    if (uvOffset != UINT32_MAX) {
        uvData.resize(vertexCount * 2 * sizeof(float));
        float *uvPtr = reinterpret_cast<float *>(uvData.data());
        for (qsizetype i = 0; i < vertexCount; ++i) {
            const char *vertexBasePtr = srcVertexData + i * srcVertexStride;
            const float *srcUv = reinterpret_cast<const float *>(vertexBasePtr + uvOffset);
            *uvPtr++ = *srcUv++;
            *uvPtr++ = *srcUv++;
        }
    }

    QSSGLightmapUVGenerator uvGen;
    QSSGLightmapUVGeneratorResult r = uvGen.run(positionData, normalData, uvData,
                                                m_indexBuffer.data, m_indexBuffer.componentType,
                                                texelsPerUnit);
    if (!r.isValid())
        return false;

    // the result can have more (but never less) vertices than the input
    const int newVertexCount = r.vertexMap.size();

    // r.indexData contains the new index data that has the same number of elements as before
    const quint32 *newIndex = reinterpret_cast<const quint32 *>(r.indexData.constData());
    if (m_indexBuffer.componentType == QSSGMesh::Mesh::ComponentType::UnsignedInt32) {
        if (r.indexData.size() != m_indexBuffer.data.size()) {
            qWarning("Index buffer size mismatch after lightmap UV unwrapping");
            return false;
        }
        quint32 *indexDst = reinterpret_cast<quint32 *>(m_indexBuffer.data.data());
        memcpy(indexDst, newIndex, m_indexBuffer.data.size());
    } else {
        if (r.indexData.size() != m_indexBuffer.data.size() * 2) {
            qWarning("Index buffer size mismatch after lightmap UV unwrapping");
            return false;
        }
        quint16 *indexDst = reinterpret_cast<quint16 *>(m_indexBuffer.data.data());
        for (size_t i = 0, count = m_indexBuffer.data.size() / sizeof(quint16); i != count; ++i)
            *indexDst++ = *newIndex++;
    }

    QVarLengthArray<QByteArray, 8> newData;
    newData.reserve(m_vertexBuffer.entries.size());

    for (const VertexBufferEntry &vbe : std::as_const(m_vertexBuffer.entries)) {
        const qsizetype byteSize = vbe.componentCount * MeshInternal::byteSizeForComponentType(vbe.componentType);
        QByteArray data(byteSize * vertexCount, Qt::Uninitialized);
        char *dst = data.data();
        for (qsizetype i = 0; i < vertexCount; ++i) {
            memcpy(dst, srcVertexData + i * srcVertexStride + vbe.offset, byteSize);
            dst += byteSize;
        }
        switch (vbe.componentType) {
        case ComponentType::UnsignedInt8:
            newData.append(QSSGLightmapUVGenerator::remap<quint8>(data, r.vertexMap, vbe.componentCount));
            break;
        case ComponentType::Int8:
            newData.append(QSSGLightmapUVGenerator::remap<qint8>(data, r.vertexMap, vbe.componentCount));
            break;
        case ComponentType::UnsignedInt16:
            newData.append(QSSGLightmapUVGenerator::remap<quint16>(data, r.vertexMap, vbe.componentCount));
            break;
        case ComponentType::Int16:
            newData.append(QSSGLightmapUVGenerator::remap<qint16>(data, r.vertexMap, vbe.componentCount));
            break;
        case ComponentType::UnsignedInt32:
            newData.append(QSSGLightmapUVGenerator::remap<quint32>(data, r.vertexMap, vbe.componentCount));
            break;
        case ComponentType::Int32:
            newData.append(QSSGLightmapUVGenerator::remap<qint32>(data, r.vertexMap, vbe.componentCount));
            break;
        case ComponentType::UnsignedInt64:
            newData.append(QSSGLightmapUVGenerator::remap<quint64>(data, r.vertexMap, vbe.componentCount));
            break;
        case ComponentType::Int64:
            newData.append(QSSGLightmapUVGenerator::remap<qint64>(data, r.vertexMap, vbe.componentCount));
            break;
        case ComponentType::Float16:
            newData.append(QSSGLightmapUVGenerator::remap<qfloat16>(data, r.vertexMap, vbe.componentCount));
            break;
        case ComponentType::Float32:
            newData.append(QSSGLightmapUVGenerator::remap<float>(data, r.vertexMap, vbe.componentCount));
            break;
        case ComponentType::Float64:
            newData.append(QSSGLightmapUVGenerator::remap<double>(data, r.vertexMap, vbe.componentCount));
            break;
        }
    }

    VertexBufferEntry lightmapUVEntry;
    lightmapUVEntry.componentType = ComponentType::Float32;
    lightmapUVEntry.componentCount = 2;
    lightmapUVEntry.offset = 0;
    lightmapUVEntry.name = lightmapAttrName;

    QByteArray newVertexBuffer;
    newVertexBuffer.reserve(newVertexCount * (srcVertexStride + 8));

    quint32 bufferAlignment = 0;
    for (int vertexIdx = 0; vertexIdx < newVertexCount; ++vertexIdx) {
        quint32 dataOffset = 0;
        for (int vbIdx = 0, end = m_vertexBuffer.entries.size(); vbIdx != end; ++vbIdx) {
            VertexBufferEntry &vbe(m_vertexBuffer.entries[vbIdx]);

            const quint32 alignment = MeshInternal::byteSizeForComponentType(vbe.componentType);
            bufferAlignment = qMax(bufferAlignment, alignment);
            const quint32 byteSize = alignment * vbe.componentCount;
            const quint32 newOffset = getAlignedOffset(dataOffset, alignment);

            if (newOffset != dataOffset) {
                QByteArray filler(newOffset - dataOffset, '\0');
                newVertexBuffer.append(filler);
            }

            if (vertexIdx == 0)
                vbe.offset = newVertexBuffer.size();

            newVertexBuffer.append(newData[vbIdx].constData() + byteSize * vertexIdx, byteSize);
            dataOffset = newOffset + byteSize;
        }

        const quint32 byteSize = 2 * sizeof(float);
        const quint32 newOffset = getAlignedOffset(dataOffset, byteSize);
        if (newOffset != dataOffset) {
            QByteArray filler(newOffset - dataOffset, '\0');
            newVertexBuffer.append(filler);
        }

        if (vertexIdx == 0)
            lightmapUVEntry.offset = newVertexBuffer.size();

        newVertexBuffer.append(r.lightmapUVChannel.constData() + byteSize * vertexIdx, byteSize);
        dataOffset = newOffset + byteSize;

        if (vertexIdx == 0)
            m_vertexBuffer.stride = getAlignedOffset(dataOffset, bufferAlignment);
    }

    m_vertexBuffer.entries.append(lightmapUVEntry);

    m_vertexBuffer.data = newVertexBuffer;

    const QSize lightmapSizeHint(r.lightmapWidth, r.lightmapHeight);
    for (Subset &subset : m_subsets)
        subset.lightmapSizeHint = lightmapSizeHint;

    return true;
}

size_t simplifyMesh(unsigned int *destination, const unsigned int *indices, size_t indexCount, const float *vertexPositions, size_t vertexCount, size_t vertexPositionsStride, size_t targetIndexCount, float targetError, unsigned int options, float *resultError)
{
    return meshopt_simplify(destination, indices, indexCount, vertexPositions, vertexCount, vertexPositionsStride, targetIndexCount, targetError, options, resultError);
}

float simplifyScale(const float *vertexPositions, size_t vertexCount, size_t vertexPositionsStride)
{
    return meshopt_simplifyScale(vertexPositions, vertexCount, vertexPositionsStride);
}

void optimizeVertexCache(unsigned int *destination, const unsigned int *indices, size_t indexCount, size_t vertexCount)
{
    meshopt_optimizeVertexCache(destination, indices, indexCount, vertexCount);
}

size_t generateVertexRemap(unsigned int *destination,
                           const unsigned int *indices,
                           size_t indexCount,
                           size_t vertexCount,
                           const MeshVertexStream *streams,
                           size_t streamCount)
{
    Q_ASSERT(streamCount > 0 && streamCount <= maxVertexStreams);
    QVarLengthArray<meshopt_Stream, 16> meshoptStreams;
    for (size_t i = 0; i < streamCount; ++i)
        meshoptStreams.append({ streams[i].data, streams[i].elementSize, streams[i].stride });
    return meshopt_generateVertexRemapMulti(destination, indices, indexCount, vertexCount,
                                            meshoptStreams.constData(), streamCount);
}

void remapVertexBuffer(void *destination,
                       const void *vertices,
                       size_t vertexCount,
                       size_t vertexSize,
                       const unsigned int *remap)
{
    meshopt_remapVertexBuffer(destination, vertices, vertexCount, vertexSize, remap);
}

void remapIndexBuffer(unsigned int *destination,
                      const unsigned int *indices,
                      size_t indexCount,
                      const unsigned int *remap)
{
    meshopt_remapIndexBuffer(destination, indices, indexCount, remap);
}

QVector<MeshLevelOfDetail> generateMeshLevelsOfDetail(const QVector<QVector3D> &positions,
                                                      const QVector<QVector3D> &normals,
                                                      const QVector<quint32> &indexes,
                                                      QVector<MeshVertexSplit> &splitVertices,
                                                      float normalMergeAngle,
                                                      float normalSplitAngle)
{
    // The split vertices are reported positionally, starting at
    // positions.size(), so stale entries would make the caller's append loop
    // line up with the wrong vertices
    splitVertices.clear();

    if (positions.isEmpty() || indexes.isEmpty())
        return {};

    // Correcting normals needs one per position. Both angles being 0.0 also
    // means the caller does not want them recalculated at all.
    const bool recalculateNormals = normals.size() == positions.size()
            && !(qFuzzyIsNull(normalMergeAngle) && qFuzzyIsNull(normalSplitAngle));
    const float normalMergeThreshold = qCos(qDegreesToRadians(normalMergeAngle));
    const float normalSplitThreshold = qCos(qDegreesToRadians(normalSplitAngle));

    quint32 splitVertexCount = positions.size();

    // An edge can only be collapsed when its vertices are shared by the faces
    // around it, so a vertex that appears more than once in the vertex buffer
    // pins every edge that touches it. Assets are very commonly authored with
    // one vertex per triangle corner - faceted normals and UV chart borders
    // both force a split, and some exporters simply never weld - and such a
    // mesh has no collapsible edge anywhere, so simplification returns the
    // input unchanged and not a single level is produced. Weld by position to
    // recover the real topology, simplify that, and translate the result back
    // to the caller's vertex numbering afterwards, so the vertex buffer they
    // hold is still the one the returned indexes refer to.
    //
    // Only the normals are rewritten below, so a position split to carry
    // different normals can be welded, but one split to carry a different
    // normal that has to be *kept* cannot: every face around it would be left
    // reading whichever of them the weld happened to pick. So when the caller
    // asked for the stored normals to be preserved, they join the weld key and
    // a hard edge stays pinned, at the cost of fewer levels for such a mesh.
    QVector<quint32> weldRemap(positions.size());
    QVarLengthArray<MeshVertexStream, 2> streams;
    streams.append({ positions.constData(), sizeof(QVector3D), sizeof(QVector3D) });
    if (!recalculateNormals && normals.size() == positions.size())
        streams.append({ normals.constData(), sizeof(QVector3D), sizeof(QVector3D) });
    const quint32 weldedVertexCount = generateVertexRemap(weldRemap.data(), indexes.constData(), indexes.size(),
                                                          positions.size(), streams.constData(),
                                                          size_t(streams.size()));
    QVector<quint32> weldedIndexes(indexes.size());
    remapIndexBuffer(weldedIndexes.data(), indexes.constData(), indexes.size(), weldRemap.constData());
    QVector<QVector3D> weldedPositions(weldedVertexCount);
    remapVertexBuffer(weldedPositions.data(), positions.constData(), positions.size(), sizeof(QVector3D),
                      weldRemap.constData());

    // Pick one original vertex to stand for each welded one, so the simplified
    // indexes can be mapped back. Every vertex welded together agrees on the
    // attributes the weld key covers, so the choice only matters for the rest:
    // where a position was split across a UV chart border the LOD has to settle
    // on one of the charts either way.
    constexpr quint32 unusedVertex = std::numeric_limits<quint32>::max();
    QVector<quint32> weldedToOriginal(weldedVertexCount, unusedVertex);
    for (quint32 i = 0, end = quint32(positions.size()); i < end; ++i) {
        const quint32 welded = weldRemap.at(i);
        // Vertices the index buffer never references are left unmapped
        if (welded != unusedVertex && weldedToOriginal.at(welded) == unusedVertex)
            weldedToOriginal[welded] = i;
    }

    const float targetError = std::numeric_limits<float>::max(); // error doesn't matter, index count is more important
    const float *vertexData = reinterpret_cast<const float *>(weldedPositions.constData());
    const float scaleFactor = simplifyScale(vertexData, weldedVertexCount, sizeof(QVector3D));
    const quint32 indexCount = indexes.size();
    quint32 indexTarget = 12;
    quint32 lastIndexCount = 0;
    QVector<MeshLevelOfDetail> lods;

    while (indexTarget < indexCount) {
        float error;
        QVector<quint32> newIndexes;
        newIndexes.resize(indexCount); // Must be the same size as the original indexes to pass to simplifyMesh
        size_t newLength = simplifyMesh(newIndexes.data(), weldedIndexes.constData(), weldedIndexes.size(),
                                        vertexData, weldedVertexCount, sizeof(QVector3D), indexTarget,
                                        targetError, 0, &error);

        // Not good enough, try again
        if (newLength < lastIndexCount * 1.5f) {
            indexTarget = indexTarget * 1.5f;
            continue;
        }

        // We are done
        if (newLength == 0 || (newLength >= (indexCount * 0.75f)))
            break;

        newIndexes.resize(newLength);

        // Back to the caller's vertex numbering, which everything below - and
        // the returned levels - is expressed in
        for (quint32 &index : newIndexes)
            index = weldedToOriginal.at(index);

        // LOD Normal Correction
        if (recalculateNormals) {
            // Cull any new degenerate triangles and get the new face normals
            QVector<QVector3D> faceNormals;
            {
                QVector<quint32> culledIndexes;
                for (quint32 j = 0; j < quint32(newIndexes.size()); j += 3) {
                    const QVector3D &v0 = positions[newIndexes[j]];
                    const QVector3D &v1 = positions[newIndexes[j + 1]];
                    const QVector3D &v2 = positions[newIndexes[j + 2]];

                    QVector3D faceNormal = QVector3D::crossProduct(v1 - v0, v2 - v0);
                    // This normalizes the vector in place and returns the magnitude
                    const float faceArea = QSSGUtils::vec3::normalize(faceNormal);
                    // It is possible that the simplifyMesh process gave us a degenerate triangle
                    // (all three at the same point, or on the same line) or such a small triangle
                    // that a float value doesn't have enough resolution. In that case cull the
                    // "face" since it would not get rendered in a meaningful way anyway
                    if (faceArea != 0.0f) {
                        faceNormals.append(faceNormal);
                        faceNormals.append(faceNormal);
                        faceNormals.append(faceNormal);
                        culledIndexes.append({newIndexes[j], newIndexes[j + 1], newIndexes[j + 2]});
                    }
                }

                if (newIndexes.size() != culledIndexes.size())
                    newIndexes = culledIndexes;
            }

            // Group all shared vertices together by position. We need to know adjacent faces
            // to do vertex normal remapping in the next step.
            const quint32 newIndexCount = quint32(newIndexes.size());
            QHash<QVector3D, QVector<quint32>> positionHash;
            for (quint32 i = 0; i < newIndexCount; ++i) {
                const quint32 index = newIndexes[i];
                const QVector3D position = positions[index];
                positionHash[position].append(i);
            }

            // Go through each vertex and calculate the normals by checking each
            // adjacent face that share the same vertex position, and create a smoothed
            // normal if the angle between thew face normals is less than the the
            // normalMergeAngle passed to this function (>= since this is cos(radian(angle)) )
            QVector<QPair<quint32, quint32>> remapIndexes;
            for (quint32 positionIndex = 0; positionIndex < newIndexCount; ++positionIndex) {
                const quint32 index = newIndexes[positionIndex];
                const QVector3D &position = positions[index];
                const QVector3D &faceNormal = faceNormals[positionIndex];
                QVector3D newNormal;
                // Find all vertices that share the same position
                const auto &sharedPositions = positionHash.value(position);
                for (const auto positionIndex2 : sharedPositions) {
                    if (positionIndex == positionIndex2) {
                        // Don't test against the current face under test
                        newNormal += faceNormal;
                    } else {
                        const QVector3D &faceNormal2 = faceNormals[positionIndex2];
                        if (QVector3D::dotProduct(faceNormal2, faceNormal) >= normalMergeThreshold)
                            newNormal += faceNormal2;
                    }
                }

                // By normalizing here we get an averaged value of all smoothed normals
                QSSGUtils::vec3::normalize(newNormal);

                // Now that we know what the smoothed normal would be, check how differnt
                // that normal is from the normal that is already stored in the current
                // index. If the angle delta is greater than normalSplitAngle then we need
                // to create a new vertex entry (making a copy of the current one) and set
                // the new normal value, and reassign the current index to point to that new
                // vertex. Generally the LOD simplification process is such that the existing
                // normal will already be ideal until we start getting to the very low lod levels
                // which changes the topology in such a way that the original normal doesn't
                // make sense anymore, thus the need to provide a more reasonable value.
                const QVector3D &originalNormal = normals[index];
                const float theta = QVector3D::dotProduct(originalNormal, newNormal);
                if (theta < normalSplitThreshold) {
                    splitVertices.append({ index, newNormal.normalized() });
                    remapIndexes.append({positionIndex, splitVertexCount++});
                }
            }

            // Do index remap now that all new normals have been calculated
            for (const auto &pair : std::as_const(remapIndexes))
                newIndexes[pair.first] = pair.second;
        }

        lods.append({error * scaleFactor, newIndexes});
        indexTarget = qMax(newLength, indexTarget) * 2;
        lastIndexCount = newLength;

        if (error == 0.0f)
            break;
    }

    return lods;
}

} // namespace QSSGMesh

QT_END_NAMESPACE
