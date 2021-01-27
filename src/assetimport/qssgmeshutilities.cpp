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
#include <QtQuick3DUtils/private/qssgdataref_p.h>

QT_BEGIN_NAMESPACE

namespace OldMesh {

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
    quint32 numItems = quint32(data.size() / sizeof(TDataType));
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

quint32 getAlignedOffset(quint32 offset, quint32 align)
{
    Q_ASSERT(align > 0);
    const quint32 leftover = (align > 0) ? offset % align : 0;
    if (leftover)
        return offset + (align - leftover);
    return offset;
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
    for (quint32 idx = 0, __numItems = (quint32)inSubsetCount; idx < __numItems; ++idx) {
        quint32 dataIdx = nextIndex(inIndexData, inIndexCompType, idx + inSubsetOffset);
        quint32 finalOffset = (dataIdx * dataStride) + posOffset;
        float v[3];
        if (finalOffset + sizeof(v) <= numBytes) {
            memcpy(v, beginPtr + finalOffset, sizeof(v));
            retval.include(QVector3D(v[0], v[1], v[2]));
        } else {
            Q_ASSERT(false);
        }
    }

    return retval;
}

struct MeshDataHeader
{
    static quint32 getFileId() { return quint32(-929005747); }
    static quint16 getCurrentFileVersion() { return 3; }
    quint32 m_fileId;
    quint16 m_fileVersion;
    quint16 m_headerFlags;
    quint32 m_sizeInBytes;
    MeshDataHeader(quint32 size = 0)
        : m_fileId(getFileId()), m_fileVersion(getCurrentFileVersion()), m_sizeInBytes(size)
    {
    }
};

// Tells us what offset a mesh with this ID starts.
struct MeshMultiEntry
{
    quint64 m_meshOffset;
    quint32 m_meshId;
    quint32 m_padding;
    MeshMultiEntry() : m_meshOffset(0), m_meshId(0), m_padding(0) {}
    MeshMultiEntry(quint64 mo, quint32 meshId) : m_meshOffset(mo), m_meshId(meshId), m_padding(0) {}
};

// The multi headers are actually saved at the end of the file.
// Thus when you append to the file we overwrite the last header
// then write out a new header structure.
// The last 8 bytes of the file contain the multi header.
// The previous N*8 bytes contain the mesh entries.
struct MeshMultiHeader
{
    quint32 m_fileId;
    quint32 m_version;
    OffsetDataRef<MeshMultiEntry> m_entries;
    static quint32 getMultiStaticFileId() { return 555777497U; }
    static quint32 getMultiStaticVersion() { return 1; }

    MeshMultiHeader() : m_fileId(getMultiStaticFileId()), m_version(getMultiStaticVersion()) {}
};

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

namespace {

template<typename TDataType>
static void assign(quint8 *inBaseAddress, quint8 *inDataAddress, OffsetDataRef<TDataType> &inBuffer, const QByteArray &inDestData)
{
    inBuffer.m_offset = (quint32)(inDataAddress - inBaseAddress);
    inBuffer.m_size = inDestData.size();
    memcpy(inDataAddress, inDestData.constData(), inDestData.size());
}
template<typename TDataType>
static void assign(quint8 *inBaseAddress, quint8 *inDataAddress, OffsetDataRef<TDataType> &inBuffer, const QVector<TDataType> &inDestData)
{
    inBuffer.m_offset = (quint32)(inDataAddress - inBaseAddress);
    inBuffer.m_size = inDestData.size();
    memcpy(inDataAddress, inDestData.constData(), inDestData.size());
}
template<typename TDataType>
static void assign(quint8 *inBaseAddress, quint8 *inDataAddress, OffsetDataRef<TDataType> &inBuffer, quint32 inDestSize)
{
    inBuffer.m_offset = (quint32)(inDataAddress - inBaseAddress);
    inBuffer.m_size = inDestSize;
}

} // namespace

QSSGMeshBuilder::QSSGMeshBuilder()
{
    reset();
}

QSSGMeshBuilder::~QSSGMeshBuilder()
{
    reset();
}

void QSSGMeshBuilder::reset()
{
    m_vertexBuffer.clear();
    m_indexBuffer.clear();
    m_joints.clear();
    m_meshSubsetDescs.clear();
    m_drawMode = QSSGRenderDrawMode::Triangles;
    m_winding = QSSGRenderWinding::CounterClockwise;
    m_meshBuffer.clear();
}

bool QSSGMeshBuilder::setVertexBuffer(const QVector<MeshBuilderVBufEntry> &entries)
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
    m_vertexBuffer.m_vertexData.clear();

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
            if (newOffset != dataOffset) {
                QByteArray filler(newOffset - dataOffset, '\0');
                m_vertexBuffer.m_vertexData.append(filler);
            }
            m_vertexBuffer.m_vertexData.append(entry.m_data.begin() + offset, byteSize);
            dataOffset = newOffset + byteSize;
        }
        Q_ASSERT(dataOffset == m_vertexBuffer.m_stride);
    }
    return retval;
}

