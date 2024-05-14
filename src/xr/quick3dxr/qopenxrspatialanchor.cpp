// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qopenxrspatialanchor_p.h"

#include "qopenxrspaceextension_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype XrSpatialAnchor
    \inherits Item
    \inqmlmodule QtQuick3D.Xr
    \brief Represents a spatial anchor in an OpenXR session.

    This type represents a spatial anchor that can be used to track
    a specific location or object in real space. It provides information about
    the anchor's position, rotation, semantic labels, and bounds.

    They are accessed through an \l XrSpatialAnchorModel.

    \note You can not create these in QML.
 */

QOpenXRSpatialAnchor::QOpenXRSpatialAnchor(XrSpace space, QUuid &uuid, QObject *parent)
    : QObject(parent)
    , m_space(space)
    , m_uuid(uuid)
{
    auto spaceExt = QOpenXRSpaceExtension::instance();
    m_has2DBounds = spaceExt->getBoundingBox2D(m_space, m_offset2D, m_extent2D);
    m_has3DBounds = spaceExt->getBoundingBox3D(m_space, m_offset3D, m_extent3D);
    if (spaceExt->isComponentSupported(m_space, XR_SPACE_COMPONENT_TYPE_SPACE_CONTAINER_FB) &&
        spaceExt->isComponentEnabled(m_space, XR_SPACE_COMPONENT_TYPE_SPACE_CONTAINER_FB)) {
        // Get the space container UUIDs
        m_spaceContainerUuids = spaceExt->collectSpaceContainerUuids(m_space);
    } else if (spaceExt->isComponentSupported(m_space, XR_SPACE_COMPONENT_TYPE_ROOM_LAYOUT_FB) &&
               spaceExt->isComponentEnabled(m_space, XR_SPACE_COMPONENT_TYPE_ROOM_LAYOUT_FB)) {
        m_roomLayoutUuids = spaceExt->collectRoomLayoutUuids(m_space);
    }
    m_semanticLabels = spaceExt->getSemanticLabels(m_space);
}

QOpenXRSpatialAnchor::~QOpenXRSpatialAnchor()
{

}

XrSpace QOpenXRSpatialAnchor::space() const
{
    return m_space;
}

/*!
    \qmlproperty vector3d XrSpatialAnchor::offset3D
    \brief The 3D offset of the spatial anchor.

    This property provides the 3D offset (in meters) from the anchor's origin to
    its position within the 3D space.
 */

QVector3D QOpenXRSpatialAnchor::offset3D() const
{
    return m_offset3D;
}

void QOpenXRSpatialAnchor::setOffset3D(const QVector3D &newOffset)
{
    if (m_offset3D == newOffset)
        return;
    m_offset3D = newOffset;
    emit offset3DChanged();
}

/*!
    \qmlproperty vector3d XrSpatialAnchor::extent3D
    \brief The 3D extent of the spatial anchor.

    This property specifies the 3D size (width, height, and depth) of the spatial anchor
    within the 3D space.
 */

QVector3D QOpenXRSpatialAnchor::extent3D() const
{
    return m_extent3D;
}

void QOpenXRSpatialAnchor::setExtent3D(const QVector3D &newExtent)
{
    if (m_extent3D == newExtent)
        return;
    m_extent3D = newExtent;
    emit extent3DChanged();
}

/*!
    \qmlproperty vector3d XrSpatialAnchor::position
    \brief The position of the spatial anchor.

    This property returns the 3D position (in meters) of the spatial anchor within the
    session's coordinate system. It emits the `positionChanged` signal when updated.
 */

QVector3D QOpenXRSpatialAnchor::position() const
{
    return m_position;
}

void QOpenXRSpatialAnchor::setPosition(const QVector3D &newPosition)
{
    if (m_position == newPosition)
        return;
    m_position = newPosition;
    emit positionChanged();
}

/*!
    \qmlproperty quaternion XrSpatialAnchor::rotation
    \brief The rotation of the spatial anchor.

    This property provides the 3D rotation (as a quaternion) of the spatial anchor.
    It emits the `rotationChanged` signal when updated.
 */

