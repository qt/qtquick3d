/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
******************************************************************************/

#ifndef ASSIMPUTILS_H
#define ASSIMPUTILS_H

#include <QtCore/qglobal.h>
#include <QtQuick3DUtils/private/qssgmesh_p.h>

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
                                const BoneIndexMap &boneIdxMap,
                                bool generateLightmapUV,
                                bool useFloatJointIndices,
                                QString &errorString);

}

QT_END_NAMESPACE

#endif // ASSIMPUTILS_H
