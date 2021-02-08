/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "randominstancing_p.h"
#include <QRandomGenerator>
#include <QObject>

QT_BEGIN_NAMESPACE


/*!
    \qmltype InstanceRange
    \inherits Object3D
    \inqmlmodule QtQuick3D.Helpers
    \brief Specifies a range for RandomInstancing.

    \preliminary

    The InstanceRange QML type is used to specify the range of variation for
    RandomInstancing attributes.
*/

/*!
    \qmlproperty Variant InstanceRange::from

    \preliminary

    This property specifies the lower bound of the range. The type needs to match the type of the attribute that this range is applied to.
*/

/*!
    \qmlproperty Variant InstanceRange::to

    \preliminary

    This property specifies the upper bound of the range. The type needs to match the type of the attribute that this range is applied to.
*/

/*!
    \qmlproperty bool InstanceRange::proportional

    \preliminary

    This property determines whether the components of the attribute vary proportionally or independently.
    The default value is \c true, meaning that all components are independent.

    For example, the following defines a grayscale color range:
    \qml
        InstanceRange {
            from: "black"
            to: "white"
            proportional: true
        }
    \endqml
    While the following defines a range that covers all colors
    \qml
        InstanceRange {
            from: "black"
            to: "white"
        }
    \endqml
*/

/*!
    \qmltype RandomInstancing
    \inherits Instancing
    \inqmlmodule QtQuick3D.Helpers
    \brief Generates a random instancing table.

    \preliminary

    The RandomInstancing type provides an easy way to generate a large number of
    random instances within defined bounds. The number of instances is defined by the
    \l instanceCount property. The bounds are defined by the properties
    \l position, \l scale, \l rotation, \l color, and \l customData.

    \sa InstanceList
*/

/*!
    \qmlproperty int RandomInstancing::instanceCount

    \preliminary

    The instanceCount property specifies the number of instances to generate. Changing this value will regenerate the whole table.

    \sa randomSeed
*/

/*!
    \qmlproperty int RandomInstancing::randomSeed

    \preliminary

    This property defines the seed for the random number generator. Setting this to a value
    different from -1 guarantees that the instance table will have the same content each time it is generated.
    Note that adding or changing attributes may cause a completely different table to be generated.

    The default value is -1, causing the table to get a new random value each time it is generated.
*/

/*!
    \qmlproperty InstanceRange RandomInstancing::position

    \preliminary

    The position property defines the geometrical bounds of the generated instances.
    The default value is empty, causing a generated position of \c{[0, 0, 0]}.

    \sa color, rotation, scale, customData

*/

/*!
    \qmlproperty InstanceRange RandomInstancing::scale

    \preliminary

    The scale property defines the scaling limits for the generated instances. The type is
    \l vector3d.
    Set \l {InstanceRange::proportional}{InstanceRange.proportional} to \c true for uniform scaling.
    The default value is empty, causing no scaling to be applied.

    \sa position, color, rotation, scale, customData
*/

/*!
    \qmlproperty InstanceRange RandomInstancing::rotation

    \preliminary

    The rotation property defines the rotation range for the generated instances. The type is
    \l vector3d, corresponding to a Euler rotation vector \c{[xRotation, yRotation, zRotation]}.
    The default value is empty, causing no rotation to be applied.

    \sa position, color, scale, customData
*/

/*!
    \qmlproperty InstanceRange RandomInstancing::color

    \preliminary

    The color property defines the color variation range for the generated instances. Thew type is \l color.
    Set \l {InstanceRange::proportional}{InstanceRange.proportional} to \c true for monochrome colors.
    The default value is empty, causing the color to be white.

    \sa position, rotation, scale, customData
*/

/*!
    \qmlproperty InstanceRange RandomInstancing::customData

    \preliminary

    The customData property defines the custom data variation range for the generated instances.
    The type is \l vector4d.
    The default value is empty, causing causing the generated data to be \c{[0, 0, 0, 0]}.

    \sa position, color, rotation, scale, customData
*/

QQuick3DRandomInstancing::QQuick3DRandomInstancing(QQuick3DObject *parent)
    : QQuick3DInstancing(parent)
{

}

