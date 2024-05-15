// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qopenxrorigin_p.h"

QT_BEGIN_NAMESPACE

QOpenXROrigin::QOpenXROrigin()
    : m_builtInCamera(new QQuick3DXrCamera(this))
{
    // These are the "real" cameras that are used for rendering.
    auto *leftEyeCamera = new QQuick3DXrEyeCamera;
    leftEyeCamera->setParent(this);
    leftEyeCamera->setParentItem(this);
    m_eyeCameras.append(leftEyeCamera);

    auto *rightEyeCamera = new QQuick3DXrEyeCamera;
    rightEyeCamera->setParent(this);
    rightEyeCamera->setParentItem(this);
    m_eyeCameras.append(rightEyeCamera);

    // This is the user facing camera
    setCamera(m_builtInCamera);
}


QQuick3DXrCamera *QOpenXROrigin::camera() const
{
    return m_camera;
}

void QOpenXROrigin::setCamera(QQuick3DXrCamera *newCamera)
{
    if (m_camera == newCamera)
        return;

    if (m_camera) {
        // connect the near/far properties to the real eye camers
        for (auto eyeCamera : m_eyeCameras) {
            // disconnnect the old camera
            disconnect(m_camera, &QQuick3DXrCamera::clipNearChanged, eyeCamera, &QQuick3DXrEyeCamera::setClipNear);
            disconnect(m_camera, &QQuick3DXrCamera::clipFarChanged, eyeCamera, &QQuick3DXrEyeCamera::setClipFar);
        }
    }

    // There will always be a camera, either the built-in one or the user provided one
    if (newCamera)
        m_camera = newCamera;
    else
        m_camera = m_builtInCamera;

    if (m_camera) {
        for (auto eyeCamera : m_eyeCameras) {
            // Set the initial value, and connect the signals
            eyeCamera->setClipNear(m_camera->clipNear());
            eyeCamera->setClipFar(m_camera->clipFar());
            connect(m_camera, &QQuick3DXrCamera::clipNearChanged, eyeCamera, &QQuick3DXrEyeCamera::setClipNear);
            connect(m_camera, &QQuick3DXrCamera::clipFarChanged, eyeCamera, &QQuick3DXrEyeCamera::setClipFar);
        }
    }
    emit cameraChanged();
}

QQuick3DXrEyeCamera *QOpenXROrigin::eyeCamera(int index) const
{
    return m_eyeCameras[index];
}

QT_END_NAMESPACE

