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

#include "qssgmeshutilities_p.h"

#include <QtCore/QVector>
#include <QtCore/QBuffer>
#include <QtQuick3DUtils/private/qssgdataref_p.h>

QT_BEGIN_NAMESPACE

namespace QSSGMeshUtilities {

struct MeshSubsetV1
{
    // See description of a logical vertex buffer below
    quint32 m_logicalVbufIndex;
    // std::numeric_limits<quint32>::max() means use all available items
    quint32 m_count;
    // Offset is in item size, not bytes.
    quint32 m_offset;
    // Bounds of this subset.  This is filled in by the builder
    // see AddMeshSubset
    QSSGBounds3 m_bounds;
};

struct LogicalVertexBuffer
{
    quint32 m_byteOffset{ 0 };
    quint32 m_byteSize{ 0 };
    LogicalVertexBuffer(quint32 byteOff, quint32 byteSize) : m_byteOffset(byteOff), m_byteSize(byteSize) {}
    LogicalVertexBuffer() = default;
};

struct MeshV1
{
    VertexBuffer m_vertexBuffer;
    IndexBuffer m_indexBuffer;
    OffsetDataRef<LogicalVertexBuffer> m_logicalVertexBuffers; // may be empty
    OffsetDataRef<MeshSubsetV1> m_subsets;
    QSSGRenderDrawMode m_drawMode;
    QSSGRenderWinding m_winding;
    typedef MeshSubsetV1 TSubsetType;
};

template<typename TSerializer>
void serialize(TSerializer &serializer, MeshV1 &mesh)
{
    quint8 *baseAddr = reinterpret_cast<quint8 *>(&mesh);
    serializer.streamify(mesh.m_vertexBuffer.m_entries);
    serializer.align();
    for (quint32 entry = 0, __numItems = (quint32)mesh.m_vertexBuffer.m_entries.size(); entry < __numItems; ++entry) {
        MeshVertexBufferEntry &entryData = const_cast<MeshVertexBufferEntry &>(mesh.m_vertexBuffer.m_entries.index(baseAddr, entry));
        serializer.streamifyCharPointerOffset(entryData.m_nameOffset);
        serializer.align();
    }
    serializer.streamify(mesh.m_vertexBuffer.m_data);
    serializer.align();
    serializer.streamify(mesh.m_indexBuffer.m_data);
    serializer.align();
    serializer.streamify(mesh.m_logicalVertexBuffers);
    serializer.align();
    serializer.streamify(mesh.m_subsets);
    serializer.align();
}

struct MeshSubsetV2
{
    quint32 m_logicalVbufIndex;
    quint32 m_count;
    quint32 m_offset;
    QSSGBounds3 m_bounds;
    OffsetDataRef<char16_t> m_name;
};

struct MeshV2
{
    static const char16_t *m_defaultName;

    VertexBuffer m_vertexBuffer;
    IndexBuffer m_indexBuffer;
    OffsetDataRef<LogicalVertexBuffer> m_logicalVertexBuffers; // may be empty
    OffsetDataRef<MeshSubsetV2> m_subsets;
    QSSGRenderDrawMode m_drawMode;
    QSSGRenderWinding m_winding;
    typedef MeshSubsetV2 TSubsetType;
};

template<typename TSerializer>
void serialize(TSerializer &serializer, MeshV2 &mesh)
{
    quint8 *baseAddr = reinterpret_cast<quint8 *>(&mesh);
    serializer.streamify(mesh.m_vertexBuffer.m_entries);
    serializer.align();
    for (quint32 entry = 0, __numItems = (quint32)mesh.m_vertexBuffer.m_entries.size(); entry < __numItems; ++entry) {
        MeshVertexBufferEntry &entryData = const_cast<MeshVertexBufferEntry &>(mesh.m_vertexBuffer.m_entries.index(baseAddr, entry));
        serializer.streamifyCharPointerOffset(entryData.m_nameOffset);
        serializer.align();
    }
    serializer.streamify(mesh.m_vertexBuffer.m_data);
    serializer.align();
    serializer.streamify(mesh.m_indexBuffer.m_data);
    serializer.align();
    serializer.streamify(mesh.m_logicalVertexBuffers);
    serializer.align();
    serializer.streamify(mesh.m_subsets);
    serializer.align();
    for (quint32 entry = 0, __numItems = (quint32)mesh.m_subsets.size(); entry < __numItems; ++entry) {
        MeshSubsetV2 &theSubset = const_cast<MeshSubsetV2 &>(mesh.m_subsets.index(baseAddr, entry));
        serializer.streamify(theSubset.m_name);
        serializer.align();
    }
}

// Localize the knowledge required to read/write a mesh into one function
// written in such a way that you can both read and write by passing
// in one serializer type or another.
// This function needs to be careful to request alignment after every write of a
// buffer that may leave us unaligned.  The easiest way to be correct is to request
// alignment a lot.  The hardest way is to use knowledge of the datatypes and
// only request alignment when necessary.
template<typename TSerializer>
void serialize(TSerializer &serializer, Mesh &mesh)
{
    quint8 *baseAddr = reinterpret_cast<quint8 *>(&mesh);
    serializer.streamify(mesh.m_vertexBuffer.m_entries);
    serializer.align();

    for (quint32 entry = 0, numItems = mesh.m_vertexBuffer.m_entries.size(); entry < numItems; ++entry) {
        MeshVertexBufferEntry &entryData = mesh.m_vertexBuffer.m_entries.index(baseAddr, entry);
        serializer.streamifyCharPointerOffset(entryData.m_nameOffset);
        serializer.align();
    }
    serializer.streamify(mesh.m_vertexBuffer.m_data);
    serializer.align();
    serializer.streamify(mesh.m_indexBuffer.m_data);
    serializer.align();
    serializer.streamify(mesh.m_subsets);
    serializer.align();

    for (quint32 entry = 0, numItems = mesh.m_subsets.size(); entry < numItems; ++entry) {
        MeshSubset &theSubset = const_cast<MeshSubset &>(mesh.m_subsets.index(baseAddr, entry));
        serializer.streamify(theSubset.m_name);
        serializer.align();
    }
    serializer.streamify(mesh.m_joints);
    serializer.align();
}

struct TotallingSerializer
{
    quint32 m_numBytes;
    const quint8 *m_baseAddress;
    TotallingSerializer(const quint8 *inBaseAddr) : m_numBytes(0), m_baseAddress(inBaseAddr) {}
    template<typename TDataType>
    void streamify(const OffsetDataRef<TDataType> &data)
    {
        m_numBytes += data.size() * sizeof(TDataType);
    }
    void streamify(const char *data)
    {
        if (data == nullptr)
            data = "";
        quint32 len = (quint32)strlen(data) + 1;
        m_numBytes += 4;
        m_numBytes += len;
    }
    void streamifyCharPointerOffset(quint32 inOffset)
    {
        if (inOffset) {
            const char *dataPtr = (const char *)(inOffset + m_baseAddress);
            streamify(dataPtr);
        } else
            streamify("");
    }
    bool needsAlignment() const { return getAlignmentAmount() > 0; }
    quint32 getAlignmentAmount() const { return 4 - (m_numBytes % 4); }
    void align()
    {
        if (needsAlignment())
            m_numBytes += getAlignmentAmount();
    }
};

struct ByteWritingSerializer
{
    QIODevice &m_stream;
    TotallingSerializer m_byteCounter;
    quint8 *m_baseAddress;
    ByteWritingSerializer(QIODevice &str, quint8 *inBaseAddress)
        : m_stream(str), m_byteCounter(inBaseAddress), m_baseAddress(inBaseAddress)
    {
    }

