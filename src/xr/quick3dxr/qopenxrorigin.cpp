// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qopenxrorigin_p.h"

QT_BEGIN_NAMESPACE

QOpenXROrigin::QOpenXROrigin()
{
    QOpenXRCamera *leftEyeCamera = new QOpenXRCamera;
    leftEyeCamera->setParentItem(this);
    leftEyeCamera->setParent(this);
    m_cameras.append(leftEyeCamera);

    QOpenXRCamera *rightEyeCamera = new QOpenXRCamera;
    rightEyeCamera->setParentItem(this);
    rightEyeCamera->setParent(this);
    m_cameras.append(rightEyeCamera);
}

QOpenXRCamera *QOpenXROrigin::camera(int index) const
{
    return m_cameras[index];
}

float QOpenXROrigin::clipNear() const
{
    return m_clipNear;
}

void QOpenXROrigin::setClipNear(float newClipNear)
{
    if (qFuzzyCompare(m_clipNear, newClipNear))
        return;
    m_clipNear = newClipNear;
    for (QOpenXRCamera *camera : std::as_const(m_cameras))
        camera->setClipNear(newClipNear);
    emit clipNearChanged();
}

float QOpenXROrigin::clipFar() const
{
    return m_clipFar;
}

void QOpenXROrigin::setClipFar(float newClipFar)
{
    if (qFuzzyCompare(m_clipFar, newClipFar))
        return;
    m_clipFar = newClipFar;
    for (QOpenXRCamera *camera : std::as_const(m_cameras))
        camera->setClipFar(newClipFar);
    emit clipFarChanged();
}

QT_END_NAMESPACE

