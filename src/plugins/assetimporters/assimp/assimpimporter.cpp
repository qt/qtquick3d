/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "assimpimporter.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/Logger.hpp>
#include <assimp/DefaultLogger.hpp>
#include <assimp/postprocess.h>

#include <QtQuick3DAssetImport/private/qssgmeshutilities_p.h>
#include <QtQuick3DAssetImport/private/qssgqmlutilities_p.h>

#include <QtGui/QImage>
#include <QtGui/QImageReader>
#include <QtGui/QImageWriter>

#include <QtCore/QBuffer>
#include <QtCore/QByteArray>
#include <qmath.h>

#include <algorithm>
#include <limits>

QT_BEGIN_NAMESPACE

AssimpImporter::AssimpImporter()
{
    m_importer = new Assimp::Importer();
    // Remove primatives that are not Triangles
    m_importer->SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_POINT | aiPrimitiveType_LINE);
}

AssimpImporter::~AssimpImporter()
{
    delete m_importer;
}

const QString AssimpImporter::name() const
{
    return QStringLiteral("assimp");
}

const QStringList AssimpImporter::inputExtensions() const
{
    QStringList extensions;
    extensions.append(QStringLiteral("fbx"));
    extensions.append(QStringLiteral("dae"));
    extensions.append(QStringLiteral("obj"));
    extensions.append(QStringLiteral("blend"));
    extensions.append(QStringLiteral("gltf"));
    extensions.append(QStringLiteral("glb"));
    return extensions;
}

const QString AssimpImporter::outputExtension() const
{
    return QStringLiteral(".qml");
}

const QString AssimpImporter::type() const
{
    return QStringLiteral("Scene");
}

const QVariantMap AssimpImporter::importOptions() const
{
    return QVariantMap();
}

#define demonPostProcessPresets ( \
    aiProcess_CalcTangentSpace              |  \
    aiProcess_GenSmoothNormals              |  \
    aiProcess_JoinIdenticalVertices         |  \
    aiProcess_ImproveCacheLocality          |  \
    aiProcess_LimitBoneWeights              |  \
    aiProcess_RemoveRedundantMaterials      |  \
    aiProcess_SplitLargeMeshes              |  \
    aiProcess_Triangulate                   |  \
    aiProcess_GenUVCoords                   |  \
    aiProcess_SortByPType                   |  \
    aiProcess_FindDegenerates               |  \
    aiProcess_FindInvalidData               |  \
    0 )

const QString AssimpImporter::import(const QString &sourceFile, const QDir &savePath, const QVariantMap &options, QStringList *generatedFiles)
{
    QString errorString;
    m_savePath = savePath;
    m_sourceFile = QFileInfo(sourceFile);

    // Create savePath if it doesn't exist already
    m_savePath.mkdir(".");

    m_scene = m_importer->ReadFile(sourceFile.toStdString(), demonPostProcessPresets);
    if (!m_scene) {
        // Scene failed to load, use logger to get the reason
        return QString::fromLocal8Bit(m_importer->GetErrorString());
    }

    // Generate Embedded Texture Sources
    // This may be used in the future, but it's not supported with any of the formats
    // we currently support
//    for (uint i = 0; i < m_scene->mNumTextures; ++i) {
//        aiTexture *texture = m_scene->mTextures[i];
//        if (texture->mHeight == 0) {
//            // compressed format, try to load with Image Loader
//            QByteArray data(reinterpret_cast<char *>(texture->pcData), texture->mWidth);
//            QBuffer readBuffer(&data);
//            QByteArray format = texture->achFormatHint;
//            QImageReader imageReader(&readBuffer, format);
//            QImage image = imageReader.read();
//            if (image.isNull()) {
//                qWarning() << imageReader.errorString();
//                continue;
//            }

//            // ### maybe dont always use png
//            const QString saveFileName = savePath.absolutePath() +
//                    QDir::separator() + QStringLiteral("maps") +
//                    QDir::separator() + QString::number(i) +
//                    QStringLiteral(".png");
//            image.save(saveFileName);

//        } else {
//            // Raw format, just convert data to QImage
//            QImage rawImage(reinterpret_cast<uchar *>(texture->pcData), texture->mWidth, texture->mHeight, QImage::Format_RGBA8888);
//            const QString saveFileName = savePath.absolutePath() +
//                    QDir::separator() + QStringLiteral("maps") +
//                    QDir::separator() + QString::number(i) +
//                    QStringLiteral(".png");
//            rawImage.save(saveFileName);
//        }
//    }

    // Check for Cameras
    if (m_scene->HasCameras()) {
        for (uint i = 0; i < m_scene->mNumCameras; ++i) {
            aiCamera *camera = m_scene->mCameras[i];
            aiNode *node = m_scene->mRootNode->FindNode(camera->mName);
            if (camera && node)
                m_cameras.insert(node, camera);
        }
    }

    // Check for Lights
    if (m_scene->HasLights()) {
        for (uint i = 0; i < m_scene->mNumLights; ++i) {
            aiLight *light = m_scene->mLights[i];
            aiNode *node = m_scene->mRootNode->FindNode(light->mName);
            if (light && node)
                m_lights.insert(node, light);
        }
    }

    // Materials

    // Traverse Node Tree

    // Animations (timeline based)

    // Create QML Component
    QFileInfo sourceFileInfo(sourceFile);


    QString targetFileName = savePath.absolutePath() + QDir::separator() +
            QSSGQmlUtilities::qmlComponentName(sourceFileInfo.baseName()) +
            QStringLiteral(".qml");
    QFile targetFile(targetFileName);
    if (!targetFile.open(QIODevice::WriteOnly)) {
        errorString += QString("Could not write to file: ") + targetFileName;
    } else {
        QTextStream output(&targetFile);

        // Imports header
        writeHeader(output);

        // Component Code
        processNode(m_scene->mRootNode, output);

        targetFile.close();
        if (generatedFiles)
            *generatedFiles += targetFileName;
    }

    return errorString;
}

