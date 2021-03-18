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

#include "uippresentation.h"
#include "enummaps.h"
#include "datamodelparser.h"
#include "propertymap.h"
#include "uniqueidmapper.h"
#include <QtQuick3DAssetImport/private/qssgqmlutilities_p.h>

QT_BEGIN_NAMESPACE

namespace Q3DS {

bool convertToPropertyType(const QStringRef &value, Q3DS::PropertyType *type, int *componentCount, const char *desc, QXmlStreamReader *reader)
{
    if (componentCount)
        *componentCount = 1;
    bool ok = false;
    if (value == QStringLiteral("StringList")) {
        ok = true;
        *type = Q3DS::StringList;
    } else if (value == QStringLiteral("FloatRange")) {
        ok = true;
        *type = Q3DS::FloatRange;
    } else if (value == QStringLiteral("LongRange")) {
        ok = true;
        *type = Q3DS::LongRange;
    } else if (value == QStringLiteral("Float") || value == QStringLiteral("float")) {
        ok = true;
        *type = Q3DS::Float;
    } else if (value == QStringLiteral("Float2")) {
        ok = true;
        *type = Q3DS::Float2;
        if (componentCount)
            *componentCount = 2;
    } else if (value == QStringLiteral("Long")) {
        ok = true;
        *type = Q3DS::Long;
    } else if (value == QStringLiteral("Matrix4x4")) {
        ok = true;
        *type = Q3DS::Matrix4x4;
        if (componentCount)
            *componentCount = 16;
    } else if (value == QStringLiteral("Vector") || value == QStringLiteral("Float3")) {
        ok = true;
        *type = Q3DS::Vector;
        if (componentCount)
            *componentCount = 3;
    } else if (value == QStringLiteral("Scale")) {
        ok = true;
        *type = Q3DS::Scale;
        if (componentCount)
            *componentCount = 3;
    } else if (value == QStringLiteral("Rotation")) {
        ok = true;
        *type = Q3DS::Rotation;
        if (componentCount)
            *componentCount = 3;
    } else if (value == QStringLiteral("Color")) {
        ok = true;
        *type = Q3DS::Color;
        if (componentCount)
            *componentCount = 4;
    } else if (value == QStringLiteral("Boolean") || value == QStringLiteral("Bool")) {
        ok = true;
        *type = Q3DS::Boolean;
    } else if (value == QStringLiteral("Slide")) {
        ok = true;
        *type = Q3DS::Slide;
    } else if (value == QStringLiteral("Font")) {
        ok = true;
        *type = Q3DS::Font;
    } else if (value == QStringLiteral("FontSize")) {
        ok = true;
        *type = Q3DS::FontSize;
    } else if (value == QStringLiteral("String")) {
        ok = true;
        *type = Q3DS::String;
    } else if (value == QStringLiteral("MultiLineString")) {
        ok = true;
        *type = Q3DS::MultiLineString;
    } else if (value == QStringLiteral("ObjectRef")) {
        ok = true;
        *type = Q3DS::ObjectRef;
    } else if (value == QStringLiteral("Image")) {
        ok = true;
        *type = Q3DS::Image;
    } else if (value == QStringLiteral("Mesh")) {
        ok = true;
        *type = Q3DS::Mesh;
    } else if (value == QStringLiteral("Import")) {
        ok = true;
        *type = Q3DS::Import;
    } else if (value == QStringLiteral("Texture")) {
        ok = true;
        *type = Q3DS::Texture;
    } else if (value == QStringLiteral("Image2D")) {
        ok = true;
        *type = Q3DS::Image2D;
    } else if (value == QStringLiteral("Buffer")) {
        ok = true;
        *type = Q3DS::Buffer;
    } else if (value == QStringLiteral("Guid")) {
        ok = true;
        *type = Q3DS::Guid;
    } else if (value == QStringLiteral("StringListOrInt")) {
        ok = true;
        *type = Q3DS::StringListOrInt;
    } else if (value == QStringLiteral("Renderable")) {
        ok = true;
        *type = Q3DS::String;
    } else if (value == QStringLiteral("PathBuffer")) {
        ok = true;
        *type = Q3DS::String;
    } else if (value == QStringLiteral("ShadowMapResolution")) {
        ok = true;
        *type = Q3DS::Long;
    } else {
        *type = Q3DS::Unknown;
        if (reader)
            reader->raiseError(QObject::tr("Invalid %1 \"%2\"").arg(QString::fromUtf8(desc)).arg(value.toString()));
    }
    return ok;
}

bool convertToInt(const QStringRef &value, int *v, const char *desc, QXmlStreamReader *reader)
{
    if (value.isEmpty()) {
        *v = 0;
        return true;
    }
    bool ok = false;
    *v = value.toInt(&ok);
    if (!ok && reader)
        reader->raiseError(QObject::tr("Invalid %1 \"%2\"").arg(QString::fromUtf8(desc)).arg(value.toString()));
    return ok;
}

bool convertToInt32(const QStringRef &value, qint32 *v, const char *desc, QXmlStreamReader *reader)
{
    if (value.isEmpty()) {
        *v = 0;
        return true;
    }
    int vv;
    bool r = convertToInt(value, &vv, desc, reader);
    if (r)
        *v = qint32(vv);
    return r;
}

bool convertToBool(const QStringRef &value, bool *v, const char *desc, QXmlStreamReader *reader)
{
    Q_UNUSED(desc);
    Q_UNUSED(reader);
    *v = (value == QStringLiteral("True") || value == QStringLiteral("true")
          || value == QStringLiteral("Yes") || value == QStringLiteral("yes")
          || value == QStringLiteral("1"));
    return true;
}

bool convertToFloat(const QStringRef &value, float *v, const char *desc, QXmlStreamReader *reader)
{
    if (value.isEmpty()) {
        *v = 0;
        return true;
    }
    bool ok = false;
    *v = value.toFloat(&ok);
    // Adjust values that are "almost" zero values to 0.0, as we don't really expect values with
    // more then 3-4 decimal points (note that qFuzzyIsNull does allow slightly higher resolution though).
    if (ok && qFuzzyIsNull(*v))
        *v = 0.0f;
    if (!ok && reader)
        reader->raiseError(QObject::tr("Invalid %1 \"%2\"").arg(QString::fromUtf8(desc)).arg(value.toString()));
    return ok;
}

bool convertToVector2D(const QStringRef &value, QVector2D *v, const char *desc, QXmlStreamReader *reader)
{
    QVector<QStringRef> floatStrings = value.split(' ', Qt::SkipEmptyParts);
    if (floatStrings.count() != 2) {
        if (reader)
            reader->raiseError(QObject::tr("Invalid %1 \"%2\"").arg(QString::fromUtf8(desc)).arg(value.toString()));
        return false;
    }
    float x;
    float y;
    if (!convertToFloat(floatStrings[0], &x, "Vector2D[x]", reader))
        return false;
    if (!convertToFloat(floatStrings[1], &y, "Vector2D[y]", reader))
        return false;
    v->setX(x);
    v->setY(y);
    return true;
}

bool convertToVector3D(const QStringRef &value, QVector3D *v, const char *desc, QXmlStreamReader *reader)
{
    QVector<QStringRef> floatStrings = value.split(' ', Qt::SkipEmptyParts);
    if (floatStrings.count() != 3) {
        if (reader)
            reader->raiseError(QObject::tr("Invalid %1 \"%2\"").arg(QString::fromUtf8(desc)).arg(value.toString()));
        return false;
    }
    float x;
    float y;
    float z;
    if (!convertToFloat(floatStrings[0], &x, "Vector3D[x]", reader))
        return false;
    if (!convertToFloat(floatStrings[1], &y, "Vector3D[y]", reader))
        return false;
    if (!convertToFloat(floatStrings[2], &z, "Vector3D[z]", reader))
        return false;
    v->setX(x);
    v->setY(y);
    v->setZ(z);
    return true;
}

bool convertToVector4D(const QStringRef &value, QVector4D *v, const char *desc, QXmlStreamReader *reader)
{
    QVector<QStringRef> floatStrings = value.split(' ', Qt::SkipEmptyParts);
    if (!(floatStrings.count() == 4 || floatStrings.count() == 3)) {
        if (reader)
            reader->raiseError(QObject::tr("Invalid %1 \"%2\"").arg(QString::fromUtf8(desc)).arg(value.toString()));
        return false;
    }
    float x;
    float y;
    float z;
    float w;
    if (!convertToFloat(floatStrings[0], &x, "Vector4D[x]", reader))
        return false;
    if (!convertToFloat(floatStrings[1], &y, "Vector4D[y]", reader))
        return false;
    if (!convertToFloat(floatStrings[2], &z, "Vector4D[z]", reader))
        return false;
    // Compat with old colors
    if (floatStrings.count() == 4) {
        if (!convertToFloat(floatStrings[3], &w, "Vector4D[w]", reader))
            return false;
    } else {
        w = 1.0;
    }

    v->setX(x);
    v->setY(y);
    v->setZ(z);
    v->setW(w);
    return true;
}

bool convertToMatrix4x4(const QStringRef &value, QMatrix4x4 *v, const char *desc, QXmlStreamReader *reader)
{
    QVector<QStringRef> floatStrings = value.split(' ', Qt::SkipEmptyParts);
    if (floatStrings.count() != 16) {
        if (reader)
            reader->raiseError(QObject::tr("Invalid %1 \"%2\"").arg(QString::fromUtf8(desc)).arg(value.toString()));
        return false;
    }

    float m[16];

    for (int i = 0; i < 16; ++i) {
        if (!convertToFloat(floatStrings[i], &m[i], ("Matrix4x4[" + QString::number(i) + "]")
                            .toUtf8().constData(), reader)) {
            return false;
        }
    }

    float *data = v->data();
    for (int i = 0; i < 16; ++i)
        data[i] = m[i];

    return true;
}

int animatablePropertyTypeToMetaType(Q3DS::PropertyType type)
{
    switch (type) {
    case Float:
        return QMetaType::Float;
    case Long:
        return QVariant::Int;
    case Float2:
        return QVariant::Vector2D;
    case Matrix4x4:
        return QVariant::Matrix4x4;
    case Vector:
    case Scale:
    case Rotation:
        return QVariant::Vector3D;
    case Color:
        return QVariant::Color;
    default:
        return QVariant::Invalid;
    }
}

QVariant convertToVariant(const QString &value, Q3DS::PropertyType type)
{
    switch (type) {
    case StringList:
    case Slide:
    case Font:
    case String:
    case MultiLineString:
    case ObjectRef:
    case Image:
    case Mesh:
    case Import:
    case Texture:
    case Image2D:
    case Buffer:
    case Guid:
    case StringListOrInt:
    case Renderable:
    case PathBuffer:
        return value;
    case LongRange:
    case Long:
        return value.toInt();
    case FloatRange:
    case Float:
    case FontSize:
        return value.toFloat();
    case Float2:
    {
        QVector2D v;
        if (convertToVector2D(&value, &v))
            return v;
    }
        break;
    case Matrix4x4:
    {
        QMatrix4x4 v;
        if (convertToMatrix4x4(&value, &v))
            return v;
    }
        break;
    case Vector:
    case Scale:
    case Rotation:
    case Color:
    {
        QVector4D v;
        if (convertToVector4D(&value, &v))
            return v;
    }
        break;
    case Boolean:
    {
        bool v;
        if (convertToBool(&value, &v))
            return v;
    }
        break;
    default:
        break;
    }

    return QVariant();
}

//QVariant convertToVariant(const QString &value, const Q3DSMaterial::PropertyElement &propMeta)
//{
//    switch (propMeta.type) {
//    case Enum:
//    {
//        int idx = propMeta.enumValues.indexOf(value);
//        return idx >= 0 ? idx : 0;
//    }
//    default:
//        return convertToVariant(value, propMeta.type);
//    }
//}

QString convertFromVariant(const QVariant &value)
{
    switch (value.type()) {
    case QVariant::Vector2D:
    {
        const QVector2D v = value.value<QVector2D>();
        return QString(QLatin1String("%1 %2"))
                .arg(QString::number(v.x())).arg(QString::number(v.y()));
    }
    case QVariant::Vector3D:
    {
        const QVector3D v = value.value<QVector3D>();
        return QString(QLatin1String("%1 %2 %3"))
                .arg(QString::number(v.x())).arg(QString::number(v.y())).arg(QString::number(v.z()));
    }
    case QVariant::Color:
    {
        const QColor c = value.value<QColor>();
        const QVector4D v = QVector4D(c.redF(), c.greenF(), c.blueF(), c.alphaF());
        return QString(QLatin1String("%1 %2 %3 %4"))
                .arg(QString::number(v.x())).arg(QString::number(v.y())).arg(QString::number(v.z())).arg(QString::number(v.w()));
    }
    case QVariant::Bool:
        return value.toBool() ? QLatin1String("true") : QLatin1String("false");
    case QVariant::Matrix4x4:
    {
        const QMatrix4x4 v = value.value<QMatrix4x4>();
        const float *data = v.constData();
        return QString(QLatin1String("%1 %2 %3 %4 %5 %6 %7 %8 %9 %10 %11 %12 %13 %14 %15 %16"))
                .arg(QString::number(data[0])).arg(QString::number(data[1])).arg(QString::number(data[2])).arg(QString::number(data[3]))
                .arg(QString::number(data[4])).arg(QString::number(data[5])).arg(QString::number(data[6])).arg(QString::number(data[7]))
                .arg(QString::number(data[8])).arg(QString::number(data[9])).arg(QString::number(data[10])).arg(QString::number(data[11]))
                .arg(QString::number(data[12])).arg(QString::number(data[13])).arg(QString::number(data[14])).arg(QString::number(data[15]));
    }
    default:
        return value.toString();
    }
}

} // namespace Q3DS

namespace {

QString qmlPresentationComponentName(const QString &name) {
    QString nameCopy = name;


    if (nameCopy.isEmpty())
        return QStringLiteral("Default");

    if (nameCopy.startsWith("#"))
        nameCopy.remove(0, 1);

    if (nameCopy.startsWith("materials/"))
        nameCopy.remove("materials/");

    if (nameCopy.startsWith("/"))
        nameCopy.remove(0, 1);

    nameCopy = QSSGQmlUtilities::qmlComponentName(nameCopy);

    return nameCopy;
}

bool writeQmlPropertyHelper(QTextStream &output, int tabLevel, GraphObject::Type type, const QString &propertyName, const QVariant &value, bool ignoreDefaultValues = false)
{
    if (!PropertyMap::instance()->propertiesForType(type)->contains(propertyName)) {
        //qWarning() << "property: " << propertyName << " not found";
        return false;
    }

    auto property = PropertyMap::instance()->propertiesForType(type)->value(propertyName);

    if ((property.defaultValue != value) || ignoreDefaultValues) {
        QString valueString = QSSGQmlUtilities::variantToQml(value);
        output << QSSGQmlUtilities::insertTabs(tabLevel) << property.name << ": " << valueString << Qt::endl;
    }
    return true;
}

}

PropertyChange PropertyChange::fromVariant(const QString &name, const QVariant &value)
{
    return PropertyChange(name, Q3DS::convertFromVariant(value));
}

void PropertyChangeList::append(const PropertyChange &change)
{
    if (!change.isValid())
        return;

    m_changes.append(change);
    m_keys.insert(change.nameStr());
}

GraphObject::GraphObject(GraphObject::Type type)
    : m_type(type)
{

}

GraphObject::~GraphObject()
{
    destroyGraph();
}

QString GraphObject::typeName() const
{
    switch (m_type) {
    case Type::Asset:
        return QStringLiteral("Asset");
    case Type::Scene:
        return QStringLiteral("Scene");
    case Type::Slide:
        return QStringLiteral("Slide");
    case Type::Image:
        return QStringLiteral("Image");
    case Type::DefaultMaterial:
        return QStringLiteral("DefaultMaterial");
    case Type::ReferencedMaterial:
        return QStringLiteral("ReferencedMaterial");
    case Type::CustomMaterial:
        return QStringLiteral("CustomMaterial");
    case Type::Effect:
        return QStringLiteral("Effect");
    case Type::Behavior:
        return QStringLiteral("Behavior");
    case Type::Layer:
        return QStringLiteral("Layer");
    case Type::Camera:
        return QStringLiteral("Camera");
    case Type::Light:
        return QStringLiteral("Light");
    case Type::Model:
        return QStringLiteral("Model");
    case Type::Group:
        return QStringLiteral("Group");
    case Type::Text:
        return QStringLiteral("Text");
    case Type::Component:
        return QStringLiteral("Component");
    case Type::Alias:
        return QStringLiteral("Alias");
    }

    Q_UNREACHABLE();
    return QString();
}

int GraphObject::childCount() const
{
    int count = 0;
    GraphObject *n = m_firstChild;
    while (n) {
        ++count;
        n = n->m_nextSibling;
    }
    return count;
}

GraphObject *GraphObject::childAtIndex(int idx) const
{
    GraphObject *n = m_firstChild;
    while (idx && n) {
        --idx;
        n = n->m_nextSibling;
    }
    return n;
}

