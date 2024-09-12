// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qquick3dxrspatialanchorlistmodel_p.h"
#include "qquick3dxrspatialanchor_p.h"

#if defined(Q_OS_VISIONOS)
#include "visionos/qquick3dxranchormanager_visionos_p.h"
#else
#include "openxr/qquick3dxranchormanager_openxr_p.h"
#endif

QT_BEGIN_NAMESPACE

/*!
    \qmltype XrSpatialAnchorListModel
    \inherits ListModel
    \inqmlmodule QtQuick3D.Xr
    \brief Provides a model containing spatial anchors.

    This type provides a list of spatial anchors, which are points in
    the physical world that can be tracked and associated with virtual content.

    The list contains elements that have an \c anchor property with the type \l XrSpatialAnchor.

    You can use it like this:

    \qml
    Repeater3D {
        model: XrSpatialAnchorListModel {
        }
        delegate: Node {
            required property XrSpatialAnchor anchor
            position: anchor.position
            rotation: anchor.rotation
            // Further use of anchor properties...
        }
    }
    \endqml
*/

QQuick3DXrSpatialAnchorListModel::QQuick3DXrSpatialAnchorListModel(QObject *parent)
    : QAbstractListModel{parent}
{
    m_anchorManager = QQuick3DXrAnchorManager::instance();
    if (m_anchorManager) {
        connect(m_anchorManager, &QQuick3DXrAnchorManager::anchorAdded, this, &QQuick3DXrSpatialAnchorListModel::handleAnchorAdded);
        connect(m_anchorManager, &QQuick3DXrAnchorManager::anchorUpdated, this, &QQuick3DXrSpatialAnchorListModel::handleAnchorUpdated);
        connect(m_anchorManager, &QQuick3DXrAnchorManager::anchorRemoved, this, &QQuick3DXrSpatialAnchorListModel::handleAnchorRemoved);
        queryAnchors();
    } else {
        qWarning("SpatialAnchorModel: Failed to get anchor manager instance");
    }
}

int QQuick3DXrSpatialAnchorListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid() || m_anchorManager == nullptr)
        return 0;

    return m_anchorManager->anchorCount();
}

QVariant QQuick3DXrSpatialAnchorListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || m_anchorManager == nullptr)
        return QVariant();

    const auto &anchors = m_anchorManager->anchors();

    // check bounds
    if (index.row() < 0 || index.row() >= anchors.count())
        return QVariant();

    const auto &anchor = anchors.at(index.row());

    if (role == Anchor)
        return QVariant::fromValue(anchor);

    // shouldn't be reachable under normal circumstances.
    return QVariant();
}

QHash<int, QByteArray> QQuick3DXrSpatialAnchorListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[Anchor] = "anchor";
    return roles;
}

void QQuick3DXrSpatialAnchorListModel::requestSceneCapture()
{
    if (m_anchorManager == nullptr)
        return;

    // not supported on the Simulator, this will be a no-op there
    m_anchorManager->requestSceneCapture();
}

void QQuick3DXrSpatialAnchorListModel::queryAnchors()
{
    if (m_anchorManager == nullptr)
        return;

    m_anchorManager->queryAllAnchors();
}

void QQuick3DXrSpatialAnchorListModel::handleAnchorAdded(QQuick3DXrSpatialAnchor *anchor)
{
    Q_UNUSED(anchor)
    // Brute Force :-p
    beginResetModel();
    endResetModel();
}

void QQuick3DXrSpatialAnchorListModel::handleAnchorRemoved(QUuid uuid)
{
    Q_UNUSED(uuid)
    // Brute Force :-p
    beginResetModel();
    endResetModel();
}

void QQuick3DXrSpatialAnchorListModel::handleAnchorUpdated(QQuick3DXrSpatialAnchor *anchor)
{
    Q_UNUSED(anchor)
    // Brute Force :-p
    beginResetModel();
    endResetModel();
}

// NOTE: filtering is not implemented yet, so the associated properties are left
// undocumented in this version. They are not removed completely since filtering
// will definitely be implemented in a future release.

/*
    \qmlproperty enumeration XrSpatialAnchorListModel::filterMode
    \brief Specifies the filter mode for spatial anchors.
    \internal

    Holds the filter mode.
    The filter mode can be one of the following:
    \value All Show all spatial anchors.
    \value Classification Show spatial anchors based on the provided classification filter flag.
    \value Identifier Show spatial anchors based on matching the provided Identifiers.
 */

QQuick3DXrSpatialAnchorListModel::FilterMode QQuick3DXrSpatialAnchorListModel::filterMode() const
{
    return m_filterMode;
}

void QQuick3DXrSpatialAnchorListModel::setFilterMode(FilterMode newFilterMode)
{
    if (m_filterMode == newFilterMode)
        return;
    m_filterMode = newFilterMode;
    emit filterModeChanged();
}

/*
    \qmlproperty list<string> XrSpatialAnchorListModel::identifierFilter
    \brief Holds the list of identifiers for filtering spatial anchors.
    \internal
 */

QStringList QQuick3DXrSpatialAnchorListModel::identifierFilter() const
{
    return m_uuids;
}

void QQuick3DXrSpatialAnchorListModel::setIdentifierFilter(const QStringList &filter)
{
    if (m_uuids == filter)
        return;
    m_uuids = filter;
    emit identifierFilterChanged();
}

/*
    \qmlproperty enumeration XrSpatialAnchorListModel::classificationFilter
    \brief  Holds the classification flag used for filtering spatial anchors.
    \internal

    The ClassificationFlag filter is represented as a combination of flags:

    \value Wall
    \value Ceiling
    \value Floor
    \value Table
    \value Seat
    \value Window
    \value Door
    \value Other
 */

QQuick3DXrSpatialAnchorListModel::ClassificationFlags QQuick3DXrSpatialAnchorListModel::classificationFilter() const
{
    return m_classFilter;
}

void QQuick3DXrSpatialAnchorListModel::setClassificationFilter(ClassificationFlags newClassFilter)
{
    if (m_classFilter == newClassFilter)
        return;
    m_classFilter = newClassFilter;
    emit classificationFilterChanged();
}

/*
    \qmlproperty list<string> XrSpatialAnchorListModel::classificationStringFilter
    \brief Holds the classification strings used for filtering spatial anchors.
    \internal

    If the \l FilterMode is set to \c Classification, this property can be used to provide a
    list of additional classification string to filter on. These labels will then be matched against
    the same value as reported by \l {XrSpatialAnchor::classificationString} property
    of the spatial anchor.

    \note Only \l {XrSpatialAnchor}{spatial anchors} that are classified as
    \l {XrSpatialAnchor::Classification::Other}{Other}
    will be checked against this filter.
 */
QStringList QQuick3DXrSpatialAnchorListModel::classificationStringFilter() const
{
    return m_classStringFilter.values();
}

void QQuick3DXrSpatialAnchorListModel::setClassificationStringFilter(const QStringList &newClassStringFilter)
{
    QSet<QString> newFilter { newClassStringFilter.cbegin(), newClassStringFilter.cend()};

    if (m_classStringFilter == newFilter)
        return;

    m_classStringFilter = newFilter;
    emit classificationStringFilterChanged();
}

QT_END_NAMESPACE