void AssimpImporter::writeHeader(QTextStream &output)
{
    output << "import QtQuick3D 1.0" << endl;
    output << "import QtQuick 2.12" << endl;
    output << "import QtQuick.Timeline 1.0" << endl;
    output << endl;
}

void AssimpImporter::processNode(aiNode *node, QTextStream &output, int tabLevel)
{
    aiNode *currentNode = node;
    if (currentNode) {
        // Figure out what kind of node this is
        if (isModel(currentNode)) {
            // Model
            output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("Model {") << endl;
            generateModelProperties(currentNode, output, tabLevel + 1);
        } else if (isLight(currentNode)) {
            // Light
            output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("Light {") << endl;
            generateLightProperties(currentNode, output, tabLevel + 1);
        } else if (isCamera(currentNode)) {
            // Camera
            output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("Camera {") << endl;
            generateCameraProperties(currentNode, output, tabLevel + 1);
        } else {
            // Transform Node

            // ### Make empty transform node removal optional
            // Check if the node actually does something before generating it
            // and return early without processing the rest of the branch
            if (!containsNodesOfConsequence(node))
                return;

            output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("Node {") << endl;
            generateNodeProperties(currentNode, output, tabLevel + 1);
        }

        // Process All Children Nodes
        for (uint i = 0; i < currentNode->mNumChildren; ++i)
            processNode(currentNode->mChildren[i], output, tabLevel + 1);

        // Write the QML Footer
        output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("}") << endl;
    }
}

void AssimpImporter::generateModelProperties(aiNode *modelNode, QTextStream &output, int tabLevel)
{
    generateNodeProperties(modelNode, output, tabLevel);

    // source
    // Combine all the meshes referenced by this model into a single MultiMesh file
    QVector<aiMesh *> meshes;
    QVector<aiMaterial *> materials;
    for (uint i = 0; i < modelNode->mNumMeshes; ++i) {
        aiMesh *mesh = m_scene->mMeshes[modelNode->mMeshes[i]];
        aiMaterial *material = m_scene->mMaterials[mesh->mMaterialIndex];
        meshes.append(mesh);
        materials.append(material);
    }

    QString outputMeshFile = QStringLiteral("meshes/") +
            QString::fromUtf8(modelNode->mName.C_Str()) + QStringLiteral(".mesh");

    m_savePath.mkdir(QStringLiteral("./meshes"));
    QFile meshFile(m_savePath.absolutePath() + QDir::separator() + outputMeshFile);
    generateMeshFile(meshFile, meshes);

    output << QSSGQmlUtilities::insertTabs(tabLevel) << "source: \"" << outputMeshFile << QStringLiteral("\"") << endl;

    // skeletonRoot

    // materials
    // If there are any new materials, add them as children of the Model first
    for (int i = 0; i < materials.count(); ++i) {
        if (!m_materialIdMap.contains(materials[i])) {
            generateMaterial(materials[i], output, tabLevel);
            output << endl;
        }
    }

    // For each sub-mesh, generate a material reference for this list
    output << QSSGQmlUtilities::insertTabs(tabLevel) << "materials: [" << endl;
    for (int i = 0; i < materials.count(); ++i) {
        output << QSSGQmlUtilities::insertTabs(tabLevel + 1) << m_materialIdMap[materials[i]];
        if (i < materials.count() - 1)
            output << QStringLiteral(",");
        output << endl;
    }
    output << QSSGQmlUtilities::insertTabs(tabLevel) << "]" << endl;
}

