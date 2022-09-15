// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qssgqmlutilities_p.h"
#include "qssgscenedesc_p.h"

#include <QVector2D>
#include <QVector3D>
#include <QVector4D>
#include <QQuaternion>
#include <QDebug>
#include <QRegularExpression>
#include <QtCore/qdir.h>
#include <QtCore/qfile.h>
#include <QtCore/qbuffer.h>

#include <QtGui/qimage.h>
#include <QtGui/qimagereader.h>

#include <QtQuick3DUtils/private/qssgmesh_p.h>

#include <QtQuick3DRuntimeRender/private/qssgrenderbuffermanager_p.h>

#ifdef QT_QUICK3D_ENABLE_RT_ANIMATIONS
#include <QtCore/QCborStreamWriter>
#include <QtQuickTimeline/private/qquicktimeline_p.h>
#endif // QT_QUICK3D_ENABLE_RT_ANIMATIONS


QT_BEGIN_NAMESPACE

namespace QSSGQmlUtilities {

QString insertTabs(int n)
{
    QString tabs;
    for (int i = 0; i < n; ++i)
        tabs += QLatin1String("    ");
    return tabs;
}

QString qmlComponentName(const QString &name) {
    QString nameCopy = name;
    if (nameCopy.isEmpty())
        return QStringLiteral("Presentation");

    nameCopy = sanitizeQmlId(nameCopy);

    if (nameCopy[0].isLower())
        nameCopy[0] = nameCopy[0].toUpper();

    return nameCopy;
}

QString colorToQml(const QColor &color) {
    QString colorString;
    colorString = QLatin1Char('\"') + color.name(QColor::HexArgb) + QLatin1Char('\"');
    return colorString;
}

QString variantToQml(const QVariant &variant) {
    switch (variant.typeId()) {
    case QMetaType::Float: {
        auto value = variant.toDouble();
        return QString::number(value);
    }
    case QMetaType::QVector2D: {
        auto value = variant.value<QVector2D>();
        return QString(QStringLiteral("Qt.vector2d(") + QString::number(double(value.x())) +
                       QStringLiteral(", ") + QString::number(double(value.y())) +
                       QStringLiteral(")"));
    }
    case QMetaType::QVector3D: {
        auto value = variant.value<QVector3D>();
        return QString(QStringLiteral("Qt.vector3d(") + QString::number(double(value.x())) +
                       QStringLiteral(", ") + QString::number(double(value.y())) +
                       QStringLiteral(", ") + QString::number(double(value.z())) +
                       QStringLiteral(")"));
    }
    case QMetaType::QVector4D: {
        auto value = variant.value<QVector4D>();
        return QString(QStringLiteral("Qt.vector4d(") + QString::number(double(value.x())) +
                       QStringLiteral(", ") + QString::number(double(value.y())) +
                       QStringLiteral(", ") + QString::number(double(value.z())) +
                       QStringLiteral(", ") + QString::number(double(value.w())) +
                       QStringLiteral(")"));
    }
    case QMetaType::QColor: {
        auto value = variant.value<QColor>();
        return colorToQml(value);
    }
    case QMetaType::QQuaternion: {
        auto value = variant.value<QQuaternion>();
        return QString(QStringLiteral("Qt.quaternion(") + QString::number(double(value.scalar())) +
                       QStringLiteral(", ") + QString::number(double(value.x())) +
                       QStringLiteral(", ") + QString::number(double(value.y())) +
                       QStringLiteral(", ") + QString::number(double(value.z())) +
                       QStringLiteral(")"));
    }
    default:
        return variant.toString();
    }
}

QString sanitizeQmlId(const QString &id)
{
    QString idCopy = id;
    // If the id starts with a number...
    if (!idCopy.isEmpty() && idCopy.at(0).isNumber())
        idCopy.prepend(QStringLiteral("node"));

    // sometimes first letter is a # (don't replace with underscore)
    if (idCopy.startsWith(QChar::fromLatin1('#')))
        idCopy.remove(0, 1);

    // Replace all the characters other than ascii letters, numbers or underscore to underscores.
    static QRegularExpression regExp(QStringLiteral("\\W"));
    idCopy.replace(regExp, QStringLiteral("_"));

    // first letter of id can not be upper case
    if (!idCopy.isEmpty() && idCopy[0].isUpper())
        idCopy[0] = idCopy[0].toLower();

    // ### qml keywords as names
    static QSet<QByteArray> keywords {
        "x",
        "y",
        "as",
        "do",
        "if",
        "in",
        "on",
        "of",
        "for",
        "get",
        "int",
        "let",
        "new",
        "set",
        "try",
        "var",
        "top",
        "byte",
        "case",
        "char",
        "else",
        "num",
        "from",
        "goto",
        "null",
        "this",
        "true",
        "void",
        "with",
        "clip",
        "item",
        "flow",
        "font",
        "text",
        "left",
        "data",
        "alias",
        "break",
        "state",
        "scale",
        "color",
        "right",
        "catch",
        "class",
        "const",
        "false",
        "float",
        "layer", // Design Studio doesn't like "layer" as an id
        "short",
        "super",
        "throw",
        "while",
        "yield",
        "border",
        "source",
        "delete",
        "double",
        "export",
        "import",
        "native",
        "public",
        "pragma",
        "return",
        "signal",
        "static",
        "switch",
        "throws",
        "bottom",
        "parent",
        "typeof",
        "boolean",
        "opacity",
        "enabled",
        "anchors",
        "padding",
        "default",
        "extends",
        "finally",
        "package",
        "private",
        "abstract",
        "continue",
        "debugger",
        "function",
        "property",
        "readonly",
        "children",
        "volatile",
        "interface",
        "protected",
        "transient",
        "implements",
        "instanceof",
        "synchronized"
    };
    if (keywords.contains(idCopy.toUtf8())) {
        idCopy += QStringLiteral("_");
    }

    // We may have removed all the characters by now
    if (idCopy.isEmpty())
        idCopy = QStringLiteral("node");

    return idCopy;
}

QString sanitizeQmlSourcePath(const QString &source, bool removeParentDirectory)
{
    QString sourceCopy = source;

    if (removeParentDirectory)
        sourceCopy = QSSGQmlUtilities::stripParentDirectory(sourceCopy);

    sourceCopy.replace(QChar::fromLatin1('\\'), QChar::fromLatin1('/'));

    // must be surrounded in quotes
    return QString(QStringLiteral("\"") + sourceCopy + QStringLiteral("\""));
}

PropertyMap *PropertyMap::instance()
{
    static PropertyMap p;
    return &p;
}

PropertyMap::PropertiesMap *PropertyMap::propertiesForType(PropertyMap::Type type)
{
    if (m_properties.contains(type))
        return m_properties[type];

    return nullptr;
}

QVariant PropertyMap::getDefaultValue(PropertyMap::Type type, const QString &property)
{
    QVariant value;

    if (m_properties.contains(type)) {
        auto properties = m_properties[type];
        if (properties->contains(property))
            value = properties->value(property);
    }

    return value;
}

bool PropertyMap::isDefaultValue(PropertyMap::Type type, const QString &property, const QVariant &value)
{
    bool isTheSame = value == getDefaultValue(type, property);
    return isTheSame;
}

PropertyMap::PropertyMap()
{
    // Node
    PropertiesMap *node = new PropertiesMap;
    node->insert(QStringLiteral("x"), 0);
    node->insert(QStringLiteral("y"), 0);
    node->insert(QStringLiteral("z"), 0);
    node->insert(QStringLiteral("position"), QVector3D(0, 0, 0));
    node->insert(QStringLiteral("position.x"), 0);
    node->insert(QStringLiteral("position.y"), 0);
    node->insert(QStringLiteral("position.z"), 0);
    node->insert(QStringLiteral("rotation"), QQuaternion(1, 0, 0, 0));
    node->insert(QStringLiteral("eulerRotation.x"), 0);
    node->insert(QStringLiteral("eulerRotation.y"), 0);
    node->insert(QStringLiteral("eulerRotation.z"), 0);
    node->insert(QStringLiteral("eulerRotation"), QVector3D(0, 0, 0));
    node->insert(QStringLiteral("scale"), QVector3D(1, 1, 1));
    node->insert(QStringLiteral("scale.x"), 1);
    node->insert(QStringLiteral("scale.y"), 1);
    node->insert(QStringLiteral("scale.z"), 1);
    node->insert(QStringLiteral("pivot"), QVector3D(0, 0, 0));
    node->insert(QStringLiteral("pivot.x"), 0);
    node->insert(QStringLiteral("pivot.y"), 0);
    node->insert(QStringLiteral("pivot.z"), 0);
    node->insert(QStringLiteral("opacity"), 1.0);
    node->insert(QStringLiteral("visible"), true);
    m_properties.insert(Type::Node, node);

    // Model
    PropertiesMap *model = new PropertiesMap;
    m_properties.insert(Type::Model, model);

    // PerspectiveCamera
    PropertiesMap *perspectiveCamera = new PropertiesMap;
    perspectiveCamera->insert(QStringLiteral("clipNear"), 10.0f);
    perspectiveCamera->insert(QStringLiteral("clipFar"), 10000.0f);
    perspectiveCamera->insert(QStringLiteral("fieldOfView"), 60.0f);
    perspectiveCamera->insert(QStringLiteral("fieldOfViewOrientation"), QStringLiteral("PerspectiveCamera.Vertical"));
    m_properties.insert(Type::PerspectiveCamera, perspectiveCamera);

    // OrthographicCamera
    PropertiesMap *orthographicCamera = new PropertiesMap;
    orthographicCamera->insert(QStringLiteral("clipNear"), 10.0f);
    orthographicCamera->insert(QStringLiteral("clipFar"), 10000.0f);
    orthographicCamera->insert(QStringLiteral("horizontalMagnification"), 1.0f);
    orthographicCamera->insert(QStringLiteral("verticalMagnification"), 1.0f);
    m_properties.insert(Type::OrthographicCamera, orthographicCamera);

    // Directional Light
    PropertiesMap *directionalLight = new PropertiesMap;
    directionalLight->insert(QStringLiteral("color"), QColor(Qt::white));
    directionalLight->insert(QStringLiteral("ambientColor"), QColor(Qt::black));
    directionalLight->insert(QStringLiteral("brightness"), 1.0f);
    directionalLight->insert(QStringLiteral("castShadow"), false);
    directionalLight->insert(QStringLiteral("shadowBias"), 0.0f);
    directionalLight->insert(QStringLiteral("shadowFactor"), 5.0f);
    directionalLight->insert(QStringLiteral("shadowMapResolution"), 9);
    directionalLight->insert(QStringLiteral("shadowMapFar"), 5000.0f);
    directionalLight->insert(QStringLiteral("shadowFilter"), 5.0f);
    m_properties.insert(Type::DirectionalLight, directionalLight);

    // Point Light
    PropertiesMap *pointLight = new PropertiesMap;
    pointLight->insert(QStringLiteral("color"), QColor(Qt::white));
    pointLight->insert(QStringLiteral("ambientColor"), QColor(Qt::black));
    pointLight->insert(QStringLiteral("brightness"), 1.0f);
    pointLight->insert(QStringLiteral("castShadow"), false);
    pointLight->insert(QStringLiteral("shadowBias"), 0.0f);
    pointLight->insert(QStringLiteral("shadowFactor"), 5.0f);
    pointLight->insert(QStringLiteral("shadowMapResolution"), 9);
    pointLight->insert(QStringLiteral("shadowMapFar"), 5000.0f);
    pointLight->insert(QStringLiteral("shadowFilter"), 5.0f);
    pointLight->insert(QStringLiteral("constantFade"), 1.0f);
    pointLight->insert(QStringLiteral("linearFade"), 0.0f);
    pointLight->insert(QStringLiteral("quadraticFade"), 1.0f);
    m_properties.insert(Type::PointLight, pointLight);

    // Spot Light
    PropertiesMap *spotLight = new PropertiesMap;
    spotLight->insert(QStringLiteral("color"), QColor(Qt::white));
    spotLight->insert(QStringLiteral("ambientColor"), QColor(Qt::black));
    spotLight->insert(QStringLiteral("brightness"), 1.0f);
    spotLight->insert(QStringLiteral("castShadow"), false);
    spotLight->insert(QStringLiteral("shadowBias"), 0.0f);
    spotLight->insert(QStringLiteral("shadowFactor"), 5.0f);
    spotLight->insert(QStringLiteral("shadowMapResolution"), 9);
    spotLight->insert(QStringLiteral("shadowMapFar"), 5000.0f);
    spotLight->insert(QStringLiteral("shadowFilter"), 5.0f);
    spotLight->insert(QStringLiteral("constantFade"), 1.0f);
    spotLight->insert(QStringLiteral("linearFade"), 0.0f);
    spotLight->insert(QStringLiteral("quadraticFade"), 1.0f);
    spotLight->insert(QStringLiteral("coneAngle"), 40.0f);
    spotLight->insert(QStringLiteral("innerConeAngle"), 30.0f);
    m_properties.insert(Type::SpotLight, spotLight);

    // DefaultMaterial
    PropertiesMap *defaultMaterial = new PropertiesMap;
    defaultMaterial->insert(QStringLiteral("lighting"), QStringLiteral("DefaultMaterial.FragmentLighting"));
    defaultMaterial->insert(QStringLiteral("blendMode"), QStringLiteral("DefaultMaterial.SourceOver"));
    defaultMaterial->insert(QStringLiteral("diffuseColor"), QColor(Qt::white));
    defaultMaterial->insert(QStringLiteral("emissiveFactor"), QVector3D(0.0, 0.0, 0.0));
    defaultMaterial->insert(QStringLiteral("specularModel"), QStringLiteral("DefaultMaterial.Default"));
    defaultMaterial->insert(QStringLiteral("specularTint"), QColor(Qt::white));
    defaultMaterial->insert(QStringLiteral("indexOfRefraction"), 1.45f);
    defaultMaterial->insert(QStringLiteral("fresnelPower"), 0.0f);
    defaultMaterial->insert(QStringLiteral("specularAmount"), 0.0f);
    defaultMaterial->insert(QStringLiteral("specularRoughness"), 0.0f);
    defaultMaterial->insert(QStringLiteral("opacity"), 1.0f);
    defaultMaterial->insert(QStringLiteral("bumpAmount"), 0.0f);
    defaultMaterial->insert(QStringLiteral("translucentFalloff"), 0.0f);
    defaultMaterial->insert(QStringLiteral("diffuseLightWrap"), 0.0f);
    defaultMaterial->insert(QStringLiteral("vertexColorsEnabled"), false);

    m_properties.insert(Type::DefaultMaterial, defaultMaterial);

    PropertiesMap *principledMaterial = new PropertiesMap;
    principledMaterial->insert(QStringLiteral("lighting"), QStringLiteral("PrincipledMaterial.FragmentLighting"));
    principledMaterial->insert(QStringLiteral("blendMode"), QStringLiteral("PrincipledMaterial.SourceOver"));
    principledMaterial->insert(QStringLiteral("alphaMode"), QStringLiteral("PrincipledMaterial.Default"));
    principledMaterial->insert(QStringLiteral("baseColor"), QColor(Qt::white));
    principledMaterial->insert(QStringLiteral("metalness"), 0.0f);
    principledMaterial->insert(QStringLiteral("specularAmount"), 1.0f);
    principledMaterial->insert(QStringLiteral("specularTint"), 0.0f);
    principledMaterial->insert(QStringLiteral("roughness"), 0.0f);
    principledMaterial->insert(QStringLiteral("emissiveFactor"), QVector3D(0.0, 0.0, 0.0));
    principledMaterial->insert(QStringLiteral("opacity"), 1.0f);
    principledMaterial->insert(QStringLiteral("normalStrength"), 1.0f);
    principledMaterial->insert(QStringLiteral("alphaCutoff"), 0.5f);
    principledMaterial->insert(QStringLiteral("occlusionAmount"), 1.0f);
    principledMaterial->insert(QStringLiteral("clearcoatAmount"), 0.0f);
    principledMaterial->insert(QStringLiteral("clearcoatRoughnessAmount"), 0.0f);
    principledMaterial->insert(QStringLiteral("transmissionFactor"), 0.0f);
    principledMaterial->insert(QStringLiteral("thicknessFactor"), 0.0f);
    principledMaterial->insert(QStringLiteral("attenuationDistance"), std::numeric_limits<float>::infinity());
    principledMaterial->insert(QStringLiteral("attenuationColor"), QColor(Qt::white));
    principledMaterial->insert(QStringLiteral("indexOfRefraction"), 1.5f);

    m_properties.insert(Type::PrincipledMaterial, principledMaterial);

    PropertiesMap *specularGlossyMaterial = new PropertiesMap;
    specularGlossyMaterial->insert(QStringLiteral("lighting"), QStringLiteral("SpecularGlossyMaterial.FragmentLighting"));
    specularGlossyMaterial->insert(QStringLiteral("blendMode"), QStringLiteral("SpecularGlossyMaterial.SourceOver"));
    specularGlossyMaterial->insert(QStringLiteral("alphaMode"), QStringLiteral("SpecularGlossyMaterial.Default"));
    specularGlossyMaterial->insert(QStringLiteral("albedoColor"), QColor(Qt::white));
    specularGlossyMaterial->insert(QStringLiteral("specularColor"), QColor(Qt::white));
    specularGlossyMaterial->insert(QStringLiteral("glossiness"), 1.0f);
    specularGlossyMaterial->insert(QStringLiteral("emissiveFactor"), QVector3D(0.0, 0.0, 0.0));
    specularGlossyMaterial->insert(QStringLiteral("opacity"), 1.0f);
    specularGlossyMaterial->insert(QStringLiteral("normalStrength"), 1.0f);
    specularGlossyMaterial->insert(QStringLiteral("alphaCutoff"), 0.5f);
    specularGlossyMaterial->insert(QStringLiteral("occlusionAmount"), 1.0f);
    specularGlossyMaterial->insert(QStringLiteral("clearcoatAmount"), 0.0f);
    specularGlossyMaterial->insert(QStringLiteral("clearcoatRoughnessAmount"), 0.0f);
    specularGlossyMaterial->insert(QStringLiteral("transmissionFactor"), 0.0f);
    specularGlossyMaterial->insert(QStringLiteral("thicknessFactor"), 0.0f);
    specularGlossyMaterial->insert(QStringLiteral("attenuationDistance"), std::numeric_limits<float>::infinity());
    specularGlossyMaterial->insert(QStringLiteral("attenuationColor"), QColor(Qt::white));

    m_properties.insert(Type::SpecularGlossyMaterial, specularGlossyMaterial);

    // Image
    PropertiesMap *texture = new PropertiesMap;
    texture->insert(QStringLiteral("scaleU"), 1.0f);
    texture->insert(QStringLiteral("scaleV"), 1.0f);
    texture->insert(QStringLiteral("mappingMode"), QStringLiteral("Texture.UV"));
    texture->insert(QStringLiteral("tilingModeHorizontal"), QStringLiteral("Texture.Repeat"));
    texture->insert(QStringLiteral("tilingModeVertical"), QStringLiteral("Texture.Repeat"));
    texture->insert(QStringLiteral("rotationUV"), 0.0f);
    texture->insert(QStringLiteral("positionU"), 0.0f);
    texture->insert(QStringLiteral("positionV"), 0.0f);
    texture->insert(QStringLiteral("pivotU"), 0.0f);
    texture->insert(QStringLiteral("pivotV"), 0.0f);
    texture->insert(QStringLiteral("indexUV"), 0);
    texture->insert(QStringLiteral("magFilter"), QStringLiteral("Texture.Linear"));
    texture->insert(QStringLiteral("minFilter"), QStringLiteral("Texture.Linear"));
    texture->insert(QStringLiteral("mipFilter"), QStringLiteral("Texture.None"));
    texture->insert(QStringLiteral("generateMipmaps"), false);
    m_properties.insert(Type::Texture, texture);
}

PropertyMap::~PropertyMap()
{
    for (const auto &proprtyMap : qAsConst(m_properties))
        delete proprtyMap;
}

struct OutputContext
{
    enum Type : quint8 { Header, RootNode, NodeTree, Resource };
    QTextStream &stream;
    QDir outdir;
    quint8 indent = 0;
    Type type = NodeTree;
    quint16 scopeDepth = 0;
};

template<QSSGSceneDesc::Material::RuntimeType T>
const char *qmlElementName() { static_assert(!std::is_same_v<decltype(T), decltype(T)>, "Unknown type"); return nullptr; }
template<> const char *qmlElementName<QSSGSceneDesc::Node::RuntimeType::Node>() { return "Node"; }

template<> const char *qmlElementName<QSSGSceneDesc::Material::RuntimeType::DefaultMaterial>() { return "DefaultMaterial"; }
template<> const char *qmlElementName<QSSGSceneDesc::Material::RuntimeType::PrincipledMaterial>() { return "PrincipledMaterial"; }
template<> const char *qmlElementName<QSSGSceneDesc::Material::RuntimeType::CustomMaterial>() { return "CustomMaterial"; }
template<> const char *qmlElementName<QSSGSceneDesc::Material::RuntimeType::SpecularGlossyMaterial>() { return "SpecularGlossyMaterial"; }
template<> const char *qmlElementName<QSSGSceneDesc::Material::RuntimeType::OrthographicCamera>() { return "OrthographicCamera"; }
template<> const char *qmlElementName<QSSGSceneDesc::Material::RuntimeType::PerspectiveCamera>() { return "PerspectiveCamera"; }

template<> const char *qmlElementName<QSSGSceneDesc::Node::RuntimeType::Model>() { return "Model"; }

template<> const char *qmlElementName<QSSGSceneDesc::Texture::RuntimeType::Image2D>() { return "Texture"; }
template<> const char *qmlElementName<QSSGSceneDesc::Texture::RuntimeType::ImageCube>() { return "CubeMapTexture"; }
template<> const char *qmlElementName<QSSGSceneDesc::Texture::RuntimeType::TextureData>() { return "TextureData"; }

template<> const char *qmlElementName<QSSGSceneDesc::Camera::RuntimeType::DirectionalLight>() { return "DirectionalLight"; }
template<> const char *qmlElementName<QSSGSceneDesc::Camera::RuntimeType::SpotLight>() { return "SpotLight"; }
template<> const char *qmlElementName<QSSGSceneDesc::Camera::RuntimeType::PointLight>() { return "PointLight"; }

template<> const char *qmlElementName<QSSGSceneDesc::Joint::RuntimeType::Joint>() { return "Joint"; }
template<> const char *qmlElementName<QSSGSceneDesc::Skeleton::RuntimeType::Skeleton>() { return "Skeleton"; }
template<> const char *qmlElementName<QSSGSceneDesc::Node::RuntimeType::Skin>() { return "Skin"; }
template<> const char *qmlElementName<QSSGSceneDesc::Node::RuntimeType::MorphTarget>() { return "MorphTarget"; }

static const char *getQmlElementName(const QSSGSceneDesc::Node &node)
{
    using RuntimeType = QSSGSceneDesc::Node::RuntimeType;
    switch (node.runtimeType) {
    case RuntimeType::Node:
        return qmlElementName<RuntimeType::Node>();
    case RuntimeType::PrincipledMaterial:
        return qmlElementName<RuntimeType::PrincipledMaterial>();
    case RuntimeType::DefaultMaterial:
        return qmlElementName<RuntimeType::DefaultMaterial>();
    case RuntimeType::CustomMaterial:
        return qmlElementName<RuntimeType::CustomMaterial>();
    case RuntimeType::SpecularGlossyMaterial:
        return qmlElementName<RuntimeType::SpecularGlossyMaterial>();
    case RuntimeType::Image2D:
        return qmlElementName<RuntimeType::Image2D>();
    case RuntimeType::ImageCube:
        return qmlElementName<RuntimeType::ImageCube>();
    case RuntimeType::TextureData:
        return qmlElementName<RuntimeType::TextureData>();
    case RuntimeType::Model:
        return qmlElementName<RuntimeType::Model>();
    case RuntimeType::OrthographicCamera:
        return qmlElementName<RuntimeType::OrthographicCamera>();
    case RuntimeType::PerspectiveCamera:
        return qmlElementName<RuntimeType::PerspectiveCamera>();
    case RuntimeType::DirectionalLight:
        return qmlElementName<RuntimeType::DirectionalLight>();
    case RuntimeType::PointLight:
        return qmlElementName<RuntimeType::PointLight>();
    case RuntimeType::SpotLight:
        return qmlElementName<RuntimeType::SpotLight>();
    case RuntimeType::Skeleton:
        return qmlElementName<RuntimeType::Skeleton>();
    case RuntimeType::Joint:
        return qmlElementName<RuntimeType::Joint>();
    case RuntimeType::Skin:
        return qmlElementName<RuntimeType::Skin>();
    case RuntimeType::MorphTarget:
        return qmlElementName<RuntimeType::MorphTarget>();
    default:
        return "UNKNOWN_TYPE";
    }
}

enum QMLBasicType
{
    Bool,
    Dobule,
    Int,
    List,
    Real,
    String,
    Url,
    Var,
    Color,
    Date,
    Font,
    Mat44,
    Point,
    Quaternion,
    Rect,
    Size,
    Vector2D,
    Vector3D,
    Vector4D,
    Unknown_Count
};

static constexpr QByteArrayView qml_basic_types[] {
    "bool",
    "double",
    "int",
    "list",
    "real",
    "string",
    "url",
    "var",
    "color",
    "date",
    "font",
    "matrix4x4",
    "point",
    "quaternion",
    "rect",
    "size",
    "vector2d",
    "vector3d",
    "vector4d"
};

static_assert(std::size(qml_basic_types) == QMLBasicType::Unknown_Count, "Missing type?");

static QByteArrayView typeName(QMetaType mt)
{
    switch (mt.id()) {
    case QMetaType::Bool:
        return qml_basic_types[QMLBasicType::Bool];
    case QMetaType::Char:
    case QMetaType::SChar:
    case QMetaType::UChar:
    case QMetaType::Char16:
    case QMetaType::Char32:
    case QMetaType::QChar:
    case QMetaType::Short:
    case QMetaType::UShort:
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::Long:
    case QMetaType::ULong:
    case QMetaType::LongLong:
    case QMetaType::ULongLong:
        return qml_basic_types[QMLBasicType::Int];
    case QMetaType::Float:
    case QMetaType::Double:
        return qml_basic_types[QMLBasicType::Real];
    case QMetaType::QByteArray:
    case QMetaType::QString:
        return qml_basic_types[QMLBasicType::String];
    case QMetaType::QDate:
    case QMetaType::QTime:
    case QMetaType::QDateTime:
        return qml_basic_types[QMLBasicType::Date];
    case QMetaType::QUrl:
        return qml_basic_types[QMLBasicType::Url];
    case QMetaType::QRect:
    case QMetaType::QRectF:
        return qml_basic_types[QMLBasicType::Rect];
    case QMetaType::QSize:
    case QMetaType::QSizeF:
        return qml_basic_types[QMLBasicType::Size];
    case QMetaType::QPoint:
    case QMetaType::QPointF:
        return qml_basic_types[QMLBasicType::Point];
    case QMetaType::QVariant:
        return qml_basic_types[QMLBasicType::Var];
    case QMetaType::QColor:
        return qml_basic_types[QMLBasicType::Color];
    case QMetaType::QMatrix4x4:
        return qml_basic_types[QMLBasicType::Mat44];
    case QMetaType::QVector2D:
        return qml_basic_types[QMLBasicType::Vector2D];
    case QMetaType::QVector3D:
        return qml_basic_types[QMLBasicType::Vector3D];
    case QMetaType::QVector4D:
        return qml_basic_types[QMLBasicType::Vector4D];
    case QMetaType::QQuaternion:
        return qml_basic_types[QMLBasicType::Quaternion];
    case QMetaType::QFont:
        return qml_basic_types[QMLBasicType::Font];
    default:
        return qml_basic_types[QMLBasicType::Var];
    }
}

using NodeNameMap = QHash<const QSSGSceneDesc::Node *, QString>;
Q_GLOBAL_STATIC(NodeNameMap, g_nodeNameMap)
using UniqueIdMap = QHash<QString, const QSSGSceneDesc::Node *>;
Q_GLOBAL_STATIC(UniqueIdMap, g_idMap)

static QString getIdForNode(const QSSGSceneDesc::Node &node)
{
    const bool nodeHasName = (node.name.size() > 0);
    QString name = nodeHasName ? QString::fromUtf8(node.name) : QString::fromLatin1(getQmlElementName(node));
    QString sanitizedName = QSSGQmlUtilities::sanitizeQmlId(name);

    // Make sure we return a unique id.
    if (const auto it = g_nodeNameMap->constFind(&node); it != g_nodeNameMap->constEnd())
        return *it;

    quint64 id = node.id;
    int attempts = 1000;
    do {
        if (const auto it = g_idMap->constFind(sanitizedName); it == g_idMap->constEnd()) {
            g_idMap->insert(sanitizedName, &node);
            g_nodeNameMap->insert(&node, sanitizedName);
            return sanitizedName;
        }

        sanitizedName = QStringLiteral("%1%2").arg(sanitizedName).arg(id++);
    } while (--attempts);

    return sanitizedName;
}

void writeQmlPropertyHelper(QTextStream &output, int tabLevel, PropertyMap::Type type, const QString &propertyName, const QVariant &value)
{
    if (!PropertyMap::instance()->propertiesForType(type)->contains(propertyName)) {
        qWarning() << "property: " << propertyName << " not found";
        return;
    }

    auto defaultValue = PropertyMap::instance()->propertiesForType(type)->value(propertyName);

    if ((defaultValue != value)) {
        QString valueString = QSSGQmlUtilities::variantToQml(value);
        output << QSSGQmlUtilities::insertTabs(tabLevel) << propertyName << ": " << valueString << Qt::endl;
    }

}

QString stripParentDirectory(const QString &filePath) {
    QString sourceCopy = filePath;
    while (sourceCopy.startsWith(QChar::fromLatin1('.')) || sourceCopy.startsWith(QChar::fromLatin1('/')) || sourceCopy.startsWith(QChar::fromLatin1('\\')))
        sourceCopy.remove(0, 1);
    return sourceCopy;
}

static const char *blockBegin() { return " {\n"; }
static const char *blockEnd() { return "}\n"; }
static const char *comment() { return "// "; }
static const char *indent() { return "    "; }

struct QSSGQmlScopedIndent
{
    enum : quint8 { QSSG_INDENT = 4 };
    explicit QSSGQmlScopedIndent(OutputContext &out) : output(out) { out.indent += QSSG_INDENT; };
    ~QSSGQmlScopedIndent() { output.indent = qMax(output.indent - QSSG_INDENT, 0); }
    OutputContext &output;
};

static QString indentString(OutputContext &output)
{
    QString str;
    for (quint8 i = 0; i < output.indent; i += QSSGQmlScopedIndent::QSSG_INDENT)
        str += QString::fromLatin1(indent());
    return str;
}

static QTextStream &indent(OutputContext &output)
{
    for (quint8 i = 0; i < output.indent; i += QSSGQmlScopedIndent::QSSG_INDENT)
        output.stream << indent();
    return output.stream;
}

static const char *blockBegin(OutputContext &output)
{
    ++output.scopeDepth;
    return blockBegin();
}

static const char *blockEnd(OutputContext &output)
{
    output.scopeDepth = qMax(0, output.scopeDepth - 1);
    return blockEnd();
}

static void writeImportHeader(OutputContext &output, bool hasAnimation = false)
{
    output.stream << "import QtQuick\n"
                  << "import QtQuick3D\n\n";
    if (hasAnimation)
        output.stream << "import QtQuick.Timeline\n\n";
}

static QString toQuotedString(const QString &text) { return QStringLiteral("\"%1\"").arg(text); }

static inline QString getMeshFolder() { return QStringLiteral("meshes/"); }
static inline QString getMeshExtension() { return QStringLiteral(".mesh"); }

QString getMeshSourceName(const QByteArrayView &name)
{
    const auto meshFolder = getMeshFolder();
    const auto extension = getMeshExtension();

    const auto sanitizedName = QSSGQmlUtilities::sanitizeQmlId(QString::fromUtf8(name));
    return QString(meshFolder + sanitizedName +  extension);
}

static inline QString getTextureFolder() { return QStringLiteral("maps/"); }

static inline QString getAnimationFolder() { return QStringLiteral("animations/"); }
static inline QString getAnimationExtension() { return QStringLiteral(".qad"); }
QString getAnimationSourceName(const QString &id, const QString &property, qsizetype index)
{
    const auto animationFolder = getAnimationFolder();
    const auto extension = getAnimationExtension();
    return QString(animationFolder + id + QStringLiteral("_")
                        + property + QStringLiteral("_")
                        + QString::number(index) + extension);
}

QString asString(const QSSGSceneDesc::Value &value)
{
    QString str;
    QMetaType::convert(value.mt, value.dptr, QMetaType::fromType<QString>(), &str);
    return str;
}

QString builtinQmlType(const QSSGSceneDesc::Value &value)
{
    switch (value.mt.id()) {
    case QMetaType::QVector2D: {
        const auto &vec2 = *reinterpret_cast<QVector2D *>(value.dptr);
        return QLatin1String("Qt.vector2d(") + QString::number(vec2.x()) + QLatin1String(", ") + QString::number(vec2.y()) + QLatin1Char(')');
    }
    case QMetaType::QVector3D: {
        const auto &vec3 = *reinterpret_cast<QVector3D *>(value.dptr);
        return QLatin1String("Qt.vector3d(") + QString::number(vec3.x()) + QLatin1String(", ")
                + QString::number(vec3.y()) + QLatin1String(", ")
                + QString::number(vec3.z()) + QLatin1Char(')');
    }
    case QMetaType::QVector4D: {
        const auto &vec4 = *reinterpret_cast<QVector4D *>(value.dptr);
        return QLatin1String("Qt.vector4d(") + QString::number(vec4.x()) + QLatin1String(", ")
                + QString::number(vec4.y()) + QLatin1String(", ")
                + QString::number(vec4.z()) + QLatin1String(", ")
                + QString::number(vec4.w()) + QLatin1Char(')');
    }
    case QMetaType::QColor: {
        const auto &color = *reinterpret_cast<QColor *>(value.dptr);
        return colorToQml(color);
    }
    case QMetaType::QQuaternion: {
        const auto &quat = *reinterpret_cast<QQuaternion *>(value.dptr);
        return QLatin1String("Qt.quaternion(") + QString::number(quat.scalar()) + QLatin1String(", ")
                + QString::number(quat.x()) + QLatin1String(", ")
                + QString::number(quat.y()) + QLatin1String(", ")
                + QString::number(quat.z()) + QLatin1Char(')');
    }
    case QMetaType::QMatrix4x4: {
        const auto &mat44 = *reinterpret_cast<QMatrix4x4 *>(value.dptr);
        return QLatin1String("Qt.matrix4x4(")
                + QString::number(mat44(0, 0)) + u", " + QString::number(mat44(0, 1)) + u", " + QString::number(mat44(0, 2)) + u", " + QString::number(mat44(0, 3)) + u", "
                + QString::number(mat44(1, 0)) + u", " + QString::number(mat44(1, 1)) + u", " + QString::number(mat44(1, 2)) + u", " + QString::number(mat44(1, 3)) + u", "
                + QString::number(mat44(2, 0)) + u", " + QString::number(mat44(2, 1)) + u", " + QString::number(mat44(2, 2)) + u", " + QString::number(mat44(2, 3)) + u", "
                + QString::number(mat44(3, 0)) + u", " + QString::number(mat44(3, 1)) + u", " + QString::number(mat44(3, 2)) + u", " + QString::number(mat44(3, 3)) + u')';
    }
    case QMetaType::QUrl:
        return QString(QLatin1String("\"%1\"")).arg(asString(value));
    case QMetaType::Float:
    case QMetaType::Double:
    case QMetaType::Int:
    case QMetaType::Char:
    case QMetaType::Long:
    case QMetaType::LongLong:
    case QMetaType::ULong:
    case QMetaType::ULongLong:
        Q_FALLTHROUGH();
    case QMetaType::Bool:
        return asString(value);
    default:
        break;
    }

    return QString();
}

QString asString(QSSGSceneDesc::Animation::Channel::TargetProperty prop)
{
    if (prop == QSSGSceneDesc::Animation::Channel::TargetProperty::Position)
        return QStringLiteral("position");
    if (prop == QSSGSceneDesc::Animation::Channel::TargetProperty::Rotation)
        return QStringLiteral("rotation");
    if (prop == QSSGSceneDesc::Animation::Channel::TargetProperty::Scale)
        return QStringLiteral("scale");
    if (prop == QSSGSceneDesc::Animation::Channel::TargetProperty::Weight)
        return QStringLiteral("weight");

    return QStringLiteral("unknown");
}



using PropertyPair = std::pair<const char * /* name */, QString /* value */>;

static PropertyPair valueToQml(const QSSGSceneDesc::Node &target, const QSSGSceneDesc::Property &property, OutputContext &output, bool *ok = nullptr)
{
    using namespace QSSGSceneDesc;
    using RuntimeType = Node::RuntimeType;

    const auto &value = property.value;

    if (value.dptr) {

        if (ok)
            *ok = true;

        // Built-in types
        {
            QString valueAsString = builtinQmlType(value);
            if (valueAsString.size() > 0)
                return { property.name, valueAsString };
        }

        // Enumerations
        if (value.mt.flags() & (QMetaType::IsEnumeration | QMetaType::IsUnsignedEnumeration)) {
            static const auto qmlEnumString = [](const QLatin1String &element, const QString &enumString) {
                return QStringLiteral("%1.%2").arg(element).arg(enumString);
            };
            QLatin1String qmlElementName(getQmlElementName(target));
            QString enumValue = asString(value);
            if (enumValue.size() > 0)
                return { property.name, qmlEnumString(qmlElementName, enumValue) };
        }

        if (value.mt.id() == qMetaTypeId<QSSGSceneDesc::Flag>()) {
            QByteArray element(getQmlElementName(target));
            if (element.size() > 0) {
                const auto &flag = *reinterpret_cast<QSSGSceneDesc::Flag *>(value.dptr);
                QByteArray keysString = flag.me.valueToKeys(int(flag.value));
                if (keysString.size() > 0) {
                    keysString.prepend(element + '.');
                    QByteArray replacement(" | " + element + '.');
                    keysString.replace('|', replacement);
                    return { property.name, QString::fromLatin1(keysString) };
                }
            }
        }

        if (value.mt.id() == qMetaTypeId<QSSGSceneDesc::NodeList *>()) {
            const auto &list = *reinterpret_cast<QSSGSceneDesc::NodeList *>(value.dptr);
            if (list.count > 0) {
                const bool useBrackets = (list.count > 1);

                const QString indentStr = indentString(output);
                QSSGQmlScopedIndent scopedIndent(output);
                const QString listIndentStr = indentString(output);

                QString str;
                if (useBrackets)
                    str.append(u"[\n");

                for (int i = 0, end = list.count; i != end; ++i) {
                    if (i != 0)
                        str.append(u",\n");
                    if (useBrackets)
                        str.append(listIndentStr);
                    str.append(getIdForNode(*(list.head[i])));
                }

                if (useBrackets)
                    str.append(u'\n' + indentStr + u']');

                return { property.name, str };
            }
        }

        if (value.mt.id() == qMetaTypeId<QSSGSceneDesc::ListView>()) {
            const auto &list = *reinterpret_cast<QSSGSceneDesc::ListView *>(value.dptr);
            if (list.count > 0) {
                const bool useBrackets = (list.count > 1);

                const QString indentStr = indentString(output);
                QSSGQmlScopedIndent scopedIndent(output);
                const QString listIndentStr = indentString(output);

                QString str;
                if (useBrackets)
                    str.append(u"[\n");

                char *vptr = reinterpret_cast<char *>(list.head.dptr);
                auto size = list.head.mt.sizeOf();

                for (int i = 0, end = list.count; i != end; ++i) {
                    if (i != 0)
                        str.append(u",\n");

                    QSSGSceneDesc::Value v{list.head.mt, reinterpret_cast<void *>(vptr + (size * i))};
                    QString valueString = builtinQmlType(v);
                    if (valueString.isEmpty())
                        valueString = asString(v);

                    if (useBrackets)
                        str.append(listIndentStr);
                    str.append(valueString);
                }

                if (useBrackets)
                    str.append(u'\n' + indentStr + u']');

                return { property.name, str };
            }
        }

        if (value.mt.id() == qMetaTypeId<QSSGSceneDesc::Node *>()) {
            if (const auto node = reinterpret_cast<QSSGSceneDesc::Node *>(value.dptr)) {
                // If this assert is triggerd it likely means that the node never got added
                // to the scene tree (see: addNode()) or that it's a type not handled as a resource, see:
                // writeQmlForResources()
                Q_ASSERT(node->id != 0);
                // The 'TextureData' node will have its data written out and become
                // a source url.
                if (node->runtimeType == RuntimeType::TextureData)
                    return { "source", getIdForNode(*node->scene->root) + QLatin1Char('.') + getIdForNode(*node) };

                return { property.name, getIdForNode(*node) };
            }
        }

        if (value.mt == QMetaType::fromType<QSSGSceneDesc::Mesh>()) {
            //
            static const auto outputMeshAsset = [](const QSSGSceneDesc::Scene &scene, const QSSGSceneDesc::Mesh &meshNode, const QDir &outdir) {
                const auto meshFolder = getMeshFolder();
                const auto meshSourceName = QSSGQmlUtilities::getMeshSourceName(meshNode.name);
                Q_ASSERT(scene.meshStorage.size() > meshNode.idx);
                const auto &mesh = scene.meshStorage.at(meshNode.idx);

                // If a mesh folder does not exist, then create one
                if (!outdir.exists(meshFolder) && !outdir.mkdir(meshFolder))
                    return QString(); // Error out

                QFile file(outdir.path() + QDir::separator() + meshSourceName);
                if (!file.open(QIODevice::WriteOnly))
                    return QString();

                if (mesh.save(&file) == 0)
                    return QString();

                return meshSourceName;
            };

            if (const auto meshNode = reinterpret_cast<const Mesh *>(value.dptr)) {
                Q_ASSERT(meshNode->nodeType == Node::Type::Mesh);
                Q_ASSERT(meshNode->scene);

                const auto &scene = *meshNode->scene;
                const auto meshSourceName = outputMeshAsset(scene, *meshNode, output.outdir);
                return { property.name, toQuotedString(meshSourceName) };
            }
        }

        if (value.mt == QMetaType::fromType<QSSGSceneDesc::UrlView>()) {
            //
            static const auto copyTextureAsset = [](const QByteArrayView &texturePath, const QDir &outdir) {
                const auto assetPath = QString::fromUtf8(texturePath);
                QFileInfo fi(assetPath);
                if (!fi.exists())
                    return assetPath;

                const auto mapsFolder = getTextureFolder();

                // If a maps folder does not exist, then create one
                if (!outdir.exists(mapsFolder) && !outdir.mkdir(mapsFolder))
                    return QString(); // Error out

                const QString relpath = mapsFolder + fi.fileName();
                const auto newfilepath = QString(outdir.canonicalPath() + QDir::separator() + relpath);
                if (!QFile::exists(newfilepath) && !QFile::copy(fi.canonicalFilePath(), newfilepath))
                    return QString();

                return relpath;
            };

            if (const auto urlView = reinterpret_cast<const UrlView *>(value.dptr)) {
                // We need to adjust source url(s) as those should contain the canonical path
                const auto &path = urlView->view;
                if (QSSGRenderGraphObject::isTexture(target.runtimeType)) {
                    const auto sourcePath = copyTextureAsset(path, output.outdir);
                    return { property.name, toQuotedString(sourcePath) };
                }

                return { property.name, toQuotedString(QString::fromUtf8(path)) };
            }
        }

        // Workaround the TextureInput item that wraps textures for the Custom material.
        if (target.runtimeType == QSSGSceneDesc::Material::RuntimeType::CustomMaterial) {
            if (value.mt.id() == qMetaTypeId<QSSGSceneDesc::Texture *>()) {
                if (const auto texture = reinterpret_cast<QSSGSceneDesc::Texture *>(value.dptr)) {
                    Q_ASSERT(QSSGRenderGraphObject::isTexture(texture->runtimeType));
                    return { property.name, QLatin1String("TextureInput { texture: ") +
                                getIdForNode(*texture) + QLatin1String(" }") };
                }
            }
        }
    }

    if (ok)
        *ok = false;

    return PropertyPair();
}

static void writeNodeProperties(const QSSGSceneDesc::Node &node, OutputContext &output)
{
    using namespace QSSGSceneDesc;

    QSSGQmlScopedIndent scopedIndent(output);

    indent(output) << "id: " << getIdForNode(node) << "\n";

    const auto &properties = node.properties;
    auto it = properties.begin();
    const auto end = properties.end();
    bool ok = false;
    for (; it != end; ++it) {
        const auto &[name, value] = valueToQml(node, (*it), output, &ok);
        if (it->type != Property::Type::Dynamic) {
            if (!ok)
                indent(output) << comment();
            indent(output) << name << ": " << value << "\n";
        } else if (ok && it->type == Property::Type::Dynamic) {
            indent(output) << "property " << typeName(it->value.mt).toByteArray() << ' ' << name << ": " << value << "\n";
        }
    }
}

static void writeQml(const QSSGSceneDesc::Node &transform, OutputContext &output)
{
    using namespace QSSGSceneDesc;
    Q_ASSERT(transform.nodeType == QSSGSceneDesc::Node::Type::Transform && transform.runtimeType == QSSGSceneDesc::Node::RuntimeType::Node);
    indent(output) << qmlElementName<QSSGSceneDesc::Node::RuntimeType::Node>() << blockBegin(output);
    writeNodeProperties(transform, output);
}

void writeQml(const QSSGSceneDesc::Material &material, OutputContext &output)
{
    using namespace QSSGSceneDesc;
    Q_ASSERT(material.nodeType == QSSGSceneDesc::Model::Type::Material);
    if (material.runtimeType == QSSGSceneDesc::Model::RuntimeType::DefaultMaterial) {
        indent(output) << qmlElementName<Material::RuntimeType::DefaultMaterial>() << blockBegin(output);
    } else if (material.runtimeType == Model::RuntimeType::PrincipledMaterial) {
        indent(output) << qmlElementName<Material::RuntimeType::PrincipledMaterial>() << blockBegin(output);
    } else if (material.runtimeType == Material::RuntimeType::CustomMaterial) {
        indent(output) << qmlElementName<Material::RuntimeType::CustomMaterial>() << blockBegin(output);
    } else if (material.runtimeType == Material::RuntimeType::SpecularGlossyMaterial) {
        indent(output) << qmlElementName<Material::RuntimeType::SpecularGlossyMaterial>() << blockBegin(output);
    } else {
        Q_UNREACHABLE();
    }

    writeNodeProperties(material, output);
}

static void writeQml(const QSSGSceneDesc::Model &model, OutputContext &output)
{
    using namespace QSSGSceneDesc;
    Q_ASSERT(model.nodeType == Node::Type::Model);
    indent(output) << qmlElementName<QSSGSceneDesc::Node::RuntimeType::Model>() << blockBegin(output);
    writeNodeProperties(model, output);
}

static void writeQml(const QSSGSceneDesc::Camera &camera, OutputContext &output)
{
    using namespace QSSGSceneDesc;
    Q_ASSERT(camera.nodeType == Node::Type::Camera);
    if (camera.runtimeType == Camera::RuntimeType::PerspectiveCamera)
        indent(output) << qmlElementName<Camera::RuntimeType::PerspectiveCamera>() << blockBegin(output);
    else if (camera.runtimeType == Camera::RuntimeType::OrthographicCamera)
        indent(output) << qmlElementName<Camera::RuntimeType::OrthographicCamera>() << blockBegin(output);
    else
        Q_UNREACHABLE();
    writeNodeProperties(camera, output);
}

static void writeQml(const QSSGSceneDesc::Texture &texture, OutputContext &output)
{
    using namespace QSSGSceneDesc;
    Q_ASSERT(texture.nodeType == Node::Type::Texture && QSSGRenderGraphObject::isTexture(texture.runtimeType));
    if (texture.runtimeType == Texture::RuntimeType::Image2D)
        indent(output) << qmlElementName<Texture::RuntimeType::Image2D>() << blockBegin(output);
    else if (texture.runtimeType == Texture::RuntimeType::ImageCube)
        indent(output) << qmlElementName<Texture::RuntimeType::ImageCube>() << blockBegin(output);
    writeNodeProperties(texture, output);
}

static void writeQml(const QSSGSceneDesc::Skin &skin, OutputContext &output)
{
    using namespace QSSGSceneDesc;
    Q_ASSERT(skin.nodeType == Node::Type::Skin && skin.runtimeType == Node::RuntimeType::Skin);
    indent(output) << qmlElementName<Node::RuntimeType::Skin>() << blockBegin(output);
    writeNodeProperties(skin, output);
}

static void writeQml(const QSSGSceneDesc::MorphTarget &morphTarget, OutputContext &output)
{
    using namespace QSSGSceneDesc;
    Q_ASSERT(morphTarget.nodeType == Node::Type::MorphTarget);
    indent(output) << qmlElementName<QSSGSceneDesc::Node::RuntimeType::MorphTarget>() << blockBegin(output);
    writeNodeProperties(morphTarget, output);
}

QString getTextureSourceName(const QString &name)
{
    const auto textureFolder = getTextureFolder();

    const auto sanitizedName = QSSGQmlUtilities::sanitizeQmlId(name);
    return QString(textureFolder + sanitizedName +  QLatin1String(".png"));
}

static QString outputTextureAsset(const QString &textureSourceName, const QImage &image, const QDir &outdir)
{
    const auto mapsFolder = getTextureFolder();

    // If a maps folder does not exist, then create one
    if (!outdir.exists(mapsFolder) && !outdir.mkdir(mapsFolder))
        return QString(); // Error out

    const auto imagePath = QString(outdir.path() + QDir::separator() + textureSourceName);

    if (!image.save(imagePath))
        return QString();

    return textureSourceName;
}

static void writeQml(const QSSGSceneDesc::TextureData &textureData, OutputContext &output)
{
    using namespace QSSGSceneDesc;
    Q_ASSERT(textureData.nodeType == Node::Type::Texture && textureData.runtimeType == Node::RuntimeType::TextureData);

    const auto &texData = textureData.data;
    const auto &size = textureData.sz;
    const bool isCompressed = ((textureData.flgs & quint8(TextureData::Flags::Compressed)) != 0);

    const auto id = getIdForNode(textureData);
    QString textureSourcePath = getTextureSourceName(id);

    if (!texData.isEmpty()) {
        QImage image;
        if (isCompressed) {
            QByteArray data = texData.toByteArray();
            QBuffer readBuffer(&data);
            QImageReader imageReader(&readBuffer);
            image = imageReader.read();
            if (image.isNull())
                qWarning() << imageReader.errorString();
        } else {
            image = QImage(reinterpret_cast<const uchar *>(texData.data()), size.width(), size.height(), QImage::Format::Format_RGBA8888);
        }

        if (!image.isNull())
            textureSourcePath = outputTextureAsset(textureSourcePath, image, output.outdir);
    }

    static const auto writeProperty = [](const QString &type, const QString &name, const QString &value) {
        return QString::fromLatin1("property %1 %2: %3").arg(type, name, value);
    };

    const auto type = QLatin1String("url");
    const auto &name = id;

    indent(output) << writeProperty(type, name, toQuotedString(textureSourcePath)) << '\n';
}

static void writeQml(const QSSGSceneDesc::Light &light, OutputContext &output)
{
    using namespace QSSGSceneDesc;
    Q_ASSERT(light.nodeType == Node::Type::Light);
    if (light.runtimeType == Light::RuntimeType::DirectionalLight)
        indent(output) << qmlElementName<Light::RuntimeType::DirectionalLight>() << blockBegin(output);
    else if (light.runtimeType == Light::RuntimeType::SpotLight)
        indent(output) << qmlElementName<Light::RuntimeType::SpotLight>() << blockBegin(output);
    else if (light.runtimeType == Light::RuntimeType::PointLight)
        indent(output) << qmlElementName<Light::RuntimeType::PointLight>() << blockBegin(output);
    else
        Q_UNREACHABLE();
    writeNodeProperties(light, output);
}

static void writeQml(const QSSGSceneDesc::Skeleton &skeleton, OutputContext &output)
{
    using namespace QSSGSceneDesc;
    Q_ASSERT(skeleton.nodeType == Node::Type::Skeleton && skeleton.runtimeType == Node::RuntimeType::Skeleton);
    indent(output) << qmlElementName<Node::RuntimeType::Skeleton>() << blockBegin(output);
    writeNodeProperties(skeleton, output);
}

static void writeQml(const QSSGSceneDesc::Joint &joint, OutputContext &output)
{
    using namespace QSSGSceneDesc;
    Q_ASSERT(joint.nodeType == Node::Type::Joint && joint.runtimeType == Node::RuntimeType::Joint);
    indent(output) << qmlElementName<Node::RuntimeType::Joint>() << blockBegin(output);
    writeNodeProperties(joint, output);
}

static void writeQmlForResourceNode(const QSSGSceneDesc::Node &node, OutputContext &output)
{
    using namespace QSSGSceneDesc;
    Q_ASSERT(output.type == OutputContext::Resource);
    Q_ASSERT(QSSGRenderGraphObject::isResource(node.runtimeType) || node.nodeType == Node::Type::Mesh || node.nodeType == Node::Type::Skeleton);

    const bool processNode = !node.properties.isEmpty() || (output.type == OutputContext::Resource);
    if (processNode) {
        QSSGQmlScopedIndent scopedIndent(output);
        switch (node.nodeType) {
        case Node::Type::Skin:
            writeQml(static_cast<const Skin &>(node), output);
            break;
        case Node::Type::MorphTarget:
            writeQml(static_cast<const MorphTarget &>(node), output);
            break;
        case Node::Type::Skeleton:
            writeQml(static_cast<const Skeleton &>(node), output);
            break;
        case Node::Type::Texture:
            if (node.runtimeType == Node::RuntimeType::Image2D)
                writeQml(static_cast<const Texture &>(node), output);
            else if (node.runtimeType == Node::RuntimeType::ImageCube)
                writeQml(static_cast<const Texture &>(node), output);
            else if (node.runtimeType == Node::RuntimeType::TextureData)
                writeQml(static_cast<const TextureData &>(node), output);
            else
                Q_UNREACHABLE();
            break;
        case Node::Type::Material:
            writeQml(static_cast<const Material &>(node), output);
            break;
        case Node::Type::Mesh:
            // Only handled as a property (see: valueToQml())
            break;
        default:
            qWarning("Unhandled resource type \'%d\'?", int(node.runtimeType));
            break;
        }
    }

    // Do something more convenient if this starts expending to more types...
    // NOTE: The TextureData type is written out as a url property...
    const bool skipBlockEnd = (node.runtimeType == Node::RuntimeType::TextureData || node.nodeType == Node::Type::Mesh);
    if (!skipBlockEnd && processNode && output.scopeDepth != 0) {
        QSSGQmlScopedIndent scopedIndent(output);
        indent(output) << blockEnd(output);
    }
}

static void writeQmlForNode(const QSSGSceneDesc::Node &node, OutputContext &output)
{
    using namespace QSSGSceneDesc;

    const bool processNode = !(node.properties.isEmpty() && node.children.isEmpty())
                                    || (output.type == OutputContext::Resource);
    if (processNode) {
        QSSGQmlScopedIndent scopedIndent(output);
        switch (node.nodeType) {
        case Node::Type::Skeleton:
            writeQml(static_cast<const Skeleton &>(node), output);
            break;
        case Node::Type::Joint:
            writeQml(static_cast<const Joint &>(node), output);
            break;
        case Node::Type::Light:
            writeQml(static_cast<const Light &>(node), output);
            break;
        case Node::Type::Transform:
            writeQml(node, output);
            break;
        case Node::Type::Camera:
            writeQml(static_cast<const Camera &>(node), output);
            break;
        case Node::Type::Model:
            writeQml(static_cast<const Model &>(node), output);
            break;
        default:
            break;
        }
    }

    for (const auto &cld : node.children) {
        if (!QSSGRenderGraphObject::isResource(cld.runtimeType) && output.type == OutputContext::NodeTree) {
            QSSGQmlScopedIndent scopedIndent(output);
            writeQmlForNode(cld, output);
        }
    }

    // Do something more convenient if this starts expending to more types...
    // NOTE: The TextureData type is written out as a url property...
    const bool skipBlockEnd = (node.runtimeType == Node::RuntimeType::TextureData || node.nodeType == Node::Type::Mesh);
    if (!skipBlockEnd && processNode && output.scopeDepth != 0) {
        QSSGQmlScopedIndent scopedIndent(output);
        indent(output) << blockEnd(output);
    }
}

void writeQmlForResources(const QSSGSceneDesc::Scene::ResourceNodes &resources, OutputContext &output)
{
    auto sortedResources = resources;
    std::sort(sortedResources.begin(), sortedResources.end(), [](const QSSGSceneDesc::Node *a, const QSSGSceneDesc::Node *b) {
        using RType = QSSGSceneDesc::Node::RuntimeType;
        if (a->runtimeType == RType::TextureData && b->runtimeType != RType::TextureData)
            return true;
        if (a->runtimeType == RType::ImageCube && (b->runtimeType != RType::TextureData && b->runtimeType != RType::ImageCube))
            return true;
        if (a->runtimeType == RType::Image2D && (b->runtimeType != RType::TextureData && b->runtimeType != RType::Image2D))
            return true;

        return false;
    });
    for (const auto &res : qAsConst(sortedResources))
        writeQmlForResourceNode(*res, output);
}

static void generateKeyframeData(const QSSGSceneDesc::Animation::Channel &channel, QByteArray &keyframeData)
{
#ifdef QT_QUICK3D_ENABLE_RT_ANIMATIONS
    QCborStreamWriter writer(&keyframeData);
    // Start root array
    writer.startArray();
    // header name
    writer.append("QTimelineKeyframes");
    // file version. Increase this if the format changes.
    const int keyframesDataVersion = 1;
    writer.append(keyframesDataVersion);
    writer.append(int(channel.keys.m_head->getValueQMetaType()));

    // Start Keyframes array
    writer.startArray();
    quint8 compEnd = quint8(channel.keys.m_head->getValueType());
    bool isQuaternion = false;
    if (compEnd == quint8(QSSGSceneDesc::Animation::KeyPosition::ValueType::Quaternion)) {
        isQuaternion = true;
        compEnd = 3;
    } else {
        compEnd++;
    }
    for (const auto &key : channel.keys) {
        writer.append(key.time);
        // Easing always linear
        writer.append(QEasingCurve::Linear);
        if (isQuaternion)
            writer.append(key.value[3]);
        for (quint8 i = 0; i < compEnd; ++i)
            writer.append(key.value[i]);
    }
    // End Keyframes array
    writer.endArray();
    // End root array
    writer.endArray();
#else
    Q_UNUSED(channel)
    Q_UNUSED(keyframeData)
#endif // QT_QUICK3D_ENABLE_RT_ANIMATIONS
}

void writeQmlForAnimation(const QSSGSceneDesc::Animation &anim, qsizetype index, OutputContext &output, bool useBinaryKeyframes = true)
{
    indent(output) << "Timeline {\n";

    QSSGQmlScopedIndent scopedIndent(output);
    // The duration property of the TimelineAnimation is an int...
    const int duration = qCeil(anim.length);
    indent(output) << "startFrame: 0\n";
    indent(output) << "endFrame: " << duration << "\n";
    indent(output) << "currentFrame: 0\n";
    indent(output) << "enabled: true\n";
    indent(output) << "animations: TimelineAnimation {\n";
    {
        QSSGQmlScopedIndent scopedIndent(output);
        indent(output) << "duration: " << duration << "\n";
        indent(output) << "from: 0\n";
        indent(output) << "to: " << duration << "\n";
        indent(output) << "running: true\n";
        indent(output) << "loops: Animation.Infinite\n";
    }
    indent(output) << blockEnd(output);

    for (const auto &channel : anim.channels) {
        QString id = getIdForNode(*channel.target);
        QString propertyName = asString(channel.targetProperty);

        indent(output) << "KeyframeGroup {\n";
        {
            QSSGQmlScopedIndent scopedIndent(output);
            indent(output) << "target: " << id << "\n";
            indent(output) << "property: " << toQuotedString(propertyName) << "\n";
            if (useBinaryKeyframes) {
                const auto animFolder = getAnimationFolder();
                const auto animSourceName = getAnimationSourceName(id, propertyName, index);
                if (!output.outdir.exists(animFolder) && !output.outdir.mkdir(animFolder)) {
                    // Make a warning
                    continue;
                }
                QFile file(output.outdir.path() + QDir::separator() + animSourceName);
                if (!file.open(QIODevice::WriteOnly))
                    continue;
                QByteArray keyframeData;
                // It is possible to store this keyframeData but we have to consider
                // all the cases including runtime only or writeQml only.
                // For now, we will generate it for each case.
                generateKeyframeData(channel, keyframeData);
                file.write(keyframeData);
                file.close();
                indent(output) << "keyframeSource: " << toQuotedString(animSourceName) << "\n";
            } else {
                Q_ASSERT(!channel.keys.isEmpty());
                for (const auto &key : channel.keys) {
                    indent(output) << "Keyframe {\n";
                    {
                        QSSGQmlScopedIndent scopedIndent(output);
                        indent(output) << "frame: " << key.time << "\n";
                        indent(output) << "value: " << variantToQml(key.getValue()) << "\n";
                    }
                    indent(output) << blockEnd(output);
                }
            }
        }
        indent(output) << blockEnd(output);
    }
}

void writeQml(const QSSGSceneDesc::Scene &scene, QTextStream &stream, const QDir &outdir)
{
    auto root = scene.root;
    Q_ASSERT(root);
    OutputContext output { stream, outdir, 0, OutputContext::Header };
    writeImportHeader(output, scene.animations.count() > 0);
    output.type = OutputContext::RootNode;
    writeQml(*root, output); // Block scope will be left open!
    output.type = OutputContext::Resource;
    writeQmlForResources(scene.resources, output);
    output.type = OutputContext::NodeTree;
    for (const auto &cld : root->children) {
        if (!QSSGRenderGraphObject::isResource(cld.runtimeType)) // If the child is a resource we can skip it
            writeQmlForNode(cld, output);
    }
    // close the root

    // animations
    qsizetype animId = 0;
    for (const auto &cld : scene.animations) {
        QSSGQmlScopedIndent scopedIndent(output);
        writeQmlForAnimation(*cld, animId++, output);
        indent(output) << blockEnd(output);
    }

    indent(output) << blockEnd(output);
}

void createTimelineAnimation(const QSSGSceneDesc::Animation &anim, QObject *parent, bool isEnabled, bool useBinaryKeyframes)
{
#ifdef QT_QUICK3D_ENABLE_RT_ANIMATIONS
    auto timeline = new QQuickTimeline(parent);
    auto timelineKeyframeGroup = timeline->keyframeGroups();
    for (const auto &channel : anim.channels) {
        auto keyframeGroup = new QQuickKeyframeGroup(timeline);
        keyframeGroup->setTargetObject(channel.target->obj);
        keyframeGroup->setProperty(asString(channel.targetProperty));

        Q_ASSERT(!channel.keys.isEmpty());
        if (useBinaryKeyframes) {
            QByteArray keyframeData;
            generateKeyframeData(channel, keyframeData);

            keyframeGroup->setKeyframeData(keyframeData);
        } else {
            auto keyframes = keyframeGroup->keyframes();
            for (const auto &key : channel.keys) {
                auto keyframe = new QQuickKeyframe(keyframeGroup);
                keyframe->setFrame(key.time);
                keyframe->setValue(key.getValue());
                keyframes.append(&keyframes, keyframe);
            }
        }
        (qobject_cast<QQmlParserStatus *>(keyframeGroup))->componentComplete();
        timelineKeyframeGroup.append(&timelineKeyframeGroup, keyframeGroup);
    }
    timeline->setEndFrame(anim.length);
    timeline->setEnabled(isEnabled);

    auto timelineAnimation = new QQuickTimelineAnimation(timeline);
    timelineAnimation->setDuration(int(anim.length));
    timelineAnimation->setFrom(0.0f);
    timelineAnimation->setTo(anim.length);
    timelineAnimation->setLoops(QQuickTimelineAnimation::Infinite);
    timelineAnimation->setTargetObject(timeline);

    (qobject_cast<QQmlParserStatus *>(timeline))->componentComplete();

    timelineAnimation->setRunning(true);
#else // QT_QUICK3D_ENABLE_RT_ANIMATIONS
    Q_UNUSED(anim)
    Q_UNUSED(parent)
    Q_UNUSED(isEnabled)
    Q_UNUSED(useBinaryKeyframes)
#endif // QT_QUICK3D_ENABLE_RT_ANIMATIONS
}

void writeQmlComponent(const QSSGSceneDesc::Node &node, QTextStream &stream, const QDir &outDir)
{
    using namespace QSSGSceneDesc;
    if (node.runtimeType == Material::RuntimeType::CustomMaterial) {
        OutputContext output { stream, outDir, 0, OutputContext::Resource };
        writeImportHeader(output);
        writeQml(static_cast<const Material &>(node), output);
        // Resources, if any, are written out as properties on the component
        const auto &resources = node.scene->resources;
        writeQmlForResources(resources, output);
        indent(output) << blockEnd(output);
    } else {
        Q_UNREACHABLE(); // Only implemented for Custom material at this point.
    }
}

}

QT_END_NAMESPACE
