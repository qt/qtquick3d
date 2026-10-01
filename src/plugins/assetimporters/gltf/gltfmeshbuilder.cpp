// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "gltfmeshbuilder.h"

#include <QtQuick3DGltf/private/qssggltfaccessorreader_p.h>
#include <QtQuick3DGltf/private/qssggltfparser_p.h>

#include <QtGui/qvector2d.h>
#include <QtGui/qvector3d.h>
#include <QtGui/qvector4d.h>

QT_BEGIN_NAMESPACE

using namespace QSSGGltf;

namespace GltfMeshBuilder {

namespace {

struct IntVector4D {
    qint32 x = 0;
    qint32 y = 0;
    qint32 z = 0;
    qint32 w = 0;
};

struct MorphTargetData {
    QVector<QVector3D> positions;
    QVector<QVector3D> normals;
    QVector<QVector3D> tangents;
};

struct PrimitiveData {
    QVector<quint32> indexes; // triangle list, relative to this primitive's vertices
    QVector<QVector3D> positions;
    QVector<QVector3D> normals;
    QVector<QVector3D> tangents;
    QVector<QVector3D> binormals;
    QVector<QVector2D> uv0;
    QVector<QVector2D> uv1;
    QVector<QVector4D> colors;
    QVector<IntVector4D> joints;
    QVector<QVector4D> weights;
    QVector<MorphTargetData> targets;
    QString materialName;
};

struct Requirements {
    bool needsNormals = false;
    bool needsTangents = false;
    bool needsUV0 = false;
    bool needsUV1 = false;
    bool needsColors = false;
    bool needsJoints = false;
    int numMorphTargets = 0;
    bool needsTargetPositions = false;
    bool needsTargetNormals = false;
    bool needsTargetTangents = false;