void GraphObject::removeChildNode(GraphObject *node)
{
    Q_ASSERT(node->parent() == this);

    GraphObject *previous = node->m_previousSibling;
    GraphObject *next = node->m_nextSibling;

    if (previous)
        previous->m_nextSibling = next;
    else
        m_firstChild = next;

    if (next)
        next->m_previousSibling = previous;
    else
        m_lastChild = previous;

    node->m_previousSibling = nullptr;
    node->m_nextSibling = nullptr;
    // now it can be nulled out
    node->m_parent = nullptr;
}

void GraphObject::removeAllChildNodes()
{
    while (m_firstChild) {
        GraphObject *node = m_firstChild;
        m_firstChild = node->m_nextSibling;
        node->m_nextSibling = nullptr;
        if (m_firstChild)
            m_firstChild->m_previousSibling = nullptr;
        else
            m_lastChild = nullptr;
        node->m_parent = nullptr;
    }
}

void GraphObject::prependChildNode(GraphObject *node)
{
    Q_ASSERT_X(!node->m_parent, "GraphObject::prependChildNode", "GraphObject already has a parent");

    if (m_firstChild)
        m_firstChild->m_previousSibling = node;
    else
        m_lastChild = node;

    node->m_nextSibling = m_firstChild;
    m_firstChild = node;
    node->m_parent = this;
}

void GraphObject::appendChildNode(GraphObject *node)
{
    Q_ASSERT_X(!node->m_parent, "GraphObject::appendChildNode", "GraphObject already has a parent");

    if (m_lastChild)
        m_lastChild->m_nextSibling = node;
    else
        m_firstChild = node;

    node->m_previousSibling = m_lastChild;
    m_lastChild = node;
    node->m_parent = this;
}

void GraphObject::insertChildNodeBefore(GraphObject *node, GraphObject *before)
{
    Q_ASSERT_X(!node->m_parent, "GraphObject::insertChildNodeBefore", "GraphObject already has a parent");
    Q_ASSERT_X(before && before->m_parent == this, "GraphObject::insertChildNodeBefore", "The parent of \'before\' is wrong");

    GraphObject *previous = before->m_previousSibling;
    if (previous)
        previous->m_nextSibling = node;
    else
        m_firstChild = node;

    node->m_previousSibling = previous;
    node->m_nextSibling = before;
    before->m_previousSibling = node;
    node->m_parent = this;
}

void GraphObject::insertChildNodeAfter(GraphObject *node, GraphObject *after)
{
    Q_ASSERT_X(!node->m_parent, "GraphObject::insertChildNodeAfter", "GraphObject already has a parent");
    Q_ASSERT_X(after && after->m_parent == this, "GraphObject::insertChildNodeAfter", "The parent of \'after\' is wrong");

    GraphObject *next = after->m_nextSibling;
    if (next)
        next->m_previousSibling = node;
    else
        m_lastChild = node;

    node->m_nextSibling = next;
    node->m_previousSibling = after;
    after->m_nextSibling = node;
    node->m_parent = this;
}

void GraphObject::reparentChildNodesTo(GraphObject *newParent)
{
    for (GraphObject *c = firstChild(); c; c = firstChild()) {
        removeChildNode(c);
        newParent->appendChildNode(c);
    }
}

// The property conversion functions all follow the same pattern:
// 1. Check if a value is provided explicitly in the attribute list.
// 2. Then, when PropSetDefaults is set, see if the metadata provided a default value.
// 3. If all else fails, just return false. This is not fatal (and perfectly normal when PropSetDefaults is not set).

// V is const iterable with name() and value() on iter
template<typename T, typename V>
bool parseProperty(const V &attrs, GraphObject::PropSetFlags flags,
                   const QString &dataModelTypeName, const QString &propName, Q3DS::PropertyType propType,
                   T *dst, std::function<bool(const QStringRef &, T *v)> convertFunc)
{
    auto it = std::find_if(attrs.cbegin(), attrs.cend(), [propName](const typename V::value_type &v) { return v.name() == propName; });
    if (it != attrs.cend()) {
        return convertFunc(it->value(), dst);
    } else if (flags.testFlag(GraphObject::PropSetDefaults)) {
        DataModelParser *dataModelParser = DataModelParser::instance();
        if (dataModelParser) {
            const QVector<DataModelParser::Property> *props = dataModelParser->propertiesForType(dataModelTypeName);
            if (props) {
                auto it = std::find_if(props->cbegin(), props->cend(),
                                       [propName](const DataModelParser::Property &v) { return v.name == propName; });
                if (it != props->cend()) {
                    Q_UNUSED(propType);
                    Q_ASSERT(it->type == propType);
                    return convertFunc(QStringRef(&it->defaultValue), dst);
                }
            }
        }
    }
    return false;
}

template<typename V>
bool parseProperty(const V &attrs, GraphObject::PropSetFlags flags, const QString &typeName, const QString &propName, bool *dst)
{
    return ::parseProperty<bool>(attrs, flags, typeName, propName, Q3DS::Boolean, dst, [](const QStringRef &s, bool *v) { return Q3DS::convertToBool(s, v); });
}

template<typename V>
bool parseProperty(const V &attrs, GraphObject::PropSetFlags flags, const QString &typeName, const QString &propName, qint32 *dst)
{
    return ::parseProperty<qint32>(attrs, flags, typeName, propName, Q3DS::Long, dst, [](const QStringRef &s, qint32 *v) { return Q3DS::convertToInt32(s, v); });
}

template<typename V>
bool parseProperty(const V &attrs, GraphObject::PropSetFlags flags, const QString &typeName, const QString &propName, float *dst)
{
    return ::parseProperty<float>(attrs, flags, typeName, propName, Q3DS::Float, dst, [](const QStringRef &s, float *v) { return Q3DS::convertToFloat(s, v); });
}

template<typename V>
bool parseProperty(const V &attrs, GraphObject::PropSetFlags flags, const QString &typeName, const QString &propName, QMatrix4x4 *dst)
{
    return ::parseProperty<float>(attrs, flags, typeName, propName, Q3DS::Matrix4x4, dst, [](const QStringRef &s, QMatrix4x4 *v) { return Q3DS::convertToMatrix4x4(s, v); });
}

template<typename V>
bool parseProperty(const V &attrs, GraphObject::PropSetFlags flags, const QString &typeName, const QString &propName, QVector3D *dst)
{
    return ::parseProperty<QVector3D>(attrs, flags, typeName, propName, Q3DS::Vector, dst, [](const QStringRef &s, QVector3D *v) { return Q3DS::convertToVector3D(s, v); });
}

template<typename V>
bool parseProperty(const V &attrs, GraphObject::PropSetFlags flags, const QString &typeName, const QString &propName, QColor *dst)
{
    QVector4D rgba;
    bool r = ::parseProperty<QVector4D>(attrs, flags, typeName, propName, Q3DS::Color, &rgba, [](const QStringRef &s, QVector4D *v) { return Q3DS::convertToVector4D(s, v); });
    if (r)
        *dst = QColor::fromRgbF(rgba.x(), rgba.y(), rgba.z(), rgba.w());

    return r;
}

template<typename V>
bool parseProperty(const V &attrs, GraphObject::PropSetFlags flags, const QString &typeName, const QString &propName, QString *dst)
{
    return ::parseProperty<QString>(attrs, flags, typeName, propName, Q3DS::String, dst, [](const QStringRef &s, QString *v) { *v = s.toString(); return true; });
}

template<typename V>
bool parseRotationProperty(const V &attrs, GraphObject::PropSetFlags flags, const QString &typeName, const QString &propName, QVector3D *dst)
{
    return ::parseProperty<QVector3D>(attrs, flags, typeName, propName, Q3DS::Rotation, dst, [](const QStringRef &s, QVector3D *v) { return Q3DS::convertToVector3D(s, v); });
}

template<typename V>
bool parseImageProperty(const V &attrs, GraphObject::PropSetFlags flags, const QString &typeName, const QString &propName, QString *dst)
{
    return ::parseProperty<QString>(attrs, flags, typeName, propName, Q3DS::Image, dst, [](const QStringRef &s, QString *v) { *v = s.toString(); return true; });
}

template<typename V>
bool parseMeshProperty(const V &attrs, GraphObject::PropSetFlags flags, const QString &typeName, const QString &propName, QString *dst)
{
    return ::parseProperty<QString>(attrs, flags, typeName, propName, Q3DS::Mesh, dst, [](const QStringRef &s, QString *v) { *v = s.toString(); return true; });
}

template<typename V>
bool parseObjectRefProperty(const V &attrs, GraphObject::PropSetFlags flags, const QString &typeName, const QString &propName, QString *dst)
{
    return ::parseProperty<QString>(attrs, flags, typeName, propName, Q3DS::ObjectRef, dst, [](const QStringRef &s, QString *v) { *v = s.toString(); return true; });
}

template<typename V>
bool parseMultiLineStringProperty(const V &attrs, GraphObject::PropSetFlags flags, const QString &typeName, const QString &propName, QString *dst)
{
    return ::parseProperty<QString>(attrs, flags, typeName, propName, Q3DS::MultiLineString, dst, [](const QStringRef &s, QString *v) { *v = s.toString(); return true; });
}

template<typename V>
bool parseFontProperty(const V &attrs, GraphObject::PropSetFlags flags, const QString &typeName, const QString &propName, QString *dst)
{
    return ::parseProperty<QString>(attrs, flags, typeName, propName, Q3DS::Font, dst, [](const QStringRef &s, QString *v) { *v = s.toString(); return true; });
}

template<typename V>
bool parseFontSizeProperty(const V &attrs, GraphObject::PropSetFlags flags, const QString &typeName, const QString &propName, float *dst)
{
    return ::parseProperty<float>(attrs, flags, typeName, propName, Q3DS::FontSize, dst, [](const QStringRef &s, float *v) { return Q3DS::convertToFloat(s, v); });
}

template<typename V>
bool parseSizeProperty(const V &attrs, GraphObject::PropSetFlags flags, const QString &typeName, const QString &propName, QVector2D *dst)
{
    return ::parseProperty<QVector2D>(attrs, flags, typeName, propName, Q3DS::Float2, dst, [](const QStringRef &s, QVector2D *v) { return Q3DS::convertToVector2D(s, v); });
}

struct StringOrInt {
    QString s;
    int n;
    bool isInt;
};

template<typename V>
bool parseProperty(const V &attrs, GraphObject::PropSetFlags flags, const QString &typeName, const QString &propName, StringOrInt *dst)
{
    // StringListOrInt -> either an enum value or an int
    QString tmp;
    if (parseProperty<QString>(attrs, flags, typeName, propName, Q3DS::StringListOrInt, &tmp,
                                 [](const QStringRef &s, QString *v) { *v = s.toString(); return true; }))
    {
        bool ok = false;
        int v = tmp.toInt(&ok);
        if (ok) {
            dst->isInt = true;
            dst->n = v;
        } else {
            dst->isInt = false;
            dst->s = tmp;
        }
        return true;
    }
    return false;
}

template<typename V>
bool parseProperty(const V &attrs, GraphObject::PropSetFlags flags, const QString &typeName, const QString &propName, Node::RotationOrder *dst)
{
    return ::parseProperty<Node::RotationOrder>(attrs, flags, typeName, propName, Q3DS::Enum, dst, [](const QStringRef &s, Node::RotationOrder *v) { return EnumMap::enumFromStr(s, v); });
}

template<typename V>
bool parseProperty(const V &attrs, GraphObject::PropSetFlags flags, const QString &typeName, const QString &propName, Node::Orientation *dst)
{
    return ::parseProperty<Node::Orientation>(attrs, flags, typeName, propName, Q3DS::Enum, dst, [](const QStringRef &s, Node::Orientation *v) { return EnumMap::enumFromStr(s, v); });
}

template<typename V>
bool parseProperty(const V &attrs,GraphObject:: PropSetFlags flags, const QString &typeName, const QString &propName, Slide::PlayMode *dst)
{
    return ::parseProperty<Slide::PlayMode>(attrs, flags, typeName, propName, Q3DS::Enum, dst, [](const QStringRef &s, Slide::PlayMode *v) { return EnumMap::enumFromStr(s, v); });
}

template<typename V>
bool parseProperty(const V &attrs, GraphObject::PropSetFlags flags, const QString &typeName, const QString &propName, Slide::InitialPlayState *dst)
{
    return ::parseProperty<Slide::InitialPlayState>(attrs, flags, typeName, propName, Q3DS::Enum, dst, [](const QStringRef &s, Slide::InitialPlayState *v) { return EnumMap::enumFromStr(s, v); });
}

template<typename V>
bool parseProperty(const V &attrs, GraphObject::PropSetFlags flags, const QString &typeName, const QString &propName, LayerNode::ProgressiveAA *dst)
{
    return ::parseProperty<LayerNode::ProgressiveAA>(attrs, flags, typeName, propName, Q3DS::Enum, dst, [](const QStringRef &s, LayerNode::ProgressiveAA *v) { return EnumMap::enumFromStr(s, v); });
}

template<typename V>
bool parseProperty(const V &attrs, GraphObject::PropSetFlags flags, const QString &typeName, const QString &propName, LayerNode::MultisampleAA *dst)
{
    return ::parseProperty<LayerNode::MultisampleAA>(attrs, flags, typeName, propName, Q3DS::Enum, dst, [](const QStringRef &s, LayerNode::MultisampleAA *v) { return EnumMap::enumFromStr(s, v); });
}

template<typename V>
bool parseProperty(const V &attrs, GraphObject::PropSetFlags flags, const QString &typeName, const QString &propName, LayerNode::LayerBackground *dst)
{
    return ::parseProperty<LayerNode::LayerBackground>(attrs, flags, typeName, propName, Q3DS::Enum, dst, [](const QStringRef &s, LayerNode::LayerBackground *v) { return EnumMap::enumFromStr(s, v); });
}

template<typename V>
bool parseProperty(const V &attrs, GraphObject::PropSetFlags flags, const QString &typeName, const QString &propName, LayerNode::BlendType *dst)
{
    return ::parseProperty<LayerNode::BlendType>(attrs, flags, typeName, propName, Q3DS::Enum, dst, [](const QStringRef &s, LayerNode::BlendType *v) { return EnumMap::enumFromStr(s, v); });
}

template<typename V>
bool parseProperty(const V &attrs, GraphObject::PropSetFlags flags, const QString &typeName, const QString &propName, LayerNode::HorizontalFields *dst)
{
    return ::parseProperty<LayerNode::HorizontalFields>(attrs, flags, typeName, propName, Q3DS::Enum, dst, [](const QStringRef &s, LayerNode::HorizontalFields *v) { return EnumMap::enumFromStr(s, v); });
}

template<typename V>
bool parseProperty(const V &attrs, GraphObject::PropSetFlags flags, const QString &typeName, const QString &propName, LayerNode::Units *dst)
{
    return ::parseProperty<LayerNode::Units>(attrs, flags, typeName, propName, Q3DS::Enum, dst, [](const QStringRef &s, LayerNode::Units *v) { return EnumMap::enumFromStr(s, v); });
}

template<typename V>
bool parseProperty(const V &attrs, GraphObject::PropSetFlags flags, const QString &typeName, const QString &propName, LayerNode::VerticalFields *dst)
{
    return ::parseProperty<LayerNode::VerticalFields>(attrs, flags, typeName, propName, Q3DS::Enum, dst, [](const QStringRef &s, LayerNode::VerticalFields *v) { return EnumMap::enumFromStr(s, v); });
}

template<typename V>
bool parseProperty(const V &attrs, GraphObject::PropSetFlags flags, const QString &typeName, const QString &propName, Image::MappingMode *dst)
{
    return ::parseProperty<Image::MappingMode>(attrs, flags, typeName, propName, Q3DS::Enum, dst, [](const QStringRef &s, Image::MappingMode *v) { return EnumMap::enumFromStr(s, v); });
}

template<typename V>
bool parseProperty(const V &attrs, GraphObject::PropSetFlags flags, const QString &typeName, const QString &propName, Image::TilingMode *dst)
{
    return ::parseProperty<Image::TilingMode>(attrs, flags, typeName, propName, Q3DS::Enum, dst, [](const QStringRef &s, Image::TilingMode *v) { return EnumMap::enumFromStr(s, v); });
}

template<typename V>
bool parseProperty(const V &attrs, GraphObject::PropSetFlags flags, const QString &typeName, const QString &propName, ModelNode::Tessellation *dst)
{
    return ::parseProperty<ModelNode::Tessellation>(attrs, flags, typeName, propName, Q3DS::Enum, dst, [](const QStringRef &s, ModelNode::Tessellation *v) { return EnumMap::enumFromStr(s, v); });
}

template<typename V>
bool parseProperty(const V &attrs, GraphObject::PropSetFlags flags, const QString &typeName, const QString &propName, LightNode::LightType *dst)
{
    return ::parseProperty<LightNode::LightType>(attrs, flags, typeName, propName, Q3DS::Enum, dst, [](const QStringRef &s, LightNode::LightType *v) { return EnumMap::enumFromStr(s, v); });
}

template<typename V>
bool parseProperty(const V &attrs, GraphObject::PropSetFlags flags, const QString &typeName, const QString &propName, DefaultMaterial::ShaderLighting *dst)
{
    return ::parseProperty<DefaultMaterial::ShaderLighting>(attrs, flags, typeName, propName, Q3DS::Enum, dst, [](const QStringRef &s, DefaultMaterial::ShaderLighting *v) { return EnumMap::enumFromStr(s, v); });
}

