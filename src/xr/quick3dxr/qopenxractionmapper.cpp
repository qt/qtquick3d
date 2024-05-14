// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qopenxractionmapper_p.h"
#include "qopenxrcontroller_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype XrActionMapper
    \inherits Object3D
    \inqmlmodule QtQuick3D.Xr
    \brief Maps XrInputActions to affect an XR application.
 */
QOpenXRActionMapper::QOpenXRActionMapper(QQuick3DObject *parent) : QQuick3DObject(parent)
{
}

void QOpenXRActionMapper::classBegin()
{
}

void QOpenXRActionMapper::componentComplete()
{
    m_controller = qobject_cast<QOpenXRController *>(parent());
    if (m_controller && !m_controller->actionMapper())
        m_controller->setActionMapper(this);
}

// returns true if inserted into m_actionMap
bool QOpenXRActionMapper::insertAction(QOpenXrInputAction *action, InputAction id)
{
    Q_ASSERT(id > CustomAction && id < NumActions);
    if (m_actionMap[id] == nullptr) {
        m_actionMap[id] = action;
        return true;
    } else {
        setOverflow(id);
        return false;
    }
}

void QOpenXRActionMapper::removeAction(QOpenXrInputAction *action)
{
    const auto idList = action->actionId();
    for (const auto &id : idList) {
        if (id != CustomAction && m_actionMap[id] == action)
            m_actionMap[id] = nullptr;
    }
    m_extraActions.removeAll(action);
}

void QOpenXRActionMapper::handleInput(InputAction id, const char *shortName, float value)
{
    auto set = [](auto action, auto value) {
        action->setValue(value);
        // TODO: distinguish between bool and float values
        action->setPressed(value > 0.9);
    };

    if (id == CustomAction) {
        emit inputValueChange(id, QLatin1StringView(shortName), value);
    } else {
        emit inputValueChange(id, {}, value);
        if (auto *action = m_actionMap[id])
            set(action, value);

    }

    if (id == CustomAction || isOverflow(id)) {
        for (auto *action : std::as_const(m_extraActions)) {
            if ((action->actionId().isEmpty() && action->actionName() == QLatin1StringView(shortName)) || action->actionId().contains(id))
                set(action, value);
        }
    }
}

void QOpenXRActionMapper::registerAction(QOpenXrInputAction *action)
{
    const auto idList = action->actionId();
    bool needsExtra = idList.isEmpty(); // We definitely need to add to m_extraActions if no actionId
    for (const auto &id : idList) {
        if (id != CustomAction)
            needsExtra = !insertAction(action, id) || needsExtra; //make sure not to shortcut the function call
    }
    if (needsExtra)
        m_extraActions.append(action);
    connect(action, &QObject::destroyed, this, [this, action](){ removeAction(action); });
}

/*!
    \qmlsignal XrActionMapper::inputValueChange(InputAction id, QString shortName, float value)
    \brief Emitted when an input action is triggered.

    This signal provides an \a id, \a shortName, and \a value when an inputValue
    has changed.
 */

/*!
    \qmltype XrInputAction
    \inherits Item
    \inqmlmodule QtQuick3D.Xr
    \brief Maps input actions to corresponding actions.
 */

void QOpenXrInputAction::setValue(float newValue)
{
    if (qFuzzyCompare(m_value, newValue))
        return;
    m_value = newValue;
    emit valueChanged();
}

/*!
    \qmlproperty bool XrInputAction::pressed
    \brief Indicates whether the input action is currently pressed.

    Use this property to check if the input action (for example, a button)
    is currently pressed.
 */

bool QOpenXrInputAction::pressed() const
{
    return m_pressed;
}

void QOpenXrInputAction::setPressed(bool newPressed)
{
    if (m_pressed == newPressed)
        return;
    m_pressed = newPressed;
    emit pressedChanged();
    if (newPressed)
        emit triggered();
}

/*!
    \qmlproperty string QOpenXrInputAction::actionName
    \brief The name of the input action.

    Use this property to specify the name of the input action you want to map.
 */

QString QOpenXrInputAction::actionName() const
{
    return m_actionName;
}

void QOpenXrInputAction::setActionName(const QString &newActionName)
{
    if (m_actionName == newActionName)
        return;
    m_actionName = newActionName;
    emit actionNameChanged();
}

QOpenXrInputAction::QOpenXrInputAction(QObject *parent)
    : QObject(parent)
{
}

/*!
    \qmlproperty float XrInputAction::value
    \brief The value associated with the input action.

    For analog inputs, such as a thumbstick position, this property holds
    the value of the input (usually in the range [0, 1]).
 */

float QOpenXrInputAction::value() const
{
    return m_value;
}

void QOpenXrInputAction::classBegin()
{
}

void QOpenXrInputAction::componentComplete()
{
    auto *mapper = qobject_cast<QOpenXRActionMapper *>(parent());
    if (mapper)
        mapper->registerAction(this);
}

