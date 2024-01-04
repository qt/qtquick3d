// Copyright (C) 2023 The Qt Company Ltd.
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

bool QOpenXRHandInput::button1Pressed() const
{
    return m_button1Pressed;
}

bool QOpenXRHandInput::button1Touched() const
{
    return m_button1Touched;
}

bool QOpenXRHandInput::button2Pressed() const
{
    return m_button2Pressed;
}

bool QOpenXRHandInput::button2Touched() const
{
    return m_button2Touched;
}

bool QOpenXRHandInput::buttonMenuPressed() const
{
    return m_buttonMenuPressed;
}

bool QOpenXRHandInput::buttonMenuTouched() const
{
    return m_buttonMenuTouched;
}

bool QOpenXRHandInput::buttonSystemPressed() const
{
    return m_buttonSystemPressed;
}

bool QOpenXRHandInput::buttonSystemTouched() const
{
    return m_buttonSystemTouched;
}

float QOpenXRHandInput::squeezeValue() const
{
    return m_squeezeValue;
}

float QOpenXRHandInput::squeezeForce() const
{
    return m_squeezeForce;
}

bool QOpenXRHandInput::squeezePressed() const
{
    return m_squeezePressed;
}

float QOpenXRHandInput::triggerValue() const
{
    return m_triggerValue;
}

bool QOpenXRHandInput::triggerPressed() const
{
    return m_triggerPressed;
}

bool QOpenXRHandInput::triggerTouched() const
{
    return m_triggerTouched;
}

QVector2D QOpenXRHandInput::thumbstickValues() const
{
    return m_thumbstickValues;
}

bool QOpenXRHandInput::thumbstickPressed() const
{
    return m_thumbstickPressed;
}

bool QOpenXRHandInput::thumbstickTouched() const
{
    return m_thumbstickTouched;
}

bool QOpenXRHandInput::thumbrestTouched() const
{
    return m_thumbrestTouched;
}

QVector2D QOpenXRHandInput::trackpadValues() const
{
    return m_trackpadValues;
}

float QOpenXRHandInput::trackpadForce() const
{
    return m_trackpadForce;
}

bool QOpenXRHandInput::trackpadTouched() const
{
    return m_trackpadTouched;
}

