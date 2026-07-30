// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "gltfsceneconverter.h"
#include "gltfmeshbuilder.h"

#include <QtQuick3DAssetUtils/private/qssgqmlutilities_p.h>
#include <QtQuick3DGltf/private/qssggltfparser_p.h>

#include <QtQuick3D/private/qquick3dmodel_p.h>
#include <QtQuick3D/private/qquick3dmorphtarget_p.h>
#include <QtQuick3D/private/qquick3dnode_p.h>
#include <QtQuick3D/private/qquick3dorthographiccamera_p.h>
#include <QtQuick3D/private/qquick3dperspectivecamera_p.h>
#include <QtQuick3D/private/qquick3dprincipledmaterial_p.h>
#include <QtQuick3D/private/qquick3dskin_p.h>
#include <QtQuick3D/private/qquick3dspecularglossymaterial_p.h>
#include <QtQuick3D/private/qquick3dtexture_p.h>

#include <QtQuick3DGltf/private/qssggltfaccessorreader_p.h>
#include <QtQuick3DGltf/private/qssggltfresourceresolver_p.h>
#include <QtQuick3DUtils/private/qssgutils_p.h>

#if QT_CONFIG(concurrent)
#include <QtConcurrent/qtconcurrentmap.h>
#endif
#include <QtCore/qdir.h>
#include <QtCore/qmath.h>
#include <QtGui/qimagereader.h>

QT_BEGIN_NAMESPACE

using namespace QSSGGltf;

namespace {

bool checkBooleanOption(QLatin1StringView optionName, const QJsonObject &options, bool defaultValue = false)
{
    const auto opt = options.constFind(optionName);
    if (opt == options.constEnd())
        return defaultValue;
    const auto value = opt->toObject().value(QLatin1String("value"));
    return value.toBool(defaultValue);
}

float getRealOption(QLatin1StringView optionName, const QJsonObject &options, float defaultValue)
{
    const auto opt = options.constFind(optionName);
    if (opt == options.constEnd())
        return defaultValue;
    const auto value = opt->toObject().value(QLatin1String("value"));
    return float(value.toDouble(defaultValue));
}

QString getStringOption(QLatin1StringView optionName, const QJsonObject &options)
{
    const auto opt = options.constFind(optionName);
    if (opt == options.constEnd())
        return {};
    return opt->toObject().value(QLatin1String("value")).toString();
}

void decomposeMatrix(const QMatrix4x4 &matrix, QVector3D &translation, QQuaternion &rotation, QVector3D &scale)
{
    translation = matrix.column(3).toVector3D();

    QVector3D columns[3] = { matrix.column(0).toVector3D(),
                             matrix.column(1).toVector3D(),
                             matrix.column(2).toVector3D() };
    scale = QVector3D(columns[0].length(), columns[1].length(), columns[2].length());

    // Fold a mirroring into one scale axis
    if (matrix.determinant() < 0.0f)
        scale.setX(-scale.x());

    for (int i = 0; i < 3; ++i) {
        const float axisScale = i == 0 ? scale.x() : (i == 1 ? scale.y() : scale.z());
        if (!qFuzzyIsNull(axisScale))
            columns[i] /= axisScale;
    }

    const float rotationValues[9] = {
        columns[0].x(), columns[1].x(), columns[2].x(),
        columns[0].y(), columns[1].y(), columns[2].y(),
        columns[0].z(), columns[1].z(), columns[2].z()
    };
    rotation = QQuaternion::fromRotationMatrix(QMatrix3x3(rotationValues)).normalized();
}

// The WebP plugin is in qtimageformats, which is not necessarily deployed
bool isWebpSupported()
{
    static const bool supported = QImageReader::supportedImageFormats().contains(QByteArrayLiteral("webp"));
    return supported;
}

} // namespace

GltfSceneConverter::Options GltfSceneConverter::parseOptions(const QJsonObject &optionsObject)
{
    // Options may arrive wrapped in an "options" object (balsam) or flat
    QJsonObject options = optionsObject;
    if (auto it = options.constFind(QLatin1String("options")); it != options.constEnd())
        options = it->toObject();

    Options result;
    result.designStudioWorkarounds = checkBooleanOption(QLatin1StringView("designStudioWorkarounds"), options);
    result.joinIdenticalVertices = checkBooleanOption(QLatin1StringView("joinIdenticalVertices"), options, true);
    result.useFloatJointIndices = checkBooleanOption(QLatin1StringView("useFloatJointIndices"), options);
    result.generateSmoothNormals = checkBooleanOption(QLatin1StringView("generateSmoothNormals"), options);
    result.forceTangentGeneration = checkBooleanOption(QLatin1StringView("calculateTangentSpace"), options);
    result.generateMipMaps = checkBooleanOption(QLatin1StringView("generateMipMaps"), options);
    result.animationSampleRate = getRealOption(QLatin1StringView("animationSampleRate"), options, 30.0f);
    result.materialVariant = getStringOption(QLatin1StringView("materialVariant"), options);

    result.generateMeshLODs = checkBooleanOption(QLatin1StringView("generateMeshLevelsOfDetail"), options);
    if (result.generateMeshLODs) {
        if (checkBooleanOption(QLatin1StringView("recalculateLodNormals"), options)) {
            const float mergeAngle =
                    getRealOption(QLatin1StringView("recalculateLodNormalsMergeAngle"), options, 60.0f);
            result.lodNormalMergeAngle = qBound(0.0f, mergeAngle, 270.0f);
            const float splitAngle =
                    getRealOption(QLatin1StringView("recalculateLodNormalsSplitAngle"), options, 25.0f);
            result.lodNormalSplitAngle = qBound(0.0f, splitAngle, 270.0f);
        } else {
            result.lodNormalMergeAngle = 0.0f;
            result.lodNormalSplitAngle = 0.0f;
        }
    }

    if (checkBooleanOption(QLatin1StringView("globalScale"), options))
        result.globalScaleValue = getRealOption(QLatin1StringView("globalScaleValue"), options, 1.0f);

    return result;
}

QString GltfSceneConverter::convert(const QSSGGltfDocument &document, const QJsonObject &optionsObject,
                                    const QFileInfo &sourceFile, QSSGSceneDesc::Scene &targetScene)
{
    m_document = &document;
    m_scene = &targetScene;
    m_options = parseOptions(optionsObject);
    if (!m_options.materialVariant.isEmpty()) {
        m_options.materialVariantIndex = int(document.materialVariants.indexOf(m_options.materialVariant));
        if (m_options.materialVariantIndex < 0) {
            // Not an error, since one option may be applied to a batch of assets
            qCWarning(lcQuick3DGltf) << "Asset has no material variant" << m_options.materialVariant
                                     << "- available variants:" << document.materialVariants;
        }
    }
    // Baking one variant leaves nothing to switch between
    if (m_options.materialVariant.isEmpty())
        targetScene.materialVariants = document.materialVariants;
    m_nodeMap.clear();
    m_meshMap.clear();
    m_materialMap.clear();
    m_textureMap.clear();
    m_textureDataMap.clear();
    m_skinMap.clear();
    m_meshUsedPrimitives.clear();
    m_meshMorphInfo.clear();
    m_morphTargetMap.clear();
    m_builtMeshes.clear();

    targetScene.sourceDir = sourceFile.path();
    // For simplicity, and convenience, we'll just use the file path as the id.
    targetScene.id = sourceFile.canonicalFilePath();

    if (!targetScene.root) {
        auto root = new QSSGSceneDesc::Node(QByteArrayLiteral("Root"), QSSGSceneDesc::Node::Type::Transform,
                                            QSSGSceneDesc::Node::RuntimeType::Node);
        QSSGSceneDesc::addNode(targetScene, *root);
    }

    if (!qFuzzyCompare(m_options.globalScaleValue, 1.0f) && !qFuzzyIsNull(m_options.globalScaleValue)) {
        const float gscale = m_options.globalScaleValue;
        QSSGSceneDesc::setProperty(*targetScene.root, "scale", &QQuick3DNode::setScale,
                                   QVector3D { gscale, gscale, gscale });
    }

    int sceneIndex = document.scene;
    if (sceneIndex < 0 && !document.scenes.isEmpty()) {
        qCWarning(lcQuick3DGltf) << "Document has no default scene, using scene 0";
        sceneIndex = 0;
    }
    if (sceneIndex < 0)
        return QStringLiteral("Document contains no scene");

    if (document.extensionsRequired.contains(QLatin1String("EXT_texture_webp")) && !isWebpSupported())
        return QStringLiteral("Asset requires EXT_texture_webp, but no WebP image plugin is available");

    // Mesh building only reads the document, so it runs in parallel
    {
        QList<int> meshIndexes(document.meshes.size());
        std::iota(meshIndexes.begin(), meshIndexes.end(), 0);
        const auto buildOneMesh = [&](int meshIndex) {
            BuiltMesh result;
            result.mesh = GltfMeshBuilder::buildMesh(document, document.meshes.at(meshIndex), m_options,
                                                     &result.usedPrimitives, &result.morphInfo);
            return result;
        };
#if QT_CONFIG(concurrent)
        QList<BuiltMesh> built = QtConcurrent::blockingMapped(meshIndexes, buildOneMesh);
#else
        QList<BuiltMesh> built;
        built.reserve(meshIndexes.size());
        for (const int meshIndex : meshIndexes)
            built.append(buildOneMesh(meshIndex));
#endif
        for (qsizetype meshIndex = 0; meshIndex < built.size(); ++meshIndex)
            m_builtMeshes.insert(int(meshIndex), std::move(built[meshIndex]));
    }

    for (const int nodeIndex : document.scenes.at(sceneIndex).nodes)
        processNode(nodeIndex, *targetScene.root);

    // Skins can only be resolved once all joint nodes exist
    convertSkins();
    convertAnimations();

    return QString();
}

