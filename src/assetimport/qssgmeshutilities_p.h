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

#ifndef QSSGMESHUTILITIES_P_H
#define QSSGMESHUTILITIES_P_H

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

#include <QtQuick3DAssetImport/private/qtquick3dassetimportglobal_p.h>

#include <QtQuick3DUtils/private/qssgbounds3_p.h>

#include <QtQuick3DUtils/private/qssgrenderbasetypes_p.h>

#include <QtCore/QString>
#include <QtCore/QByteArray>
#include <QtCore/QIODevice>

QT_BEGIN_NAMESPACE

namespace QSSGMeshUtilities {

struct RuntimeMeshData // for custom geometry (QQuick3DGeometry)
{
    enum PrimitiveType { // must match QSSGRenderGeometry::PrimitiveType
        Points = 0,
        LineStrip,
        LineLoop,
        Lines,
        TriangleStrip,
        TriangleFan,
        Triangles, // Default primitive type
        Patches
    };

    struct Attribute {
        enum Semantic {
            IndexSemantic = 0,
            PositionSemantic,                       // attr_pos
            NormalSemantic,                         // attr_norm
            TexCoordSemantic,                       // attr_uv0
            TangentSemantic,                        // attr_textan
            BinormalSemantic,                       // attr_binormal
            JointSemantic,                          // attr_joints
            WeightSemantic,                         // attr_weights
            ColorSemantic,                          // attr_color
            TargetPositionSemantic,                 // attr_tpos0
            TargetNormalSemantic,                   // attr_tnorm0
            TargetTangentSemantic,                  // attr_ttan0
            TargetBinormalSemantic,                 // attr_tbinorm0
            TexCoord1Semantic,                      // attr_uv1
            TexCoord0Semantic = TexCoordSemantic    // attr_uv0
        };
        enum ComponentType { // must match QSSGRenderGeometry::Attribute::ComponentType
            U8Type = 0,
            I8Type,
            U16Type,
            I16Type,
            U32Type,
            I32Type,
            U64Type,
            I64Type,
            F16Type,
            F32Type,
            F64Type
        };

        int typeSize() const
        {
            switch (componentType) {
            case U8Type:  return 1;
            case I8Type:  return 1;
            case U16Type: return 2;
            case I16Type: return 2;
            case U32Type: return 4;
            case I32Type: return 4;
            case U64Type: return 8;
            case I64Type: return 8;
            case F16Type: return 2;
            case F32Type: return 4;
            case F64Type: return 8;
            default:
                Q_ASSERT(false);
                return 0;
            }
        }

        int componentCount() const
        {
            switch (semantic) {
            case IndexSemantic:             return 1;
            case PositionSemantic:          return 3;
            case NormalSemantic:            return 3;
            case TexCoord0Semantic:         return 2;
            case TexCoord1Semantic:         return 2;
            case TangentSemantic:           return 3;
            case BinormalSemantic:          return 3;
            case JointSemantic:             return 4;
            case WeightSemantic:            return 4;
            case ColorSemantic:             return 4;
            case TargetPositionSemantic:    return 3;
            case TargetNormalSemantic:      return 3;
            case TargetTangentSemantic:     return 3;
            case TargetBinormalSemantic:    return 3;
            default:
                Q_ASSERT(false);
                return 0;
            }
        }

        Semantic semantic = PositionSemantic;
        ComponentType componentType = F32Type;
        int offset = 0;
    };

    static const int MAX_ATTRIBUTES = 8;

    void clear()
    {
        m_vertexBuffer.clear();
        m_indexBuffer.clear();
        m_attributeCount = 0;
        m_primitiveType = Triangles;
    }

    QByteArray m_vertexBuffer;
    QByteArray m_indexBuffer;