    template<typename TDataType>
    void streamify(const OffsetDataRef<TDataType> &data)
    {
        m_byteCounter.streamify(data);
        const int written = m_stream.write(reinterpret_cast<const char *>(data.begin(m_baseAddress)), data.size() * sizeof(TDataType));
        (void)written;
    }
    void streamify(const char *data)
    {
        m_byteCounter.streamify(data);
        if (data == nullptr)
            data = "";
        quint32 len = (quint32)strlen(data) + 1;
        int written = m_stream.write(reinterpret_cast<const char *>(&len), sizeof(quint32));
        written = m_stream.write(data, len);
        (void)written;
    }
    void streamifyCharPointerOffset(quint32 inOffset)
    {
        const char *dataPtr = (const char *)(inOffset + m_baseAddress);
        streamify(dataPtr);
    }

    void align()
    {
        if (m_byteCounter.needsAlignment()) {
            quint8 buffer[] = { 0, 0, 0, 0 };
            const int written = m_stream.write(reinterpret_cast<const char *>(buffer), m_byteCounter.getAlignmentAmount());
            (void)written;
            m_byteCounter.align();
        }
    }
};

struct MemoryAssigningSerializer
{
    const quint8 *m_memory;
    const quint8 *m_baseAddress;
    quint32 m_size;
    TotallingSerializer m_byteCounter;
    bool m_failure;
    MemoryAssigningSerializer(const quint8 *data, quint32 size, quint32 startOffset)
        : m_memory(data + startOffset), m_baseAddress(data), m_size(size), m_byteCounter(data), m_failure(false)
    {
        // We expect 4 byte aligned memory to begin with
        Q_ASSERT((((size_t)m_memory) % 4) == 0);
    }

