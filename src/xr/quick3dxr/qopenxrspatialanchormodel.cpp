// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qopenxrspatialanchormodel_p.h"
#include "qopenxrspatialanchor_p.h"
#include "qopenxrspaceextension_p.h"

QT_BEGIN_NAMESPACE

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

QT_END_NAMESPACE