void AssimpImporter::generateLightProperties(aiNode *lightNode, QTextStream &output, int tabLevel)
{
    aiLight *light = m_lights.value(lightNode);
    // We assume that the direction vector for a light is (0, 0, 1)
    // so if the direction vector is non-null, but not (0, 0, 1) we
    // need to correct the translation
    aiMatrix4x4 correctionMatrix;
    if (light->mDirection != aiVector3D(0, 0, 0)) {
        if (light->mDirection != aiVector3D(0, 0, 1)) {
            aiMatrix4x4::FromToMatrix(light->mDirection, aiVector3D(0, 0, 1), correctionMatrix);
        }
    }

    generateNodeProperties(lightNode, output, tabLevel, correctionMatrix, true);

    // lightType
    if (light->mType == aiLightSource_DIRECTIONAL || light->mType == aiLightSource_AMBIENT ) {
        QSSGQmlUtilities::writeQmlPropertyHelper(output, tabLevel, QSSGQmlUtilities::PropertyMap::Light, QStringLiteral("lightType"), QStringLiteral("Light.Directional"));
    } else if (light->mType == aiLightSource_POINT) {
        QSSGQmlUtilities::writeQmlPropertyHelper(output, tabLevel, QSSGQmlUtilities::PropertyMap::Light, QStringLiteral("lightType"), QStringLiteral("Light.Point"));
    } else if (light->mType == aiLightSource_SPOT) {
        // ### This is not and area light, but it's the closest we have right now
        QSSGQmlUtilities::writeQmlPropertyHelper(output, tabLevel, QSSGQmlUtilities::PropertyMap::Light, QStringLiteral("lightType"), QStringLiteral("Light.Area"));
    }

    // diffuseColor
    QColor diffuseColor = QColor::fromRgbF(light->mColorDiffuse.r, light->mColorDiffuse.g, light->mColorDiffuse.b);
    QSSGQmlUtilities::writeQmlPropertyHelper(output,tabLevel, QSSGQmlUtilities::PropertyMap::Light, QStringLiteral("diffuseColor"), diffuseColor);

    // specularColor
    QColor specularColor = QColor::fromRgbF(light->mColorSpecular.r, light->mColorSpecular.g, light->mColorSpecular.b);
    QSSGQmlUtilities::writeQmlPropertyHelper(output,tabLevel, QSSGQmlUtilities::PropertyMap::Light, QStringLiteral("specularColor"), specularColor);

    // ambientColor
    if (light->mType == aiLightSource_AMBIENT) {
        // We only want ambient light color if it is explicit
        QColor ambientColor = QColor::fromRgbF(light->mColorAmbient.r, light->mColorAmbient.g, light->mColorAmbient.b);
        QSSGQmlUtilities::writeQmlPropertyHelper(output,tabLevel, QSSGQmlUtilities::PropertyMap::Light, QStringLiteral("ambientColor"), ambientColor);
    }
    // brightness
    //QSSGQmlUtilities::writeQmlPropertyHelper(output,tabLevel, QSSGQmlUtilities::PropertyMap::Light, QStringLiteral("brightness"), light->mAttenuationConstant);

    // linearFade
    QSSGQmlUtilities::writeQmlPropertyHelper(output,tabLevel, QSSGQmlUtilities::PropertyMap::Light, QStringLiteral("linearFade"), light->mAttenuationLinear);

    // exponentialFade
    QSSGQmlUtilities::writeQmlPropertyHelper(output,tabLevel, QSSGQmlUtilities::PropertyMap::Light, QStringLiteral("exponentialFade"), light->mAttenuationQuadratic);

    // areaWidth

    // areaHeight

    // castShadow

    // shadowBias

    // shadowFactor

    // shadowMapResolution

    // shadowMapFar

    // shadowMapFieldOFView

    // shadowFilter
}

void AssimpImporter::generateCameraProperties(aiNode *cameraNode, QTextStream &output, int tabLevel)
{
    aiCamera *camera = m_cameras.value(cameraNode);

    // We assume these default forward and up vectors, so if this isn't
    // the case we have to do additional transform
    aiMatrix4x4 correctionMatrix;
    if (camera->mLookAt != aiVector3D(0, 0, 1))
    {
        aiMatrix4x4 lookAtCorrection;
        aiMatrix4x4::FromToMatrix(camera->mLookAt, aiVector3D(0, 0, 1), lookAtCorrection);
        correctionMatrix *= lookAtCorrection;
    }

    if (camera->mUp != aiVector3D(0, 1, 0)) {
        aiMatrix4x4 upCorrection;
        aiMatrix4x4::FromToMatrix(camera->mUp, aiVector3D(0, 1, 0), upCorrection);
        correctionMatrix *= upCorrection;
    }

    generateNodeProperties(cameraNode, output, tabLevel, correctionMatrix, true);

    // clipNear
    QSSGQmlUtilities::writeQmlPropertyHelper(output,tabLevel, QSSGQmlUtilities::PropertyMap::Camera, QStringLiteral("clipNear"), camera->mClipPlaneNear);

    // clipFar
    QSSGQmlUtilities::writeQmlPropertyHelper(output,tabLevel, QSSGQmlUtilities::PropertyMap::Camera, QStringLiteral("clipFar"), camera->mClipPlaneFar);

    // fieldOfView
    float fov = qRadiansToDegrees(camera->mHorizontalFOV);
    QSSGQmlUtilities::writeQmlPropertyHelper(output,tabLevel, QSSGQmlUtilities::PropertyMap::Camera, QStringLiteral("fieldOfView"), fov);

    // isFieldOFViewHorizontal
    QSSGQmlUtilities::writeQmlPropertyHelper(output,tabLevel, QSSGQmlUtilities::PropertyMap::Camera, QStringLiteral("isFieldOFViewHorizontal"), true);


    // projectionMode

    // scaleMode

    // scaleAnchor

    // frustomScaleX

    // frustomScaleY

}

