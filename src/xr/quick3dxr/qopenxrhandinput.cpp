// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qopenxrhandinput_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype XrHandInput
    \inherits Item
    \inqmlmodule QtQuick3D.Xr
    \brief Represents a hand input.

    This type provides information about hand poses, position, and rotation.
 */

QOpenXRHandInput::QOpenXRHandInput(QObject *parent)
    : QObject(parent)
{

}

/*!
    \qmlproperty bool XrHandInput::isActive
    Indicates whether the hand input is active.
*/

bool QOpenXRHandInput::isActive() const
{
    return m_isActive;
}

/*!
    \qmlproperty enumeration XrHandInput::poseSpace
     Specifies the hand pose space.
     Can be one of the following:

     \value XrHandInput.GripPose
     \value XrHandINput.AimPose
*/

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

/*!
    \qmlproperty vector3d XrHandInput::posePosition
    The position of the hand pose in 3D space.
*/

const QVector3D &QOpenXRHandInput::posePosition() const
{
    return m_posePosition;
}

/*!
    \qmlproperty Quaternion XrHandInput::poseRotation
    The rotation of the hand pose.
 */

const QQuaternion &QOpenXRHandInput::poseRotation() const
{
    return m_poseRotation;
}

/*!
    \qmlsignal XrHandInput::isActiveChanged()
    Emitted when the isActive property changes.
*/

/*!
    \qmlsignal XrHandInput::poseSpaceChanged()
    Emitted when the poseSpace property changes.
*/

/*!
    \qmlsignal XrHandInput::posePositionChanged()
    Emitted when the posePosition property changes.
*/

/*!
    \qmlsignal XrHandInput::poseRotationChanged()
    Emitted when the poseRotation property changes.
*/

QT_END_NAMESPACE