QQuick3DRandomInstancing::~QQuick3DRandomInstancing()
{
}

void QQuick3DRandomInstancing::setInstanceCount(int instanceCount)
{
    if (instanceCount == m_randomCount)
        return;
    m_randomCount = instanceCount;
    m_dirty = true;
    markDirty();
}

void QQuick3DRandomInstancing::setRandomSeed(int randomSeed)
{
    if (m_randomSeed == randomSeed)
        return;

    m_randomSeed = randomSeed;
    emit randomSeedChanged(m_randomSeed);
    m_dirty = true;
    markDirty();
}

void QQuick3DRandomInstancing::setPosition(QQuick3DInstanceRange *position)
{
    if (m_position == position)
        return;

    if (m_position)
        disconnect(m_position,  &QQuick3DInstanceRange::changed, this, &QQuick3DRandomInstancing::handleChange);
    m_position = position;
    emit positionChanged(m_position);
    m_dirty = true;
    markDirty();
    if (m_position) {
        connect(m_position, &QQuick3DInstanceRange::changed, this, &QQuick3DRandomInstancing::handleChange);
        connect(m_position, &QObject::destroyed, this, [this](QObject *obj){ if (obj == m_position) m_position = nullptr; });
    }
}

void QQuick3DRandomInstancing::setScale(QQuick3DInstanceRange *scale)
{
    if (m_scale == scale)
        return;

    if (m_scale)
        disconnect(m_scale,  &QQuick3DInstanceRange::changed, this, &QQuick3DRandomInstancing::handleChange);
    m_scale = scale;
    emit scaleChanged(m_scale);
    m_dirty = true;
    markDirty();
    if (m_scale) {
        connect(m_scale, &QQuick3DInstanceRange::changed, this, &QQuick3DRandomInstancing::handleChange);
        connect(m_scale, &QObject::destroyed, this, [this](QObject *obj){ if (obj == m_scale) m_scale = nullptr; });
    }
}

void QQuick3DRandomInstancing::setRotation(QQuick3DInstanceRange *rotation)
{
    if (m_rotation == rotation)
        return;

    if (m_rotation)
        disconnect(m_rotation,  &QQuick3DInstanceRange::changed, this, &QQuick3DRandomInstancing::handleChange);
    m_rotation = rotation;
    emit rotationChanged(m_rotation);
    m_dirty = true;
    markDirty();
    if (m_rotation) {
        connect(m_rotation, &QQuick3DInstanceRange::changed, this, &QQuick3DRandomInstancing::handleChange);
        connect(m_rotation, &QObject::destroyed, this, [this](QObject *obj){ if (obj == m_rotation) m_rotation = nullptr; });
    }
}

void QQuick3DRandomInstancing::setColor(QQuick3DInstanceRange *color)
{
    if (m_color == color)
        return;

    if (m_color)
        disconnect(m_color,  &QQuick3DInstanceRange::changed, this, &QQuick3DRandomInstancing::handleChange);
    m_color = color;
    emit colorChanged(m_color);
    m_dirty = true;
    markDirty();
    if (m_color) {
        connect(m_color, &QQuick3DInstanceRange::changed, this, &QQuick3DRandomInstancing::handleChange);
        connect(m_color, &QObject::destroyed, this, [this](QObject *obj){ if (obj == m_color) m_color = nullptr; });
    }

}

void QQuick3DRandomInstancing::setCustomData(QQuick3DInstanceRange *customData)
{
    if (m_customData == customData)
        return;

    if (m_customData)
        disconnect(m_customData,  &QQuick3DInstanceRange::changed, this, &QQuick3DRandomInstancing::handleChange);
    m_customData = customData;
    emit customDataChanged(m_customData);
    m_dirty = true;
    markDirty();
    if (m_customData) {
        connect(m_customData, &QQuick3DInstanceRange::changed, this, &QQuick3DRandomInstancing::handleChange);
        connect(m_customData, &QObject::destroyed, this, [this](QObject *obj){ if (obj == m_customData) m_customData = nullptr; });
    }
}

void QQuick3DRandomInstancing::handleChange()
{
    m_dirty = true;
    markDirty();
}

