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

#include "qssgqmlutilities_p.h"

#include <QVector2D>
#include <QVector3D>
#include <QVector4D>
#include <QQuaternion>
#include <QDebug>
#include <QRegularExpression>

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
    auto valueType = static_cast<QMetaType::Type>(variant.type());
    if (valueType == QMetaType::Float) {
        auto value = variant.toDouble();
        return QString::number(value);
    }
    if (valueType == QMetaType::QVector2D) {
        auto value = variant.value<QVector2D>();
        return QString(QStringLiteral("Qt.vector2d(") + QString::number(double(value.x())) +
                       QStringLiteral(", ") + QString::number(double(value.y())) +
                       QStringLiteral(")"));
    }
    if (valueType == QMetaType::QVector3D) {
        auto value = variant.value<QVector3D>();
        return QString(QStringLiteral("Qt.vector3d(") + QString::number(double(value.x())) +
                       QStringLiteral(", ") + QString::number(double(value.y())) +
                       QStringLiteral(", ") + QString::number(double(value.z())) +
                       QStringLiteral(")"));
    }
    if (valueType == QMetaType::QVector4D) {
        auto value = variant.value<QVector4D>();
        return QString(QStringLiteral("Qt.vector4d(") + QString::number(double(value.x())) +
                       QStringLiteral(", ") + QString::number(double(value.y())) +
                       QStringLiteral(", ") + QString::number(double(value.z())) +
                       QStringLiteral(", ") + QString::number(double(value.w())) +
                       QStringLiteral(")"));
    }
    if (valueType == QMetaType::QColor) {
        auto value = variant.value<QColor>();
        return colorToQml(value);
    }

    if (valueType == QMetaType::QQuaternion) {
        auto value = variant.value<QQuaternion>();
        return QString(QStringLiteral("Qt.quaternion(") + QString::number(double(value.scalar())) +
                       QStringLiteral(", ") + QString::number(double(value.x())) +
                       QStringLiteral(", ") + QString::number(double(value.y())) +
                       QStringLiteral(", ") + QString::number(double(value.z())) +
                       QStringLiteral(")"));
    }

    return variant.toString();
}