void GltfSceneConverter::convertSkins()
{
    for (auto it = m_skinMap.constBegin(); it != m_skinMap.constEnd(); ++it) {
        const Skin &source = m_document->skins.at(it.key());
        QSSGSceneDesc::Skin *skinNode = it.value();

        QVarLengthArray<QSSGSceneDesc::Node *> joints;
        QList<QMatrix4x4> inverseBindPoses;
        joints.reserve(source.joints.size());

        const QList<float> ibmData = QSSGGltfAccessorReader::readAsFloats(*m_document, source.inverseBindMatrices);
        const bool hasBindMatrices = ibmData.size() >= source.joints.size() * 16;

        for (qsizetype jointIndex = 0; jointIndex < source.joints.size(); ++jointIndex) {
            QSSGSceneDesc::Node *jointNode = m_nodeMap.value(source.joints.at(jointIndex), nullptr);
            if (!jointNode) {
                qCWarning(lcQuick3DGltf) << "Skin" << it.key() << "joint" << jointIndex
                                         << "references a node outside the scene";
                continue;
            }
            joints.push_back(jointNode);
            if (hasBindMatrices) {
                // glTF matrices are column-major; QMatrix4x4(float*) is row-major
                inverseBindPoses.append(QMatrix4x4(ibmData.constData() + jointIndex * 16).transposed());
            } else {
                inverseBindPoses.append(QMatrix4x4());
            }
        }

        QSSGSceneDesc::setProperty(*skinNode, "joints", &QQuick3DSkin::joints, joints);
        QSSGSceneDesc::setProperty(*skinNode, "inverseBindPoses", &QQuick3DSkin::setInverseBindPoses, inverseBindPoses);
    }
}

void GltfSceneConverter::convertAnimations()
{
    using QSSGAnimation = QSSGSceneDesc::Animation;

    const auto makeKey = [](float timeMs, const QVector4D &value, QSSGAnimation::KeyPosition::ValueType valueType) {
        const auto flag = quint16(quint16(QSSGAnimation::KeyPosition::KeyType::Time) | quint16(valueType));
        return new QSSGAnimation::KeyPosition { value, timeMs, flag };
    };

    for (qsizetype animationIndex = 0; animationIndex < m_document->animations.size(); ++animationIndex) {
        const Animation &sourceAnimation = m_document->animations.at(animationIndex);

        QSSGAnimation targetAnimation;
        targetAnimation.name = sourceAnimation.name.isEmpty()
                ? QByteArrayLiteral("animation") + QByteArray::number(animationIndex)
                : sourceAnimation.name.toUtf8();
        // Key times are in milliseconds
        targetAnimation.framesPerSecond = 1000.0f;

        for (const AnimationChannel &channel : sourceAnimation.channels) {
            QSSGSceneDesc::Node *targetNode = m_nodeMap.value(channel.targetNode, nullptr);
            if (!targetNode)
                continue;

            if (channel.path == AnimationChannel::Path::Weights) {
                // Weights are interleaved per key across all targets
                const QList<QSSGSceneDesc::MorphTarget *> morphTargets = m_morphTargetMap.value(channel.targetNode);
                if (morphTargets.isEmpty())
                    continue;
                const Node &sourceNode = m_document->nodes.at(channel.targetNode);
                int sourceTargetCount = 0;
                for (const MeshPrimitive &primitive : m_document->meshes.at(sourceNode.mesh).primitives)
                    sourceTargetCount = qMax(sourceTargetCount, int(primitive.targets.size()));
                if (sourceTargetCount == 0)
                    continue;

                const AnimationSampler &weightSampler = sourceAnimation.samplers.at(channel.sampler);
                // CUBICSPLINE stores in-tangent, value, out-tangent per key
                const bool isCubic = weightSampler.interpolation == AnimationSampler::Interpolation::CubicSpline;
                if (isCubic)
                    qCWarning(lcQuick3DGltf) << "Cubic spline morph weight animation is not supported, using linear";
                const bool isStep = weightSampler.interpolation == AnimationSampler::Interpolation::Step;
                const int valuesPerKey = isCubic ? 3 : 1;
                const QList<float> times = QSSGGltfAccessorReader::readAsFloats(*m_document, weightSampler.input);
                const QList<float> values = QSSGGltfAccessorReader::readAsFloats(*m_document, weightSampler.output);
                const qsizetype keyCount =
                        qMin(qsizetype(times.size()), values.size() / (sourceTargetCount * valuesPerKey));

                for (qsizetype targetIndex = 0; targetIndex < morphTargets.size(); ++targetIndex) {
                    auto *weightChannel = new QSSGAnimation::Channel;
                    weightChannel->target = morphTargets.at(targetIndex);
                    weightChannel->targetProperty = QSSGAnimation::Channel::TargetProperty::Weight;
                    for (qsizetype key = 0; key < keyCount; ++key) {
                        const qsizetype block = key * valuesPerKey + (isCubic ? 1 : 0);
                        const QVector4D weight(values.value(block * sourceTargetCount + targetIndex), 0.0f, 0.0f, 0.0f);
                        weightChannel->keys.push_back(makeKey(times.at(key) * 1000.0f, weight,
                                                              QSSGAnimation::KeyPosition::ValueType::Number));
                        if (isStep && key + 1 < keyCount) {
                            const float holdTime = times.at(key + 1) * 1000.0f - 1.0f;
                            if (holdTime > times.at(key) * 1000.0f) {
                                weightChannel->keys.push_back(makeKey(holdTime, weight,
                                                                      QSSGAnimation::KeyPosition::ValueType::Number));
                            }
                        }
                    }
                    if (weightChannel->keys.isEmpty()) {
                        delete weightChannel;
                        continue;
                    }
                    targetAnimation.length = qMax(targetAnimation.length, weightChannel->keys.last()->time);
                    targetAnimation.channels.push_back(weightChannel);
                }
                continue;
            }

            const AnimationSampler &sampler = sourceAnimation.samplers.at(channel.sampler);
            const QList<float> times = QSSGGltfAccessorReader::readAsFloats(*m_document, sampler.input);
            const QList<float> values = QSSGGltfAccessorReader::readAsFloats(*m_document, sampler.output);
            if (times.isEmpty())
                continue;

            const bool isRotation = channel.path == AnimationChannel::Path::Rotation;
            const int components = isRotation ? 4 : 3;
            const auto valueType = isRotation ? QSSGAnimation::KeyPosition::ValueType::Quaternion
                                              : QSSGAnimation::KeyPosition::ValueType::Vec3;

            // CUBICSPLINE stores in-tangent, value, out-tangent per key
            const int valuesPerKey = sampler.interpolation == AnimationSampler::Interpolation::CubicSpline ? 3 : 1;
            const qsizetype keyCount = qMin(qsizetype(times.size()), values.size() / (components * valuesPerKey));
            if (keyCount == 0)
                continue;

            const auto valueAt = [&](qsizetype key, int part = 0) {
                // part: 0 in-tangent, 1 value, 2 out-tangent
                const qsizetype base = (key * valuesPerKey + (valuesPerKey == 3 ? part : 0)) * components;
                return QVector4D(values.value(base), values.value(base + 1), values.value(base + 2),
                                 components == 4 ? values.value(base + 3) : 0.0f);
            };
            // Keep consecutive keys in one hemisphere for shortest-path interpolation
            QVector4D previousValue;
            const auto fixQuaternionContinuity = [&](QVector4D value) {
                if (isRotation && QVector4D::dotProduct(value, previousValue) < 0.0f)
                    value = -value;
                previousValue = value;
                return value;
            };

            auto *targetChannel = new QSSGAnimation::Channel;
            targetChannel->target = targetNode;
            targetChannel->targetProperty = channel.path == AnimationChannel::Path::Translation
                    ? QSSGAnimation::Channel::TargetProperty::Position
                    : (isRotation ? QSSGAnimation::Channel::TargetProperty::Rotation
                                  : QSSGAnimation::Channel::TargetProperty::Scale);

            switch (sampler.interpolation) {
            case AnimationSampler::Interpolation::Linear:
                for (qsizetype key = 0; key < keyCount; ++key)
                    targetChannel->keys.push_back(makeKey(times.at(key) * 1000.0f,
                                                          fixQuaternionContinuity(valueAt(key, 1)), valueType));
                break;
            case AnimationSampler::Interpolation::Step:
                // Hold each value until just before the next key
                for (qsizetype key = 0; key < keyCount; ++key) {
                    const QVector4D value = fixQuaternionContinuity(valueAt(key, 1));
                    targetChannel->keys.push_back(makeKey(times.at(key) * 1000.0f, value, valueType));
                    if (key + 1 < keyCount) {
                        const float holdTime = times.at(key + 1) * 1000.0f - 1.0f;
                        if (holdTime > times.at(key) * 1000.0f)
                            targetChannel->keys.push_back(makeKey(holdTime, value, valueType));
                    }
                }
                break;
            case AnimationSampler::Interpolation::CubicSpline: {
                // Baked at a fixed rate, with a cap against hostile key times
                constexpr qsizetype maxBakedKeys = 100000;
                const float sampleRate = qMax(1.0f, m_options.animationSampleRate);
                const float startTime = times.first();
                const float endTime = times.last();
                if (!qIsFinite(startTime) || !qIsFinite(endTime) || endTime < startTime) {
                    qCWarning(lcQuick3DGltf) << "Ignoring cubic spline animation channel with invalid key times";
                    delete targetChannel;
                    continue;
                }
                const double idealSamples = std::ceil(double(endTime - startTime) * double(sampleRate));
                const qsizetype sampleCount =
                        qMax(qsizetype(1), qsizetype(qMin(idealSamples, double(maxBakedKeys)))) + 1;
                if (idealSamples > double(maxBakedKeys)) {
                    qCWarning(lcQuick3DGltf) << "Cubic spline animation channel is" << (endTime - startTime)
                                             << "seconds long; baking only" << maxBakedKeys << "keys";
                }
                qsizetype segment = 0;
                for (qsizetype sample = 0; sample < sampleCount; ++sample) {
                    const float t = qMin(startTime + sample / sampleRate, endTime);
                    while (segment + 2 < keyCount && t >= times.at(segment + 1))
                        ++segment;
                    const float t0 = times.at(segment);
                    const float t1 = times.at(qMin(segment + 1, keyCount - 1));
                    const float dt = t1 - t0;
                    QVector4D value;
                    if (dt <= 0.0f || segment + 1 >= keyCount) {
                        value = valueAt(segment, 1);
                    } else {
                        // Cubic Hermite spline as the specification defines it
                        const float u = (t - t0) / dt;
                        const float u2 = u * u, u3 = u2 * u;
                        const QVector4D p0 = valueAt(segment, 1);
                        const QVector4D m0 = valueAt(segment, 2) * dt;
                        const QVector4D p1 = valueAt(segment + 1, 1);
                        const QVector4D m1 = valueAt(segment + 1, 0) * dt;
                        value = (2 * u3 - 3 * u2 + 1) * p0 + (u3 - 2 * u2 + u) * m0
                                + (-2 * u3 + 3 * u2) * p1 + (u3 - u2) * m1;
                    }
                    if (isRotation)
                        value.normalize();
                    targetChannel->keys.push_back(makeKey(t * 1000.0f, fixQuaternionContinuity(value), valueType));
                }
                break;
            }
            }

            if (targetChannel->keys.isEmpty()) {
                delete targetChannel;
                continue;
            }

            targetAnimation.length = qMax(targetAnimation.length, targetChannel->keys.last()->time);
            targetAnimation.channels.push_back(targetChannel);
        }

        if (!targetAnimation.channels.isEmpty())
            m_scene->animations.push_back(new QSSGAnimation(targetAnimation));
    }
}

