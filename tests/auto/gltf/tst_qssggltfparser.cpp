// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QtTest>

#include <QtQuick3DGltf/private/qssggltfaccessorreader_p.h>
#include <QtQuick3DGltf/private/qssggltfparser_p.h>
#include <QtQuick3DGltf/private/qssggltfresourceresolver_p.h>

#include <QtCore/qendian.h>
#include <QtCore/qfile.h>

class tst_qssggltfparser : public QObject
{
    Q_OBJECT

private slots:
    void externalBuffer();
    void embeddedBuffer();
    void glbContainer();
    void glbBadMagic();
    void glbTruncated();
    void glbUnsupportedVersion();
    void dataUriDecoding();
    void requiredExtensionGate();
    void materials();
    void lightsAndCameras();
    void sparseAccessorStructure();
    void structuralValidation();
    void hostileInputValidation();
    void readAccessorData();
    void readSparseAccessor();
    void readInterleavedAccessor();
    void readNormalizedAccessor();

private:
    QString testFile(const QString &name) const
    {
        return QFINDTESTDATA(QStringLiteral("data/") + name);
    }

    QByteArray makeGlb(const QByteArray &json, const QByteArray &bin = {}) const;
};

// Builds a GLB container from a JSON chunk and an optional BIN chunk
QByteArray tst_qssggltfparser::makeGlb(const QByteArray &json, const QByteArray &bin) const
{
    const auto appendUInt32 = [](QByteArray &out, quint32 value) {
        quint32 le;
        qToLittleEndian(value, &le);
        out.append(reinterpret_cast<const char *>(&le), 4);
    };
    const auto padded = [](QByteArray chunk, char padByte) {
        while (chunk.size() % 4)
            chunk.append(padByte);
        return chunk;
    };

    const QByteArray jsonChunk = padded(json, ' ');
    const QByteArray binChunk = padded(bin, '\0');

    QByteArray out;
    appendUInt32(out, 0x46546C67); // magic
    appendUInt32(out, 2); // version
    quint32 total = 12 + 8 + jsonChunk.size();
    if (!bin.isEmpty())
        total += 8 + binChunk.size();
    appendUInt32(out, total);

    appendUInt32(out, jsonChunk.size());
    appendUInt32(out, 0x4E4F534A); // 'JSON'
    out += jsonChunk;

    if (!bin.isEmpty()) {
        appendUInt32(out, binChunk.size());
        appendUInt32(out, 0x004E4942); // 'BIN\0'
        out += binChunk;
    }
    return out;
}

void tst_qssggltfparser::externalBuffer()
{
    QSSGGltfParser parser;
    QSSGGltfDocument document;
    QVERIFY2(parser.parseFile(testFile(QStringLiteral("triangle.gltf")), &document),
             qPrintable(parser.errorMessage()));

    QCOMPARE(document.asset.version, QStringLiteral("2.0"));
    QCOMPARE(document.buffers.size(), 1);
    QCOMPARE(document.buffers.first().data.size(), 44);
    QCOMPARE(document.bufferViews.size(), 2);
    QCOMPARE(document.accessors.size(), 2);
    QCOMPARE(document.accessors.at(0).type, QSSGGltf::Accessor::Type::Vec3);
    QCOMPARE(document.accessors.at(0).componentType, QSSGGltf::Accessor::ComponentType::Float);
    QCOMPARE(document.accessors.at(0).count, 3);
    QCOMPARE(document.accessors.at(1).componentType, QSSGGltf::Accessor::ComponentType::UnsignedShort);
    QCOMPARE(document.meshes.size(), 1);
    QCOMPARE(document.meshes.first().primitives.size(), 1);
    QCOMPARE(document.meshes.first().primitives.first().attributes.value("POSITION"), 0);
    QCOMPARE(document.meshes.first().primitives.first().indices, 1);
    QCOMPARE(document.nodes.size(), 1);
    QCOMPARE(document.nodes.first().translation, QVector3D(1, 2, 3));
    QCOMPARE(document.scene, 0);
    QCOMPARE(document.scenes.first().nodes, QList<int>({ 0 }));

    // Buffer contents: second vertex is (1, 0, 0)
    const float *positions = reinterpret_cast<const float *>(document.buffers.first().data.constData());
    QCOMPARE(positions[3], 1.0f);
}

void tst_qssggltfparser::embeddedBuffer()
{
    QSSGGltfParser parser;
    QSSGGltfDocument document;
    QVERIFY2(parser.parseFile(testFile(QStringLiteral("triangle_embedded.gltf")), &document),
             qPrintable(parser.errorMessage()));
    QCOMPARE(document.buffers.first().data.size(), 44);

    // The embedded buffer must decode to the same bytes as the external one
    QFile binFile(testFile(QStringLiteral("triangle.bin")));
    QVERIFY(binFile.open(QIODevice::ReadOnly));
    QCOMPARE(document.buffers.first().data, binFile.readAll());
}

