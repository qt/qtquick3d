// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default

#pragma once

#include <QtCore/qglobal.h>
#include <QtQuick3DUtils/private/qssgmesh_p.h>

#include <QtGui/qvectornd.h>

#include <QtCore/qcontainerfwd.h>
#include <QtCore/qhashfunctions.h>

struct aiScene;
struct aiMesh;

QT_BEGIN_NAMESPACE

class QString;

namespace AssimpUtils
{

using BoneIndexMap = QHash<QString, qint32>;
using MeshList = QVector<const aiMesh *>;

QSSGMesh::Mesh generateMeshData(const aiScene &scene,
                                const MeshList &meshes,
                                bool useFloatJointIndices,
                                bool generateLevelsOfDetail,
                                float normalMergeAngle,
                                float normalSplitAngle,
                                QString &errorString);
}

#ifndef QVECTORND_IS_HASHABLE
inline size_t qHash(QVector3D vector, size_t seed = 0) noexcept
{
    return qHashMulti(seed, vector.x(), vector.y(), vector.z());
}
#endif

QT_END_NAMESPACE