// Only exposes the protected calculateTableEntryFromQuaternion()
struct InstanceTableEntryBuilder : QQuick3DInstancing
{
    using QQuick3DInstancing::calculateTableEntryFromQuaternion;
};

// Returns null when there are no usable transform attributes
QSSGSceneDesc::Instancing *GltfSceneConverter::convertInstancing(const QSSGGltf::Node &source,
                                                                 QSSGSceneDesc::Node &owner)
{
    const QList<float> translations = QSSGGltfAccessorReader::readAsFloats(*m_document, source.instanceTranslation);
    const QList<float> rotations = QSSGGltfAccessorReader::readAsFloats(*m_document, source.instanceRotation);
    const QList<float> scales = QSSGGltfAccessorReader::readAsFloats(*m_document, source.instanceScale);

    const auto typeMatches = [this](int accessor, QSSGGltf::Accessor::Type expected, const char *what) {
        if (accessor < 0)
            return true;
        const QSSGGltf::Accessor::Type actual = m_document->accessors.at(accessor).type;
        if (actual == expected)
            return true;
        qCWarning(lcQuick3DGltf) << "Ignoring instancing attribute" << what << "with unexpected accessor type";
        return false;
    };
    const bool haveTranslation = typeMatches(source.instanceTranslation, QSSGGltf::Accessor::Type::Vec3,
                                             "TRANSLATION");
    const bool haveRotation = typeMatches(source.instanceRotation, QSSGGltf::Accessor::Type::Vec4, "ROTATION");
    const bool haveScale = typeMatches(source.instanceScale, QSSGGltf::Accessor::Type::Vec3, "SCALE");

    // Counts must match; use the smallest
    qsizetype count = -1;
    const auto considerCount = [&count](qsizetype attributeCount) {
        if (attributeCount >= 0)
            count = count < 0 ? attributeCount : qMin(count, attributeCount);
    };
    if (source.instanceTranslation >= 0 && haveTranslation)
        considerCount(translations.size() / 3);
    if (source.instanceRotation >= 0 && haveRotation)
        considerCount(rotations.size() / 4);
    if (source.instanceScale >= 0 && haveScale)
        considerCount(scales.size() / 3);
    if (count < 0)
        return nullptr;

    QByteArray table;
    table.reserve(count * qsizetype(sizeof(QQuick3DInstancing::InstanceTableEntry)));
    for (qsizetype i = 0; i < count; ++i) {
        QVector3D position;
        QVector3D scale { 1.0f, 1.0f, 1.0f };
        QQuaternion rotation;
        if (source.instanceTranslation >= 0 && haveTranslation)
            position = QVector3D(translations.at(i * 3), translations.at(i * 3 + 1), translations.at(i * 3 + 2));
        if (source.instanceRotation >= 0 && haveRotation) {
            // glTF quaternions are (x, y, z, w)
            rotation = QQuaternion(rotations.at(i * 4 + 3), rotations.at(i * 4), rotations.at(i * 4 + 1),
                                   rotations.at(i * 4 + 2));
        }
        if (source.instanceScale >= 0 && haveScale)
            scale = QVector3D(scales.at(i * 3), scales.at(i * 3 + 1), scales.at(i * 3 + 2));
        const auto entry = InstanceTableEntryBuilder::calculateTableEntryFromQuaternion(position, scale, rotation,
                                                                                        QColor(Qt::white), {});
        table.append(reinterpret_cast<const char *>(&entry), sizeof(entry));
    }

    auto *instancing = new QSSGSceneDesc::Instancing;
    if (!source.name.isEmpty())
        instancing->name = source.name.toUtf8();
    QSSGSceneDesc::addNode(owner, *instancing);
    instancing->instanceData = table;
    instancing->instanceCount = count;
    return instancing;
}

