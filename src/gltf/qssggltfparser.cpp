// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "qssggltfparser_p.h"
#include "qssggltfresourceresolver_p.h"

#include <QtCore/qendian.h>
#include <QtCore/qfile.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qjsonarray.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qset.h>
#include <QtCore/qvarlengtharray.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcQuick3DGltf, "qt.quick3d.gltf")

using namespace QSSGGltf;

namespace {

// GLB container constants
constexpr quint32 GLB_MAGIC = 0x46546C67; // 'glTF'
constexpr quint32 GLB_CHUNK_JSON = 0x4E4F534A; // 'JSON'
constexpr quint32 GLB_CHUNK_BIN = 0x004E4942; // 'BIN\0'

QVector3D toVector3D(const QJsonArray &array, const QVector3D &defaultValue = {})
{
    if (array.size() < 3)
        return defaultValue;
    return QVector3D(float(array.at(0).toDouble()), float(array.at(1).toDouble()), float(array.at(2).toDouble()));
}

QVector4D toVector4D(const QJsonArray &array, const QVector4D &defaultValue = {})
{
    if (array.size() < 4)
        return defaultValue;
    return QVector4D(float(array.at(0).toDouble()), float(array.at(1).toDouble()),
                     float(array.at(2).toDouble()), float(array.at(3).toDouble()));
}

TextureInfo parseTextureInfo(const QJsonObject &object, const char *scaleOrStrengthKey = nullptr)
{
    TextureInfo info;
    if (object.isEmpty())
        return info;
    info.index = object.value(QLatin1String("index")).toInt(-1);
    info.texCoord = object.value(QLatin1String("texCoord")).toInt(0);
    if (scaleOrStrengthKey)
        info.scaleOrStrength = float(object.value(QLatin1String(scaleOrStrengthKey)).toDouble(1.0));

    const QJsonObject extensions = object.value(QLatin1String("extensions")).toObject();
    const QJsonValue transformValue = extensions.value(QLatin1String("KHR_texture_transform"));
    if (transformValue.isObject()) {
        const QJsonObject t = transformValue.toObject();
        TextureTransform transform;
        const QJsonArray offset = t.value(QLatin1String("offset")).toArray();
        if (offset.size() >= 2)
            transform.offset = QVector2D(float(offset.at(0).toDouble()), float(offset.at(1).toDouble()));
        const QJsonArray scale = t.value(QLatin1String("scale")).toArray();
        if (scale.size() >= 2)
            transform.scale = QVector2D(float(scale.at(0).toDouble(1.0)), float(scale.at(1).toDouble(1.0)));
        transform.rotation = float(t.value(QLatin1String("rotation")).toDouble(0.0));
        transform.texCoord = t.value(QLatin1String("texCoord")).toInt(-1);
        info.transform = transform;
    }
    return info;
}

Accessor::Type accessorTypeFromString(const QString &type, bool *ok)
{
    *ok = true;
    if (type == QLatin1String("SCALAR"))
        return Accessor::Type::Scalar;
    if (type == QLatin1String("VEC2"))
        return Accessor::Type::Vec2;
    if (type == QLatin1String("VEC3"))
        return Accessor::Type::Vec3;
    if (type == QLatin1String("VEC4"))
        return Accessor::Type::Vec4;
    if (type == QLatin1String("MAT2"))
        return Accessor::Type::Mat2;
    if (type == QLatin1String("MAT3"))
        return Accessor::Type::Mat3;
    if (type == QLatin1String("MAT4"))
        return Accessor::Type::Mat4;
    *ok = false;
    return Accessor::Type::Scalar;
}

bool isValidComponentType(int value)
{
    switch (Accessor::ComponentType(value)) {
    case Accessor::ComponentType::Byte:
    case Accessor::ComponentType::UnsignedByte:
    case Accessor::ComponentType::Short:
    case Accessor::ComponentType::UnsignedShort:
    case Accessor::ComponentType::UnsignedInt:
    case Accessor::ComponentType::Float:
        return true;
    }
    return false;
}

} // namespace

/*!
    \class QSSGGltfParser
    \internal

    Parses glTF 2.0 content (.gltf JSON or binary .glb container) into a
    QSSGGltfDocument. Buffers (external files, data: URIs, and the GLB BIN
    chunk) are resolved eagerly; image contents are left unresolved.

    Validation is two-tier: structural violations (out-of-range indices,
    accessors extending past their buffer view, truncated GLB, unsupported
    required extensions) fail the parse with an error message; tolerable
    deviations from the specification are logged as warnings and clamped,
    repaired, or ignored, matching the leniency of other glTF consumers. A
    node with more than one parent and a scene root that is not unique both
    fall in the second group: the offending edge is dropped, because the node
    hierarchy has to be a forest for consumers to recurse over it safely, but
    the rest of the asset is still worth loading.
*/

/*!
    \internal

    Returns the extensions the parser understands. Assets that list anything
    else in extensionsRequired fail to parse.

    Being on this list means the extension is represented in QSSGGltfDocument,
    not that any particular consumer acts on it: a consumer that ignores one
    of these still gets a document it can load.
*/
QStringList QSSGGltfParser::supportedExtensions()
{
    // Order does not matter; keep alphabetical for readability. Extensions
    // needing a decoder we do not ship (KHR_draco_mesh_compression,
    // KHR_texture_basisu) are intentionally absent and get a clear error
    // when required by an asset.
    return {
        QStringLiteral("EXT_mesh_gpu_instancing"),
        QStringLiteral("KHR_lights_punctual"),
        QStringLiteral("KHR_materials_clearcoat"),
        QStringLiteral("KHR_materials_emissive_strength"),
        QStringLiteral("KHR_materials_ior"),
        QStringLiteral("KHR_materials_pbrSpecularGlossiness"),
        QStringLiteral("KHR_materials_specular"),
        QStringLiteral("KHR_materials_transmission"),
        QStringLiteral("KHR_materials_unlit"),
        QStringLiteral("KHR_materials_variants"),
        QStringLiteral("KHR_materials_volume"),
        QStringLiteral("KHR_mesh_quantization"),
        QStringLiteral("KHR_texture_transform"),
    };
}