void tst_qssggltfparser::glbContainer()
{
    QFile jsonFile(testFile(QStringLiteral("triangle_embedded.gltf")));
    QVERIFY(jsonFile.open(QIODevice::ReadOnly));
    const QByteArray glb = makeGlb(jsonFile.readAll());

    QSSGGltfParser parser;
    QSSGGltfDocument document;
    QVERIFY2(parser.parse(glb, QString(), &document), qPrintable(parser.errorMessage()));
    QCOMPARE(document.meshes.size(), 1);

    // GLB with the buffer moved into the BIN chunk
    QFile binFile(testFile(QStringLiteral("triangle.bin")));
    QVERIFY(binFile.open(QIODevice::ReadOnly));
    const QByteArray bin = binFile.readAll();
    const QByteArray json = QByteArrayLiteral(
            "{ \"asset\": { \"version\": \"2.0\" },"
            "  \"buffers\": [ { \"byteLength\": 44 } ],"
            "  \"bufferViews\": [ { \"buffer\": 0, \"byteOffset\": 0, \"byteLength\": 36 } ],"
            "  \"accessors\": [ { \"bufferView\": 0, \"componentType\": 5126, \"count\": 3, \"type\": \"VEC3\" } ] }");
    QVERIFY2(parser.parse(makeGlb(json, bin), QString(), &document), qPrintable(parser.errorMessage()));
    QCOMPARE(document.buffers.first().data, bin);
}

void tst_qssggltfparser::glbBadMagic()
{
    QByteArray glb = makeGlb(QByteArrayLiteral("{ \"asset\": { \"version\": \"2.0\" } }"));
    glb[0] = 'X';

    // Not GLB magic, so it falls through to JSON parsing and fails there
    QSSGGltfParser parser;
    QSSGGltfDocument document;
    QVERIFY(!parser.parse(glb, QString(), &document));
    QVERIFY(parser.errorMessage().contains(QStringLiteral("Invalid glTF JSON")));
}

void tst_qssggltfparser::glbTruncated()
{
    const QByteArray glb = makeGlb(QByteArrayLiteral("{ \"asset\": { \"version\": \"2.0\" } }"));

    QSSGGltfParser parser;
    QSSGGltfDocument document;
    QVERIFY(!parser.parse(glb.left(glb.size() - 8), QString(), &document));
    QVERIFY2(parser.errorMessage().contains(QStringLiteral("Truncated")), qPrintable(parser.errorMessage()));
}

void tst_qssggltfparser::glbUnsupportedVersion()
{
    QByteArray glb = makeGlb(QByteArrayLiteral("{ \"asset\": { \"version\": \"2.0\" } }"));
    glb[4] = 3; // container version

    QSSGGltfParser parser;
    QSSGGltfDocument document;
    QVERIFY(!parser.parse(glb, QString(), &document));
    QVERIFY(parser.errorMessage().contains(QStringLiteral("version")));
}

void tst_qssggltfparser::dataUriDecoding()
{
    QCOMPARE(QSSGGltfResourceResolver::decodeDataUri(u"data:application/octet-stream;base64,SGVsbG8="),
             QByteArrayLiteral("Hello"));
    QCOMPARE(QSSGGltfResourceResolver::decodeDataUri(u"data:,Hello%20World"),
             QByteArrayLiteral("Hello World"));

    // A non-base64 payload carries arbitrary octets, not text. Decoding it via
    // QString would turn every sequence that is not valid UTF-8 into U+FFFD,
    // which changes the length as well as the contents, and a buffer whose
    // length still passes its size check then loads silently corrupted.
    QCOMPARE(QSSGGltfResourceResolver::decodeDataUri(u"data:,%FF%FE%00A"),
             QByteArrayLiteral("\xff\xfe\x00" "A"));

    QString error;
    QVERIFY(QSSGGltfResourceResolver::decodeDataUri(u"data:application/octet-stream;base64", &error).isEmpty());
    QVERIFY(!error.isEmpty());

    QVERIFY(QSSGGltfResourceResolver::isDataUri(u"data:,x"));
    QVERIFY(!QSSGGltfResourceResolver::isDataUri(u"buffer.bin"));
}

void tst_qssggltfparser::requiredExtensionGate()
{
    QSSGGltfParser parser;
    QSSGGltfDocument document;
    QVERIFY(!parser.parseFile(testFile(QStringLiteral("draco_required.gltf")), &document));
    QCOMPARE(parser.errorMessage(),
             QStringLiteral("Asset requires unsupported glTF extension 'KHR_draco_mesh_compression'"));

    // The same extension only listed in extensionsUsed parses with a warning
    const QByteArray json = QByteArrayLiteral(
            "{ \"asset\": { \"version\": \"2.0\" },"
            "  \"extensionsUsed\": [ \"KHR_texture_basisu\" ] }");
    QVERIFY2(parser.parse(json, QString(), &document), qPrintable(parser.errorMessage()));
}