void AssimpImporter::generateNodeProperties(aiNode *node, QTextStream &output, int tabLevel, const aiMatrix4x4 &transformCorrection, bool skipScaling)
{
    // id
    QString name = QString::fromUtf8(node->mName.C_Str());
    if (!name.isEmpty()) {
        // ### we may need to account of non-unique and empty names
        QString id = generateUniqueId(QSSGQmlUtilities::sanitizeQmlId(name));
        output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("id: ") << id << endl;
    }

    // Apply correction if necessary
    aiMatrix4x4 transformMatrix = node->mTransformation;
    if (!transformCorrection.IsIdentity())
        transformMatrix *= transformCorrection;

    // Decompose Transform Matrix to get properties
    aiVector3D scaling;
    aiVector3D rotation;
    aiVector3D translation;
    transformMatrix.Decompose(scaling, rotation, translation);

    // translate
    QSSGQmlUtilities::writeQmlPropertyHelper(output, tabLevel, QSSGQmlUtilities::PropertyMap::Node, QStringLiteral("x"), translation.x);
    QSSGQmlUtilities::writeQmlPropertyHelper(output, tabLevel, QSSGQmlUtilities::PropertyMap::Node, QStringLiteral("y"), translation.y);
    QSSGQmlUtilities::writeQmlPropertyHelper(output, tabLevel, QSSGQmlUtilities::PropertyMap::Node, QStringLiteral("z"), -translation.z);

    // rotation
    QVector3D rotationAngles(qRadiansToDegrees(rotation.x),
                             qRadiansToDegrees(rotation.y),
                             qRadiansToDegrees(rotation.z));
    QSSGQmlUtilities::writeQmlPropertyHelper(output, tabLevel, QSSGQmlUtilities::PropertyMap::Node, QStringLiteral("rotation"), rotationAngles);

    // scale
    if (!skipScaling)
        QSSGQmlUtilities::writeQmlPropertyHelper(output, tabLevel, QSSGQmlUtilities::PropertyMap::Node, QStringLiteral("scale"), QVector3D(scaling.x, scaling.y, scaling.z));

    // pivot

    // opacity

    // boneid

    // rotation order
    QSSGQmlUtilities::writeQmlPropertyHelper(output, tabLevel, QSSGQmlUtilities::PropertyMap::Node, QStringLiteral("rotationOrder"), QStringLiteral("Node.XYZr"));

    // orientation
    QSSGQmlUtilities::writeQmlPropertyHelper(output, tabLevel, QSSGQmlUtilities::PropertyMap::Node, QStringLiteral("orientation"), QStringLiteral("Node.RightHanded"));

    // visible

}

