// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "assimputils.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/Logger.hpp>
#include <assimp/DefaultLogger.hpp>
#include <assimp/postprocess.h>
#include <assimp/importerdesc.h>

#include <QtCore/qstring.h>

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

QT_BEGIN_NAMESPACE

namespace
{

struct SubsetEntryData {
    QString name;
    int indexLength;
    int indexOffset;
    quint32 lightmapWidth;
    quint32 lightmapHeight;
};

}

QSSGMesh::Mesh AssimpUtils::generateMeshData(const aiScene &scene,
                                             const MeshList &meshes,
                                             bool useFloatJointIndices,
                                             QString &errorString)
{
    // Check if we need placeholders in certain channels
    bool needsPositionData = false;
    bool needsNormalData = false;
    bool needsTangentData = false;
    bool needsVertexColorData = false;
    unsigned uv0Components = 0;
    unsigned uv1Components = 0;
    bool needsBones = false;

    // GLTF should support at least 8 attributes for morphing.
    // The supported combinations are the followings.
    // 1. 8 targets having only positions.
    // 2. 4 targets having both positions and normals.
    // 3. 2 targets having positions, normals, and tangents(with binormals)
    //
    // 4. 2 targets having only positions and 3 targets having both positions
    //   and normals,
    // 5. ....
    //
    // Handling the same types is simple but let's think about 4.
    // In this case, animMeshes should be sorted by descending order of the
    // number of input attributes. It means that we need to process 3 targets
    // having more attributes first and then 2 remaining targets.
    // However, we will assume the asset is made by this correct order.

    quint32 numMorphTargets = 0;
    QVector<bool> needsTargetPosition;
    QVector<bool> needsTargetNormal;
    QVector<bool> needsTargetTangent;
    QVector<float> targetWeight;

    for (const auto *mesh : meshes) {
        uv0Components = qMax(mesh->mNumUVComponents[0], uv0Components);
        uv1Components = qMax(mesh->mNumUVComponents[1], uv1Components);
        needsPositionData |= mesh->HasPositions();
        needsNormalData |= mesh->HasNormals();
        needsTangentData |= mesh->HasTangentsAndBitangents();
        needsVertexColorData |=mesh->HasVertexColors(0);
        needsBones |= mesh->HasBones();
        if (mesh->mNumAnimMeshes && mesh->mAnimMeshes) {
            if (mesh->mNumAnimMeshes > 8)
                qWarning() << "QtQuick3D supports maximum 8 morph targets, remains will be ignored\n";
            const quint32 numAnimMeshes = qMin(8U, mesh->mNumAnimMeshes);
            if (numMorphTargets < numAnimMeshes) {
                numMorphTargets = numAnimMeshes;
                needsTargetPosition.resize(numMorphTargets);
                needsTargetNormal.resize(numMorphTargets);
                needsTargetTangent.resize(numMorphTargets);
                targetWeight.resize(numMorphTargets);
            }
            for (uint i = 0; i < numAnimMeshes; ++i) {
                auto animMesh = mesh->mAnimMeshes[i];
                needsTargetPosition[i] |= animMesh->HasPositions();
                needsTargetNormal[i] |= animMesh->HasNormals();
                needsTargetTangent[i] |= animMesh->HasTangentsAndBitangents();
                targetWeight[i] = animMesh->mWeight;
            }
        }
    }

    QByteArray positionData;
    QByteArray normalData;
    QByteArray uv0Data;
    QByteArray uv1Data;
    QByteArray tangentData;
    QByteArray binormalData;
    QByteArray vertexColorData;
    QByteArray indexBufferData;
    QByteArray boneIndexData;
    QByteArray boneWeightData;
    QByteArray targetPositionData[8];
    QByteArray targetNormalData[8];
    QByteArray targetTangentData[8];
    QByteArray targetBinormalData[8];
    QVector<SubsetEntryData> subsetData;
    quint32 baseIndex = 0;

    // Always use 32-bit indices. Metal has a requirement of 4 byte alignment
    // for index buffer offsets, and we cannot risk hitting that.
    QSSGMesh::Mesh::ComponentType indexType = QSSGMesh::Mesh::ComponentType::UnsignedInt32;

    const quint32 float32ByteSize = QSSGMesh::MeshInternal::byteSizeForComponentType(QSSGMesh::Mesh::ComponentType::Float32);
    for (const auto *mesh : meshes) {
        // Index Buffer
        QVector<quint32> indexes;
        indexes.reserve(mesh->mNumFaces * 3);
        for (unsigned int faceIndex = 0;faceIndex < mesh->mNumFaces; ++faceIndex) {
            const auto face = mesh->mFaces[faceIndex];
            // Faces should always have 3 indicides
            Q_ASSERT(face.mNumIndices == 3);
            // 'indexes' is global, with entries referring to the merged
            // (per-model, not per-submesh) data.
            indexes.append(quint32(face.mIndices[0]) + baseIndex);
            indexes.append(quint32(face.mIndices[1]) + baseIndex);
            indexes.append(quint32(face.mIndices[2]) + baseIndex);
        }
        baseIndex += mesh->mNumVertices;

        QByteArray positions;
        if (mesh->HasPositions()) {
            positions = QByteArray::fromRawData(reinterpret_cast<const char *>(mesh->mVertices),
                                                mesh->mNumVertices * 3 * float32ByteSize);
        }

        QByteArray normals;
        if (mesh->HasNormals()) {
            normals = QByteArray::fromRawData(reinterpret_cast<const char *>(mesh->mNormals),
                                              mesh->mNumVertices * 3 * float32ByteSize);
        }

        QByteArray uv0;
        if (mesh->HasTextureCoords(0)) {
            QVector<float> uvCoords;
            uvCoords.resize(uv0Components * mesh->mNumVertices);
            for (uint i = 0; i < mesh->mNumVertices; ++i) {
                int offset = i * uv0Components;
                aiVector3D *textureCoords = mesh->mTextureCoords[0];
                uvCoords[offset] = textureCoords[i].x;
                uvCoords[offset + 1] = textureCoords[i].y;
                if (uv0Components == 3)
                    uvCoords[offset + 2] = textureCoords[i].z;
            }
            uv0 = QByteArray(reinterpret_cast<const char*>(uvCoords.constData()), uvCoords.size() * sizeof(float));
        }

        if (mesh->HasPositions())
            positionData += positions;
        else if (needsPositionData)
            positionData += QByteArray(mesh->mNumVertices * 3 * float32ByteSize, '\0');

        if (mesh->HasNormals())
            normalData += normals;
        else if (needsNormalData)
            normalData += QByteArray(mesh->mNumVertices * 3 * float32ByteSize, '\0');

        if (mesh->HasTextureCoords(0))
            uv0Data += uv0;
        else
            uv0Data += QByteArray(mesh->mNumVertices * uv0Components * float32ByteSize, '\0');

        if (mesh->HasTextureCoords(1)) {
            QVector<float> uvCoords;
            uvCoords.resize(uv1Components * mesh->mNumVertices);
            for (uint i = 0; i < mesh->mNumVertices; ++i) {
                int offset = i * uv1Components;
                aiVector3D *textureCoords = mesh->mTextureCoords[1];
                uvCoords[offset] = textureCoords[i].x;
                uvCoords[offset + 1] = textureCoords[i].y;
                if (uv1Components == 3)
                    uvCoords[offset + 2] = textureCoords[i].z;
            }
            uv1Data += QByteArray(reinterpret_cast<const char*>(uvCoords.constData()), uvCoords.size() * sizeof(float));
        } else {
            uv1Data += QByteArray(mesh->mNumVertices * uv1Components * float32ByteSize, '\0');
        }

        if (mesh->HasTangentsAndBitangents()) {
            const QByteArray tangents = QByteArray::fromRawData(reinterpret_cast<const char *>(mesh->mTangents),
                                                                mesh->mNumVertices * 3 * float32ByteSize);
            tangentData += tangents;

            // Binormals (They are actually supposed to be Bitangents despite what they are called)
            const QByteArray binormals = QByteArray::fromRawData(reinterpret_cast<const char*>(mesh->mBitangents),
                                                                 mesh->mNumVertices * 3 * float32ByteSize);
            binormalData += binormals;
        } else if (needsTangentData) {
            tangentData += QByteArray(mesh->mNumVertices * 3 * float32ByteSize, '\0');
            binormalData += QByteArray(mesh->mNumVertices * 3 * float32ByteSize, '\0');
        }

        if (mesh->HasVertexColors(0)) {
            const QByteArray colors = QByteArray::fromRawData(reinterpret_cast<const char *>(mesh->mColors[0]),
                                                              mesh->mNumVertices * 4 * float32ByteSize);
            vertexColorData += colors;
        } else if (needsVertexColorData) {
            vertexColorData += QByteArray(mesh->mNumVertices * 4 * float32ByteSize, '\0');
        }

        // Bones + Weights
        QVector<qint32> boneIndexes;
        QVector<float> fBoneIndexes;
        QVector<float> weights;
        if (mesh->HasBones()) {
            weights.resize(mesh->mNumVertices * 4, 0.0f);
            if (useFloatJointIndices)
                fBoneIndexes.resize(mesh->mNumVertices * 4, 0);
            else
                boneIndexes.resize(mesh->mNumVertices * 4, 0);

            for (uint i = 0; i < mesh->mNumBones; ++i) {
                QString boneName = QString::fromUtf8(mesh->mBones[i]->mName.C_Str());

                const uint vId = i;
                for (uint j = 0; j < mesh->mBones[i]->mNumWeights; ++j) {
                    quint32 vertexId = mesh->mBones[i]->mWeights[j].mVertexId;
                    float weight = mesh->mBones[i]->mWeights[j].mWeight;

                    // skip a bone transform having small weight
                    if (weight <= 0.01f)
                        continue;

                    //  if any vertex has more weights than 4, it will be ignored
                    for (uint ii = 0; ii < 4; ++ii) {
                        if (weights[vertexId * 4 + ii] == 0.0f) {
                            if (useFloatJointIndices)
                                fBoneIndexes[vertexId * 4 + ii] = float(vId);
                            else
                                boneIndexes[vertexId * 4 + ii] = vId;
                            weights[vertexId * 4 + ii] = weight;
                            break;
                        } else if (ii == 3) {
                            qWarning("vertexId %d has already 4 weights and index %d's weight %f will be ignored.", vertexId, vId, weight);
                        }
                    }
                }
            }

            if (useFloatJointIndices) {
                const QByteArray boneIndices = QByteArray::fromRawData(reinterpret_cast<const char *>(fBoneIndexes.constData()),
                                                                       fBoneIndexes.size() * sizeof(float));
                boneIndexData += boneIndices;
            } else {
                const QByteArray boneIndices = QByteArray::fromRawData(reinterpret_cast<const char *>(boneIndexes.constData()),
                                                                       boneIndexes.size() * sizeof(qint32));
                boneIndexData += boneIndices;
            }

            const QByteArray boneWeights = QByteArray::fromRawData(reinterpret_cast<const char *>(weights.constData()),
                                                                   weights.size() * sizeof(float));
            boneWeightData += boneWeights;
        } else if (needsBones) {
            boneIndexData += QByteArray(mesh->mNumVertices * 4 * QSSGMesh::MeshInternal::byteSizeForComponentType(QSSGMesh::Mesh::ComponentType::Int32), '\0');
            boneWeightData += QByteArray(mesh->mNumVertices * 4 * float32ByteSize, '\0');
        }

        for (uint i = 0; i < numMorphTargets; ++i) {
            aiAnimMesh *animMesh = nullptr;
            if (i < mesh->mNumAnimMeshes) {
                animMesh = mesh->mAnimMeshes[i];
                Q_ASSERT(animMesh->mNumVertices == mesh->mNumVertices);
            }
            if (needsTargetPosition[i]) {
                if (animMesh && animMesh->HasPositions()) {
                    const QByteArray targetPositions = QByteArray::fromRawData(reinterpret_cast<const char *>(animMesh->mVertices),
                                                                              mesh->mNumVertices * 3 * float32ByteSize);
                    targetPositionData[i] += targetPositions;
                 } else {
                    targetPositionData[i] += QByteArray(mesh->mNumVertices * 3 * float32ByteSize, '\0');
                }
            }
            if (needsTargetNormal[i]) {
                if (animMesh && animMesh->HasNormals()) {
                    const QByteArray targetNormals = QByteArray::fromRawData(reinterpret_cast<const char *>(animMesh->mNormals),
                                                                             mesh->mNumVertices * 3 * float32ByteSize);
                    targetNormalData[i] += targetNormals;
                } else {
                    targetNormalData[i] += QByteArray(mesh->mNumVertices * 3 * float32ByteSize, '\0');
                }
            }
            if (needsTargetTangent[i]) {
                if (animMesh && animMesh->HasTangentsAndBitangents()) {
                    const QByteArray targetTangents = QByteArray::fromRawData(reinterpret_cast<const char *>(animMesh->mTangents),
                                                                              mesh->mNumVertices * 3 * float32ByteSize);
                    const QByteArray targetBinormals = QByteArray::fromRawData(reinterpret_cast<const char *>(animMesh->mBitangents),
                                                                               mesh->mNumVertices * 3 * float32ByteSize);
                    targetTangentData[i] += targetTangents;
                    targetBinormalData[i] += targetBinormals;
                } else {
                    targetTangentData[i] += QByteArray(mesh->mNumVertices * 3 * float32ByteSize, '\0');
                    targetBinormalData[i] += QByteArray(mesh->mNumVertices * 3 * float32ByteSize, '\0');
                }
            }
        }

        SubsetEntryData subsetEntry;
        subsetEntry.indexOffset = indexBufferData.size() / QSSGMesh::MeshInternal::byteSizeForComponentType(indexType);
        subsetEntry.indexLength = indexes.size();
        if (indexType == QSSGMesh::Mesh::ComponentType::UnsignedInt32) {
            indexBufferData += QByteArray(reinterpret_cast<const char *>(indexes.constData()),
                                          indexes.size() * QSSGMesh::MeshInternal::byteSizeForComponentType(indexType));
        } else {
            // convert data to quint16
            QVector<quint16> shortIndexes;
            shortIndexes.resize(indexes.size());
            for (int i = 0; i < shortIndexes.size(); ++i)
                shortIndexes[i] = quint16(indexes[i]);
            indexBufferData += QByteArray(reinterpret_cast<const char *>(shortIndexes.constData()),
                                          shortIndexes.size() * QSSGMesh::MeshInternal::byteSizeForComponentType(indexType));
        }

        subsetEntry.name = QString::fromUtf8(scene.mMaterials[mesh->mMaterialIndex]->GetName().C_Str());
        subsetEntry.lightmapWidth = 0;
        subsetEntry.lightmapHeight = 0;
        subsetData.append(subsetEntry);
    }

    QVector<QSSGMesh::AssetVertexEntry> entries;
    if (positionData.size() > 0) {
        entries.append({
                           QSSGMesh::MeshInternal::getPositionAttrName(),
                           positionData,
                           QSSGMesh::Mesh::ComponentType::Float32,
                           3
                       });
    }
    if (normalData.size() > 0) {
        entries.append({
                           QSSGMesh::MeshInternal::getNormalAttrName(),
                           normalData,
                           QSSGMesh::Mesh::ComponentType::Float32,
                           3
                       });
    }
    if (uv0Data.size() > 0) {
        entries.append({
                           QSSGMesh::MeshInternal::getUV0AttrName(),
                           uv0Data,
                           QSSGMesh::Mesh::ComponentType::Float32,
                           uv0Components
                       });
    }
    if (uv1Data.size() > 0) {
        entries.append({
                           QSSGMesh::MeshInternal::getUV1AttrName(),
                           uv1Data,
                           QSSGMesh::Mesh::ComponentType::Float32,
                           uv1Components
                       });
    }

    if (tangentData.size() > 0) {
        entries.append({
                           QSSGMesh::MeshInternal::getTexTanAttrName(),
                           tangentData,
                           QSSGMesh::Mesh::ComponentType::Float32,
                           3
                       });
    }

    if (binormalData.size() > 0) {
        entries.append({
                           QSSGMesh::MeshInternal::getTexBinormalAttrName(),
                           binormalData,
                           QSSGMesh::Mesh::ComponentType::Float32,
                           3
                       });
    }

    if (vertexColorData.size() > 0) {
        entries.append({
                           QSSGMesh::MeshInternal::getColorAttrName(),
                           vertexColorData,
                           QSSGMesh::Mesh::ComponentType::Float32,
                           4
                       });
    }

    if (boneIndexData.size() > 0) {
        entries.append({
                           QSSGMesh::MeshInternal::getJointAttrName(),
                           boneIndexData,
                           QSSGMesh::Mesh::ComponentType::Int32,
                           4
                       });
        entries.append({
                           QSSGMesh::MeshInternal::getWeightAttrName(),
                           boneWeightData,
                           QSSGMesh::Mesh::ComponentType::Float32,
                           4
                       });
    }
    for (uint i = 0; i < numMorphTargets; ++i) {
        if (targetPositionData[i].size() > 0) {
            entries.append({
                               QSSGMesh::MeshInternal::getTargetPositionAttrName(i),
                               targetPositionData[i],
                               QSSGMesh::Mesh::ComponentType::Float32,
                               3
                           });
        }
        if (targetNormalData[i].size() > 0) {
            entries.append({
                               QSSGMesh::MeshInternal::getTargetNormalAttrName(i),
                               targetNormalData[i],
                               QSSGMesh::Mesh::ComponentType::Float32,
                               3
                           });
        }
        if (targetTangentData[i].size() > 0) {
            entries.append({
                               QSSGMesh::MeshInternal::getTargetTangentAttrName(i),
                               targetTangentData[i],
                               QSSGMesh::Mesh::ComponentType::Float32,
                               3
                           });
        }
        if (targetBinormalData[i].size() > 0) {
            entries.append({
                               QSSGMesh::MeshInternal::getTargetBinormalAttrName(i),
                               targetBinormalData[i],
                               QSSGMesh::Mesh::ComponentType::Float32,
                               3
                           });
        }
    }

    QVector<QSSGMesh::AssetMeshSubset> subsets;
    for (const SubsetEntryData &subset : subsetData) {
        subsets.append({
                           subset.name,
                           quint32(subset.indexLength),
                           quint32(subset.indexOffset),
                           0, // the builder will calculate bounds from the position data
                           subset.lightmapWidth,
                           subset.lightmapHeight
                       });
    }

    return QSSGMesh::Mesh::fromAssetData(entries, indexBufferData, indexType, subsets);
}

QT_END_NAMESPACE
