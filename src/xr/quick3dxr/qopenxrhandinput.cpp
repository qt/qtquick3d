// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qopenxrhandinput_p.h"

QT_BEGIN_NAMESPACE

QOpenXRHandInput::QOpenXRHandInput(QObject *parent)
    : QObject(parent)
{

}

bool QOpenXRHandInput::isActive() const
{
    return m_isActive;
}

QOpenXRHandInput::HandPoseSpace QOpenXRHandInput::poseSpace() const
{
    return m_poseSpace;
}

void QOpenXRHandInput::setIsActive(bool isActive)
{
    if (m_isActive == isActive)
        return;

    m_isActive = isActive;
    emit isActiveChanged();
}

void QOpenXRHandInput::setPosePosition(const QVector3D &position)
{
    if (m_posePosition == position)
        return;

    m_posePosition = position;
    emit posePositionChanged();
}

void QOpenXRHandInput::setPoseRotation(const QQuaternion &rotation)
{
    if (m_poseRotation == rotation)
        return;

    m_poseRotation = rotation;
    emit poseRotationChanged();
}

void QOpenXRHandInput::setPoseSpace(QOpenXRHandInput::HandPoseSpace poseSpace)
{
    if (poseSpace == m_poseSpace)
        return;

    m_poseSpace = poseSpace;
    emit poseSpaceChanged();
}

const QVector3D &QOpenXRHandInput::posePosition() const
{
    return m_posePosition;
}

const QQuaternion &QOpenXRHandInput::poseRotation() const
{
    return m_poseRotation;
}

QT_END_NAMESPACE
