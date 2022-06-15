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

#include <QtGui/QVector2D>
#include <QtCore/QVector>

QT_BEGIN_NAMESPACE

struct Q_QUICK3DUTILS_EXPORT QSSGMeshBVHNode {
    ~QSSGMeshBVHNode() {
        delete left;
        delete right;
    }

    // Internal
    QSSGMeshBVHNode *left = nullptr;
    QSSGMeshBVHNode *right = nullptr;
    QSSGBounds3 boundingData;
    //splitAxis

    // Leaf
    int offset = 0;
    int count = 0;
};

struct Q_QUICK3DUTILS_EXPORT QSSGMeshBVHTriangle {
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
    QSSGMeshBVH(const QVector<QSSGMeshBVHNode *> &bvhRoots,
                const QVector<QSSGMeshBVHTriangle *> &bvhTriangles)
        : roots(bvhRoots)
        , triangles(bvhTriangles)
    {}
    ~QSSGMeshBVH();

    QVector<QSSGMeshBVHNode *> roots;
    QVector<QSSGMeshBVHTriangle *> triangles;
};

QT_END_NAMESPACE

#endif // QSSGMESHBVH_H