bool QSSGGltfParser::setError(const QString &message)
{
    m_errorMessage = message;
    qCWarning(lcQuick3DGltf) << message;
    return false;
}

/*!
    \internal

    Convenience overload of parse() reading \a filePath, a local file or qrc
    path, into \a document.
*/
bool QSSGGltfParser::parseFile(const QString &filePath, QSSGGltfDocument *document)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return setError(QStringLiteral("Failed to open '%1': %2").arg(filePath, file.errorString()));
    return parse(file.readAll(), QFileInfo(filePath).path(), document);
}

/*!
    \internal

    Parses .gltf JSON or a .glb container in \a data into \a document.
    \a baseDir is the local or qrc directory used to resolve relative
    buffer and image URIs.
*/
bool QSSGGltfParser::parse(const QByteArray &data, const QString &baseDir, QSSGGltfDocument *document)
{
    Q_ASSERT(document);
    m_errorMessage.clear();
    *document = QSSGGltfDocument();
    document->baseDir = baseDir;

    QByteArray json = data;
    QByteArray binChunk;

    // Binary container? (12-byte header: magic, version, length)
    if (data.size() >= 12 && qFromLittleEndian<quint32>(data.constData()) == GLB_MAGIC) {
        const quint32 version = qFromLittleEndian<quint32>(data.constData() + 4);
        if (version != 2)
            return setError(QStringLiteral("Unsupported GLB container version %1").arg(version));
        const quint32 length = qFromLittleEndian<quint32>(data.constData() + 8);
        if (qint64(length) > data.size())
            return setError(QStringLiteral("Truncated GLB file: header declares %1 bytes, got %2")
                            .arg(length).arg(data.size()));

        json.clear();
        qint64 offset = 12;
        while (offset + 8 <= qint64(length)) {
            const quint32 chunkLength = qFromLittleEndian<quint32>(data.constData() + offset);
            const quint32 chunkType = qFromLittleEndian<quint32>(data.constData() + offset + 4);
            offset += 8;
            if (offset + qint64(chunkLength) > qint64(length))
                return setError(QStringLiteral("Truncated GLB chunk at offset %1").arg(offset - 8));

            if (chunkType == GLB_CHUNK_JSON && json.isEmpty())
                json = data.mid(offset, chunkLength);
            else if (chunkType == GLB_CHUNK_BIN && binChunk.isEmpty())
                binChunk = data.mid(offset, chunkLength);
            else
                qCWarning(lcQuick3DGltf) << "Skipping unknown GLB chunk type" << Qt::hex << chunkType;

            // Chunks are 4-byte aligned
            offset += chunkLength;
            if (offset % 4)
                offset += 4 - (offset % 4);
        }
        if (json.isEmpty())
            return setError(QStringLiteral("GLB container has no JSON chunk"));
    }

    QJsonParseError jsonError;
    const QJsonDocument jsonDocument = QJsonDocument::fromJson(json, &jsonError);
    if (jsonDocument.isNull())
        return setError(QStringLiteral("Invalid glTF JSON: %1").arg(jsonError.errorString()));
    if (!jsonDocument.isObject())
        return setError(QStringLiteral("Invalid glTF JSON: root is not an object"));

    const QJsonObject root = jsonDocument.object();

    // asset (required)
    {
        const QJsonValue assetValue = root.value(QLatin1String("asset"));
        if (!assetValue.isObject())
            return setError(QStringLiteral("Not a glTF document: no asset object"));
        const QJsonObject asset = assetValue.toObject();
        document->asset.version = asset.value(QLatin1String("version")).toString();
        document->asset.minVersion = asset.value(QLatin1String("minVersion")).toString();
        document->asset.generator = asset.value(QLatin1String("generator")).toString();
        document->asset.copyright = asset.value(QLatin1String("copyright")).toString();

        const int major = document->asset.version.section(QLatin1Char('.'), 0, 0).toInt();
        if (major != 2)
            return setError(QStringLiteral("Unsupported glTF version '%1'").arg(document->asset.version));

        // The specification requires a client to fail when minVersion asks for
        // more than it implements. Only 2.0 exists so far, so this is about not
        // silently loading a future asset that says it needs more.
        if (!document->asset.minVersion.isEmpty()) {
            const int minMajor = document->asset.minVersion.section(QLatin1Char('.'), 0, 0).toInt();
            const int minMinor = document->asset.minVersion.section(QLatin1Char('.'), 1, 1).toInt();
            if (minMajor != 2 || minMinor > 0)
                return setError(QStringLiteral("Asset requires glTF version '%1' or higher")
                                        .arg(document->asset.minVersion));
        }
    }

    // extensionsUsed / extensionsRequired
    for (const auto &value : root.value(QLatin1String("extensionsUsed")).toArray())
        document->extensionsUsed.append(value.toString());
    for (const auto &value : root.value(QLatin1String("extensionsRequired")).toArray())
        document->extensionsRequired.append(value.toString());
    document->rootExtensions = root.value(QLatin1String("extensions")).toObject();

    const QStringList supported = supportedExtensions();
    for (const QString &required : std::as_const(document->extensionsRequired)) {
        if (!supported.contains(required))
            return setError(QStringLiteral("Asset requires unsupported glTF extension '%1'").arg(required));
    }
    for (const QString &used : std::as_const(document->extensionsUsed)) {
        if (!supported.contains(used))
            qCWarning(lcQuick3DGltf) << "Ignoring unsupported glTF extension" << used;
    }

    // buffers
    for (const auto &value : root.value(QLatin1String("buffers")).toArray()) {
        const QJsonObject object = value.toObject();
        Buffer buffer;
        buffer.uri = object.value(QLatin1String("uri")).toString();
        buffer.byteLength = qint64(object.value(QLatin1String("byteLength")).toDouble());
        if (buffer.byteLength < 0)
            return setError(QStringLiteral("Buffer %1 has negative byte length").arg(document->buffers.size()));
        if (buffer.uri.isEmpty()) {
            // GLB BIN chunk; only valid for the first buffer
            if (document->buffers.isEmpty() && !binChunk.isEmpty())
                buffer.data = binChunk;
            else if (binChunk.isEmpty())
                return setError(
                        QStringLiteral("Buffer %1 has no URI and there is no GLB BIN chunk")
                                .arg(document->buffers.size()));
            else
                return setError(QStringLiteral("Only the first buffer may refer to the GLB BIN chunk"));
        } else {
            QString resolveError;
            buffer.data = QSSGGltfResourceResolver::loadUri(buffer.uri, baseDir, &resolveError);
            // Loading can legitimately produce nothing, for an empty file or an
            // empty data URI payload, so the error string is what says whether
            // it failed. The length check below rejects a short buffer anyway.
            if (!resolveError.isEmpty())
                return setError(QStringLiteral("Failed to load buffer %1: %2")
                                .arg(document->buffers.size()).arg(resolveError));
        }
        if (buffer.data.size() < buffer.byteLength)
            return setError(QStringLiteral("Buffer %1 is %2 bytes, expected at least %3")
                                    .arg(document->buffers.size()).arg(buffer.data.size()).arg(buffer.byteLength));
        document->buffers.append(buffer);
    }

    // bufferViews
    for (const auto &value : root.value(QLatin1String("bufferViews")).toArray()) {
        const QJsonObject object = value.toObject();
        BufferView view;
        view.buffer = object.value(QLatin1String("buffer")).toInt(-1);
        view.byteOffset = qint64(object.value(QLatin1String("byteOffset")).toDouble(0));
        view.byteLength = qint64(object.value(QLatin1String("byteLength")).toDouble());
        view.byteStride = object.value(QLatin1String("byteStride")).toInt(0);
        view.target = object.value(QLatin1String("target")).toInt(0);
        view.name = object.value(QLatin1String("name")).toString();

        if (view.buffer < 0 || view.buffer >= document->buffers.size())
            return setError(
                    QStringLiteral("Buffer view %1 references invalid buffer %2")
                            .arg(document->bufferViews.size()).arg(view.buffer));
        // The stride bound is from the specification; together with
        // non-negative offsets and lengths it also keeps all later offset
        // arithmetic far away from overflowing 64 bits.
        if (view.byteOffset < 0 || view.byteLength < 0 || view.byteStride < 0 || view.byteStride > 252)
            return setError(QStringLiteral("Buffer view %1 has an invalid byte offset, length, or stride")
                                    .arg(document->bufferViews.size()));
        // Compared without adding the two together: both are individually
        // positive by the check above, but their sum can overflow to a negative
        // value and slip under the buffer length.
        const qint64 bufferLength = document->buffers.at(view.buffer).byteLength;
        if (view.byteLength > bufferLength || view.byteOffset > bufferLength - view.byteLength)
            return setError(
                    QStringLiteral("Buffer view %1 extends past the end of buffer %2")
                            .arg(document->bufferViews.size()).arg(view.buffer));
        document->bufferViews.append(view);
    }

    // accessors
    for (const auto &value : root.value(QLatin1String("accessors")).toArray()) {
        const QJsonObject object = value.toObject();
        Accessor accessor;
        accessor.bufferView = object.value(QLatin1String("bufferView")).toInt(-1);
        accessor.byteOffset = qint64(object.value(QLatin1String("byteOffset")).toDouble(0));
        const int componentType = object.value(QLatin1String("componentType")).toInt();
        if (!isValidComponentType(componentType))
            return setError(QStringLiteral("Accessor %1 has invalid component type %2")
                            .arg(document->accessors.size()).arg(componentType));
        accessor.componentType = Accessor::ComponentType(componentType);
        bool typeOk = false;
        accessor.type = accessorTypeFromString(object.value(QLatin1String("type")).toString(), &typeOk);
        if (!typeOk)
            return setError(QStringLiteral("Accessor %1 has invalid type '%2'")
                                    .arg(document->accessors.size())
                                    .arg(object.value(QLatin1String("type")).toString()));
        // The specification puts no upper bound on count, but the offset
        // arithmetic below needs one. A count near the top of the qint64 range
        // makes those products overflow, which is undefined behavior, and in
        // practice wraps the range test at the end of this loop into an
        // acceptance: a ten-line document then yields an accessor claiming
        // more elements than could ever be allocated. One element occupies at
        // least one byte, so no real asset comes anywhere near this cap, and
        // capping in elements keeps every product here well inside 64 bits.
        constexpr qint64 maxAccessorElements = qint64(256) * 1024 * 1024;
        const double rawCount = object.value(QLatin1String("count")).toDouble();
        if (!(rawCount >= 0.0 && rawCount <= double(maxAccessorElements)))
            return setError(QStringLiteral("Accessor %1 has a negative or unreasonably large count")
                                    .arg(document->accessors.size()));
        accessor.count = qint64(rawCount);
        if (accessor.byteOffset < 0)
            return setError(QStringLiteral("Accessor %1 has a negative byte offset")
                                    .arg(document->accessors.size()));
        accessor.normalized = object.value(QLatin1String("normalized")).toBool(false);
        accessor.name = object.value(QLatin1String("name")).toString();
        for (const auto &m : object.value(QLatin1String("min")).toArray())
            accessor.min.append(m.toDouble());
        for (const auto &m : object.value(QLatin1String("max")).toArray())
            accessor.max.append(m.toDouble());

        const QJsonValue sparseValue = object.value(QLatin1String("sparse"));
        if (sparseValue.isObject()) {
            const QJsonObject sparseObject = sparseValue.toObject();
            Accessor::Sparse sparse;
            // Bounded the same way as accessor.count, and for the same reason:
            // it is multiplied by a component size a few lines down
            const double rawSparseCount = sparseObject.value(QLatin1String("count")).toDouble();
            if (!(rawSparseCount >= 0.0 && rawSparseCount <= double(maxAccessorElements)))
                return setError(QStringLiteral("Accessor %1 has a negative or unreasonably large sparse count")
                                        .arg(document->accessors.size()));
            sparse.count = qint64(rawSparseCount);
            const QJsonObject indices = sparseObject.value(QLatin1String("indices")).toObject();
            sparse.indicesBufferView = indices.value(QLatin1String("bufferView")).toInt(-1);
            sparse.indicesByteOffset = qint64(indices.value(QLatin1String("byteOffset")).toDouble(0));
            const int indicesComponentType = indices.value(QLatin1String("componentType")).toInt();
            if (!isValidComponentType(indicesComponentType))
                return setError(
                        QStringLiteral("Accessor %1 sparse indices have invalid component type")
                                .arg(document->accessors.size()));
            sparse.indicesComponentType = Accessor::ComponentType(indicesComponentType);
            const QJsonObject values = sparseObject.value(QLatin1String("values")).toObject();
            sparse.valuesBufferView = values.value(QLatin1String("bufferView")).toInt(-1);
            sparse.valuesByteOffset = qint64(values.value(QLatin1String("byteOffset")).toDouble(0));

            if (sparse.indicesBufferView < 0 || sparse.indicesBufferView >= document->bufferViews.size()
                || sparse.valuesBufferView < 0 || sparse.valuesBufferView >= document->bufferViews.size()) {
                return setError(QStringLiteral("Accessor %1 sparse data references an invalid buffer view")
                                        .arg(document->accessors.size()));
            }
            if (sparse.count > accessor.count
                || sparse.indicesByteOffset < 0 || sparse.valuesByteOffset < 0) {
                return setError(QStringLiteral("Accessor %1 has an invalid sparse count or byte offset")
                                        .arg(document->accessors.size()));
            }
            // Offsets kept out of the sums, like the buffer view check above
            const BufferView &indicesView = document->bufferViews.at(sparse.indicesBufferView);
            const BufferView &valuesView = document->bufferViews.at(sparse.valuesBufferView);
            const qint64 indicesSpan =
                    sparse.count * Accessor::componentByteSize(sparse.indicesComponentType);
            const qint64 valuesSpan = sparse.count * accessor.elementByteSize();
            if (indicesSpan > indicesView.byteLength
                || sparse.indicesByteOffset > indicesView.byteLength - indicesSpan
                || valuesSpan > valuesView.byteLength
                || sparse.valuesByteOffset > valuesView.byteLength - valuesSpan) {
                return setError(
                        QStringLiteral("Accessor %1 sparse data extends past the end of its buffer view")
                                .arg(document->accessors.size()));
            }
            accessor.sparse = sparse;
        }

        if (accessor.bufferView >= document->bufferViews.size())
            return setError(
                    QStringLiteral("Accessor %1 references invalid buffer view %2")
                            .arg(document->accessors.size()).arg(accessor.bufferView));
        if (accessor.bufferView >= 0) {
            const BufferView &view = document->bufferViews.at(accessor.bufferView);
            const qint64 elementSize = accessor.elementByteSize();
            const qint64 stride = view.byteStride > 0 ? view.byteStride : elementSize;
            // Offset kept out of the sum, like the buffer view check above
            const qint64 span = (accessor.count - 1) * stride + elementSize;
            if (accessor.count > 0
                && (span > view.byteLength || accessor.byteOffset > view.byteLength - span))
                return setError(QStringLiteral("Accessor %1 extends past the end of buffer view %2")
                                        .arg(document->accessors.size())
                                        .arg(accessor.bufferView));
        } else {
            // An accessor without a buffer view is read as all zeros, so no
            // buffer bounds its size. The element cap above keeps this product
            // from overflowing, but 256M elements of MAT4 would still be a
            // 16 GB allocation, so bound the byte count as well.
            constexpr qint64 maxSyntheticAccessorBytes = qint64(256) * 1024 * 1024;
            if (accessor.count * accessor.elementByteSize() > maxSyntheticAccessorBytes)
                return setError(QStringLiteral("Accessor %1 has no buffer view and an unreasonably large count")
                                        .arg(document->accessors.size()));
        }
        document->accessors.append(accessor);
    }

    // images
    for (const auto &value : root.value(QLatin1String("images")).toArray()) {
        const QJsonObject object = value.toObject();
        Image image;
        image.uri = object.value(QLatin1String("uri")).toString();
        image.bufferView = object.value(QLatin1String("bufferView")).toInt(-1);
        image.mimeType = object.value(QLatin1String("mimeType")).toString();
        image.name = object.value(QLatin1String("name")).toString();
        if (image.bufferView >= document->bufferViews.size())
            return setError(QStringLiteral("Image %1 references invalid buffer view %2")
                            .arg(document->images.size()).arg(image.bufferView));
        document->images.append(image);
    }

    // samplers
    for (const auto &value : root.value(QLatin1String("samplers")).toArray()) {
        const QJsonObject object = value.toObject();
        Sampler sampler;
        sampler.magFilter = object.value(QLatin1String("magFilter")).toInt(0);
        sampler.minFilter = object.value(QLatin1String("minFilter")).toInt(0);
        sampler.wrapS = object.value(QLatin1String("wrapS")).toInt(Sampler::Repeat);
        sampler.wrapT = object.value(QLatin1String("wrapT")).toInt(Sampler::Repeat);
        sampler.name = object.value(QLatin1String("name")).toString();
        document->samplers.append(sampler);
    }

    // textures
    for (const auto &value : root.value(QLatin1String("textures")).toArray()) {
        const QJsonObject object = value.toObject();
        Texture texture;
        texture.sampler = object.value(QLatin1String("sampler")).toInt(-1);
        texture.source = object.value(QLatin1String("source")).toInt(-1);
        texture.name = object.value(QLatin1String("name")).toString();
        texture.extensions = object.value(QLatin1String("extensions")).toObject();
        if (texture.sampler >= document->samplers.size())
            return setError(QStringLiteral("Texture %1 references invalid sampler %2")
                            .arg(document->textures.size()).arg(texture.sampler));
        if (texture.source >= document->images.size())
            return setError(QStringLiteral("Texture %1 references invalid image %2")
                            .arg(document->textures.size()).arg(texture.source));
        document->textures.append(texture);
    }

    // materials
    //
    // Every texture reference goes through parseTexture() rather than
    // parseTextureInfo() directly, so that the index is range checked in one
    // place and a newly supported extension cannot forget to do it. The
    // textures array is already parsed at this point.
    int badTextureIndex = -1;
    const auto parseTexture = [&](const QJsonObject &object,
                                  const char *scaleOrStrengthKey = nullptr) {
        const TextureInfo info = parseTextureInfo(object, scaleOrStrengthKey);
        if (info.index >= document->textures.size())
            badTextureIndex = info.index;
        return info;
    };

    for (const auto &value : root.value(QLatin1String("materials")).toArray()) {
        const QJsonObject object = value.toObject();
        Material material;
        material.name = object.value(QLatin1String("name")).toString();

        const QJsonValue pbrValue = object.value(QLatin1String("pbrMetallicRoughness"));
        if (pbrValue.isObject()) {
            const QJsonObject pbr = pbrValue.toObject();
            material.baseColorFactor =
                    toVector4D(pbr.value(QLatin1String("baseColorFactor")).toArray(), material.baseColorFactor);
            material.baseColorTexture = parseTexture(pbr.value(QLatin1String("baseColorTexture")).toObject());
            material.metallicFactor = float(pbr.value(QLatin1String("metallicFactor")).toDouble(1.0));
            material.roughnessFactor = float(pbr.value(QLatin1String("roughnessFactor")).toDouble(1.0));
            material.metallicRoughnessTexture =
                    parseTexture(pbr.value(QLatin1String("metallicRoughnessTexture")).toObject());
        }

        material.normalTexture = parseTexture(object.value(QLatin1String("normalTexture")).toObject(), "scale");
        material.occlusionTexture =
                parseTexture(object.value(QLatin1String("occlusionTexture")).toObject(), "strength");
        material.emissiveTexture = parseTexture(object.value(QLatin1String("emissiveTexture")).toObject());
        material.emissiveFactor =
                toVector3D(object.value(QLatin1String("emissiveFactor")).toArray(), material.emissiveFactor);

        const QString alphaMode = object.value(QLatin1String("alphaMode")).toString();
        if (alphaMode == QLatin1String("MASK"))
            material.alphaMode = Material::AlphaMode::Mask;
        else if (alphaMode == QLatin1String("BLEND"))
            material.alphaMode = Material::AlphaMode::Blend;
        material.alphaCutoff = float(object.value(QLatin1String("alphaCutoff")).toDouble(0.5));
        material.doubleSided = object.value(QLatin1String("doubleSided")).toBool(false);

        material.extensions = object.value(QLatin1String("extensions")).toObject();
        const QJsonObject &ext = material.extensions;

        material.unlit = ext.contains(QLatin1String("KHR_materials_unlit"));

        const QJsonValue sgValue = ext.value(QLatin1String("KHR_materials_pbrSpecularGlossiness"));
        if (sgValue.isObject()) {
            const QJsonObject sg = sgValue.toObject();
            Material::SpecularGlossiness specularGlossiness;
            specularGlossiness.diffuseFactor = toVector4D(sg.value(QLatin1String("diffuseFactor")).toArray(),
                                                          specularGlossiness.diffuseFactor);
            specularGlossiness.diffuseTexture = parseTexture(sg.value(QLatin1String("diffuseTexture")).toObject());
            specularGlossiness.specularFactor = toVector3D(sg.value(QLatin1String("specularFactor")).toArray(),
                                                           specularGlossiness.specularFactor);
            specularGlossiness.glossinessFactor = float(sg.value(QLatin1String("glossinessFactor")).toDouble(1.0));
            specularGlossiness.specularGlossinessTexture = parseTexture(
                    sg.value(QLatin1String("specularGlossinessTexture")).toObject());
            material.specularGlossiness = specularGlossiness;
        }

        const QJsonValue ccValue = ext.value(QLatin1String("KHR_materials_clearcoat"));
        if (ccValue.isObject()) {
            const QJsonObject cc = ccValue.toObject();
            Material::Clearcoat clearcoat;
            clearcoat.clearcoatFactor = float(cc.value(QLatin1String("clearcoatFactor")).toDouble(0.0));
            clearcoat.clearcoatTexture = parseTexture(cc.value(QLatin1String("clearcoatTexture")).toObject());
            clearcoat.clearcoatRoughnessFactor =
                    float(cc.value(QLatin1String("clearcoatRoughnessFactor")).toDouble(0.0));
            clearcoat.clearcoatRoughnessTexture =
                    parseTexture(cc.value(QLatin1String("clearcoatRoughnessTexture")).toObject());
            clearcoat.clearcoatNormalTexture =
                    parseTexture(cc.value(QLatin1String("clearcoatNormalTexture")).toObject(), "scale");
            material.clearcoat = clearcoat;
        }

        const QJsonValue trValue = ext.value(QLatin1String("KHR_materials_transmission"));
        if (trValue.isObject()) {
            const QJsonObject tr = trValue.toObject();
            Material::Transmission transmission;
            transmission.transmissionFactor = float(tr.value(QLatin1String("transmissionFactor")).toDouble(0.0));
            transmission.transmissionTexture =
                    parseTexture(tr.value(QLatin1String("transmissionTexture")).toObject());
            material.transmission = transmission;
        }

        const QJsonValue volValue = ext.value(QLatin1String("KHR_materials_volume"));
        if (volValue.isObject()) {
            const QJsonObject vol = volValue.toObject();
            Material::Volume volume;
            volume.thicknessFactor = float(vol.value(QLatin1String("thicknessFactor")).toDouble(0.0));
            volume.thicknessTexture = parseTexture(vol.value(QLatin1String("thicknessTexture")).toObject());
            volume.attenuationDistance = float(vol.value(QLatin1String("attenuationDistance")).toDouble(0.0));
            volume.attenuationColor =
                    toVector3D(vol.value(QLatin1String("attenuationColor")).toArray(), volume.attenuationColor);
            material.volume = volume;
        }

        const QJsonValue iorValue = ext.value(QLatin1String("KHR_materials_ior"));
        if (iorValue.isObject())
            material.ior = float(iorValue.toObject().value(QLatin1String("ior")).toDouble(1.5));

        const QJsonValue esValue = ext.value(QLatin1String("KHR_materials_emissive_strength"));
        if (esValue.isObject())
            material.emissiveStrength =
                    float(esValue.toObject().value(QLatin1String("emissiveStrength")).toDouble(1.0));

        const QJsonValue spValue = ext.value(QLatin1String("KHR_materials_specular"));
        if (spValue.isObject()) {
            const QJsonObject sp = spValue.toObject();
            Material::Specular specular;
            specular.specularFactor = float(sp.value(QLatin1String("specularFactor")).toDouble(1.0));
            specular.specularTexture = parseTexture(sp.value(QLatin1String("specularTexture")).toObject());
            specular.specularColorFactor = toVector3D(sp.value(QLatin1String("specularColorFactor")).toArray(),
                                                      specular.specularColorFactor);
            specular.specularColorTexture =
                    parseTexture(sp.value(QLatin1String("specularColorTexture")).toObject());
            material.specular = specular;
        }

        document->materials.append(material);
    }

    if (badTextureIndex >= 0)
        return setError(QStringLiteral("Material references invalid texture %1").arg(badTextureIndex));

    // meshes
    for (const auto &value : root.value(QLatin1String("meshes")).toArray()) {
        const QJsonObject object = value.toObject();
        Mesh mesh;
        mesh.name = object.value(QLatin1String("name")).toString();
        for (const auto &w : object.value(QLatin1String("weights")).toArray())
            mesh.weights.append(float(w.toDouble()));

        for (const auto &primitiveValue : object.value(QLatin1String("primitives")).toArray()) {
            const QJsonObject primitiveObject = primitiveValue.toObject();
            MeshPrimitive primitive;
            const QJsonObject attributes = primitiveObject.value(QLatin1String("attributes")).toObject();
            for (auto it = attributes.constBegin(); it != attributes.constEnd(); ++it)
                primitive.attributes.insert(it.key().toUtf8(), it.value().toInt(-1));
            primitive.indices = primitiveObject.value(QLatin1String("indices")).toInt(-1);
            primitive.material = primitiveObject.value(QLatin1String("material")).toInt(-1);
            primitive.mode = primitiveObject.value(QLatin1String("mode")).toInt(MeshPrimitive::Triangles);
            for (const auto &targetValue : primitiveObject.value(QLatin1String("targets")).toArray()) {
                const QJsonObject targetObject = targetValue.toObject();
                QHash<QByteArray, int> target;
                for (auto it = targetObject.constBegin(); it != targetObject.constEnd(); ++it)
                    target.insert(it.key().toUtf8(), it.value().toInt(-1));
                primitive.targets.append(target);
            }
            primitive.extensions = primitiveObject.value(QLatin1String("extensions")).toObject();

            // Structural validation of accessor references. As everywhere in
            // this parser, any negative index means "unset": the glTF defaults
            // are -1, the readers return nothing for a negative index, and so
            // only the upper bound needs checking here.
            const auto checkAccessor = [&](int accessor, const char *what) {
                if (accessor >= document->accessors.size())
                    return setError(QStringLiteral("Mesh %1 primitive references invalid %2 accessor %3")
                                            .arg(document->meshes.size()).arg(QLatin1String(what)).arg(accessor));
                return true;
            };
            for (auto it = primitive.attributes.constBegin(); it != primitive.attributes.constEnd(); ++it) {
                if (!checkAccessor(it.value(), it.key().constData()))
                    return false;
            }
            if (primitive.indices >= 0 && !checkAccessor(primitive.indices, "index"))
                return false;
            if (primitive.material >= document->materials.size())
                return setError(QStringLiteral("Mesh %1 primitive references invalid material %2")
                                        .arg(document->meshes.size())
                                        .arg(primitive.material));

            mesh.primitives.append(primitive);
        }
        if (mesh.primitives.isEmpty())
            qCWarning(lcQuick3DGltf) << "Mesh" << document->meshes.size() << "has no primitives";
        document->meshes.append(mesh);
    }

    // cameras
    for (const auto &value : root.value(QLatin1String("cameras")).toArray()) {
        const QJsonObject object = value.toObject();
        Camera camera;
        camera.name = object.value(QLatin1String("name")).toString();
        const QString type = object.value(QLatin1String("type")).toString();
        if (type == QLatin1String("orthographic")) {
            camera.type = Camera::Type::Orthographic;
            const QJsonObject ortho = object.value(QLatin1String("orthographic")).toObject();
            camera.xmag = float(ortho.value(QLatin1String("xmag")).toDouble());
            camera.ymag = float(ortho.value(QLatin1String("ymag")).toDouble());
            camera.znear = float(ortho.value(QLatin1String("znear")).toDouble());
            camera.zfar = float(ortho.value(QLatin1String("zfar")).toDouble());
        } else {
            camera.type = Camera::Type::Perspective;
            const QJsonObject perspective = object.value(QLatin1String("perspective")).toObject();
            camera.aspectRatio = float(perspective.value(QLatin1String("aspectRatio")).toDouble(0.0));
            camera.yfov = float(perspective.value(QLatin1String("yfov")).toDouble());
            camera.znear = float(perspective.value(QLatin1String("znear")).toDouble());
            camera.zfar = float(perspective.value(QLatin1String("zfar")).toDouble(0.0));
        }
        document->cameras.append(camera);
    }

    // KHR_lights_punctual (document level)
    {
        const QJsonValue lightsExt = document->rootExtensions.value(QLatin1String("KHR_lights_punctual"));
        if (lightsExt.isObject()) {
            for (const auto &value : lightsExt.toObject().value(QLatin1String("lights")).toArray()) {
                const QJsonObject object = value.toObject();
                Light light;
                light.name = object.value(QLatin1String("name")).toString();
                const QString type = object.value(QLatin1String("type")).toString();
                if (type == QLatin1String("point"))
                    light.type = Light::Type::Point;
                else if (type == QLatin1String("spot"))
                    light.type = Light::Type::Spot;
                else
                    light.type = Light::Type::Directional;
                light.color = toVector3D(object.value(QLatin1String("color")).toArray(), light.color);
                light.intensity = float(object.value(QLatin1String("intensity")).toDouble(1.0));
                light.range = float(object.value(QLatin1String("range")).toDouble(0.0));
                if (light.type == Light::Type::Spot) {
                    const QJsonObject spot = object.value(QLatin1String("spot")).toObject();
                    light.innerConeAngle = float(spot.value(QLatin1String("innerConeAngle")).toDouble(0.0));
                    light.outerConeAngle = float(spot.value(QLatin1String("outerConeAngle")).toDouble(M_PI_4));
                }
                document->lights.append(light);
            }
        }
    }

    // nodes
    for (const auto &value : root.value(QLatin1String("nodes")).toArray()) {
        const QJsonObject object = value.toObject();
        Node node;
        node.name = object.value(QLatin1String("name")).toString();
        for (const auto &child : object.value(QLatin1String("children")).toArray())
            node.children.append(child.toInt(-1));
        node.mesh = object.value(QLatin1String("mesh")).toInt(-1);
        node.skin = object.value(QLatin1String("skin")).toInt(-1);
        node.camera = object.value(QLatin1String("camera")).toInt(-1);

        const QJsonValue matrixValue = object.value(QLatin1String("matrix"));
        if (matrixValue.isArray()) {
            const QJsonArray m = matrixValue.toArray();
            if (m.size() == 16) {
                float values[16];
                for (int i = 0; i < 16; ++i)
                    values[i] = float(m.at(i).toDouble());
                // glTF matrices are column-major; QMatrix4x4(float*) is row-major
                node.matrix = QMatrix4x4(values).transposed();
                node.hasMatrix = true;
            }
        }
        node.translation = toVector3D(object.value(QLatin1String("translation")).toArray(), node.translation);
        const QJsonArray rotation = object.value(QLatin1String("rotation")).toArray();
        if (rotation.size() == 4) {
            // glTF quaternions are (x, y, z, w)
            node.rotation = QQuaternion(float(rotation.at(3).toDouble()), float(rotation.at(0).toDouble()),
                                        float(rotation.at(1).toDouble()), float(rotation.at(2).toDouble()));
        }
        node.scale = toVector3D(object.value(QLatin1String("scale")).toArray(), node.scale);
        for (const auto &w : object.value(QLatin1String("weights")).toArray())
            node.weights.append(float(w.toDouble()));
        node.extensions = object.value(QLatin1String("extensions")).toObject();

        const QJsonValue lightExt = node.extensions.value(QLatin1String("KHR_lights_punctual"));
        if (lightExt.isObject())
            node.light = lightExt.toObject().value(QLatin1String("light")).toInt(-1);

        if (node.mesh >= document->meshes.size())
            return setError(QStringLiteral("Node %1 references invalid mesh %2")
                            .arg(document->nodes.size()).arg(node.mesh));
        if (node.camera >= document->cameras.size())
            return setError(QStringLiteral("Node %1 references invalid camera %2")
                            .arg(document->nodes.size()).arg(node.camera));
        if (node.light >= document->lights.size())
            return setError(QStringLiteral("Node %1 references invalid light %2")
                            .arg(document->nodes.size()).arg(node.light));
        document->nodes.append(node);
    }

    // Validate node graph references (children and skins can be forward
    // references). The specification requires the hierarchy to be a forest,
    // and consumers that recurse over the graph rely on that to be safe from
    // cycles and exponential blowup, so the shape is repaired here rather than
    // left for them to cope with.
    const int nodeCount = document->nodes.size();
    QVarLengthArray<int, 64> parentCount(nodeCount);
    std::fill(parentCount.begin(), parentCount.end(), 0);
    for (int i = 0; i < nodeCount; ++i) {
        Node &node = document->nodes[i];
        for (qsizetype c = 0; c < node.children.size(); ++c) {
            const int child = node.children.at(c);
            if (child < 0 || child >= nodeCount)
                return setError(QStringLiteral("Node %1 references invalid child node %2").arg(i).arg(child));
            if (++parentCount[child] > 1) {
                // A second parent cannot be represented at all: a node
                // occupies exactly one place in a scene graph. Keep the first
                // edge and drop this one, so the rest of the asset still
                // loads.
                qCWarning(lcQuick3DGltf) << "Node" << child
                                         << "has more than one parent; ignoring the edge from node" << i;
                --parentCount[child];
                node.children.removeAt(c--);
            }
        }
    }
    // With single parents the only remaining hazards are parentless cycles
    // and excessive depth; both are caught by walking up each node's parent
    // chain, memoizing the depths so every edge is visited only once.
    {
        constexpr int maxNodeDepth = 4096;
        QVarLengthArray<int, 64> parent(nodeCount);
        std::fill(parent.begin(), parent.end(), -1);
        for (int i = 0; i < nodeCount; ++i) {
            for (int child : document->nodes.at(i).children)
                parent[child] = i;
        }
        QVarLengthArray<int, 64> depth(nodeCount);
        std::fill(depth.begin(), depth.end(), -1);
        QVarLengthArray<int, 64> chain;
        for (int i = 0; i < nodeCount; ++i) {
            chain.clear();
            int n = i;
            while (n >= 0 && depth[n] < 0 && chain.size() <= nodeCount) {
                chain.append(n);
                n = parent[n];
            }
            if (chain.size() > nodeCount)
                return setError(QStringLiteral("Node hierarchy contains a cycle"));
            int d = n >= 0 ? depth[n] : 0;
            for (auto it = chain.rbegin(); it != chain.rend(); ++it)
                depth[*it] = ++d;
            if (d > maxNodeDepth)
                return setError(QStringLiteral("Node hierarchy is deeper than %1").arg(maxNodeDepth));
        }
    }

    // skins
    for (const auto &value : root.value(QLatin1String("skins")).toArray()) {
        const QJsonObject object = value.toObject();
        Skin skin;
        skin.name = object.value(QLatin1String("name")).toString();
        skin.inverseBindMatrices = object.value(QLatin1String("inverseBindMatrices")).toInt(-1);
        skin.skeleton = object.value(QLatin1String("skeleton")).toInt(-1);
        for (const auto &joint : object.value(QLatin1String("joints")).toArray())
            skin.joints.append(joint.toInt(-1));

        if (skin.inverseBindMatrices >= document->accessors.size())
            return setError(QStringLiteral("Skin %1 references invalid accessor %2")
                            .arg(document->skins.size()).arg(skin.inverseBindMatrices));
        for (int joint : std::as_const(skin.joints)) {
            if (joint < 0 || joint >= nodeCount)
                return setError(QStringLiteral("Skin %1 references invalid joint node %2")
                                .arg(document->skins.size()).arg(joint));
        }
        document->skins.append(skin);
    }
    for (int i = 0; i < nodeCount; ++i) {
        if (document->nodes.at(i).skin >= document->skins.size())
            return setError(QStringLiteral("Node %1 references invalid skin %2")
                            .arg(i).arg(document->nodes.at(i).skin));
    }

    // animations
    for (const auto &value : root.value(QLatin1String("animations")).toArray()) {
        const QJsonObject object = value.toObject();
        Animation animation;
        animation.name = object.value(QLatin1String("name")).toString();

        for (const auto &samplerValue : object.value(QLatin1String("samplers")).toArray()) {
            const QJsonObject samplerObject = samplerValue.toObject();
            AnimationSampler sampler;
            sampler.input = samplerObject.value(QLatin1String("input")).toInt(-1);
            sampler.output = samplerObject.value(QLatin1String("output")).toInt(-1);
            const QString interpolation = samplerObject.value(QLatin1String("interpolation")).toString();
            if (interpolation == QLatin1String("STEP"))
                sampler.interpolation = AnimationSampler::Interpolation::Step;
            else if (interpolation == QLatin1String("CUBICSPLINE"))
                sampler.interpolation = AnimationSampler::Interpolation::CubicSpline;
            if (sampler.input < 0 || sampler.input >= document->accessors.size()
                || sampler.output < 0 || sampler.output >= document->accessors.size()) {
                return setError(
                        QStringLiteral("Animation %1 sampler references an invalid accessor")
                                .arg(document->animations.size()));
            }
            animation.samplers.append(sampler);
        }

        for (const auto &channelValue : object.value(QLatin1String("channels")).toArray()) {
            const QJsonObject channelObject = channelValue.toObject();
            AnimationChannel channel;
            channel.sampler = channelObject.value(QLatin1String("sampler")).toInt(-1);
            const QJsonObject target = channelObject.value(QLatin1String("target")).toObject();
            channel.targetNode = target.value(QLatin1String("node")).toInt(-1);
            const QString path = target.value(QLatin1String("path")).toString();
            if (path == QLatin1String("rotation"))
                channel.path = AnimationChannel::Path::Rotation;
            else if (path == QLatin1String("scale"))
                channel.path = AnimationChannel::Path::Scale;
            else if (path == QLatin1String("weights"))
                channel.path = AnimationChannel::Path::Weights;
            else if (path != QLatin1String("translation")) {
                qCWarning(lcQuick3DGltf) << "Ignoring animation channel with unsupported path" << path;
                continue;
            }
            if (channel.sampler < 0 || channel.sampler >= animation.samplers.size())
                return setError(QStringLiteral("Animation %1 channel references invalid sampler %2")
                                        .arg(document->animations.size())
                                        .arg(channel.sampler));
            if (channel.targetNode >= nodeCount)
                return setError(QStringLiteral("Animation %1 channel references invalid node %2")
                                        .arg(document->animations.size())
                                        .arg(channel.targetNode));
            animation.channels.append(channel);
        }

        document->animations.append(animation);
    }

    // scenes
    for (const auto &value : root.value(QLatin1String("scenes")).toArray()) {
        const QJsonObject object = value.toObject();
        Scene scene;
        scene.name = object.value(QLatin1String("name")).toString();
        QSet<int> seenRoots;
        for (const auto &nodeIndex : object.value(QLatin1String("nodes")).toArray()) {
            const int index = nodeIndex.toInt(-1);
            if (index < 0 || index >= nodeCount)
                return setError(QStringLiteral("Scene %1 references invalid node %2")
                                .arg(document->scenes.size()).arg(index));
            // Scene nodes must be distinct root nodes; a repeated node, or one
            // that is also somebody's child, would have its subtree emitted
            // more than once. Skip the offending entry rather than failing, so
            // that the scene still contains everything it should, once.
            if (parentCount[index] != 0 || seenRoots.contains(index)) {
                qCWarning(lcQuick3DGltf) << "Scene" << document->scenes.size() << "references node" << index
                                         << "which is not a unique root node; ignoring it";
                continue;
            }
            seenRoots.insert(index);
            scene.nodes.append(index);
        }
        document->scenes.append(scene);
    }
    document->scene = root.value(QLatin1String("scene")).toInt(-1);
    if (document->scene >= document->scenes.size())
        return setError(QStringLiteral("Document references invalid default scene %1").arg(document->scene));

    return true;
}

QT_END_NAMESPACE