void tst_qssggltfparser::materials()
{
    QSSGGltfParser parser;
    QSSGGltfDocument document;
    QVERIFY2(parser.parseFile(testFile(QStringLiteral("material_extensions.gltf")), &document),
             qPrintable(parser.errorMessage()));

    QCOMPARE(document.materials.size(), 3);

    const QSSGGltf::Material &full = document.materials.at(0);
    QCOMPARE(full.name, QStringLiteral("full"));
    QCOMPARE(full.baseColorFactor, QVector4D(0.8f, 0.6f, 0.4f, 1.0f));
    QCOMPARE(full.metallicFactor, 0.7f);
    QCOMPARE(full.roughnessFactor, 0.3f);
    QCOMPARE(full.baseColorTexture.index, 0);
    QVERIFY(full.baseColorTexture.transform.has_value());
    QCOMPARE(full.baseColorTexture.transform->offset, QVector2D(0.25f, 0.5f));
    QCOMPARE(full.baseColorTexture.transform->scale, QVector2D(2.0f, 3.0f));
    QCOMPARE(full.normalTexture.scaleOrStrength, 0.9f);
    QCOMPARE(full.occlusionTexture.scaleOrStrength, 0.8f);
    QCOMPARE(full.emissiveFactor, QVector3D(0.1f, 0.2f, 0.3f));
    QCOMPARE(full.alphaMode, QSSGGltf::Material::AlphaMode::Mask);
    QCOMPARE(full.alphaCutoff, 0.25f);
    QVERIFY(full.doubleSided);

    QVERIFY(full.clearcoat.has_value());
    QCOMPARE(full.clearcoat->clearcoatFactor, 0.6f);
    QCOMPARE(full.clearcoat->clearcoatRoughnessFactor, 0.2f);
    QVERIFY(full.transmission.has_value());
    QCOMPARE(full.transmission->transmissionFactor, 0.75f);
    QVERIFY(full.volume.has_value());
    QCOMPARE(full.volume->thicknessFactor, 1.2f);
    QCOMPARE(full.volume->attenuationDistance, 0.5f);
    QCOMPARE(full.volume->attenuationColor, QVector3D(0.9f, 0.8f, 0.7f));
    QVERIFY(full.ior.has_value());
    QCOMPARE(*full.ior, 1.4f);
    QVERIFY(full.emissiveStrength.has_value());
    QCOMPARE(*full.emissiveStrength, 5.0f);
    QVERIFY(full.specular.has_value());
    QCOMPARE(full.specular->specularFactor, 0.5f);
    QCOMPARE(full.specular->specularColorFactor, QVector3D(0.2f, 0.4f, 0.6f));

    const QSSGGltf::Material &specGloss = document.materials.at(1);
    QVERIFY(specGloss.specularGlossiness.has_value());
    QCOMPARE(specGloss.specularGlossiness->diffuseFactor, QVector4D(0.5f, 0.5f, 0.5f, 1.0f));
    QCOMPARE(specGloss.specularGlossiness->specularFactor, QVector3D(0.1f, 0.2f, 0.3f));
    QCOMPARE(specGloss.specularGlossiness->glossinessFactor, 0.4f);

    QVERIFY(document.materials.at(2).unlit);

    // Sampler values
    QCOMPARE(document.samplers.size(), 1);
    QCOMPARE(document.samplers.first().magFilter, int(QSSGGltf::Sampler::Nearest));
    QCOMPARE(document.samplers.first().minFilter, int(QSSGGltf::Sampler::LinearMipMapNearest));
    QCOMPARE(document.samplers.first().wrapS, int(QSSGGltf::Sampler::ClampToEdge));
    QCOMPARE(document.samplers.first().wrapT, int(QSSGGltf::Sampler::MirroredRepeat));
}

void tst_qssggltfparser::lightsAndCameras()
{
    QSSGGltfParser parser;
    QSSGGltfDocument document;
    QVERIFY2(parser.parseFile(testFile(QStringLiteral("material_extensions.gltf")), &document),
             qPrintable(parser.errorMessage()));

    QCOMPARE(document.lights.size(), 3);
    QCOMPARE(document.lights.at(0).type, QSSGGltf::Light::Type::Directional);
    QCOMPARE(document.lights.at(0).color, QVector3D(1.0f, 0.9f, 0.8f));
    QCOMPARE(document.lights.at(0).intensity, 3.0f);
    QCOMPARE(document.lights.at(1).type, QSSGGltf::Light::Type::Point);
    QCOMPARE(document.lights.at(1).range, 10.0f);
    QCOMPARE(document.lights.at(2).type, QSSGGltf::Light::Type::Spot);
    QCOMPARE(document.lights.at(2).innerConeAngle, 0.2f);
    QCOMPARE(document.lights.at(2).outerConeAngle, 0.5f);

    QCOMPARE(document.nodes.at(0).light, 2);
    QCOMPARE(document.nodes.at(1).camera, 0);

    QCOMPARE(document.cameras.size(), 2);
    QCOMPARE(document.cameras.at(0).type, QSSGGltf::Camera::Type::Perspective);
    QCOMPARE(document.cameras.at(0).yfov, 0.66f);
    QCOMPARE(document.cameras.at(0).zfar, 1000.0f);
    QCOMPARE(document.cameras.at(1).type, QSSGGltf::Camera::Type::Orthographic);
    QCOMPARE(document.cameras.at(1).xmag, 2.0f);
    QCOMPARE(document.cameras.at(1).ymag, 1.5f);
}

