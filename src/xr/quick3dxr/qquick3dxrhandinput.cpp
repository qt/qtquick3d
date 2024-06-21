// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "openxr/qopenxrinputmanager_p.h"
#include "qquick3dxrhandinput_p.h"
#include "qquick3dxrinputmanager_p.h"

QT_BEGIN_NAMESPACE

QQuick3DXrHandInput::QQuick3DXrHandInput(QObject *parent)
    : QObject(parent)
{

}

bool QQuick3DXrHandInput::isActive() const
{
    return m_isActive;
}

QQuick3DXrHandInput::HandPoseSpace QQuick3DXrHandInput::poseSpace() const
{
    return m_poseSpace;
}

void QQuick3DXrHandInput::setIsActive(bool isActive)
{
    if (m_isActive == isActive)
        return;

    m_isActive = isActive;
    emit isActiveChanged();
}

void QQuick3DXrHandInput::setPosePosition(const QVector3D &position)
{
    if (m_posePosition == position)
        return;

    m_posePosition = position;
    emit posePositionChanged();
}

void QQuick3DXrHandInput::setPoseRotation(const QQuaternion &rotation)
{
    if (m_poseRotation == rotation)
        return;

    m_poseRotation = rotation;
    emit poseRotationChanged();
}

void QQuick3DXrHandInput::setPoseSpace(QQuick3DXrHandInput::HandPoseSpace poseSpace)
{
    if (poseSpace == m_poseSpace)
        return;

    m_poseSpace = poseSpace;
    emit poseSpaceChanged();
}

const QVector3D &QQuick3DXrHandInput::posePosition() const
{
    return m_posePosition;
}

const QQuaternion &QQuick3DXrHandInput::poseRotation() const
{
    return m_poseRotation;
}

void QQuick3DXrHandInput::setJointPositionsAndRotations(const QList<QVector3D> &newJointPositions, const QList<QQuaternion> &newJointRotations)
{
    m_jointPositions = newJointPositions;
    emit jointPositionsChanged();
    m_jointRotations = newJointRotations;
    emit jointRotationsChanged();
    emit jointDataUpdated();

    QQuick3DXrInputManager *inputMan = QQuick3DXrInputManager::instance();
    const auto pokeIndex = QQuick3DXrInputManagerPrivate::get(inputMan)->getPokeJointIndex();

    if (pokeIndex >= 0 && pokeIndex < m_jointPositions.size())
        setPokePosition(m_jointPositions[pokeIndex]);
}

QList<QVector3D> QQuick3DXrHandInput::jointPositions() const
{
    return m_jointPositions;
}

QList<QQuaternion> QQuick3DXrHandInput::jointRotations() const
{
    return m_jointRotations;
}

QVector3D QQuick3DXrHandInput::pokePosition() const
{
    return m_pokePosition;
}

void QQuick3DXrHandInput::setPokePosition(const QVector3D &newPokePosition)
{
    if (m_pokePosition == newPokePosition)
        return;
    m_pokePosition = newPokePosition;
    emit pokePositionChanged();
}

bool QQuick3DXrHandInput::isHandTrackingActive() const
{
    return m_isHandTracking;
}

void QQuick3DXrHandInput::setIsHandTrackingActive(bool newIsHandTracking)
{
    if (m_isHandTracking == newIsHandTracking)
        return;
    m_isHandTracking = newIsHandTracking;
    emit isHandTrackingChanged();
}

QT_END_NAMESPACE
