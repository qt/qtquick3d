// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qopenxrhandtrackerinput_p.h"

QT_BEGIN_NAMESPACE

QOpenXRHandTrackerInput::QOpenXRHandTrackerInput(QObject *parent)
    : QObject(parent)
{

}

bool QOpenXRHandTrackerInput::isActive() const
{
    return m_isActive;
}

QOpenXRHandTrackerInput::HandPoseSpace QOpenXRHandTrackerInput::poseSpace() const
{
    return m_poseSpace;
}

void QOpenXRHandTrackerInput::setIsActive(bool isActive)
{
    if (m_isActive == isActive)
        return;

    m_isActive = isActive;
    emit isActiveChanged();
}

void QOpenXRHandTrackerInput::setPoseSpace(QOpenXRHandTrackerInput::HandPoseSpace poseSpace)
{
    if (poseSpace == m_poseSpace)
        return;

    m_poseSpace = poseSpace;
    emit poseSpaceChanged();
}

const QVector3D &QOpenXRHandTrackerInput::posePosition() const
{
    return m_posePosition;
}

const QQuaternion &QOpenXRHandTrackerInput::poseRotation() const
{
    return m_poseRotation;
}

QQuick3DGeometry *QOpenXRHandTrackerInput::handGeometry() const
{
    return m_handGeometry;
}

void QOpenXRHandTrackerInput::setHandGeometry(QQuick3DGeometry *newHandGeometry)
{
    if (m_handGeometry == newHandGeometry)
        return;
    m_handGeometry = newHandGeometry;
    emit handGeometryChanged();
}

QT_END_NAMESPACE