    void collect(const PrimitiveData &primitive)
    {
        needsNormals |= !primitive.normals.isEmpty();
        needsTangents |= !primitive.tangents.isEmpty();
        needsUV0 |= !primitive.uv0.isEmpty();
        needsUV1 |= !primitive.uv1.isEmpty();
        needsColors |= !primitive.colors.isEmpty();
        needsJoints |= !primitive.joints.isEmpty();
        numMorphTargets = qMax(numMorphTargets, int(primitive.targets.size()));
        for (const MorphTargetData &target : primitive.targets) {
            needsTargetPositions |= !target.positions.isEmpty();
            needsTargetNormals |= !target.normals.isEmpty();
            needsTargetTangents |= !target.tangents.isEmpty();
        }
    }
};

// Drops degenerate strip triangles and an incomplete trailing triangle
QVector<quint32> toTriangleList(int mode, QVector<quint32> indexes)
{
    if (mode == MeshPrimitive::Triangles) {
        if (const qsizetype excess = indexes.size() % 3) {
            qCWarning(lcQuick3DGltf) << "Triangle list has" << indexes.size()
                                     << "indices, which is not a multiple of 3";
            indexes.resize(indexes.size() - excess);
        }
        return indexes;
    }

    QVector<quint32> result;
    if (mode == MeshPrimitive::TriangleStrip) {
        result.reserve(qMax(0, (indexes.size() - 2)) * 3);
        for (qsizetype i = 0; i + 2 < indexes.size(); ++i) {
            quint32 a = indexes[i], b = indexes[i + 1], c = indexes[i + 2];
            if (i % 2)
                std::swap(a, b);
            if (a == b || b == c || a == c)
                continue;
            result << a << b << c;
        }
    } else if (mode == MeshPrimitive::TriangleFan) {
        result.reserve(qMax(0, (indexes.size() - 2)) * 3);
        for (qsizetype i = 1; i + 1 < indexes.size(); ++i)
            result << indexes[0] << indexes[i] << indexes[i + 1];
    }
    return result;
}

template<typename Vec>
QVector<Vec> toVectors(const QList<float> &floats, int components)
{
    QVector<Vec> result;
    const qsizetype count = floats.size() / components;
    result.reserve(count);
    for (qsizetype i = 0; i < count; ++i) {
        Vec v;
        for (int c = 0; c < qMin(components, int(sizeof(Vec) / sizeof(float))); ++c)
            v[c] = floats[i * components + c];
        result.append(v);
    }
    return result;
}

template<typename T>
QVector<T> deindex(const QVector<T> &data, const QVector<quint32> &indexes)
{
    if (data.isEmpty())
        return {};
    QVector<T> result;
    result.reserve(indexes.size());
    for (const quint32 index : indexes)
        result.append(data.value(index));
    return result;
}

// Expands the primitive so every triangle has unique vertices
void deindexPrimitive(PrimitiveData &primitive)
{
    primitive.positions = deindex(primitive.positions, primitive.indexes);
    primitive.normals = deindex(primitive.normals, primitive.indexes);
    primitive.tangents = deindex(primitive.tangents, primitive.indexes);
    primitive.binormals = deindex(primitive.binormals, primitive.indexes);
    primitive.uv0 = deindex(primitive.uv0, primitive.indexes);
    primitive.uv1 = deindex(primitive.uv1, primitive.indexes);
    primitive.colors = deindex(primitive.colors, primitive.indexes);
    primitive.joints = deindex(primitive.joints, primitive.indexes);
    primitive.weights = deindex(primitive.weights, primitive.indexes);
    for (MorphTargetData &target : primitive.targets) {
        target.positions = deindex(target.positions, primitive.indexes);
        target.normals = deindex(target.normals, primitive.indexes);
        target.tangents = deindex(target.tangents, primitive.indexes);
    }
    std::iota(primitive.indexes.begin(), primitive.indexes.end(), 0);
}

void generateNormals(PrimitiveData &primitive, bool smooth)
{
    if (!smooth) {
        // Flat normals need per-face vertices
        deindexPrimitive(primitive);
    }

    primitive.normals.fill(QVector3D(), primitive.positions.size());
    for (qsizetype i = 0; i + 2 < primitive.indexes.size(); i += 3) {
        const quint32 i0 = primitive.indexes[i], i1 = primitive.indexes[i + 1], i2 = primitive.indexes[i + 2];
        const QVector3D faceNormal = QVector3D::crossProduct(primitive.positions[i1] - primitive.positions[i0],
                                                             primitive.positions[i2] - primitive.positions[i0]);
        // Area weighted
        primitive.normals[i0] += faceNormal;
        primitive.normals[i1] += faceNormal;
        primitive.normals[i2] += faceNormal;
    }
    for (QVector3D &normal : primitive.normals)
        normal.normalize();
}

// Per corner, since a UV mirror seam gives a vertex two tangents
void generateTangents(PrimitiveData &primitive)
{
    if (primitive.uv0.isEmpty() || primitive.normals.isEmpty())
        return;
    deindexPrimitive(primitive);

    const qsizetype cornerCount = primitive.indexes.size();
    if (cornerCount == 0)
        return;

    // xyz tangent, w bitangent sign
    QVector<float> generated(cornerCount * 4);
    QSSGMesh::generateTangents(generated.data(), primitive.indexes.constData(), size_t(cornerCount),
                               reinterpret_cast<const float *>(primitive.positions.constData()),
                               size_t(primitive.positions.size()), sizeof(QVector3D),
                               reinterpret_cast<const float *>(primitive.normals.constData()), sizeof(QVector3D),
                               reinterpret_cast<const float *>(primitive.uv0.constData()), sizeof(QVector2D));

    primitive.tangents.resize(cornerCount);
    primitive.binormals.resize(cornerCount);
    for (qsizetype i = 0; i < cornerCount; ++i) {
        const float *t = generated.constData() + i * 4;
        const QVector3D tangent(t[0], t[1], t[2]);
        primitive.tangents[i] = tangent;
        primitive.binormals[i] = QVector3D::crossProduct(primitive.normals.at(i), tangent) * t[3];
    }
}

bool materialNeedsTangents(const QSSGGltfDocument &document, int materialIndex)
{
    if (materialIndex < 0 || materialIndex >= document.materials.size())
        return false;
    const Material &material = document.materials.at(materialIndex);
    // Anisotropy needs a tangent frame that follows the UVs
    return material.normalTexture.isSet()
            || (material.clearcoat && material.clearcoat->clearcoatNormalTexture.isSet())
            || (material.anisotropy && material.anisotropy->anisotropyStrength > 0.0f);
}

// Equivalent of Assimp's joinIdenticalVertices
void weldVertices(PrimitiveData &primitive)
{
    const qsizetype vertexCount = primitive.positions.size();
    if (vertexCount == 0)
        return;

    QVarLengthArray<QSSGMesh::MeshVertexStream, 16> streams;
    const auto addStream = [&](const auto &data) {
        using Element = std::decay_t<decltype(*data.constData())>;
        if (!data.isEmpty())
            streams.append({ data.constData(), sizeof(Element), sizeof(Element) });
    };
    addStream(primitive.positions);
    addStream(primitive.normals);
    addStream(primitive.tangents);
    addStream(primitive.binormals);
    addStream(primitive.uv0);
    addStream(primitive.uv1);
    addStream(primitive.colors);
    addStream(primitive.joints);
    addStream(primitive.weights);
    for (const MorphTargetData &target : std::as_const(primitive.targets)) {
        addStream(target.positions);
        addStream(target.normals);
        addStream(target.tangents);
    }

    // Welding is only an optimization, so skip it instead
    if (size_t(streams.size()) > QSSGMesh::maxVertexStreams) {
        qCWarning(lcQuick3DGltf) << "Not welding vertices for a mesh with" << streams.size()
                                 << "attribute streams; at most" << QSSGMesh::maxVertexStreams
                                 << "are supported";
        return;
    }

    QVector<unsigned int> remap(vertexCount);
    const size_t uniqueCount = QSSGMesh::generateVertexRemap(remap.data(), primitive.indexes.constData(),
                                                             primitive.indexes.size(), vertexCount,
                                                             streams.constData(), streams.size());
    if (qsizetype(uniqueCount) == vertexCount)
        return;

    const auto remapStream = [&](auto &data) {
        using Element = std::decay_t<decltype(*data.constData())>;
        if (data.isEmpty())
            return;
        QVector<Element> welded(uniqueCount);
        QSSGMesh::remapVertexBuffer(welded.data(), data.constData(), vertexCount, sizeof(Element), remap.constData());
        data = std::move(welded);
    };
    remapStream(primitive.positions);
    remapStream(primitive.normals);
    remapStream(primitive.tangents);
    remapStream(primitive.binormals);
    remapStream(primitive.uv0);
    remapStream(primitive.uv1);
    remapStream(primitive.colors);
    remapStream(primitive.joints);
    remapStream(primitive.weights);
    for (MorphTargetData &target : primitive.targets) {
        remapStream(target.positions);
        remapStream(target.normals);
        remapStream(target.tangents);
    }

    QSSGMesh::remapIndexBuffer(primitive.indexes.data(), primitive.indexes.constData(),
                               primitive.indexes.size(), remap.constData());
}

bool loadPrimitive(const QSSGGltfDocument &document, const MeshPrimitive &source,
                   const GltfSceneConverter::Options &options, PrimitiveData &primitive)
{
    const int positionAccessor = source.attributes.value(QByteArrayLiteral("POSITION"), -1);
    if (positionAccessor < 0) {
        qCWarning(lcQuick3DGltf) << "Skipping primitive without POSITION data";
        return false;
    }

    primitive.positions = toVectors<QVector3D>(QSSGGltfAccessorReader::readAsFloats(document, positionAccessor), 3);
    const qsizetype vertexCount = primitive.positions.size();

    // Indices; non-indexed primitives get a sequential index buffer
    QVector<quint32> indexes;
    if (source.indices >= 0) {
        indexes = QVector<quint32>(QSSGGltfAccessorReader::readIndices(document, source.indices));
    } else {
        indexes.resize(vertexCount);
        std::iota(indexes.begin(), indexes.end(), 0);
    }
    primitive.indexes = toTriangleList(source.mode, std::move(indexes));
    if (primitive.indexes.isEmpty()) {
        if (source.mode != MeshPrimitive::Triangles && source.mode != MeshPrimitive::TriangleStrip
            && source.mode != MeshPrimitive::TriangleFan) {
            qCWarning(lcQuick3DGltf) << "Skipping primitive with unsupported mode" << source.mode
                                     << "(points and lines are not supported)";
        }
        return false;
    }

    // Everything below indexes the vertex data directly
    for (const quint32 index : std::as_const(primitive.indexes)) {
        if (qsizetype(index) >= vertexCount) {
            qCWarning(lcQuick3DGltf) << "Skipping primitive with out-of-range index value" << index;
            return false;
        }
    }

    // TANGENT needs the normals, so NORMAL comes first
    static const QByteArray knownSemantics[] = {
        QByteArrayLiteral("POSITION"),   QByteArrayLiteral("NORMAL"),    QByteArrayLiteral("TANGENT"),
        QByteArrayLiteral("TEXCOORD_0"), QByteArrayLiteral("TEXCOORD_1"), QByteArrayLiteral("COLOR_0"),
        QByteArrayLiteral("JOINTS_0"),   QByteArrayLiteral("WEIGHTS_0"),
    };
    for (auto it = source.attributes.constBegin(); it != source.attributes.constEnd(); ++it) {
        if (std::find(std::begin(knownSemantics), std::end(knownSemantics), it.key()) == std::end(knownSemantics)) {
            qCWarning(lcQuick3DGltf) << "Ignoring unsupported vertex attribute" << it.key();
        }
    }

    for (const QByteArray &semantic : knownSemantics) {
        const int accessorIndex = source.attributes.value(semantic, -1);
        if (accessorIndex < 0 || accessorIndex >= document.accessors.size())
            continue;
        const Accessor &accessor = document.accessors.at(accessorIndex);
        const auto floats = [&] { return QSSGGltfAccessorReader::readAsFloats(document, accessorIndex); };

        if (semantic == QByteArrayLiteral("POSITION")) {
            // already read
        } else if (semantic == QByteArrayLiteral("NORMAL")) {
            primitive.normals = toVectors<QVector3D>(floats(), 3);
        } else if (semantic == QByteArrayLiteral("TANGENT")) {
            // The specification requires ignoring tangents without normals
            if (primitive.normals.isEmpty())
                continue;
            // xyz tangent, w handedness
            const QList<float> data = floats();
            const qsizetype count = data.size() / 4;
            primitive.tangents.reserve(count);
            primitive.binormals.reserve(count);
            for (qsizetype i = 0; i < count; ++i) {
                const QVector3D tangent(data[i * 4], data[i * 4 + 1], data[i * 4 + 2]);
                const float handedness = data[i * 4 + 3];
                primitive.tangents.append(tangent);
                const QVector3D normal = primitive.normals.value(i);
                primitive.binormals.append(QVector3D::crossProduct(normal, tangent) * handedness);
            }
        } else if (semantic == QByteArrayLiteral("TEXCOORD_0") || semantic == QByteArrayLiteral("TEXCOORD_1")) {
            // glTF UV origin is top-left, Qt Quick 3D bottom-left
            auto uv = toVectors<QVector2D>(floats(), Accessor::componentCount(accessor.type));
            for (QVector2D &coord : uv)
                coord.setY(1.0f - coord.y());
            if (semantic == QByteArrayLiteral("TEXCOORD_0"))
                primitive.uv0 = std::move(uv);
            else
                primitive.uv1 = std::move(uv);
        } else if (semantic == QByteArrayLiteral("COLOR_0")) {
            const int components = Accessor::componentCount(accessor.type);
            if (components == 3) {
                const QList<float> data = floats();
                const qsizetype count = data.size() / 3;
                primitive.colors.reserve(count);
                for (qsizetype i = 0; i < count; ++i)
                    primitive.colors.append(QVector4D(data[i * 3], data[i * 3 + 1], data[i * 3 + 2], 1.0f));
            } else {
                primitive.colors = toVectors<QVector4D>(floats(), 4);
            }
        } else if (semantic == QByteArrayLiteral("JOINTS_0")) {
            const QList<float> data = floats();
            const qsizetype count = data.size() / 4;
            primitive.joints.reserve(count);
            // Also maps NaN, whose cast would be undefined, to 0
            const auto toJointIndex = [](float value) {
                return value >= 0.0f && value < 2.0e9f ? qint32(value) : 0;
            };
            for (qsizetype i = 0; i < count; ++i) {
                primitive.joints.append({ toJointIndex(data[i * 4]), toJointIndex(data[i * 4 + 1]),
                                          toJointIndex(data[i * 4 + 2]), toJointIndex(data[i * 4 + 3]) });
            }
        } else if (semantic == QByteArrayLiteral("WEIGHTS_0")) {
            auto weights = toVectors<QVector4D>(floats(), 4);
            // Renormalize so the four influences sum to one
            for (QVector4D &weight : weights) {
                const float sum = weight.x() + weight.y() + weight.z() + weight.w();
                if (!qFuzzyIsNull(sum) && !qFuzzyCompare(sum, 1.0f))
                    weight /= sum;
            }
            primitive.weights = std::move(weights);
        }
    }

    // The renderer supports at most 8 morph targets
    constexpr int maxMorphTargets = 8;
    if (source.targets.size() > maxMorphTargets) {
        qCWarning(lcQuick3DGltf) << "Mesh primitive has" << source.targets.size()
                                 << "morph targets, only the first" << maxMorphTargets << "are used";
    }
    const qsizetype targetCount = qMin(qsizetype(maxMorphTargets), source.targets.size());
    for (qsizetype targetIndex = 0; targetIndex < targetCount; ++targetIndex) {
        const QHash<QByteArray, int> &sourceTarget = source.targets.at(targetIndex);
        MorphTargetData target;
        if (const int accessor = sourceTarget.value(QByteArrayLiteral("POSITION"), -1); accessor >= 0)
            target.positions = toVectors<QVector3D>(QSSGGltfAccessorReader::readAsFloats(document, accessor), 3);
        if (const int accessor = sourceTarget.value(QByteArrayLiteral("NORMAL"), -1); accessor >= 0)
            target.normals = toVectors<QVector3D>(QSSGGltfAccessorReader::readAsFloats(document, accessor), 3);
        if (const int accessor = sourceTarget.value(QByteArrayLiteral("TANGENT"), -1); accessor >= 0)
            target.tangents = toVectors<QVector3D>(QSSGGltfAccessorReader::readAsFloats(document, accessor), 3);
        primitive.targets.append(target);
    }

    // Everything below relies on matching attribute counts
    const auto normalizeCount = [vertexCount](auto &data, const char *what) {
        if (!data.isEmpty() && data.size() != vertexCount) {
            qCWarning(lcQuick3DGltf) << what << "data has" << data.size() << "elements, expected" << vertexCount;
            data.resize(vertexCount);
        }
    };
    normalizeCount(primitive.normals, "NORMAL");
    normalizeCount(primitive.tangents, "TANGENT");
    normalizeCount(primitive.binormals, "TANGENT");
    normalizeCount(primitive.uv0, "TEXCOORD_0");
    normalizeCount(primitive.uv1, "TEXCOORD_1");
    normalizeCount(primitive.colors, "COLOR_0");
    normalizeCount(primitive.joints, "JOINTS_0");
    normalizeCount(primitive.weights, "WEIGHTS_0");
    for (MorphTargetData &target : primitive.targets) {
        normalizeCount(target.positions, "morph target POSITION");
        normalizeCount(target.normals, "morph target NORMAL");
        normalizeCount(target.tangents, "morph target TANGENT");
    }

    if (primitive.normals.isEmpty())
        generateNormals(primitive, options.generateSmoothNormals);

    // Switchable variants need tangents if any of their materials does
    const auto anyVariantNeedsTangents = [&] {
        if (!options.materialVariant.isEmpty() || document.materialVariants.isEmpty())
            return false;
        for (const MeshPrimitive::VariantMapping &mapping : source.variantMappings) {
            if (materialNeedsTangents(document, mapping.material))
                return true;
        }
        return false;
    };
    if (primitive.tangents.isEmpty()
        && (options.forceTangentGeneration
            || materialNeedsTangents(document, source.effectiveMaterial(options.materialVariantIndex))
            || anyVariantNeedsTangents())) {
        generateTangents(primitive);
    }

    // glTF targets are deltas, the renderer expects absolute values
    for (MorphTargetData &target : primitive.targets) {
        for (qsizetype i = 0; i < target.positions.size() && i < primitive.positions.size(); ++i)
            target.positions[i] += primitive.positions.at(i);
        for (qsizetype i = 0; i < target.normals.size() && i < primitive.normals.size(); ++i)
            target.normals[i] += primitive.normals.at(i);
        for (qsizetype i = 0; i < target.tangents.size() && i < primitive.tangents.size(); ++i)
            target.tangents[i] += primitive.tangents.at(i);
    }

    if (options.joinIdenticalVertices)
        weldVertices(primitive);

    if (const int materialIndex = source.effectiveMaterial(options.materialVariantIndex); materialIndex >= 0)
        primitive.materialName = document.materials.at(materialIndex).name;

    QSSGMesh::optimizeVertexCache(primitive.indexes.data(), primitive.indexes.data(),
                                  primitive.indexes.size(), primitive.positions.size());
    return true;
}

template<typename T>
void appendVertexData(QByteArray &buffer, const QVector<T> &data, qsizetype vertexCount)
{
    if (!data.isEmpty())
        buffer.append(reinterpret_cast<const char *>(data.constData()), data.size() * sizeof(T));
    // Zero-fill so all attributes cover the same vertex range
    if (data.size() < vertexCount)
        buffer.append((vertexCount - data.size()) * sizeof(T), '\0');
}

} // namespace

/*!
    \internal

    Builds a QSSGMesh::Mesh from the glTF mesh \a mesh in \a document, with
    one subset per triangle primitive. Primitives with non-triangle topology
    that cannot be converted (points and lines) are skipped with a warning.
    Returns an invalid mesh when no primitive could be converted. When
    \a usedPrimitives is given, it receives the indices of the source
    primitives that became subsets, in subset order, so callers can build a
    matching material list. \a morphInfo receives the number of morph targets
    stored in the mesh and which attributes they carry.
*/
QSSGMesh::Mesh buildMesh(const QSSGGltfDocument &document, const QSSGGltf::Mesh &mesh,
                         const GltfSceneConverter::Options &options,
                         QList<int> *usedPrimitives,
                         GltfMorphTargetInfo *morphInfo)
{
    QVector<PrimitiveData> primitives;
    primitives.reserve(mesh.primitives.size());
    Requirements requirements;
    for (qsizetype primitiveIndex = 0; primitiveIndex < mesh.primitives.size(); ++primitiveIndex) {
        PrimitiveData primitive;
        if (loadPrimitive(document, mesh.primitives.at(primitiveIndex), options, primitive)) {
            requirements.collect(primitive);
            primitives.append(std::move(primitive));
            if (usedPrimitives)
                usedPrimitives->append(int(primitiveIndex));
        }
    }

    if (primitives.isEmpty())
        return {};

    QByteArray indexBufferData;
    QByteArray positionData;
    QByteArray normalData;
    QByteArray tangentData;
    QByteArray binormalData;
    QByteArray uv0Data;
    QByteArray uv1Data;
    QByteArray colorData;
    QByteArray jointData;
    QByteArray weightData;
    // vec3 padded to vec4, one buffer per target and component
    struct TargetBuffers {
        QByteArray positions;
        QByteArray normals;
        QByteArray tangents;
    };
    QVector<TargetBuffers> targetData(requirements.numMorphTargets);
    const auto appendPaddedVec3 = [](QByteArray &buffer, const QVector<QVector3D> &data, qsizetype vertexCount) {
        for (const QVector3D &v : data) {
            const float padded[4] = { v.x(), v.y(), v.z(), 0.0f };
            buffer.append(reinterpret_cast<const char *>(padded), sizeof(padded));
        }
        if (data.size() < vertexCount)
            buffer.append((vertexCount - data.size()) * 4 * sizeof(float), '\0');
    };
    QVector<QSSGMesh::AssetMeshSubset> subsets;

    // 32-bit, since Metal needs 4 byte aligned index buffer offsets
    const auto indexType = QSSGMesh::Mesh::ComponentType::UnsignedInt32;

    const auto appendCopy = [](auto &data, qsizetype sourceIndex) {
        if (!data.isEmpty())
            data.append(data.at(sourceIndex));
    };

    quint32 baseVertex = 0;
    for (PrimitiveData &primitive : primitives) {
        quint32 indexOffset = indexBufferData.size() / QSSGMesh::MeshInternal::byteSizeForComponentType(indexType);

        // LOD indexes precede the original ones, lowest quality first
        QVector<QSSGMesh::Mesh::Lod> meshLods;
        QVector<quint32> lodIndexes;
        if (options.generateMeshLODs && !primitive.normals.isEmpty()) {
            QVector<QSSGMesh::MeshVertexSplit> splitVertices;
            const auto lods = QSSGMesh::generateMeshLevelsOfDetail(primitive.positions, primitive.normals,
                                                                   primitive.indexes, splitVertices,
                                                                   options.lodNormalMergeAngle,
                                                                   options.lodNormalSplitAngle);
            // Split vertices are copies with a recalculated normal
            for (const QSSGMesh::MeshVertexSplit &split : std::as_const(splitVertices)) {
                primitive.positions.append(primitive.positions.at(split.sourceIndex));
                primitive.normals.append(split.normal);
                appendCopy(primitive.tangents, split.sourceIndex);
                appendCopy(primitive.binormals, split.sourceIndex);
                appendCopy(primitive.uv0, split.sourceIndex);
                appendCopy(primitive.uv1, split.sourceIndex);
                appendCopy(primitive.colors, split.sourceIndex);
                appendCopy(primitive.joints, split.sourceIndex);
                appendCopy(primitive.weights, split.sourceIndex);
                for (MorphTargetData &target : primitive.targets) {
                    appendCopy(target.positions, split.sourceIndex);
                    appendCopy(target.normals, split.sourceIndex);
                    appendCopy(target.tangents, split.sourceIndex);
                }
            }
            for (const QSSGMesh::MeshLevelOfDetail &lodEntry : lods) {
                QSSGMesh::Mesh::Lod lod;
                lod.offset = indexOffset;
                lod.count = lodEntry.indexes.size();
                lod.distance = lodEntry.distance;
                meshLods.prepend(lod);
                indexOffset += lod.count;
                QVector<quint32> currentLodIndexes = lodEntry.indexes;
                QSSGMesh::optimizeVertexCache(currentLodIndexes.data(), currentLodIndexes.data(),
                                              currentLodIndexes.size(), primitive.positions.size());
                lodIndexes += currentLodIndexes;
            }
        }
        const qsizetype vertexCount = primitive.positions.size();

        QVector<quint32> globalIndexes = lodIndexes + primitive.indexes;
        primitive.indexes.clear();
        for (quint32 &index : globalIndexes)
            index += baseVertex;
        indexBufferData.append(reinterpret_cast<const char *>(globalIndexes.constData()),
                               globalIndexes.size() * sizeof(quint32));
        const quint32 subsetIndexCount = globalIndexes.size() - lodIndexes.size();

        appendVertexData(positionData, primitive.positions, vertexCount);
        if (requirements.needsNormals)
            appendVertexData(normalData, primitive.normals, vertexCount);
        if (requirements.needsTangents) {
            appendVertexData(tangentData, primitive.tangents, vertexCount);
            appendVertexData(binormalData, primitive.binormals, vertexCount);
        }
        if (requirements.needsUV0)
            appendVertexData(uv0Data, primitive.uv0, vertexCount);
        if (requirements.needsUV1)
            appendVertexData(uv1Data, primitive.uv1, vertexCount);
        if (requirements.needsColors)
            appendVertexData(colorData, primitive.colors, vertexCount);
        if (requirements.needsJoints) {
            if (options.useFloatJointIndices) {
                QVector<QVector4D> floatJoints;
                floatJoints.reserve(primitive.joints.size());
                for (const IntVector4D &joint : std::as_const(primitive.joints))
                    floatJoints.append(QVector4D(float(joint.x), float(joint.y), float(joint.z), float(joint.w)));
                appendVertexData(jointData, floatJoints, vertexCount);
            } else {
                appendVertexData(jointData, primitive.joints, vertexCount);
            }
            appendVertexData(weightData, primitive.weights, vertexCount);
        }

        for (int targetIndex = 0; targetIndex < requirements.numMorphTargets; ++targetIndex) {
            const MorphTargetData target = primitive.targets.value(targetIndex);
            if (requirements.needsTargetPositions)
                appendPaddedVec3(targetData[targetIndex].positions, target.positions, vertexCount);
            if (requirements.needsTargetNormals)
                appendPaddedVec3(targetData[targetIndex].normals, target.normals, vertexCount);
            if (requirements.needsTargetTangents)
                appendPaddedVec3(targetData[targetIndex].tangents, target.tangents, vertexCount);
        }

        subsets.append({ primitive.materialName,
                         subsetIndexCount,
                         indexOffset,
                         0, // the builder calculates the bounds from the position data
                         0, 0, // lightmap size hint
                         meshLods });
        baseVertex += vertexCount;
    }

    QVector<QSSGMesh::AssetVertexEntry> entries;
    entries.append({ QSSGMesh::MeshInternal::getPositionAttrName(), positionData,
                     QSSGMesh::Mesh::ComponentType::Float32, 3 });
    if (!normalData.isEmpty()) {
        entries.append({ QSSGMesh::MeshInternal::getNormalAttrName(), normalData,
                         QSSGMesh::Mesh::ComponentType::Float32, 3 });
    }
    if (!uv0Data.isEmpty()) {
        entries.append({ QSSGMesh::MeshInternal::getUV0AttrName(), uv0Data,
                         QSSGMesh::Mesh::ComponentType::Float32, 2 });
    }
    if (!uv1Data.isEmpty()) {
        entries.append({ QSSGMesh::MeshInternal::getUV1AttrName(), uv1Data,
                         QSSGMesh::Mesh::ComponentType::Float32, 2 });
    }
    if (!tangentData.isEmpty()) {
        entries.append({ QSSGMesh::MeshInternal::getTexTanAttrName(), tangentData,
                         QSSGMesh::Mesh::ComponentType::Float32, 3 });
        entries.append({ QSSGMesh::MeshInternal::getTexBinormalAttrName(), binormalData,
                         QSSGMesh::Mesh::ComponentType::Float32, 3 });
    }
    if (!colorData.isEmpty()) {
        entries.append({ QSSGMesh::MeshInternal::getColorAttrName(), colorData,
                         QSSGMesh::Mesh::ComponentType::Float32, 4 });
    }
    if (!jointData.isEmpty()) {
        entries.append({ QSSGMesh::MeshInternal::getJointAttrName(), jointData,
                         options.useFloatJointIndices ? QSSGMesh::Mesh::ComponentType::Float32
                                                      : QSSGMesh::Mesh::ComponentType::Int32, 4 });
        entries.append({ QSSGMesh::MeshInternal::getWeightAttrName(), weightData,
                         QSSGMesh::Mesh::ComponentType::Float32, 4 });
    }

    // Grouped per target, as Mesh::fromAssetData expects
    int numTargetComps = 0;
    if (requirements.numMorphTargets > 0) {
        numTargetComps = int(requirements.needsTargetPositions) + int(requirements.needsTargetNormals)
                + int(requirements.needsTargetTangents);
        for (int targetIndex = 0; targetIndex < requirements.numMorphTargets; ++targetIndex) {
            if (requirements.needsTargetPositions) {
                entries.append({ QSSGMesh::MeshInternal::getPositionAttrName(), targetData[targetIndex].positions,
                                 QSSGMesh::Mesh::ComponentType::Float32, 3, targetIndex });
            }
            if (requirements.needsTargetNormals) {
                entries.append({ QSSGMesh::MeshInternal::getNormalAttrName(), targetData[targetIndex].normals,
                                 QSSGMesh::Mesh::ComponentType::Float32, 3, targetIndex });
            }
            if (requirements.needsTargetTangents) {
                entries.append({ QSSGMesh::MeshInternal::getTexTanAttrName(), targetData[targetIndex].tangents,
                                 QSSGMesh::Mesh::ComponentType::Float32, 3, targetIndex });
            }
        }
    }

    if (morphInfo) {
        morphInfo->count = requirements.numMorphTargets;
        morphInfo->hasPositions = requirements.needsTargetPositions;
        morphInfo->hasNormals = requirements.needsTargetNormals;
        morphInfo->hasTangents = requirements.needsTargetTangents;
    }

    return QSSGMesh::Mesh::fromAssetData(entries, indexBufferData, indexType, subsets,
                                         quint32(requirements.numMorphTargets), quint32(numTargetComps));
}

} // namespace GltfMeshBuilder

QT_END_NAMESPACE
