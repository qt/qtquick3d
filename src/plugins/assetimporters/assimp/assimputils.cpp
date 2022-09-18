// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "assimputils.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/Logger.hpp>
#include <assimp/DefaultLogger.hpp>
#include <assimp/postprocess.h>
#include <assimp/importerdesc.h>

#include <QtQuick3DUtils/private/qssgutils_p.h>

#include <QtCore/qstring.h>
#include <QtCore/QHash>
#include <QtCore/QSet>

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
    QVector<QSSGMesh::Mesh::Lod> lods;
};

struct IntVector4D {
    qint32 x = 0;
    qint32 y = 0;
    qint32 z = 0;
    qint32 w = 0;
};

struct VertexAttributeData {
    QVector3D position;
    QVector3D normal;
    QVector3D uv0;
    QVector3D uv1;
    QVector3D tangent;
    QVector3D binormal;
    QVector4D color;
    IntVector4D boneIndexes;
    QVector4D boneWeights;
    QVector3D morphTargetPostions[8];
    QVector3D morphTargetNormals[8];
    QVector3D morphTargetTangents[8];
    QVector3D morphTargetBinormals[8];
};

struct VertexDataRequirments {
    bool needsPositionData = false;
    bool needsNormalData = false;
    bool needsTangentData = false;
    bool needsVertexColorData = false;
    unsigned uv0Components = 0;
    unsigned uv1Components = 0;
    bool needsUV0Data = false;
    bool needsUV1Data = false;
    bool needsBones = false;
    bool useFloatJointIndices = false;

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