    template<typename TDataType>
    void streamify(const OffsetDataRef<TDataType> &_data)
    {
        OffsetDataRef<TDataType> &data = const_cast<OffsetDataRef<TDataType> &>(_data);
        if (m_failure) {
            data.m_size = 0;
            data.m_offset = 0;
            return;
        }
        quint32 current = m_byteCounter.m_numBytes;
        m_byteCounter.streamify(_data);
        if (m_byteCounter.m_numBytes > m_size) {
            data.m_size = 0;
            data.m_offset = 0;
            m_failure = true;
            return;
        }
        quint32 numBytes = m_byteCounter.m_numBytes - current;
        if (numBytes) {
            data.m_offset = (quint32)(m_memory - m_baseAddress);
            updateMemoryBuffer(numBytes);
        } else {
            data.m_offset = 0;
            data.m_size = 0;
        }
    }
    void streamify(const char *&_data)
    {
        quint32 len;
        m_byteCounter.m_numBytes += 4;
        if (m_byteCounter.m_numBytes > m_size) {
            _data = "";
            m_failure = true;
            return;
        }
        memcpy(&len, m_memory, 4);
        updateMemoryBuffer(4);
        m_byteCounter.m_numBytes += len;
        if (m_byteCounter.m_numBytes > m_size) {
            _data = "";
            m_failure = true;
            return;
        }
        _data = (const char *)m_memory;
        updateMemoryBuffer(len);
    }
    void streamifyCharPointerOffset(quint32 &inOffset)
    {
        const char *dataPtr;
        streamify(dataPtr);
        inOffset = (quint32)(dataPtr - (const char *)m_baseAddress);
    }
    void align()
    {
        if (m_byteCounter.needsAlignment()) {
            quint32 numBytes = m_byteCounter.getAlignmentAmount();
            m_byteCounter.align();
            updateMemoryBuffer(numBytes);
        }
    }
    void updateMemoryBuffer(quint32 numBytes) { m_memory += numBytes; }
};

inline quint32 getMeshDataSize(Mesh &mesh)
{
    TotallingSerializer s(reinterpret_cast<quint8 *>(&mesh));
    serialize(s, mesh);
    return s.m_numBytes;
}

template<typename TDataType>
quint32 nextIndex(const quint8 *inBaseAddress, const OffsetDataRef<quint8> data, quint32 idx)
{
    quint32 numItems = data.size() / sizeof(TDataType);
    if (idx < numItems) {
        const TDataType *dataPtr(reinterpret_cast<const TDataType *>(data.begin(inBaseAddress)));
        return dataPtr[idx];
    } else {
        Q_UNREACHABLE();
        return 0;
    }
}

template<typename TDataType>
quint32 nextIndex(const QByteArray &data, quint32 idx)
{
    quint32 numItems = data.size() / sizeof(TDataType);
    if (idx < numItems) {
        const TDataType *dataPtr(reinterpret_cast<const TDataType *>(data.begin()));
        return dataPtr[idx];
    } else {
        Q_ASSERT(false);
        return 0;
    }
}

inline quint32 nextIndex(const QByteArray &inData, QSSGRenderComponentType inCompType, quint32 idx)
{
    if (inData.size() == 0)
        return idx;
    switch (inCompType) {
    case QSSGRenderComponentType::UnsignedInteger8:
        return nextIndex<quint8>(inData, idx);
    case QSSGRenderComponentType::Integer8:
        return nextIndex<quint8>(inData, idx);
    case QSSGRenderComponentType::UnsignedInteger16:
        return nextIndex<quint16>(inData, idx);
    case QSSGRenderComponentType::Integer16:
        return nextIndex<qint16>(inData, idx);
    case QSSGRenderComponentType::UnsignedInteger32:
        return nextIndex<quint32>(inData, idx);
    case QSSGRenderComponentType::Integer32:
        return nextIndex<qint32>(inData, idx);
    default:
        // Invalid index buffer index type.
        Q_ASSERT(false);
    }

    return 0;
}

template<typename TMeshType>
// Not exposed to the outside world
TMeshType *doInitialize(quint16 /*meshFlags*/, QSSGByteView data)
{
    const quint8 *newMem = data.begin();
    quint32 amountLeft = data.size() - sizeof(TMeshType);
    MemoryAssigningSerializer s(newMem, amountLeft, sizeof(TMeshType));
    TMeshType *retval = (TMeshType *)newMem;
    serialize(s, *retval);
    if (s.m_failure)
        return nullptr;
    return retval;
}

static char16_t g_DefaultName[] = { 0 };

const char16_t *Mesh::m_defaultName = g_DefaultName;

template<typename TMeshType>
struct SubsetNameHandler
{
};

template<>
struct SubsetNameHandler<MeshV1>
{
    void assignName(const quint8 * /*v1BaseAddress*/, const MeshSubsetV1 & /*mesh*/, quint8 * /*baseAddress*/, quint8 *& /*nameBuffer*/, MeshSubset &outDest)
    {
        outDest.m_name = OffsetDataRef<char16_t>();
    }
    quint32 nameLength(const MeshSubsetV1 &) { return 0; }
};

template<>
struct SubsetNameHandler<MeshV2>
{
    void assignName(const quint8 *v2BaseAddress, const MeshSubsetV2 &mesh, quint8 *baseAddress, quint8 *&nameBuffer, MeshSubset &outDest)
    {
        outDest.m_name.m_size = mesh.m_name.m_size;
        outDest.m_name.m_offset = (quint32)(nameBuffer - baseAddress);
        quint32 dtypeSize = mesh.m_name.m_size * 2;
        memcpy(nameBuffer, mesh.m_name.begin(v2BaseAddress), dtypeSize);
        nameBuffer += dtypeSize;
    }
    quint32 nameLength(const MeshSubsetV2 &mesh) { return (mesh.m_name.size() + 1) * 2; }
};

quint32 getAlignedOffset(quint32 offset, quint32 align)
{
    Q_ASSERT(align > 0);
    const quint32 leftover = (align > 0) ? offset % align : 0;
    if (leftover)
        return offset + (align - leftover);
    return offset;
}

template<typename TPreviousMeshType>
Mesh *createMeshFromPreviousMesh(TPreviousMeshType *temp)
{
    quint32 newMeshSize = sizeof(Mesh);
    quint8 *tempBaseAddress = reinterpret_cast<quint8 *>(temp);
    quint32 alignment = sizeof(void *);

    quint32 vertBufferSize = getAlignedOffset(temp->m_vertexBuffer.m_data.size(), alignment);
    newMeshSize += vertBufferSize;
    quint32 entryDataSize = temp->m_vertexBuffer.m_entries.size() * sizeof(MeshVertexBufferEntry);
    newMeshSize += entryDataSize;
    quint32 indexBufferSize = getAlignedOffset(temp->m_indexBuffer.m_data.size(), alignment);
    newMeshSize += indexBufferSize;
    quint32 entryNameSize = 0;
    for (quint32 entryIdx = 0, entryEnd = temp->m_vertexBuffer.m_entries.size(); entryIdx < entryEnd; ++entryIdx) {
        const QSSGRenderVertexBufferEntry theEntry = temp->m_vertexBuffer.m_entries.index(tempBaseAddress, entryIdx).toVertexBufferEntry(tempBaseAddress);
        const char *namePtr = theEntry.m_name;
        if (namePtr == nullptr)
            namePtr = "";

        entryNameSize += (quint32)strlen(theEntry.m_name) + 1;
    }
    entryNameSize = getAlignedOffset(entryNameSize, alignment);

    newMeshSize += entryNameSize;
    quint32 subsetBufferSize = temp->m_subsets.size() * sizeof(MeshSubset);
    newMeshSize += subsetBufferSize;
    quint32 nameLength = 0;
    for (quint32 subsetIdx = 0, subsetEnd = temp->m_subsets.size(); subsetIdx < subsetEnd; ++subsetIdx) {
        nameLength += SubsetNameHandler<TPreviousMeshType>().nameLength(temp->m_subsets.index(tempBaseAddress, subsetIdx));
    }
    nameLength = getAlignedOffset(nameLength, alignment);

    newMeshSize += nameLength;

    Mesh *retval = new Mesh();
    quint8 *baseOffset = reinterpret_cast<quint8 *>(retval);
    quint8 *vertBufferData = baseOffset + sizeof(Mesh);
    quint8 *entryBufferData = vertBufferData + vertBufferSize;
    quint8 *entryNameBuffer = entryBufferData + entryDataSize;
    quint8 *indexBufferData = entryNameBuffer + entryNameSize;
    quint8 *subsetBufferData = indexBufferData + indexBufferSize;
    quint8 *nameData = subsetBufferData + subsetBufferSize;

    retval->m_drawMode = temp->m_drawMode;
    retval->m_winding = temp->m_winding;
    retval->m_vertexBuffer = temp->m_vertexBuffer;
    retval->m_vertexBuffer.m_data.m_offset = (quint32)(vertBufferData - baseOffset);
    retval->m_vertexBuffer.m_entries.m_offset = (quint32)(entryBufferData - baseOffset);
    memcpy(vertBufferData, temp->m_vertexBuffer.m_data.begin(tempBaseAddress), temp->m_vertexBuffer.m_data.size());
    memcpy(entryBufferData, temp->m_vertexBuffer.m_entries.begin(tempBaseAddress), entryDataSize);
    for (quint32 idx = 0, __numItems = (quint32)temp->m_vertexBuffer.m_entries.size(); idx < __numItems; ++idx) {
        const MeshVertexBufferEntry &src = temp->m_vertexBuffer.m_entries.index(tempBaseAddress, idx);
        MeshVertexBufferEntry &dest = retval->m_vertexBuffer.m_entries.index(baseOffset, idx);

        const char *targetName = reinterpret_cast<const char *>(src.m_nameOffset + tempBaseAddress);
        if (src.m_nameOffset == 0)
            targetName = "";
        quint32 nameLen = (quint32)strlen(targetName) + 1;
        dest.m_nameOffset = (quint32)(entryNameBuffer - baseOffset);
        memcpy(entryNameBuffer, targetName, nameLen);
        entryNameBuffer += nameLen;
    }

    retval->m_indexBuffer = temp->m_indexBuffer;
    retval->m_indexBuffer.m_data.m_offset = (quint32)(indexBufferData - baseOffset);
    memcpy(indexBufferData, temp->m_indexBuffer.m_data.begin(tempBaseAddress), temp->m_indexBuffer.m_data.size());

    retval->m_subsets.m_size = temp->m_subsets.m_size;
    retval->m_subsets.m_offset = (quint32)(subsetBufferData - baseOffset);

    for (quint32 idx = 0, numItems = (quint32)temp->m_subsets.size(); idx < numItems; ++idx) {
        MeshSubset &dest = const_cast<MeshSubset &>(retval->m_subsets.index(baseOffset, idx));
        const typename TPreviousMeshType::TSubsetType &src = temp->m_subsets.index(tempBaseAddress, idx);
        dest.m_count = src.m_count;
        dest.m_offset = src.m_offset;
        dest.m_bounds = src.m_bounds;
        SubsetNameHandler<TPreviousMeshType>().assignName(tempBaseAddress, src, baseOffset, nameData, dest);
    }
    return retval;
}

QSSGBounds3 Mesh::calculateSubsetBounds(const QSSGRenderVertexBufferEntry &inEntry,
                                          const QByteArray &inVertxData,
                                          quint32 inStride,
                                          const QByteArray &inIndexData,
                                          QSSGRenderComponentType inIndexCompType,
                                          quint32 inSubsetCount,
                                          quint32 inSubsetOffset)
{
    QSSGBounds3 retval = QSSGBounds3();
    const QSSGRenderVertexBufferEntry &entry(inEntry);
    if (entry.m_componentType != QSSGRenderComponentType::Float32 || entry.m_numComponents != 3) {
        Q_ASSERT(false);
        return retval;
    }

    const quint8 *beginPtr = reinterpret_cast<const quint8 *>(inVertxData.constData());
    quint32 numBytes = inVertxData.size();
    quint32 dataStride = inStride;
    quint32 posOffset = entry.m_firstItemOffset;
    // The loop below could be template specialized *if* we wanted to do this.
    // and the perf of the existing loop was determined to be a problem.
    // Else I would rather stay way from the template specialization.
    for (quint32 idx = 0, __numItems = (quint32)inSubsetCount; idx < __numItems; ++idx) {
        quint32 dataIdx = nextIndex(inIndexData, inIndexCompType, idx + inSubsetOffset);
        quint32 finalOffset = (dataIdx * dataStride) + posOffset;
        if (finalOffset + sizeof(Vec3) <= numBytes) {
            const quint8 *dataPtr = beginPtr + finalOffset;
            const auto vec3 = *reinterpret_cast<const Vec3 *>(dataPtr);
            retval.include(QVector3D(vec3.x, vec3.y, vec3.z));
        } else {
            Q_ASSERT(false);
        }
    }

    return retval;
}

void Mesh::save(QIODevice &outStream) const
{
    Mesh &mesh(const_cast<Mesh &>(*this));
    quint8 *baseAddress = reinterpret_cast<quint8 *>(&mesh);
    quint32 meshSize = sizeof(Mesh);
    quint32 meshDataSize = getMeshDataSize(mesh);
    quint32 numBytes = meshSize + meshDataSize;
    MeshDataHeader header(numBytes);
    int written = outStream.write(reinterpret_cast<const char *>(&header), sizeof(MeshDataHeader)); // 12 bytes
    written = outStream.write(reinterpret_cast<const char *>(this), sizeof(Mesh));
    (void)written;
    ByteWritingSerializer writer(outStream, baseAddress);
    serialize(writer, mesh);
}

bool Mesh::save(const char *inFilePath) const
{
    QFile file(QString::fromLocal8Bit(inFilePath));
    if (!file.open(QIODevice::ReadWrite)) {
        Q_ASSERT(false);
        return false;
    }

    save(file);
    file.close();
    return true;
}

Mesh *Mesh::load(QIODevice &inStream)
{
    MeshDataHeader header;
    inStream.read(reinterpret_cast<char *>(&header), sizeof(MeshDataHeader));
    Q_ASSERT(header.m_fileId == MeshDataHeader::getFileId());
    if (header.m_fileId != MeshDataHeader::getFileId())
        return nullptr;
    if (header.m_fileVersion < 1 || header.m_fileVersion > MeshDataHeader::getCurrentFileVersion())
        return nullptr;
    if (header.m_sizeInBytes < sizeof(Mesh))
        return nullptr;
    char *meshBufferData = reinterpret_cast<char *>(::malloc(header.m_sizeInBytes));
    qint64 sizeRead = inStream.read(meshBufferData, header.m_sizeInBytes);
    //    QByteArray meshBuffer = inStream.read(header.m_sizeInBytes);
    if (sizeRead == header.m_sizeInBytes) {
        QSSGByteView meshBuffer = toByteView(meshBufferData, header.m_sizeInBytes);
        if (header.m_fileVersion == 1) {
            MeshV1 *temp = doInitialize<MeshV1>(header.m_headerFlags, meshBuffer);
            if (temp == nullptr)
                goto failure;
            return createMeshFromPreviousMesh(temp);

        } else if (header.m_fileVersion == 2) {
            MeshV2 *temp = doInitialize<MeshV2>(header.m_headerFlags, meshBuffer);
            if (temp == nullptr)
                goto failure;
            return createMeshFromPreviousMesh(temp);
        } else {
            Mesh *retval = initialize(header.m_fileVersion, header.m_headerFlags, meshBuffer);
            if (retval == nullptr)
                goto failure;
            return retval;
        }
    }

failure:
    Q_ASSERT(false);
    ::free(meshBufferData);
    return nullptr;
}

Mesh *Mesh::load(const char *inFilePath)
{
    QFile file(QString::fromLocal8Bit(inFilePath));
    if (!file.open(QIODevice::ReadOnly)) {
        Q_ASSERT(false);
        return nullptr;
    }

    auto mesh = load(file);
    file.close();
    return mesh;
}

Mesh *Mesh::initialize(quint16 meshVersion, quint16 meshFlags, QSSGByteView data)
{
    if (meshVersion != MeshDataHeader::getCurrentFileVersion())
        return nullptr;
    return doInitialize<Mesh>(meshFlags, data);
}

quint32 Mesh::saveMulti(QIODevice &inStream, quint32 inId) const
{
    quint32 nextId = 1;
    MeshMultiHeader tempHeader;
    MeshMultiHeader *theHeader = nullptr;
    MeshMultiHeader *theWriteHeader = nullptr;

    qint64 newMeshStartPos = 0;
    if (inStream.size() != 0) {
        theHeader = loadMultiHeader(inStream);
        if (theHeader == nullptr) {
            Q_ASSERT(false);
            return 0;
        }
        quint8 *headerBaseAddr = reinterpret_cast<quint8 *>(theHeader);
        for (quint32 idx = 0, end = theHeader->m_entries.size(); idx < end; ++idx) {
            if (inId != 0) {
                Q_ASSERT(inId != theHeader->m_entries.index(headerBaseAddr, idx).m_meshId);
            }
            nextId = qMax(nextId, theHeader->m_entries.index(headerBaseAddr, idx).m_meshId + 1);
        }
        newMeshStartPos = sizeof(MeshMultiHeader) + theHeader->m_entries.size() * sizeof(MeshMultiEntry);
        theWriteHeader = theHeader;
    } else {
        theWriteHeader = &tempHeader;
    }

    // inStream.SetPosition(-newMeshStartPos, SeekPosition::End);
    inStream.seek(inStream.size() - newMeshStartPos); // ### not sure about this one
    qint64 meshOffset = inStream.pos();

    save(inStream);

    if (inId != 0)
        nextId = inId;
    quint8 *theWriteBaseAddr = reinterpret_cast<quint8 *>(theWriteHeader);
    // Now write a new header out.
    int written = inStream.write(reinterpret_cast<char *>(theWriteHeader->m_entries.begin(theWriteBaseAddr)),
                   theWriteHeader->m_entries.size());
    MeshMultiEntry newEntry(static_cast<qint64>(meshOffset), nextId);
    written = inStream.write(reinterpret_cast<char *>(&newEntry), sizeof(MeshMultiEntry));
    theWriteHeader->m_entries.m_size++;
    written = inStream.write(reinterpret_cast<char *>(theWriteHeader), sizeof(MeshMultiHeader));
    (void)written;

    return static_cast<quint32>(nextId);
}

quint32 Mesh::saveMulti(const char *inFilePath) const
{
    QFile file(QString::fromLocal8Bit(inFilePath));
    if (!file.open(QIODevice::ReadWrite)) {
        Q_ASSERT(false);
        return (quint32)-1;
    }

    quint32 id = saveMulti(file);
    file.close();
    return id;
}

MultiLoadResult Mesh::loadMulti(QIODevice &inStream, quint32 inId)
{
    MeshMultiHeader *theHeader(loadMultiHeader(inStream));
    if (theHeader == nullptr) {
        return MultiLoadResult();
    }
    quint64 fileOffset = (quint64)-1;
    quint32 theId = inId;
    quint8 *theHeaderBaseAddr = reinterpret_cast<quint8 *>(theHeader);
    bool foundMesh = false;
    for (quint32 idx = 0, end = theHeader->m_entries.size(); idx < end && !foundMesh; ++idx) {
        const MeshMultiEntry &theEntry(theHeader->m_entries.index(theHeaderBaseAddr, idx));
        if (theEntry.m_meshId == inId || (inId == 0 && theEntry.m_meshId > theId)) {
            if (theEntry.m_meshId == inId)
                foundMesh = true;
            theId = qMax(theId, (quint32)theEntry.m_meshId);
            fileOffset = theEntry.m_meshOffset;
        }
    }
    Mesh *retval = nullptr;
    if (fileOffset == (quint64)-1) {
        goto endFunction;
    }

    inStream.seek(static_cast<qint64>(fileOffset));
    retval = load(inStream);
endFunction:
    return MultiLoadResult(retval, theId);
}

MultiLoadResult Mesh::loadMulti(const char *inFilePath, quint32 inId)
{
    QFile file(QString::fromLocal8Bit(inFilePath));
    if (!file.open(QIODevice::ReadOnly)) {
        Q_ASSERT(false);
        return MultiLoadResult();
    }

    auto result = loadMulti(file, inId);
    file.close();
    return result;
}

bool Mesh::isMulti(QIODevice &inStream)
{
    MeshMultiHeader theHeader;
    inStream.seek(inStream.size() - ((qint64)(sizeof(MeshMultiHeader))));
    quint32 numBytes = inStream.read(reinterpret_cast<char *>(&theHeader), sizeof(MeshMultiHeader));
    if (numBytes != sizeof(MeshMultiHeader))
        return false;
    return theHeader.m_version == MeshMultiHeader::getMultiStaticVersion();
}

MeshMultiHeader *Mesh::loadMultiHeader(QIODevice &inStream)
{
    MeshMultiHeader theHeader;
    inStream.seek(inStream.size() - ((qint64)sizeof(MeshMultiHeader)));
    quint32 numBytes = inStream.read(reinterpret_cast<char *>(&theHeader), sizeof(MeshMultiHeader));
    if (numBytes != sizeof(MeshMultiHeader) || theHeader.m_fileId != MeshMultiHeader::getMultiStaticFileId()
        || theHeader.m_version > MeshMultiHeader::getMultiStaticVersion()) {
        return nullptr;
    }
    size_t allocSize = sizeof(MeshMultiHeader) + theHeader.m_entries.m_size * sizeof(MeshMultiEntry);
    quint8 *baseAddr = static_cast<quint8 *>(::malloc(allocSize));
    if (baseAddr == nullptr) {
        Q_ASSERT(false);
        return nullptr;
    }
    MeshMultiHeader *retval = new (baseAddr) MeshMultiHeader(theHeader);
    quint8 *entryData = baseAddr + sizeof(MeshMultiHeader);
    retval->m_entries.m_offset = (quint32)(entryData - baseAddr);
    inStream.seek(inStream.size() - ((qint64)allocSize));

    numBytes = inStream.read(reinterpret_cast<char *>(entryData), retval->m_entries.m_size * sizeof(MeshMultiEntry));
    if (numBytes != retval->m_entries.m_size * sizeof(MeshMultiEntry)) {
        Q_ASSERT(false);
        delete retval;
        retval = nullptr;
    }
    return retval;
}

MeshMultiHeader *Mesh::loadMultiHeader(const char *inFilePath)
{
    QFile file(QString::fromLocal8Bit(inFilePath));
    if (!file.open(QIODevice::ReadOnly)) {
        Q_ASSERT(false);
        return nullptr;
    }

    auto result = loadMultiHeader(file);
    file.close();
    return result;
}

quint32 GetHighestId(MeshMultiHeader *inHeader)
{
    if (inHeader == nullptr) {
        Q_ASSERT(false);
        return 0;
    }
    quint8 *baseHeaderAddr = reinterpret_cast<quint8 *>(inHeader);
    quint32 highestId = 0;
    for (quint32 idx = 0, end = inHeader->m_entries.size(); idx < end; ++idx)
        highestId = qMax(highestId, inHeader->m_entries.index(baseHeaderAddr, idx).m_meshId);
    return highestId;
}

quint32 Mesh::getHighestMultiVersion(QIODevice &inStream)
{
    return GetHighestId(loadMultiHeader(inStream));
}

quint32 Mesh::getHighestMultiVersion(const char *inFilePath)
{
    QFile file(QString::fromLocal8Bit(inFilePath));
    if (!file.open(QIODevice::ReadOnly)) {
        Q_ASSERT(false);
        return (quint32)-1;
    }

    auto result = getHighestMultiVersion(file);
    file.close();
    return result;
}

namespace {

#if 0
MeshBuilderVBufEntry ToEntry(const QVector<float> &data, const char *name, quint32 numComponents)
{
    return MeshBuilderVBufEntry(name, QByteArray(reinterpret_cast<const char *>(data.data())), QSSGRenderComponentTypes::Float32, numComponents);
}
#endif

struct DynamicVBuf
{
    quint32 m_stride;
    QVector<QSSGRenderVertexBufferEntry> m_vertexBufferEntries;
    QByteArray m_vertexData;