    Attribute m_attributes[MAX_ATTRIBUTES];
    int m_attributeCount = 0;
    PrimitiveType m_primitiveType = Triangles;
    int m_stride = 0;
};

template<typename DataType>
struct OffsetDataRef
{
    quint32 m_offset;
    quint32 m_size;
    OffsetDataRef() : m_offset(0), m_size(0) {}
    DataType *begin(quint8 *inBase)
    {
        DataType *value = reinterpret_cast<DataType *>(inBase + m_offset);
        return value;
    }
    DataType *end(quint8 *inBase) { return begin(inBase) + m_size; }
    const DataType *begin(const quint8 *inBase) const { return reinterpret_cast<const DataType *>(inBase + m_offset); }
    const DataType *end(const quint8 *inBase) const { return begin(inBase) + m_size; }
    quint32 size() const { return m_size; }
    bool empty() const { return m_size == 0; }
    DataType &index(quint8 *inBase, quint32 idx)
    {
        Q_ASSERT(idx < m_size);
        return begin(inBase)[idx];
    }
    const DataType &index(const quint8 *inBase, quint32 idx) const
    {
        Q_ASSERT(idx < m_size);
        return begin(inBase)[idx];
    }
};

struct MeshVertexBufferEntry
{
    quint32 m_nameOffset;
    /** Datatype of the this entry points to in the buffer */
    QSSGRenderComponentType m_componentType;
    /** Number of components of each data member. 1,2,3, or 4.  Don't be stupid.*/
    quint32 m_numComponents;
    /** Offset from the beginning of the buffer of the first item */
    quint32 m_firstItemOffset;
    MeshVertexBufferEntry()
        : m_nameOffset(0), m_componentType(QSSGRenderComponentType::Float32), m_numComponents(3), m_firstItemOffset(0)
    {
    }
    QSSGRenderVertexBufferEntry toVertexBufferEntry(quint8 *inBaseAddress) const
    {
        const char *nameBuffer = "";
        if (m_nameOffset)
            nameBuffer = reinterpret_cast<const char *>(inBaseAddress + m_nameOffset);
        return QSSGRenderVertexBufferEntry(nameBuffer, m_componentType, m_numComponents, m_firstItemOffset);
    }
};

struct VertexBuffer
{
    OffsetDataRef<MeshVertexBufferEntry> m_entries;
    quint32 m_stride;
    OffsetDataRef<quint8> m_data;
    VertexBuffer(OffsetDataRef<MeshVertexBufferEntry> entries, quint32 stride, OffsetDataRef<quint8> data)
        : m_entries(entries), m_stride(stride), m_data(data)
    {
    }
    VertexBuffer() : m_stride(0) {}
};

struct IndexBuffer
{
    // Component types must be either UnsignedInt16 or UnsignedInt8 in order for the
    // graphics hardware to deal with the buffer correctly.
    QSSGRenderComponentType m_componentType;
    OffsetDataRef<quint8> m_data;
    // Either quint8 or quint16 component types are allowed by the underlying rendering
    // system, so you would be wise to stick with those.
    IndexBuffer(QSSGRenderComponentType compType, OffsetDataRef<quint8> data)
        : m_componentType(compType), m_data(data)
    {
    }
    IndexBuffer() : m_componentType(QSSGRenderComponentType::UnsignedInteger16) {}
};

struct MeshSubset
{
    // Item (index or vertex) count.
    quint32 m_count;
    // Offset is in items, not bytes.
    quint32 m_offset;
    // Bounds of this subset.  This is filled in by the builder
    // see AddMeshSubset
    QSSGBounds3 m_bounds;

    // Subsets have to be named else artists will be unable to use
    // a mesh with multiple subsets as they won't have any idea
    // while part of the model a given mesh actually maps to.
    OffsetDataRef<char16_t> m_name;

    MeshSubset(quint32 count, quint32 off, const QSSGBounds3 &bounds, OffsetDataRef<char16_t> inName)
        : m_count(count), m_offset(off), m_bounds(bounds), m_name(inName)
    {
    }
    MeshSubset() : m_count(quint32(-1)), m_offset(0), m_bounds() {}
};

struct Joint
{
    qint32 m_jointID;
    qint32 m_parentID;
    float m_invBindPose[16];
    float m_localToGlobalBoneSpace[16];