    void collectRequirmentsForMesh(const aiMesh *mesh) {
        uv0Components = qMax(mesh->mNumUVComponents[0], uv0Components);
        uv1Components = qMax(mesh->mNumUVComponents[1], uv1Components);
        needsUV0Data |= mesh->HasTextureCoords(0);
        needsUV1Data |= mesh->HasTextureCoords(1);
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
};

QVector<VertexAttributeData> getVertexAttributeData(const aiMesh *mesh, const VertexDataRequirments &requirments)
{
    QVector<VertexAttributeData> vertexAttributes;

    vertexAttributes.resize(mesh->mNumVertices);

    // Positions
    if (mesh->HasPositions()) {
        for (unsigned int index = 0; index < mesh->mNumVertices; ++index) {
            const auto vertex = mesh->mVertices[index];
            vertexAttributes[index].position = QVector3D(vertex.x, vertex.y, vertex.z);
        }
    }

    // Normals
    if (mesh->HasNormals()) {
        for (unsigned int index = 0; index < mesh->mNumVertices; ++index) {
            const auto normal = mesh->mNormals[index];
            vertexAttributes[index].normal = QVector3D(normal.x, normal.y, normal.z);
        }
    }

    // UV0
    if (mesh->HasTextureCoords(0)) {
        const auto texCoords = mesh->mTextureCoords[0];
        if (requirments.uv0Components == 2) {
            for (unsigned int index = 0; index < mesh->mNumVertices; ++index) {
                const auto uv = texCoords[index];
                vertexAttributes[index].uv0 = QVector3D(uv.x, uv.y, 0.0f);
            }
        } else if (requirments.uv0Components == 3) {
            for (unsigned int index = 0; index < mesh->mNumVertices; ++index) {
                const auto uv = texCoords[index];
                vertexAttributes[index].uv0 = QVector3D(uv.x, uv.y, uv.z);
            }
        }
    }

    // UV1
    if (mesh->HasTextureCoords(1)) {
        const auto texCoords = mesh->mTextureCoords[1];
        if (requirments.uv1Components == 2) {
            for (unsigned int index = 0; index < mesh->mNumVertices; ++index) {
                const auto uv = texCoords[index];
                vertexAttributes[index].uv1 = QVector3D(uv.x, uv.y, 0.0f);
            }
        } else if (requirments.uv1Components == 3) {
            for (unsigned int index = 0; index < mesh->mNumVertices; ++index) {
                const auto uv = texCoords[index];
                vertexAttributes[index].uv1 = QVector3D(uv.x, uv.y, uv.z);
            }
        }
    }

    // Tangents and Binormals
    if (mesh->HasTangentsAndBitangents()) {
        for (unsigned int index = 0; index < mesh->mNumVertices; ++index) {
            const auto tangent = mesh->mTangents[index];
            const auto binormal = mesh->mBitangents[index];
            vertexAttributes[index].tangent = QVector3D(tangent.x, tangent.y, tangent.z);
            vertexAttributes[index].binormal = QVector3D(binormal.x, binormal.y, binormal.z);
        }
    }

    // Vertex Colors
    if (mesh->HasVertexColors(0)) {
        for (unsigned int index = 0; index < mesh->mNumVertices; ++index) {
            const auto color = mesh->mColors[0][index];
            vertexAttributes[index].color = QVector4D(color.r, color.g, color.b, color.a);
        }
    }

    // Bones + Weights
    if (mesh->HasBones()) {
        for (uint i = 0; i < mesh->mNumBones; ++i) {
            const uint vId = i;
            for (uint j = 0; j < mesh->mBones[i]->mNumWeights; ++j) {
                quint32 vertexId = mesh->mBones[i]->mWeights[j].mVertexId;
                float weight = mesh->mBones[i]->mWeights[j].mWeight;

                // skip a bone transform having small weight
                if (weight <= 0.01f)
                    continue;

                //  if any vertex has more weights than 4, it will be ignored
                if (vertexAttributes[vertexId].boneWeights.x() == 0.0f) {
                    vertexAttributes[vertexId].boneIndexes.x = qint32(vId);
                    vertexAttributes[vertexId].boneWeights.setX(weight);
                } else if (vertexAttributes[vertexId].boneWeights.y() == 0.0f) {
                    vertexAttributes[vertexId].boneIndexes.y = qint32(vId);
                    vertexAttributes[vertexId].boneWeights.setY(weight);
                } else if (vertexAttributes[vertexId].boneWeights.z() == 0.0f) {
                    vertexAttributes[vertexId].boneIndexes.z = qint32(vId);
                    vertexAttributes[vertexId].boneWeights.setZ(weight);
                } else if (vertexAttributes[vertexId].boneWeights.w() == 0.0f) {
                    vertexAttributes[vertexId].boneIndexes.w = qint32(vId);
                    vertexAttributes[vertexId].boneWeights.setW(weight);
                } else {
                    qWarning("vertexId %d has already 4 weights and index %d's weight %f will be ignored.", vertexId, vId, weight);
                }
            }
        }
    }

    // Morph Targets
    for (uint i = 0; i < requirments.numMorphTargets; ++i) {
        aiAnimMesh *animMesh = nullptr;
        if (i < mesh->mNumAnimMeshes) {
            animMesh = mesh->mAnimMeshes[i];
            Q_ASSERT(animMesh->mNumVertices == mesh->mNumVertices);
        }
        if (requirments.needsTargetPosition[i]) {
            if (animMesh && animMesh->HasPositions()) {
                for (unsigned int index = 0; index < mesh->mNumVertices; ++index) {
                    const auto vertex = animMesh->mVertices[index];
                    vertexAttributes[index].morphTargetPostions[i] = QVector3D(vertex.x, vertex.y, vertex.z);
                }
            }
        }
        if (requirments.needsTargetNormal[i]) {
            if (animMesh && animMesh->HasNormals()) {
                for (unsigned int index = 0; index < mesh->mNumVertices; ++index) {
                    const auto normal = animMesh->mNormals[index];
                    vertexAttributes[index].morphTargetNormals[i] = QVector3D(normal.x, normal.y, normal.z);
                }
            }
        }
        if (requirments.needsTargetTangent[i]) {
            if (animMesh && animMesh->HasTangentsAndBitangents()) {
                for (unsigned int index = 0; index < mesh->mNumVertices; ++index) {
                    const auto tangent = animMesh->mTangents[index];
                    const auto binormal = animMesh->mBitangents[index];
                    vertexAttributes[index].morphTargetTangents[i] = QVector3D(tangent.x, tangent.y, tangent.z);
                    vertexAttributes[index].morphTargetBinormals[i] = QVector3D(binormal.x, binormal.y, binormal.z);
                }
            }
        }
    }

    return vertexAttributes;
}

struct VertexBufferData {
    QByteArray positionData;
    QByteArray normalData;
    QByteArray uv0Data;
    QByteArray uv1Data;
    QByteArray tangentData;
    QByteArray binormalData;
    QByteArray vertexColorData;
    QByteArray boneIndexData;
    QByteArray boneWeightData;
    QByteArray targetPositionData[8];
    QByteArray targetNormalData[8];
    QByteArray targetTangentData[8];
    QByteArray targetBinormalData[8];