    void clear()
    {
        m_stride = 0;
        m_vertexBufferEntries.clear();
        m_vertexData.clear();
    }
};
struct DynamicIndexBuf
{
    QSSGRenderComponentType m_compType;
    QByteArray m_indexData;
    void clear() { m_indexData.clear(); }
};

struct SubsetDesc
{
    quint32 m_count{ 0 };
    quint32 m_offset{ 0 };

    QSSGBounds3 m_bounds;
    QString m_name;
    SubsetDesc(quint32 c, quint32 off) : m_count(c), m_offset(off) {}
    SubsetDesc() = default;
};

class MeshBuilderImpl : public QSSGMeshBuilder
{
    DynamicVBuf m_vertexBuffer;
    DynamicIndexBuf m_indexBuffer;
    QVector<Joint> m_joints;
    QVector<SubsetDesc> m_meshSubsetDescs;
    QSSGRenderDrawMode m_drawMode;
    QSSGRenderWinding m_winding;
    QByteArray m_newIndexBuffer;
    QVector<quint8> m_meshBuffer;

public:
    MeshBuilderImpl() { reset(); }
    ~MeshBuilderImpl() override { reset(); }
    void release() override { delete this; }
    void reset() override
    {
        m_vertexBuffer.clear();
        m_indexBuffer.clear();
        m_joints.clear();
        m_meshSubsetDescs.clear();
        m_drawMode = QSSGRenderDrawMode::Triangles;
        m_winding = QSSGRenderWinding::CounterClockwise;
        m_meshBuffer.clear();
    }