QString AssimpImporter::generateMeshFile(QIODevice &file, const QVector<aiMesh *> &meshes)
{
    if (!file.open(QIODevice::WriteOnly))
        return QStringLiteral("Could not open device to write mesh file");


    auto meshBuilder = QSSGMeshUtilities::QSSGMeshBuilder::createMeshBuilder();

    struct SubsetEntryData {
        QString name;
        int indexLength;
        int indexOffset;
    };

    // Check if we need placeholders in certain channels
    bool needsPositionData = false;
    bool needsNormalData = false;
    bool needsUV1Data = false;
    bool needsUV2Data = false;
    bool needsTangentData = false;
    bool needsVertexColorData = false;
    unsigned uv1Components = 0;
    unsigned uv2Components = 0;
    unsigned totalVertices = 0;
    for (const auto *mesh : meshes) {
        totalVertices += mesh->mNumVertices;
        uv1Components = qMax(mesh->mNumUVComponents[0], uv1Components);
        uv2Components = qMax(mesh->mNumUVComponents[1], uv2Components);
        needsPositionData |= mesh->HasPositions();
        needsNormalData |= mesh->HasNormals();
        needsUV1Data |= mesh->HasTextureCoords(0);
        needsUV2Data |= mesh->HasTextureCoords(1);
        needsTangentData |= mesh->HasTangentsAndBitangents();
        needsVertexColorData |=mesh->HasVertexColors(0);
    }

    QByteArray positionData;
    QByteArray normalData;
    QByteArray uv1Data;
    QByteArray uv2Data;
    QByteArray tangentData;
    QByteArray binormalData;
    QByteArray vertexColorData;
    QByteArray indexBufferData;
    QVector<SubsetEntryData> subsetData;
    quint32 baseIndex = 0;
    QSSGRenderComponentType indexType = QSSGRenderComponentType::UnsignedInteger32;
    if ((totalVertices / 3) > std::numeric_limits<quint16>::max())
        indexType = QSSGRenderComponentType::UnsignedInteger32;

    for (const auto *mesh : meshes) {
        // Position
        if (mesh->HasPositions())
            positionData += QByteArray(reinterpret_cast<char*>(mesh->mVertices), mesh->mNumVertices * 3 * getSizeOfType(QSSGRenderComponentType::Float32));
        else if (needsPositionData)
            positionData += QByteArray(mesh->mNumVertices * 3 * getSizeOfType(QSSGRenderComponentType::Float32), '\0');

        // Normal
        if (mesh->HasNormals())
            normalData += QByteArray(reinterpret_cast<char*>(mesh->mNormals), mesh->mNumVertices * 3 * getSizeOfType(QSSGRenderComponentType::Float32));
        else if (needsNormalData)
            normalData += QByteArray(mesh->mNumVertices * 3 * getSizeOfType(QSSGRenderComponentType::Float32), '\0');

        // UV1
        if (mesh->HasTextureCoords(0)) {
            QVector<float> uvCoords;
            uvCoords.resize(uv1Components * mesh->mNumVertices);
            for (uint i = 0; i < mesh->mNumVertices; ++i) {
                int offset = i * uv1Components;
                aiVector3D *textureCoords = mesh->mTextureCoords[0];
                uvCoords[offset] = textureCoords[i].x;
                uvCoords[offset + 1] = textureCoords[i].y;
                if (uv1Components == 3)
                    uvCoords[offset + 2] = textureCoords[i].z;
            }
            uv1Data += QByteArray(reinterpret_cast<const char*>(uvCoords.constData()), uvCoords.size() * sizeof(float));
        } else {
            uv1Data += QByteArray(mesh->mNumVertices * uv1Components * getSizeOfType(QSSGRenderComponentType::Float32), '\0');
        }

        // UV2
        if (mesh->HasTextureCoords(1)) {
            QVector<float> uvCoords;
            uvCoords.resize(uv2Components * mesh->mNumVertices);
            for (uint i = 0; i < mesh->mNumVertices; ++i) {
                int offset = i * uv2Components;
                aiVector3D *textureCoords = mesh->mTextureCoords[1];
                uvCoords[offset] = textureCoords[i].x;
                uvCoords[offset + 1] = textureCoords[i].y;
                if (uv2Components == 3)
                    uvCoords[offset + 2] = textureCoords[i].z;
            }
            uv2Data += QByteArray(reinterpret_cast<const char*>(uvCoords.constData()), uvCoords.size() * sizeof(float));
        } else {
            uv2Data += QByteArray(mesh->mNumVertices * uv2Components * getSizeOfType(QSSGRenderComponentType::Float32), '\0');
        }

        if (mesh->HasTangentsAndBitangents()) {
            // Tangents
            tangentData += QByteArray(reinterpret_cast<char*>(mesh->mTangents), mesh->mNumVertices * 3 * getSizeOfType(QSSGRenderComponentType::Float32));
            // Binormals (They are actually supposed to be Bitangents despite what they are called)
            binormalData += QByteArray(reinterpret_cast<char*>(mesh->mBitangents), mesh->mNumVertices * 3 * getSizeOfType(QSSGRenderComponentType::Float32));
        } else if (needsTangentData) {
            tangentData += QByteArray(mesh->mNumVertices * 3 * getSizeOfType(QSSGRenderComponentType::Float32), '\0');
            binormalData += QByteArray(mesh->mNumVertices * 3 * getSizeOfType(QSSGRenderComponentType::Float32), '\0');
        }
        // ### Bones + Weights

        // Color
        if (mesh->HasVertexColors(0))
            vertexColorData += QByteArray(reinterpret_cast<char*>(mesh->mColors[0]), mesh->mNumVertices * 4 * getSizeOfType(QSSGRenderComponentType::Float32));
        else if (needsVertexColorData)
            vertexColorData += QByteArray(mesh->mNumVertices * 4 * getSizeOfType(QSSGRenderComponentType::Float32), '\0');
        // Index Buffer
        QVector<quint32> indexes;
        indexes.reserve(mesh->mNumFaces * 3);

        for (unsigned int faceIndex = 0;faceIndex < mesh->mNumFaces; ++faceIndex) {
            const auto face = mesh->mFaces[faceIndex];
            // Faces should always have 3 indicides
            Q_ASSERT(face.mNumIndices == 3);
            indexes.append(quint32(face.mIndices[0]) + baseIndex);
            indexes.append(quint32(face.mIndices[1]) + baseIndex);
            indexes.append(quint32(face.mIndices[2]) + baseIndex);
        }
        // Since we might be combining multiple meshes together, we also need to change the index offset
        baseIndex = *std::max_element(indexes.constBegin(), indexes.constEnd()) + 1;

        SubsetEntryData subsetEntry;
        subsetEntry.indexOffset = indexBufferData.length() / getSizeOfType(indexType);
        subsetEntry.indexLength = indexes.length();
        if (indexType == QSSGRenderComponentType::UnsignedInteger32) {
            indexBufferData += QByteArray(reinterpret_cast<const char *>(indexes.constData()), indexes.length() * getSizeOfType(indexType));
        } else {
            // convert data to quint16
            QVector<quint16> shortIndexes;
            shortIndexes.resize(indexes.length());
            for (int i = 0; i < shortIndexes.length(); ++i)
                shortIndexes[i] = quint16(indexes[i]);
            indexBufferData += QByteArray(reinterpret_cast<const char *>(shortIndexes.constData()), shortIndexes.length() * getSizeOfType(indexType));
        }

        // Subset
        subsetEntry.name = QString::fromUtf8(m_scene->mMaterials[mesh->mMaterialIndex]->GetName().C_Str());
        subsetData.append(subsetEntry);
    }

    // Vertex Buffer Entries
    QVector<QSSGMeshUtilities::MeshBuilderVBufEntry> entries;
    if (positionData.length() > 0) {
        QSSGMeshUtilities::MeshBuilderVBufEntry positionAttribute( QSSGMeshUtilities::Mesh::getPositionAttrName(),
                                                                     positionData,
                                                                     QSSGRenderComponentType::Float32,
                                                                     3);
        entries.append(positionAttribute);
    }
    if (normalData.length() > 0) {
        QSSGMeshUtilities::MeshBuilderVBufEntry normalAttribute( QSSGMeshUtilities::Mesh::getNormalAttrName(),
                                                                   normalData,
                                                                   QSSGRenderComponentType::Float32,
                                                                   3);
        entries.append(normalAttribute);
    }
    if (uv1Data.length() > 0) {
        QSSGMeshUtilities::MeshBuilderVBufEntry uv1Attribute( QSSGMeshUtilities::Mesh::getUVAttrName(),
                                                                uv1Data,
                                                                QSSGRenderComponentType::Float32,
                                                                uv1Components);
        entries.append(uv1Attribute);
    }
    if (uv2Data.length() > 0) {
        QSSGMeshUtilities::MeshBuilderVBufEntry uv2Attribute( QSSGMeshUtilities::Mesh::getUV2AttrName(),
                                                                uv2Data,
                                                                QSSGRenderComponentType::Float32,
                                                                uv2Components);
        entries.append(uv2Attribute);
    }

    if (tangentData.length() > 0) {
        QSSGMeshUtilities::MeshBuilderVBufEntry tangentsAttribute( QSSGMeshUtilities::Mesh::getTexTanAttrName(),
                                                                     tangentData,
                                                                     QSSGRenderComponentType::Float32,
                                                                     3);
        entries.append(tangentsAttribute);
    }

    if (binormalData.length() > 0) {
        QSSGMeshUtilities::MeshBuilderVBufEntry binormalAttribute( QSSGMeshUtilities::Mesh::getTexBinormalAttrName(),
                                                                     binormalData,
                                                                     QSSGRenderComponentType::Float32,
                                                                     3);
        entries.append(binormalAttribute);
    }

    if (vertexColorData.length() > 0) {
        QSSGMeshUtilities::MeshBuilderVBufEntry vertexColorAttribute( QSSGMeshUtilities::Mesh::getColorAttrName(),
                                                                        vertexColorData,
                                                                        QSSGRenderComponentType::Float32,
                                                                        4);
        entries.append(vertexColorAttribute);
    }

    meshBuilder->setVertexBuffer(entries);
    meshBuilder->setIndexBuffer(indexBufferData, indexType);

    // Subsets
    for (const auto &subset : subsetData)
        meshBuilder->addMeshSubset(reinterpret_cast<const char16_t *>(subset.name.utf16()),
                                   subset.indexLength,
                                   subset.indexOffset,
                                   0);



    auto &outputMesh = meshBuilder->getMesh();
    outputMesh.saveMulti(file, 0);

    file.close();
    return QString();
}