    void addVertexAttributeData(const VertexAttributeData &vertex, const VertexDataRequirments &requirments)
    {
        // Position
        if (requirments.needsPositionData)
            positionData += QByteArray::fromRawData(reinterpret_cast<const char *>(&vertex.position), sizeof(QVector3D));
        // Normal
        if (requirments.needsNormalData)
            normalData += QByteArray::fromRawData(reinterpret_cast<const char *>(&vertex.normal), sizeof(QVector3D));
        // UV0

        if (requirments.needsUV0Data) {
            if (requirments.uv0Components == 2) {
                const QVector2D uv(vertex.uv0.x(), vertex.uv0.y());
                uv0Data += QByteArray::fromRawData(reinterpret_cast<const char *>(&uv), sizeof(QVector2D));
            } else {
                uv0Data += QByteArray::fromRawData(reinterpret_cast<const char *>(&vertex.uv0), sizeof(QVector3D));
            }
        }

        // UV1
        if (requirments.needsUV1Data) {
            if (requirments.uv1Components == 2) {
                const QVector2D uv(vertex.uv1.x(), vertex.uv1.y());
                uv1Data += QByteArray::fromRawData(reinterpret_cast<const char *>(&uv), sizeof(QVector2D));
            } else {
                uv1Data += QByteArray::fromRawData(reinterpret_cast<const char *>(&vertex.uv1), sizeof(QVector3D));
            }
        }

        // Tangent
        // Binormal
        if (requirments.needsTangentData) {
            tangentData += QByteArray::fromRawData(reinterpret_cast<const char *>(&vertex.tangent), sizeof(QVector3D));
            binormalData += QByteArray::fromRawData(reinterpret_cast<const char *>(&vertex.binormal), sizeof(QVector3D));
        }

        // Color
        if (requirments.needsVertexColorData)
            vertexColorData += QByteArray::fromRawData(reinterpret_cast<const char *>(&vertex.color), sizeof(QVector4D));

        // Bone Indexes
        // Bone Weights
        if (requirments.needsBones) {
            if (requirments.useFloatJointIndices) {
                const QVector4D fBoneIndex(float(vertex.boneIndexes.x), float(vertex.boneIndexes.y), float(vertex.boneIndexes.z), float(vertex.boneIndexes.w));
                boneIndexData += QByteArray::fromRawData(reinterpret_cast<const char *>(&fBoneIndex), sizeof(QVector4D));
            } else {
                boneIndexData += QByteArray::fromRawData(reinterpret_cast<const char *>(&vertex.boneIndexes), sizeof(IntVector4D));
            }
            boneWeightData += QByteArray::fromRawData(reinterpret_cast<const char *>(&vertex.boneWeights), sizeof(QVector4D));
        }

        // Morph Targets
        for (uint i = 0; i < requirments.numMorphTargets; ++i) {
            if (requirments.needsTargetPosition[i]) {
                targetPositionData[i] += QByteArray::fromRawData(reinterpret_cast<const char *>(&vertex.morphTargetPostions[i]), sizeof(QVector3D));
            }
            if (requirments.needsTargetNormal[i]) {
                targetNormalData[i] += QByteArray::fromRawData(reinterpret_cast<const char *>(&vertex.morphTargetNormals[i]), sizeof(QVector3D));
            }
            if (requirments.needsTargetTangent[i]) {
                targetTangentData[i] += QByteArray::fromRawData(reinterpret_cast<const char *>(&vertex.morphTargetTangents[i]), sizeof(QVector3D));
                targetBinormalData[i] += QByteArray::fromRawData(reinterpret_cast<const char *>(&vertex.morphTargetBinormals[i]), sizeof(QVector3D));
            }
        }
    }