template<typename V>
bool parseProperty(const V &attrs, GraphObject::PropSetFlags flags, const QString &typeName, const QString &propName, DefaultMaterial::BlendMode *dst)
{
    return ::parseProperty<DefaultMaterial::BlendMode>(attrs, flags, typeName, propName, Q3DS::Enum, dst, [](const QStringRef &s, DefaultMaterial::BlendMode *v) { return EnumMap::enumFromStr(s, v); });
}

template<typename V>
bool parseProperty(const V &attrs, GraphObject::PropSetFlags flags, const QString &typeName, const QString &propName, DefaultMaterial::SpecularModel *dst)
{
    return ::parseProperty<DefaultMaterial::SpecularModel>(attrs, flags, typeName, propName, Q3DS::Enum, dst, [](const QStringRef &s, DefaultMaterial::SpecularModel *v) { return EnumMap::enumFromStr(s, v); });
}

template<typename V>
bool parseProperty(const V &attrs, GraphObject::PropSetFlags flags, const QString &typeName, const QString &propName, TextNode::HorizontalAlignment *dst)
{
    return ::parseProperty<TextNode::HorizontalAlignment>(attrs, flags, typeName, propName, Q3DS::Enum, dst, [](const QStringRef &s, TextNode::HorizontalAlignment *v) { return EnumMap::enumFromStr(s, v); });
}

template<typename V>
bool parseProperty(const V &attrs, GraphObject::PropSetFlags flags, const QString &typeName, const QString &propName, TextNode::VerticalAlignment *dst)
{
    return ::parseProperty<TextNode::VerticalAlignment>(attrs, flags, typeName, propName, Q3DS::Enum, dst, [](const QStringRef &s, TextNode::VerticalAlignment *v) { return EnumMap::enumFromStr(s, v); });
}

template<typename V>
bool parseProperty(const V &attrs, GraphObject::PropSetFlags flags, const QString &typeName, const QString &propName, TextNode::WordWrap *dst)
{
    return ::parseProperty<TextNode::WordWrap>(attrs, flags, typeName, propName, Q3DS::Enum, dst, [](const QStringRef &s, TextNode::WordWrap *v) { return EnumMap::enumFromStr(s, v); });
}

template<typename V>
bool parseProperty(const V &attrs, GraphObject::PropSetFlags flags, const QString &typeName, const QString &propName, TextNode::Elide *dst)
{
    return ::parseProperty<TextNode::Elide>(attrs, flags, typeName, propName, Q3DS::Enum, dst, [](const QStringRef &s, TextNode::Elide *v) { return EnumMap::enumFromStr(s, v); });
}

template<typename V>
void GraphObject::setProps(const V &attrs, PropSetFlags flags)
{
    const QString typeName = QStringLiteral("Asset");
    parseProperty(attrs, flags, typeName, QStringLiteral("starttime"), &m_startTime);
    parseProperty(attrs, flags, typeName, QStringLiteral("endtime"), &m_endTime);

    // name is not parsed here since the data model metadata defines a default
    // value per type, so leave it to the subclasses
}

void GraphObject::setProperties(const QXmlStreamAttributes &attrs, PropSetFlags flags)
{
    setProps(attrs, flags);
}

void GraphObject::applyPropertyChanges(const PropertyChangeList &changeList)
{
    setProps(changeList, {});
}

void GraphObject::writeQmlFooter(QTextStream &output, int tabLevel)
{
    output << QSSGQmlUtilities::insertTabs(tabLevel) << "}\n";
}

QString GraphObject::qmlId()
{
    // does not matter
    return QSSGQmlUtilities::sanitizeQmlId(m_id);
}

void GraphObject::destroyGraph()
{
    if (m_parent) {
        m_parent->removeChildNode(this);
        Q_ASSERT(!m_parent);
    }
    while (m_firstChild) {
        GraphObject *child = m_firstChild;
        removeChildNode(child);
        Q_ASSERT(!child->m_parent);
        delete child;
    }
    Q_ASSERT(!m_firstChild && !m_lastChild);
}

Scene::Scene()
    : GraphObject(GraphObject::Scene)
{

}

Scene::~Scene()
{
    destroyGraph();
}

void Scene::setProperties(const QXmlStreamAttributes &attrs, PropSetFlags flags)
{
    // Asset properties (starttime, endtime) are not in use, hence no base call.

    const QString typeName = QStringLiteral("Scene");
    parseProperty(attrs, flags, typeName, QStringLiteral("bgcolorenable"), &m_useClearColor);
    parseProperty(attrs, flags, typeName, QStringLiteral("backgroundcolor"), &m_clearColor);

    // Different default value.
    parseProperty(attrs, flags, typeName, QStringLiteral("name"), &m_name);
}

void Scene::writeQmlHeader(QTextStream &output, int tabLevel)
{
    Q_UNUSED(output)
    Q_UNUSED(tabLevel)
}

void Scene::writeQmlProperties(QTextStream &output, int tabLevel, bool isInRootLevel)
{
    Q_UNUSED(output)
    Q_UNUSED(tabLevel)
    Q_UNUSED(isInRootLevel)
}

void Scene::writeQmlProperties(const PropertyChangeList &changeList, QTextStream &output, int tabLevel)
{
    Q_UNUSED(changeList)
    Q_UNUSED(output)
    Q_UNUSED(tabLevel)
}

void Scene::writeQmlFooter(QTextStream &output, int tabLevel)
{
    Q_UNUSED(output);
    Q_UNUSED(tabLevel);
}

Slide::Slide()
    : GraphObject(GraphObject::Slide)
{

}

Slide::~Slide()
{
    if (!parent())
        destroyGraph();

    qDeleteAll(m_propChanges);
}

template<typename V>
void Slide::setProps(const V &attrs, PropSetFlags flags)
{
    const QString typeName = QStringLiteral("Slide");
    parseProperty(attrs, flags, typeName, QStringLiteral("playmode"), &m_playMode);
    parseProperty(attrs, flags, typeName, QStringLiteral("initialplaystate"), &m_initialPlayState);

    StringOrInt pt;
    if (parseProperty(attrs, flags, typeName, QStringLiteral("playthroughto"), &pt)) {
        const bool isRef = (!pt.isInt && pt.s.startsWith(QLatin1Char('#')));
        const bool hasExplicitValue = (pt.isInt || isRef);
        if (hasExplicitValue) {
            m_playThrough = Value;
            m_playThroughValue = isRef ? QVariant::fromValue(pt.s) : QVariant::fromValue(pt.n);
        } else {
            EnumMap::enumFromStr(QStringRef(&pt.s), &m_playThrough);
        }
    }

    // Different default value.
    parseProperty(attrs, flags, typeName, QStringLiteral("name"), &m_name);
}

void Slide::setProperties(const QXmlStreamAttributes &attrs, PropSetFlags flags)
{
    GraphObject::setProperties(attrs, flags);
    setProps(attrs, flags);
}

void Slide::applyPropertyChanges(const PropertyChangeList &changeList)
{
    GraphObject::applyPropertyChanges(changeList);
    setProps(changeList, {});
}

void Slide::addObject(GraphObject *obj)
{
    m_objects.insert(obj);
}

void Slide::removeObject(GraphObject *obj)
{
    auto it = m_objects.find(obj);
    if (it != m_objects.end()) {
        m_objects.erase(it);
    }
}

void Slide::addPropertyChanges(GraphObject *target, PropertyChangeList *changeList)
{
    m_propChanges.insert(target, changeList);
}

void Slide::removePropertyChanges(GraphObject *target)
{
    delete takePropertyChanges(target);
}

PropertyChangeList *Slide::takePropertyChanges(GraphObject *target)
{
    auto it = m_propChanges.find(target);
    if (it != m_propChanges.end()) {
        PropertyChangeList *propChanges = *it;
        m_propChanges.erase(it);
        return propChanges;
    }
    return nullptr;
}

void Slide::addAnimation(const AnimationTrack &track)
{
    m_anims.append(track);
}

void Slide::removeAnimation(const AnimationTrack &track)
{
    const int idx = m_anims.indexOf(track);
    if (idx >= 0) {
        m_anims.removeAt(idx);
    }
}

void Slide::writeQmlHeader(QTextStream &output, int tabLevel)
{
    Q_UNUSED(output)
    Q_UNUSED(tabLevel)
}

void Slide::writeQmlProperties(QTextStream &output, int tabLevel, bool isInRootLevel)
{
    Q_UNUSED(output)
    Q_UNUSED(tabLevel)
    Q_UNUSED(isInRootLevel)
}

void Slide::writeQmlProperties(const PropertyChangeList &changeList, QTextStream &output, int tabLevel)
{
    Q_UNUSED(changeList)
    Q_UNUSED(output)
    Q_UNUSED(tabLevel)
}

void Slide::writeQmlFooter(QTextStream &output, int tabLevel)
{
    Q_UNUSED(output)
    Q_UNUSED(tabLevel)
}

Image::Image()
    : GraphObject(GraphObject::Image)
{

}

void Image::setProperties(const QXmlStreamAttributes &attrs, PropSetFlags flags)
{
    GraphObject::setProperties(attrs, flags);
    setProps(attrs, flags);
}

void Image::applyPropertyChanges(const PropertyChangeList &changeList)
{
    GraphObject::applyPropertyChanges(changeList);
    setProps(changeList, {});
}

bool Image::isDefaultScaleAndRotation()
{
    return m_scaleU == 1.0f && m_scaleV == 1.0f && m_rotationUV == 0.0f;
}

void Image::writeQmlHeader(QTextStream &output, int tabLevel)
{
    output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("Texture {\n");
}

namespace  {
QString mappingModeToString(Image::MappingMode mode)
{
    switch (mode) {
    case Image::EnvironmentalMapping:
        return QStringLiteral("Texture.Environment");
    case Image::LightProbe:
    case Image::IBLOverride:
        return QStringLiteral("Texture.LightProbe");
    default:
        return QStringLiteral("Texture.UV");
    }
}

QString tilingModeToString(Image::TilingMode mode)
{
    switch (mode) {
    case Image::Tiled:
        return QStringLiteral("Texture.Repeat");
    case Image::Mirrored:
        return QStringLiteral("Texture.MirroredRepeat");
    case Image::NoTiling:
        return QStringLiteral("Texture.ClampToEdge");
    }

    Q_ASSERT(false);
    return QString();
}
}

void Image::writeQmlProperties(QTextStream &output, int tabLevel, bool isInRootLevel)
{
    Q_UNUSED(isInRootLevel)
    output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("id: ") << qmlId() << Qt::endl;
    if (m_subPresentation.isEmpty()) {
        // if there is no sub-presentation, there is a source
        QString sanitizedSource = QSSGQmlUtilities::sanitizeQmlSourcePath(m_sourcePath, true);
        if (!isInRootLevel)
            sanitizedSource.insert(1, QLatin1String("../"));
        output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("source: ") <<  sanitizedSource << Qt::endl;
    } else {
        output << QSSGQmlUtilities::insertTabs(tabLevel) << "sourceItem: "
            << QSSGQmlUtilities::qmlComponentName(m_subPresentation) << " { }\n";
    }
    writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("scaleu"), m_scaleU);
    writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("scalev"), m_scaleV);
    writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("mappingmode"), mappingModeToString(m_mappingMode));
    writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("tilingmodehorz"), tilingModeToString(m_tilingHoriz));
    writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("tilingmodevert"), tilingModeToString(m_tilingVert));
    writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("rotationuv"), m_rotationUV);
    if (isDefaultScaleAndRotation()) {
        // As positionU/V means different between Qt3DStudio and QtQuick3D,
        // this converts the value in most of the cases.
        writeQmlPropertyHelper(output, tabLevel, type(),
                               QStringLiteral("positionu"), m_positionU + m_pivotU);
        writeQmlPropertyHelper(output, tabLevel, type(),
                               QStringLiteral("positionv"), m_positionV + m_pivotV);
    } else {
        // In some cases, the value needs to be fixed manually afterwards
        // as there is no easy conversion.
        writeQmlPropertyHelper(output, tabLevel, type(),
                               QStringLiteral("positionu"), m_positionU);
        writeQmlPropertyHelper(output, tabLevel, type(),
                               QStringLiteral("positionv"), m_positionV);
    }
    writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("pivotu"), m_pivotU);
    writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("pivotv"), m_pivotV);
}

void Image::writeQmlProperties(const PropertyChangeList &changeList, QTextStream &output, int tabLevel)
{
    // apply the changes so the values are translated
    applyPropertyChanges(changeList);
    for (auto change : changeList) {
        QString targetProperty = change.nameStr();
        if (targetProperty == QStringLiteral("sourcepath")) {
            output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("source: ") <<  QSSGQmlUtilities::sanitizeQmlSourcePath(m_sourcePath) << Qt::endl;
        } else if (targetProperty == QStringLiteral("subpresentation")) {
            output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("sourceItem: ") << QSSGQmlUtilities::qmlComponentName(m_subPresentation) << " { }\n";
        } else if (targetProperty == QStringLiteral("scaleu")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("scaleu"), m_scaleU, true);
        } else if (targetProperty == QStringLiteral("scalev")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("scalev"), m_scaleV, true);
        } else if (targetProperty == QStringLiteral("mappingmode")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("mappingmode"), mappingModeToString(m_mappingMode), true);
        } else if (targetProperty == QStringLiteral("tilingmodehorz")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("tilingmodehorz"), tilingModeToString(m_tilingHoriz), true);
        } else if (targetProperty == QStringLiteral("tilingmodevert")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("tilingmodevert"), tilingModeToString(m_tilingVert), true);
        } else if (targetProperty == QStringLiteral("rotationuv")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("rotationuv"), m_rotationUV, true);
        } else if (targetProperty == QStringLiteral("positionu")) {
            if (isDefaultScaleAndRotation()) {
                // As positionU means different between Qt3DStudio and QtQuick3D,
                // this converts the value in most of the cases.
                writeQmlPropertyHelper(output, tabLevel, type(),
                                       QStringLiteral("positionu"), m_positionU + m_pivotU, true);
            } else {
                // In some cases, the value needs to be fixed manually afterwards
                // as there is no easy conversion.
                writeQmlPropertyHelper(output, tabLevel, type(),
                                       QStringLiteral("positionu"), m_positionU, true);
            }
        } else if (targetProperty == QStringLiteral("positionv")) {
            if (isDefaultScaleAndRotation()) {
                // As positionV means different between Qt3DStudio and QtQuick3D,
                // this converts the value in most of the cases.
                writeQmlPropertyHelper(output, tabLevel, type(),
                                       QStringLiteral("positionv"), m_positionV + m_pivotV, true);
            } else {
                // In some cases, the value needs to be fixed manually afterwards
                // as there is no easy conversion.
                writeQmlPropertyHelper(output, tabLevel, type(),
                                       QStringLiteral("positionu"), m_positionV, true);
            }
        } else if (targetProperty == QStringLiteral("pivotu")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("pivotu"), m_pivotU, true);
        } else if (targetProperty == QStringLiteral("pivotv")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("pivotv"), m_pivotV, true);
        }
    }
}

template<typename V>
void Image::setProps(const V &attrs, PropSetFlags flags)
{
    const QString typeName = QStringLiteral("Image");
    parseProperty(attrs, flags, typeName, QStringLiteral("sourcepath"), &m_sourcePath);
    parseProperty(attrs, flags, typeName, QStringLiteral("scaleu"), &m_scaleU);
    parseProperty(attrs, flags, typeName, QStringLiteral("scalev"), &m_scaleV);
    parseProperty(attrs, flags, typeName, QStringLiteral("mappingmode"), &m_mappingMode);
    // Legacy behavior for light probes - default is tiled as opposed to ordinary image.
    // Therefore if a light probe does not have explicit horizontal tiling, set it to tiled.
    if (m_mappingMode == LightProbe || m_mappingMode == IBLOverride) {
        bool res = parseProperty(attrs, {}, typeName,
                                 QStringLiteral("tilingmodehorz"), &m_tilingHoriz);
        if (!res)
            m_tilingHoriz = Tiled;
    } else {
        parseProperty(attrs, flags, typeName, QStringLiteral("tilingmodehorz"), &m_tilingHoriz);
    }
    parseProperty(attrs, flags, typeName, QStringLiteral("tilingmodevert"), &m_tilingVert);
    parseProperty(attrs, flags, typeName, QStringLiteral("rotationuv"), &m_rotationUV);
    parseProperty(attrs, flags, typeName, QStringLiteral("positionu"), &m_positionU);
    parseProperty(attrs, flags, typeName, QStringLiteral("positionv"), &m_positionV);
    parseProperty(attrs, flags, typeName, QStringLiteral("pivotu"), &m_pivotU);
    parseProperty(attrs, flags, typeName, QStringLiteral("pivotv"), &m_pivotV);
    parseProperty(attrs, flags, typeName, QStringLiteral("subpresentation"), &m_subPresentation);

    // Different default value.
    parseProperty(attrs, flags, typeName, QStringLiteral("name"), &m_name);
    parseProperty(attrs, flags, typeName, QStringLiteral("endtime"), &m_endTime);
}

Node::Node(GraphObject::Type type)
    : GraphObject(type)
{

}

