// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QSSGMESHBVHBUILDER_H
#define QSSGMESHBVHBUILDER_H

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
#include <QtQuick3DUtils/private/qtquick3dutilsglobal_p.h>
#include <QtQuick3DUtils/private/qssgmeshbvh_p.h>
#include <QtQuick3DUtils/private/qssgmesh_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICK3DUTILS_EXPORT QSSGMeshBVHBuilder
{
public:
    QSSGMeshBVHBuilder(const QSSGMesh::Mesh &mesh);
    QSSGMeshBVHBuilder(const QByteArray &vertexBuffer,
                       int stride,
                       int posOffset,
                       bool hasUV = false,
                       int uvOffset = -1,
                       bool hasIndexBuffer = false,
                       const QByteArray &indexBuffer = QByteArray(),
                       QSSGRenderComponentType indexBufferType = QSSGRenderComponentType::Int32);

    QSSGMeshBVH* buildTree();

private:
    enum class Axis
    {
        None = -1,
        X = 0,
        Y = 1,
        Z = 2
    };
    struct Split {
        Axis axis;
        float pos;
    };

    QVector<QSSGMeshBVHTriangle*> calculateTriangleBounds(quint32 indexOffset, quint32 indexCount) const;
    quint32 getIndexBufferValue(quint32 index) const;
    QVector3D getVertexBufferValuePosition(quint32 index) const;
    QVector2D getVertexBufferValueUV(quint32 index) const;

    QSSGMeshBVHNode *splitNode(QSSGMeshBVHNode *node, quint32 offset, quint32 count, quint32 depth = 0);
    QSSGBounds3 getBounds(quint32 offset, quint32 count) const;
    Split getOptimalSplit(const QSSGBounds3 &nodeBounds, quint32 offset, quint32 count) const;
    static Axis getLongestDimension(const QSSGBounds3 &nodeBounds);
    float getAverageValue(quint32 offset, quint32 count, Axis axis) const;
    quint32 partition(quint32 offset, quint32 count, const Split &split);

    QSSGMesh::Mesh m_mesh;
    QSSGRenderComponentType m_indexBufferComponentType;
    QByteArray m_indexBufferData;
    QByteArray m_vertexBufferData;
    quint32 m_vertexStride;
    bool m_hasPositionData = false;
    quint32 m_vertexPosOffset;
    bool m_hasUVData = false;
    quint32 m_vertexUVOffset;
    bool m_hasIndexBuffer = true;


    QVector<QSSGMeshBVHTriangle *> m_triangleBounds;
    QVector<QSSGMeshBVHNode *> m_roots;
    quint32 m_maxTreeDepth = 40;
    quint32 m_maxLeafTriangles = 10;
};

QT_END_NAMESPACE

#endif // QSSGMESHBVHBUILDER_H