void tst_qssggltfparser::sparseAccessorStructure()
{
    QSSGGltfParser parser;
    QSSGGltfDocument document;
    QVERIFY2(parser.parseFile(testFile(QStringLiteral("sparse.gltf")), &document),
             qPrintable(parser.errorMessage()));

    QCOMPARE(document.accessors.size(), 1);
    const QSSGGltf::Accessor &accessor = document.accessors.first();
    QVERIFY(accessor.sparse.has_value());
    QCOMPARE(accessor.sparse->count, 2);
    QCOMPARE(accessor.sparse->indicesBufferView, 1);
    QCOMPARE(accessor.sparse->indicesComponentType, QSSGGltf::Accessor::ComponentType::UnsignedShort);
    QCOMPARE(accessor.sparse->valuesBufferView, 2);
}

void tst_qssggltfparser::structuralValidation()
{
    QSSGGltfParser parser;
    QSSGGltfDocument document;

    // Accessor past the end of its buffer view
    QVERIFY(!parser.parse(QByteArrayLiteral(
            "{ \"asset\": { \"version\": \"2.0\" },"
            "  \"buffers\": [ { \"uri\": \"data:,0123456789\", \"byteLength\": 10 } ],"
            "  \"bufferViews\": [ { \"buffer\": 0, \"byteLength\": 10 } ],"
            "  \"accessors\": [ { \"bufferView\": 0, \"componentType\": 5126, \"count\": 3, \"type\": \"VEC3\" } ] }"),
            QString(), &document));
    QVERIFY2(parser.errorMessage().contains(QStringLiteral("extends past")), qPrintable(parser.errorMessage()));

    // Buffer view past the end of its buffer
    QVERIFY(!parser.parse(QByteArrayLiteral(
            "{ \"asset\": { \"version\": \"2.0\" },"
            "  \"buffers\": [ { \"uri\": \"data:,0123456789\", \"byteLength\": 10 } ],"
            "  \"bufferViews\": [ { \"buffer\": 0, \"byteOffset\": 8, \"byteLength\": 10 } ] }"),
            QString(), &document));

    // Node referencing an out-of-range mesh
    QVERIFY(!parser.parse(QByteArrayLiteral(
            "{ \"asset\": { \"version\": \"2.0\" }, \"nodes\": [ { \"mesh\": 7 } ] }"),
            QString(), &document));

    // Scene referencing an out-of-range node
    QVERIFY(!parser.parse(QByteArrayLiteral(
            "{ \"asset\": { \"version\": \"2.0\" }, \"scenes\": [ { \"nodes\": [3] } ], \"scene\": 0 }"),
            QString(), &document));

    // glTF 1.x is rejected
    QVERIFY(!parser.parse(QByteArrayLiteral("{ \"asset\": { \"version\": \"1.0\" } }"), QString(), &document));
    QVERIFY(parser.errorMessage().contains(QStringLiteral("version")));

    // Missing asset object
    QVERIFY(!parser.parse(QByteArrayLiteral("{ }"), QString(), &document));
}

