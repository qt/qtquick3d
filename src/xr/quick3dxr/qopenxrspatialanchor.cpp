// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qopenxrspatialanchor_p.h"

#include "qopenxrspaceextension_p.h"

QT_BEGIN_NAMESPACE

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

bool QOpenXRSpatialAnchor::has2DBounds() const
{
    return m_has2DBounds;
}

bool QOpenXRSpatialAnchor::has3DBounds() const
{
    return m_has3DBounds;
}

QVector2D QOpenXRSpatialAnchor::offset2D() const
{
    return m_offset2D;
}

QVector2D QOpenXRSpatialAnchor::extent2D() const
{
    return m_extent2D;
}

QUuid QOpenXRSpatialAnchor::uuid() const
{
    return m_uuid;
}

QT_END_NAMESPACE
