// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qssgmeshbvhbuilder_p.h"

QT_BEGIN_NAMESPACE

QSSGMeshBVHBuilder::QSSGMeshBVHBuilder(const QSSGMesh::Mesh &mesh)
    : m_mesh(mesh)
{
    const QSSGMesh::Mesh::VertexBuffer vb = mesh.vertexBuffer();
    const QSSGMesh::Mesh::IndexBuffer ib = mesh.indexBuffer();
    m_vertexBufferData = vb.data;
    m_indexBufferData = ib.data;
    m_indexBufferComponentType = QSSGRenderComponentType(ib.componentType);
    if (m_indexBufferComponentType == QSSGRenderComponentType::Int16)
        m_indexBufferComponentType = QSSGRenderComponentType::UnsignedInt16;
    else if (m_indexBufferComponentType == QSSGRenderComponentType::Int32)
        m_indexBufferComponentType = QSSGRenderComponentType::UnsignedInt32;

    // Get VertexBuffer Information
    // When using the texture coordinates, UV0 has priority but if the mesh has
    // UV1 without UV0, UV1 will be used instead of UV0.
    for (quint32 entryIdx = 0, entryEnd = vb.entries.size(); entryIdx < entryEnd; ++entryIdx) {
        QSSGRenderVertexBufferEntry entry = vb.entries[entryIdx].toRenderVertexBufferEntry();
        if (!strcmp(entry.m_name, QSSGMesh::MeshInternal::getPositionAttrName())) {
            m_hasPositionData = true;
            m_vertexPosOffset = entry.m_firstItemOffset;
        } else if (!strcmp(entry.m_name, QSSGMesh::MeshInternal::getUV0AttrName())) {
            m_hasUVData = true;
            m_vertexUVOffset = entry.m_firstItemOffset;
        } else if (!m_hasUVData && !strcmp(entry.m_name, QSSGMesh::MeshInternal::getUV1AttrName())) {
            m_hasUVData = true;
            m_vertexUVOffset = entry.m_firstItemOffset;
        }
    }
    m_vertexStride = vb.stride;
}

QSSGMeshBVHBuilder::QSSGMeshBVHBuilder(const QByteArray &vertexBuffer,
                                       int stride,
                                       int posOffset,
                                       bool hasUV,
                                       int uvOffset,
                                       bool hasIndexBuffer,
                                       const QByteArray &indexBuffer,
                                       QSSGRenderComponentType indexBufferType)
{
    m_vertexBufferData = vertexBuffer;
    m_vertexStride = stride;
    m_hasPositionData = true;
    m_vertexPosOffset = posOffset;
    m_hasUVData = hasUV;
    m_vertexUVOffset = uvOffset;
    m_hasIndexBuffer = hasIndexBuffer;
    m_indexBufferData = indexBuffer;
    m_indexBufferComponentType = indexBufferType;
    if (m_indexBufferComponentType == QSSGRenderComponentType::Int16)
        m_indexBufferComponentType = QSSGRenderComponentType::UnsignedInt16;
    else if (m_indexBufferComponentType == QSSGRenderComponentType::Int32)
        m_indexBufferComponentType = QSSGRenderComponentType::UnsignedInt32;
}

QSSGMeshBVH* QSSGMeshBVHBuilder::buildTree()
{
    m_roots.clear();

    // This only works with triangles
    if (m_mesh.isValid() && m_mesh.drawMode() != QSSGMesh::Mesh::DrawMode::Triangles)
        return nullptr;

    // Calculate the bounds for each triangle in whole mesh once
    quint32 indexCount = 0;
    if (m_hasIndexBuffer)
        indexCount = quint32(m_indexBufferData.size() / QSSGBaseTypeHelpers::getSizeOfType(m_indexBufferComponentType));
    else
        indexCount = m_vertexBufferData.size() / m_vertexStride;
    m_triangleBounds = calculateTriangleBounds(0, indexCount);

    // For each submesh, generate a root bvh node
    if (m_mesh.isValid()) {
        const QVector<QSSGMesh::Mesh::Subset> subsets = m_mesh.subsets();
        for (quint32 subsetIdx = 0, subsetEnd = subsets.size(); subsetIdx < subsetEnd; ++subsetIdx) {
            const QSSGMesh::Mesh::Subset &source(subsets[subsetIdx]);
            QSSGMeshBVHNode *root = new QSSGMeshBVHNode();
            // Offsets provided by subset are for the index buffer
            // Convert them to work with the triangle bounds list
            const quint32 triangleOffset = source.offset / 3;
            const quint32 triangleCount = source.count / 3;
            root->boundingData = getBounds(triangleOffset, triangleCount);
            // Recursively split the mesh into a tree of smaller bounding volumns
            root = splitNode(root, triangleOffset, triangleCount);
            m_roots.append(root);
        }
    } else {
        // Custom Geometry only has one subset
        QSSGMeshBVHNode *root = new QSSGMeshBVHNode();
        root->boundingData = getBounds(0, m_triangleBounds.size());
        root = splitNode(root, 0, m_triangleBounds.size());
        m_roots.append(root);
    }
    return new QSSGMeshBVH(m_roots, m_triangleBounds);
}