// Inputs a well-meaning exporter would never produce, but a hostile file
// can: each of these used to be able to reach out-of-bounds reads or
// writes, oversized allocations, or infinite recursion in consumers.
void tst_qssggltfparser::hostileInputValidation()
{
    QSSGGltfParser parser;
    QSSGGltfDocument document;

    // Negative buffer view byte offset (would address before the buffer)
    QVERIFY(!parser.parse(QByteArrayLiteral(
            "{ \"asset\": { \"version\": \"2.0\" },"
            "  \"buffers\": [ { \"uri\": \"data:,0123456789\", \"byteLength\": 10 } ],"
            "  \"bufferViews\": [ { \"buffer\": 0, \"byteOffset\": -1000000, \"byteLength\": 10 } ] }"),
            QString(), &document));

    // Buffer view byte stride beyond the specification maximum of 252
    QVERIFY(!parser.parse(QByteArrayLiteral(
            "{ \"asset\": { \"version\": \"2.0\" },"
            "  \"buffers\": [ { \"uri\": \"data:,0123456789\", \"byteLength\": 10 } ],"
            "  \"bufferViews\": [ { \"buffer\": 0, \"byteLength\": 10, \"byteStride\": 1000000000 } ] }"),
            QString(), &document));

    // Buffer view byte offset and length that are each positive but whose sum
    // overflows to a negative value, which would otherwise read as fitting
    // inside a ten-byte buffer
    QVERIFY(!parser.parse(QByteArrayLiteral(
            "{ \"asset\": { \"version\": \"2.0\" },"
            "  \"buffers\": [ { \"uri\": \"data:,0123456789\", \"byteLength\": 10 } ],"
            "  \"bufferViews\": [ { \"buffer\": 0, \"byteOffset\": 5000000000000000000,"
            "                       \"byteLength\": 5000000000000000000 } ] }"),
            QString(), &document));
    QVERIFY2(parser.errorMessage().contains(QStringLiteral("extends past")), qPrintable(parser.errorMessage()));

    // Negative accessor byte offset and negative count
    QVERIFY(!parser.parse(QByteArrayLiteral(
            "{ \"asset\": { \"version\": \"2.0\" },"
            "  \"buffers\": [ { \"uri\": \"data:,0123456789\", \"byteLength\": 10 } ],"
            "  \"bufferViews\": [ { \"buffer\": 0, \"byteLength\": 10 } ],"
            "  \"accessors\": [ { \"bufferView\": 0, \"byteOffset\": -8, \"componentType\": 5126,"
            "                     \"count\": 1, \"type\": \"SCALAR\" } ] }"),
            QString(), &document));
    QVERIFY(!parser.parse(QByteArrayLiteral(
            "{ \"asset\": { \"version\": \"2.0\" },"
            "  \"accessors\": [ { \"componentType\": 5126, \"count\": -5, \"type\": \"SCALAR\" } ] }"),
            QString(), &document));

    // Accessor without a buffer view reads as zeros, so a huge count is an
    // allocation bomb rather than a data reference
    QVERIFY(!parser.parse(QByteArrayLiteral(
            "{ \"asset\": { \"version\": \"2.0\" },"
            "  \"accessors\": [ { \"componentType\": 5126, \"count\": 4000000000, \"type\": \"VEC4\" } ] }"),
            QString(), &document));
    QVERIFY2(parser.errorMessage().contains(QStringLiteral("unreasonably large")),
             qPrintable(parser.errorMessage()));

    // A count of 2^62 is exactly representable as a double, so it survives JSON
    // parsing intact. Without an element cap, (count - 1) * stride + elementSize
    // overflows to 0, the bounds check below it reads as satisfied, and a
    // ten-line document yields an accessor claiming 2^62 elements.
    QVERIFY(!parser.parse(QByteArrayLiteral(
            "{ \"asset\": { \"version\": \"2.0\" },"
            "  \"buffers\": [ { \"uri\": \"data:,0123456789\", \"byteLength\": 10 } ],"
            "  \"bufferViews\": [ { \"buffer\": 0, \"byteLength\": 10 } ],"
            "  \"accessors\": [ { \"bufferView\": 0, \"componentType\": 5126,"
            "                     \"count\": 4611686018427387904, \"type\": \"SCALAR\" } ] }"),
            QString(), &document));
    QVERIFY2(parser.errorMessage().contains(QStringLiteral("unreasonably large")),
             qPrintable(parser.errorMessage()));

    // The same cap has to apply to the sparse count, which is multiplied by a
    // component size while validating the sparse buffer views
    QVERIFY(!parser.parse(QByteArrayLiteral(
            "{ \"asset\": { \"version\": \"2.0\" },"
            "  \"buffers\": [ { \"uri\": \"data:,0123456789\", \"byteLength\": 10 } ],"
            "  \"bufferViews\": [ { \"buffer\": 0, \"byteLength\": 10 } ],"
            "  \"accessors\": [ { \"bufferView\": 0, \"componentType\": 5126, \"count\": 1,"
            "                     \"type\": \"SCALAR\","
            "                     \"sparse\": { \"count\": 4611686018427387904,"
            "                                   \"indices\": { \"bufferView\": 0, \"componentType\": 5123 },"
            "                                   \"values\": { \"bufferView\": 0 } } } ] }"),
            QString(), &document));

    // A near-2^63 offset only overflows a span of 1025 bytes or more, and a span
    // that large has to fit its buffer view, hence the 2048-byte buffer
    const auto withLargeBuffer = [](const QByteArray &accessor) {
        return QByteArrayLiteral("{ \"asset\": { \"version\": \"2.0\" },"
                                 "  \"buffers\": [ { \"uri\": \"data:,")
                + QByteArray(2048, 'A')
                + QByteArrayLiteral("\", \"byteLength\": 2048 } ],"
                                    "  \"bufferViews\": [ { \"buffer\": 0, \"byteLength\": 2048 } ],"
                                    "  \"accessors\": [ ")
                + accessor + QByteArrayLiteral(" ] }");
    };

    // Accessor byte offset that overflows once its span is added
    QVERIFY(!parser.parse(withLargeBuffer(QByteArrayLiteral(
            "{ \"bufferView\": 0, \"byteOffset\": 9223372036854774784,"
            "  \"componentType\": 5126, \"count\": 300, \"type\": \"SCALAR\" }")),
            QString(), &document));
    QVERIFY2(parser.errorMessage().contains(QStringLiteral("extends past")),
             qPrintable(parser.errorMessage()));

    // The same overflow in each of the two sparse byte offsets
    QVERIFY(!parser.parse(withLargeBuffer(QByteArrayLiteral(
            "{ \"componentType\": 5123, \"count\": 600, \"type\": \"SCALAR\","
            "  \"sparse\": { \"count\": 600,"
            "                \"indices\": { \"bufferView\": 0, \"componentType\": 5123,"
            "                               \"byteOffset\": 9223372036854774784 },"
            "                \"values\": { \"bufferView\": 0 } } }")),
            QString(), &document));
    QVERIFY2(parser.errorMessage().contains(QStringLiteral("sparse")), qPrintable(parser.errorMessage()));
    QVERIFY(!parser.parse(withLargeBuffer(QByteArrayLiteral(
            "{ \"componentType\": 5123, \"count\": 600, \"type\": \"SCALAR\","
            "  \"sparse\": { \"count\": 600,"
            "                \"indices\": { \"bufferView\": 0, \"componentType\": 5123 },"
            "                \"values\": { \"bufferView\": 0,"
            "                              \"byteOffset\": 9223372036854774784 } } }")),
            QString(), &document));
    QVERIFY2(parser.errorMessage().contains(QStringLiteral("sparse")), qPrintable(parser.errorMessage()));

    // A material naming a texture that does not exist
    QVERIFY(!parser.parse(QByteArrayLiteral(
            "{ \"asset\": { \"version\": \"2.0\" },"
            "  \"materials\": [ { \"pbrMetallicRoughness\":"
            "                     { \"baseColorTexture\": { \"index\": 3 } } } ] }"),
            QString(), &document));
    QVERIFY2(parser.errorMessage().contains(QStringLiteral("invalid texture")),
             qPrintable(parser.errorMessage()));

    // A buffer URI is a relative reference; one that climbs out of the asset
    // directory must not be resolved and loaded
    QVERIFY(QSSGGltfResourceResolver::resolveFilePath(u"../../../etc/passwd",
                                                      QStringLiteral("/tmp/assets")).isEmpty());
    QVERIFY(QSSGGltfResourceResolver::resolveFilePath(u"/etc/passwd",
                                                      QStringLiteral("/tmp/assets")).isEmpty());
    QVERIFY(QSSGGltfResourceResolver::resolveFilePath(u":/secret.bin",
                                                      QStringLiteral("/tmp/assets")).isEmpty());
    QCOMPARE(QSSGGltfResourceResolver::resolveFilePath(u"textures/wood.png",
                                                       QStringLiteral("/tmp/assets")),
             QStringLiteral("/tmp/assets/textures/wood.png"));

    // minVersion above what is implemented has to fail rather than load
    QVERIFY(!parser.parse(QByteArrayLiteral(
            "{ \"asset\": { \"version\": \"2.0\", \"minVersion\": \"2.1\" } }"),
            QString(), &document));

    // Sparse indices extending past their buffer view
    QVERIFY(!parser.parse(QByteArrayLiteral(
            "{ \"asset\": { \"version\": \"2.0\" },"
            "  \"buffers\": [ { \"uri\": \"data:,0123456789\", \"byteLength\": 10 } ],"
            "  \"bufferViews\": [ { \"buffer\": 0, \"byteLength\": 10 } ],"
            "  \"accessors\": [ { \"componentType\": 5126, \"count\": 1000, \"type\": \"SCALAR\","
            "                     \"sparse\": { \"count\": 1000,"
            "                                   \"indices\": { \"bufferView\": 0, \"componentType\": 5125 },"
            "                                   \"values\": { \"bufferView\": 0 } } } ] }"),
            QString(), &document));
    QVERIFY2(parser.errorMessage().contains(QStringLiteral("sparse")), qPrintable(parser.errorMessage()));

    // A node hierarchy cycle would recurse forever in consumers
    QVERIFY(!parser.parse(QByteArrayLiteral(
            "{ \"asset\": { \"version\": \"2.0\" },"
            "  \"nodes\": [ { \"children\": [1] }, { \"children\": [0] } ],"
            "  \"scenes\": [ { \"nodes\": [] } ], \"scene\": 0 }"),
            QString(), &document));

    // A node with two parents cannot be represented, and emitting its subtree
    // twice is not an option either, so the second edge is dropped and the
    // rest of the asset still loads. The first parent keeps the child.
    QTest::ignoreMessage(QtWarningMsg, "Node 2 has more than one parent; ignoring the edge from node 1");
    QVERIFY2(parser.parse(QByteArrayLiteral(
            "{ \"asset\": { \"version\": \"2.0\" },"
            "  \"nodes\": [ { \"children\": [2] }, { \"children\": [2] }, { } ],"
            "  \"scenes\": [ { \"nodes\": [0, 1] } ], \"scene\": 0 }"),
            QString(), &document), qPrintable(parser.errorMessage()));
    QCOMPARE(document.nodes.at(0).children, QList<int>({ 2 }));
    QCOMPARE(document.nodes.at(1).children, QList<int>());

    // The same node listed twice as a child of one parent
    QTest::ignoreMessage(QtWarningMsg, "Node 1 has more than one parent; ignoring the edge from node 0");
    QVERIFY2(parser.parse(QByteArrayLiteral(
            "{ \"asset\": { \"version\": \"2.0\" },"
            "  \"nodes\": [ { \"children\": [1, 1] }, { } ] }"),
            QString(), &document), qPrintable(parser.errorMessage()));
    QCOMPARE(document.nodes.at(0).children, QList<int>({ 1 }));

    // Scene nodes must be unique root nodes, so a repeated entry, or one that
    // is somebody's child, is dropped from the scene rather than failing it
    QTest::ignoreMessage(QtWarningMsg, "Scene 0 references node 0 which is not a unique root node; ignoring it");
    QVERIFY2(parser.parse(QByteArrayLiteral(
            "{ \"asset\": { \"version\": \"2.0\" },"
            "  \"nodes\": [ { }, { } ],"
            "  \"scenes\": [ { \"nodes\": [0, 0] } ], \"scene\": 0 }"),
            QString(), &document), qPrintable(parser.errorMessage()));
    QCOMPARE(document.scenes.at(0).nodes, QList<int>({ 0 }));

    QTest::ignoreMessage(QtWarningMsg, "Scene 0 references node 1 which is not a unique root node; ignoring it");
    QVERIFY2(parser.parse(QByteArrayLiteral(
            "{ \"asset\": { \"version\": \"2.0\" },"
            "  \"nodes\": [ { \"children\": [1] }, { } ],"
            "  \"scenes\": [ { \"nodes\": [0, 1] } ], \"scene\": 0 }"),
            QString(), &document), qPrintable(parser.errorMessage()));
    QCOMPARE(document.scenes.at(0).nodes, QList<int>({ 0 }));

    // An absurdly deep hierarchy would exhaust the stack in recursive
    // consumers; a moderately deep one must keep working
    const auto chainOfNodes = [](int count) {
        QByteArray json = QByteArrayLiteral("{ \"asset\": { \"version\": \"2.0\" }, \"nodes\": [");
        for (int i = 0; i < count; ++i) {
            if (i)
                json += ',';
            json += i + 1 < count ? QByteArray("{ \"children\": [" + QByteArray::number(i + 1) + "] }")
                                  : QByteArray("{ }");
        }
        json += QByteArrayLiteral("], \"scenes\": [ { \"nodes\": [0] } ], \"scene\": 0 }");
        return json;
    };
    QVERIFY2(parser.parse(chainOfNodes(512), QString(), &document), qPrintable(parser.errorMessage()));
    QVERIFY(!parser.parse(chainOfNodes(100000), QString(), &document));
    QVERIFY2(parser.errorMessage().contains(QStringLiteral("deeper")), qPrintable(parser.errorMessage()));
}

