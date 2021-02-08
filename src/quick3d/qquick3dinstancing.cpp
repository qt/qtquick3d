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
#include "qquick3dinstancing_p.h"
#include "qquick3dscenemanager_p.h"
#include <QtQuick3DRuntimeRender/private/qssgrenderinstancetable_p.h>
#include <QtQuick3DUtils/private/qssgutils_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype Instancing
    \inherits Object3D
    \inqmlmodule QtQuick3D
    \instantiates QQuick3DInstancing
    \brief Base type for instance tables.

    \preliminary

    Instancing allows duplicating a model with variations. In contrast to
    using a Repeater3D, the model and its graphics resources are only allocated
    once. The rendering of the duplicated instances is done at a low level by the GPU,
    with a single draw call. Depending on the complexity of the model, this can give a
    performance improvement of several orders of magnitude.

    In practice, instancing is done by defining a table that specifies how each instance
    is modified relative to the base model. The table has an entry for each index, containing a
    transform matrix, a color, and generic data for use by custom materials.

    To use instancing, set its \l{Model::instancing}{instancing} property to reference an
    Instancing object.

    An application can define an Instancing object in C++ by subclassing QQuick3DInstancing,
    or it can use one of the pre-defined QML types: InstanceList or RandomInstancing.
*/

/*!
    \qmlproperty int Instancing::instanceCountOverride

    Set this property to limit the number of instances without regenerating or re-uploading the instance table.
    This allows very inexpensive animation of the number of instances rendered.
*/

/*!
    \qmlproperty bool Instancing::hasTransparency

    Set this property to true if the instancing table contains alpha values that should be used when
    rendering the model. This property only makes a difference if the model is opaque: If the model has a
    transparent \l{Model::materials}{material}, or an \l{Node::opacity}{opacity} less than one, the
    alpha value from the table will be used regardless.
*/

/*!
    \class QQuick3DInstancing
    \inmodule QtQuick3D
    \inherits QQuick3DObject
    \since 6.1
    \brief Base class for defining instance tables.

    \preliminary

    The QQuick3DInstancing class can be inherited to specify a custom instance table
    for a Model in the Qt Quick 3D scene.

    This class is abstract: To use it, create a subclass and implement \l getInstanceBuffer().
    The subclass is then exposed to QML by registering it to the type
    system. The \l{Model::instancing}{instancing} property of a Model can then be
    set to reference an object of the registered type.
*/

/*!
    \fn QByteArray QQuick3DInstancing::getInstanceBuffer(int *instanceCount)

    \preliminary

    Implement this function to return the contents of the instance table. The number of instances should be
    returned in \a instanceCount. The subclass is responsible for caching the result if necessary.
 */

QQuick3DInstancingPrivate::QQuick3DInstancingPrivate()
    : QQuick3DObjectPrivate(QQuick3DObjectPrivate::Type::ModelInstance)
{
}

QQuick3DInstancing::QQuick3DInstancing(QQuick3DObject *parent)
    : QQuick3DObject(*new QQuick3DInstancingPrivate, parent)
{
}

QQuick3DInstancing::~QQuick3DInstancing()
{
}

QByteArray QQuick3DInstancing::instanceBuffer(int *instanceCount)
{
    Q_D(QQuick3DInstancing);
    QByteArray retval = getInstanceBuffer(instanceCount);
    if (instanceCount && d->m_instanceCountOverride >= 0)
        *instanceCount = qMin(d->m_instanceCountOverride, *instanceCount);
    return retval;
}

int QQuick3DInstancing::instanceCountOverride() const
{
    Q_D(const QQuick3DInstancing);
    return d->m_instanceCountOverride;
}

bool QQuick3DInstancing::hasTransparency() const
{
    Q_D(const QQuick3DInstancing);
    return d->m_hasTransparency;
}

void QQuick3DInstancing::setInstanceCountOverride(int instanceCountOverride)
{
    Q_D(QQuick3DInstancing);
    if (d->m_instanceCountOverride == instanceCountOverride)
        return;

    d->m_instanceCountOverride = instanceCountOverride;
    d->dirty(QQuick3DObjectPrivate::DirtyType::Content);
    emit instanceCountOverrideChanged();
}

