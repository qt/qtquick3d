// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default

#pragma once

#include <QtQuick3DAssetUtils/private/qssgscenedesc_p.h>
#include <QtQuick3DGltf/private/qssggltfdocument_p.h>
#include <QtQuick3DUtils/private/qssgmesh_p.h>

#include <QtCore/qfileinfo.h>
#include <QtCore/qhash.h>
#include <QtCore/qjsonobject.h>

QT_BEGIN_NAMESPACE

struct GltfMorphTargetInfo
{
    int count = 0;
    bool hasPositions = false;
    bool hasNormals = false;
    bool hasTangents = false;
};

// Converts a QSSGGltfDocument into a QSSGSceneDesc::Scene
class GltfSceneConverter
{
public:
    struct Options
    {
        bool designStudioWorkarounds = false;
        bool joinIdenticalVertices = true;
        bool useFloatJointIndices = false;
        bool generateSmoothNormals = false;
        bool forceTangentGeneration = false;
        bool generateMipMaps = false;
        bool generateMeshLODs = false;
        float lodNormalMergeAngle = 60.0f;
        float lodNormalSplitAngle = 25.0f;
        float globalScaleValue = 1.0f;
        float animationSampleRate = 30.0f;
    };

    // Returns an empty string on success, the error message otherwise.
    QString convert(const QSSGGltfDocument &document, const QJsonObject &optionsObject,
                    const QFileInfo &sourceFile, QSSGSceneDesc::Scene &targetScene);

private:
    void processNode(int nodeIndex, QSSGSceneDesc::Node &parent);
    QSSGSceneDesc::Node *createSceneNode(const QSSGGltf::Node &node);
    void setNodeProperties(QSSGSceneDesc::Node &target, const QSSGGltf::Node &source);
    void setCameraProperties(QSSGSceneDesc::Camera &target, const QSSGGltf::Camera &source);
    void setLightProperties(QSSGSceneDesc::Light &target, const QSSGGltf::Light &source);
    void convertAnimations();
    void convertSkins();
    QSSGSceneDesc::Instancing *convertInstancing(const QSSGGltf::Node &source, QSSGSceneDesc::Node &owner);
    void setModelProperties(QSSGSceneDesc::Model &target, const QSSGGltf::Node &source, int nodeIndex);
    void setMaterialProperties(QSSGSceneDesc::Material &target, const QSSGGltf::Material &source);
    void setSpecularGlossyProperties(QSSGSceneDesc::Material &target, const QSSGGltf::Material &source);
    QSSGSceneDesc::Material *ensureMaterial(int materialIndex, QSSGSceneDesc::Node &owner);
    QSSGSceneDesc::Texture *ensureTexture(const QSSGGltf::TextureInfo &textureInfo, QSSGSceneDesc::Node &owner);

    static Options parseOptions(const QJsonObject &optionsObject);

    const QSSGGltfDocument *m_document = nullptr;
    QSSGSceneDesc::Scene *m_scene = nullptr;
    Options m_options;
    QHash<int, QSSGSceneDesc::Node *> m_nodeMap; // glTF node index -> scene node
    QHash<int, QSSGSceneDesc::Mesh *> m_meshMap; // glTF mesh index -> mesh resource
    QHash<int, QList<int>> m_meshUsedPrimitives; // glTF mesh index -> primitives that became subsets
    QHash<int, GltfMorphTargetInfo> m_meshMorphInfo; // glTF mesh index -> morph target info
    struct BuiltMesh {
        QSSGMesh::Mesh mesh;
        QList<int> usedPrimitives;
        GltfMorphTargetInfo morphInfo;
    };
    QHash<int, BuiltMesh> m_builtMeshes; // glTF mesh index -> pre-built mesh data
    QHash<int, QList<QSSGSceneDesc::MorphTarget *>> m_morphTargetMap; // glTF node index -> morph target nodes
    QHash<int, QSSGSceneDesc::Material *> m_materialMap; // glTF material index -> material (-1 = default)
    QHash<QByteArray, QSSGSceneDesc::Texture *> m_textureMap; // dedup key -> texture
    QHash<int, QSSGSceneDesc::TextureData *> m_textureDataMap; // glTF image index -> embedded data
    QHash<int, QSSGSceneDesc::Skin *> m_skinMap; // glTF skin index -> skin resource
};

QT_END_NAMESPACE
