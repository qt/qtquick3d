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

#include "keyframegroupgenerator.h"

#include <QtQuick3DAssetImport/private/qssgqmlutilities_p.h>

#include "propertymap.h"

QT_BEGIN_NAMESPACE

KeyframeGroupGenerator::KeyframeGroupGenerator(float fps)
    : m_fps(fps)
{
}

KeyframeGroupGenerator::~KeyframeGroupGenerator()
{
    for (auto keyframeGroupMap : m_targetKeyframeMap.values())
        for (auto keyframeGroup : keyframeGroupMap.values())
            delete keyframeGroup;
}

void KeyframeGroupGenerator::addAnimation(const AnimationTrack &animation)
{
    auto keyframeGroupMap = m_targetKeyframeMap.find(animation.m_target);
    QStringList propertyParts = animation.m_property.split(".");
    auto propertyType = KeyframeGroup::getPropertyValueType(animation.m_target->type(),
                                                            animation.m_property);
    QString property = propertyType == KeyframeGroup::KeyFrame::Unhandled
            ? propertyParts.at(0) : animation.m_property;
    QString field = QStringLiteral("x");
    if (propertyParts.count() > 1)
        field = propertyParts.last();

    if (keyframeGroupMap != m_targetKeyframeMap.end()) {
        // Check if the property name already exists in the keyframes
        auto keyframeGroup = keyframeGroupMap.value().find(property);
        if (keyframeGroup != keyframeGroupMap.value().end()) {
            auto keyframeGroupInstance = keyframeGroup.value();
            // verify the keyframe lists are the same size
            if (keyframeGroupInstance->keyframes.count() == animation.m_keyFrames.count()) {
                for (int i = 0; i < keyframeGroupInstance->keyframes.count(); ++i)
                    keyframeGroupInstance->keyframes[i]->setValue(animation.m_keyFrames[i].value, field);
            } else {
                qWarning() << "Keyframe lists are not the same size, bad things are going to happen";
            }
        } else {
            // if not then add a new property
            keyframeGroupMap.value().insert(property,
                                            new KeyframeGroup(animation, property, field, m_fps));
        }
    } else {
        // Add a new KeyframeGroupMap
        auto keyframeGroupMap = KeyframeGroupMap();
        keyframeGroupMap.insert(property, new KeyframeGroup(animation, property, field, m_fps));
        m_targetKeyframeMap.insert(animation.m_target, keyframeGroupMap);
    }
}

void KeyframeGroupGenerator::generateKeyframeGroups(QTextStream &output, int tabLevel)
{
    for (auto groupMaps : m_targetKeyframeMap.values())
        for (auto keyframeGroup : groupMaps.values())
            keyframeGroup->generateKeyframeGroupQml(output, tabLevel);
}

KeyframeGroupGenerator::KeyframeGroup::KeyFrame::KeyFrame(const AnimationTrack::KeyFrame &keyframe,
                                                          ValueType type, const QString &field,
                                                          float fps)
{
    valueType = type;
    frame = qRound(keyframe.time * fps);
    setValue(keyframe.value, field);
    c2time = keyframe.c2time;
    c2value = keyframe.c2value;
    c1time = keyframe.c1time;
    c1value = keyframe.c1value;
}

void KeyframeGroupGenerator::KeyframeGroup::KeyFrame::setValue(float newValue, const QString &field)
{
    if (valueType == ValueType::Float)
        value.setX(newValue);
    else if (field == QStringLiteral("x"))
        value.setX(newValue);
    else if (field == QStringLiteral("y"))
        value.setY(newValue);
    else if (field == QStringLiteral("z"))
        value.setZ(newValue);
    else if (field == QStringLiteral("w"))
        value.setW(newValue);
    else
        value.setX(newValue);
}

QString KeyframeGroupGenerator::KeyframeGroup::KeyFrame::valueToString() const
{
    if (valueType == ValueType::Float)
        return QString::number(double(value.x()));
    if (valueType == ValueType::Vector2D) {
        return QString(QStringLiteral("Qt.vector2d(") + QString::number(double(value.x())) +
                       QStringLiteral(", ") + QString::number(double(value.y())) +
                       QStringLiteral(")"));
    }
    if (valueType == ValueType::Vector3D) {
        return QString(QStringLiteral("Qt.vector3d(") + QString::number(double(value.x())) +
                       QStringLiteral(", ") + QString::number(double(value.y())) +
                       QStringLiteral(", ") + QString::number(double(value.z())) +
                       QStringLiteral(")"));
    }
    if (valueType == ValueType::Vector4D) {
        return QString(QStringLiteral("Qt.vector4d(") + QString::number(double(value.x())) +
                       QStringLiteral(", ") + QString::number(double(value.y())) +
                       QStringLiteral(", ") + QString::number(double(value.z())) +
                       QStringLiteral(", ") + QString::number(double(value.w())) +
                       QStringLiteral(")"));
    }
    if (valueType == ValueType::Color) {
        return QLatin1Char('\"')
                + QColor::fromRgbF(double(value.x()), double(value.y()),
                                   double(value.z()), double(value.w())).name(QColor::HexArgb)
                + QLatin1Char('\"');
    }
    Q_UNREACHABLE();
    return QString();
}