    void setDrawParameters(QSSGRenderDrawMode drawMode, QSSGRenderWinding winding) override
    {
        m_drawMode = drawMode;
        m_winding = winding;
    }

    // Somewhat burly method to interleave the data as tightly as possible
    // while taking alignment into account.
    bool setVertexBuffer(const QVector<MeshBuilderVBufEntry> &entries) override
    {
        quint32 currentOffset = 0;
        quint32 bufferAlignment = 0;
        quint32 numItems = 0;
        bool retval = true;
        for (quint32 idx = 0, __numItems = (quint32)entries.size(); idx < __numItems; ++idx) {
            const MeshBuilderVBufEntry &entry(entries[idx]);
            // Ignore entries with no data.
            if (entry.m_data.begin() == nullptr || entry.m_data.size() == 0)
                continue;

            quint32 alignment = getSizeOfType(entry.m_componentType);
            bufferAlignment = qMax(bufferAlignment, alignment);
            quint32 byteSize = alignment * entry.m_numComponents;

            if (entry.m_data.size() % alignment != 0) {
                Q_ASSERT(false);
                retval = false;
            }

            quint32 localNumItems = entry.m_data.size() / byteSize;
            if (numItems == 0) {
                numItems = localNumItems;
            } else if (numItems != localNumItems) {
                Q_ASSERT(false);
                retval = false;
                numItems = qMin(numItems, localNumItems);
            }
            // Lots of platforms can't handle non-aligned data.
            // so ensure we are aligned.
            currentOffset = getAlignedOffset(currentOffset, alignment);
            QSSGRenderVertexBufferEntry vbufEntry(entry.m_name, entry.m_componentType, entry.m_numComponents, currentOffset);
            m_vertexBuffer.m_vertexBufferEntries.push_back(vbufEntry);
            currentOffset += byteSize;
        }
        m_vertexBuffer.m_stride = getAlignedOffset(currentOffset, bufferAlignment);

        // Packed interleave the data
        for (quint32 idx = 0; idx < numItems; ++idx) {
            quint32 dataOffset = 0;
            for (qint32 entryIdx = 0; entryIdx < entries.size(); ++entryIdx) {
                const MeshBuilderVBufEntry &entry(entries[entryIdx]);
                // Ignore entries with no data.
                if (entry.m_data.begin() == nullptr || entry.m_data.size() == 0)
                    continue;

                quint32 alignment = (quint32)getSizeOfType(entry.m_componentType);
                quint32 byteSize = alignment * entry.m_numComponents;
                quint32 offset = byteSize * idx;
                quint32 newOffset = getAlignedOffset(dataOffset, alignment);
                QBuffer vertexDataBuffer(&m_vertexBuffer.m_vertexData);
                vertexDataBuffer.open(QIODevice::WriteOnly | QIODevice::Append);
                if (newOffset != dataOffset) {
                    QByteArray filler(newOffset - dataOffset, '\0');
                    vertexDataBuffer.write(filler);
                }
                vertexDataBuffer.write(entry.m_data.begin() + offset, byteSize);
                vertexDataBuffer.close();
                dataOffset = newOffset + byteSize;
            }
            Q_ASSERT(dataOffset == m_vertexBuffer.m_stride);
        }
        return retval;
    }