void GltfSceneConverter::processNode(int nodeIndex, QSSGSceneDesc::Node &parent)
{
    const Node &source = m_document->nodes.at(nodeIndex);

    if (int(source.camera >= 0) + int(source.mesh >= 0) + int(source.light >= 0) > 1) {
        qCWarning(lcQuick3DGltf) << "Node" << nodeIndex << "has more than one of a camera, mesh and light;"
                                 << "only its" << (source.camera >= 0 ? "camera" : "mesh") << "is imported";
    }

    QSSGSceneDesc::Node *target = createSceneNode(source);

    // The node's transform goes on a parent, so that the model as its own
    // instance root applies the instance transforms before it
    QSSGSceneDesc::Node *instancedModel = nullptr;
    if (source.hasInstancing) {
        if (target->nodeType == QSSGSceneDesc::Node::Type::Model) {
            instancedModel = target;
            target = new QSSGSceneDesc::Node(QSSGSceneDesc::Node::Type::Transform,
                                             QSSGSceneDesc::Node::RuntimeType::Node);
        } else {
            qCWarning(lcQuick3DGltf) << "Ignoring instancing on node" << nodeIndex << "without a mesh";
        }
    }

    QSSGSceneDesc::addNode(parent, *target);
    m_nodeMap.insert(nodeIndex, target);

    if (instancedModel) {
        QSSGSceneDesc::addNode(*target, *instancedModel);
        setModelProperties(static_cast<QSSGSceneDesc::Model &>(*instancedModel), source, nodeIndex);
        if (QSSGSceneDesc::Instancing *instancing = convertInstancing(source, *instancedModel)) {
            QSSGSceneDesc::setProperty(*instancedModel, "instancing", &QQuick3DModel::setInstancing, instancing);
            QSSGSceneDesc::setProperty(*instancedModel, "instanceRoot", &QQuick3DModel::setInstanceRoot,
                                       instancedModel);
        }
        // The name goes on the parent, which the node's children attach to
        setNodeProperties(*target, source);
        for (const int child : source.children)
            processNode(child, *target);
        return;
    }

    // Properties can only be recorded once the node is part of the scene
    if (source.camera >= 0 && target->nodeType == QSSGSceneDesc::Node::Type::Camera)
        setCameraProperties(static_cast<QSSGSceneDesc::Camera &>(*target), m_document->cameras.at(source.camera));
    else if (target->nodeType == QSSGSceneDesc::Node::Type::Model)
        setModelProperties(static_cast<QSSGSceneDesc::Model &>(*target), source, nodeIndex);
    else if (source.light >= 0 && target->nodeType == QSSGSceneDesc::Node::Type::Light)
        setLightProperties(static_cast<QSSGSceneDesc::Light &>(*target), m_document->lights.at(source.light));

    setNodeProperties(*target, source);

    for (const int child : source.children)
        processNode(child, *target);
}

QSSGSceneDesc::Node *GltfSceneConverter::createSceneNode(const QSSGGltf::Node &node)
{
    if (node.camera >= 0) {
        const Camera &camera = m_document->cameras.at(node.camera);
        const auto runtimeType = camera.type == Camera::Type::Orthographic
                ? QSSGSceneDesc::Node::RuntimeType::OrthographicCamera
                : QSSGSceneDesc::Node::RuntimeType::PerspectiveCamera;
        return new QSSGSceneDesc::Camera(runtimeType);
    }

    if (node.mesh >= 0)
        return new QSSGSceneDesc::Model();

    if (node.light >= 0) {
        const Light &light = m_document->lights.at(node.light);
        auto runtimeType = QSSGSceneDesc::Node::RuntimeType::DirectionalLight;
        if (light.type == Light::Type::Point)
            runtimeType = QSSGSceneDesc::Node::RuntimeType::PointLight;
        else if (light.type == Light::Type::Spot)
            runtimeType = QSSGSceneDesc::Node::RuntimeType::SpotLight;
        return new QSSGSceneDesc::Light(runtimeType);
    }

    return new QSSGSceneDesc::Node(QSSGSceneDesc::Node::Type::Transform, QSSGSceneDesc::Node::RuntimeType::Node);
}

void GltfSceneConverter::setNodeProperties(QSSGSceneDesc::Node &target, const QSSGGltf::Node &source)
{
    if (target.name.isNull()) {
        if (!source.name.isEmpty())
            target.name = source.name.toUtf8();
        else
            target.name = QSSGQmlUtilities::getQmlElementName(target);
    }

    QVector3D translation = source.translation;
    QQuaternion rotation = source.rotation;
    QVector3D scale = source.scale;
    if (source.hasMatrix)
        decomposeMatrix(source.matrix, translation, rotation, scale);

    if (!m_options.designStudioWorkarounds) {
        QSSGSceneDesc::setProperty(target, "position", &QQuick3DNode::setPosition, translation);
    } else {
        QSSGSceneDesc::setProperty(target, "x", &QQuick3DNode::setX, translation.x());
        QSSGSceneDesc::setProperty(target, "y", &QQuick3DNode::setY, translation.y());
        QSSGSceneDesc::setProperty(target, "z", &QQuick3DNode::setZ, translation.z());
    }

    QSSGSceneDesc::setProperty(target, "rotation", &QQuick3DNode::setRotation, rotation);
    QSSGSceneDesc::setProperty(target, "scale", &QQuick3DNode::setScale, scale);

    // KHR_node_visibility
    if (!source.visible)
        QSSGSceneDesc::setProperty(target, "visible", &QQuick3DNode::setVisible, false);
}

void GltfSceneConverter::setModelProperties(QSSGSceneDesc::Model &target, const QSSGGltf::Node &source, int nodeIndex)
{
    const Mesh &mesh = m_document->meshes.at(source.mesh);

    // glTF meshes referenced from several nodes share one mesh resource
    QSSGSceneDesc::Mesh *meshNode = m_meshMap.value(source.mesh, nullptr);
    QList<int> usedPrimitives;
    if (!meshNode) {
        BuiltMesh &builtMesh = m_builtMeshes[source.mesh];
        const QList<int> &builtUsedPrimitives = builtMesh.usedPrimitives;
        const GltfMorphTargetInfo morphInfo = builtMesh.morphInfo;
        usedPrimitives = builtUsedPrimitives;
        if (!builtMesh.mesh.isValid()) {
            qCWarning(lcQuick3DGltf) << "No usable geometry in mesh" << source.mesh;
            return;
        }
        m_scene->meshStorage.push_back(std::move(builtMesh.mesh));
        const auto idx = m_scene->meshStorage.size() - 1;

        QByteArray meshName = mesh.name.toUtf8();
        if (meshName.isEmpty())
            meshName = QByteArrayLiteral("mesh") + QByteArray::number(source.mesh);
        meshNode = new QSSGSceneDesc::Mesh(meshName, idx);
        QSSGSceneDesc::addNode(target, *meshNode);
        m_meshMap.insert(source.mesh, meshNode);
        m_meshUsedPrimitives.insert(source.mesh, usedPrimitives);
        m_meshMorphInfo.insert(source.mesh, morphInfo);
    } else {
        usedPrimitives = m_meshUsedPrimitives.value(source.mesh);
    }

    QSSGSceneDesc::setProperty(target, "source", &QQuick3DModel::setSource, QVariant::fromValue(meshNode));

    if (source.skin >= 0) {
        // Joints are filled in by convertSkins()
        QSSGSceneDesc::Skin *skinNode = m_skinMap.value(source.skin, nullptr);
        if (!skinNode) {
            skinNode = new QSSGSceneDesc::Skin;
            QSSGSceneDesc::addNode(target, *skinNode);
            m_skinMap.insert(source.skin, skinNode);
        }
        QSSGSceneDesc::setProperty(target, "skin", &QQuick3DModel::setSkin, skinNode);
    }

    // Node weights override the mesh defaults
    const GltfMorphTargetInfo morphInfo = m_meshMorphInfo.value(source.mesh);
    if (morphInfo.count > 0) {
        QQuick3DMorphTarget::MorphTargetAttributes attributes;
        if (morphInfo.hasPositions)
            attributes |= QQuick3DMorphTarget::MorphTargetAttribute::Position;
        if (morphInfo.hasNormals)
            attributes |= QQuick3DMorphTarget::MorphTargetAttribute::Normal;
        if (morphInfo.hasTangents)
            attributes |= QQuick3DMorphTarget::MorphTargetAttribute::Tangent;

        const QList<float> &weights = source.weights.isEmpty() ? mesh.weights : source.weights;
        QVarLengthArray<QSSGSceneDesc::MorphTarget *> morphTargets;
        QList<QSSGSceneDesc::MorphTarget *> morphTargetList;
        morphTargets.reserve(morphInfo.count);
        for (int targetIndex = 0; targetIndex < morphInfo.count; ++targetIndex) {
            auto *morphNode = new QSSGSceneDesc::MorphTarget;
            QSSGSceneDesc::addNode(target, *morphNode);
            QSSGSceneDesc::setProperty(*morphNode, "weight", &QQuick3DMorphTarget::setWeight,
                                       weights.value(targetIndex, 0.0f));
            QSSGSceneDesc::setProperty(*morphNode, "attributes", &QQuick3DMorphTarget::setAttributes, attributes);
            morphTargets.push_back(morphNode);
            morphTargetList.append(morphNode);
        }
        QSSGSceneDesc::setProperty(target, "morphTargets", &QQuick3DModel::morphTargets, morphTargets);
        m_morphTargetMap.insert(nodeIndex, morphTargetList);
    }

    // One material per subset, in subset order
    QVarLengthArray<QSSGSceneDesc::Material *> materials;
    materials.reserve(usedPrimitives.size());
    for (const int primitiveIndex : std::as_const(usedPrimitives)) {
        materials.push_back(ensureMaterial(
                mesh.primitives.at(primitiveIndex).effectiveMaterial(m_options.materialVariantIndex), target));
    }

    // Record every variant's materials for models that vary, unless one
    // variant is baked
    const bool modelVaries = std::any_of(usedPrimitives.cbegin(), usedPrimitives.cend(),
                                         [&mesh](int primitiveIndex) {
                                             return !mesh.primitives.at(primitiveIndex).variantMappings.isEmpty();
                                         });
    if (modelVaries && m_options.materialVariant.isEmpty()) {
        for (QSSGSceneDesc::Material *material : std::as_const(materials))
            target.defaultMaterials.append(material);
        for (int variant = 0; variant < m_document->materialVariants.size(); ++variant) {
            QList<QSSGSceneDesc::Node *> variantList;
            variantList.reserve(usedPrimitives.size());
            for (const int primitiveIndex : std::as_const(usedPrimitives))
                variantList.append(ensureMaterial(mesh.primitives.at(primitiveIndex).effectiveMaterial(variant), target));
            target.variantMaterials.append(variantList);
        }
    }

    if (!materials.isEmpty())
        QSSGSceneDesc::setProperty(target, "materials", &QQuick3DModel::materials, materials);
}

