// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QOPENXRSPATIALANCHORMODEL_H
#define QOPENXRSPATIALANCHORMODEL_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QAbstractListModel>
#include <QQmlEngine>
#include <QUuid>

QT_BEGIN_NAMESPACE

class QOpenXRSpaceExtension;
class QOpenXRSpatialAnchor;

class QOpenXRSpatialAnchorModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(FilterMode filterMode READ filterMode WRITE setFilterMode NOTIFY filterModeChanged FINAL)
    Q_PROPERTY(SemanticLabels labels READ labels WRITE setLabels NOTIFY labelsChanged FINAL)
    Q_PROPERTY(QList<QUuid> uuids READ uuids WRITE setUuids NOTIFY uuidsChanged FINAL)
    QML_NAMED_ELEMENT(XrSpatialAnchorModel)
    QML_ADDED_IN_VERSION(6, 8)
public:
    enum FilterMode {
        All,
        Labels,
        UUID
    };
    Q_ENUM(FilterMode)

    enum SemanticLabel {
        Ceiling = 0x01,
        DoorFrame = 0x02,
        Floor = 0x04,
        WallArt = 0x08,
        WallFace = 0x10,
        WindowFrame = 0x20,
        Couch = 0x40,
        Table = 0x80,
        Bed = 0x100,
        Lamp = 0x200,
        Plant = 0x400,
        Screen = 0x800,
        Storage = 0x1000,
        Other = 0x2000
    };
    Q_ENUM(SemanticLabel)
    Q_DECLARE_FLAGS(SemanticLabels , SemanticLabel)
    Q_FLAG(SemanticLabels)

    enum SpatialAnchorRoles {
        Anchor = Qt::UserRole + 21
    };

    explicit QOpenXRSpatialAnchorModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void requestSceneCapture();
    Q_INVOKABLE void queryAnchors();

    FilterMode filterMode() const;
    void setFilterMode(FilterMode newFilterMode);

    QList<QUuid> uuids() const;
    void setUuids(const QList<QUuid> &newUuids);

    SemanticLabels labels() const;
    void setLabels(const SemanticLabels &newLabels);

signals:
    void filterModeChanged();
    void uuidsChanged();
    void labelsChanged();

private Q_SLOTS:
    void handleAnchorAdded(QOpenXRSpatialAnchor* anchor);

private:
    QOpenXRSpaceExtension *m_spaceExtension = nullptr;
    FilterMode m_filterMode = FilterMode::All;
    QList<QUuid> m_uuids;
    SemanticLabels m_labels;
};

QT_END_NAMESPACE

#endif // QOPENXRSPATIALANCHORMODEL_H