void Node::setProperties(const QXmlStreamAttributes &attrs, PropSetFlags flags)
{
    GraphObject::setProperties(attrs, flags);
    setProps(attrs, flags);
}

void Node::applyPropertyChanges(const PropertyChangeList &changeList)
{
    GraphObject::applyPropertyChanges(changeList);
    setProps(changeList, {});
}

void Node::writeQmlHeader(QTextStream &output, int tabLevel)
{
    output << QSSGQmlUtilities::insertTabs(tabLevel) << "Node {\n";
}

namespace {
QString rotationOrderToString(Node::RotationOrder ro) {
    switch (ro) {
    case Node::XYZ:
        return QStringLiteral("Node.XYZ");
    case Node::YZX:
        return QStringLiteral("Node.YZX");
    case Node::ZXY:
        return QStringLiteral("Node.ZXY");
    case Node::XZY:
        return QStringLiteral("Node.XZY");
    case Node::YXZ:
        return QStringLiteral("Node.YXZ");
    case Node::ZYX:
        return QStringLiteral("Node.ZYX");
    case Node::XYZr:
        return QStringLiteral("Node.XYZr");
    case Node::YZXr:
        return QStringLiteral("Node.YZXr");
    case Node::ZXYr:
        return QStringLiteral("Node.ZXYr");
    case Node::XZYr:
        return QStringLiteral("Node.XZYr");
    case Node::YXZr:
        return QStringLiteral("Node.YXZr");
    case Node::ZYXr:
        return QStringLiteral("Node.ZYXr");
    }

    Q_ASSERT(false);
    return QString();
}
}

void Node::writeQmlProperties(QTextStream &output, int tabLevel, bool isInRootLevel)
{
    Q_UNUSED(isInRootLevel)
    const float handednessAdjustment = (m_orientation == Node::LeftHanded) ? -1.0f : 1.0f;
    output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("id: ") << qmlId() << Qt::endl;
    writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("position.x"), m_position.x());
    writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("position.y"), m_position.y());
    writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("position.z"), m_position.z() * handednessAdjustment);
    writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("rotation.x"), m_rotation.x() * handednessAdjustment);
    writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("rotation.y"), m_rotation.y() * handednessAdjustment);
    writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("rotation.z"), m_rotation.z());
    if (type() != Light) {
        writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("scale.x"), m_scale.x());
        writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("scale.y"), m_scale.y());
        writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("scale.z"), m_scale.z());
    }
    writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("pivot.x"), m_pivot.x());
    writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("pivot.y"), m_pivot.y());
    writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("pivot.z"), m_pivot.z());
    writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("opacity"), m_localOpacity * 0.01f);
    writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("visible"), m_flags.testFlag(Node::Active));
}

void Node::writeQmlProperties(const PropertyChangeList &changeList, QTextStream &output, int tabLevel)
{
    // apply the changes so the values are translated
    applyPropertyChanges(changeList);
    const float handednessAdjustment = (m_orientation == Node::LeftHanded) ? -1.0f : 1.0f;
    for (auto change : changeList) {
        QString targetProperty = change.nameStr();
        if (targetProperty == QStringLiteral("position")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("position.x"), m_position.x(), true);
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("position.y"), m_position.y(), true);
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("position.z"), m_position.z() * handednessAdjustment, true);
        } else if (targetProperty == QStringLiteral("rotation")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("rotation.x"), m_rotation.x() * handednessAdjustment, true);
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("rotation.y"), m_rotation.y() * handednessAdjustment, true);
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("rotation.z"), m_rotation.z(), true);
        } else if (targetProperty == QStringLiteral("scale")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("scale.x"), m_scale.x(), true);
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("scale.y"), m_scale.y(), true);
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("scale.z"), m_scale.z(), true);
        } else if (targetProperty == QStringLiteral("pivot")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("pivot.x"), m_pivot.x(), true);
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("pivot.y"), m_pivot.y(), true);
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("pivot.z"), m_pivot.z(), true);
        } else if (targetProperty == QStringLiteral("opacity")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("opacity"), m_localOpacity * 0.01f, true);
        } else if (targetProperty == QStringLiteral("rotationorder")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("rotationorder"), rotationOrderToString(m_rotationOrder), true);
        } else if (targetProperty == QStringLiteral("visible")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("visible"), m_flags.testFlag(Node::Active), true);
        }
    }
}

template<typename V>
void Node::setProps(const V &attrs, PropSetFlags flags)
{
    const QString typeName = QStringLiteral("Node");

    bool b;
    if (parseProperty(attrs, flags, typeName, QStringLiteral("eyeball"), &b))
        m_flags.setFlag(Active, b);
    if (parseProperty(attrs, flags, typeName, QStringLiteral("ignoresparent"), &b))
        m_flags.setFlag(IgnoresParentTransform, b);

    parseRotationProperty(attrs, flags, typeName, QStringLiteral("rotation"), &m_rotation);
    parseProperty(attrs, flags, typeName, QStringLiteral("position"), &m_position);
    parseProperty(attrs, flags, typeName, QStringLiteral("scale"), &m_scale);
    parseProperty(attrs, flags, typeName, QStringLiteral("pivot"), &m_pivot);
    parseProperty(attrs, flags, typeName, QStringLiteral("opacity"), &m_localOpacity);
    parseProperty(attrs, flags, typeName, QStringLiteral("boneid"), &m_skeletonId);
    parseProperty(attrs, flags, typeName, QStringLiteral("rotationorder"), &m_rotationOrder);
    parseProperty(attrs, flags, typeName, QStringLiteral("orientation"), &m_orientation);
}

LayerNode::LayerNode()
    : Node(GraphObject::Layer)
{

}

void LayerNode::setProperties(const QXmlStreamAttributes &attrs, PropSetFlags flags)
{
    Node::setProperties(attrs, flags);
    setProps(attrs, flags);
}

void LayerNode::applyPropertyChanges(const PropertyChangeList &changeList)
{
    Node::applyPropertyChanges(changeList);
    setProps(changeList, {});
}

void LayerNode::writeQmlHeader(QTextStream &output, int tabLevel)
{
    // If there is a sub-presentation, just use that component instead
    if (m_sourcePath.isEmpty())
        output << QSSGQmlUtilities::insertTabs(tabLevel) << "View3D {\n";
    else
        output << QSSGQmlUtilities::insertTabs(tabLevel) << QSSGQmlUtilities::qmlComponentName(m_sourcePath) << " {\n";
}

namespace {
QString antialiasingQualityToString(int quality)
{
    switch (quality) {
    case 1: //2x
        return QStringLiteral("SceneEnvironment.Medium");
    case 2: //4x
        return QStringLiteral("SceneEnvironment.High");
    case 3: //8x
        return QStringLiteral("SceneEnvironment.VeryHigh");
    }

    Q_ASSERT(false);
    return QString();
}

QString layerBackgroundToString(LayerNode::LayerBackground mode)
{
    switch (mode) {
    case LayerNode::Transparent:
        return QStringLiteral("SceneEnvironment.Transparent");
    case LayerNode::SolidColor:
        return QStringLiteral("SceneEnvironment.Color");
    case LayerNode::Unspecified:
        return QStringLiteral("SceneEnvironment.Unspecified");
    }

    Q_ASSERT(false);
    return QString();
}

#if 0
QString blendTypeToString(LayerNode::BlendType type)
{
    switch (type) {
    case LayerNode::Normal:
        return QStringLiteral("SceneEnvironment.Normal");
    case LayerNode::Screen:
        return QStringLiteral("SceneEnvironment.Screen");
    case LayerNode::Multiply:
        return QStringLiteral("SceneEnvironment.Multiply");
    case LayerNode::Add:
        return QStringLiteral("SceneEnvironment.Add");
    case LayerNode::Subtract:
        return QStringLiteral("SceneEnvironment.Subtract");
    case LayerNode::Overlay:
        return QStringLiteral("SceneEnvironment.Overlay");
    case LayerNode::ColorBurn:
        return QStringLiteral("SceneEnvironment.ColorBurn");
    case LayerNode::ColorDodge:
        return QStringLiteral("DemonSceneEnvironment.ColorDodge");
    }

    Q_ASSERT(false);
    return QString();
}
#endif

}

// This helper method makes sure that max one antialiasingMode property is used per layer.
// Note: If UIP contains both "progressiveaa" and "multisampleaa", ProgressiveAA is used.
void LayerNode::outputAAModeAndQuality(QTextStream &output, int tabLevel, const QString &propertyName)
{
    if (!m_antialiasingSet) {
        if (m_progressiveAA != NoPAA) {
            m_antialiasingSet = true;
            int quality = int(m_progressiveAA);
            output << QSSGQmlUtilities::insertTabs(tabLevel) << "antialiasingMode: SceneEnvironment.ProgressiveAA" << Qt::endl;
            writeQmlPropertyHelper(output, tabLevel, type(), propertyName, antialiasingQualityToString(quality));
        } else if (m_multisampleAA != NoMSAA) {
            m_antialiasingSet = true;
            QString aaType = (m_multisampleAA == MultisampleAA::SSAA) ? "SSAA" : "MSAA";
            // SSAA quality is always VeryHigh (2.0) for compatibility
            int quality = (m_multisampleAA == MultisampleAA::SSAA) ? 3 : int(m_multisampleAA);
            output << QSSGQmlUtilities::insertTabs(tabLevel) << "antialiasingMode: SceneEnvironment." << aaType << Qt::endl;
            writeQmlPropertyHelper(output, tabLevel, type(), propertyName, antialiasingQualityToString(quality));
        }
    }
}

void LayerNode::writeQmlProperties(QTextStream &output, int tabLevel, bool isInRootLevel)
{
    Q_UNUSED(isInRootLevel)
    output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("id: ") << qmlId() << Qt::endl;
    //need to manually call visible flag here
    writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("visible"), m_flags.testFlag(Node::Active));

    // QQuickItem position/anchors
    if (m_horizontalFields == LeftWidth) {
        // left anchor
        output << QSSGQmlUtilities::insertTabs(tabLevel) << "anchors.left: parent.left\n";
        if (m_leftUnits == Pixels)
            output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("anchors.leftMargin: ") << m_left << Qt::endl;
        else
            output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("anchors.leftMargin: parent.width * ") << m_left * 0.01f << Qt::endl;

        // width
        if (m_widthUnits == Pixels)
            output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("width: ") << m_width << Qt::endl;
        else
            output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("width: parent.width * ") << m_width * 0.01f << Qt::endl;

    } else if (m_horizontalFields == LeftRight) {
        // left anchor
        output << QSSGQmlUtilities::insertTabs(tabLevel) << "anchors.left: parent.left\n";
        if (m_leftUnits == Pixels)
            output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("anchors.leftMargin: ") << m_left << Qt::endl;
        else
            output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("anchors.leftMargin: parent.width * ") << m_left * 0.01f << Qt::endl;

        // right anchor
        output << QSSGQmlUtilities::insertTabs(tabLevel) << "anchors.right: parent.right\n";
        if (m_rightUnits == Pixels)
            output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("anchors.rightMargin: ") << m_right << Qt::endl;
        else
            output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("anchors.rightMargin: parent.width * ") << m_right * 0.01f << Qt::endl;

    } else if (m_horizontalFields == WidthRight) {
        // width
        if (m_widthUnits == Pixels)
            output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("width: ") << m_width << Qt::endl;
        else
            output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("width: parent.width * ") << m_width * 0.01f << Qt::endl;

        // right anchor
        output << QSSGQmlUtilities::insertTabs(tabLevel) << "anchors.right: parent.right\n";
        if (m_rightUnits == Pixels)
            output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("anchors.rightMargin: ") << m_right << Qt::endl;
        else
            output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("anchors.rightMargin: parent.width * ") << m_right * 0.01f << Qt::endl;
    }

    if (m_verticalFields == TopHeight) {
        // top anchor
        output << QSSGQmlUtilities::insertTabs(tabLevel) << "anchors.top: parent.top\n";
        if (m_topUnits == Pixels)
            output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("anchors.topMargin: ") << m_top << Qt::endl;
        else
            output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("anchors.topMargin: parent.height * ") << m_top * 0.01f << Qt::endl;

        // height
        if (m_heightUnits == Pixels)
            output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("height: ") << m_height << Qt::endl;
        else
            output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("height: parent.height * ") << m_height * 0.01f << Qt::endl;

    } else if (m_verticalFields == TopBottom) {
        // top anchor
        output << QSSGQmlUtilities::insertTabs(tabLevel) << "anchors.top: parent.top\n";
        if (m_topUnits == Pixels)
            output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("anchors.topMargin: ") << m_top << Qt::endl;
        else
            output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("anchors.topMargin: parent.height * ") << m_top * 0.01f << Qt::endl;

        // bottom anchor
        output << QSSGQmlUtilities::insertTabs(tabLevel) << "anchors.bottom: parent.bottom\n";
        if (m_bottomUnits == Pixels)
            output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("anchors.bottomMargin: ") << m_bottom << Qt::endl;
        else
            output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("anchors.bottomMargin: parent.height * ") << m_bottom * 0.01f << Qt::endl;


    } else if (m_verticalFields == HeightBottom) {
        // height
        if (m_heightUnits == Pixels)
            output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("height: ") << m_height << Qt::endl;
        else
            output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("height: parent.height * ") << m_height * 0.01f << Qt::endl;

        // bottom anchor
        output << QSSGQmlUtilities::insertTabs(tabLevel) << "anchors.bottom: parent.bottom\n";
        if (m_bottomUnits == Pixels)
            output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("anchors.bottomMargin: ") << m_bottom << Qt::endl;
        else
            output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("anchors.bottomMargin: parent.height * ") << m_bottom * 0.01f << Qt::endl;
    }

    if (m_sourcePath.isEmpty()) {
        // SceneEnvironment Properties (seperate component)
        output << QSSGQmlUtilities::insertTabs(tabLevel) << "environment: SceneEnvironment {\n";
        outputAAModeAndQuality(output, tabLevel + 1, QStringLiteral("progressiveaa"));
        outputAAModeAndQuality(output, tabLevel + 1, QStringLiteral("multisampleaa"));
        writeQmlPropertyHelper(output, tabLevel + 1, type(), QStringLiteral("background"), layerBackgroundToString(m_layerBackground));
        writeQmlPropertyHelper(output, tabLevel + 1, type(), QStringLiteral("backgroundcolor"), m_backgroundColor);
        //writeQmlPropertyHelper(output, tabLevel + 1, type(), QStringLiteral("blendtype"), blendTypeToString(m_blendType));

        writeQmlPropertyHelper(output, tabLevel + 1, type(), QStringLiteral("aostrength"), m_aoStrength);
        writeQmlPropertyHelper(output, tabLevel + 1, type(), QStringLiteral("aodistance"), m_aoDistance);
        writeQmlPropertyHelper(output, tabLevel + 1, type(), QStringLiteral("aosoftness"), m_aoSoftness);
        writeQmlPropertyHelper(output, tabLevel + 1, type(), QStringLiteral("aodither"), m_aoDither);
        writeQmlPropertyHelper(output, tabLevel + 1, type(), QStringLiteral("aosamplerate"), m_aoSampleRate);
        writeQmlPropertyHelper(output, tabLevel + 1, type(), QStringLiteral("aobias"), m_aoBias);

        // disabledepth* maps to depth*Enabled in qml so an inversion of the flag is needed
        writeQmlPropertyHelper(output, tabLevel + 1, type(), QStringLiteral("disabledepthtest"),  !m_layerFlags.testFlag(DisableDepthTest));
        writeQmlPropertyHelper(output, tabLevel + 1, type(), QStringLiteral("disabledepthprepass"),  !m_layerFlags.testFlag(DisableDepthPrePass));

        if (!m_lightProbe_unresolved.isEmpty()) {
            output << QSSGQmlUtilities::insertTabs(tabLevel + 1) << "lightProbe: " << UniqueIdMapper::instance()->queryId(m_lightProbe_unresolved) << Qt::endl;
            writeQmlPropertyHelper(output, tabLevel + 1, type(), QStringLiteral("probebright"), m_probeBright);
            writeQmlPropertyHelper(output, tabLevel + 1, type(), QStringLiteral("fastibl"), m_layerFlags.testFlag(LayerNode::FastIBL));
            writeQmlPropertyHelper(output, tabLevel + 1, type(), QStringLiteral("probehorizon"), m_probeHorizon);
            writeQmlPropertyHelper(output, tabLevel + 1, type(), QStringLiteral("probefov"), m_probeFov);
        }

        writeQmlPropertyHelper(output, tabLevel + 1, type(), QStringLiteral("temporalaa"), (m_layerFlags.testFlag(LayerNode::TemporalAA)));
        output << QSSGQmlUtilities::insertTabs(tabLevel) << "}\n";
    }
}