QSSGSceneDesc::Material *GltfSceneConverter::ensureMaterial(int materialIndex, QSSGSceneDesc::Node &owner)
{
    if (auto *material = m_materialMap.value(materialIndex, nullptr))
        return material;

    // KHR_materials_pbrSpecularGlossiness
    auto runtimeType = QSSGSceneDesc::Material::RuntimeType::PrincipledMaterial;
    if (materialIndex >= 0 && m_document->materials.at(materialIndex).specularGlossiness)
        runtimeType = QSSGSceneDesc::Material::RuntimeType::SpecularGlossyMaterial;

    auto *material = new QSSGSceneDesc::Material(runtimeType);
    QSSGSceneDesc::addNode(owner, *material);

    if (materialIndex >= 0) {
        setMaterialProperties(*material, m_document->materials.at(materialIndex));
    } else {
        // The specification's default material: white, fully metallic, fully rough
        material->name = QByteArrayLiteral("defaultMaterial");
        QSSGSceneDesc::setProperty(*material, "metalness", &QQuick3DPrincipledMaterial::setMetalness, 1.0f);
        QSSGSceneDesc::setProperty(*material, "roughness", &QQuick3DPrincipledMaterial::setRoughness, 1.0f);
    }

    m_materialMap.insert(materialIndex, material);
    return material;
}

