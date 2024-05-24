// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QQUICK3DXRORIGIN_H
#define QQUICK3DXRORIGIN_H

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

#include <QtQuick3D/private/qquick3dnode_p.h>
#include <QtQuick3DXr/private/qquick3dxrcamera_p.h>
#include <QtQml/QQmlEngine>
#include <QtQml/qqml.h>

QT_BEGIN_NAMESPACE

class Q_QUICK3DXR_EXPORT QQuick3DXrOrigin : public QQuick3DNode
{
    Q_OBJECT
    Q_PROPERTY(QQuick3DXrCamera *camera READ camera WRITE setCamera NOTIFY cameraChanged)
    QML_NAMED_ELEMENT(XrOrigin)

public:
    QQuick3DXrOrigin();

    QQuick3DXrCamera *camera() const;
    void setCamera(QQuick3DXrCamera *newCamera);

Q_SIGNALS:
    void cameraChanged();

private:
    QQuick3DXrEyeCamera *eyeCamera(int index) const;
    QQuick3DXrCamera *m_camera = nullptr;
    QQuick3DXrCamera *m_builtInCamera = nullptr;
    QVarLengthArray<QQuick3DXrEyeCamera *, 2> m_eyeCameras;

    friend class QQuick3DXrManager;
    friend class QQuick3DXrManagerPrivate;
};

QT_END_NAMESPACE

#endif // QQUICK3DXRORIGIN_H