void tst_qssggltfparser::readAccessorData()
{
    QSSGGltfParser parser;
    QSSGGltfDocument document;
    QVERIFY2(parser.parseFile(testFile(QStringLiteral("triangle.gltf")), &document),
             qPrintable(parser.errorMessage()));

    const QList<float> positions = QSSGGltfAccessorReader::readAsFloats(document, 0);
    QCOMPARE(positions.size(), 9);
    QCOMPARE(positions.at(3), 1.0f); // second vertex x
    QCOMPARE(positions.at(7), 1.0f); // third vertex y

    const QList<quint32> indices = QSSGGltfAccessorReader::readIndices(document, 1);
    QCOMPARE(indices, QList<quint32>({ 0, 1, 2 }));

    // Out-of-range accessor indices are safe
    QVERIFY(QSSGGltfAccessorReader::readAsFloats(document, -1).isEmpty());
    QVERIFY(QSSGGltfAccessorReader::readAsFloats(document, 7).isEmpty());
}

void tst_qssggltfparser::readSparseAccessor()
{
    QSSGGltfParser parser;
    QSSGGltfDocument document;
    QVERIFY2(parser.parseFile(testFile(QStringLiteral("sparse.gltf")), &document),
             qPrintable(parser.errorMessage()));

    // The base data is all zeros; sparse substitution replaces elements 2 and 4
    const QList<float> positions = QSSGGltfAccessorReader::readAsFloats(document, 0);
    QCOMPARE(positions.size(), 18);
    QCOMPARE(positions.mid(0, 3), QList<float>({ 0, 0, 0 }));
    QCOMPARE(positions.mid(6, 3), QList<float>({ 1, 2, 3 }));
    QCOMPARE(positions.mid(9, 3), QList<float>({ 0, 0, 0 }));
    QCOMPARE(positions.mid(12, 3), QList<float>({ 4, 5, 6 }));
}