void GltfSceneConverter::setMaterialProperties(QSSGSceneDesc::Material &target, const QSSGGltf::Material &source)
{
    using QSSGSceneDesc::setProperty;

    if (target.name.isNull()) {
        if (!source.name.isEmpty())
            target.name = source.name.toUtf8();
        else
            target.name = QSSGQmlUtilities::getQmlElementName(target);
    }

    if (target.runtimeType == QSSGSceneDesc::Material::RuntimeType::SpecularGlossyMaterial) {
        setSpecularGlossyProperties(target, source);
        return;
    }

    // glTF factors are linear, Qt Quick colors are sRGB
    const QColor baseColor = QSSGUtils::color::linearTosRGB(source.baseColorFactor);
    setProperty(target, "baseColor", &QQuick3DPrincipledMaterial::setBaseColor, baseColor);

    if (auto *baseColorTexture = ensureTexture(source.baseColorTexture, target)) {
        setProperty(target, "baseColorMap", &QQuick3DPrincipledMaterial::setBaseColorMap, baseColorTexture);
        setProperty(target, "opacityChannel", &QQuick3DPrincipledMaterial::setOpacityChannel,
                    QQuick3DPrincipledMaterial::TextureChannelMapping::A);
    }

    setProperty(target, "metalness", &QQuick3DPrincipledMaterial::setMetalness, source.metallicFactor);
    setProperty(target, "roughness", &QQuick3DPrincipledMaterial::setRoughness, source.roughnessFactor);

    if (auto *metallicRoughnessTexture = ensureTexture(source.metallicRoughnessTexture, target)) {
        setProperty(target, "metalnessMap", &QQuick3DPrincipledMaterial::setMetalnessMap, metallicRoughnessTexture);
        setProperty(target, "metalnessChannel", &QQuick3DPrincipledMaterial::setMetalnessChannel,
                    QQuick3DPrincipledMaterial::TextureChannelMapping::B);
        setProperty(target, "roughnessMap", &QQuick3DPrincipledMaterial::setRoughnessMap, metallicRoughnessTexture);
        setProperty(target, "roughnessChannel", &QQuick3DPrincipledMaterial::setRoughnessChannel,
                    QQuick3DPrincipledMaterial::TextureChannelMapping::G);
    }

    if (auto *normalTexture = ensureTexture(source.normalTexture, target)) {
        setProperty(target, "normalMap", &QQuick3DPrincipledMaterial::setNormalMap, normalTexture);
        setProperty(target, "normalStrength", &QQuick3DPrincipledMaterial::setNormalStrength,
                    source.normalTexture.scaleOrStrength);
    }

    if (auto *occlusionTexture = ensureTexture(source.occlusionTexture, target)) {
        setProperty(target, "occlusionMap", &QQuick3DPrincipledMaterial::setOcclusionMap, occlusionTexture);
        setProperty(target, "occlusionChannel", &QQuick3DPrincipledMaterial::setOcclusionChannel,
                    QQuick3DPrincipledMaterial::TextureChannelMapping::R);
        setProperty(target, "occlusionAmount", &QQuick3DPrincipledMaterial::setOcclusionAmount,
                    source.occlusionTexture.scaleOrStrength);
    }

    if (auto *emissiveTexture = ensureTexture(source.emissiveTexture, target))
        setProperty(target, "emissiveMap", &QQuick3DPrincipledMaterial::setEmissiveMap, emissiveTexture);
    // KHR_materials_emissive_strength
    const QVector3D emissiveFactor = source.emissiveFactor * source.emissiveStrength.value_or(1.0f);
    if (!emissiveFactor.isNull())
        setProperty(target, "emissiveFactor", &QQuick3DPrincipledMaterial::setEmissiveFactor, emissiveFactor);

    if (source.doubleSided) {
        setProperty(target, "cullMode", &QQuick3DPrincipledMaterial::setCullMode,
                    QQuick3DPrincipledMaterial::CullMode::NoCulling);
    }

    if (source.alphaMode == QSSGGltf::Material::AlphaMode::Mask) {
        setProperty(target, "alphaMode", &QQuick3DPrincipledMaterial::setAlphaMode,
                    QQuick3DPrincipledMaterial::AlphaMode::Mask);
        setProperty(target, "alphaCutoff", &QQuick3DPrincipledMaterial::setAlphaCutoff, source.alphaCutoff);
        // Masked materials need the opaque prepass depth draw mode to render correctly
        setProperty(target, "depthDrawMode", &QQuick3DPrincipledMaterial::setDepthDrawMode,
                    QQuick3DMaterial::OpaquePrePassDepthDraw);
    } else if (source.alphaMode == QSSGGltf::Material::AlphaMode::Blend) {
        setProperty(target, "alphaMode", &QQuick3DPrincipledMaterial::setAlphaMode,
                    QQuick3DPrincipledMaterial::AlphaMode::Blend);
    } else {
        setProperty(target, "alphaMode", &QQuick3DPrincipledMaterial::setAlphaMode,
                    QQuick3DPrincipledMaterial::AlphaMode::Opaque);
    }

    // KHR_materials_unlit
    if (source.unlit) {
        setProperty(target, "lighting", &QQuick3DPrincipledMaterial::setLighting,
                    QQuick3DPrincipledMaterial::Lighting::NoLighting);
    }

    // KHR_materials_clearcoat
    if (source.clearcoat) {
        const QSSGGltf::Material::Clearcoat &clearcoat = *source.clearcoat;
        setProperty(target, "clearcoatAmount", &QQuick3DPrincipledMaterial::setClearcoatAmount,
                    clearcoat.clearcoatFactor);
        setProperty(target, "clearcoatRoughnessAmount", &QQuick3DPrincipledMaterial::setClearcoatRoughnessAmount,
                    clearcoat.clearcoatRoughnessFactor);
        if (auto *clearcoatTexture = ensureTexture(clearcoat.clearcoatTexture, target))
            setProperty(target, "clearcoatMap", &QQuick3DPrincipledMaterial::setClearcoatMap, clearcoatTexture);
        if (auto *clearcoatRoughnessTexture = ensureTexture(clearcoat.clearcoatRoughnessTexture, target)) {
            setProperty(target, "clearcoatRoughnessMap", &QQuick3DPrincipledMaterial::setClearcoatRoughnessMap,
                        clearcoatRoughnessTexture);
        }
        if (auto *clearcoatNormalTexture = ensureTexture(clearcoat.clearcoatNormalTexture, target)) {
            setProperty(target, "clearcoatNormalMap", &QQuick3DPrincipledMaterial::setClearcoatNormalMap,
                        clearcoatNormalTexture);
            setProperty(target, "clearcoatNormalStrength", &QQuick3DPrincipledMaterial::setClearcoatNormalStrength,
                        clearcoat.clearcoatNormalTexture.scaleOrStrength);
        }
    }

    // KHR_materials_transmission
    if (source.transmission) {
        setProperty(target, "transmissionFactor", &QQuick3DPrincipledMaterial::setTransmissionFactor,
                    source.transmission->transmissionFactor);
        if (auto *transmissionTexture = ensureTexture(source.transmission->transmissionTexture, target))
            setProperty(target, "transmissionMap",
                        &QQuick3DPrincipledMaterial::setTransmissionMap, transmissionTexture);
    }

    // KHR_materials_volume (only meaningful together with transmission)
    if (source.volume) {
        const QSSGGltf::Material::Volume &volume = *source.volume;
        setProperty(target, "thicknessFactor", &QQuick3DPrincipledMaterial::setThicknessFactor, volume.thicknessFactor);
        if (auto *thicknessTexture = ensureTexture(volume.thicknessTexture, target))
            setProperty(target, "thicknessMap", &QQuick3DPrincipledMaterial::setThicknessMap, thicknessTexture);
        if (volume.attenuationDistance > 0.0f) {
            setProperty(target, "attenuationDistance", &QQuick3DPrincipledMaterial::setAttenuationDistance,
                        volume.attenuationDistance);
        }
        setProperty(target, "attenuationColor", &QQuick3DPrincipledMaterial::setAttenuationColor,
                    QColor::fromRgbF(volume.attenuationColor.x(), volume.attenuationColor.y(),
                                     volume.attenuationColor.z()));
    }

    // KHR_materials_ior
    if (source.ior)
        setProperty(target, "indexOfRefraction", &QQuick3DPrincipledMaterial::setIndexOfRefraction, *source.ior);

    // KHR_materials_specular; the color factor and strength alpha are not representable
    if (source.specular) {
        const QSSGGltf::Material::Specular &specular = *source.specular;
        setProperty(target, "specularAmount", &QQuick3DPrincipledMaterial::setSpecularAmount, specular.specularFactor);
        if (auto *specularColorTexture = ensureTexture(specular.specularColorTexture, target))
            setProperty(target, "specularMap", &QQuick3DPrincipledMaterial::setSpecularMap, specularColorTexture);
        if (specular.specularColorFactor != QVector3D(1.0f, 1.0f, 1.0f))
            qCWarning(lcQuick3DGltf) << "KHR_materials_specular specularColorFactor is not supported";
        if (specular.specularTexture.isSet())
            qCWarning(lcQuick3DGltf) << "KHR_materials_specular specularTexture is not supported";
    }

    // KHR_materials_sheen
    if (source.sheen) {
        const QSSGGltf::Material::Sheen &sheen = *source.sheen;
        setProperty(target,
                    "sheenColor",
                    &QQuick3DPrincipledMaterial::setSheenColor,
                    QSSGUtils::color::linearTosRGB(QVector4D(sheen.sheenColorFactor, 1.0f)));
        setProperty(target, "sheenRoughness", &QQuick3DPrincipledMaterial::setSheenRoughness, sheen.sheenRoughnessFactor);
        if (auto *sheenColorTexture = ensureTexture(sheen.sheenColorTexture, target))
            setProperty(target, "sheenColorMap", &QQuick3DPrincipledMaterial::setSheenColorMap, sheenColorTexture);
        // Alpha, the default channel
        if (auto *sheenRoughnessTexture = ensureTexture(sheen.sheenRoughnessTexture, target)) {
            setProperty(target, "sheenRoughnessMap", &QQuick3DPrincipledMaterial::setSheenRoughnessMap, sheenRoughnessTexture);
        }
    }

    // KHR_materials_anisotropy
    if (source.anisotropy) {
        const QSSGGltf::Material::Anisotropy &anisotropy = *source.anisotropy;
        setProperty(target, "anisotropyStrength", &QQuick3DPrincipledMaterial::setAnisotropyStrength, anisotropy.anisotropyStrength);
        // Radians in glTF, degrees in Qt Quick 3D
        setProperty(target,
                    "anisotropyRotation",
                    &QQuick3DPrincipledMaterial::setAnisotropyRotation,
                    qRadiansToDegrees(anisotropy.anisotropyRotation));
        if (auto *anisotropyTexture = ensureTexture(anisotropy.anisotropyTexture, target))
            setProperty(target, "anisotropyMap", &QQuick3DPrincipledMaterial::setAnisotropyMap, anisotropyTexture);
    }

    // KHR_materials_iridescence
    if (source.iridescence) {
        const QSSGGltf::Material::Iridescence &iridescence = *source.iridescence;
        setProperty(target, "iridescenceFactor", &QQuick3DPrincipledMaterial::setIridescenceFactor, iridescence.iridescenceFactor);
        setProperty(target,
                    "iridescenceIndexOfRefraction",
                    &QQuick3DPrincipledMaterial::setIridescenceIndexOfRefraction,
                    iridescence.iridescenceIor);
        setProperty(target,
                    "iridescenceThicknessMinimum",
                    &QQuick3DPrincipledMaterial::setIridescenceThicknessMinimum,
                    iridescence.iridescenceThicknessMinimum);
        setProperty(target,
                    "iridescenceThicknessMaximum",
                    &QQuick3DPrincipledMaterial::setIridescenceThicknessMaximum,
                    iridescence.iridescenceThicknessMaximum);
        // Red and green, the default channels
        if (auto *iridescenceTexture = ensureTexture(iridescence.iridescenceTexture, target))
            setProperty(target, "iridescenceMap", &QQuick3DPrincipledMaterial::setIridescenceMap, iridescenceTexture);
        if (auto *thicknessTexture = ensureTexture(iridescence.iridescenceThicknessTexture, target)) {
            setProperty(target, "iridescenceThicknessMap", &QQuick3DPrincipledMaterial::setIridescenceThicknessMap, thicknessTexture);
        }
    }

    // KHR_materials_dispersion, which requires KHR_materials_volume
    if (source.dispersion)
        setProperty(target, "dispersion", &QQuick3DPrincipledMaterial::setDispersion, *source.dispersion);
}