void QSSGMeshBuilder::setIndexBuffer(const QByteArray &data, QSSGRenderComponentType comp)
{
    m_indexBuffer.m_compType = comp;
    m_indexBuffer.m_indexData = data;
}

void QSSGMeshBuilder::addJoint(qint32 jointID, qint32 parentID, const float *invBindPose, const float *localToGlobalBoneSpace)
{
    m_joints.push_back(Joint(jointID, parentID, invBindPose, localToGlobalBoneSpace));
}

void QSSGMeshBuilder::addMeshSubset(const char16_t *inName, quint32 count, quint32 offset, quint32 boundsPositionEntryIndex)
{
    if (inName == nullptr)
        inName = u"";
    SubsetDesc retval(count, offset);
    retval.m_name = QString::fromUtf16(inName);

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

Mesh &QSSGMeshBuilder::getMesh()
{
    quint32 meshSize = sizeof(Mesh);
    quint32 alignment = sizeof(void *);
    quint32 vertDataSize = getAlignedOffset(m_vertexBuffer.m_vertexData.size(), alignment);
    meshSize += vertDataSize;
    quint32 entrySize = quint32(m_vertexBuffer.m_vertexBufferEntries.size()) * sizeof(QSSGRenderVertexBufferEntry);
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
    quint32 subsetSize = quint32(m_meshSubsetDescs.size()) * sizeof(MeshSubset);
    quint32 nameSize = 0;
    for (quint32 idx = 0, end = m_meshSubsetDescs.size(); idx < end; ++idx) {
        if (!m_meshSubsetDescs[idx].m_name.isEmpty())
            nameSize += m_meshSubsetDescs[idx].m_name.size() + 1;
    }
    nameSize *= sizeof(char16_t);
    nameSize = getAlignedOffset(nameSize, alignment);

    meshSize += subsetSize + nameSize;
    quint32 jointsSize = quint32(m_joints.size()) * sizeof(Joint);
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

} // namespace OldMesh

namespace NewMesh {

MeshInternal::MultiMeshInfo MeshInternal::loadFileHeader(QIODevice *device)
{
    // fileId, fileVersion, offset, count
    static const size_t MULTI_HEADER_STRUCT_SIZE = 16;
    const qint64 multiHeaderStartOffset = device->size() - qint64(MULTI_HEADER_STRUCT_SIZE);

    device->seek(multiHeaderStartOffset);
    QDataStream inputStream(device);
    inputStream.setByteOrder(QDataStream::LittleEndian);

    MultiMeshInfo meshFileInfo;
    inputStream >> meshFileInfo.fileId >> meshFileInfo.fileVersion;

    if (!meshFileInfo.isValid()) {
        qWarning("Mesh file invalid");
        return {};
    }

    quint32 multiEntriesOffset; // unused
    quint32 meshCount;
    inputStream >> multiEntriesOffset >> meshCount;

    // meshOffset, meshId, padding
    static const size_t MULTI_ENTRY_STRUCT_SIZE = 16;

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

quint64 MeshInternal::loadMeshData(QIODevice *device, quint64 offset, Mesh *mesh, MeshDataHeader *header)
{
    device->seek(offset);
    QDataStream inputStream(device);
    inputStream.setByteOrder(QDataStream::LittleEndian);
    inputStream.setFloatingPointPrecision(QDataStream::SinglePrecision);

    // fileId, fileVersion, flags, size
    static const size_t MESH_HEADER_STRUCT_SIZE = 12;

    inputStream >> header->fileId >> header->fileVersion >> header->flags >> header->sizeInBytes;
    if (!header->isValid()) {
        qWarning() << "Mesh data invalid";
        return 0;
    }

    MeshInternal::MeshOffsetTracker offsetTracker(offset + MESH_HEADER_STRUCT_SIZE);
    device->seek(offsetTracker.offset());

    // vertexBuffer, indexBuffer, subsets, joints, drawMode, winding
    static const size_t MESH_STRUCT_SIZE = 56;

    quint32 vertexBufferEntriesOffset;
    quint32 vertexBufferEntriesSize;
    quint32 vertexBufferDataOffset;
    quint32 vertexBufferDataSize;
    inputStream >> vertexBufferEntriesOffset
                >> vertexBufferEntriesSize
                >> mesh->m_vertexBuffer.stride
                >> vertexBufferDataOffset
                >> vertexBufferDataSize;

    quint32 indexBufferComponentType;
    quint32 indexBufferOffset;
    quint32 indexBufferSize;
    inputStream >> indexBufferComponentType
                >> indexBufferOffset
                >> indexBufferSize;
    mesh->m_indexBuffer.componentType = Mesh::ComponentType(indexBufferComponentType);

    quint32 subsetsOffsets;
    quint32 subsetsSize;
    inputStream >> subsetsOffsets >> subsetsSize;

    quint32 jointsOffsets;
    quint32 jointsSize;
    inputStream >> jointsOffsets >> jointsSize;

    quint32 drawMode;
    quint32 winding;
    inputStream >> drawMode >> winding;
    mesh->m_drawMode = Mesh::DrawMode(drawMode);
    mesh->m_winding = Mesh::Winding(winding);

    offsetTracker.advance(MESH_STRUCT_SIZE);

    // vertex buffer entry list: nameOffset, componentType, componentCount, offset
    static const size_t VERTEX_BUFFER_ENTRY_STRUCT_SIZE = 16;

    quint32 entriesByteSize = 0;
    for (quint32 i = 0; i < vertexBufferEntriesSize; ++i) {
        Mesh::VertexBufferEntry vertexBufferEntry;
        quint32 componentType;
        quint32 nameOffset; // ignored
        inputStream >> nameOffset
                    >> componentType
                    >> vertexBufferEntry.componentCount
                    >> vertexBufferEntry.offset;
        vertexBufferEntry.componentType = Mesh::ComponentType(componentType);
        mesh->m_vertexBuffer.entries.append(vertexBufferEntry);
        entriesByteSize += VERTEX_BUFFER_ENTRY_STRUCT_SIZE;
    }
    offsetTracker.alignedAdvance(entriesByteSize);
    device->seek(offsetTracker.offset());

    // vertex buffer entry names
    for (auto &entry : mesh->m_vertexBuffer.entries) {
        quint32 nameLength;
        inputStream >> nameLength;
        offsetTracker.advance(sizeof(quint32));
        const QByteArray nameWithZeroTerminator = device->read(nameLength);
        entry.name = QByteArray(nameWithZeroTerminator.constData(), qMax(0, nameWithZeroTerminator.count() - 1));
        offsetTracker.alignedAdvance(nameLength);
        device->seek(offsetTracker.offset());
    }

    mesh->m_vertexBuffer.data = device->read(vertexBufferDataSize);
    offsetTracker.alignedAdvance(vertexBufferDataSize);
    device->seek(offsetTracker.offset());

    mesh->m_indexBuffer.data = device->read(indexBufferSize);
    offsetTracker.alignedAdvance(indexBufferSize);
    device->seek(offsetTracker.offset());

    // subset list: count, offset, minXYZ, maxXYZ, nameOffset, nameLength
    static const size_t SUBSET_STRUCT_SIZE = 40;

    quint32 subsetByteSize = 0;
    QVector<MeshInternal::Subset> internalSubsets;
    for (quint32 i = 0; i < subsetsSize; ++i) {
        MeshInternal::Subset subset;
        float minX;
        float minY;
        float minZ;
        float maxX;
        float maxY;
        float maxZ;
        quint32 nameOffset;
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
        internalSubsets.append(subset);
        subsetByteSize += SUBSET_STRUCT_SIZE;
    }
    // Basically forces a 4 byte padding after subsets
    offsetTracker.alignedAdvance(subsetByteSize);
    device->seek(offsetTracker.offset());

    for (MeshInternal::Subset &internalSubset : internalSubsets) {
        internalSubset.rawNameUtf16 = device->read(internalSubset.nameLength * 2); //UTF_16_le
        offsetTracker.alignedAdvance(internalSubset.nameLength * 2);
        device->seek(offsetTracker.offset());
    }

    for (const MeshInternal::Subset &internalSubset : internalSubsets)
        mesh->m_subsets.append(internalSubset.toMeshSubset());

    // joint list: jointId, parentId, invBindPose[16], localToGlobal[16]
    static const size_t JOINT_STRUCT_SIZE = 136;

    for (quint32 i = 0; i < jointsSize; ++i) {
        Mesh::Joint joint;
        inputStream >> joint.jointId >> joint.parentId;
        float invBindPos[16];
        for (int j = 0; j < 16; ++j)
            inputStream >> invBindPos[j];
        float localToGlobalBoneSpace[16];
        for (int j = 0; j < 16; ++j)
            inputStream >> localToGlobalBoneSpace[j];
        joint.inverseBindPose = QMatrix4x4(invBindPos);
        joint.localToGlobalBoneSpace = QMatrix4x4(localToGlobalBoneSpace);
        offsetTracker.alignedAdvance(JOINT_STRUCT_SIZE);
        device->seek(offsetTracker.offset());
        mesh->m_joints.append(joint);
    }

    return header->sizeInBytes;
}

Mesh Mesh::loadMesh(QIODevice *device, quint32 id)
{
    MeshInternal::MeshDataHeader header;
    const MeshInternal::MultiMeshInfo meshFileInfo = MeshInternal::loadFileHeader(device);
    auto it = meshFileInfo.meshEntries.constFind(id);
    if (it != meshFileInfo.meshEntries.constEnd()) {
        Mesh mesh;
        quint64 size = MeshInternal::loadMeshData(device, *it, &mesh, &header);
        if (size)
            return mesh;
    } else if (id == 0 && !meshFileInfo.meshEntries.isEmpty()) {
        Mesh mesh;
        quint64 size = MeshInternal::loadMeshData(device, *meshFileInfo.meshEntries.cbegin(), &mesh, &header);
        if (size)
            return mesh;
    }
    return Mesh();
}

QMap<quint32, Mesh> Mesh::loadAll(QIODevice *device)
{
    MeshInternal::MeshDataHeader header;
    const MeshInternal::MultiMeshInfo meshFileInfo = MeshInternal::loadFileHeader(device);
    QMap<quint32, Mesh> meshes;
    for (auto it = meshFileInfo.meshEntries.cbegin(), end = meshFileInfo.meshEntries.cend(); it != end; ++it) {
        Mesh mesh;
        quint64 size = MeshInternal::loadMeshData(device, *it, &mesh, &header);
        if (size)
            meshes.insert(it.key(), mesh);
        else
            qWarning("Failed to find mesh #%u", it.key());
    }
    return meshes;
}

Mesh Mesh::fromRuntimeData(const RuntimeMeshData &data, const QSSGBounds3 &bounds, QString *error)
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

    bool hasIndexBuffer = false;
    ComponentType indexBufferComponentType = ComponentType::UnsignedInt16;
    int indexBufferComponentByteSize = 2;
    for (int i = 0; i < data.m_attributeCount; ++i) {
        const RuntimeMeshData::Attribute &att = data.m_attributes[i];
        if (att.semantic == RuntimeMeshData::Attribute::IndexSemantic) {
            hasIndexBuffer = true;
            indexBufferComponentType = att.componentType;
            indexBufferComponentByteSize = MeshInternal::byteSizeForComponentType(att.componentType);
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
            case RuntimeMeshData::Attribute::TargetPositionSemantic:
                name = MeshInternal::getTargetPositionAttrName(0);
                break;
            case RuntimeMeshData::Attribute::TargetNormalSemantic:
                name = MeshInternal::getTargetNormalAttrName(0);
                break;
            case RuntimeMeshData::Attribute::TargetTangentSemantic:
                name = MeshInternal::getTargetTangentAttrName(0);
                break;
            case RuntimeMeshData::Attribute::TargetBinormalSemantic:
                name = MeshInternal::getTargetBinormalAttrName(0);
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

    Subset subset;
    subset.bounds.min = bounds.minimum;
    subset.bounds.max = bounds.maximum;
    subset.offset = 0;

    if (hasIndexBuffer) {
        mesh.m_indexBuffer.data = data.m_indexBuffer;
        mesh.m_indexBuffer.componentType = indexBufferComponentType;
        subset.count = mesh.m_indexBuffer.data.size() / indexBufferComponentByteSize;
    } else {
        subset.count = mesh.m_vertexBuffer.data.size() / mesh.m_vertexBuffer.stride;
    }

    mesh.m_subsets.append(subset);

    return mesh;
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

} // namespace NewMesh

QT_END_NAMESPACE