bool QOpenXRHandInput::trackpadPressed() const
{
    return m_trackpadPressed;
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

void QOpenXRHandInput::setButton1Pressed(bool button1Pressed)
{
    if (m_button1Pressed == button1Pressed)
        return;

    m_button1Pressed = button1Pressed;
    emit button1PressedChanged();
}

void QOpenXRHandInput::setButton1Touched(bool button1Touched)
{
    if (m_button1Touched == button1Touched)
        return;

    m_button1Touched = button1Touched;
    emit button1TouchedChanged();
}

void QOpenXRHandInput::setButton2Pressed(bool button2Pressed)
{
    if (m_button2Pressed == button2Pressed)
        return;

    m_button2Pressed = button2Pressed;
    emit button2PressedChanged();
}

void QOpenXRHandInput::setButton2Touched(bool button2Touched)
{
    if (m_button2Touched == button2Touched)
        return;

    m_button2Touched = button2Touched;
    emit button2TouchedChanged();
}

void QOpenXRHandInput::setButtonMenuPressed(bool buttonMenuPressed)
{
    if (m_buttonMenuPressed == buttonMenuPressed)
        return;

    m_buttonMenuPressed = buttonMenuPressed;
    emit buttonMenuPressedChanged();
}

void QOpenXRHandInput::setButtonMenuTouched(bool buttonMenuTouched)
{
    if (m_buttonMenuTouched == buttonMenuTouched)
        return;

    m_buttonMenuTouched = buttonMenuTouched;
    emit buttonMenuTouchedChanged();
}

void QOpenXRHandInput::setButtonSystemPressed(bool buttonSystemPressed)
{
    if (m_buttonSystemPressed == buttonSystemPressed)
        return;

    m_buttonSystemPressed = buttonSystemPressed;
    emit buttonSystemPressedChanged();
}

void QOpenXRHandInput::setButtonSystemTouched(bool buttonSystemTouched)
{
    if (m_buttonSystemTouched == buttonSystemTouched)
        return;

    m_buttonSystemTouched = buttonSystemTouched;
    emit buttonSystemTouchedChanged();
}

void QOpenXRHandInput::setSqueezeValue(float squeezeValue)
{
    if (qFuzzyCompare(m_squeezeValue, squeezeValue))
        return;

    m_squeezeValue = squeezeValue;
    emit squeezeValueChanged();
}

void QOpenXRHandInput::setSqueezeForce(float squeezeForce)
{
    if (qFuzzyCompare(m_squeezeForce, squeezeForce))
        return;

    m_squeezeForce = squeezeForce;
    emit squeezeForceChanged();
}

void QOpenXRHandInput::setSqueezePressed(bool squeezePressed)
{
    if (m_squeezePressed == squeezePressed)
        return;

    m_squeezePressed = squeezePressed;
    emit squeezePressedChanged();
}

void QOpenXRHandInput::setTriggerValue(float triggerValue)
{
    if (qFuzzyCompare(m_triggerValue, triggerValue))
        return;

    m_triggerValue = triggerValue;
    emit triggerValueChanged();
}

void QOpenXRHandInput::setTriggerPressed(bool triggerPressed)
{
    if (m_triggerPressed == triggerPressed)
        return;

    m_triggerPressed = triggerPressed;
    emit triggerPressedChanged();
}

void QOpenXRHandInput::setTriggerTouched(bool triggerTouched)
{
    if (m_triggerTouched == triggerTouched)
        return;

    m_triggerTouched = triggerTouched;
    emit triggerTouchedChanged();
}

void QOpenXRHandInput::setThumbstickX(float thumbstickX)
{
    setThumbstickValues(QVector2D(thumbstickX, m_thumbstickValues.y()));
}

void QOpenXRHandInput::setThumbstickY(float thumbstickY)
{
    setThumbstickValues(QVector2D(m_thumbstickValues.x(), thumbstickY));
}

void QOpenXRHandInput::setThumbstickValues(const QVector2D &values)
{
    if (m_thumbstickValues == values)
        return;
    m_thumbstickValues = values;
    emit thumbstickValuesChanged();
}

void QOpenXRHandInput::setThumbstickPressed(bool thumbstickPressed)
{
    if (m_thumbstickPressed == thumbstickPressed)
        return;

    m_thumbstickPressed = thumbstickPressed;
    emit thumbstickPressedChanged();
}

void QOpenXRHandInput::setThumbstickTouched(bool thumbstickTouched)
{
    if (m_thumbstickTouched == thumbstickTouched)
        return;

    m_thumbstickTouched = thumbstickTouched;
    emit thumbstickTouchedChanged();
}

void QOpenXRHandInput::setThumbrestTouched(bool thumbrestTouched)
{
    if (m_thumbrestTouched == thumbrestTouched)
        return;

    m_thumbrestTouched = thumbrestTouched;
    emit thumbrestTouchedChanged();
}

void QOpenXRHandInput::setTrackpadX(float trackpadX)
{
    setTrackpadValues(QVector2D(trackpadX, m_trackpadValues.y()));
}

void QOpenXRHandInput::setTrackpadY(float trackpadY)
{
    setTrackpadValues(QVector2D(m_trackpadValues.x(), trackpadY));
}

void QOpenXRHandInput::setTrackpadValues(const QVector2D &values)
{
    if (m_trackpadValues == values)
        return;
    m_trackpadValues = values;
    emit trackpadValuesChanged();
}

void QOpenXRHandInput::setTrackpadForce(float trackpadForce)
{
    if (qFuzzyCompare(m_trackpadForce, trackpadForce))
        return;

    m_trackpadForce = trackpadForce;
    emit trackpadForceChanged();
}

void QOpenXRHandInput::setTrackpadTouched(bool trackpadTouched)
{
    if (m_trackpadTouched == trackpadTouched)
        return;

    m_trackpadTouched = trackpadTouched;
    emit trackpadTouchedChanged();
}

void QOpenXRHandInput::setTrackpadPressed(bool trackpadPressed)
{
    if (m_trackpadPressed == trackpadPressed)
        return;

    m_trackpadPressed = trackpadPressed;
    emit trackpadPressedChanged();
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