QString sanitizeQmlId(const QString &id)
{
    QString idCopy = id;
    // If the id starts with a number...
    if (!idCopy.isEmpty() && idCopy.at(0).isNumber())
        idCopy.prepend(QStringLiteral("node"));

    // sometimes first letter is a # (don't replace with underscore)
    if (idCopy.startsWith('#'))
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

    sourceCopy.replace('\\', '/');

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
    model->insert(QStringLiteral("tesselationMode"), QStringLiteral("Model.NoTessellation"));
    model->insert(QStringLiteral("edgeTess"), 1);
    model->insert(QStringLiteral("innerTess"), 1);
    m_properties.insert(Type::Model, model);

    // Camera
    PropertiesMap *camera = new PropertiesMap;
    camera->insert(QStringLiteral("projectionMode"), QStringLiteral("Camera.Perspective"));
    camera->insert(QStringLiteral("clipNear"), 10.0f);
    camera->insert(QStringLiteral("clipFar"), 10000.0f);
    camera->insert(QStringLiteral("fieldOfView"), 60.0f);
    camera->insert(QStringLiteral("fieldOfViewOrientation"), QStringLiteral("Camera.Vertical"));
    m_properties.insert(Type::Camera, camera);

    // Directional Light
    PropertiesMap *directionalLight = new PropertiesMap;
    directionalLight->insert(QStringLiteral("color"), QColor(Qt::white));
    directionalLight->insert(QStringLiteral("ambientColor"), QColor(Qt::black));
    directionalLight->insert(QStringLiteral("brightness"), 100.0f);
    directionalLight->insert(QStringLiteral("castShadow"), false);
    directionalLight->insert(QStringLiteral("shadowBias"), 0.0f);
    directionalLight->insert(QStringLiteral("shadowFactor"), 5.0f);
    directionalLight->insert(QStringLiteral("shadowMapResolution"), 9);
    directionalLight->insert(QStringLiteral("shadowMapFar"), 5000.0f);
    directionalLight->insert(QStringLiteral("shadowMapFieldOfView"), 90.0f);
    directionalLight->insert(QStringLiteral("shadowFilter"), 35.0f);
    m_properties.insert(Type::DirectionalLight, directionalLight);


    // Point Light
    PropertiesMap *pointLight = new PropertiesMap;
    pointLight->insert(QStringLiteral("color"), QColor(Qt::white));
    pointLight->insert(QStringLiteral("ambientColor"), QColor(Qt::black));
    pointLight->insert(QStringLiteral("brightness"), 100.0f);
    pointLight->insert(QStringLiteral("castShadow"), false);
    pointLight->insert(QStringLiteral("shadowBias"), 0.0f);
    pointLight->insert(QStringLiteral("shadowFactor"), 5.0f);
    pointLight->insert(QStringLiteral("shadowMapResolution"), 9);
    pointLight->insert(QStringLiteral("shadowMapFar"), 5000.0f);
    pointLight->insert(QStringLiteral("shadowMapFieldOfView"), 90.0f);
    pointLight->insert(QStringLiteral("shadowFilter"), 35.0f);
    pointLight->insert(QStringLiteral("constantFade"), 0.0f);
    pointLight->insert(QStringLiteral("linearFade"), 0.0f);
    pointLight->insert(QStringLiteral("quadraticFade"), 0.0f);
    m_properties.insert(Type::PointLight, pointLight);

    // Area Light
    PropertiesMap *areaLight = new PropertiesMap;
    areaLight->insert(QStringLiteral("color"), QColor(Qt::white));
    areaLight->insert(QStringLiteral("ambientColor"), QColor(Qt::black));
    areaLight->insert(QStringLiteral("brightness"), 100.0f);
    areaLight->insert(QStringLiteral("castShadow"), false);
    areaLight->insert(QStringLiteral("shadowBias"), 0.0f);
    areaLight->insert(QStringLiteral("shadowFactor"), 5.0f);
    areaLight->insert(QStringLiteral("shadowMapResolution"), 9);
    areaLight->insert(QStringLiteral("shadowMapFar"), 5000.0f);
    areaLight->insert(QStringLiteral("shadowMapFieldOfView"), 90.0f);
    areaLight->insert(QStringLiteral("shadowFilter"), 35.0f);
    areaLight->insert(QStringLiteral("width"), 0.0f);
    areaLight->insert(QStringLiteral("height"), 0.0f);
    m_properties.insert(Type::AreaLight, areaLight);

    // Spot Light
    PropertiesMap *spotLight = new PropertiesMap;
    spotLight->insert(QStringLiteral("color"), QColor(Qt::white));
    spotLight->insert(QStringLiteral("ambientColor"), QColor(Qt::black));
    spotLight->insert(QStringLiteral("brightness"), 100.0f);
    spotLight->insert(QStringLiteral("castShadow"), false);
    spotLight->insert(QStringLiteral("shadowBias"), 0.0f);
    spotLight->insert(QStringLiteral("shadowFactor"), 5.0f);
    spotLight->insert(QStringLiteral("shadowMapResolution"), 9);
    spotLight->insert(QStringLiteral("shadowMapFar"), 5000.0f);
    spotLight->insert(QStringLiteral("shadowMapFieldOfView"), 90.0f);
    spotLight->insert(QStringLiteral("shadowFilter"), 35.0f);
    spotLight->insert(QStringLiteral("constantFade"), 0.0f);
    spotLight->insert(QStringLiteral("linearFade"), 0.0f);
    spotLight->insert(QStringLiteral("quadraticFade"), 0.0f);
    spotLight->insert(QStringLiteral("coneAngle"), 0.0f);
    spotLight->insert(QStringLiteral("innerConeAngle"), 0.0f);
    m_properties.insert(Type::SpotLight, spotLight);


    // DefaultMaterial
    PropertiesMap *defaultMaterial = new PropertiesMap;
    defaultMaterial->insert(QStringLiteral("lighting"), QStringLiteral("DefaultMaterial.FragmentLighting"));
    defaultMaterial->insert(QStringLiteral("blendMode"), QStringLiteral("DefaultMaterial.SourceOver"));
    defaultMaterial->insert(QStringLiteral("diffuseColor"), QColor(Qt::white));
    defaultMaterial->insert(QStringLiteral("emissiveFactor"), 0.0f);
    defaultMaterial->insert(QStringLiteral("emissiveColor"), QColor(Qt::white));
    defaultMaterial->insert(QStringLiteral("specularModel"), QStringLiteral("DefaultMaterial.Default"));
    defaultMaterial->insert(QStringLiteral("specularTint"), QColor(Qt::white));
    defaultMaterial->insert(QStringLiteral("indexOfRefraction"), 0.2f);
    defaultMaterial->insert(QStringLiteral("fresnelPower"), 0.0f);
    defaultMaterial->insert(QStringLiteral("specularAmount"), 1.0f);
    defaultMaterial->insert(QStringLiteral("specularRoughness"), 50.0f);
    defaultMaterial->insert(QStringLiteral("opacity"), 1.0f);
    defaultMaterial->insert(QStringLiteral("bumpAmount"), 0.0f);
    defaultMaterial->insert(QStringLiteral("translucentFalloff"), 0.0f);
    defaultMaterial->insert(QStringLiteral("diffuseLightWrap"), 0.0f);
    defaultMaterial->insert(QStringLiteral("vertexColorsEnabled"), false);
    defaultMaterial->insert(QStringLiteral("displacementAmount"), 0.0f);

    m_properties.insert(Type::DefaultMaterial, defaultMaterial);

    PropertiesMap *principledMaterial = new PropertiesMap;
    principledMaterial->insert(QStringLiteral("lighting"), QStringLiteral("PrincipledMaterial.FragmentLighting"));
    principledMaterial->insert(QStringLiteral("blendMode"), QStringLiteral("PrincipledMaterial.SourceOver"));
    principledMaterial->insert(QStringLiteral("alphaMode"), QStringLiteral("PrincipledMaterial.Opaque"));
    principledMaterial->insert(QStringLiteral("baseColor"), QColor(Qt::white));
    principledMaterial->insert(QStringLiteral("metalness"), 1.0f);
    principledMaterial->insert(QStringLiteral("specularAmount"), 0.0f);
    principledMaterial->insert(QStringLiteral("specularTint"), QColor(Qt::black));
    principledMaterial->insert(QStringLiteral("roughness"), 0.0f);
    principledMaterial->insert(QStringLiteral("indexOfRefraction"), 1.45f);
    principledMaterial->insert(QStringLiteral("emissiveColor"), QColor(Qt::black));
    principledMaterial->insert(QStringLiteral("emissiveFactor"), 0.0f);
    principledMaterial->insert(QStringLiteral("opacity"), 1.0f);
    principledMaterial->insert(QStringLiteral("normalStrength"), 1.0f);
    principledMaterial->insert(QStringLiteral("alphaCutoff"), 0.5f);

    m_properties.insert(Type::PrincipledMaterial, principledMaterial);

    // Image
    PropertiesMap *texture = new PropertiesMap;
    texture->insert(QStringLiteral("scaleU"), 1.0f);
    texture->insert(QStringLiteral("scaleV"), 1.0f);
    texture->insert(QStringLiteral("mappingMode"), QStringLiteral("Texture.Normal"));
    texture->insert(QStringLiteral("tilingModeHorizontal"), QStringLiteral("Texture.ClampToEdge"));
    texture->insert(QStringLiteral("tilingModeVertical"), QStringLiteral("Texture.ClampToEdge"));
    texture->insert(QStringLiteral("rotationUV"), 0.0f);
    texture->insert(QStringLiteral("positionU"), 0.0f);
    texture->insert(QStringLiteral("positionV"), 0.0f);
    texture->insert(QStringLiteral("pivotU"), 0.0f);
    texture->insert(QStringLiteral("pivotV"), 0.0f);
    m_properties.insert(Type::Texture, texture);
}

PropertyMap::~PropertyMap()
{
    for (auto proprtyMap : m_properties.values())
        delete proprtyMap;
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
    while(sourceCopy.startsWith('.') || sourceCopy.startsWith('/') || sourceCopy.startsWith('\\'))
        sourceCopy.remove(0, 1);
    return sourceCopy;
}

}

QT_END_NAMESPACE