void LayerNode::writeQmlProperties(const PropertyChangeList &changeList, QTextStream &output, int tabLevel)
{
    // apply the changes so the values are translated
    applyPropertyChanges(changeList);

    // TODO: Layer -> Item Anchors (requires a differnt type of change)

    for (auto change : changeList) {
        QString targetProperty = change.nameStr();
        if (targetProperty == QStringLiteral("progressiveaa")) {
            outputAAModeAndQuality(output, tabLevel + 1, QStringLiteral("environment.progressiveaa"));
        } else if (targetProperty == QStringLiteral("multisampleaa")) {
            outputAAModeAndQuality(output, tabLevel + 1, QStringLiteral("environment.multisampleaa"));
        } else if (targetProperty == QStringLiteral("background")) {
            writeQmlPropertyHelper(output, tabLevel + 1, type(), QStringLiteral("environment.background"), layerBackgroundToString(m_layerBackground));
        } else if (targetProperty == QStringLiteral("backgroundcolor")) {
            writeQmlPropertyHelper(output, tabLevel + 1, type(), QStringLiteral("environment.backgroundcolor"), m_backgroundColor);
        } /*else if (targetProperty == QStringLiteral("blendtype")) {
            writeQmlPropertyHelper(output, tabLevel + 1, type(), QStringLiteral("environment.blendtype"), blendTypeToString(m_blendType));
        }*/ else if (targetProperty == QStringLiteral("aostrength")) {
            writeQmlPropertyHelper(output, tabLevel + 1, type(), QStringLiteral("environment.aostrength"), m_aoStrength);
        } else if (targetProperty == QStringLiteral("aodistance")) {
            writeQmlPropertyHelper(output, tabLevel + 1, type(), QStringLiteral("environment.aodistance"), m_aoDistance);
        } else if (targetProperty == QStringLiteral("aosoftness")) {
            writeQmlPropertyHelper(output, tabLevel + 1, type(), QStringLiteral("environment.aosoftness"), m_aoSoftness);
        } else if (targetProperty == QStringLiteral("aodither")) {
            writeQmlPropertyHelper(output, tabLevel + 1, type(), QStringLiteral("environment.aodither"), m_aoDither);
        } else if (targetProperty == QStringLiteral("aosamplerate")) {
            writeQmlPropertyHelper(output, tabLevel + 1, type(), QStringLiteral("environment.aosamplerate"), m_aoSampleRate);
        } else if (targetProperty == QStringLiteral("aobias")) {
            writeQmlPropertyHelper(output, tabLevel + 1, type(), QStringLiteral("environment.aobias"), m_aoBias);
        } else if (targetProperty == QStringLiteral("disabledepthtest")) {
            writeQmlPropertyHelper(output, tabLevel + 1, type(), QStringLiteral("environment.disabledepthtest"), !m_layerFlags.testFlag(DisableDepthTest));
        } else if (targetProperty == QStringLiteral("disabledepthprepass")) {
            writeQmlPropertyHelper(output, tabLevel + 1, type(), QStringLiteral("environment.disabledepthprepass"), !m_layerFlags.testFlag(DisableDepthPrePass));
        } else if (targetProperty == QStringLiteral("temporalaa")) {
            writeQmlPropertyHelper(output, tabLevel + 1, type(), QStringLiteral("environment.temporalaa"), m_layerFlags.testFlag(LayerNode::TemporalAA));
        } else if (targetProperty == QStringLiteral("lightprobe")) {
            output << QSSGQmlUtilities::insertTabs(tabLevel + 1) << "environment.lightProbe: " << UniqueIdMapper::instance()->queryId(m_lightProbe_unresolved) << Qt::endl;
        } else if (targetProperty == QStringLiteral("probebright")) {
            writeQmlPropertyHelper(output, tabLevel + 1, type(), QStringLiteral("environment.probebright"), m_probeBright);
        } else if (targetProperty == QStringLiteral("fastibl")) {
            writeQmlPropertyHelper(output, tabLevel + 1, type(), QStringLiteral("environment.fastibl"), m_layerFlags.testFlag(LayerNode::FastIBL));
        } else if (targetProperty == QStringLiteral("probehorizon")) {
            writeQmlPropertyHelper(output, tabLevel + 1, type(), QStringLiteral("environment.probehorizon"), m_probeHorizon);
        } else if (targetProperty == QStringLiteral("probefov")) {
            writeQmlPropertyHelper(output, tabLevel + 1, type(), QStringLiteral("environment.probefov"), m_probeFov);
        } else if (targetProperty == QStringLiteral("eyeball")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("visible"), m_flags.testFlag(Node::Active));
        }
    }
}

template<typename V>
void LayerNode::setProps(const V &attrs, PropSetFlags flags)
{
    const QString typeName = QStringLiteral("Layer");

    bool b;
    if (parseProperty(attrs, flags, typeName, QStringLiteral("disabledepthtest"), &b))
        m_layerFlags.setFlag(DisableDepthTest, b);
    if (parseProperty(attrs, flags, typeName, QStringLiteral("disabledepthprepass"), &b))
        m_layerFlags.setFlag(DisableDepthPrePass, b);

    parseProperty(attrs, flags, typeName, QStringLiteral("progressiveaa"), &m_progressiveAA);
    parseProperty(attrs, flags, typeName, QStringLiteral("multisampleaa"), &m_multisampleAA);

    if (parseProperty(attrs, flags, typeName, QStringLiteral("temporalaa"), &b))
        m_layerFlags.setFlag(TemporalAA, b);

    parseProperty(attrs, flags, typeName, QStringLiteral("background"), &m_layerBackground);
    parseProperty(attrs, flags, typeName, QStringLiteral("backgroundcolor"), &m_backgroundColor);
    parseProperty(attrs, flags, typeName, QStringLiteral("blendtype"), &m_blendType);

    parseProperty(attrs, flags, typeName, QStringLiteral("horzfields"), &m_horizontalFields);
    parseProperty(attrs, flags, typeName, QStringLiteral("left"), &m_left);
    parseProperty(attrs, flags, typeName, QStringLiteral("leftunits"), &m_leftUnits);
    parseProperty(attrs, flags, typeName, QStringLiteral("width"), &m_width);
    parseProperty(attrs, flags, typeName, QStringLiteral("widthunits"), &m_widthUnits);
    parseProperty(attrs, flags, typeName, QStringLiteral("right"), &m_right);
    parseProperty(attrs, flags, typeName, QStringLiteral("rightunits"), &m_rightUnits);
    parseProperty(attrs, flags, typeName, QStringLiteral("vertfields"), &m_verticalFields);
    parseProperty(attrs, flags, typeName, QStringLiteral("top"), &m_top);
    parseProperty(attrs, flags, typeName, QStringLiteral("topunits"), &m_topUnits);
    parseProperty(attrs, flags, typeName, QStringLiteral("height"), &m_height);
    parseProperty(attrs, flags, typeName, QStringLiteral("heightunits"), &m_heightUnits);
    parseProperty(attrs, flags, typeName, QStringLiteral("bottom"), &m_bottom);
    parseProperty(attrs, flags, typeName, QStringLiteral("bottomunits"), &m_bottomUnits);

    parseProperty(attrs, flags, typeName, QStringLiteral("sourcepath"), &m_sourcePath);

    // SSAO
    parseProperty(attrs, flags, typeName, QStringLiteral("aostrength"), &m_aoStrength);
    parseProperty(attrs, flags, typeName, QStringLiteral("aodistance"), &m_aoDistance);
    parseProperty(attrs, flags, typeName, QStringLiteral("aosoftness"), &m_aoSoftness);
    parseProperty(attrs, flags, typeName, QStringLiteral("aobias"), &m_aoBias);
    parseProperty(attrs, flags, typeName, QStringLiteral("aosamplerate"), &m_aoSampleRate);
    parseProperty(attrs, flags, typeName, QStringLiteral("aodither"), &m_aoDither);

    // IBL
    parseImageProperty(attrs, flags, typeName, QStringLiteral("lightprobe"), &m_lightProbe_unresolved);
    parseProperty(attrs, flags, typeName, QStringLiteral("probebright"), &m_probeBright);
    if (parseProperty(attrs, flags, typeName, QStringLiteral("fastibl"), &b))
        m_layerFlags.setFlag(FastIBL, b);
    parseProperty(attrs, flags, typeName, QStringLiteral("probehorizon"), &m_probeHorizon);
    parseProperty(attrs, flags, typeName, QStringLiteral("probefov"), &m_probeFov);

    // Different default value.
    parseProperty(attrs, flags, typeName, QStringLiteral("name"), &m_name);
}

CameraNode::CameraNode()
    : Node(GraphObject::Camera)
{

}

void CameraNode::setProperties(const QXmlStreamAttributes &attrs, PropSetFlags flags)
{
    Node::setProperties(attrs, flags);
    setProps(attrs, flags);
}

void CameraNode::applyPropertyChanges(const PropertyChangeList &changeList)
{
    Node::applyPropertyChanges(changeList);
    setProps(changeList, {});
}

void CameraNode::writeQmlHeader(QTextStream &output, int tabLevel)
{
    if (m_orthographic)
        output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("OrthographicCamera {\n");
    else
        output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("PerspectiveCamera {\n");
}

void CameraNode::writeQmlProperties(QTextStream &output, int tabLevel, bool isInRootLevel)
{
    Q_UNUSED(isInRootLevel)
    Node::writeQmlProperties(output, tabLevel);
    writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("clipnear"), m_clipNear);
    writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("clipfar"), m_clipFar);
    writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("enablefrustumculling"),
                           m_frustumCulling);
    if (!m_orthographic) {
        writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("fov"), m_fov);
        writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("fovhorizontal"),
                               m_fovHorizontal ? QStringLiteral("Camera.Horizontal")
                                               : QStringLiteral("Camera.Vertical"));
    }
}

void CameraNode::writeQmlProperties(const PropertyChangeList &changeList, QTextStream &output, int tabLevel)
{
    Node::writeQmlProperties(changeList, output, tabLevel);

    // apply the changes so the values are translated
    applyPropertyChanges(changeList);

    for (auto change : changeList) {
        QString targetProperty = change.nameStr();
        if (targetProperty == QStringLiteral("clipnear")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("clipnear"), m_clipNear);
        } else if (targetProperty == QStringLiteral("clipfar")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("clipfar"), m_clipFar);
        } else if (targetProperty == QStringLiteral("enablefrustumculling")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("enablefrustumculling"),
                                   m_frustumCulling);
        } else if (targetProperty == QStringLiteral("fov")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("fov"), m_fov);
        } else if (targetProperty == QStringLiteral("fovhorizontal")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("fovhorizontal"),
                                   m_fovHorizontal ? QStringLiteral("Camera.Horizontal")
                                                   : QStringLiteral("Camera.Vertical"));
        }
    }
}

template<typename V>
void CameraNode::setProps(const V &attrs, PropSetFlags flags)
{
    const QString typeName = QStringLiteral("Camera");

    parseProperty(attrs, flags, typeName, QStringLiteral("orthographic"), &m_orthographic);
    parseProperty(attrs, flags, typeName, QStringLiteral("fov"), &m_fov);
    parseProperty(attrs, flags, typeName, QStringLiteral("fovhorizontal"), &m_fovHorizontal);
    parseProperty(attrs, flags, typeName, QStringLiteral("clipnear"), &m_clipNear);
    parseProperty(attrs, flags, typeName, QStringLiteral("clipfar"), &m_clipFar);
    parseProperty(attrs, flags, typeName, QStringLiteral("enablefrustumculling"),
                  &m_frustumCulling);
    // Different default value.
    parseProperty(attrs, flags, typeName, QStringLiteral("name"), &m_name);
    parseProperty(attrs, flags, typeName, QStringLiteral("position"), &m_position);
}

LightNode::LightNode()
    : Node(GraphObject::Light)
{

}

void LightNode::setProperties(const QXmlStreamAttributes &attrs, PropSetFlags flags)
{
    Node::setProperties(attrs, flags);
    setProps(attrs, flags);
}

void LightNode::applyPropertyChanges(const PropertyChangeList &changeList)
{
    Node::applyPropertyChanges(changeList);
    setProps(changeList, {});
}

namespace {
QString shadowMapQualityToString(qint32 res)
{
    switch (res) {
    case 8:
        return QStringLiteral("Light.ShadowMapQualityLow");
    case 9:
        return QStringLiteral("Light.ShadowMapQualityMedium");
    case 10:
        return QStringLiteral("Light.ShadowMapQualityHigh");
    case 11:
        return QStringLiteral("Light.ShadowMapQualityVeryHigh");
    default:
        qCritical() << QObject::tr("Undefined shadowmap quality '%1'").arg(res);
        return QString();
    }
}
}

void LightNode::writeQmlHeader(QTextStream &output, int tabLevel)
{
    switch (m_lightType) {
    case LightNode::Directional:
        output << QSSGQmlUtilities::insertTabs(tabLevel) << "DirectionalLight {\n";
        break;
    case LightNode::Point:
        output << QSSGQmlUtilities::insertTabs(tabLevel) << "PointLight {\n";
        break;
    case LightNode::Area:
        output << QSSGQmlUtilities::insertTabs(tabLevel) << "AreaLight {\n";
        break;
    }
}

void LightNode::writeQmlProperties(QTextStream &output, int tabLevel, bool isInRootLevel)
{
    Q_UNUSED(isInRootLevel)
    Node::writeQmlProperties(output, tabLevel);
    writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("lightdiffuse"), m_lightDiffuse);
    writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("lightspecular"), m_lightSpecular);
    writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("lightambient"), m_lightAmbient);
    writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("brightness"), m_brightness);
    if (m_lightType == LightNode::Point) {
        writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("constantfade"), m_constantFade);
        writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("linearfade"), m_linearFade);
        writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("expfade"), m_expFade);
    }
    if (m_lightType == LightNode::Area) {
        writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("areawidth"), m_areaWidth);
        writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("areaheight"), m_areaHeight);
    }
    writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("castshadow"), m_castShadow);
    writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("shdwbias"), m_shadowBias);
    writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("shdwfactor"), m_shadowFactor);
    writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("shdwmapres"),
                           shadowMapQualityToString(m_shadowMapRes));
    writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("shdwmapfar"), m_shadowMapFar);
    writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("shdwmapfov"), m_shadowMapFov);
    writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("shdwfilter"), m_shadowFilter);
}

void LightNode::writeQmlProperties(const PropertyChangeList &changeList, QTextStream &output, int tabLevel)
{
    Node::writeQmlProperties(changeList, output, tabLevel);

    // apply the changes so the values are translated
    applyPropertyChanges(changeList);

    for (auto change : changeList) {
        QString targetProperty = change.nameStr();
        if (targetProperty == QStringLiteral("lightdiffuse")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("lightdiffuse"), m_lightDiffuse);
        } else if (targetProperty == QStringLiteral("lightspecular")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("lightspecular"), m_lightSpecular);
        } else if (targetProperty == QStringLiteral("lightambient")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("lightambient"), m_lightAmbient);
        } else if (targetProperty == QStringLiteral("brightness")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("brightness"), m_brightness);
        } else if (targetProperty == QStringLiteral("constantfade")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("constantfade"), m_constantFade);
        } else if (targetProperty == QStringLiteral("linearfade")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("linearfade"), m_linearFade);
        } else if (targetProperty == QStringLiteral("expfade")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("expfade"), m_expFade);
        } else if (targetProperty == QStringLiteral("scale.x")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("areawidth"), m_areaWidth);
        } else if (targetProperty == QStringLiteral("scale.y")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("areaheight"), m_areaHeight);
        } else if (targetProperty == QStringLiteral("castshadow")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("castshadow"), m_castShadow);
        } else if (targetProperty == QStringLiteral("shdwbias")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("shdwbias"), m_shadowBias);
        } else if (targetProperty == QStringLiteral("shdwfactor")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("shdwfactor"), m_shadowFactor);
        } else if (targetProperty == QStringLiteral("shdwmapres")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("shdwmapres"),
                                   shadowMapQualityToString(m_shadowMapRes));
        } else if (targetProperty == QStringLiteral("shdwmapfar")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("shdwmapfar"), m_shadowMapFar);
        } else if (targetProperty == QStringLiteral("shdwmapfov")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("shdwmapfov"), m_shadowMapFov);
        } else if (targetProperty == QStringLiteral("shdwfilter")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("shdwfilter"), m_shadowFilter);
        }
    }
}

template<typename V>
void LightNode::setProps(const V &attrs, PropSetFlags flags)
{
    const QString typeName = QStringLiteral("Light");

    parseObjectRefProperty(attrs, flags, typeName, QStringLiteral("scope"), &m_scope_unresolved);

    parseProperty(attrs, flags, typeName, QStringLiteral("lighttype"), &m_lightType);
    parseProperty(attrs, flags, typeName, QStringLiteral("lightdiffuse"), &m_lightDiffuse);
    parseProperty(attrs, flags, typeName, QStringLiteral("lightspecular"), &m_lightSpecular);
    parseProperty(attrs, flags, typeName, QStringLiteral("lightambient"), &m_lightAmbient);
    parseProperty(attrs, flags, typeName, QStringLiteral("brightness"), &m_brightness);
    parseProperty(attrs, flags, typeName, QStringLiteral("constantfade"), &m_constantFade);
    parseProperty(attrs, flags, typeName, QStringLiteral("linearfade"), &m_linearFade);
    parseProperty(attrs, flags, typeName, QStringLiteral("expfade"), &m_expFade);
    parseProperty(attrs, flags, typeName, QStringLiteral("scale.x"), &m_areaWidth);
    parseProperty(attrs, flags, typeName, QStringLiteral("scale.y"), &m_areaHeight);
    QVector3D scale;
    parseProperty(attrs, flags, typeName, QStringLiteral("scale"), &scale);
    m_areaWidth = scale[0];
    m_areaHeight = scale[1];
    parseProperty(attrs, flags, typeName, QStringLiteral("castshadow"), &m_castShadow);
    parseProperty(attrs, flags, typeName, QStringLiteral("shdwfactor"), &m_shadowFactor);
    parseProperty(attrs, flags, typeName, QStringLiteral("shdwfilter"), &m_shadowFilter);
    parseProperty(attrs, flags, typeName, QStringLiteral("shdwmapres"), &m_shadowMapRes);
    parseProperty(attrs, flags, typeName, QStringLiteral("shdwbias"), &m_shadowBias);
    parseProperty(attrs, flags, typeName, QStringLiteral("shdwmapfar"), &m_shadowMapFar);
    parseProperty(attrs, flags, typeName, QStringLiteral("shdwmapfov"), &m_shadowMapFov);

    // Different default value.
    parseProperty(attrs, flags, typeName, QStringLiteral("name"), &m_name);
}