void QQuick3DInstancing::setHasTransparency(bool hasTransparency)
{
    Q_D(QQuick3DInstancing);
    if (d->m_hasTransparency == hasTransparency)
        return;

    d->m_hasTransparency = hasTransparency;
    d->dirty(QQuick3DObjectPrivate::DirtyType::Content);
    emit hasTransparencyChanged();
}

void QQuick3DInstancing::markDirty()
{
    Q_D(QQuick3DInstancing);
    d->dirty(QQuick3DObjectPrivate::DirtyType::Content);
    d->m_instanceDataChanged = true;
}

QSSGRenderGraphObject *QQuick3DInstancing::updateSpatialNode(QSSGRenderGraphObject *node)
{
    Q_D(QQuick3DInstancing);
    if (!node) {
        markAllDirty();
        node = new QSSGRenderInstanceTable();
        emit instanceNodeDirty();
        d->m_instanceDataChanged = true;
    }
    auto *instanceTable = static_cast<QSSGRenderInstanceTable *>(node);
    if (d->m_instanceDataChanged) {
        int count;
        QByteArray buffer = instanceBuffer(&count);
        //   qDebug() << "QQuick3DInstancing:updateSpatialNode setting instance buffer data" << count;
        instanceTable->setData(buffer, count);
    }
    instanceTable->setHasTransparency(d->m_hasTransparency);
    return node;
}

//TODO: Do we need an Instancing subclass that takes a QQmlModel?

static inline QQuick3DInstancing::InstanceTableEntry calculate(const QVector3D &position, const QVector3D &scale,
                                                                         const QVector3D &eulerRotation, const QColor &color,
                                                                         const QVector4D &customData)
{
    QMatrix4x4 xform;

    xform(0, 0) = scale[0];
    xform(1, 1) = scale[1];
    xform(2, 2) = scale[2];

    QQuaternion quaternion = QQuaternion::fromEulerAngles(eulerRotation);
    xform = QMatrix4x4(quaternion.toRotationMatrix()) * xform;

    xform(0, 3) += position[0];
    xform(1, 3) += position[1];
    xform(2, 3) += position[2];

    auto linearColor = color::sRGBToLinear(color);

    return {
        xform.row(0),
        xform.row(1),
        xform.row(2),
        linearColor,
        customData
    };
}

/*!
    \preliminary

   Converts the
   \a position
   \a scale
   \a eulerRotation
   \a color
   and
   \a customData
   to the instance table format expected by the standard vertex shaders. Typical pattern:

   \code
    QByteArray MyInstanceTable::getInstanceBuffer(int *instanceCount)
    {
        QByteArray instanceData;

        ...

        auto entry = calculateTableEntry({xPos, yPos, zPos}, {xScale, yScale, zScale}, {xRot, yRot, zRot}, color, {});
        instanceData.append(reinterpret_cast<const char *>(&entry), sizeof(entry));
   \endcode

   \sa calculateTableEntryFromQuaternion
 */
QQuick3DInstancing::InstanceTableEntry QQuick3DInstancing::calculateTableEntry(const QVector3D &position, const QVector3D &scale,
                                   const QVector3D &eulerRotation, const QColor &color, const QVector4D &customData)
{
    return calculate(position, scale, eulerRotation, color, customData);
}

/*!
    \preliminary

   Converts the
   \a position
   \a scale
   \a rotation
   \a color
   and
   \a customData
   to the instance table format expected by the standard vertex shaders. Typical pattern:

    This is the same as calculateTableEntry(), except for using a quaternion to specify the rotation.
 */
QQuick3DInstancing::InstanceTableEntry QQuick3DInstancing::calculateTableEntryFromQuaternion(const QVector3D &position, const QVector3D &scale, const QQuaternion &rotation, const QColor &color, const QVector4D &customData)
{
    QMatrix4x4 xform;

    xform(0, 0) = scale[0];
    xform(1, 1) = scale[1];
    xform(2, 2) = scale[2];

    xform = QMatrix4x4(rotation.toRotationMatrix()) * xform;

    xform(0, 3) += position[0];
    xform(1, 3) += position[1];
    xform(2, 3) += position[2];

    auto linearColor = color::sRGBToLinear(color);

    return {
        xform.row(0),
        xform.row(1),
        xform.row(2),
        linearColor,
        customData
    };
}

