// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QSSGMESHBVH_H
#define QSSGMESHBVH_H

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
#include <QtQuick3DUtils/private/qssgbounds3_p.h>
#include <QtQuick3DUtils/private/qssgassert_p.h>

#include <QtGui/QVector2D>
#include <QtCore/QVector>

QT_BEGIN_NAMESPACE

struct QSSGMeshBVH;

struct Q_QUICK3DUTILS_EXPORT QSSGMeshBVHNode
{
    struct Handle
    {
        qsizetype idx = -1;
        QSSGMeshBVH *owner = nullptr;

        bool isNull() const { return !owner || idx < 0; }

        inline explicit operator const QSSGMeshBVHNode *() const;
        inline QSSGMeshBVHNode *operator->() const;
    };

    // Internal
    Handle left;
    Handle right;
    QSSGBounds3 boundingData;
    //splitAxis

    // Leaf
    int offset = 0;
    int count = 0;
};

struct Q_QUICK3DUTILS_EXPORT QSSGMeshBVHTriangle
{
    QSSGBounds3 bounds;
    QVector3D vertex1;
    QVector3D vertex2;
    QVector3D vertex3;
    QVector2D uvCoord1;
    QVector2D uvCoord2;
    QVector2D uvCoord3;
};

struct Q_QUICK3DUTILS_EXPORT QSSGMeshBVH
{
    QSSGMeshBVH() = default;
    QSSGMeshBVH(const QVector<QSSGMeshBVHNode::Handle> &bvhRoots,
                const QVector<QSSGMeshBVHNode> nodes,
                const QVector<QSSGMeshBVHTriangle> &bvhTriangles)
        : roots(bvhRoots)
        , m_nodes(nodes)
        , triangles(bvhTriangles)
    {}
    ~QSSGMeshBVH();

    QSSGMeshBVHNode *value(qsizetype idx)
    {
        return (idx >= 0 && idx < m_nodes.size()) ? const_cast<QSSGMeshBVHNode *>(&m_nodes.at(idx)) : nullptr;
    }

    const QSSGMeshBVHNode *value(qsizetype idx) const
    {
        return (idx >= 0 && idx < m_nodes.size()) ? &m_nodes.at(idx) : nullptr;
    }

    QVector<QSSGMeshBVHNode::Handle> roots;
    QVector<QSSGMeshBVHNode> m_nodes;
    QVector<QSSGMeshBVHTriangle> triangles;
};

QSSGMeshBVHNode::Handle::operator const QSSGMeshBVHNode *() const
{
    QSSG_ASSERT(owner, return nullptr);
    return owner->value(idx);
}

QSSGMeshBVHNode *QSSGMeshBVHNode::Handle::operator->() const
{
    return owner->value(idx);
}

QT_END_NAMESPACE

#endif // QSSGMESHBVH_H