    void setVertexBuffer(const QVector<QSSGRenderVertexBufferEntry> &entries, quint32 stride, QByteArray data) override
    {
        for (quint32 idx = 0, __numItems = (quint32)entries.size(); idx < __numItems; ++idx) {
            m_vertexBuffer.m_vertexBufferEntries.push_back(entries[idx]);
        }
        QBuffer vertexDataBuffer(&m_vertexBuffer.m_vertexData);
        vertexDataBuffer.open(QIODevice::WriteOnly);
        vertexDataBuffer.write(data);
        vertexDataBuffer.close();
        if (stride == 0) {
            // Calculate the stride of the buffer using the vbuf entries
            for (quint32 idx = 0, __numItems = (quint32)entries.size(); idx < __numItems; ++idx) {
                const QSSGRenderVertexBufferEntry &entry(entries[idx]);
                stride = qMax(stride,
                              (quint32)(entry.m_firstItemOffset
                                        + (entry.m_numComponents * getSizeOfType(entry.m_componentType))));
            }
        }
        m_vertexBuffer.m_stride = stride;
    }

    void setIndexBuffer(const QByteArray &data, QSSGRenderComponentType comp) override
    {
        m_indexBuffer.m_compType = comp;
        QBuffer indexBuffer(&m_indexBuffer.m_indexData);
        indexBuffer.open(QIODevice::WriteOnly);
        indexBuffer.write(data);
        indexBuffer.close();
    }

    void addJoint(qint32 jointID, qint32 parentID, const float *invBindPose, const float *localToGlobalBoneSpace) override
    {
        m_joints.push_back(Joint(jointID, parentID, invBindPose, localToGlobalBoneSpace));
    }

    SubsetDesc createSubset(const char16_t *inName, quint32 count, quint32 offset)
    {
        if (inName == nullptr)
            inName = u"";
        SubsetDesc retval(count, offset);
        retval.m_name = QString::fromUtf16(inName);
        return retval;
    }

    // indexBuffer std::numeric_limits<quint32>::max() means no index buffer.
    // count of std::numeric_limits<quint32>::max() means use all available items.
    // offset means exactly what you would think.  Offset is in item size, not bytes.
    void addMeshSubset(const char16_t *inName, quint32 count, quint32 offset, quint32 boundsPositionEntryIndex) override
    {
        SubsetDesc retval = createSubset(inName, count, offset);
        if (boundsPositionEntryIndex != std::numeric_limits<quint32>::max()) {
            retval.m_bounds = Mesh::calculateSubsetBounds(m_vertexBuffer.m_vertexBufferEntries[boundsPositionEntryIndex],
                                                          m_vertexBuffer.m_vertexData,
                                                          m_vertexBuffer.m_stride,
                                                          m_indexBuffer.m_indexData,
                                                          m_indexBuffer.m_compType,
                                                          count,
                                                          offset);
        }
        m_meshSubsetDescs.push_back(retval);
    }

    void addMeshSubset(const char16_t *inName, quint32 count, quint32 offset, const QSSGBounds3 &inBounds) override
    {
        SubsetDesc retval = createSubset(inName, count, offset);
        retval.m_bounds = inBounds;
        m_meshSubsetDescs.push_back(retval);
    }