namespace {

QColor aiColorToQColor(const aiColor3D &color)
{
    return QColor::fromRgbF(color.r, color.g, color.b);
}
}

void AssimpImporter::generateMaterial(aiMaterial *material, QTextStream &output, int tabLevel)
{
    output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("DefaultMaterial {") << endl;

    // id
    QString id = generateUniqueId(QSSGQmlUtilities::sanitizeQmlId(material->GetName().C_Str() + QStringLiteral("_material")));
    output << QSSGQmlUtilities::insertTabs(tabLevel + 1) << QStringLiteral("id: ") << id << endl;
    m_materialIdMap.insert(material, id);

    aiReturn result;

    int shadingModel = 0;
    result = material->Get(AI_MATKEY_SHADING_MODEL, shadingModel);
    // lighting
    if (result == aiReturn_SUCCESS) {
        if (shadingModel == aiShadingMode_NoShading)
            output << QSSGQmlUtilities::insertTabs(tabLevel + 1) << QStringLiteral("lighting: DefaultMaterial.NoLighting") << endl;
    }


    QString diffuseMapImage = generateImage(material, aiTextureType_DIFFUSE, 0, tabLevel + 1);
    if (!diffuseMapImage.isNull())
        output << QSSGQmlUtilities::insertTabs(tabLevel + 1) << QStringLiteral("diffuseMap: ") << diffuseMapImage << endl;

    QString diffuseMap2Image = generateImage(material, aiTextureType_DIFFUSE, 1, tabLevel + 1);
    if (!diffuseMap2Image.isNull())
        output << QSSGQmlUtilities::insertTabs(tabLevel + 1) << QStringLiteral("diffuseMap2: ") << diffuseMap2Image << endl;

    QString diffuseMap3Image = generateImage(material, aiTextureType_DIFFUSE, 2, tabLevel + 1);
    if (!diffuseMap3Image.isNull())
        output << QSSGQmlUtilities::insertTabs(tabLevel + 1) << QStringLiteral("diffuseMap3: ") << diffuseMap3Image << endl;

    // For some reason the normal behavior is that either you have a diffuseMap[s] or a diffuse color
    // but no a mix of both... So only set the diffuse color if none of the diffuse maps are set:
    if (diffuseMapImage.isNull() && diffuseMap2Image.isNull() && diffuseMap3Image.isNull()) {
        aiColor3D diffuseColor;
        result = material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor);
        if (result == aiReturn_SUCCESS) {
            QSSGQmlUtilities::writeQmlPropertyHelper(output,
                                                       tabLevel + 1,
                                                       QSSGQmlUtilities::PropertyMap::DefaultMaterial,
                                                       QStringLiteral("diffuseColor"),
                                                       aiColorToQColor(diffuseColor));
        }
    }

    // emissivePower

    QString emissiveMapImage = generateImage(material, aiTextureType_EMISSIVE, 0, tabLevel + 1);
    if (!emissiveMapImage.isNull())
        output << QSSGQmlUtilities::insertTabs(tabLevel + 1) << QStringLiteral("emissiveMap: ") << emissiveMapImage << endl;

    QString emissiveMap2Image = generateImage(material, aiTextureType_EMISSIVE, 0, tabLevel + 1);
    if (!emissiveMap2Image.isNull())
        output << QSSGQmlUtilities::insertTabs(tabLevel + 1) << QStringLiteral("emissiveMap2: ") << emissiveMap2Image << endl;

    // emissiveColor AI_MATKEY_COLOR_EMISSIVE
    aiColor3D emissiveColor;
    result = material->Get(AI_MATKEY_COLOR_EMISSIVE, emissiveColor);
    if (result == aiReturn_SUCCESS) {
        // ### set emissive color
    }
    // specularReflectionMap

    QString specularMapImage = generateImage(material, aiTextureType_SPECULAR, 0, tabLevel + 1);
    if (!specularMapImage.isNull())
        output << QSSGQmlUtilities::insertTabs(tabLevel + 1) << QStringLiteral("specularMap: ") << specularMapImage << endl;

    // specularModel AI_MATKEY_SHADING_MODEL

    // specularTint AI_MATKEY_COLOR_SPECULAR
    aiColor3D specularTint;
    result = material->Get(AI_MATKEY_COLOR_SPECULAR, specularTint);
    if (result == aiReturn_SUCCESS) {
        // ### set specular color
    }

    // indexOfRefraction AI_MATKEY_REFRACTI

    // fresnelPower

    // specularAmount

    // specularRoughness

    // roughnessMap

    // opacity AI_MATKEY_OPACITY
    ai_real opacity;
    result = material->Get(AI_MATKEY_OPACITY, opacity);
    if (result == aiReturn_SUCCESS) {
        QSSGQmlUtilities::writeQmlPropertyHelper(output,
                                                   tabLevel + 1,
                                                   QSSGQmlUtilities::PropertyMap::DefaultMaterial,
                                                   QStringLiteral("opacity"),
                                                   opacity);
    }

    // opacityMap aiTextureType_OPACITY 0
    QString opacityMapImage = generateImage(material, aiTextureType_OPACITY, 0, tabLevel + 1);
    if (!opacityMapImage.isNull())
        output << QSSGQmlUtilities::insertTabs(tabLevel + 1) << QStringLiteral("opacityMap: ") << opacityMapImage;

    // bumpMap aiTextureType_HEIGHT 0
    QString bumpMapImage = generateImage(material, aiTextureType_HEIGHT, 0, tabLevel + 1);
    if (!bumpMapImage.isNull())
        output << QSSGQmlUtilities::insertTabs(tabLevel + 1) << QStringLiteral("bumpMap: ") << bumpMapImage;

    // bumpAmount AI_MATKEY_BUMPSCALING

    // normalMap aiTextureType_NORMALS 0
    QString normalMapImage = generateImage(material, aiTextureType_NORMALS, 0, tabLevel + 1);
    if (!normalMapImage.isNull())
        output << QSSGQmlUtilities::insertTabs(tabLevel + 1) << QStringLiteral("normalMap: ") << normalMapImage;

    // translucencyMap

    // translucentFalloff AI_MATKEY_TRANSPARENCYFACTOR

    // diffuseLightWrap

    // (enable) vertexColors

    // displacementMap aiTextureType_DISPLACEMENT 0
    QString displacementMapImage = generateImage(material, aiTextureType_DISPLACEMENT, 0, tabLevel + 1);
    if (!displacementMapImage.isNull())
        output << QSSGQmlUtilities::insertTabs(tabLevel + 1) << QStringLiteral("displacementMap: ") << displacementMapImage;

    // displacementAmount

    output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("}");
}

