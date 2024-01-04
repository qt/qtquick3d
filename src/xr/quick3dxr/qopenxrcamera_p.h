// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QOPENXRCAMERA_H
#define QOPENXRCAMERA_H

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

#include <QObject>
#include <QtQuick3D/private/qquick3dcamera_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICK3DXR_EXPORT QOpenXRCamera : public QQuick3DCamera
{
    Q_OBJECT
    Q_PROPERTY(float angleLeft READ angleLeft WRITE setAngleLeft NOTIFY angleLeftChanged)
    Q_PROPERTY(float angleRight READ angleRight WRITE setAngleRight NOTIFY angleRightChanged)
    Q_PROPERTY(float angleUp READ angleUp WRITE setAngleUp NOTIFY angleUpChanged)
    Q_PROPERTY(float angleDown READ angleDown WRITE setAngleDown NOTIFY angleDownChanged)
    Q_PROPERTY(float clipNear READ clipNear WRITE setClipNear NOTIFY clipNearChanged)
    Q_PROPERTY(float clipFar READ clipFar WRITE setClipFar NOTIFY clipFarChanged)

public:
    QOpenXRCamera(QQuick3DNode *parent = nullptr);

    float angleLeft() const;
    float angleRight() const;
    float angleUp() const;
    float angleDown() const;
    float clipNear() const;
    float clipFar() const;

public Q_SLOTS:
    void setAngleLeft(float angleLeft);
    void setAngleRight(float angleRight);
    void setAngleUp(float angleUp);
    void setAngleDown(float angleDown);
    void setClipNear(float clipNear);
    void setClipFar(float clipFar);

Q_SIGNALS:
    void angleLeftChanged(float angleLeft);
    void angleRightChanged(float angleRight);
    void angleUpChanged(float angleUp);
    void angleDownChanged(float angleDown);
    void clipNearChanged(float clipNear);
    void clipFarChanged(float clipFar);

protected:
    QSSGRenderGraphObject *updateSpatialNode(QSSGRenderGraphObject *node) override;

private:
    void markProjectionDirty();
    void maybeUpdateProjection();
    float m_angleLeft = -1.0f;
    float m_angleRight = 1.0f;
    float m_angleUp = 1.0f;
    float m_angleDown = -1.0f;
    float m_clipNear = 1.0f;
    float m_clipFar = 10000.0f;
    QMatrix4x4 m_projection;
    bool m_projectionDirty = true;
};

QT_END_NAMESPACE

#endif // QOPENXRCAMERA_H