QVector<QSSGMeshBVHTriangle *> QSSGMeshBVHBuilder::calculateTriangleBounds(quint32 indexOffset, quint32 indexCount) const
{
    QVector<QSSGMeshBVHTriangle *> triangleBounds;
    const quint32 triangleCount = indexCount / 3;

    for (quint32 i = 0; i < triangleCount; ++i) {
        // Get the indices for the triangle
        const quint32 triangleIndex = i * 3 + indexOffset;

        quint32 index1 = triangleIndex + 0;
        quint32 index2 = triangleIndex + 1;
        quint32 index3 = triangleIndex + 2;

        if (m_hasIndexBuffer) {
            index1 = getIndexBufferValue(triangleIndex + 0);
            index2 = getIndexBufferValue(triangleIndex + 1);
            index3 = getIndexBufferValue(triangleIndex + 2);
        }

        QSSGMeshBVHTriangle *triangle = new QSSGMeshBVHTriangle();

        triangle->vertex1 = getVertexBufferValuePosition(index1);
        triangle->vertex2 = getVertexBufferValuePosition(index2);
        triangle->vertex3 = getVertexBufferValuePosition(index3);
        triangle->uvCoord1 = getVertexBufferValueUV(index1);
        triangle->uvCoord2 = getVertexBufferValueUV(index2);
        triangle->uvCoord3 = getVertexBufferValueUV(index3);

        triangle->bounds.include(triangle->vertex1);
        triangle->bounds.include(triangle->vertex2);
        triangle->bounds.include(triangle->vertex3);
        triangleBounds.append(triangle);
    }
    return triangleBounds;
}

quint32 QSSGMeshBVHBuilder::getIndexBufferValue(quint32 index) const
{
    quint32 result = 0;
    const quint32 indexCount = quint32(m_indexBufferData.size() / QSSGBaseTypeHelpers::getSizeOfType(m_indexBufferComponentType));
    Q_ASSERT(index < indexCount);

    if (m_indexBufferComponentType == QSSGRenderComponentType::UnsignedInt16) {
        QSSGDataView<quint16> shortIndex(reinterpret_cast<const quint16 *>(m_indexBufferData.begin()), indexCount);
        result = shortIndex[index];
    } else if (m_indexBufferComponentType == QSSGRenderComponentType::UnsignedInt32) {
        QSSGDataView<quint32> longIndex(reinterpret_cast<const quint32 *>(m_indexBufferData.begin()), indexCount);
        result = longIndex[index];
    } else {
        // If you get here something terrible happend
        Q_ASSERT(false);
    }
    return result;
}

QVector3D QSSGMeshBVHBuilder::getVertexBufferValuePosition(quint32 index) const
{
    if (!m_hasPositionData)
        return QVector3D();

    const quint32 offset = index * m_vertexStride + m_vertexPosOffset;
    const QVector3D *position = reinterpret_cast<const QVector3D *>(m_vertexBufferData.begin() + offset);

    return *position;
}

QVector2D QSSGMeshBVHBuilder::getVertexBufferValueUV(quint32 index) const
{
    if (!m_hasUVData)
        return QVector2D();

    const quint32 offset = index * m_vertexStride + m_vertexUVOffset;
    const QVector2D *uv = reinterpret_cast<const QVector2D *>(m_vertexBufferData.begin() + offset);

    return *uv;
}