namespace  {
QString aiTilingMode(int tilingMode) {
    if (tilingMode == aiTextureMapMode_Wrap)
        return QStringLiteral("Texture.Repeat");
    if (tilingMode == aiTextureMapMode_Mirror)
        return QStringLiteral("Texture.Mirror");

    return QStringLiteral("Texture.ClampToEdge");
}
}

QString AssimpImporter::generateImage(aiMaterial *material, aiTextureType textureType, unsigned index, int tabLevel)
{
    // Figure out if there is actually something to generate
    aiString texturePath;
    material->Get(AI_MATKEY_TEXTURE(textureType, index), texturePath);
    // If there is no texture, then there is nothing to generate
    if (texturePath.length == 0)
        return QString();
    QString texture = QString::fromUtf8(texturePath.C_Str());
    // Check that this file exists
    QFileInfo sourceFile(m_sourceFile.absolutePath() + QDir::separator() + texture);
    // If it doesn't exist, there is nothing to generate
    if (!sourceFile.exists()) {
        qWarning() << sourceFile.absoluteFilePath() << "does not exist, skipping";
        return QString();
    }
    QString targetFileName = QStringLiteral("maps/") + sourceFile.fileName();
    // Copy the file to the maps directory
    m_savePath.mkdir(QStringLiteral("./maps"));
    QFileInfo targetFile = m_savePath.absolutePath() + QDir::separator() + targetFileName;
    if (QFile::copy(sourceFile.absoluteFilePath(), targetFile.absoluteFilePath()))
        m_generatedFiles += targetFile.absoluteFilePath();

    // Start QML generation
    QString outputString;
    QTextStream output(&outputString, QIODevice::WriteOnly);
    output << QStringLiteral("Texture {") << endl;

    output << QSSGQmlUtilities::insertTabs(tabLevel + 1) << QStringLiteral("source: \"") << targetFileName << QStringLiteral("\"") << endl;

    // mapping
    int textureMapping;
    aiReturn result = material->Get(AI_MATKEY_MAPPING(textureType, index), textureMapping);
    if (result == aiReturn_SUCCESS) {
        if (textureMapping == aiTextureMapping_UV) {
            // So we should be able to always hit this case by passing the right flags
            // at import.
            QSSGQmlUtilities::writeQmlPropertyHelper(output,
                                                       tabLevel + 1,
                                                       QSSGQmlUtilities::PropertyMap::Image,
                                                       QStringLiteral("mappingMode"),
                                                       QStringLiteral("Texture.Normal"));
            // It would be possible to use another channel than UV0 to map image data
            // but for now we force everything to use UV0
            //int uvSource;
            //material->Get(AI_MATKEY_UVWSRC(textureType, index), uvSource);
        } else if (textureMapping == aiTextureMapping_SPHERE) {
            // (not supported)
        } else if (textureMapping == aiTextureMapping_CYLINDER) {
            // (not supported)
        } else if (textureMapping == aiTextureMapping_BOX) {
            // (not supported)
        } else if (textureMapping == aiTextureMapping_PLANE) {
            // (not supported)
        } else {
            // other... (not supported)
        }
    }

    // mapping mode U
    int mappingModeU;
    result = material->Get(AI_MATKEY_MAPPINGMODE_U(textureType, index), mappingModeU);
    if (result == aiReturn_SUCCESS) {
        QSSGQmlUtilities::writeQmlPropertyHelper(output,
                                                   tabLevel + 1,
                                                   QSSGQmlUtilities::PropertyMap::Image,
                                                   QStringLiteral("tilingModeHorizontal"),
                                                   aiTilingMode(mappingModeU));
    } else {
        // import formats seem to think repeat is the default
        QSSGQmlUtilities::writeQmlPropertyHelper(output,
                                                   tabLevel + 1,
                                                   QSSGQmlUtilities::PropertyMap::Image,
                                                   QStringLiteral("tilingModeHorizontal"),
                                                   QStringLiteral("Texture.Repeat"));
    }

    // mapping mode V
    int mappingModeV;
    result = material->Get(AI_MATKEY_MAPPINGMODE_V(textureType, index), mappingModeV);
    if (result == aiReturn_SUCCESS) {
        QSSGQmlUtilities::writeQmlPropertyHelper(output,
                                                   tabLevel + 1,
                                                   QSSGQmlUtilities::PropertyMap::Image,
                                                   QStringLiteral("tilingModeVertical"),
                                                   aiTilingMode(mappingModeV));
    } else {
        // import formats seem to think repeat is the default
        QSSGQmlUtilities::writeQmlPropertyHelper(output,
                                                   tabLevel + 1,
                                                   QSSGQmlUtilities::PropertyMap::Image,
                                                   QStringLiteral("tilingModeVertical"),
                                                   QStringLiteral("Texture.Repeat"));
    }

    aiUVTransform transforms;
    result = material->Get(AI_MATKEY_UVTRANSFORM(textureType, index), transforms);
    if (result == aiReturn_SUCCESS) {
        QSSGQmlUtilities::writeQmlPropertyHelper(output,
                                                   tabLevel + 1,
                                                   QSSGQmlUtilities::PropertyMap::Image,
                                                   QStringLiteral("rotationUV"),
                                                   transforms.mRotation);
        QSSGQmlUtilities::writeQmlPropertyHelper(output,
                                                   tabLevel + 1,
                                                   QSSGQmlUtilities::PropertyMap::Image,
                                                   QStringLiteral("positionU"),
                                                   transforms.mTranslation.x);
        QSSGQmlUtilities::writeQmlPropertyHelper(output,
                                                   tabLevel + 1,
                                                   QSSGQmlUtilities::PropertyMap::Image,
                                                   QStringLiteral("positionV"),
                                                   transforms.mTranslation.y);
        QSSGQmlUtilities::writeQmlPropertyHelper(output,
                                                   tabLevel + 1,
                                                   QSSGQmlUtilities::PropertyMap::Image,
                                                   QStringLiteral("scaleU"),
                                                   transforms.mScaling.x);
        QSSGQmlUtilities::writeQmlPropertyHelper(output,
                                                   tabLevel + 1,
                                                   QSSGQmlUtilities::PropertyMap::Image,
                                                   QStringLiteral("scaleV"),
                                                   transforms.mScaling.y);
    }
    // We don't make use of the data here, but there are additional flags
    // available for example the usage of the alpha channel
    // texture flags
    //int textureFlags;
    //material->Get(AI_MATKEY_TEXFLAGS(textureType, index), textureFlags);

    output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("}");

    return outputString;
}