ModelNode::ModelNode()
    : Node(GraphObject::Model)
{

}

ModelNode::~ModelNode()
{

}

void ModelNode::setProperties(const QXmlStreamAttributes &attrs, PropSetFlags flags)
{
    Node::setProperties(attrs, flags);
    setProps(attrs, flags);
}

void ModelNode::applyPropertyChanges(const PropertyChangeList &changeList)
{
    Node::applyPropertyChanges(changeList);
    setProps(changeList, {});
}

void ModelNode::writeQmlHeader(QTextStream &output, int tabLevel)
{
    output << QSSGQmlUtilities::insertTabs(tabLevel) << "Model {\n";
}

namespace {
QString tesselationModeToString(ModelNode::Tessellation mode)
{
    switch (mode) {
    case ModelNode::None:
        return QStringLiteral("Model.NoTessellation");
    case ModelNode::Linear:
        return QStringLiteral("Model.Linear");
    case ModelNode::Phong:
        return QStringLiteral("Model.Phong");
    case ModelNode::NPatch:
        return QStringLiteral("Model.NPatch");
    }
    Q_ASSERT(false);
    return QString();
}
}

void ModelNode::writeQmlProperties(QTextStream &output, int tabLevel, bool isInRootLevel)
{
    Node::writeQmlProperties(output, tabLevel);
    QString sanitizedSource = QSSGQmlUtilities::sanitizeQmlSourcePath(m_mesh_unresolved, true);
    if (!isInRootLevel)
        sanitizedSource.insert(1, QLatin1String("../"));
    output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("source: ") << sanitizedSource << Qt::endl;
    writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("tessellation"), tesselationModeToString(m_tessellation));
    writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("edgetess"), m_edgeTess);
    writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("innertess"), m_innerTess);
}

void ModelNode::writeQmlProperties(const PropertyChangeList &changeList, QTextStream &output, int tabLevel)
{
    Node::writeQmlProperties(changeList, output, tabLevel);

    // apply the changes so the values are translated
    applyPropertyChanges(changeList);

    for (auto change : changeList) {
        QString targetProperty = change.nameStr();
        if (targetProperty == QStringLiteral("source")) {
            output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("source: ") << QSSGQmlUtilities::sanitizeQmlSourcePath(m_mesh_unresolved) << Qt::endl;
        } else if (targetProperty == QStringLiteral("tessellation")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("tessellation"), tesselationModeToString(m_tessellation));
        } else if (targetProperty == QStringLiteral("edgetess")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("edgetess"), m_edgeTess);
        } else if (targetProperty == QStringLiteral("innertess")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("innertess"), m_innerTess);
        }
    }
}

template<typename V>
void ModelNode::setProps(const V &attrs, PropSetFlags flags)
{
    const QString typeName = QStringLiteral("Model");
    parseMeshProperty(attrs, flags, typeName, QStringLiteral("sourcepath"), &m_mesh_unresolved);
    parseProperty(attrs, flags, typeName, QStringLiteral("tessellation"), &m_tessellation);
    parseProperty(attrs, flags, typeName, QStringLiteral("edgetess"), &m_edgeTess);
    parseProperty(attrs, flags, typeName, QStringLiteral("innertess"), &m_innerTess);

    // Different default value.
    parseProperty(attrs, flags, typeName, QStringLiteral("name"), &m_name);
}

GroupNode::GroupNode()
    : Node(GraphObject::Group)
{

}

void GroupNode::setProperties(const QXmlStreamAttributes &attrs, PropSetFlags flags)
{
    Node::setProperties(attrs, flags);
    setProps(attrs, flags);
}

void GroupNode::applyPropertyChanges(const PropertyChangeList &changeList)
{
    Node::applyPropertyChanges(changeList);
    setProps(changeList, {});
}

void GroupNode::writeQmlHeader(QTextStream &output, int tabLevel)
{
    Node::writeQmlHeader(output, tabLevel);
}

void GroupNode::writeQmlProperties(QTextStream &output, int tabLevel, bool isInRootLevel)
{
    Q_UNUSED(isInRootLevel)
    Node::writeQmlProperties(output, tabLevel);
}

void GroupNode::writeQmlProperties(const PropertyChangeList &changeList, QTextStream &output, int tabLevel)
{
    Node::writeQmlProperties(changeList, output, tabLevel);
}

template<typename V>
void GroupNode::setProps(const V &attrs, PropSetFlags flags)
{
    const QString typeName = QStringLiteral("Group");

    // Different default value.
    parseProperty(attrs, flags, typeName, QStringLiteral("name"), &m_name);
}

ComponentNode::ComponentNode()
    : Node(GraphObject::Component)
{

}

ComponentNode::~ComponentNode()
{

}

void ComponentNode::setProperties(const QXmlStreamAttributes &attrs, PropSetFlags flags)
{
    Node::setProperties(attrs, flags);
    setProps(attrs, flags);
}

void ComponentNode::applyPropertyChanges(const PropertyChangeList &changeList)
{
    Node::applyPropertyChanges(changeList);
    setProps(changeList, {});
}

void ComponentNode::writeQmlHeader(QTextStream &output, int tabLevel)
{
    output << QSSGQmlUtilities::insertTabs(tabLevel) << QSSGQmlUtilities::qmlComponentName(m_id) << " {\n";
}

void ComponentNode::writeQmlProperties(QTextStream &output, int tabLevel, bool isInRootLevel)
{
    Q_UNUSED(isInRootLevel)
    Node::writeQmlProperties(output, tabLevel);
}

void ComponentNode::writeQmlProperties(const PropertyChangeList &changeList, QTextStream &output, int tabLevel)
{
    Node::writeQmlProperties(changeList, output, tabLevel);
}

template<typename V>
void ComponentNode::setProps(const V &attrs, PropSetFlags flags)
{
    const QString typeName = QStringLiteral("Component");

    // Different default value.
    parseProperty(attrs, flags, typeName, QStringLiteral("name"), &m_name);
}

TextNode::TextNode()
    : Node(GraphObject::Text)
{

}

void TextNode::setProperties(const QXmlStreamAttributes &attrs, PropSetFlags flags)
{
    Node::setProperties(attrs, flags);
    setProps(attrs, flags);
}

void TextNode::applyPropertyChanges(const PropertyChangeList &changeList)
{
    Node::applyPropertyChanges(changeList);
    setProps(changeList, {});
}

void TextNode::writeQmlHeader(QTextStream &output, int tabLevel)
{
    output << QSSGQmlUtilities::insertTabs(tabLevel) << "Node {\n";
}

namespace {
QString textHorizontalAlignToString(TextNode::HorizontalAlignment mode)
{
    if (mode == TextNode::Left) {
        return QStringLiteral("Text.AlignLeft");
    } else if (mode == TextNode::Center) {
        return QStringLiteral("Text.AlignHCenter");
    } else if (mode == TextNode::Right) {
        return QStringLiteral("Text.AlignRight");
    } else {
        return QStringLiteral("Text.AlignJustify");
    }
}
QString textVerticalAlignToString(TextNode::VerticalAlignment mode)
{
    if (mode == TextNode::Top) {
        return QStringLiteral("Text.AlignTop");
    } else if (mode == TextNode::Middle) {
        return QStringLiteral("Text.AlignVCenter");
    } else {
        return QStringLiteral("Text.AlignBottom");
    }
}
QString textWrapModeToString(TextNode::WordWrap mode)
{
    if (mode == TextNode::Clip) {
        return QStringLiteral("Text.NoWrap");
    } else if (mode == TextNode::WrapWord) {
        return QStringLiteral("Text.WordWrap");
    } else {
        return QStringLiteral("Text.WrapAnywhere");
    }
}
QString textElideToString(TextNode::Elide mode)
{
    if (mode == TextNode::ElideNone) {
        return QStringLiteral("Text.ElideNone");
    } else if (mode == TextNode::ElideLeft) {
        return QStringLiteral("Text.ElideLeft");
    } else if (mode == TextNode::ElideMiddle) {
        return QStringLiteral("Text.ElideMiddle");
    } else {
        return QStringLiteral("Text.ElideRight");
    }
}
}

void TextNode::writeQmlProperties(QTextStream &output, int tabLevel, bool isInRootLevel)
{
    Q_UNUSED(isInRootLevel)
    Node::writeQmlProperties(output, tabLevel);
    output << QSSGQmlUtilities::insertTabs(tabLevel) << "Text {\n";
    m_text.prepend('"');
    m_text.replace(QString(""), QString("\\n"));
    m_text.append('"');
    writeQmlPropertyHelper(output, tabLevel + 1, type(), QStringLiteral("textstring"), m_text);
    writeQmlPropertyHelper(output, tabLevel + 1, type(), QStringLiteral("textcolor"), m_color);
    writeQmlPropertyHelper(output, tabLevel + 1, type(), QStringLiteral("size"), m_size);
    m_font.prepend('"');
    m_font.append('"');
    writeQmlPropertyHelper(output, tabLevel + 1, type(), QStringLiteral("font"), m_font);
    writeQmlPropertyHelper(output, tabLevel + 1, type(), QStringLiteral("horzalign"),
                           textHorizontalAlignToString(m_horizAlign));
    writeQmlPropertyHelper(output, tabLevel + 1, type(), QStringLiteral("vertalign"),
                           textVerticalAlignToString(m_vertAlign));
    writeQmlPropertyHelper(output, tabLevel + 1, type(), QStringLiteral("wordWrap"),
                           textWrapModeToString(m_wordWrap));
    if (m_wordWrap == Clip) {
        output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("clip: true") << Qt::endl;
    }

    writeQmlPropertyHelper(output, tabLevel + 1, type(), QStringLiteral("elide"),
                           textElideToString(m_elide));
    output << QSSGQmlUtilities::insertTabs(tabLevel) << "}\n";
}

void TextNode::writeQmlProperties(const PropertyChangeList &changeList, QTextStream &output, int tabLevel)
{
    applyPropertyChanges(changeList);
    for (auto change : changeList) {
        QString targetProperty = change.nameStr();
        if (targetProperty == QStringLiteral("textstring")) {
            m_text.prepend('"');
            m_text.replace(QString(""), QString("\\n"));
            m_text.append('"');
            writeQmlPropertyHelper(output, tabLevel + 1, type(), QStringLiteral("textstring"), m_text);
        } else if (targetProperty == QStringLiteral("textcolor")) {
            writeQmlPropertyHelper(output, tabLevel + 1, type(), QStringLiteral("textcolor"), m_color);
        } else if (targetProperty == QStringLiteral("size")) {
            writeQmlPropertyHelper(output, tabLevel + 1, type(), QStringLiteral("size"), m_size);
        } else if (targetProperty == QStringLiteral("font")) {
            m_font.prepend('"');
            m_font.append('"');
            writeQmlPropertyHelper(output, tabLevel + 1, type(), QStringLiteral("font"), m_font);
        } else if (targetProperty == QStringLiteral("horzalign")) {
            writeQmlPropertyHelper(output, tabLevel + 1, type(), QStringLiteral("horzalign"),
                                   textHorizontalAlignToString(m_horizAlign));
        } else if (targetProperty == QStringLiteral("vertalign")) {
            writeQmlPropertyHelper(output, tabLevel + 1, type(), QStringLiteral("vertalign"),
                                   textVerticalAlignToString(m_vertAlign));
        } else if (targetProperty == QStringLiteral("wordWrap")) {
            writeQmlPropertyHelper(output, tabLevel + 1, type(), QStringLiteral("wordWrap"),
                                   textWrapModeToString(m_wordWrap));
            if (m_wordWrap == Clip) {
                output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("clip: true") << Qt::endl;
    }
        } else if (targetProperty == QStringLiteral("elide")) {
            writeQmlPropertyHelper(output, tabLevel + 1, type(), QStringLiteral("elide"),
                                   textElideToString(m_elide));
        }
    }
}

template<typename V>
void TextNode::setProps(const V &attrs, PropSetFlags flags)
{
    const QString typeName = QStringLiteral("Text");
    parseMultiLineStringProperty(attrs, flags, typeName, QStringLiteral("textstring"), &m_text);
    parseProperty(attrs, flags, typeName, QStringLiteral("textcolor"), &m_color);
    parseFontProperty(attrs, flags, typeName, QStringLiteral("font"), &m_font);
    parseFontSizeProperty(attrs, flags, typeName, QStringLiteral("size"), &m_size);
    parseProperty(attrs, flags, typeName, QStringLiteral("horzalign"), &m_horizAlign);
    parseProperty(attrs, flags, typeName, QStringLiteral("vertalign"), &m_vertAlign);
    parseProperty(attrs, flags, typeName, QStringLiteral("leading"), &m_leading);
    parseProperty(attrs, flags, typeName, QStringLiteral("tracking"), &m_tracking);
    parseProperty(attrs, flags, typeName, QStringLiteral("dropshadow"), &m_shadow);
    parseProperty(attrs, flags, typeName, QStringLiteral("dropshadowstrength"), &m_shadowStrength);
    parseProperty(attrs, flags, typeName, QStringLiteral("dropshadowoffsetx"), &m_shadowOffsetX);
    parseProperty(attrs, flags, typeName, QStringLiteral("dropshadowoffsety"), &m_shadowOffsetY);
    parseProperty(attrs, flags, typeName, QStringLiteral("dropshadowoffset"), &m_shadowOffset); // To be removed in 2.x (when UIP version is next updated)
    parseProperty(attrs, flags, typeName, QStringLiteral("dropshadowhorzalign"), &m_shadowHorzAlign); // To be removed in 2.x (when UIP version is next updated)
    parseProperty(attrs, flags, typeName, QStringLiteral("dropshadowvertalign"), &m_shadowVertAlign); // To be removed in 2.x (when UIP version is next updated)
    parseSizeProperty(attrs, flags, typeName, QStringLiteral("boundingbox"), &m_boundingBox);
    parseProperty(attrs, flags, typeName, QStringLiteral("wordwrap"), &m_wordWrap);
    parseProperty(attrs, flags, typeName, QStringLiteral("elide"), &m_elide);

    // Different default value.
    parseProperty(attrs, flags, typeName, QStringLiteral("name"), &m_name);
}

DefaultMaterial::DefaultMaterial()
    : GraphObject(GraphObject::DefaultMaterial)
{

}

void DefaultMaterial::setProperties(const QXmlStreamAttributes &attrs, PropSetFlags flags)
{
    GraphObject::setProperties(attrs, flags);
    setProps(attrs, flags);
}

void DefaultMaterial::applyPropertyChanges(const PropertyChangeList &changeList)
{
    GraphObject::applyPropertyChanges(changeList);
    setProps(changeList, {});
}

void DefaultMaterial::writeQmlHeader(QTextStream &output, int tabLevel)
{
    output << QSSGQmlUtilities::insertTabs(tabLevel) << "DefaultMaterial {\n";
}

namespace {
QString shaderLightingToString(DefaultMaterial::ShaderLighting mode)
{
    if (mode == DefaultMaterial::PixelShaderLighting) {
        return QStringLiteral("DefaultMaterial.FragmentLighting");
    } else {
        return QStringLiteral("DefaultMaterial.NoLighting");
    }
}
QString shaderBlendModeToString(DefaultMaterial::BlendMode mode)
{
    switch (mode) {
    case DefaultMaterial::Normal:
        return QStringLiteral("DefaultMaterial.SourceOver");
    case DefaultMaterial::Screen:
        return QStringLiteral("DefaultMaterial.Screen");
    case DefaultMaterial::Multiply:
        return QStringLiteral("DefaultMaterial.Multiply");
    case DefaultMaterial::Overlay:
        return QStringLiteral("DefaultMaterial.Overlay");
    case DefaultMaterial::ColorBurn:
        return QStringLiteral("DefaultMaterial.ColorBurn");
    case DefaultMaterial::ColorDodge:
        return QStringLiteral("DefaultMaterial.ColorDodge");
    }
    Q_ASSERT(false);
    return QString();
}
QString shaderSpecularModelToString(DefaultMaterial::SpecularModel model)
{
    switch (model) {
        case DefaultMaterial::DefaultSpecularModel:
            return QStringLiteral("DefaultMaterial.Default");
    case DefaultMaterial::KGGX:
        return QStringLiteral("DefaultMaterial.KGGX");
    case DefaultMaterial::KWard:
        return QStringLiteral("DefaultMaterial.KWard");
    }
    Q_ASSERT(false);
    return QString();
}
}