void GltfSceneConverter::setSpecularGlossyProperties(QSSGSceneDesc::Material &target, const QSSGGltf::Material &source)
{
    using QSSGSceneDesc::setProperty;

    const QSSGGltf::Material::SpecularGlossiness &specularGlossiness = *source.specularGlossiness;

    const QColor albedo = QSSGUtils::color::linearTosRGB(specularGlossiness.diffuseFactor);
    setProperty(target, "albedoColor", &QQuick3DSpecularGlossyMaterial::setAlbedoColor, albedo);

    if (auto *albedoTexture = ensureTexture(specularGlossiness.diffuseTexture, target)) {
        setProperty(target, "albedoMap", &QQuick3DSpecularGlossyMaterial::setAlbedoMap, albedoTexture);
        setProperty(target, "opacityChannel", &QQuick3DSpecularGlossyMaterial::setOpacityChannel,
                    QQuick3DSpecularGlossyMaterial::TextureChannelMapping::A);
    }

    const QColor specularColor = QSSGUtils::color::linearTosRGB(
            QVector4D(specularGlossiness.specularFactor, 1.0f));
    setProperty(target, "specularColor", &QQuick3DSpecularGlossyMaterial::setSpecularColor, specularColor);
    setProperty(target, "glossiness", &QQuick3DSpecularGlossyMaterial::setGlossiness,
                specularGlossiness.glossinessFactor);

    if (auto *specularGlossinessTexture = ensureTexture(specularGlossiness.specularGlossinessTexture, target)) {
        setProperty(target, "specularMap", &QQuick3DSpecularGlossyMaterial::setSpecularMap, specularGlossinessTexture);
        setProperty(target, "glossinessMap",
                    &QQuick3DSpecularGlossyMaterial::setGlossinessMap, specularGlossinessTexture);
        setProperty(target, "glossinessChannel", &QQuick3DSpecularGlossyMaterial::setGlossinessChannel,
                    QQuick3DSpecularGlossyMaterial::TextureChannelMapping::A);
    }

    if (auto *normalTexture = ensureTexture(source.normalTexture, target)) {
        setProperty(target, "normalMap", &QQuick3DSpecularGlossyMaterial::setNormalMap, normalTexture);
        setProperty(target, "normalStrength", &QQuick3DSpecularGlossyMaterial::setNormalStrength,
                    source.normalTexture.scaleOrStrength);
    }

    if (auto *occlusionTexture = ensureTexture(source.occlusionTexture, target)) {
        setProperty(target, "occlusionMap", &QQuick3DSpecularGlossyMaterial::setOcclusionMap, occlusionTexture);
        setProperty(target, "occlusionChannel", &QQuick3DSpecularGlossyMaterial::setOcclusionChannel,
                    QQuick3DSpecularGlossyMaterial::TextureChannelMapping::R);
        setProperty(target, "occlusionAmount", &QQuick3DSpecularGlossyMaterial::setOcclusionAmount,
                    source.occlusionTexture.scaleOrStrength);
    }

    if (auto *emissiveTexture = ensureTexture(source.emissiveTexture, target))
        setProperty(target, "emissiveMap", &QQuick3DSpecularGlossyMaterial::setEmissiveMap, emissiveTexture);
    const QVector3D emissiveFactor = source.emissiveFactor * source.emissiveStrength.value_or(1.0f);
    if (!emissiveFactor.isNull())
        setProperty(target, "emissiveFactor", &QQuick3DSpecularGlossyMaterial::setEmissiveFactor, emissiveFactor);

    if (source.doubleSided) {
        setProperty(target, "cullMode", &QQuick3DSpecularGlossyMaterial::setCullMode,
                    QQuick3DSpecularGlossyMaterial::CullMode::NoCulling);
    }

    if (source.alphaMode == QSSGGltf::Material::AlphaMode::Mask) {
        setProperty(target, "alphaMode", &QQuick3DSpecularGlossyMaterial::setAlphaMode,
                    QQuick3DSpecularGlossyMaterial::AlphaMode::Mask);
        setProperty(target, "alphaCutoff", &QQuick3DSpecularGlossyMaterial::setAlphaCutoff, source.alphaCutoff);
        setProperty(target, "depthDrawMode", &QQuick3DSpecularGlossyMaterial::setDepthDrawMode,
                    QQuick3DMaterial::OpaquePrePassDepthDraw);
    } else if (source.alphaMode == QSSGGltf::Material::AlphaMode::Blend) {
        setProperty(target, "alphaMode", &QQuick3DSpecularGlossyMaterial::setAlphaMode,
                    QQuick3DSpecularGlossyMaterial::AlphaMode::Blend);
    } else {
        setProperty(target, "alphaMode", &QQuick3DSpecularGlossyMaterial::setAlphaMode,
                    QQuick3DSpecularGlossyMaterial::AlphaMode::Opaque);
    }

    if (source.unlit) {
        setProperty(target, "lighting", &QQuick3DSpecularGlossyMaterial::setLighting,
                    QQuick3DSpecularGlossyMaterial::Lighting::NoLighting);
    }
}