bool AssimpImporter::isModel(aiNode *node)
{
    return node && node->mNumMeshes > 0;
}

bool AssimpImporter::isLight(aiNode *node)
{
    return node && m_lights.contains(node);
}

bool AssimpImporter::isCamera(aiNode *node)
{
    return node && m_cameras.contains(node);
}

QString AssimpImporter::generateUniqueId(const QString &id)
{
    int index = 0;
    QString uniqueID = id;
    while (m_uniqueIds.contains(uniqueID))
        uniqueID = id + QStringLiteral("_") + QString::number(++index);
    m_uniqueIds.insert(uniqueID);
    return uniqueID;
}

// This method is used to walk a subtree to see if any of the nodes actually
// add any state to the scene.  A branch of empty transform nodes would only be
// useful if they were being used somewhere else (like where to aim a camera),
// but the general case is that they can be safely culled
bool AssimpImporter::containsNodesOfConsequence(aiNode *node)
{
    bool isUseful = false;

    isUseful |= isLight(node);
    isUseful |= isModel(node);
    isUseful |= isCamera(node);

    // Return early if we know already
    if (isUseful)
        return true;

    for (uint i = 0; i < node->mNumChildren; ++i)
        isUseful |= containsNodesOfConsequence(node->mChildren[i]);

    return isUseful;
}

QT_END_NAMESPACE
