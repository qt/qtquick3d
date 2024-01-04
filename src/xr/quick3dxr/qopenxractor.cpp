// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qopenxractor_p.h"

QT_BEGIN_NAMESPACE

QOpenXRActor::QOpenXRActor()
{
    m_leftEyeCamera = new QOpenXRCamera();
    m_leftEyeCamera->setParentItem(this);
    m_leftEyeCamera->setParent(this);

    m_rightEyeCamera = new QOpenXRCamera();
    m_rightEyeCamera->setParentItem(this);
    m_rightEyeCamera->setParent(this);
    emit rightCameraChanged();
}

QOpenXRCamera *QOpenXRActor::leftCamera() const
{
    return m_leftEyeCamera;
}

QOpenXRCamera *QOpenXRActor::rightCamera() const
{
    return m_rightEyeCamera;
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
    m_leftEyeCamera->setClipNear(newClipNear);
    m_rightEyeCamera->setClipNear(newClipNear);
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
    m_leftEyeCamera->setClipFar(newClipFar);
    m_rightEyeCamera->setClipFar(newClipFar);
    emit clipFarChanged();
}

QT_END_NAMESPACE
