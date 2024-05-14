// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qopenxrspatialanchormodel_p.h"
#include "qopenxrspatialanchor_p.h"
#include "qopenxrspaceextension_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype XrSpatialAnchorModel
    \inherits ListModel
    \inqmlmodule QtQuick3D.Xr
    \brief Provides a model for managing spatial anchors.

    This type provides a way to manage spatial anchors, which are points in
    the physical world that can be tracked and associated with virtual content.

    You can use it like so:
    \qml
    XrSpatialAnchorModel {
        id: anchorModel
        filterMode: XrSpatialAnchorModel.Labels
        labels: XrSpatialAnchorModel.Ceiling | XrSpatialAnchorModel.Floor
        // ... other properties and methods ...
    \endqml
    }
*/

QOpenXRSpatialAnchorModel::QOpenXRSpatialAnchorModel(QObject *parent)
    : QAbstractListModel{parent}
{
    m_spaceExtension = QOpenXRSpaceExtension::instance();
    queryAnchors();
    connect(m_spaceExtension, &QOpenXRSpaceExtension::anchorAdded, this, &QOpenXRSpatialAnchorModel::handleAnchorAdded);
}

int QOpenXRSpatialAnchorModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid() || m_spaceExtension == nullptr)
        return 0;

    return m_spaceExtension->anchors().count();
}

QVariant QOpenXRSpatialAnchorModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || m_spaceExtension == nullptr)
        return QVariant();

    const auto &anchors = m_spaceExtension->anchors();

    // check bounds
    if (index.row() < 0 || index.row() >= anchors.count())
        return QVariant();

    const auto &anchor = anchors.at(index.row());

    if (role == Anchor)
        return QVariant::fromValue(anchor);

    // shouldn't be reachable under normal circumstances.
    return QVariant();
}

QHash<int, QByteArray> QOpenXRSpatialAnchorModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[Anchor] = "anchor";
    return roles;
}

void QOpenXRSpatialAnchorModel::requestSceneCapture()
{
    if (m_spaceExtension == nullptr)
        return;

    // not supported on the Simulator, this will be a no-op there
    m_spaceExtension->requestSceneCapture();
}

void QOpenXRSpatialAnchorModel::queryAnchors()
{
    if (m_spaceExtension == nullptr)
        return;

    m_spaceExtension->queryAllAnchors();
}

void QOpenXRSpatialAnchorModel::handleAnchorAdded(QOpenXRSpatialAnchor *anchor)
{
    Q_UNUSED(anchor)
    // Brute Force :-p
    beginResetModel();
    endResetModel();
}

/*!
    \qmlproperty enumeration XrSpatialAnchorModel::filterMode
    \brief Specifies the filter mode for spatial anchors.

    Holds the filter mode.
    The filter mode can be one of the following:
    \value All Show all spatial anchors.
    \value Label Show spatial anchors based on semantic labels.
    \value UUID  Show spatial anchors based on UUIDs.
 */

QOpenXRSpatialAnchorModel::FilterMode QOpenXRSpatialAnchorModel::filterMode() const
{
    return m_filterMode;
}

void QOpenXRSpatialAnchorModel::setFilterMode(FilterMode newFilterMode)
{
    if (m_filterMode == newFilterMode)
        return;
    m_filterMode = newFilterMode;
    emit filterModeChanged();
}

/*!
    \qmlproperty list XrSpatialAnchorModel::uuids
    \brief Holds the list of UUIDs for filtering spatial anchors.
 */

QList<QUuid> QOpenXRSpatialAnchorModel::uuids() const
{
    return m_uuids;
}

void QOpenXRSpatialAnchorModel::setUuids(const QList<QUuid> &newUuids)
{
    if (m_uuids == newUuids)
        return;
    m_uuids = newUuids;
    emit uuidsChanged();
}

/*!
    \qmlproperty enumeration XrSpatialAnchorModel::labels
    \brief  Holds the semantic labels used for filtering spatial anchors.

    The semantic labels are represented as a combination of flags:

    \value Ceiling
    \value DoorFrame
    \value Floor
    \value WallArt
    \value WallFace
    \value WindowFrame
    \value Couch
    \value Table
    \value Bed
    \value Lamp
    \value Plant
    \value Screen
    \value Storage
    \value Other
 */

QOpenXRSpatialAnchorModel::SemanticLabels QOpenXRSpatialAnchorModel::labels() const
{
    return m_labels;
}

void QOpenXRSpatialAnchorModel::setLabels(const SemanticLabels &newLabels)
{
    if (m_labels == newLabels)
        return;
    m_labels = newLabels;
    emit labelsChanged();
}

/*!
    \qmlsignal XrSpatialAnchorModel::filterModeChanged()
    \brief Emitted when the filter mode changes.
 */

/*!
    \qmlsignal XrSpatialAnchorModel::uuidsChanged()
    \brief Emitted when the list of UUIDs changes.
 */

/*!
    \qmlsignal XrSpatialAnchorModel::labelsChanged()
    \brief Emitted when the semantic labels changes.
*/

QT_END_NAMESPACE
