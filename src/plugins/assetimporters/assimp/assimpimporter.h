// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef ASSIMPIMPORTER_H
#define ASSIMPIMPORTER_H

#include <QtQuick3DAssetImport/private/qssgassetimporter_p.h>
#include <QtQuick3DAssetUtils/private/qssgqmlutilities_p.h>

#include <QtCore/QVector>
#include <QtCore/QList>
#include <QtCore/QStringList>
#include <QtCore/QTextStream>
#include <QtCore/QHash>
#include <QtCore/QTemporaryDir>
#include <QtCore/QSet>
#include <QtCore/QVariant>
#include <QtCore/QCborStreamWriter>
#include <QtCore/qjsonobject.h>

#include "assimputils.h"

#include <assimp/matrix4x4.h>
#include <assimp/material.h>
#include <assimp/anim.h>
#include <assimp/postprocess.h>

namespace Assimp {
class Importer;
}

struct aiNode;
struct aiCamera;
struct aiLight;
struct aiScene;
struct aiMesh;
struct aiAnimMesh;
struct aiBone;
struct weightKey {
    weightKey(double time, double value) {
        mTime = time;
        mValue = value;
    }
    double mTime;
    double mValue;
};

enum class QSSGRenderComponentType;

QT_BEGIN_NAMESPACE

class AssimpImporter : public QSSGAssetImporter
{
public:
    AssimpImporter();
    ~AssimpImporter() override;

    QString name() const override;
    QStringList inputExtensions() const override;
    QString outputExtension() const override;
    QString type() const override;
    QString typeDescription() const override;
    QJsonObject importOptions() const override;
    QString import(const QString &sourceFile, const QDir &savePath, const QJsonObject &options,
                   QStringList *generatedFiles) override;
    QString import(const QUrl &sourceFile, const QJsonObject &options, QSSGSceneDesc::Scene &scene) override;

private:
    void writeHeader(QTextStream &output);
    void processScene(QTextStream &output);
    void processNode(aiNode *node, QTextStream &output, int tabLevel = 1);
    void generateModelProperties(aiNode *modelNode, QTextStream &output, int tabLevel);
    QSSGQmlUtilities::PropertyMap::Type generateLightProperties(aiNode *lightNode, QTextStream &output, int tabLevel);
    QSSGQmlUtilities::PropertyMap::Type generateCameraProperties(aiNode *cameraNode, QTextStream &output, int tabLevel);
    void generateNodeProperties(aiNode *node, QTextStream &output, int tabLevel, aiMatrix4x4 *transformCorrection = nullptr, bool skipScaling = false);
    QString generateMeshFile(aiNode *node, QFile &file, const AssimpUtils::MeshList &meshes);
    void processMaterials(QTextStream &output);
    void generateMaterial(aiMaterial *material, QTextStream &output, int tabLevel = 1);
    QVector<QString> generateMorphing(aiNode *node, const AssimpUtils::MeshList &meshes, QTextStream &output, int tabLevel);
    QString generateImage(aiMaterial *material, aiTextureType textureType, unsigned index, int tabLevel);
    void processAnimations(QTextStream &output);
    template <typename T>
    void generateKeyframes(const QString &id, const QString &propertyName,
                           uint numKeys, const T *keys,
                           QTextStream &output, qreal animFreq, qreal &maxKeyframeTime);
    template <typename T>
    bool generateAnimationFile(QFile &file, const QList<T> &keyframes);
    void generateMorphKeyframes(const QString &id,
                                uint numKeys, const aiMeshMorphKey *keys,
                                QTextStream &output,
                                qreal animFreq, qreal &maxKeyframeTime);

    bool isModel(aiNode *node);
    bool isLight(aiNode *node);
    bool isCamera(aiNode *node);
    bool isBone(aiNode *node);
    QString generateUniqueId(const QString &id);
    void processOptions(QJsonObject options);
    bool checkBooleanOption(const QString &optionName, const QJsonObject &options);
    qreal getRealOption(const QString &optionName, const QJsonObject &options);

    Assimp::Importer *m_importer = nullptr;
    const aiScene *m_scene = nullptr;

    QHash<aiNode *, aiCamera *> m_cameras;
    QHash<aiNode *, aiLight *> m_lights;

    QVector<QHash<aiNode *, aiNodeAnim *> *> m_animations;
    QVector<QHash<aiNode *, aiMeshMorphAnim *> *> m_morphAnimations;
    // aiAnimation has mTicksPerSecond in order to correct the timing.
    // m_animFreqs[i] stores (1000 / mTicksPerSecond)
    //                   for m_animations[i] and m_morphAnimations[i]
    QVector<qreal> m_animFreqs;
    QHash<aiMaterial *, QString> m_materialIdMap;
    QSet<QString> m_uniqueIds;
    QHash<aiNode *, QString> m_nodeIdMap;
    QHash<aiNode *, QSSGQmlUtilities::PropertyMap::Type> m_nodeTypeMap;

    QDir m_savePath;
    QFileInfo m_sourceFile;
    QStringList m_generatedFiles;
    QMap<int, QString> m_embeddedTextureSources; // id -> destination path

    bool m_gltfMode = false;
    bool m_binaryKeyframes = false;
    bool m_forceMipMapGeneration = false;
    bool m_useFloatJointIndices = false;
    bool m_generateLightmapUV = false;
    int m_lightmapBaseResolution = 1024;
    qreal m_globalScaleValue = 1.0;

    QJsonObject m_options;
    aiPostProcessSteps m_postProcessSteps;

};

QT_END_NAMESPACE

#endif // ASSIMPIMPORTER_H