QSSGMeshBVHNode *QSSGMeshBVHBuilder::splitNode(QSSGMeshBVHNode *node, quint32 offset, quint32 count, quint32 depth)
{
    // Force a leaf node if the there are too few triangles or the tree depth
    // has exceeded the maximum depth
    if (count < m_maxLeafTriangles || depth >= m_maxTreeDepth) {
        node->offset = offset;
        node->count = count;
        return node;
    }

    // Determine where to split the current bounds
    const QSSGMeshBVHBuilder::Split split = getOptimalSplit(node->boundingData, offset, count);
    // Really this shouldn't happen unless there is invalid bounding data, but if that
    // that does happen make this a leaf node.
    if (split.axis == QSSGMeshBVHBuilder::Axis::None) {
        node->offset = offset;
        node->count = count;
        return node;
    }

    // Create the split by sorting the values in m_triangleBounds between
    // offset - count based on the split axis and position. The returned offset
    // will determine which values go into the left and right nodes.
    const quint32 splitOffset = partition(offset, count, split);

    // Create the leaf nodes
    if (splitOffset == offset || splitOffset == (offset + count)) {
        // If the split is at the start or end, this is a leaf node now
        // because there is no further branches necessary.
        node->offset = offset;
        node->count = count;
    } else {
        // Create the Left Node
        node->left = new QSSGMeshBVHNode();
        const quint32 leftOffset = offset;
        const quint32 leftCount = splitOffset - offset;
        node->left->boundingData = getBounds(leftOffset, leftCount);
        node->left = splitNode(node->left, leftOffset, leftCount, depth + 1);

        // Create the Right Node
        node->right = new QSSGMeshBVHNode();
        const quint32 rightOffset = splitOffset;
        const quint32 rightCount = count - leftCount;
        node->right->boundingData = getBounds(rightOffset, rightCount);
        node->right = splitNode(node->right, rightOffset, rightCount, depth + 1);
    }

    return node;
}

QSSGBounds3 QSSGMeshBVHBuilder::getBounds(quint32 offset, quint32 count) const
{
    QSSGBounds3 totalBounds;

    for (quint32 i = 0; i < count; ++i) {
        QSSGBounds3 bounds = m_triangleBounds[i + offset]->bounds;
        totalBounds.include(bounds);
    }
    return totalBounds;
}

QSSGMeshBVHBuilder::Split QSSGMeshBVHBuilder::getOptimalSplit(const QSSGBounds3 &nodeBounds, quint32 offset, quint32 count) const
{
    QSSGMeshBVHBuilder::Split split;
    split.axis = getLongestDimension(nodeBounds);
    split.pos = 0.f;

    if (split.axis != Axis::None)
        split.pos = getAverageValue(offset, count, split.axis);

    return split;
}

QSSGMeshBVHBuilder::Axis QSSGMeshBVHBuilder::getLongestDimension(const QSSGBounds3 &nodeBounds)
{
    QSSGMeshBVHBuilder::Axis axis = Axis::None;
    float largestDistance = std::numeric_limits<float>::min();

    if (!nodeBounds.isFinite() || nodeBounds.isEmpty())
        return axis;

    const QVector3D delta = nodeBounds.maximum - nodeBounds.minimum;

    if (delta.x() > largestDistance) {
        axis = Axis::X;
        largestDistance = delta.x();
    }
    if (delta.y() > largestDistance) {
        axis = Axis::Y;
        largestDistance = delta.y();
    }
    if (delta.z() > largestDistance)
        axis = Axis::Z;

    return axis;
}

// Get the average values of triangles for a given axis
float QSSGMeshBVHBuilder::getAverageValue(quint32 offset, quint32 count, QSSGMeshBVHBuilder::Axis axis) const
{
    float average = 0;

    Q_ASSERT(axis != Axis::None);
    Q_ASSERT(count != 0);

    for (quint32 i = 0; i < count; ++i)
        average += m_triangleBounds[i + offset]->bounds.center(int(axis));

    return average / count;
}

quint32 QSSGMeshBVHBuilder::partition(quint32 offset, quint32 count, const QSSGMeshBVHBuilder::Split &split)
{
    int left = offset;
    int right = offset + count - 1;
    const float pos = split.pos;
    const int axis = int(split.axis);

    while (true) {
        while (left <= right && m_triangleBounds[left]->bounds.center()[axis] < pos)
            left++;

        while (left <= right && m_triangleBounds[right]->bounds.center()[axis] >= pos)
            right--;

        if (left < right) {
            // Swap triangleBounds at left and right
            auto temp = m_triangleBounds[left];
            m_triangleBounds[left] = m_triangleBounds[right];
            m_triangleBounds[right] = temp;

            left++;
            right--;
        } else {
            return left;
        }
    }
    Q_UNREACHABLE();
}

QT_END_NAMESPACE