    QVector<QSSGMesh::AssetVertexEntry> createEntries(const VertexDataRequirments &requirments) {
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
                               requirments.uv0Components
                           });
        }
        if (uv1Data.size() > 0) {
            entries.append({
                               QSSGMesh::MeshInternal::getUV1AttrName(),
                               uv1Data,
                               QSSGMesh::Mesh::ComponentType::Float32,
                               requirments.uv1Components
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
        for (uint i = 0; i < requirments.numMorphTargets; ++i) {
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
        return entries;
    }
};

QVector<QPair<float, QVector<quint32>>> generateMeshLevelsOfDetail(QVector<VertexAttributeData> &vertexAttributes, QVector<quint32> &indexes, float normalMergeAngle = 60.0f, float normalSplitAngle = 25.0f)
{
    // If both normalMergeAngle and normalSplitAngle are 0.0, then don't recalculate normals
    const bool recalculateNormals = !(qFuzzyIsNull(normalMergeAngle) && qFuzzyIsNull(normalSplitAngle));
    const float normalMergeThreshold = qCos(qDegreesToRadians(normalMergeAngle));
    const float normalSplitThreshold = qCos(qDegreesToRadians(normalSplitAngle));

    QVector<QVector3D> positions;
    positions.reserve(vertexAttributes.size());
    QVector<QVector3D> normals;
    normals.reserve(vertexAttributes.size());
    for (const auto &vertex : vertexAttributes) {
        positions.append(vertex.position);
        normals.append(vertex.normal);
    }

    QVector<QVector3D> splitVertexNormals;
    QVector<quint32> splitVertexIndices;
    quint32 splitVertexCount = vertexAttributes.size();

    const float targetError = std::numeric_limits<float>::max(); // error doesn't matter, index count is more important
    const float *vertexData = reinterpret_cast<const float *>(positions.constData());
    const float scaleFactor = QSSGMesh::simplifyScale(vertexData, positions.size(), sizeof(QVector3D));
    const quint32 indexCount = indexes.size();
    quint32 indexTarget = 12;
    quint32 lastIndexCount = 0;
    QVector<QPair<float, QVector<quint32>>> lods;

    while (indexTarget < indexCount) {
        float error;
        QVector<quint32> newIndexes;
        newIndexes.resize(indexCount); // Must be the same size as the original indexes to pass to simplifyMesh
        size_t newLength = QSSGMesh::simplifyMesh(newIndexes.data(), indexes.constData(), indexes.size(), vertexData, positions.size(), sizeof(QVector3D), indexTarget, targetError, 0, &error);

        // Not good enough, try again
        if (newLength < lastIndexCount * 1.5f) {
            indexTarget = indexTarget * 1.5f;
            continue;
        }

        // We are done
        if (newLength == 0 || (newLength >= (indexCount * 0.75f)))
            break;

        newIndexes.resize(newLength);

        // LOD Normal Correction
        if (recalculateNormals) {
            // Cull any new degenerate triangles and get the new face normals
            QVector<QVector3D> faceNormals;
            {
                QVector<quint32> culledIndexes;
                for (quint32 j = 0; j < newIndexes.size(); j += 3) {
                    const QVector3D &v0 = positions[newIndexes[j]];
                    const QVector3D &v1 = positions[newIndexes[j + 1]];
                    const QVector3D &v2 = positions[newIndexes[j + 2]];

                    QVector3D faceNormal = QVector3D::crossProduct(v1 - v0, v2 - v0);
                    // This normalizes the vector in place and returns the magnitude
                    const float faceArea = vec3::normalize(faceNormal);
                    // It is possible that the simplifyMesh process gave us a degenerate triangle
                    // (all three at the same point, or on the same line) or such a small triangle
                    // that a float value doesn't have enough resolution. In that case cull the
                    // "face" since it would not get rendered in a meaningful way anyway
                    if (faceArea != 0.0f) {
                        faceNormals.append(faceNormal);
                        faceNormals.append(faceNormal);
                        faceNormals.append(faceNormal);
                        culledIndexes.append({newIndexes[j], newIndexes[j + 1], newIndexes[j + 2]});
                    }
                }

                if (newIndexes.size() != culledIndexes.size())
                    newIndexes = culledIndexes;
            }

            // Group all shared vertices together by position. We need to know adjacent faces
            // to do vertex normal remapping in the next step.
            QHash<QVector3D, QVector<quint32>> positionHash;
            for (quint32 i = 0; i < newIndexes.size(); ++i) {
                const quint32 index = newIndexes[i];
                const QVector3D position = vertexAttributes[index].position;
                positionHash[position].append(i);
            }

            // Go through each vertex and calculate the normals by checking each
            // adjacent face that share the same vertex position, and create a smoothed
            // normal if the angle between thew face normals is less than the the
            // normalMergeAngle passed to this function (>= since this is cos(radian(angle)) )
            QVector<QPair<quint32, quint32>> remapIndexes;
            for (quint32 positionIndex = 0; positionIndex < newIndexes.size(); ++positionIndex) {
                const quint32 index = newIndexes[positionIndex];
                const QVector3D &position = vertexAttributes[index].position;
                const QVector3D &faceNormal = faceNormals[positionIndex];
                QVector3D newNormal;
                // Find all vertices that share the same position
                const auto &sharedPositions = positionHash.value(position);
                for (auto positionIndex2 : sharedPositions) {
                    if (positionIndex == positionIndex2) {
                        // Don't test against the current face under test
                        newNormal += faceNormal;
                    } else {
                        const quint32 index2 = newIndexes[positionIndex2];
                        const QVector3D &position2 = vertexAttributes[index2].position;
                        const QVector3D &faceNormal2 = faceNormals[positionIndex2];
                        if (QVector3D::dotProduct(faceNormal2, faceNormal) >= normalMergeThreshold)
                            newNormal += faceNormal2;
                    }
                }

                // By normalizing here we get an averaged value of all smoothed normals
                vec3::normalize(newNormal);

                // Now that we know what the smoothed normal would be, check how differnt
                // that normal is from the normal that is already stored in the current
                // index. If the angle delta is greater than normalSplitAngle then we need
                // to create a new vertex entry (making a copy of the current one) and set
                // the new normal value, and reassign the current index to point to that new
                // vertex. Generally the LOD simplification process is such that the existing
                // normal will already be ideal until we start getting to the very low lod levels
                // which changes the topology in such a way that the original normal doesn't
                // make sense anymore, thus the need to provide a more reasonable value.
                const QVector3D &originalNormal = vertexAttributes[index].normal;
                const float theta = QVector3D::dotProduct(originalNormal, newNormal);
                if (theta < normalSplitThreshold) {
                    splitVertexIndices.append(index);
                    splitVertexNormals.append(newNormal.normalized());
                    remapIndexes.append({positionIndex, splitVertexCount++});
                }
            }

            // Do index remap now that all new normals have been calculated
            for (auto pair : remapIndexes)
                newIndexes[pair.first] = pair.second;
        }

        lods.append({error * scaleFactor, newIndexes});
        indexTarget = qMax(newLength, indexTarget) * 2;
        lastIndexCount = newLength;

        if (error == 0.0f)
            break;
    }
    // Here we need to add the new index and vertex values from
    // splitVertexIndices and splitVertexNormals
    for (quint32 i = 0; i < splitVertexIndices.size(); ++i) {
        quint32 index = splitVertexIndices[i];
        QVector3D newNormal = splitVertexNormals[i];
        auto newVertex = vertexAttributes[index];
        newVertex.normal = newNormal;
        vertexAttributes.append(newVertex);
    }

    return lods;
}

}

QSSGMesh::Mesh AssimpUtils::generateMeshData(const aiScene &scene,
                                             const MeshList &meshes,
                                             bool useFloatJointIndices,
                                             bool generateLevelsOfDetail,
                                             float normalMergeAngle,
                                             float normalSplitAngle,
                                             QString &errorString)
{
    // All Mesh subsets are stored in the same Vertex Buffer so we need to make
    // sure that all attributes from each subset have common data by potentially
    // adding placeholder data or doing conversions as necessary.
    // So we need to walk through each subset first and see what the requirments are
    VertexDataRequirments requirments;
    requirments.useFloatJointIndices = useFloatJointIndices;
    for (const auto *mesh : meshes)
        requirments.collectRequirmentsForMesh(mesh);

    // This is the actual data we will pass to the QSSGMesh that will get filled by
    // each of the subset meshes
    QByteArray indexBufferData;
    VertexBufferData vertexBufferData;
    QVector<SubsetEntryData> subsetData;

    // Since the vertex data of subsets are stored one after the other, the values in
    // the index buffer need to be augmented to reflect this offset. baseIndex is used
    // to track the new 0 value of a subset by keeping track of the current vertex
    // count as each new subset is added
    quint32 baseIndex = 0;

    // Always use 32-bit indices. Metal has a requirement of 4 byte alignment
    // for index buffer offsets, and we cannot risk hitting that.
    const QSSGMesh::Mesh::ComponentType indexType = QSSGMesh::Mesh::ComponentType::UnsignedInt32;

    for (const auto *mesh : meshes) {
        // Get the index values for just this mesh
        // The index values should be relative to this meshes
        // vertices and will later need to be corrected using
        // baseIndex to be relative to our combined vertex data
        QVector<quint32> indexes;
        indexes.reserve(mesh->mNumFaces * 3);
        for (unsigned int faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex) {
            const auto face = mesh->mFaces[faceIndex];
            // Faces should always have 3 indices
            Q_ASSERT(face.mNumIndices == 3);
            // Index data for now is relative to the local vertex locations
            // This must be corrected for later to be global
            indexes.append(quint32(face.mIndices[0]));
            indexes.append(quint32(face.mIndices[1]));
            indexes.append(quint32(face.mIndices[2]));
        }

        // Get the Vertex Attribute Data for this mesh
        auto vertexAttributes = getVertexAttributeData(mesh, requirments);

        // Starting point for index buffer offsets
        quint32 baseIndexOffset = indexBufferData.size() / QSSGMesh::MeshInternal::byteSizeForComponentType(indexType);
        QVector<quint32> lodIndexes;
        QVector<QSSGMesh::Mesh::Lod> meshLods;

        // Generate Automatic Mesh Levels of Detail
        if (generateLevelsOfDetail) {
            // Returns a list of lod pairs <distance, lodIndexList> sorted from smallest
            // to largest as this is how they are stored in the index buffer. We still need to
            // populate meshLods with push_front though because subset lod data is sorted from
            // highest detail to lowest
            auto lods = generateMeshLevelsOfDetail(vertexAttributes, indexes, normalMergeAngle, normalSplitAngle);
            for (const auto &lodPair : lods) {
                QSSGMesh::Mesh::Lod lod;
                lod.offset = baseIndexOffset;
                lod.count = lodPair.second.size();
                lod.distance = lodPair.first;
                meshLods.push_front(lod);
                baseIndexOffset += lod.count;
                // Optimize the vertex cache for this lod level
                auto currentLodIndexes = lodPair.second;
                QSSGMesh::optimizeVertexCache(currentLodIndexes.data(), currentLodIndexes.data(), currentLodIndexes.size(), vertexAttributes.size());
                lodIndexes += currentLodIndexes;
            }
        }

        // Write the results to the Global Index/Vertex/SubsetData buffers
        // Optimize the vertex chache for the original index values
        QSSGMesh::optimizeVertexCache(indexes.data(), indexes.data(), indexes.size(), vertexAttributes.size());

        // Write Index Buffer Data
        QVector<quint32> combinedIndexValues = lodIndexes + indexes;
        // Set the absolute index relative to the larger vertex buffer
        for (auto &index : combinedIndexValues)
            index += baseIndex;
        indexBufferData += QByteArray(reinterpret_cast<const char *>(combinedIndexValues.constData()),
                                      combinedIndexValues.size() * QSSGMesh::MeshInternal::byteSizeForComponentType(indexType));

        // Index Data is setup such that LOD indexes will come first
        // from lowest quality to original
        // | LOD3 | LOD2 | LOD1 | Original |
        // If there were no LOD levels then indexOffset just points to that here
        // baseIndexOffset has already been calculated to be correct at this point
        SubsetEntryData subsetEntry;
        subsetEntry.indexOffset = baseIndexOffset; // baseIndexOffset will be after lod indexes if available
        subsetEntry.indexLength = indexes.size(); // Yes, only original index values, because this is for the non-lod indexes
        subsetEntry.name = QString::fromUtf8(scene.mMaterials[mesh->mMaterialIndex]->GetName().C_Str());
        subsetEntry.lightmapWidth = 0;
        subsetEntry.lightmapHeight = 0;
        subsetEntry.lods = meshLods;
        subsetData.append(subsetEntry);

        // Fill the rest of the vertex data
        baseIndex += vertexAttributes.size(); // Final count of vertices added
        for (const auto &vertex : vertexAttributes)
            vertexBufferData.addVertexAttributeData(vertex, requirments);

    }

    // Now that we have all the data for the mesh, generate the entries list
    QVector<QSSGMesh::AssetVertexEntry> entries = vertexBufferData.createEntries(requirments);

    QVector<QSSGMesh::AssetMeshSubset> subsets;
    for (const SubsetEntryData &subset : subsetData) {
        subsets.append({
                           subset.name,
                           quint32(subset.indexLength),
                           quint32(subset.indexOffset),
                           0, // the builder will calculate the bounds from the position data
                           subset.lightmapWidth,
                           subset.lightmapHeight,
                           subset.lods
                       });
    }

    return QSSGMesh::Mesh::fromAssetData(entries, indexBufferData, indexType, subsets);
}

QT_END_NAMESPACE
