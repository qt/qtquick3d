// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QSSGGLTFDOCUMENT_P_H
#define QSSGGLTFDOCUMENT_P_H

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

#include <QtQuick3DGltf/qtquick3dgltfexports.h>

#include <QtCore/qbytearray.h>
#include <QtCore/qhash.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qlist.h>
#include <QtCore/qmath.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>
#include <QtGui/qmatrix4x4.h>
#include <QtGui/qquaternion.h>
#include <QtGui/qvector2d.h>
#include <QtGui/qvector3d.h>
#include <QtGui/qvector4d.h>

#include <optional>

QT_BEGIN_NAMESPACE

// A value-struct representation of a glTF 2.0 document, mirroring the JSON
// schema. Object references are integer indices into the document-level
// arrays, exactly as in the format; -1 means "not set". Data specific to
// supported extensions is parsed into typed members; anything else stays
// available through the raw extension objects.
namespace QSSGGltf {

struct Asset
{
    QString version;
    QString minVersion;
    QString generator;
    QString copyright;
};

struct Buffer
{
    QString uri;
    qint64 byteLength = 0;
    QByteArray data; // resolved contents (GLB BIN chunk, data: URI, or external file)
};

struct BufferView
{
    int buffer = -1;
    qint64 byteOffset = 0;
    qint64 byteLength = 0;
    int byteStride = 0; // 0 = tightly packed
    int target = 0;
    QString name;
};

struct Accessor
{
    // Values match the glTF componentType constants
    enum class ComponentType {
        Byte = 5120,
        UnsignedByte = 5121,
        Short = 5122,
        UnsignedShort = 5123,
        UnsignedInt = 5125,
        Float = 5126
    };
    enum class Type {
        Scalar,
        Vec2,
        Vec3,
        Vec4,
        Mat2,
        Mat3,
        Mat4
    };

    struct Sparse
    {
        qint64 count = 0;
        int indicesBufferView = -1;
        qint64 indicesByteOffset = 0;
        ComponentType indicesComponentType = ComponentType::UnsignedInt;
        int valuesBufferView = -1;
        qint64 valuesByteOffset = 0;
    };

    int bufferView = -1; // -1 = all zeros (unless sparse)
    qint64 byteOffset = 0;
    ComponentType componentType = ComponentType::Float;
    Type type = Type::Scalar;
    qint64 count = 0;
    bool normalized = false;
    QList<double> min;
    QList<double> max;
    std::optional<Sparse> sparse;
    QString name;

    static int componentByteSize(ComponentType componentType)
    {
        switch (componentType) {
        case ComponentType::Byte:
        case ComponentType::UnsignedByte:
            return 1;
        case ComponentType::Short:
        case ComponentType::UnsignedShort:
            return 2;
        case ComponentType::UnsignedInt:
        case ComponentType::Float:
            return 4;
        }
        return 0;
    }

    static int componentCount(Type type)
    {
        switch (type) {
        case Type::Scalar: return 1;
        case Type::Vec2: return 2;
        case Type::Vec3: return 3;
        case Type::Vec4: return 4;
        case Type::Mat2: return 4;
        case Type::Mat3: return 9;
        case Type::Mat4: return 16;
        }
        return 0;
    }

    int elementByteSize() const
    {
        return componentByteSize(componentType) * componentCount(type);
    }
};

struct Image
{
    QString uri;
    int bufferView = -1;
    QString mimeType;
    QString name;
};

struct Sampler
{
    // Values match the glTF/GL filter and wrap constants; 0 = unspecified
    enum Filter {
        Nearest = 9728,
        Linear = 9729,
        NearestMipMapNearest = 9984,
        LinearMipMapNearest = 9985,
        NearestMipMapLinear = 9986,
        LinearMipMapLinear = 9987
    };
    enum WrapMode {
        ClampToEdge = 33071,
        MirroredRepeat = 33648,
        Repeat = 10497
    };

