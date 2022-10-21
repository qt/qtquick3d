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

#include "qssglightmapuvgenerator_p.h"
#include "xatlas.h"

QT_BEGIN_NAMESPACE

QSSGLightmapUVGeneratorResult QSSGLightmapUVGenerator::run(const QByteArray &positions,
                                                           const QByteArray &normals,
                                                           const QByteArray &uv0,
                                                           const QByteArray &index,
                                                           QSSGMesh::Mesh::ComponentType indexComponentType)
{
    QSSGLightmapUVGeneratorResult result;

    xatlas::MeshDecl meshInfo;

    if (indexComponentType == QSSGMesh::Mesh::ComponentType::UnsignedInt16) {
        meshInfo.indexFormat = xatlas::IndexFormat::UInt16;
    } else if (indexComponentType == QSSGMesh::Mesh::ComponentType::UnsignedInt32) {
        meshInfo.indexFormat = xatlas::IndexFormat::UInt32;
    } else {
        qWarning("Lightmap UV generator: Unknown index type %d; cannot generate",
                 int(indexComponentType));
        return result;
    }

    const quint32 indexComponentByteSize = QSSGMesh::MeshInternal::byteSizeForComponentType(indexComponentType);
    const quint32 indexCount = index.size() / indexComponentByteSize;

    meshInfo.indexCount = indexCount;
    meshInfo.indexData = index.constData();

    const quint32 positionStride = 3 * sizeof(float);
    const quint32 normalStride = 3 * sizeof(float);
    const quint32 uvStride = 2 * sizeof(float);

    meshInfo.vertexCount = positions.size() / positionStride;
    meshInfo.vertexPositionData = positions.data();
    meshInfo.vertexPositionStride = positionStride;

    if (!normals.isEmpty()) {
        meshInfo.vertexNormalData = normals.constData();
        meshInfo.vertexNormalStride = normalStride;
    } else {
        meshInfo.vertexNormalData = nullptr;
        meshInfo.vertexNormalStride = 0;
    }

    if (!uv0.isEmpty()) {
        meshInfo.vertexUvData = uv0.constData();
        meshInfo.vertexUvStride = uvStride;
    } else {
        meshInfo.vertexUvData = nullptr;
        meshInfo.vertexUvStride = 0;
    }

    xatlas::PackOptions packOptions;
    packOptions.maxChartSize = 4096;
    packOptions.padding = 1;
    packOptions.blockAlign = true;

    xatlas::ChartOptions chartOptions;

    xatlas::Atlas *atlas = xatlas::Create();
    xatlas::AddMeshError err = xatlas::AddMesh(atlas, meshInfo, 1);
    if (err != xatlas::AddMeshError::Success) {
        qWarning("Failed to register mesh for UV unwrapping (error %d)", int(err));
        xatlas::Destroy(atlas);
        return result;
    }
    xatlas::Generate(atlas, chartOptions, packOptions);

    const uint32_t textureWidth = atlas->width;
    const uint32_t textureHeight = atlas->height;
    if (textureWidth == 0 || textureHeight == 0) {
        qWarning("Texture size is empty, UV unwrapping failed");
        xatlas::Destroy(atlas);
        return result;
    }
    result.lightmapWidth = textureWidth;
    result.lightmapHeight = textureHeight;

    const xatlas::Mesh &output = atlas->meshes[0];
    result.lightmapUVChannel.resize(output.vertexCount * uvStride);
    result.vertexMap.resize(output.vertexCount);

    float *uvPtr = reinterpret_cast<float *>(result.lightmapUVChannel.data());
    for (uint32_t i = 0; i < output.vertexCount; ++i) {
        const float u = output.vertexArray[i].uv[0] / float(textureWidth);
        const float v = output.vertexArray[i].uv[1] / float(textureHeight);
        *uvPtr++ = u;
        *uvPtr++ = v;
        result.vertexMap[i] = output.vertexArray[i].xref;
    }

    result.indexData.resize(output.indexCount * sizeof(quint32));
    quint32 *indexPtr = reinterpret_cast<quint32 *>(result.indexData.data());
    for (uint32_t i = 0; i < output.indexCount; ++i)
        *indexPtr++ = output.indexArray[i];

    xatlas::Destroy(atlas);

    return result;
}

QT_END_NAMESPACE
