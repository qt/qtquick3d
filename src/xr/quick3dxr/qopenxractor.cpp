// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qopenxractor_p.h"

QT_BEGIN_NAMESPACE

QOpenXRActor::QOpenXRActor()
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

QOpenXRCamera *QOpenXRActor::camera(int index) const
{
    return m_cameras[index];
}

float QOpenXRActor::clipNear() const
{
    return m_clipNear;
}

void QOpenXRActor::setClipNear(float newClipNear)
{
    if (qFuzzyCompare(m_clipNear, newClipNear))
        return;
    m_clipNear = newClipNear;
    for (QOpenXRCamera *camera : std::as_const(m_cameras))
        camera->setClipNear(newClipNear);
    emit clipNearChanged();
}

float QOpenXRActor::clipFar() const
{
    return m_clipFar;
}

void QOpenXRActor::setClipFar(float newClipFar)
{
    if (qFuzzyCompare(m_clipFar, newClipFar))
        return;
    m_clipFar = newClipFar;
    for (QOpenXRCamera *camera : std::as_const(m_cameras))
        camera->setClipFar(newClipFar);
    emit clipFarChanged();
}

QT_END_NAMESPACE