/*!
    \qmltype InstanceList
    \inherits Instancing
    \inqmlmodule QtQuick3D
    \brief Allows manually specifying instancing in QML.

    \preliminary

    The InstanceList type makes it possible to define an instance table manually in QML.

    The following example creates an instance table with two items:
    \qml
    InstanceList {
        id: manualInstancing
        instances: [
            InstanceListEntry {
                position: Qt.vector3d(0, 0, -60)
                eulerRotation: Qt.vector3d(-10, 0, 30)
                color: "red"
            },
            InstanceListEntry {
                position: Qt.vector3d(50, 10, 100)
                eulerRotation: Qt.vector3d(0, 180, 0)
                color: "green"
            }
        ]
    }
    \endqml

    Each InstanceListEntry is an object that can have property bindings and animations. This gives great flexibility, but also
    causes memory overhead. Therefore, it is not recommended to use InstanceList for procedurally generated tables containing thousands
    (or millions) of instances.

    \sa RandomInstancing, QQuick3DInstancing
*/

/*!
    \qmlproperty List<QtQuick3D::InstanceListEntry> InstanceList::instances

    \preliminary

    This property contains the list of instance definitions. Modifying this list, or any of its elements, will cause the instance table to be updated.
*/

QQuick3DInstanceList::QQuick3DInstanceList(QQuick3DObject *parent) : QQuick3DInstancing(parent) {}

QQuick3DInstanceList::~QQuick3DInstanceList() {}

QByteArray QQuick3DInstanceList::getInstanceBuffer(int *instanceCount)
{
    if (m_dirty)
        generateInstanceData();
    if (instanceCount)
        *instanceCount = m_instances.size();
    return m_instanceData;
}

QQmlListProperty<QQuick3DInstanceListEntry> QQuick3DInstanceList::instances()
{

    return QQmlListProperty<QQuick3DInstanceListEntry>(this,
                                                          nullptr,
                                                          qmlAppendInstanceListEntry,
                                                          qmlInstanceListEntriesCount,
                                                          qmlInstanceListEntryAt,
                                                          qmlClearInstanceListEntries);
}

void QQuick3DInstanceList::onInstanceDestroyed(QObject *object)
{
    if (m_instances.removeAll(object))
        handleInstanceChange();
}

void QQuick3DInstanceList::qmlAppendInstanceListEntry(QQmlListProperty<QQuick3DInstanceListEntry> *list, QQuick3DInstanceListEntry *instance)
{
    if (instance == nullptr)
        return;
    auto *self = static_cast<QQuick3DInstanceList *>(list->object);
    self->m_instances.push_back(instance);

    if (instance->parentItem() == nullptr)
        instance->setParentItem(self);
    connect(instance, &QQuick3DInstanceListEntry::changed, self, &QQuick3DInstanceList::handleInstanceChange);
    connect(instance, &QObject::destroyed, self, &QQuick3DInstanceList::onInstanceDestroyed);
    self->handleInstanceChange();
}

QQuick3DInstanceListEntry *QQuick3DInstanceList::qmlInstanceListEntryAt(QQmlListProperty<QQuick3DInstanceListEntry> *list, qsizetype index)
{
    auto *self = static_cast<QQuick3DInstanceList *>(list->object);
    return self->m_instances.at(index);
}

qsizetype QQuick3DInstanceList::qmlInstanceListEntriesCount(QQmlListProperty<QQuick3DInstanceListEntry> *list)
{
    auto *self = static_cast<QQuick3DInstanceList *>(list->object);
    return self->m_instances.count();
}