static inline float genRandom(float from, float to, QRandomGenerator *rgen)
{
    float c = rgen->bounded(1.0);
    return from + c * (to - from);
}

static QVector3D genRandom(const QVector3D &from, const QVector3D &to, bool proportional, QRandomGenerator *rgen)
{
    if (proportional) {
        float c = rgen->bounded(1.0);
        return from + c * (to - from);
    }
    return { genRandom(from.x(), to.x(), rgen), genRandom(from.y(), to.y(), rgen), genRandom(from.z(), to.z(), rgen) };
}

static QVector4D genRandom(const QVector4D &from, const QVector4D &to, bool proportional, QRandomGenerator *rgen)
{
    if (proportional) {
        float c = rgen->bounded(1.0);
        return from + c * (to - from);
    }
    return { genRandom(from.x(), to.x(), rgen), genRandom(from.y(), to.y(), rgen), genRandom(from.z(), to.z(), rgen), genRandom(from.w(), to.w(), rgen) };
}

static QColor genRandom(const QColor &from, const QColor &to, bool proportional, QRandomGenerator *rgen)
{
    QVector4D v1, v2;
    from.getRgbF(&v1[0], &v1[1], &v1[2], &v1[3]);
    to.getRgbF(&v2[0], &v2[1], &v2[2], &v2[3]);
    QVector4D r = genRandom(v1, v2, proportional, rgen);

    return QColor::fromRgbF(r[0], r[1], r[2], r[3]);
}

QByteArray QQuick3DRandomInstancing::getInstanceBuffer(int *instanceCount)
{
    if (m_dirty)
        generateInstanceTable();
    if (instanceCount)
        *instanceCount = m_randomCount;
    return m_instanceData;
}

void QQuick3DRandomInstancing::generateInstanceTable()
{
    m_dirty = false;
    const int count = m_randomCount;

    QRandomGenerator rgen(m_randomSeed);
    if (m_randomSeed == -1)
        rgen.seed(QRandomGenerator::global()->generate());

    qsizetype tableSize = count * sizeof(InstanceTableEntry);
    m_instanceData.resize(tableSize);

    //qDebug() << "generating" << count << "instances, for total size" << tableSize;
    auto *array = reinterpret_cast<InstanceTableEntry*>(m_instanceData.data());
    for (int i = 0; i < count; ++i) {
        QVector3D pos;
        QVector3D scale{1, 1, 1};
        QVector3D eulerRotation;
        QColor color(Qt::white);
        QVector4D customData;
        if (m_position)
            pos = genRandom(m_position->from().value<QVector3D>(), m_position->to().value<QVector3D>(), m_position->proportional(), &rgen);
        if (m_scale)
            scale = genRandom(m_scale->from().value<QVector3D>(), m_scale->to().value<QVector3D>(), m_scale->proportional(), &rgen);
        if (m_rotation) //TODO: quaternion rotation???
            eulerRotation = genRandom(m_rotation->from().value<QVector3D>(), m_rotation->to().value<QVector3D>(), m_rotation->proportional(), &rgen);
        if (m_color)
            color = genRandom(m_color->from().value<QColor>(), m_color->to().value<QColor>(), m_color->proportional(), &rgen);
        if (m_customData)
            customData = genRandom(m_customData->from().value<QVector4D>(), m_customData->to().value<QVector4D>(), m_customData->proportional(), &rgen);

        array[i] = calculateTableEntry(pos, scale, eulerRotation, color, customData);
    }
}


QQuick3DInstanceRange::QQuick3DInstanceRange(QQuick3DObject *parent)
    : QQuick3DObject(parent)
{

}

void QQuick3DInstanceRange::setFrom(QVariant from)
{
    if (m_from == from)
        return;

    m_from = from;
    emit fromChanged(m_from);
    emit changed();
}

void QQuick3DInstanceRange::setTo(QVariant to)
{
    if (m_to == to)
        return;

    m_to = to;
    emit toChanged(m_to);
    emit changed();
}

void QQuick3DInstanceRange::setProportional(bool proportional)
{
    if (m_proportional == proportional)
        return;

    m_proportional = proportional;
    emit proportionalChanged(m_proportional);
    emit changed();
}

QT_END_NAMESPACE
