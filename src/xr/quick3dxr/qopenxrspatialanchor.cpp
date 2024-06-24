// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qopenxrspatialanchor_p.h"

#if defined(Q_OS_VISIONOS)
#include "visionos/qquick3dxranchormanager_visionos_p.h"
#else
#include "openxr/qquick3dxranchormanager_openxr_p.h"
#endif

QT_BEGIN_NAMESPACE

/*!
    \qmltype XrSpatialAnchor
    \inherits Item
    \inqmlmodule QtQuick3D.Xr
    \brief Represents a spatial anchor in an XR session.

    This type represents a spatial anchor that can be used to track
    a specific location or object in real space. It provides information about
    the anchor's position, rotation, semantic labels, and bounds.

    They are accessed through an \l XrSpatialAnchorModel.

    \note You can not create these in QML.
 */

QQuick3DXrSpatialAnchor::QQuick3DXrSpatialAnchor(QtQuick3DXr::XrSpaceId space, QUuid &uuid, QObject *parent)
    : QObject(parent)
    , m_space(space)
    , m_uuid(uuid)
{
}

QQuick3DXrSpatialAnchor::~QQuick3DXrSpatialAnchor()
{

}


/*!
    \qmlproperty vector3d XrSpatialAnchor::offset3D
    \brief The 3D offset of the spatial anchor.

    This property provides the 3D offset (in meters) from the anchor's origin to
    its position within the 3D space.
 */

QVector3D QQuick3DXrSpatialAnchor::offset3D() const
{
    return m_offset3D;
}

void QQuick3DXrSpatialAnchor::setOffset3D(const QVector3D &newOffset)
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

QVector3D QQuick3DXrSpatialAnchor::extent3D() const
{
    return m_extent3D;
}

void QQuick3DXrSpatialAnchor::setExtent3D(const QVector3D &newExtent)
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

QVector3D QQuick3DXrSpatialAnchor::position() const
{
    return m_position;
}

void QQuick3DXrSpatialAnchor::setPosition(const QVector3D &newPosition)
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

QQuaternion QQuick3DXrSpatialAnchor::rotation() const
{
    return m_rotation;
}

void QQuick3DXrSpatialAnchor::setRotation(const QQuaternion &newRotation)
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

QString QQuick3DXrSpatialAnchor::semanticLabels() const
{
    return m_semanticLabels;
}

void QQuick3DXrSpatialAnchor::setSemanticLabels(const QString &newSemanticLabels)
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

bool QQuick3DXrSpatialAnchor::has2DBounds() const
{
    return m_has2DBounds;
}

void QQuick3DXrSpatialAnchor::setBounds2D(const QVector2D &offset, const QVector2D &extent)
{
    m_offset2D = offset;
    m_extent2D = extent;

    // FIXME: verify
    m_has2DBounds = true;
}

/*!
    \qmlproperty bool XrSpatialAnchor::has3DBounds
    \brief Indicates whether the spatial anchor has 3D bounds.

    This property returns true if the spatial anchor has 3D bounds, indicating
    that it represents a volume (for example, a table).
    Otherwise, it returns false.

 */


bool QQuick3DXrSpatialAnchor::has3DBounds() const
{
    return m_has3DBounds;
}

void QQuick3DXrSpatialAnchor::setBounds3D(const QVector3D &offset, const QVector3D &extent)
{
    m_offset3D = offset;
    m_extent3D = extent;
    // FIXME: Store the 3D bounds and verify
    m_has3DBounds = true;
}

QVector2D QQuick3DXrSpatialAnchor::offset2D() const
{
    return m_offset2D;
}

/*!
    \qmlproperty vector2d XrSpatialAnchor::extent2D
    \brief The 2D extent of the spatial anchor.

    This property specifies the 2D size (width and height) of the spatial anchor within
    the 2D plane.
 */

QVector2D QQuick3DXrSpatialAnchor::extent2D() const
{
    return m_extent2D;
}

/*!
    \qmlproperty QUuid XrSpatialAnchor::identifier
    \brief A unique identifier for this spatial anchor.

    This property returns a unique identifier associated with the
    spatial anchor. This is the same identified referenced by a \l XrSpatialAnchorModel.
 */

QString QQuick3DXrSpatialAnchor::identifier() const
{
    return QString::fromLatin1(m_uuid.toRfc4122());
}

QSet<QUuid> QQuick3DXrSpatialAnchor::roomLayoutUuids() const
{
    return m_roomLayoutUuids;
}

void QQuick3DXrSpatialAnchor::setRoomLayoutUuids(const QSet<QUuid> &newRoomLayoutUuids)
{
    m_roomLayoutUuids = newRoomLayoutUuids;
}

QSet<QUuid> QQuick3DXrSpatialAnchor::spaceContainerUuids() const
{
    return m_spaceContainerUuids;
}

void QQuick3DXrSpatialAnchor::setSpaceContainerUuids(const QSet<QUuid> &newSpaceContainerUuids)
{
    m_spaceContainerUuids = newSpaceContainerUuids;
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