void QQuick3DInstanceList::qmlClearInstanceListEntries(QQmlListProperty<QQuick3DInstanceListEntry> *list)
{
    auto *self = static_cast<QQuick3DInstanceList *>(list->object);
    for (auto *instance : self->m_instances) {
        disconnect(instance, &QObject::destroyed, self, &QQuick3DInstanceList::onInstanceDestroyed);
        disconnect(instance, &QQuick3DInstanceListEntry::changed, self, &QQuick3DInstanceList::handleInstanceChange);
    }
    self->m_instances.clear();
    self->handleInstanceChange();
}

void QQuick3DInstanceList::handleInstanceChange()
{
    m_dirty = true;
    markDirty();
}

void QQuick3DInstanceList::generateInstanceData()
{
    m_dirty = false;
    const int count = m_instances.size();

    qsizetype tableSize = count * sizeof(InstanceTableEntry);
    m_instanceData.resize(tableSize);
    auto *array = reinterpret_cast<InstanceTableEntry*>(m_instanceData.data());
    for (int i = 0; i < count; ++i) {
        const auto *inst = m_instances.at(i);
        if (inst->m_useEulerRotation)
            array[i] = calculateTableEntry(inst->position(), inst->scale(), inst->eulerRotation(), inst->color(), inst->customData());
        else
            array[i] = calculateTableEntryFromQuaternion(inst->position(), inst->scale(), inst->rotation(), inst->color(), inst->customData());
    }
}

/*!
    \qmltype InstanceListEntry
    \inherits Object3D
    \inqmlmodule QtQuick3D
    \brief Specifies an instance in an InstanceList.

    \preliminary

    The InstanceListEntry QML type is used to specify one instance in an instance list.
*/


QQuick3DInstanceListEntry::QQuick3DInstanceListEntry(QQuick3DObject *parent)
    : QQuick3DObject(parent)
{
}

/*!
    \qmlproperty vector3d QtQuick3D::InstanceListEntry::position

    \preliminary

    This property specifies the position for the instance.
*/
void QQuick3DInstanceListEntry::setPosition(QVector3D position)
{
    if (m_position == position)
        return;

    m_position = position;
    emit positionChanged();
    emit changed();
}

/*!
    \qmlproperty vector3d QtQuick3D::InstanceListEntry::scale

    \preliminary

    This property specifies the scale for the instance as a vector containing the scale factor along the x, y and z axes.
*/
void QQuick3DInstanceListEntry::setScale(QVector3D scale)
{
    if (m_scale == scale)
        return;

    m_scale = scale;
    emit scaleChanged();
    emit changed();
}

/*!
    \qmlproperty vector3d QtQuick3D::InstanceListEntry::eulerRotation

    \preliminary

    This property specifies the rotation for the instance as an Euler vector, that
    is a vector containing the rotation in degrees around the x, y and z axes.
*/
void QQuick3DInstanceListEntry::setEulerRotation(QVector3D eulerRotation)
{
    if (m_eulerRotation == eulerRotation)
        return;
    m_eulerRotation = eulerRotation;
    m_useEulerRotation = true;
    emit eulerRotationChanged();
    emit changed();
}

/*!
    \qmlproperty quaternion QtQuick3D::InstanceListEntry::rotation

    \preliminary

    This property specifies the rotation for the instance as a quaternion.
*/
void QQuick3DInstanceListEntry::setRotation(QQuaternion rotation)
{
    if (m_rotation == rotation)
        return;

    m_rotation = rotation;
    m_useEulerRotation = false;
    emit rotationChanged();
    emit changed();
}

/*!
    \qmlproperty vector3d QtQuick3D::InstanceListEntry::color

    \preliminary

    This property specifies the color for the instance.
*/
void QQuick3DInstanceListEntry::setColor(QColor color)
{
    if (m_color == color)
        return;

    m_color = color;
    emit colorChanged();
    emit changed();
}

/*!
    \qmlproperty vector3d QtQuick3D::InstanceListEntry::customData

    \preliminary

    This property specifies the custom data for the instance. This is not used by default,
    but is made available to the vertex shader of custom materials as \c INSTANCE_DATA.
*/
void QQuick3DInstanceListEntry::setCustomData(QVector4D customData)
{
    if (m_customData == customData)
        return;

    m_customData = customData;
    emit customDataChanged();
    emit changed();
}

QT_END_NAMESPACE
