/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