QQuaternion QOpenXRSpatialAnchor::rotation() const
{
    return m_rotation;
}

void QOpenXRSpatialAnchor::setRotation(const QQuaternion &newRotation)
{
    if (m_rotation == newRotation)
        return;
    m_rotation = newRotation;
    emit rotationChanged();
}

/*!
    \qmlproperty string XrSpatialAnchor::semanticLabels
    \brief The semantic labels associated with the spatial anchor.

    This property returns a comma-separated string containing semantic labels
    (for example,\c table or \c chair) describing the anchor's purpose or context.
 */

QString QOpenXRSpatialAnchor::semanticLabels() const
{
    return m_semanticLabels;
}

void QOpenXRSpatialAnchor::setSemanticLabels(const QString &newSemanticLabels)
{
    if (m_semanticLabels == newSemanticLabels)
        return;
    m_semanticLabels = newSemanticLabels;
    emit semanticLabelsChanged();
}

/*!
    \qmlproperty bool XrSpatialAnchor::has2DBounds
    \brief Indicates whether the spatial anchor has 2D bounds.

    This property returns true if the spatial anchor has 2D bounds,
    indicating that it represents a flat surface (for example, a floor or wall)

    Otherwise, it returns false.
 */

bool QOpenXRSpatialAnchor::has2DBounds() const
{
    return m_has2DBounds;
}

/*!
    \qmlproperty bool XrSpatialAnchor::has3DBounds
    \brief Indicates whether the spatial anchor has 3D bounds.

    This property returns true if the spatial anchor has 3D bounds, indicating
    that it represents a volume (for example, a table).
    Otherwise, it returns false.

 */


bool QOpenXRSpatialAnchor::has3DBounds() const
{
    return m_has3DBounds;
}

QVector2D QOpenXRSpatialAnchor::offset2D() const
{
    return m_offset2D;
}

/*!
    \qmlproperty vector2d XrSpatialAnchor::extent2D
    \brief The 2D extent of the spatial anchor.

    This property specifies the 2D size (width and height) of the spatial anchor within
    the 2D plane.
 */

QVector2D QOpenXRSpatialAnchor::extent2D() const
{
    return m_extent2D;
}

/*!
    \qmlproperty QUuid XrSpatialAnchor::uuid
    \brief The unique identifier (UUID) of the spatial anchor.

    This property returns a universally unique identifier (UUID) associated with the
    spatial anchor. This is what is referenced by a \l XrSpatialAnchorModel.
 */

QUuid QOpenXRSpatialAnchor::uuid() const
{
    return m_uuid;
}

/*!
    \qmlsignal XrSpatialAnchor::offset3DChanged()
    \brief Emitted when the 3D offset of the spatial anchor changes.

    This signal indicates that the offset3D property has been updated.
 */

/*!
    \qmlsignal XrSpatialAnchor::extent3DChanged()
    \brief Emitted when the 3D extent of the spatial anchor changes.

    This signal indicates that the extent3D property has been updated.
 */

/*!
    \qmlsignal XrSpatialAnchor::positionChanged()
    \brief Emitted when the position of the spatial anchor changes.

    This signal indicates that the `position` property has been updated.
 */

/*!
    \qmlsignal XrSpatialAnchor::rotationChanged()
    \brief Emitted when the rotation of the spatial anchor changes.

    This signal indicates that the rotation property has been updated.
 */

/*!
    \qmlsignal XrSpatialAnchor::semanticLabelsChanged()
    \brief Emitted when the semantic labels associated with the spatial anchor change.

    This signal indicates that the semanticLabels property has been updated.
 */

/*!
    \qmlsignal XrSpatialAnchor::has2DBoundsChanged()
    \brief Emitted when the 2D bounds state of the spatial anchor changes.

    This signal indicates that the has2DBounds property has been updated.
 */

/*!
    \qmlsignal XrSpatialAnchor::has3DBoundsChanged()
    \brief Emitted when the 3D bounds state of the spatial anchor changes.

    This signal indicates that the has3DBounds property has been updated.
 */

QT_END_NAMESPACE
