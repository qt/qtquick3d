// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QQUICK3DXRANCHORMANAGER_VISIONOS_P_H
#define QQUICK3DXRANCHORMANAGER_VISIONOS_P_H

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

#include <QtQuick3DXr/private/qtquick3dxrglobal_p.h>

class QVector2D;
class QVector3D;


QT_BEGIN_NAMESPACE

class QQuick3DXrSpatialAnchor;

class QQuick3DXrAnchorManager : public QObject
{
    Q_OBJECT
public:
    static QQuick3DXrAnchorManager* instance();

    void requestSceneCapture();
    bool queryAllAnchors();
    const QList<QQuick3DXrSpatialAnchor *> &anchors() const;

    bool getBoundingBox3D(QtQuick3DXr::XrSpaceId space, QVector3D &offset, QVector3D &extent);
    bool getBoundingBox2D(QtQuick3DXr::XrSpaceId space, QVector2D &offset, QVector2D &extent);

    bool setupSpatialAnchor(QtQuick3DXr::XrSpaceId space, QQuick3DXrSpatialAnchor &anchor);

Q_SIGNALS:
    void anchorAdded(QQuick3DXrSpatialAnchor *anchor);
    void sceneCaptureCompleted();

private:
    explicit QQuick3DXrAnchorManager(QObject *parent = nullptr);
    ~QQuick3DXrAnchorManager() override;
};


QT_END_NAMESPACE

#endif // QQUICK3DXRANCHORMANAGER_VISIONOS_P_H
