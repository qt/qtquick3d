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

class Q_QUICK3DXR_EXPORT QOpenXREyeCamera : public QQuick3DCamera
{
    Q_OBJECT
    Q_PROPERTY(float leftTangent READ leftTangent WRITE setLeftTangent NOTIFY leftTangentChanged)
    Q_PROPERTY(float rightTangent READ rightTangent WRITE setRightTangent NOTIFY rightTangentChanged)
    Q_PROPERTY(float upTangent READ upTangent WRITE setUpTangent NOTIFY upTangentChanged)
    Q_PROPERTY(float downTangent READ downTangent WRITE setDownTangent NOTIFY downTangentChanged)
    Q_PROPERTY(float clipNear READ clipNear WRITE setClipNear NOTIFY clipNearChanged)
    Q_PROPERTY(float clipFar READ clipFar WRITE setClipFar NOTIFY clipFarChanged)

public:
    QOpenXREyeCamera(QQuick3DNode *parent = nullptr);

    float leftTangent() const;
    float rightTangent() const;
    float upTangent() const;
    float downTangent() const;
    float clipNear() const;
    float clipFar() const;

public Q_SLOTS:
    void setLeftTangent(float leftTangent);
    void setRightTangent(float rightTangent);
    void setUpTangent(float upTangent);
    void setDownTangent(float downTangent);
    void setClipNear(float clipNear);
    void setClipFar(float clipFar);

Q_SIGNALS:
    void leftTangentChanged(float leftTangent);
    void rightTangentChanged(float rightTangent);
    void upTangentChanged(float upTangent);
    void downTangentChanged(float downTangent);
    void clipNearChanged(float clipNear);
    void clipFarChanged(float clipFar);

public:
    QMatrix4x4 m_projection;

protected:
    QSSGRenderGraphObject *updateSpatialNode(QSSGRenderGraphObject *node) override;

private:
    void markProjectionDirty();
    void maybeUpdateProjection();
    float m_leftTangent = -0.017455064928218f;  // tan(-1)
    float m_rightTangent = 0.017455064928218f;  // tan(1)
    float m_upTangent = 0.017455064928218f;     // tan(1)
    float m_downTangent = -0.017455064928218f;  // tan(-1)
    float m_clipNear = 1.0f;
    float m_clipFar = 10000.0f;
    bool m_projectionDirty = true;
};

class Q_QUICK3DXR_EXPORT QOpenXRCamera : public QQuick3DNode
{
    Q_OBJECT
    Q_PROPERTY(float clipNear READ clipNear WRITE setClipNear NOTIFY clipNearChanged)
    Q_PROPERTY(float clipFar READ clipFar WRITE setClipFar NOTIFY clipFarChanged)
    QML_NAMED_ELEMENT(XrCamera)

public:
    QOpenXRCamera(QQuick3DNode *parent = nullptr);
    ~QOpenXRCamera();
    float clipNear() const;
    float clipFar() const;

public Q_SLOTS:
    void setClipNear(float clipNear);
    void setClipFar(float clipFar);

Q_SIGNALS:
    void clipNearChanged(float clipNear);
    void clipFarChanged(float clipFar);

private:
    float m_clipNear = 1.0f;
    float m_clipFar = 10000.0f;
};


QT_END_NAMESPACE

#endif // QOPENXRCAMERA_H