/*!
    \qmlproperty List<enumeration> XrInputAction::actionId
    \brief The value associated with the input action.

    Holds a List of InputActions Ids, that can be of the following values:

    \value XrActionmappter.CustomAction Represents a custom action with a value
    of -1.
    \value XrActionmappter.Button1Pressed Indicates that Button 1 is pressed.
    \value XrActionMapper.Button1Touched Indicates that Button 1 is touched.
    \value XrActionMapper.Button2Pressed Indicates that Button 2 is pressed.
    \value XrActionMapper.Button2Touched Indicates that Button 2 is touched.
    \value XrActionMapper.ButtonMenuPressed Indicates that the menu button is
    pressed.
    \value XrActionMapper.ButtonMenuTouched Indicates that the menu button is
    touched.
    \value XrActionMapper.ButtonSystemPressed Indicates that the system button
    is pressed.
    \value XrActionMapper.ButtonSystemTouched Indicates that the system button
    is touched.
    \value XrActionMapper.SqueezeValue Represents the squeeze value in a
    controller.
    \value XrActionMapper.SqueezeForce Represents the force of a squeeze action.
    \value XrActionMapper.SqueezePressed Indicates that the squeeze action is
    pressed.
    \value XrActionMapper.TriggerValue Represents the value of the trigger
    (for example, how much it's pressed).
    \value XrActionMapper.TriggerPressed Indicates that the trigger is pressed.
    \value XrActionMapper.TriggerTouched Indicates that the trigger is touched.
    \value XrActionMapper.ThumbstickX Represents the X-axis value of the
    thumbstick.
    \value XrActionMapper.ThumbstickY Represents the Y-axis value of the
    thumbstick.
    \value XrActionMapper.ThumbstickPressed Indicates that the thumbstick
    is pressed.
    \value XrActionMapper.ThumbstickTouched Indicates that the thumbstick
    is touched.
    \value XrActionMapper.ThumbrestTouched Indicates that the thumbrest
    is touched.
    \value XrActionMapper.TrackpadX Represents the X-axis value of the trackpad.
    \value XrActionMapper.TrackpadY Represents the Y-axis value of the trackpad.
    \value XrActionMapper.TrackpadForce Represents the force applied on the
    trackpad.
    \value XrActionMapper.TrackpadTouched Indicates that the trackpad is touched.
    \value XrActionMapper.TrackpadPressed Indicates that the trackpad is pressed.
    \value XrActionMapper.IndexFingerPinch Indicates that the index finger is
    pinched.
    \value XrActionMapper.MiddleFingerPinch Indicates that the middle finger is
    pinched.
    \value XrActionMapper.RingFingerPinch Indicates that the ring finger is pinched.
    \value XrActionMapper.LittleFingerPinch Indicates that the little finger is
    pinched.
    \value XrActionMapper.HandTrackingMenuPress Indicates a menu press in hand
    tracking.
    \value XrActionMapper.NumHandActions Represents the total number of hand
    actions.
    \value XrActionMapper.GamepadButtonMenuPressed Indicates that a gamepad menu
    button is pressed.
    \value XrActionMapper.GamepadButtonViewPressed Indicates that a gamepad view
    button is pressed.
    \value XrActionMapper.GamepadButtonAPressed Indicates that the gamepad A
    button is pressed.
    \value XrActionMapper.GamepadButtonBPressed Indicates that the gamepad B
    button is pressed.
    \value XrActionMapper.GamepadButtonXPressed Indicates that the gamepad X
    button is pressed.
    \value XrActionMapper.GamepadButtonYPressed Indicates that the gamepad Y
    button is pressed.
    \value XrActionMapper.GamepadButtonDownPressed Indicates that the gamepad
    down button is pressed.
    \value XrActionMapper.GamepadButtonRightPressed Indicates that the gamepad
    right button is pressed.
    \value XrActionMapper.GamepadButtonUpPressed Indicates that the gamepad up
    button is pressed.
    \value XrActionMapper.GamepadButtonLeftPressed Indicates that the gamepad
    left button is pressed.
    \value XrActionMapper.GamepadShoulderLeftPressed Indicates that the left
    shoulder button on the gamepad is pressed.
    \value XrActionMapper.GamepadShoulderRightPressed Indicates that the right
    shoulder button on the gamepad is pressed.
    \value XrActionMapper.GamepadThumbstickLeftPressed Indicates that the left
    thumbstick on the gamepad is pressed.
    \value XrActionMapper.GamepadThumbstickRightPressed Indicates that the right
    thumbstick on the gamepad is pressed.
    \value XrActionMapper.GamepadTriggerLeft Represents the value of the left
    trigger on the gamepad.
    \value XrActionMapper.GamepadTriggerRight Represents the value of the right
    trigger on the gamepad.
    \value XrActionMapper.GamepadThumbstickLeftX Represents the X-axis value of
    the left thumbstick on the gamepad.
    \value XrActionMapper.GamepadThumbstickLeftY Represents the Y-axis value of
    the left thumbstick on the gamepad.
    \value XrActionMapper.GamepadThumbstickRightX Represents the X-axis value of
    the right thumbstick on the gamepad.
    \value XrActionMapper.GamepadThumbstickRightY Represents the Y-axis value of
    the right  thumbstick on the gamepad.
    \value XrActionMapper.NumActions Number of actions.
 */

QList<QOpenXRActionMapper::InputAction> QOpenXrInputAction::actionId() const
{
    return m_actionIds;
}

void QOpenXrInputAction::setActionId(const QList<QOpenXRActionMapper::InputAction> &newActionId)
{
    if (m_actionIds == newActionId)
        return;
    m_actionIds = newActionId;
    emit actionIdChanged();
}

/*!
    \qmlsignal XrInputAction::valueChanged()
    \brief Emitted when the value property changes.
 */
 /*!
    \qmlsignal XrInputAction::pressedChanged()
    \brief Emitted when the pressed property changes.
 */
/*!
    \qmlsignal XrInputAction::triggered()
    \brief Emitted when the input action is triggered.
 */
 /*!
    \qmlsignal XrInputAction::actionNameChanged()
    \brief Emitted when the name of the input action changes.
 */
 /*!
    \qmlsignal XrInputAction::actionIdChanged()
    \brief Emitted when the ID of the input action changes.
 */

QT_END_NAMESPACE