    int magFilter = 0;
    int minFilter = 0;
    int wrapS = Repeat;
    int wrapT = Repeat;
    QString name;
};

struct Texture
{
    int sampler = -1;
    int source = -1;
    QString name;
    QJsonObject extensions;
};

struct TextureTransform // KHR_texture_transform
{
    QVector2D offset { 0.0f, 0.0f };
    QVector2D scale { 1.0f, 1.0f };
    float rotation = 0.0f;
    int texCoord = -1; // -1 = keep the TextureInfo texCoord
};

struct TextureInfo
{
    int index = -1;
    int texCoord = 0;
    // scale for normalTexture, strength for occlusionTexture; 1.0 otherwise
    float scaleOrStrength = 1.0f;
    std::optional<TextureTransform> transform;

    bool isSet() const { return index >= 0; }
};

struct Material
{
    enum class AlphaMode {
        Opaque,
        Mask,
        Blend
    };

    struct SpecularGlossiness // KHR_materials_pbrSpecularGlossiness
    {
        QVector4D diffuseFactor { 1.0f, 1.0f, 1.0f, 1.0f };
        TextureInfo diffuseTexture;
        QVector3D specularFactor { 1.0f, 1.0f, 1.0f };
        float glossinessFactor = 1.0f;
        TextureInfo specularGlossinessTexture;
    };

    struct Clearcoat // KHR_materials_clearcoat
    {
        float clearcoatFactor = 0.0f;
        TextureInfo clearcoatTexture;
        float clearcoatRoughnessFactor = 0.0f;
        TextureInfo clearcoatRoughnessTexture;
        TextureInfo clearcoatNormalTexture; // scaleOrStrength = normal scale
    };

    struct Transmission // KHR_materials_transmission
    {
        float transmissionFactor = 0.0f;
        TextureInfo transmissionTexture;
    };

    struct Volume // KHR_materials_volume
    {
        float thicknessFactor = 0.0f;
        TextureInfo thicknessTexture;
        float attenuationDistance = 0.0f; // 0 = +infinity per spec default
        QVector3D attenuationColor { 1.0f, 1.0f, 1.0f };
    };

    struct Specular // KHR_materials_specular
    {
        float specularFactor = 1.0f;
        TextureInfo specularTexture;
        QVector3D specularColorFactor { 1.0f, 1.0f, 1.0f };
        TextureInfo specularColorTexture;
    };

    QString name;

    // pbrMetallicRoughness
    QVector4D baseColorFactor { 1.0f, 1.0f, 1.0f, 1.0f };
    TextureInfo baseColorTexture;
    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;
    TextureInfo metallicRoughnessTexture;

    TextureInfo normalTexture; // scaleOrStrength = scale
    TextureInfo occlusionTexture; // scaleOrStrength = strength
    TextureInfo emissiveTexture;
    QVector3D emissiveFactor { 0.0f, 0.0f, 0.0f };

    AlphaMode alphaMode = AlphaMode::Opaque;
    float alphaCutoff = 0.5f;
    bool doubleSided = false;

    // Extensions
    bool unlit = false; // KHR_materials_unlit
    std::optional<SpecularGlossiness> specularGlossiness;
    std::optional<Clearcoat> clearcoat;
    std::optional<Transmission> transmission;
    std::optional<Volume> volume;
    std::optional<float> ior; // KHR_materials_ior
    std::optional<float> emissiveStrength; // KHR_materials_emissive_strength
    std::optional<Specular> specular;

    QJsonObject extensions; // raw, incl. unsupported ones (sheen, iridescence, ...)
};

struct MeshPrimitive
{
    // Values match the glTF primitive.mode constants
    enum Mode {
        Points = 0,
        Lines = 1,
        LineLoop = 2,
        LineStrip = 3,
        Triangles = 4,
        TriangleStrip = 5,
        TriangleFan = 6
    };