QSSGSceneDesc::Texture *GltfSceneConverter::ensureTexture(const QSSGGltf::TextureInfo &textureInfo,
                                                          QSSGSceneDesc::Node &owner)
{
    using QSSGSceneDesc::setProperty;

    if (!textureInfo.isSet() || textureInfo.index >= m_document->textures.size())
        return nullptr;

    const QSSGGltf::Texture &texture = m_document->textures.at(textureInfo.index);
    // EXT_texture_webp, keeping the regular source as the fallback
    const bool webpSupported = isWebpSupported();
    const int sourceImage = (texture.webpSource >= 0 && (webpSupported || texture.source < 0))
            ? texture.webpSource
            : texture.source;
    if (sourceImage >= 0 && sourceImage == texture.webpSource && !webpSupported) {
        qCWarning(lcQuick3DGltf) << "Texture" << textureInfo.index
                                 << "has only a WebP image, but no WebP image plugin is available";
    }
    if (sourceImage < 0 || sourceImage >= m_document->images.size()) {
        qCWarning(lcQuick3DGltf) << "Texture" << textureInfo.index << "has no image source";
        return nullptr;
    }

    const int texCoord = textureInfo.transform && textureInfo.transform->texCoord >= 0
            ? textureInfo.transform->texCoord
            : textureInfo.texCoord;

    // Textures differing in texCoord or transform cannot be shared
    QByteArray key;
    key += QByteArray::number(textureInfo.index);
    key += ':' + QByteArray::number(texCoord);
    if (textureInfo.transform) {
        const QSSGGltf::TextureTransform &transform = *textureInfo.transform;
        key += ':' + QByteArray::number(transform.offset.x()) + ',' + QByteArray::number(transform.offset.y());
        key += ':' + QByteArray::number(transform.scale.x()) + ',' + QByteArray::number(transform.scale.y());
        key += ':' + QByteArray::number(transform.rotation);
    }
    if (auto *cached = m_textureMap.value(key, nullptr))
        return cached;

    const QSSGGltf::Image &image = m_document->images.at(sourceImage);

    // A data: URI would put the whole payload into the name
    QByteArray textureName;
    if (!QSSGGltfResourceResolver::isDataUri(image.uri))
        textureName = image.uri.toUtf8();
    if (textureName.isEmpty())
        textureName = texture.name.isEmpty() ? image.name.toUtf8() : texture.name.toUtf8();

    auto *sceneTexture = new QSSGSceneDesc::Texture(QSSGSceneDesc::Texture::RuntimeType::Image2D, textureName);
    QSSGSceneDesc::addNode(owner, *sceneTexture);
    m_textureMap.insert(key, sceneTexture);

    setProperty(*sceneTexture, "mappingMode", &QQuick3DTexture::setMappingMode, QQuick3DTexture::MappingMode::UV);
    if (texCoord > 0) {
        // Quick3D supports two texture coordinate sets
        setProperty(*sceneTexture, "indexUV", &QQuick3DTexture::setIndexUV, qMin(texCoord, 1));
    }

    QSSGGltf::Sampler sampler;
    if (texture.sampler >= 0 && texture.sampler < m_document->samplers.size())
        sampler = m_document->samplers.at(texture.sampler);

    static const auto asQtTilingMode = [](int mode) {
        switch (mode) {
        case QSSGGltf::Sampler::ClampToEdge:
            return QQuick3DTexture::TilingMode::ClampToEdge;
        case QSSGGltf::Sampler::MirroredRepeat:
            return QQuick3DTexture::TilingMode::MirroredRepeat;
        case QSSGGltf::Sampler::Repeat:
        default:
            return QQuick3DTexture::TilingMode::Repeat;
        }
    };
    setProperty(*sceneTexture, "tilingModeHorizontal",
                &QQuick3DTexture::setHorizontalTiling, asQtTilingMode(sampler.wrapS));
    setProperty(*sceneTexture, "tilingModeVertical",
                &QQuick3DTexture::setVerticalTiling, asQtTilingMode(sampler.wrapT));

    setProperty(*sceneTexture, "magFilter", &QQuick3DTexture::setMagFilter,
                sampler.magFilter == QSSGGltf::Sampler::Nearest ? QQuick3DTexture::Filter::Nearest
                                                      : QQuick3DTexture::Filter::Linear);

    auto minFilter = QQuick3DTexture::Filter::Linear;
    auto mipFilter = m_options.generateMipMaps ? QQuick3DTexture::Filter::Linear : QQuick3DTexture::Filter::None;
    switch (sampler.minFilter) {
    case QSSGGltf::Sampler::Nearest:
        minFilter = QQuick3DTexture::Filter::Nearest;
        break;
    case QSSGGltf::Sampler::NearestMipMapNearest:
        minFilter = QQuick3DTexture::Filter::Nearest;
        mipFilter = QQuick3DTexture::Filter::Nearest;
        break;
    case QSSGGltf::Sampler::LinearMipMapNearest:
        mipFilter = QQuick3DTexture::Filter::Nearest;
        break;
    case QSSGGltf::Sampler::NearestMipMapLinear:
        minFilter = QQuick3DTexture::Filter::Nearest;
        mipFilter = QQuick3DTexture::Filter::Linear;
        break;
    case QSSGGltf::Sampler::LinearMipMapLinear:
        mipFilter = QQuick3DTexture::Filter::Linear;
        break;
    case QSSGGltf::Sampler::Linear:
    default:
        break;
    }
    setProperty(*sceneTexture, "minFilter", &QQuick3DTexture::setMinFilter, minFilter);
    if (mipFilter != QQuick3DTexture::Filter::None) {
        setProperty(*sceneTexture, "generateMipmaps", &QQuick3DTexture::setGenerateMipmaps, true);
        setProperty(*sceneTexture, "mipFilter", &QQuick3DTexture::setMipFilter, mipFilter);
    }

    // KHR_texture_transform, applied around the flipped V origin
    if (textureInfo.transform) {
        const QSSGGltf::TextureTransform &transform = *textureInfo.transform;
        setProperty(*sceneTexture, "pivotV", &QQuick3DTexture::setPivotV, 1.0f);
        setProperty(*sceneTexture, "positionU", &QQuick3DTexture::setPositionU, transform.offset.x());
        setProperty(*sceneTexture, "positionV", &QQuick3DTexture::setPositionV, -transform.offset.y());
        setProperty(*sceneTexture, "rotationUV", &QQuick3DTexture::setRotationUV,
                    float(qRadiansToDegrees(transform.rotation)));
        setProperty(*sceneTexture, "scaleU", &QQuick3DTexture::setScaleU, transform.scale.x());
        setProperty(*sceneTexture, "scaleV", &QQuick3DTexture::setScaleV, transform.scale.y());
    }

    // Image data: external file, or embedded in a buffer view or data: URI
    const bool isEmbedded = image.bufferView >= 0 || QSSGGltfResourceResolver::isDataUri(image.uri);
    if (!isEmbedded) {
        QString relativePath = image.uri;
        relativePath.replace(QLatin1Char('\\'), QLatin1Char('/'));
        const QString resolved = QSSGGltfResourceResolver::resolveFilePath(relativePath, QString());
        if (resolved.isEmpty()) {
            qCWarning(lcQuick3DGltf) << "Ignoring image URI outside the asset directory:" << image.uri;
            return sceneTexture;
        }
        const QString path = QDir(m_document->baseDir).absoluteFilePath(resolved);
        setProperty(*sceneTexture, "source", &QQuick3DTexture::setSource, QUrl { path });
        return sceneTexture;
    }

    QSSGSceneDesc::TextureData *textureData = m_textureDataMap.value(sourceImage, nullptr);
    if (!textureData) {
        QByteArray imageData;
        QString mimeType = image.mimeType;
        if (image.bufferView >= 0) {
            const QSSGGltf::BufferView &view = m_document->bufferViews.at(image.bufferView);
            imageData = m_document->buffers.at(view.buffer).data.mid(view.byteOffset, view.byteLength);
        } else {
            QString error;
            imageData = QSSGGltfResourceResolver::decodeDataUri(image.uri, &error);
            if (mimeType.isEmpty()) {
                const qsizetype header = image.uri.indexOf(QLatin1Char(','));
                // data:<mediatype>[;base64], drop any parameters
                mimeType = image.uri.mid(5, header - 5).section(QLatin1Char(';'), 0, 0);
            }
        }
        if (imageData.isEmpty()) {
            qCWarning(lcQuick3DGltf) << "Failed to load embedded image" << sourceImage;
            return sceneTexture;
        }

        // The format hint doubles as the file suffix
        QByteArray format = mimeType.section(QLatin1Char('/'), 1, 1).trimmed().toLower().toLatin1();
        if (format == QByteArrayLiteral("jpeg"))
            format = QByteArrayLiteral("jpg");
        else if (format.isEmpty())
            format = QByteArrayLiteral("png");
        textureData = new QSSGSceneDesc::TextureData(imageData, QSize(), format,
                                                     quint8(QSSGSceneDesc::TextureData::Flags::Compressed));
        QSSGSceneDesc::addNode(*sceneTexture, *textureData);
        m_textureDataMap.insert(sourceImage, textureData);
    }
    setProperty(*sceneTexture, "textureData", &QQuick3DTexture::setTextureData, textureData);

    return sceneTexture;
}

void GltfSceneConverter::setLightProperties(QSSGSceneDesc::Light &target, const QSSGGltf::Light &source)
{
    using QSSGSceneDesc::setProperty;

    // Brightness is the largest component of color * intensity
    const QVector3D scaledColor = source.color * source.intensity;
    const float brightness = qMax(qMax(1.0f, scaledColor.x()), qMax(scaledColor.y(), scaledColor.z()));
    const QColor color = QColor::fromRgbF(scaledColor.x() / brightness, scaledColor.y() / brightness,
                                          scaledColor.z() / brightness);
    setProperty(target, "color", &QQuick3DAbstractLight::setColor, color);
    setProperty(target, "brightness", &QQuick3DAbstractLight::setBrightness, brightness);

    // Inverse square falloff, scaled for Quick3D units
    if (source.type == Light::Type::Point) {
        setProperty(target, "linearFade", &QQuick3DPointLight::setLinearFade, 0.0f);
        setProperty(target, "quadraticFade", &QQuick3DPointLight::setQuadraticFade, 10000.0f);
    } else if (source.type == Light::Type::Spot) {
        setProperty(target, "linearFade", &QQuick3DSpotLight::setLinearFade, 0.0f);
        setProperty(target, "quadraticFade", &QQuick3DSpotLight::setQuadraticFade, 10000.0f);
        // glTF cone angles are half angles from the center axis
        setProperty(target, "coneAngle", &QQuick3DSpotLight::setConeAngle,
                    float(qRadiansToDegrees(source.outerConeAngle) * 2.0));
        setProperty(target, "innerConeAngle", &QQuick3DSpotLight::setInnerConeAngle,
                    float(qRadiansToDegrees(source.innerConeAngle) * 2.0));
    }
}

void GltfSceneConverter::setCameraProperties(QSSGSceneDesc::Camera &target, const QSSGGltf::Camera &source)
{
    using QSSGSceneDesc::setProperty;

    if (target.runtimeType == QSSGSceneDesc::Node::RuntimeType::PerspectiveCamera) {
        setProperty(target, "clipNear", &QQuick3DPerspectiveCamera::setClipNear, source.znear);
        // zfar 0 means infinite in glTF; pick a generous finite plane
        const float clipFar = source.zfar > 0.0f ? source.zfar : 10000.0f;
        setProperty(target, "clipFar", &QQuick3DPerspectiveCamera::setClipFar, clipFar);
        setProperty(target, "fieldOfView", &QQuick3DPerspectiveCamera::setFieldOfView,
                    float(qRadiansToDegrees(source.yfov)));
    } else {
        setProperty(target, "clipNear", &QQuick3DOrthographicCamera::setClipNear, source.znear);
        // Required by the specification, but not enforced by the parser
        const float clipFar = source.zfar > 0.0f ? source.zfar : 10000.0f;
        setProperty(target, "clipFar", &QQuick3DOrthographicCamera::setClipFar, clipFar);
        // xmag/ymag are half the orthographic view size in scene units
        setProperty(target, "horizontalMagnification", &QQuick3DOrthographicCamera::setHorizontalMagnification,
                    source.xmag * 2.0f);
        setProperty(target, "verticalMagnification", &QQuick3DOrthographicCamera::setVerticalMagnification,
                    source.ymag * 2.0f);
    }
}

QT_END_NAMESPACE
