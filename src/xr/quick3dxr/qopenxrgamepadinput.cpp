// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qopenxrgamepadinput_p.h"

QT_BEGIN_NAMESPACE

QOpenXRGamepadInput::QOpenXRGamepadInput(QObject *parent) : QObject(parent)
{

}

bool QOpenXRGamepadInput::buttonMenu() const
{
    return m_buttonMenu;
}

bool QOpenXRGamepadInput::buttonView() const
{
    return m_buttonView;
}

bool QOpenXRGamepadInput::buttonA() const
{
    return m_buttonA;
}

bool QOpenXRGamepadInput::buttonB() const
{
    return m_buttonB;
}

bool QOpenXRGamepadInput::buttonX() const
{
    return m_buttonX;
}

bool QOpenXRGamepadInput::buttonY() const
{
    return m_buttonY;
}

bool QOpenXRGamepadInput::dpadDown() const
{
    return m_dpadDown;
}

bool QOpenXRGamepadInput::dpadUp() const
{
    return m_dpadUp;
}

bool QOpenXRGamepadInput::dpadRight() const
{
    return m_dpadRight;
}

bool QOpenXRGamepadInput::dpadLeft() const
{
    return m_dpadLeft;
}

bool QOpenXRGamepadInput::shoulderLeft() const
{
    return m_shoulderLeft;
}

bool QOpenXRGamepadInput::shoulderRight() const
{
    return m_shoulderRight;
}

bool QOpenXRGamepadInput::buttonThumbstickLeft() const
{
    return m_buttonThumbstickLeft;
}

bool QOpenXRGamepadInput::buttonThumbstickRight() const
{
    return m_buttonThumbstickRight;
}

float QOpenXRGamepadInput::triggerLeft() const
{
    return m_triggerLeft;
}

float QOpenXRGamepadInput::triggerRight() const
{
    return m_triggerRight;
}

QVector2D QOpenXRGamepadInput::thumbstickLeft() const
{
    return m_thumbstickLeft;
}

QVector2D QOpenXRGamepadInput::thumbstickRight() const
{
    return m_thumbstickRight;
}

void QOpenXRGamepadInput::setButtonMenu(bool value)
{
    if (m_buttonMenu == value)
        return;

    m_buttonMenu = value;
    emit buttonMenuChanged();
}

void QOpenXRGamepadInput::setButtonView(bool value)
{
    if (m_buttonView == value)
        return;

    m_buttonView = value;
    emit buttonViewChanged();
}

void QOpenXRGamepadInput::setButtonA(bool value)
{
    if (m_buttonA == value)
        return;

    m_buttonA = value;
    emit buttonAChanged();
}

void QOpenXRGamepadInput::setButtonB(bool value)
{
    if (m_buttonB == value)
        return;

    m_buttonB = value;
    emit buttonBChanged();
}

void QOpenXRGamepadInput::setButtonX(bool value)
{
    if (m_buttonX == value)
        return;

    m_buttonX = value;
    emit buttonXChanged();
}

void QOpenXRGamepadInput::setButtonY(bool value)
{
    if (m_buttonY == value)
        return;

    m_buttonY = value;
    emit buttonYChanged();
}

void QOpenXRGamepadInput::setDpadDown(bool value)
{
    if (m_dpadDown == value)
        return;

    m_dpadDown = value;
    emit dpadDownChanged();
}

void QOpenXRGamepadInput::setDpadUp(bool value)
{
    if (m_dpadUp == value)
        return;

    m_dpadUp = value;
    emit dpadUpChanged();
}

void QOpenXRGamepadInput::setDpadRight(bool value)
{
    if (m_dpadRight == value)
        return;

    m_dpadRight = value;
    emit dpadRightChanged();
}

void QOpenXRGamepadInput::setDpadLeft(bool value)
{
    if (m_dpadLeft == value)
        return;

    m_dpadLeft = value;
    emit dpadLeftChanged();
}

void QOpenXRGamepadInput::setShoulderRight(bool value)
{
    if (m_shoulderRight == value)
        return;

    m_shoulderRight = value;
    emit shoulderRightChanged();
}

void QOpenXRGamepadInput::setShoulderLeft(bool value)
{
    if (m_shoulderLeft == value)
        return;

    m_shoulderLeft = value;
    emit shoulderLeftChanged();
}

void QOpenXRGamepadInput::setButtonThumbstickLeft(bool value)
{
    if (m_buttonThumbstickLeft == value)
        return;

    m_buttonThumbstickLeft = value;
    emit buttonThumbstickLeftChanged();
}

void QOpenXRGamepadInput::setButtonThumbstickRight(bool value)
{
    if (m_buttonThumbstickRight == value)
        return;

    m_buttonThumbstickRight = value;
    emit buttonThumbstickRightChanged();
}

void QOpenXRGamepadInput::setTriggerLeft(float value)
{
    if (qFuzzyCompare(m_triggerLeft, value))
        return;

    m_triggerLeft = value;
    emit triggerLeftChanged();
}

void QOpenXRGamepadInput::setTriggerRight(float value)
{
    if (qFuzzyCompare(m_triggerRight, value))
        return;

    m_triggerRight = value;
    emit triggerRightChanged();
}

void QOpenXRGamepadInput::setThumbstickLeftX(float value)
{
    setThumbstickLeft(QVector2D(value, m_thumbstickLeft.y()));
}

void QOpenXRGamepadInput::setThumbstickLeftY(float value)
{
    setThumbstickLeft(QVector2D(m_thumbstickLeft.x(), value));
}

void QOpenXRGamepadInput::setThumbstickLeft(const QVector2D &value)
{
    if (m_thumbstickLeft == value)
        return;

    m_thumbstickLeft = value;
    emit thumbstickLeftChanged();
}

void QOpenXRGamepadInput::setThumbstickRightX(float value)
{
    setThumbstickRight(QVector2D(value, m_thumbstickRight.y()));
}

void QOpenXRGamepadInput::setThumbstickRightY(float value)
{
    setThumbstickRight(QVector2D(m_thumbstickRight.x(), value));
}

void QOpenXRGamepadInput::setThumbstickRight(const QVector2D &value)
{
    if (m_thumbstickRight == value)
        return;

    m_thumbstickRight = value;
    emit thumbstickRightChanged();
}

QT_END_NAMESPACE