void tst_qssggltfparser::readInterleavedAccessor()
{
    // Two vec3 attributes interleaved in one buffer view with byteStride 24:
    // element i of the first attribute at offset i*24, of the second at i*24+12
    QByteArray bin;
    const float data[] = { 1, 2, 3, 10, 20, 30, 4, 5, 6, 40, 50, 60 };
    bin.append(reinterpret_cast<const char *>(data), sizeof(data));

    const QByteArray json = QByteArrayLiteral(
            "{ \"asset\": { \"version\": \"2.0\" },"
            "  \"buffers\": [ { \"byteLength\": 48 } ],"
            "  \"bufferViews\": [ { \"buffer\": 0, \"byteLength\": 48, \"byteStride\": 24 } ],"
            "  \"accessors\": ["
            "    { \"bufferView\": 0, \"byteOffset\": 0, \"componentType\": 5126, \"count\": 2, \"type\": \"VEC3\" },"
            "    { \"bufferView\": 0, \"byteOffset\": 12, \"componentType\": 5126, \"count\": 2, \"type\": \"VEC3\" } ] }");

    QSSGGltfParser parser;
    QSSGGltfDocument document;
    QVERIFY2(parser.parse(makeGlb(json, bin), QString(), &document), qPrintable(parser.errorMessage()));

    QCOMPARE(QSSGGltfAccessorReader::readAsFloats(document, 0), QList<float>({ 1, 2, 3, 4, 5, 6 }));
    QCOMPARE(QSSGGltfAccessorReader::readAsFloats(document, 1), QList<float>({ 10, 20, 30, 40, 50, 60 }));
}

