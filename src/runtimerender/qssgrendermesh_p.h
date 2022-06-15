// Copyright (C) 2008-2012 NVIDIA Corporation.
// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QSSG_RENDER_MESH_H
#define QSSG_RENDER_MESH_H

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

#include <QtQuick3DRuntimeRender/private/qssgrhicontext_p.h>

#include <QtQuick3DUtils/private/qssgbounds3_p.h>
#include <QtQuick3DUtils/private/qssgmeshbvh_p.h>

QT_BEGIN_NAMESPACE

struct QSSGRenderSubset
{
    quint32 count;
    quint32 offset;
    QSSGBounds3 bounds; // Vertex buffer bounds
    QSSGMeshBVHNode *bvhRoot = nullptr;
    struct {
        QSSGRef<QSSGRhiBuffer> vertexBuffer;
        QSSGRef<QSSGRhiBuffer> indexBuffer;
        QSSGRhiInputAssemblerState ia;
    } rhi;

    QSSGRenderSubset() = default;
    QSSGRenderSubset(const QSSGRenderSubset &inOther)
        : count(inOther.count)
        , offset(inOther.offset)
        , bounds(inOther.bounds)
        , bvhRoot(inOther.bvhRoot)
        , rhi(inOther.rhi)
    {
    }
    QSSGRenderSubset &operator=(const QSSGRenderSubset &inOther)
    {
        if (this != &inOther) {
            count = inOther.count;
            offset = inOther.offset;
            bounds = inOther.bounds;
            bvhRoot = inOther.bvhRoot;
            rhi = inOther.rhi;
        }
        return *this;
    }
};

struct QSSGRenderMesh
{
    Q_DISABLE_COPY(QSSGRenderMesh)

    QVector<QSSGRenderSubset> subsets;
    QSSGRenderDrawMode drawMode;
    QSSGRenderWinding winding;
    QSSGMeshBVH *bvh = nullptr;
    QSize lightmapSizeHint;

    QSSGRenderMesh(QSSGRenderDrawMode inDrawMode, QSSGRenderWinding inWinding)
        : drawMode(inDrawMode), winding(inWinding)
    {
    }

    ~QSSGRenderMesh()
    {
        delete bvh;
    }
};
QT_END_NAMESPACE

#endif