    // We connect sub meshes which habe the same material
    void connectSubMeshes() override
    {
        if (m_meshSubsetDescs.size() < 2) {
            // nothing to do
            return;
        }

        quint32 matDuplicates = 0;

        // as a pre-step we check if we have duplicate material at all
        for (quint32 i = 0, subsetEnd = m_meshSubsetDescs.size(); i < subsetEnd && !matDuplicates; ++i) {
            SubsetDesc &currentSubset = m_meshSubsetDescs[i];

            for (quint32 j = 0, subsetEnd = m_meshSubsetDescs.size(); j < subsetEnd; ++j) {
                SubsetDesc &theSubset = m_meshSubsetDescs[j];

                if (i == j)
                    continue;

                if (currentSubset.m_name == theSubset.m_name) {
                    matDuplicates++;
                    break; // found a duplicate bail out
                }
            }
        }

        // did we find some duplicates?
        if (matDuplicates) {
            QVector<SubsetDesc> newMeshSubsetDescs;
            QVector<SubsetDesc>::iterator theIter;
            QString curMatName;
            m_newIndexBuffer.clear();

            for (theIter = m_meshSubsetDescs.begin(); theIter != m_meshSubsetDescs.end(); ++theIter) {
                bool bProcessed = false;

                for (QVector<SubsetDesc>::iterator iter = newMeshSubsetDescs.begin(); iter != newMeshSubsetDescs.end(); ++iter) {
                    if (theIter->m_name == iter->m_name) {
                        bProcessed = true;
                        break;
                    }
                }

                if (bProcessed)
                    continue;

                curMatName = theIter->m_name;

                quint32 theIndexCompSize = (quint32)getSizeOfType(m_indexBuffer.m_compType);
                // get pointer to indices
                char *theIndices = (m_indexBuffer.m_indexData.begin()) + (theIter->m_offset * theIndexCompSize);
                // write new offset
                theIter->m_offset = m_newIndexBuffer.size() / theIndexCompSize;
                // store indices
                QBuffer newIndexBuffer(&m_newIndexBuffer);
                newIndexBuffer.open(QIODevice::WriteOnly);
                newIndexBuffer.write(theIndices, theIter->m_count * theIndexCompSize);

                for (int j = 0, subsetEnd = m_meshSubsetDescs.size(); j < subsetEnd; ++j) {
                    if (theIter == &m_meshSubsetDescs[j])
                        continue;

                    SubsetDesc &theSubset = m_meshSubsetDescs[j];

                    if (curMatName == theSubset.m_name) {
                        // get pointer to indices
                        char *theIndices = (m_indexBuffer.m_indexData.data()) + (theSubset.m_offset * theIndexCompSize);
                        // store indices
                        newIndexBuffer.write(theIndices, theSubset.m_count * theIndexCompSize);
                        // increment indices count
                        theIter->m_count += theSubset.m_count;
                    }
                    newIndexBuffer.close();
                }

                newMeshSubsetDescs.push_back(*theIter);
            }

            m_meshSubsetDescs.clear();
            m_meshSubsetDescs = newMeshSubsetDescs;
            m_indexBuffer.m_indexData.clear();
            QBuffer indexBuffer(&m_indexBuffer.m_indexData);
            indexBuffer.open(QIODevice::WriteOnly);
            indexBuffer.write(m_newIndexBuffer);
            indexBuffer.close();
            // compute new bounding box
            for (theIter = m_meshSubsetDescs.begin(); theIter != m_meshSubsetDescs.end(); ++theIter) {
                theIter->m_bounds = Mesh::calculateSubsetBounds(m_vertexBuffer.m_vertexBufferEntries[0],
                                                                m_vertexBuffer.m_vertexData,
                                                                m_vertexBuffer.m_stride,
                                                                m_indexBuffer.m_indexData,
                                                                m_indexBuffer.m_compType,
                                                                theIter->m_count,
                                                                theIter->m_offset);
            }
        }
    }

    // Here is the NVTriStrip magic.
    void optimizeMesh() override
    {
        if (getSizeOfType(m_indexBuffer.m_compType) != 2) {
            // we currently re-arrange unsigned int indices.
            // this is because NvTriStrip only supports short indices
            Q_ASSERT(getSizeOfType(m_indexBuffer.m_compType) == 4);
            return;
        }
    }