void DefaultMaterial::writeQmlProperties(QTextStream &output, int tabLevel, bool isInRootLevel)
{
    Q_UNUSED(isInRootLevel)
    output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("id: ") << qmlId() << Qt::endl;
    writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("shaderlighting"), shaderLightingToString(m_shaderLighting));
    writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("blendmode"), shaderBlendModeToString(m_blendMode));
    writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("diffuse"), m_diffuse);
    if (!m_diffuseMap_unresolved.isEmpty())
        output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("diffuseMap: ") << UniqueIdMapper::instance()->queryId(m_diffuseMap_unresolved) << Qt::endl;

    writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("emissivepower"), m_emissiveFactor / 100.0f);
    if (!m_emissiveMap_unresolved.isEmpty())
        output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("emissiveMap: ") << UniqueIdMapper::instance()->queryId(m_emissiveMap_unresolved) << Qt::endl;
    writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("emissivecolor"), m_emissiveColor);

    if (!m_specularReflection_unresolved.isEmpty())
        output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("specularReflectionMap: ") << UniqueIdMapper::instance()->queryId(m_specularReflection_unresolved) << Qt::endl;
    if (!m_specularMap_unresolved.isEmpty())
        output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("specularMap: ") << UniqueIdMapper::instance()->queryId(m_specularMap_unresolved) << Qt::endl;

    writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("specularmodel"), shaderSpecularModelToString(m_specularModel));
    writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("speculartint"), m_specularTint);
    writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("ior"), m_ior);
    writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("fresnelPower"), m_fresnelPower);
    writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("specularamount"), m_specularAmount);
    writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("specularroughness"), m_specularRoughness);

    if (!m_roughnessMap_unresolved.isEmpty())
        output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("roughnessMap: ") << UniqueIdMapper::instance()->queryId(m_roughnessMap_unresolved) << Qt::endl;

    writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("opacity"), m_opacity * 0.01);
    if (!m_opacityMap_unresolved.isEmpty())
        output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("opacityMap: ") << UniqueIdMapper::instance()->queryId(m_opacityMap_unresolved) << Qt::endl;

    if (!m_bumpMap_unresolved.isEmpty())
        output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("bumpMap: ") << UniqueIdMapper::instance()->queryId(m_bumpMap_unresolved) << Qt::endl;
    writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("bumpamount"), m_bumpAmount);

    if (!m_normalMap_unresolved.isEmpty())
        output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("normalMap: ") << UniqueIdMapper::instance()->queryId(m_normalMap_unresolved) << Qt::endl;

    if (!m_translucencyMap_unresolved.isEmpty())
        output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("translucencyMap: ") << UniqueIdMapper::instance()->queryId(m_translucencyMap_unresolved) << Qt::endl;
    writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("translucentfalloff"), m_translucentFalloff);

    writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("diffuselightwrap"), m_diffuseLightWrap);
    writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("vertexcolors"), m_vertexColors);

    // Common Material values
    if (!m_lightmapIndirectMap_unresolved.isEmpty())
        output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("lightmapIndirect: ") << UniqueIdMapper::instance()->queryId(m_lightmapIndirectMap_unresolved) << Qt::endl;
    if (!m_lightmapRadiosityMap_unresolved.isEmpty())
        output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("lightmapRadiosity: ") << UniqueIdMapper::instance()->queryId(m_lightmapRadiosityMap_unresolved) << Qt::endl;
    if (!m_lightmapShadowMap_unresolved.isEmpty())
        output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("lightmapShadow: ") << UniqueIdMapper::instance()->queryId(m_lightmapShadowMap_unresolved) << Qt::endl;
    if (!m_lightProbe_unresolved.isEmpty())
        output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("lightProbe: ") << UniqueIdMapper::instance()->queryId(m_lightProbe_unresolved) << Qt::endl;
    if (!m_displacementMap_unresolved.isEmpty())
        output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("displacementMap: ") << UniqueIdMapper::instance()->queryId(m_displacementMap_unresolved) << Qt::endl;
    writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("displacementamount"), m_displaceAmount);
}

void DefaultMaterial::writeQmlProperties(const PropertyChangeList &changeList, QTextStream &output, int tabLevel)
{
    // apply the changes so the values are translated
    applyPropertyChanges(changeList);

    for (auto change : changeList) {
        QString targetProperty = change.nameStr();
        if (targetProperty == QStringLiteral("shaderlighting")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("shaderlighting"), shaderLightingToString(m_shaderLighting));
        } else if (targetProperty == QStringLiteral("blendmode")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("blendmode"), shaderBlendModeToString(m_blendMode));
        } else if (targetProperty == QStringLiteral("diffuse")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("diffuse"), m_diffuse);
        } else if (targetProperty == QStringLiteral("diffusemap")) {
            output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("diffuseMap: ") << UniqueIdMapper::instance()->queryId(m_diffuseMap_unresolved) << Qt::endl;
        } else if (targetProperty == QStringLiteral("emissivepower")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("emissivepower"), m_emissiveFactor / 100.0f);
        } else if (targetProperty == QStringLiteral("emissivecolor")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("emissivecolor"), m_emissiveColor);
        } else if (targetProperty == QStringLiteral("emissivemap")) {
            output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("emissiveMap: ") << UniqueIdMapper::instance()->queryId(m_emissiveMap_unresolved) << Qt::endl;
        } else if (targetProperty == QStringLiteral("specularreflection")) {
            output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("specularReflectionMap: ") << UniqueIdMapper::instance()->queryId(m_specularReflection_unresolved) << Qt::endl;
        } else if (targetProperty == QStringLiteral("specularmap")) {
            output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("specularMap: ") << UniqueIdMapper::instance()->queryId(m_specularMap_unresolved) << Qt::endl;
        } else if (targetProperty == QStringLiteral("specularmodel")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("specularmodel"), shaderSpecularModelToString(m_specularModel));
        } else if (targetProperty == QStringLiteral("speculartint")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("speculartint"), m_specularTint);
        } else if (targetProperty == QStringLiteral("ior")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("ior"), m_ior);
        } else if (targetProperty == QStringLiteral("fresnelPower")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("fresnelPower"), m_fresnelPower);
        } else if (targetProperty == QStringLiteral("specularamount")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("specularamount"), m_specularAmount);
        } else if (targetProperty == QStringLiteral("specularroughness")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("specularroughness"), m_specularRoughness);
        } else if (targetProperty == QStringLiteral("roughnessmap")) {
            output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("roughnessMap: ") << UniqueIdMapper::instance()->queryId(m_roughnessMap_unresolved) << Qt::endl;
        } else if (targetProperty == QStringLiteral("opacity")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("opacity"), m_opacity * 0.01);
        } else if (targetProperty == QStringLiteral("opacitymap")) {
            output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("opacityMap: ") << UniqueIdMapper::instance()->queryId(m_opacityMap_unresolved) << Qt::endl;
        } else if (targetProperty == QStringLiteral("bumpmap")) {
            output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("bumpMap: ") << UniqueIdMapper::instance()->queryId(m_bumpMap_unresolved) << Qt::endl;
        } else if (targetProperty == QStringLiteral("bumpamount")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("bumpamount"), m_bumpAmount);
        } else if (targetProperty == QStringLiteral("normalmap")) {
            output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("normalMap: ") << UniqueIdMapper::instance()->queryId(m_normalMap_unresolved) << Qt::endl;
        } else if (targetProperty == QStringLiteral("translucencymap")) {
            output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("translucencyMap: ") << UniqueIdMapper::instance()->queryId(m_translucencyMap_unresolved) << Qt::endl;
        } else if (targetProperty == QStringLiteral("translucentfalloff")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("translucentfalloff"), m_translucentFalloff);
        } else if (targetProperty == QStringLiteral("diffuselightwrap")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("diffuselightwrap"), m_diffuseLightWrap);
        } else if (targetProperty == QStringLiteral("vertexcolors")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("vertexcolors"), m_vertexColors);
        } else if (targetProperty == QStringLiteral("lightmapindirect")) {
            output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("lightmapIndirect: ") << UniqueIdMapper::instance()->queryId(m_lightmapIndirectMap_unresolved) << Qt::endl;
        } else if (targetProperty == QStringLiteral("lightmapradiosity")) {
            output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("lightmapRadiosity: ") << UniqueIdMapper::instance()->queryId(m_lightmapRadiosityMap_unresolved) << Qt::endl;
        } else if (targetProperty == QStringLiteral("lightmapshadow")) {
            output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("lightmapShadow: ") << UniqueIdMapper::instance()->queryId(m_lightmapShadowMap_unresolved) << Qt::endl;
        } else if (targetProperty == QStringLiteral("iblprobe")) {
            output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("lightProbe: ") << UniqueIdMapper::instance()->queryId(m_lightProbe_unresolved) << Qt::endl;
        } else if (targetProperty == QStringLiteral("displacementmap")) {
            output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("displacementMap: ") << UniqueIdMapper::instance()->queryId(m_displacementMap_unresolved) << Qt::endl;
        } else if (targetProperty == QStringLiteral("displacementamount")) {
            writeQmlPropertyHelper(output, tabLevel, type(), QStringLiteral("displacementamount"), m_displaceAmount);
        }
    }
}

template<typename V>
void DefaultMaterial::setProps(const V &attrs, PropSetFlags flags)
{
    const QString typeName = QStringLiteral("Material");

    parseProperty(attrs, flags, typeName, QStringLiteral("shaderlighting"), &m_shaderLighting);
    parseProperty(attrs, flags, typeName, QStringLiteral("blendmode"), &m_blendMode);

    parseProperty(attrs, flags, typeName, QStringLiteral("vertexcolors"), &m_vertexColors);
    parseProperty(attrs, flags, typeName, QStringLiteral("diffuse"), &m_diffuse);

    parseImageProperty(attrs, flags, typeName, QStringLiteral("diffusemap"), &m_diffuseMap_unresolved);
    parseImageProperty(attrs, flags, typeName, QStringLiteral("specularreflection"), &m_specularReflection_unresolved);

    parseProperty(attrs, flags, typeName, QStringLiteral("speculartint"), &m_specularTint);
    parseProperty(attrs, flags, typeName, QStringLiteral("specularamount"), &m_specularAmount);

    parseImageProperty(attrs, flags, typeName, QStringLiteral("specularmap"), &m_specularMap_unresolved);

    parseProperty(attrs, flags, typeName, QStringLiteral("specularmodel"), &m_specularModel);
    parseProperty(attrs, flags, typeName, QStringLiteral("specularroughness"), &m_specularRoughness);

    parseImageProperty(attrs, flags, typeName, QStringLiteral("roughnessmap"), &m_roughnessMap_unresolved);

    // Yes it really is fresnelPower (for some reason this has a capital P)
    parseProperty(attrs, flags, typeName, QStringLiteral("fresnelPower"), &m_fresnelPower);
    parseProperty(attrs, flags, typeName, QStringLiteral("ior"), &m_ior);

    parseImageProperty(attrs, flags, typeName, QStringLiteral("bumpmap"), &m_bumpMap_unresolved);
    parseImageProperty(attrs, flags, typeName, QStringLiteral("normalmap"), &m_normalMap_unresolved);

    parseProperty(attrs, flags, typeName, QStringLiteral("bumpamount"), &m_bumpAmount);

    parseImageProperty(attrs, flags, typeName, QStringLiteral("displacementmap"), &m_displacementMap_unresolved);

    parseProperty(attrs, flags, typeName, QStringLiteral("displaceamount"), &m_displaceAmount);
    parseProperty(attrs, flags, typeName, QStringLiteral("opacity"), &m_opacity);

    parseImageProperty(attrs, flags, typeName, QStringLiteral("opacitymap"), &m_opacityMap_unresolved);

    parseProperty(attrs, flags, typeName, QStringLiteral("emissivecolor"), &m_emissiveColor);
    parseProperty(attrs, flags, typeName, QStringLiteral("emissivepower"), &m_emissiveFactor);

    parseImageProperty(attrs, flags, typeName, QStringLiteral("emissivemap"), &m_emissiveMap_unresolved);
    parseImageProperty(attrs, flags, typeName, QStringLiteral("translucencymap"), &m_translucencyMap_unresolved);

    parseProperty(attrs, flags, typeName, QStringLiteral("translucentfalloff"), &m_translucentFalloff);
    parseProperty(attrs, flags, typeName, QStringLiteral("diffuselightwrap"), &m_diffuseLightWrap);

    parseImageProperty(attrs, flags, typeName, QStringLiteral("lightmapindirect"), &m_lightmapIndirectMap_unresolved);
    parseImageProperty(attrs, flags, typeName, QStringLiteral("lightmapradiosity"), &m_lightmapRadiosityMap_unresolved);
    parseImageProperty(attrs, flags, typeName, QStringLiteral("lightmapshadow"), &m_lightmapShadowMap_unresolved);
    parseImageProperty(attrs, flags, typeName, QStringLiteral("iblprobe"), &m_lightProbe_unresolved);

    // Different default value.
    parseProperty(attrs, flags, typeName, QStringLiteral("name"), &m_name);
}

ReferencedMaterial::ReferencedMaterial()
    : GraphObject(GraphObject::ReferencedMaterial)
{

}

void ReferencedMaterial::setProperties(const QXmlStreamAttributes &attrs, PropSetFlags flags)
{
    GraphObject::setProperties(attrs, flags);
    setProps(attrs, flags);
}

void ReferencedMaterial::applyPropertyChanges(const PropertyChangeList &changeList)
{
    GraphObject::applyPropertyChanges(changeList);
    setProps(changeList, {});
}

void ReferencedMaterial::writeQmlHeader(QTextStream &output, int tabLevel)
{
    // This is a bit special because it references a component
    QString componentName = qmlPresentationComponentName(m_referencedMaterial_unresolved);
    output << QSSGQmlUtilities::insertTabs(tabLevel) << componentName << " {\n";
}

void ReferencedMaterial::writeQmlProperties(QTextStream &output, int tabLevel, bool isInRootLevel)
{
    Q_UNUSED(isInRootLevel)
    output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("id: ") << qmlId() << Qt::endl;
    if (!m_lightmapIndirectMap_unresolved.isEmpty())
        output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("lightmapIndirect: ") << UniqueIdMapper::instance()->queryId(m_lightmapIndirectMap_unresolved) << Qt::endl;
    if (!m_lightmapRadiosityMap_unresolved.isEmpty())
        output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("lightmapRadiosity: ") << UniqueIdMapper::instance()->queryId(m_lightmapRadiosityMap_unresolved) << Qt::endl;
    if (!m_lightmapShadowMap_unresolved.isEmpty())
        output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("lightmapShadow: ") << UniqueIdMapper::instance()->queryId(m_lightmapShadowMap_unresolved) << Qt::endl;
    if (!m_lightProbe_unresolved.isEmpty())
        output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("lightProbe: ") << UniqueIdMapper::instance()->queryId(m_lightProbe_unresolved) << Qt::endl;
}

void ReferencedMaterial::writeQmlProperties(const PropertyChangeList &changeList, QTextStream &output, int tabLevel)
{
    Q_UNUSED(changeList)
    Q_UNUSED(output)
    Q_UNUSED(tabLevel)
}

template<typename V>
void ReferencedMaterial::setProps(const V &attrs, PropSetFlags flags)
{
    const QString typeName = QStringLiteral("ReferencedMaterial");
    parseObjectRefProperty(attrs, flags, typeName, QStringLiteral("referencedmaterial"), &m_referencedMaterial_unresolved);

    parseImageProperty(attrs, flags, typeName, QStringLiteral("lightmapindirect"), &m_lightmapIndirectMap_unresolved);
    parseImageProperty(attrs, flags, typeName, QStringLiteral("lightmapradiosity"), &m_lightmapRadiosityMap_unresolved);
    parseImageProperty(attrs, flags, typeName, QStringLiteral("lightmapshadow"), &m_lightmapShadowMap_unresolved);
    parseImageProperty(attrs, flags, typeName, QStringLiteral("iblprobe"), &m_lightProbe_unresolved);

    // Different default value.
    parseProperty(attrs, flags, typeName, QStringLiteral("name"), &m_name);
}

CustomMaterialInstance::CustomMaterialInstance()
    : GraphObject(GraphObject::CustomMaterial)
{

}

void CustomMaterialInstance::setProperties(const QXmlStreamAttributes &attrs, PropSetFlags flags)
{
    GraphObject::setProperties(attrs, flags);
    setProps(attrs, flags);

    // Save attributes for the 2nd pass (resolveReferences) since they may
    // refer to custom properties defined in the custom material.
    for (const QXmlStreamAttribute &attr : attrs)
        m_pendingCustomProperties.append(PropertyChange(attr.name().toString(), attr.value().toString()));
}

void CustomMaterialInstance::applyPropertyChanges(const PropertyChangeList &changeList)
{
    GraphObject::applyPropertyChanges(changeList);
    setProps(changeList, {});

//    QVariantMap propChanges;
//    for (const PropertyChange &change : changeList) {
//        const Q3DS::PropertyType type = m_material.properties().value(change.nameStr()).type;
//        propChanges[change.nameStr()] = Q3DS::convertToVariant(change.valueStr(), type);
//    }

//    if (!propChanges.isEmpty())
    //        applyDynamicProperties(propChanges);
}

