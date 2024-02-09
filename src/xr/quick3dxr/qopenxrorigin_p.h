// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QOPENXRORIGIN_H
#define QOPENXRORIGIN_H

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
#include <QtQuick3DXr/private/qopenxrcamera_p.h>
#include <QtQml/QQmlEngine>
#include <QtQml/qqml.h>

QT_BEGIN_NAMESPACE

class Q_QUICK3DXR_EXPORT QOpenXROrigin : public QQuick3DNode
{
    Q_OBJECT
    Q_PROPERTY(QOpenXRCamera *camera READ camera WRITE setCamera NOTIFY cameraChanged)
    QML_NAMED_ELEMENT(XrOrigin)

public:
    QOpenXROrigin();

    QOpenXRCamera *camera() const;
    void setCamera(QOpenXRCamera *newCamera);

Q_SIGNALS:
    void cameraChanged();

private:
    QOpenXREyeCamera *eyeCamera(int index) const;
    QOpenXRCamera *m_camera = nullptr;
    QOpenXRCamera *m_builtInCamera = nullptr;
    QVarLengthArray<QOpenXREyeCamera *, 2> m_eyeCameras;

    friend class QOpenXRManager;
};

QT_END_NAMESPACE

#endif // QOPENXRORIGIN_H