    template<typename TDataType>
    static void assign(quint8 *inBaseAddress, quint8 *inDataAddress, OffsetDataRef<TDataType> &inBuffer, const QByteArray &inDestData)
    {
        inBuffer.m_offset = (quint32)(inDataAddress - inBaseAddress);
        inBuffer.m_size = inDestData.size();
        memcpy(inDataAddress, inDestData.data(), inDestData.size());
    }
    template<typename TDataType>
    static void assign(quint8 *inBaseAddress, quint8 *inDataAddress, OffsetDataRef<TDataType> &inBuffer, const QVector<TDataType> &inDestData)
    {
        inBuffer.m_offset = (quint32)(inDataAddress - inBaseAddress);
        inBuffer.m_size = inDestData.size();
        memcpy(inDataAddress, inDestData.data(), inDestData.size());
    }
    template<typename TDataType>
    static void assign(quint8 *inBaseAddress, quint8 *inDataAddress, OffsetDataRef<TDataType> &inBuffer, quint32 inDestSize)
    {
        inBuffer.m_offset = (quint32)(inDataAddress - inBaseAddress);
        inBuffer.m_size = inDestSize;
    }
    // Return the current mesh.  This is only good for this function call, item may change or be
    // released
    // due to any further function calls.
    Mesh &getMesh() override
    {
        quint32 meshSize = sizeof(Mesh);
        quint32 alignment = sizeof(void *);
        quint32 vertDataSize = getAlignedOffset(m_vertexBuffer.m_vertexData.size(), alignment);
        meshSize += vertDataSize;
        quint32 entrySize = m_vertexBuffer.m_vertexBufferEntries.size() * sizeof(QSSGRenderVertexBufferEntry);
        meshSize += entrySize;
        quint32 entryNameSize = 0;
        for (quint32 idx = 0, end = m_vertexBuffer.m_vertexBufferEntries.size(); idx < end; ++idx) {
            const QSSGRenderVertexBufferEntry &theEntry(m_vertexBuffer.m_vertexBufferEntries[idx]);
            const char *entryName = theEntry.m_name;
            if (entryName == nullptr)
                entryName = "";
            entryNameSize += (quint32)(strlen(theEntry.m_name)) + 1;
        }
        entryNameSize = getAlignedOffset(entryNameSize, alignment);
        meshSize += entryNameSize;
        quint32 indexBufferSize = getAlignedOffset(m_indexBuffer.m_indexData.size(), alignment);
        meshSize += indexBufferSize;
        quint32 subsetSize = m_meshSubsetDescs.size() * sizeof(MeshSubset);
        quint32 nameSize = 0;
        for (quint32 idx = 0, end = m_meshSubsetDescs.size(); idx < end; ++idx) {
            if (!m_meshSubsetDescs[idx].m_name.isEmpty())
                nameSize += m_meshSubsetDescs[idx].m_name.size() + 1;
        }
        nameSize *= sizeof(char16_t);
        nameSize = getAlignedOffset(nameSize, alignment);

        meshSize += subsetSize + nameSize;
        quint32 jointsSize = m_joints.size() * sizeof(Joint);
        meshSize += jointsSize;
        m_meshBuffer.resize(meshSize);
        quint8 *baseAddress = m_meshBuffer.data();
        Mesh *retval = reinterpret_cast<Mesh *>(baseAddress);
        retval->m_drawMode = m_drawMode;
        retval->m_winding = m_winding;
        quint8 *vertBufferData = baseAddress + sizeof(Mesh);
        quint8 *vertEntryData = vertBufferData + vertDataSize;
        quint8 *vertEntryNameData = vertEntryData + entrySize;
        quint8 *indexBufferData = vertEntryNameData + entryNameSize;
        quint8 *subsetBufferData = indexBufferData + indexBufferSize;
        quint8 *nameBufferData = subsetBufferData + subsetSize;
        quint8 *jointBufferData = nameBufferData + nameSize;

        retval->m_vertexBuffer.m_stride = m_vertexBuffer.m_stride;
        assign(baseAddress, vertBufferData, retval->m_vertexBuffer.m_data, m_vertexBuffer.m_vertexData);
        retval->m_vertexBuffer.m_entries.m_size = m_vertexBuffer.m_vertexBufferEntries.size();
        retval->m_vertexBuffer.m_entries.m_offset = (quint32)(vertEntryData - baseAddress);
        for (quint32 idx = 0, end = m_vertexBuffer.m_vertexBufferEntries.size(); idx < end; ++idx) {
            const QSSGRenderVertexBufferEntry &theEntry(m_vertexBuffer.m_vertexBufferEntries[idx]);
            MeshVertexBufferEntry &theDestEntry(retval->m_vertexBuffer.m_entries.index(baseAddress, idx));
            theDestEntry.m_componentType = theEntry.m_componentType;
            theDestEntry.m_firstItemOffset = theEntry.m_firstItemOffset;
            theDestEntry.m_numComponents = theEntry.m_numComponents;
            const char *targetName = theEntry.m_name;
            if (targetName == nullptr)
                targetName = "";

            quint32 entryNameLen = (quint32)(strlen(targetName)) + 1;
            theDestEntry.m_nameOffset = (quint32)(vertEntryNameData - baseAddress);
            memcpy(vertEntryNameData, theEntry.m_name, entryNameLen);
            vertEntryNameData += entryNameLen;
        }

        retval->m_indexBuffer.m_componentType = m_indexBuffer.m_compType;
        assign(baseAddress, indexBufferData, retval->m_indexBuffer.m_data, m_indexBuffer.m_indexData);
        assign(baseAddress, subsetBufferData, retval->m_subsets, m_meshSubsetDescs.size());
        for (quint32 idx = 0, end = m_meshSubsetDescs.size(); idx < end; ++idx) {
            SubsetDesc &theDesc = m_meshSubsetDescs[idx];
            MeshSubset &theSubset = reinterpret_cast<MeshSubset *>(subsetBufferData)[idx];
            theSubset.m_bounds = theDesc.m_bounds;
            theSubset.m_count = theDesc.m_count;
            theSubset.m_offset = theDesc.m_offset;
            if (!theDesc.m_name.isEmpty()) {
                theSubset.m_name.m_size = theDesc.m_name.size() + 1;
                theSubset.m_name.m_offset = (quint32)(nameBufferData - baseAddress);
                std::transform(theDesc.m_name.begin(),
                               theDesc.m_name.end(),
                               reinterpret_cast<char16_t *>(nameBufferData),
                               [](QChar c) { return static_cast<char16_t>(c.unicode()); });
                reinterpret_cast<char16_t *>(nameBufferData)[theDesc.m_name.size()] = 0;
                nameBufferData += (theDesc.m_name.size() + 1) * sizeof(char16_t);
            } else {
                theSubset.m_name.m_size = 0;
                theSubset.m_name.m_offset = 0;
            }
        }
        assign(baseAddress, jointBufferData, retval->m_joints, m_joints);
        return *retval;
    }

    Mesh *buildMesh(const MeshData &meshData, QString &error, const QSSGBounds3 &inBounds) override
    {
        // Do some basic validation of the meshData
        if (meshData.m_vertexBuffer.size() == 0) {
            error = QObject::tr("Vertex buffer empty");
            return nullptr;
        }
        if (meshData.m_attributeCount == 0) {
            error = QObject::tr("No attributes defined");
            return nullptr;
        }

        reset();
        setDrawParameters(static_cast<QSSGRenderDrawMode>(meshData.m_primitiveType),
                          QSSGRenderWinding::CounterClockwise);

        // The expectation is that the vertex buffer included in meshData is already properly
        // formatted and doesn't need further processing.

        // Validate attributes
        QVector<QSSGRenderVertexBufferEntry> vBufEntries;
        QSSGRenderComponentType indexBufferComponentType = QSSGRenderComponentType::Unknown;
        int indexBufferTypeSize = 0;
        for (int i = 0; i < meshData.m_attributeCount; ++i) {
            const MeshData::Attribute &att = meshData.m_attributes[i];
            auto componentType = static_cast<QSSGRenderComponentType>(att.componentType);
            if (att.semantic == MeshData::Attribute::IndexSemantic) {
                indexBufferComponentType = componentType;
                indexBufferTypeSize = att.typeSize();
            } else {
                const char *name = nullptr;
                switch (att.semantic) {
                case MeshData::Attribute::PositionSemantic:
                    name = Mesh::getPositionAttrName();
                    break;
                case MeshData::Attribute::NormalSemantic:
                    name = Mesh::getNormalAttrName();
                    break;
                case MeshData::Attribute::TexCoordSemantic:
                    name = Mesh::getUVAttrName();
                    break;
                case MeshData::Attribute::TangentSemantic:
                    name = Mesh::getTexTanAttrName();
                    break;
                case MeshData::Attribute::BinormalSemantic:
                    name = Mesh::getTexBinormalAttrName();
                    break;
                default:
                    error = QObject::tr("Warning: Invalid attribute semantic: %1")
                            .arg(att.semantic);
                    return nullptr;
                }
                vBufEntries << QSSGRenderVertexBufferEntry(name, componentType,
                                                           unsigned(att.componentCount()),
                                                           unsigned(att.offset));
            }
        }
        setVertexBuffer(vBufEntries, unsigned(meshData.m_stride), meshData.m_vertexBuffer);

        int vertexCount = 0;
        if (indexBufferComponentType != QSSGRenderComponentType::Unknown) {
            setIndexBuffer(meshData.m_indexBuffer, indexBufferComponentType);
            vertexCount = meshData.m_indexBuffer.size() / indexBufferTypeSize;
        } else {
            vertexCount = meshData.m_vertexBuffer.size() / meshData.m_stride;
        }

        addMeshSubset(Mesh::m_defaultName, unsigned(vertexCount), 0, inBounds);

        return &getMesh();
    }
};
}

QSSGMeshBuilder::~QSSGMeshBuilder() = default;

QSSGRef<QSSGMeshBuilder> QSSGMeshBuilder::createMeshBuilder()
{
    return QSSGRef<QSSGMeshBuilder>(new MeshBuilderImpl());
}
}

QT_END_NAMESPACE