void CustomMaterialInstance::writeQmlHeader(QTextStream &output, int tabLevel)
{
    output << QSSGQmlUtilities::insertTabs(tabLevel) << "CustomMaterial {\n";
}

void CustomMaterialInstance::writeQmlProperties(QTextStream &output, int tabLevel, bool isInRootLevel)
{
    Q_UNUSED(isInRootLevel)
    output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("id: ") << qmlId() << Qt::endl;
    output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("source: ") << QStringLiteral("\"") << UniqueIdMapper::instance()->queryId(m_material_unresolved) << QStringLiteral("\"") << Qt::endl;

    // Common Material values
    if (!m_lightmapIndirectMap_unresolved.isEmpty())
        output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("lightmapIndirect: ") << UniqueIdMapper::instance()->queryId(m_lightmapIndirectMap_unresolved) << Qt::endl;
    if (!m_lightmapRadiosityMap_unresolved.isEmpty())
        output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("lightmapRadiosity: ") << UniqueIdMapper::instance()->queryId(m_lightmapRadiosityMap_unresolved) << Qt::endl;
    if (!m_lightmapShadowMap_unresolved.isEmpty())
        output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("lightmapShadow: ") << UniqueIdMapper::instance()->queryId(m_lightmapShadowMap_unresolved) << Qt::endl;
    if (!m_lightProbe_unresolved.isEmpty())
        output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("lightProbe: ") << UniqueIdMapper::instance()->queryId(m_lightProbe_unresolved) << Qt::endl;
}

void CustomMaterialInstance::writeQmlProperties(const PropertyChangeList &changeList, QTextStream &output, int tabLevel)
{
    Q_UNUSED(changeList)
    Q_UNUSED(output)
    Q_UNUSED(tabLevel)
}

template<typename V>
void CustomMaterialInstance::setProps(const V &attrs, PropSetFlags flags)
{
    const QString typeName = QStringLiteral("CustomMaterial");
    if (parseProperty(attrs, flags, typeName, QStringLiteral("class"), &m_material_unresolved))
        m_materialIsResolved = false;

    parseImageProperty(attrs, flags, typeName, QStringLiteral("lightmapindirect"), &m_lightmapIndirectMap_unresolved);
    parseImageProperty(attrs, flags, typeName, QStringLiteral("lightmapradiosity"), &m_lightmapRadiosityMap_unresolved);
    parseImageProperty(attrs, flags, typeName, QStringLiteral("lightmapshadow"), &m_lightmapShadowMap_unresolved);
    parseImageProperty(attrs, flags, typeName, QStringLiteral("iblprobe"), &m_lightProbe_unresolved);

    // Different default value.
    parseProperty(attrs, flags, typeName, QStringLiteral("name"), &m_name);
}

EffectInstance::EffectInstance()
    : GraphObject(GraphObject::Effect)
{

}

void EffectInstance::setProperties(const QXmlStreamAttributes &attrs, PropSetFlags flags)
{
    GraphObject::setProperties(attrs, flags);
    setProps(attrs, flags);

    // Save attributes for the 2nd pass (resolveReferences) since they may
    // refer to custom properties defined in the effect.
    for (const QXmlStreamAttribute &attr : attrs)
        m_pendingCustomProperties.append(PropertyChange(attr.name().toString(), attr.value().toString()));
}

void EffectInstance::applyPropertyChanges(const PropertyChangeList &changeList)
{
    GraphObject::applyPropertyChanges(changeList);
    setProps(changeList, {});

//    // could be a custom effect property
//    QVariantMap propChanges;
//    for (const PropertyChange &change : changeList) {
//        const Q3DS::PropertyType type = m_effect.properties().value(change.nameStr()).type;
//        propChanges[change.nameStr()] = Q3DS::convertToVariant(change.valueStr(), type);
//    }

//    if (!propChanges.isEmpty())
    //        applyDynamicProperties(propChanges);
}

void EffectInstance::writeQmlHeader(QTextStream &output, int tabLevel)
{
    Q_UNUSED(output)
    Q_UNUSED(tabLevel)
    //output << QSSGQmlUtilities::insertTabs(tabLevel) << "Effect {\n";
}

void EffectInstance::writeQmlProperties(QTextStream &output, int tabLevel, bool isInRootLevel)
{
    Q_UNUSED(output)
    Q_UNUSED(tabLevel)
    Q_UNUSED(isInRootLevel)
    //output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("id: ") << qmlId() << Qt::endl;
    //output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("source: ") << QSSGQmlUtilities::sanitizeQmlId(m_effect_unresolved) << Qt::endl;
}

void EffectInstance::writeQmlProperties(const PropertyChangeList &changeList, QTextStream &output, int tabLevel)
{
    Q_UNUSED(changeList)
    Q_UNUSED(output)
    Q_UNUSED(tabLevel)
}

void EffectInstance::writeQmlFooter(QTextStream &output, int tabLevel)
{
    Q_UNUSED(output)
    Q_UNUSED(tabLevel)
}

template<typename V>
void EffectInstance::setProps(const V &attrs, PropSetFlags flags)
{
    const QString typeName = QStringLiteral("Effect");
    if (parseProperty(attrs, flags, typeName, QStringLiteral("class"), &m_effect_unresolved))
        m_effectIsResolved = false;

    parseProperty(attrs, flags, typeName, QStringLiteral("eyeball"), &m_eyeballEnabled);

    // Different default value.
    parseProperty(attrs, flags, typeName, QStringLiteral("name"), &m_name);
}

BehaviorInstance::BehaviorInstance()
    : GraphObject(GraphObject::Behavior)
{

}

void BehaviorInstance::setProperties(const QXmlStreamAttributes &attrs, PropSetFlags flags)
{
    GraphObject::setProperties(attrs, flags);
    setProps(attrs, flags);

    // Save attributes for the 2nd pass (resolveReferences) since they may
    // refer to custom properties defined in the behavior.
    for (const QXmlStreamAttribute &attr : attrs)
        m_pendingCustomProperties.append(PropertyChange(attr.name().toString(), attr.value().toString()));
}

void BehaviorInstance::applyPropertyChanges(const PropertyChangeList &changeList)
{
    GraphObject::applyPropertyChanges(changeList);
    setProps(changeList, {});

//    // could be a custom behavior property
//    QVariantMap propChanges;
//    for (const PropertyChange &change : changeList) {
//        const Q3DS::PropertyType type = m_behavior.properties().value(change.nameStr()).type;
//        propChanges[change.nameStr()] = Q3DS::convertToVariant(change.valueStr(), type);
//    }

    //    if (!propChanges.isEmpty())
}

void BehaviorInstance::writeQmlHeader(QTextStream &output, int tabLevel)
{
    Q_UNUSED(output)
    Q_UNUSED(tabLevel)
}

void BehaviorInstance::writeQmlProperties(QTextStream &output, int tabLevel, bool isInRootLevel)
{
    Q_UNUSED(output)
    Q_UNUSED(tabLevel)
    Q_UNUSED(isInRootLevel)
}

void BehaviorInstance::writeQmlProperties(const PropertyChangeList &changeList, QTextStream &output, int tabLevel)
{
    Q_UNUSED(changeList)
    Q_UNUSED(output)
    Q_UNUSED(tabLevel)
}

void BehaviorInstance::writeQmlFooter(QTextStream &output, int tabLevel)
{
    Q_UNUSED(output)
    Q_UNUSED(tabLevel)
}

template<typename V>
void BehaviorInstance::setProps(const V &attrs, PropSetFlags flags)
{
    const QString typeName = QStringLiteral("Behavior");
    parseProperty(attrs, flags, typeName, QStringLiteral("class"), &m_behavior_unresolved);

    parseProperty(attrs, flags, typeName, QStringLiteral("eyeball"), &m_eyeballEnabled);

    // Different default value.
    parseProperty(attrs, flags, typeName, QStringLiteral("name"), &m_name);
}

AliasNode::AliasNode()
    : Node(Node::Alias)
{

}

void AliasNode::setProperties(const QXmlStreamAttributes &attrs, PropSetFlags flags)
{
    Node::setProperties(attrs, flags);
    setProps(attrs, flags);
}

void AliasNode::applyPropertyChanges(const PropertyChangeList &changeList)
{
    Node::applyPropertyChanges(changeList);
    setProps(changeList, {});
}

void AliasNode::writeQmlHeader(QTextStream &output, int tabLevel)
{
    // This is a bit special because it references a component
    QString componentName = qmlPresentationComponentName(m_referencedNode_unresolved);
    output << QSSGQmlUtilities::insertTabs(tabLevel) << componentName << " {\n";
}

void AliasNode::writeQmlProperties(QTextStream &output, int tabLevel, bool isInRootLevel)
{
    Q_UNUSED(isInRootLevel)
    Node::writeQmlProperties(output, tabLevel);
}

template<typename V>
void AliasNode::setProps(const V &attrs, PropSetFlags flags)
{
    const QString typeName = QStringLiteral("Alias");
    parseObjectRefProperty(attrs, flags, typeName, QStringLiteral("referencednode"), &m_referencedNode_unresolved);

    // Different default value.
    parseProperty(attrs, flags, typeName, QStringLiteral("name"), &m_name);
}

UipPresentation::UipPresentation()
    : d(new UipPresentationData)
{

}

UipPresentation::~UipPresentation()
{
    delete d->scene;
    delete d->masterSlide;
}

void UipPresentation::reset()
{
    delete d->scene;
    delete d->masterSlide;
    d.reset(new UipPresentationData);
}

QString UipPresentation::sourceFile() const
{
    return d->sourceFile;
}

void UipPresentation::setSourceFile(const QString &s)
{
    d->sourceFile = s;
}

QString UipPresentation::assetFileName(const QString &xmlFileNameRef, int *part) const
{
    QString rawName = xmlFileNameRef;
    if (rawName.startsWith('#')) {
        // Can be a built-in primitive ref, like #Cube.
        if (part)
            *part = 1;
        return rawName;
    }

    if (rawName.contains('#')) {
        int pos = rawName.lastIndexOf('#');
        bool ok = false;
        int idx = rawName.mid(pos + 1).toInt(&ok);
        if (!ok) {
            qWarning() << QObject::tr("Invalid part index '%1'").arg(rawName);
            return QString();
        }
        if (part)
            *part = idx;
        rawName = rawName.left(pos);
    } else {
        // If no part is specified return -1 so the mesh parser can decide which
        // part is the best.  This will usually be 1 but for older versions
        // of the editor multi-meshes were used for revisions, and we would
        // need to return the last part in the list, not the first.
        if (part)
            *part = -1;
    }

    rawName.replace('\\', '/');
    if (rawName.startsWith(QStringLiteral("./")))
        rawName = rawName.mid(2);

    // workaround for confused users of the QML API trying to pass an URL
    // starting with qrc:/ instead of a normal string with a filename where :/
    // is the only valid option
    if (rawName.startsWith(QStringLiteral("qrc:/")))
        rawName = rawName.mid(3);

    if (QFileInfo(rawName).isAbsolute())
        return rawName;

    QString path;
    if (d->sourceFile.isEmpty()) {
        path = QFileInfo(rawName).canonicalFilePath();
    } else {
        QString fn = QFileInfo(d->sourceFile).canonicalPath();
        fn += QLatin1Char('/');
        fn += rawName;
        path = QFileInfo(fn).absoluteFilePath();

        // We need the path to uia files for some assets, for example the textures of effects
        // As that is not readily available here, go up the directory hoping to find the
        // file wanted. We're using 3 levels for now, as that is what Editor is using as well.
        if (!QFileInfo(path).exists()) {
            int loops = 0;
            QString searchPath = QFileInfo(d->sourceFile).canonicalPath();
            searchPath.append(QLatin1String("/../"));
            while (!QFileInfo(searchPath + rawName).exists() && ++loops < 3)
                searchPath.append(QLatin1String("../"));
            path = searchPath + rawName;
            if (!QFileInfo(path).exists())
                path = QFileInfo(fn).absoluteFilePath();
        }
    }
    return path;
}

QString UipPresentation::name() const
{
    return d->name;
}

void UipPresentation::setName(const QString &s)
{
    d->name = s;
}

QString UipPresentation::author() const
{
    return d->author;
}

QString UipPresentation::company() const
{
    return d->company;
}

int UipPresentation::presentationWidth() const
{
    return d->presentationWidth;
}

int UipPresentation::presentationHeight() const
{
    return d->presentationHeight;
}

UipPresentation::Rotation UipPresentation::presentationRotation() const
{
    return d->presentationRotation;
}

bool UipPresentation::maintainAspectRatio() const
{
    return d->maintainAspectRatio;
}

void UipPresentation::setAuthor(const QString &author)
{
    d->author = author;
}

void UipPresentation::setCompany(const QString &company)
{
    d->company = company;
}

void UipPresentation::setPresentationWidth(int w)
{
    d->presentationWidth = w;
}

void UipPresentation::setPresentationHeight(int h)
{
    d->presentationHeight = h;
}

void UipPresentation::setPresentationRotation(UipPresentation::Rotation r)
{
    d->presentationRotation = r;
}

void UipPresentation::setMaintainAspectRatio(bool maintain)
{
    d->maintainAspectRatio = maintain;
}

Scene *UipPresentation::scene() const
{
    return d->scene;
}

Slide *UipPresentation::masterSlide() const
{
    return d->masterSlide;
}

void UipPresentation::setScene(Scene *p)
{
    d->scene = p;
}

void UipPresentation::setMasterSlide(Slide *p)
{
    d->masterSlide = p;
}

bool UipPresentation::registerObject(const QByteArray &id, GraphObject *p)
{
    if (d->objects.contains(id)) {
        qWarning("UipPresentation: Multiple registrations for object id '%s'", id.constData());
        return false;
    }
    p->m_id = id;
    d->objects[id] = p;
    return true;
}

void UipPresentation::unregisterObject(const QByteArray &id)
{
    d->objects.remove(id);
}

void UipPresentation::registerImageBuffer(const QString &sourcePath, bool hasTransparency)
{
    d->imageBuffers[sourcePath] = hasTransparency;
}

const UipPresentation::ImageBufferMap &UipPresentation::imageBuffer() const
{
    return d->imageBuffers;
}

void UipPresentation::applyPropertyChanges(const Slide::PropertyChanges &changeList) const
{
    for (auto it = changeList.cbegin(), ite = changeList.cend(); it != ite; ++it) {
//        for (auto change = it.value()->begin(); change != it.value()->end(); change++)
//            qDebug() << "\t" << it.key() << "applying property change:" << change->name() << change->value();

        it.key()->applyPropertyChanges(*it.value());
    }
}

void UipPresentation::applySlidePropertyChanges(Slide *slide) const
{
    const auto &changeList = slide->propertyChanges();
    //qDebug("Applying %d property changes from slide %s", changeList.count(), slide->m_id.constData());
    applyPropertyChanges(changeList);
}

GraphObject *UipPresentation::newObject(const char *type, const QByteArray &id)
{
    GraphObject *obj = nullptr;

    if (type == QByteArrayLiteral("Scene"))
        obj = newObject<Scene>(id);
    else if (type == QByteArrayLiteral("Slide"))
        obj = newObject<Slide>(id);
    else if (type == QByteArrayLiteral("Image"))
        obj = newObject<Image>(id);
    else if (type == QByteArrayLiteral("DefaultMaterial"))
        obj = newObject<DefaultMaterial>(id);
    else if (type == QByteArrayLiteral("ReferencedMaterial"))
        obj = newObject<ReferencedMaterial>(id);
    else if (type == QByteArrayLiteral("CustomMaterial"))
        obj = newObject<CustomMaterialInstance>(id);
    else if (type == QByteArrayLiteral("Effect"))
        obj = newObject<EffectInstance>(id);
    else if (type == QByteArrayLiteral("Behavior"))
        obj = newObject<BehaviorInstance>(id);
    else if (type == QByteArrayLiteral("Layer"))
        obj = newObject<LayerNode>(id);
    else if (type == QByteArrayLiteral("Camera"))
        obj = newObject<CameraNode>(id);
    else if (type == QByteArrayLiteral("Light"))
        obj = newObject<LightNode>(id);
    else if (type == QByteArrayLiteral("Model"))
        obj = newObject<ModelNode>(id);
    else if (type == QByteArrayLiteral("Group"))
        obj = newObject<GroupNode>(id);
    else if (type == QByteArrayLiteral("Text"))
        obj = newObject<TextNode>(id);
    else if (type == QByteArrayLiteral("Component"))
        obj = newObject<ComponentNode>(id);
    else if (type == QByteArrayLiteral("Alias"))
        obj = newObject<AliasNode>(id);

    return obj;
}

GraphObject *UipPresentation::getObject(const QByteArray &id) const
{
    return d->objects.value(id);
}

GraphObject *UipPresentation::getObjectByName(const QString &name) const
{
    for (auto it = d->objects.cbegin(), itEnd = d->objects.cend(); it != itEnd; ++it) {
        if ((*it)->m_name == name)
            return *it;
    }
    return nullptr;
}

void AliasNode::writeQmlProperties(const PropertyChangeList &changeList, QTextStream &output, int tabLevel)
{
    Node::writeQmlProperties(changeList, output, tabLevel);
}


QT_END_NAMESPACE