void tst_qssggltfparser::readNormalizedAccessor()
{
    // Normalized u8 and s16 component data (as KHR_mesh_quantization produces)
    QByteArray bin;
    const quint8 u8data[] = { 0, 51, 255, 128 };
    bin.append(reinterpret_cast<const char *>(u8data), sizeof(u8data));
    const qint16 s16data[] = { -32768, -16384, 16384, 32767 };
    bin.append(reinterpret_cast<const char *>(s16data), sizeof(s16data));

    const QByteArray json = QByteArrayLiteral(
            "{ \"asset\": { \"version\": \"2.0\" },"
            "  \"buffers\": [ { \"byteLength\": 12 } ],"
            "  \"bufferViews\": ["
            "    { \"buffer\": 0, \"byteOffset\": 0, \"byteLength\": 4 },"
            "    { \"buffer\": 0, \"byteOffset\": 4, \"byteLength\": 8 } ],"
            "  \"accessors\": ["
            "    { \"bufferView\": 0, \"componentType\": 5121, \"count\": 1, \"type\": \"VEC4\", \"normalized\": true },"
            "    { \"bufferView\": 1, \"componentType\": 5122, \"count\": 1, \"type\": \"VEC4\", \"normalized\": true },"
            "    { \"bufferView\": 0, \"componentType\": 5121, \"count\": 1, \"type\": \"VEC4\" } ] }");

    QSSGGltfParser parser;
    QSSGGltfDocument document;
    QVERIFY2(parser.parse(makeGlb(json, bin), QString(), &document), qPrintable(parser.errorMessage()));

    const QList<float> u8normalized = QSSGGltfAccessorReader::readAsFloats(document, 0);
    QCOMPARE(u8normalized.at(0), 0.0f);
    QCOMPARE(u8normalized.at(1), 51.0f / 255.0f);
    QCOMPARE(u8normalized.at(2), 1.0f);

    const QList<float> s16normalized = QSSGGltfAccessorReader::readAsFloats(document, 1);
    QCOMPARE(s16normalized.at(0), -1.0f); // clamped per spec: max(c/32767, -1)
    QCOMPARE(s16normalized.at(3), 1.0f);

    // Non-normalized integers are cast, not scaled
    const QList<float> u8plain = QSSGGltfAccessorReader::readAsFloats(document, 2);
    QCOMPARE(u8plain.at(1), 51.0f);
    QCOMPARE(u8plain.at(2), 255.0f);
}

QTEST_APPLESS_MAIN(tst_qssggltfparser)
#include "tst_qssggltfparser.moc"