    QHash<QByteArray, int> attributes; // semantic (POSITION, NORMAL, ...) -> accessor
    int indices = -1;
    int material = -1;
    int mode = Triangles;
    QList<QHash<QByteArray, int>> targets; // morph targets
    QJsonObject extensions;
};

struct Mesh
{
    QList<MeshPrimitive> primitives;
    QList<float> weights; // default morph target weights
    QString name;
};

struct Node
{
    QList<int> children;
    int mesh = -1;
    int skin = -1;
    int camera = -1;
    int light = -1; // KHR_lights_punctual
    bool hasMatrix = false;
    QMatrix4x4 matrix;
    QVector3D translation { 0.0f, 0.0f, 0.0f };
    QQuaternion rotation;
    QVector3D scale { 1.0f, 1.0f, 1.0f };
    QList<float> weights; // overrides mesh.weights
    QString name;
    QJsonObject extensions;
};

struct Skin
{
    int inverseBindMatrices = -1; // -1 = identity matrices
    int skeleton = -1;
    QList<int> joints;
    QString name;
};

struct AnimationSampler
{
    enum class Interpolation {
        Linear,
        Step,
        CubicSpline
    };

    int input = -1; // accessor with key times (seconds)
    int output = -1; // accessor with key values
    Interpolation interpolation = Interpolation::Linear;
};

struct AnimationChannel
{
    enum class Path {
        Translation,
        Rotation,
        Scale,
        Weights
    };

    int sampler = -1;
    int targetNode = -1;
    Path path = Path::Translation;
};

struct Animation
{
    QList<AnimationSampler> samplers;
    QList<AnimationChannel> channels;
    QString name;
};

struct Camera
{
    enum class Type {
        Perspective,
        Orthographic
    };

    Type type = Type::Perspective;
    // perspective
    float aspectRatio = 0.0f; // 0 = unspecified
    float yfov = 0.0f; // radians
    // orthographic
    float xmag = 0.0f;
    float ymag = 0.0f;
    // shared
    float znear = 0.0f;
    float zfar = 0.0f; // 0 = infinite for perspective
    QString name;
};

struct Light // KHR_lights_punctual
{
    enum class Type {
        Directional,
        Point,
        Spot
    };

    Type type = Type::Directional;
    QVector3D color { 1.0f, 1.0f, 1.0f };
    float intensity = 1.0f; // candela for point/spot, lux for directional
    float range = 0.0f; // 0 = +infinity per spec default
    // spot only
    float innerConeAngle = 0.0f; // radians
    float outerConeAngle = qDegreesToRadians(45.0f);
    QString name;
};

struct Scene
{
    QList<int> nodes;
    QString name;
};

} // namespace QSSGGltf

class Q_QUICK3DGLTF_EXPORT QSSGGltfDocument
{
public:
    QSSGGltf::Asset asset;
    QList<QSSGGltf::Buffer> buffers;
    QList<QSSGGltf::BufferView> bufferViews;
    QList<QSSGGltf::Accessor> accessors;
    QList<QSSGGltf::Image> images;
    QList<QSSGGltf::Sampler> samplers;
    QList<QSSGGltf::Texture> textures;
    QList<QSSGGltf::Material> materials;
    QList<QSSGGltf::Mesh> meshes;
    QList<QSSGGltf::Node> nodes;
    QList<QSSGGltf::Skin> skins;
    QList<QSSGGltf::Animation> animations;
    QList<QSSGGltf::Camera> cameras;
    QList<QSSGGltf::Light> lights; // KHR_lights_punctual
    QList<QSSGGltf::Scene> scenes;
    int scene = -1;

    QStringList extensionsUsed;
    QStringList extensionsRequired;
    QJsonObject rootExtensions;

    // Directory the document was loaded from, used to resolve relative URIs.
    // Local filesystem or qrc (":/...") path.
    QString baseDir;
};

QT_END_NAMESPACE

#endif // QSSGGLTFDOCUMENT_P_H