KeyframeGroupGenerator::KeyframeGroup::KeyframeGroup(const AnimationTrack &animation,
                                                     const QString &p, const QString &field,
                                                     float fps)
{
    type = KeyframeGroup::AnimationType(animation.m_type);
    target = animation.m_target;
    property = getQmlPropertyName(p); // convert to qml property
    isDynamic = animation.m_dynamic;
    for (const auto &keyframe : animation.m_keyFrames) {
        keyframes.append(new KeyFrame(keyframe, getPropertyValueType(target->type(), p),
                                      field, fps));
    }
}

KeyframeGroupGenerator::KeyframeGroup::~KeyframeGroup()
{
    for (auto keyframe : keyframes)
        delete keyframe;
}

void KeyframeGroupGenerator::KeyframeGroup::generateKeyframeGroupQml(QTextStream &output,
                                                                     int tabLevel) const
{
    output << Qt::endl;
    output << QSSGQmlUtilities::insertTabs(tabLevel) << "KeyframeGroup {\n";
    output << QSSGQmlUtilities::insertTabs(tabLevel + 1) << QStringLiteral("target: ")
           << target->qmlId() << Qt::endl;
    output << QSSGQmlUtilities::insertTabs(tabLevel + 1) << QStringLiteral("property: ")
           << '"' << property << "\"\n";

    for (auto keyframe : keyframes) {
        output << QSSGQmlUtilities::insertTabs(tabLevel + 1) << "Keyframe {\n";
        output << QSSGQmlUtilities::insertTabs(tabLevel + 2) << QStringLiteral("frame: ")
               << keyframe->frame << Qt::endl;
        // special handling just for opacity value
        if (property == QLatin1String("opacity")) {
            output << QSSGQmlUtilities::insertTabs(tabLevel + 2) << QStringLiteral("value: ")
                   << QString::number(double(keyframe->value.x()) * 0.01) << Qt::endl;
        } else {
            output << QSSGQmlUtilities::insertTabs(tabLevel + 2) << QStringLiteral("value: ")
                   << keyframe->valueToString() << Qt::endl;
        }

        // ### Only linear supported at the moment, add support for EaseInOut and Bezier

        output << QSSGQmlUtilities::insertTabs(tabLevel + 1) << "}\n";
    }

    output << QSSGQmlUtilities::insertTabs(tabLevel) << "}\n";
}

KeyframeGroupGenerator::KeyframeGroup::KeyFrame::ValueType
KeyframeGroupGenerator::KeyframeGroup::getPropertyValueType(GraphObject::Type type,
                                                            const QString &propertyName)
{
    PropertyMap *propertyMap = PropertyMap::instance();
    const auto properties = propertyMap->propertiesForType(type);
    if (properties->contains(propertyName)) {
        switch (properties->value(propertyName).type) {
        case Q3DS::PropertyType::FloatRange:
        case Q3DS::PropertyType::LongRange:
        case Q3DS::PropertyType::Float:
        case Q3DS::PropertyType::Long:
        case Q3DS::PropertyType::FontSize:
            return KeyFrame::ValueType::Float;
        case Q3DS::PropertyType::Float2:
            return KeyFrame::ValueType::Vector2D;
        case Q3DS::PropertyType::Vector:
        case Q3DS::PropertyType::Scale:
        case Q3DS::PropertyType::Rotation:
            return KeyFrame::ValueType::Vector3D;
        case Q3DS::PropertyType::Color:
            return KeyFrame::ValueType::Color;
        default:
            return KeyFrame::ValueType::Unhandled;
        }
    }
    return KeyFrame::ValueType::Unhandled;
}

QString KeyframeGroupGenerator::KeyframeGroup::getQmlPropertyName(const QString &propertyName)
{
    PropertyMap *propertyMap = PropertyMap::instance();
    const auto properties = propertyMap->propertiesForType(target->type());
    if (properties->contains(propertyName)) {
        return properties->value(propertyName).name;
    }

    return propertyName;
}

QT_END_NAMESPACE
