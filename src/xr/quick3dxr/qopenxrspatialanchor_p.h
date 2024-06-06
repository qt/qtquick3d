// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QQUICK3DXRSPATIALANCHOR_P_H
#define QQUICK3DXRSPATIALANCHOR_P_H

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

#include <QtQuick3DXr/qtquick3dxrglobal.h>
#include <QtQuick3DXr/private/qtquick3dxrglobal_p.h>

#include <QtCore/qobject.h>
#include <QtCore/quuid.h>
#include <QtCore/qstring.h>

#include <QtGui/qvectornd.h>
#include <QtGui/qquaternion.h>

#include <QtQmlIntegration/qqmlintegration.h>

QT_BEGIN_NAMESPACE

class QQuick3DXrSpatialAnchor : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool has2DBounds READ has2DBounds CONSTANT)
    Q_PROPERTY(bool has3DBounds READ has3DBounds CONSTANT)
    Q_PROPERTY(QVector2D offset2D READ offset2D CONSTANT)
    Q_PROPERTY(QVector2D extent2D READ extent2D CONSTANT)
    Q_PROPERTY(QVector3D offset3D READ offset3D CONSTANT)
    Q_PROPERTY(QVector3D extent3D READ extent3D CONSTANT)
    Q_PROPERTY(QVector3D position READ position NOTIFY positionChanged)
    Q_PROPERTY(QQuaternion rotation READ rotation NOTIFY rotationChanged)
    Q_PROPERTY(QString semanticLabels READ semanticLabels CONSTANT)
    Q_PROPERTY(QUuid uuid READ uuid CONSTANT)
    QML_NAMED_ELEMENT(XrSpatialAnchor)
    QML_UNCREATABLE("Spatial anchor objects cannot be created in QML");
    QML_ADDED_IN_VERSION(6, 8)
public:
    QQuick3DXrSpatialAnchor(QtQuick3DXr::XrSpaceId space, QUuid &uuid, QObject *parent = nullptr);
    ~QQuick3DXrSpatialAnchor() override;

    QVector3D offset3D() const;
    void setOffset3D(const QVector3D &newOffset);

    QVector3D extent3D() const;
    void setExtent3D(const QVector3D &newExtent);

    QVector3D position() const;
    void setPosition(const QVector3D &newPosition);

    QQuaternion rotation() const;
    void setRotation(const QQuaternion &newRotation);

    QString semanticLabels() const;
    void setSemanticLabels(const QString &newSemanticLabels);

    bool has2DBounds() const;
    void setBounds2D(const QVector2D &offset, const QVector2D &extent);
    bool has3DBounds() const;
    void setBounds3D(const QVector3D &offset, const QVector3D &extent);

    QVector2D offset2D() const;
    QVector2D extent2D() const;

    QUuid uuid() const;

    QSet<QUuid> roomLayoutUuids() const;
    void setRoomLayoutUuids(const QSet<QUuid> &newRoomLayoutUuids);

    QSet<QUuid> spaceContainerUuids() const;
    void setSpaceContainerUuids(const QSet<QUuid> &newSpaceContainerUuids);

    QtQuick3DXr::XrSpaceId space() const { return m_space; }

signals:
    void offset3DChanged();
    void extent3DChanged();
    void positionChanged();
    void rotationChanged();
    void semanticLabelsChanged();
    void has2DBoundsChanged();
    void has3DBoundsChanged();
    void offset2DChanged();
    void extent2DChanged();

private:
    QtQuick3DXr::XrSpaceId m_space { };
    QUuid m_uuid;
    QVector3D m_offset3D;
    QVector3D m_extent3D;
    QVector3D m_position;
    QQuaternion m_rotation;
    QString m_semanticLabels;
    QSet<QUuid> m_roomLayoutUuids;
    QSet<QUuid> m_spaceContainerUuids;
    bool m_has2DBounds = false;
    bool m_has3DBounds = false;
    QVector2D m_offset2D;
    QVector2D m_extent2D;
};

QT_END_NAMESPACE

#endif // QQUICK3DXRSPATIALANCHOR_P_H