    Joint(qint32 jointID, qint32 parentID, const float *invBindPose, const float *localToGlobalBoneSpace)
        : m_jointID(jointID), m_parentID(parentID)
    {
        ::memcpy(m_invBindPose, invBindPose, sizeof(float) * 16);
        ::memcpy(m_localToGlobalBoneSpace, localToGlobalBoneSpace, sizeof(float) * 16);
    }
    Joint() : m_jointID(-1), m_parentID(-1)
    {
        ::memset(m_invBindPose, 0, sizeof(float) * 16);
        ::memset(m_localToGlobalBoneSpace, 0, sizeof(float) * 16);
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

struct Mesh;

// Result of a multi-load operation.  This returns both the mesh
// and the id of the mesh that was loaded.
struct MultiLoadResult
{
    Mesh *m_mesh;
    quint32 m_id;
    MultiLoadResult(Mesh *inMesh, quint32 inId) : m_mesh(inMesh), m_id(inId) {}
    MultiLoadResult() : m_mesh(nullptr), m_id(0) {}
    operator Mesh *() { return m_mesh; }
};

struct Q_QUICK3DASSETIMPORT_EXPORT Mesh
{
    VertexBuffer m_vertexBuffer;
    IndexBuffer m_indexBuffer;
    OffsetDataRef<MeshSubset> m_subsets;
    OffsetDataRef<Joint> m_joints;
    QSSGRenderDrawMode m_drawMode;
    QSSGRenderWinding m_winding;

    Mesh() : m_drawMode(QSSGRenderDrawMode::Triangles), m_winding(QSSGRenderWinding::CounterClockwise) {}
    Mesh(VertexBuffer vbuf,
         IndexBuffer ibuf,
         const OffsetDataRef<MeshSubset> &insts,
         const OffsetDataRef<Joint> &joints,
         QSSGRenderDrawMode drawMode = QSSGRenderDrawMode::Triangles,
         QSSGRenderWinding winding = QSSGRenderWinding::CounterClockwise)
        : m_vertexBuffer(vbuf), m_indexBuffer(ibuf), m_subsets(insts), m_joints(joints), m_drawMode(drawMode), m_winding(winding)
    {
    }

    static const char *getPositionAttrName() { return "attr_pos"; }
    static const char *getNormalAttrName() { return "attr_norm"; }
    static const char *getUV0AttrName() { return "attr_uv0"; }
    static const char *getUV1AttrName() { return "attr_uv1"; }
    static const char *getTexTanAttrName() { return "attr_textan"; }
    static const char *getTexBinormalAttrName() { return "attr_binormal"; }
    static const char *getColorAttrName() { return "attr_color"; }
    static const char *getJointAttrName() { return "attr_joints"; }
    static const char *getWeightAttrName() { return "attr_weights"; }
    static const char *getMorphTargetAttrNamePrefix() { return "attr_t"; }
    static const char *getTargetPositionAttrName(int idx)
    {
        switch (idx) {
            case 0:
                return "attr_tpos0";
            case 1:
                return "attr_tpos1";
            case 2:
                return "attr_tpos2";
            case 3:
                return "attr_tpos3";
            case 4:
                return "attr_tpos4";
            case 5:
                return "attr_tpos5";
            case 6:
                return "attr_tpos6";
            case 7:
                return "attr_tpos7";
        }
        return "attr_unsupported";
    }
    static const char *getTargetNormalAttrName(int idx)
    {
        switch (idx) {
            case 0:
                return "attr_tnorm0";
            case 1:
                return "attr_tnorm1";
            case 2:
                return "attr_tnorm2";
            case 3:
                return "attr_tnorm3";
        }
        return "attr_unsupported";
    }
    static const char *getTargetTangentAttrName(int idx)
    {
        switch (idx) {
            case 0:
                return "attr_ttan0";
            case 1:
                return "attr_ttan1";
        }
        return "attr_unsupported";
    }
    static const char *getTargetBinormalAttrName(int idx)
    {
        switch (idx) {
            case 0:
                return "attr_tbinorm0";
            case 1:
                return "attr_tbinorm1";
        }
        return "attr_unsupported";
    }

    // Run through the vertex buffer items indicated by subset Assume vbuf
    // entry[posEntryIndex] is the position entry This entry has to be 3
    // component float. Using this entry and the (possibly empty) index buffer
    // along with the (possibly emtpy) logical vbuf data return a bounds of the
    // given vertex buffer.
    static QSSGBounds3 calculateSubsetBounds(const QSSGRenderVertexBufferEntry &inEntry,
                                               const QByteArray &inVertxData,
                                               quint32 inStride,
                                               const QByteArray &inIndexData,
                                               QSSGRenderComponentType inIndexCompType,
                                               quint32 inSubsetCount,
                                               quint32 inSubsetOffset);

    void save(QIODevice &outStream) const;
    static Mesh *load(QIODevice &inStream);

    // Create a mesh given this header, and that data.  data.size() must match
    // header.SizeInBytes.  The mesh returned starts a data[0], so however data
    // was allocated is how the mesh should be deallocated.
    static Mesh *initialize(quint16 meshVersion, quint16 meshFlags, QSSGByteView data);

    quint32 saveMulti(QIODevice &inStream, quint32 inId = 0) const;
    static MultiLoadResult loadMulti(QIODevice &inStream, quint32 inId);
    static bool isMulti(QIODevice &inStream);
    static MeshMultiHeader *loadMultiHeader(QIODevice &inStream);
};

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

struct MeshBuilderVBufEntry
{
    const char *m_name;
    QByteArray m_data;
    QSSGRenderComponentType m_componentType;
    quint32 m_numComponents;
    MeshBuilderVBufEntry() : m_name(nullptr), m_componentType(QSSGRenderComponentType::Float32), m_numComponents(0)
    {
    }
    MeshBuilderVBufEntry(const char *name, const QByteArray &data, QSSGRenderComponentType componentType, quint32 numComponents)
        : m_name(name), m_data(data), m_componentType(componentType), m_numComponents(numComponents)
    {
    }
};

class Q_QUICK3DASSETIMPORT_EXPORT QSSGMeshBuilder
{
public:
    QAtomicInt ref;

    QSSGMeshBuilder();
    ~QSSGMeshBuilder();
    void reset();

    // Set the vertex buffer and have the mesh builder interleave the data for you
    bool setVertexBuffer(const QVector<MeshBuilderVBufEntry> &entries);
    // Set the vertex buffer from interleaved data.
    void setVertexBuffer(const QVector<QSSGRenderVertexBufferEntry> &entries, quint32 stride, const QByteArray &data);
    // The builder (and the majority of the rest of the product) only supports unsigned 16 bit
    // indexes
    void setIndexBuffer(const QByteArray &data, QSSGRenderComponentType comp);
    // Assets if the supplied parameters are out of range.
    void addJoint(qint32 jointID, qint32 parentID, const float *invBindPose, const float *localToGlobalBoneSpace);

    // Add a subset, which equates roughly to a draw call. If the mesh has an
    // index buffer then this subset refers to that index buffer, else it is
    // assumed to index into the vertex buffer. count and offset do exactly
    // what they seem to do, while boundsPositionEntryIndex, if set to
    // something other than std::numeric_limits<quint32>::max(), drives the
    // calculation of the aa-bounds of the subset using calculateSubsetBounds.
    void addMeshSubset(const char16_t *inSubsetName,
                       quint32 count,
                       quint32 offset,
                       quint32 boundsPositionEntryIndex = std::numeric_limits<quint32>::max());

    void addMeshSubset(const char16_t *inSubsetName,
                       quint32 count,
                       quint32 offset,
                       const QSSGBounds3 &inBounds);

    // Alternative to setVertexBuffer() et al for models with custom geometry.
    // Returns true if successful. When successful, getMesh() can be called.
    bool setRuntimeData(const RuntimeMeshData &data, QString &error, const QSSGBounds3 &inBounds);

    // Builds and returns the current mesh. The returned reference is valid
    // only until the builder is alive or is reset.
    Mesh &getMesh();

private:
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

    SubsetDesc createSubset(const char16_t *inName, quint32 count, quint32 offset)
    {
        if (inName == nullptr)
            inName = u"";
        SubsetDesc retval(count, offset);
        retval.m_name = QString::fromUtf16(inName);
        return retval;
    }

    DynamicVBuf m_vertexBuffer;
    DynamicIndexBuf m_indexBuffer;
    QVector<Joint> m_joints;
    QVector<SubsetDesc> m_meshSubsetDescs;
    QSSGRenderDrawMode m_drawMode;
    QSSGRenderWinding m_winding;
    QByteArray m_newIndexBuffer;
    QVector<quint8> m_meshBuffer;
};

} // end QSSGMeshUtilities namespace

QT_END_NAMESPACE

#endif // QSSGMESHUTILITIES_P_H
