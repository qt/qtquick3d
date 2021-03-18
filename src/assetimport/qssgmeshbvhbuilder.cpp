/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "qssgmeshbvhbuilder_p.h"

QT_BEGIN_NAMESPACE

QSSGMeshBVHBuilder::QSSGMeshBVHBuilder(QSSGMeshUtilities::Mesh *mesh)
    : m_mesh(mesh)
{
    m_baseAddress = reinterpret_cast<quint8 *>(m_mesh);
    m_vertexBufferData = QSSGByteView(m_mesh->m_vertexBuffer.m_data.begin(m_baseAddress),
                                      m_mesh->m_vertexBuffer.m_data.size());
    m_indexBufferData = QSSGByteView(m_mesh->m_indexBuffer.m_data.begin(m_baseAddress),
                                     m_mesh->m_indexBuffer.m_data.size());
    m_indexBufferComponentType = m_mesh->m_indexBuffer.m_componentType;
    if (m_indexBufferComponentType == QSSGRenderComponentType::Integer16)
        m_indexBufferComponentType = QSSGRenderComponentType::UnsignedInteger16;
    else if (m_indexBufferComponentType == QSSGRenderComponentType::Integer32)
        m_indexBufferComponentType = QSSGRenderComponentType::UnsignedInteger32;

    // Get VertexBuffer Information
    const auto &entries = m_mesh->m_vertexBuffer.m_entries;
    for (quint32 entryIdx = 0, entryEnd = entries.size(); entryIdx < entryEnd; ++entryIdx) {
        QSSGRenderVertexBufferEntry entry = entries.index(m_baseAddress, entryIdx).toVertexBufferEntry(m_baseAddress);
        if (!strcmp(entry.m_name, QSSGMeshUtilities::Mesh::getPositionAttrName())) {
            m_hasPositionData = true;
            m_vertexPosOffset = entry.m_firstItemOffset;
        } else if (!strcmp(entry.m_name, QSSGMeshUtilities::Mesh::getUVAttrName())) {
            m_hasUVData = true;
            m_vertexUV0Offset = entry.m_firstItemOffset;
        }
    }
    m_vertexStride = m_mesh->m_vertexBuffer.m_stride;

}

QSSGMeshBVH* QSSGMeshBVHBuilder::buildTree()
{
    m_roots.clear();

    // This only works with triangles
    if (m_mesh->m_drawMode != QSSGRenderDrawMode::Triangles)
        return nullptr;

    // Calculate the bounds for each triangle in whole mesh once
    const quint32 indexCount = m_indexBufferData.size() / getSizeOfType(m_indexBufferComponentType);
    m_triangleBounds = calculateTriangleBounds(0, indexCount);

    // For each submesh, generate a root bvh node
    for (quint32 subsetIdx = 0, subsetEnd = m_mesh->m_subsets.size(); subsetIdx < subsetEnd; ++subsetIdx) {
        const QSSGMeshUtilities::MeshSubset &source(m_mesh->m_subsets.index(m_baseAddress, subsetIdx));
        QSSGMeshBVHNode *root = new QSSGMeshBVHNode();
        // Offsets provided by subset are for the index buffer
        // Convert them to work with the triangle bounds list
        const quint32 triangleOffset = source.m_offset / 3;
        const quint32 triangleCount = source.m_count / 3;
        root->boundingData = getBounds(triangleOffset, triangleCount);
        // Recursively split the mesh into a tree of smaller bounding volumns
        root = splitNode(root, triangleOffset, triangleCount);
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

        const quint32 index1 = getIndexBufferValue(triangleIndex + 0);
        const quint32 index2 = getIndexBufferValue(triangleIndex + 1);
        const quint32 index3 = getIndexBufferValue(triangleIndex + 2);

        QSSGMeshBVHTriangle *triangle = new QSSGMeshBVHTriangle();

        triangle->vertex1 = getVertexBufferValuePosition(index1);
        triangle->vertex2 = getVertexBufferValuePosition(index2);
        triangle->vertex3 = getVertexBufferValuePosition(index3);
        triangle->uvCoord1 = getVertexBufferValueUV0(index1);
        triangle->uvCoord2 = getVertexBufferValueUV0(index2);
        triangle->uvCoord3 = getVertexBufferValueUV0(index3);

        triangle->bounds = QSSGBounds3::empty();
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
    const quint32 indexCount = m_indexBufferData.size() / getSizeOfType(m_indexBufferComponentType);
    Q_ASSERT(index < indexCount);

    if (m_indexBufferComponentType == QSSGRenderComponentType::UnsignedInteger16) {
        QSSGDataView<quint16> shortIndex(reinterpret_cast<const quint16 *>(m_indexBufferData.begin()), indexCount);
        result = shortIndex[index];
    } else if (m_indexBufferComponentType == QSSGRenderComponentType::UnsignedInteger32) {
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

QVector2D QSSGMeshBVHBuilder::getVertexBufferValueUV0(quint32 index) const
{
    if (!m_hasUVData)
        return QVector2D();

    const quint32 offset = index * m_vertexStride + m_vertexUV0Offset;
    const QVector2D *uv0 = reinterpret_cast<const QVector2D *>(m_vertexBufferData.begin() + offset);

    return *uv0;
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
    QSSGBounds3 totalBounds = QSSGBounds3::empty();

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
    if (delta.z() > largestDistance) {
        axis = Axis::Z;
        largestDistance = delta.z();
    }
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
