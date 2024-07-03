// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qquick3dxractionmapper_p.h"

QT_BEGIN_NAMESPACE

QQuick3DXrActionMapper::QQuick3DXrActionMapper(QObject *parent) : QObject(parent)
{
}

static inline quint32 actionIntKey(const QQuick3DXrInputAction::Action id, const QQuick3DXrInputAction::Hand hand)
{
    return quint16(id) | (quint32(hand) << 16);
}

static inline QString actionStringKey(const QString &name, const QQuick3DXrInputAction::Hand hand)
{
    return QString::number(hand) + name;
}

QQuick3DXrActionMapper *QQuick3DXrActionMapper::instance()
{
    static QQuick3DXrActionMapper instance;
    return &instance;
}

void QQuick3DXrActionMapper::handleInput(QQuick3DXrInputAction::Action id, QQuick3DXrInputAction::Hand hand, const char *shortName, float value)
{
    auto *that = instance();
    auto set = [](auto action, auto value) {
        action->setValue(value);
        // TODO: distinguish between bool and float values
        action->setPressed(value > 0.9);
    };

    const QLatin1StringView name(shortName);
    emit that->inputValueChange(id, name, value); // TODO: emit signal from public class (XrController?)

    QList<QQuick3DXrInputAction *> actions;
    if (id == QQuick3DXrInputAction::CustomAction) {
        actions = that->m_customActions.values(actionStringKey(name, hand));
    } else {
        actions = that->m_actions.values(actionIntKey(id, hand));
    }

    for (const auto &action : std::as_const(actions))
        set(action, value);
}

// Note: it is the responsibility of the caller to call removeAction() before changing actionId/actionName
void QQuick3DXrActionMapper::registerAction(QQuick3DXrInputAction *action)
{
    auto *that = instance();

    const auto &idList = action->actionId();
    const auto hand = action->hand();

    if (idList.isEmpty()) {
        that->m_customActions.insert(actionStringKey(action->actionName(), hand), action);
    } else {
        for (const auto &id : idList) {
            if (id != QQuick3DXrInputAction::CustomAction)
                that->m_actions.insert(actionIntKey(id, hand), action);
        }
    }

    connect(action, &QObject::destroyed, that, [that, action](){ that->removeAction(action); });
}

void QQuick3DXrActionMapper::removeAction(QQuick3DXrInputAction *action)
{
    auto *that = instance();

    const auto idList = action->actionId();
    const auto hand = action->hand();
    if (idList.isEmpty()) {
        that->m_customActions.remove(action->actionName(), action);
    } else {
        for (const auto &id : idList) {
            if (id != QQuick3DXrInputAction::CustomAction)
                that->m_actions.remove(actionIntKey(id, hand));
        }
    }
}

/*!
    \qmltype XrInputAction
    \inherits Item
    \inqmlmodule QtQuick3D.Xr
    \brief Maps input actions to corresponding actions.
 */

void QQuick3DXrInputAction::setValue(float newValue)
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

bool QQuick3DXrInputAction::pressed() const
{
    return m_pressed;
}

void QQuick3DXrInputAction::setPressed(bool newPressed)
{
    if (m_pressed == newPressed)
        return;
    m_pressed = newPressed;
    emit pressedChanged();
    if (newPressed)
        emit triggered();
}

/*!
    \qmlproperty string QQuick3DXrInputAction::actionName
    \brief The name of the input action.

    Use this property to specify the name of the custom input action you want to map. This property does not have an effect if \l actionId is set.
 */

QString QQuick3DXrInputAction::actionName() const
{
    return m_actionName;
}

void QQuick3DXrInputAction::setActionName(const QString &newActionName)
{
    if (m_actionName == newActionName)
        return;
    const bool needsRemap = m_actionIds.isEmpty() && m_componentComplete;
    if (needsRemap)
        QQuick3DXrActionMapper::removeAction(this);
    m_actionName = newActionName;
    if (needsRemap)
        QQuick3DXrActionMapper::registerAction(this);
    emit actionNameChanged();
}

QQuick3DXrInputAction::QQuick3DXrInputAction(QObject *parent)
    : QObject(parent)
{
}

/*!
    \qmlproperty float XrInputAction::value
    \brief The value associated with the input action.

    For analog inputs, such as a thumbstick position, this property holds
    the value of the input (usually in the range [0, 1]).
 */

float QQuick3DXrInputAction::value() const
{
    return m_value;
}

void QQuick3DXrInputAction::classBegin()
{
}

void QQuick3DXrInputAction::componentComplete()
{
    QQuick3DXrActionMapper::registerAction(this);
    m_componentComplete = true;
}

/*!
    \qmlproperty List<enumeration> XrInputAction::actionId
    \brief The value associated with the input action.

    Holds a List of InputActions Ids, that can be of the following values:

    \value XrInputAction.CustomAction Represents a custom action with a value
    of -1.
    \value XrInputAction.Button1Pressed Indicates that Button 1 is pressed.
    \value XrInputAction.Button1Touched Indicates that Button 1 is touched.
    \value XrInputAction.Button2Pressed Indicates that Button 2 is pressed.
    \value XrInputAction.Button2Touched Indicates that Button 2 is touched.
    \value XrInputAction.ButtonMenuPressed Indicates that the menu button is
    pressed.
    \value XrInputAction.ButtonMenuTouched Indicates that the menu button is
    touched.
    \value XrInputAction.ButtonSystemPressed Indicates that the system button
    is pressed.
    \value XrInputAction.ButtonSystemTouched Indicates that the system button
    is touched.
    \value XrInputAction.SqueezeValue Represents the squeeze value in a
    controller.
    \value XrInputAction.SqueezeForce Represents the force of a squeeze action.
    \value XrInputAction.SqueezePressed Indicates that the squeeze action is
    pressed.
    \value XrInputAction.TriggerValue Represents the value of the trigger
    (for example, how much it's pressed).
    \value XrInputAction.TriggerPressed Indicates that the trigger is pressed.
    \value XrInputAction.TriggerTouched Indicates that the trigger is touched.
    \value XrInputAction.ThumbstickX Represents the X-axis value of the
    thumbstick.
    \value XrInputAction.ThumbstickY Represents the Y-axis value of the
    thumbstick.
    \value XrInputAction.ThumbstickPressed Indicates that the thumbstick
    is pressed.
    \value XrInputAction.ThumbstickTouched Indicates that the thumbstick
    is touched.
    \value XrInputAction.ThumbrestTouched Indicates that the thumbrest
    is touched.
    \value XrInputAction.TrackpadX Represents the X-axis value of the trackpad.
    \value XrInputAction.TrackpadY Represents the Y-axis value of the trackpad.
    \value XrInputAction.TrackpadForce Represents the force applied on the
    trackpad.
    \value XrInputAction.TrackpadTouched Indicates that the trackpad is touched.
    \value XrInputAction.TrackpadPressed Indicates that the trackpad is pressed.
    \value XrInputAction.IndexFingerPinch Indicates that the index finger is
    pinched.
    \value XrInputAction.MiddleFingerPinch Indicates that the middle finger is
    pinched.
    \value XrInputAction.RingFingerPinch Indicates that the ring finger is pinched.
    \value XrInputAction.LittleFingerPinch Indicates that the little finger is
    pinched.
    \value XrInputAction.HandTrackingMenuPress Indicates a menu press in hand
    tracking.
    \value XrInputAction.NumHandActions Represents the total number of hand
    actions.
    \value XrInputAction.NumActions Number of actions.
 */

QList<QQuick3DXrInputAction::Action> QQuick3DXrInputAction::actionId() const
{
    return m_actionIds;
}

void QQuick3DXrInputAction::setActionId(const QList<Action> &newActionId)
{
    if (m_actionIds == newActionId)
        return;

    if (m_componentComplete)
        QQuick3DXrActionMapper::removeAction(this);

    m_actionIds = newActionId;

    if (m_componentComplete)
        QQuick3DXrActionMapper::registerAction(this);

    emit actionIdChanged();
}


QQuick3DXrInputAction::Hand QQuick3DXrInputAction::hand() const
{
    return m_hand;
}

void QQuick3DXrInputAction::setHand(Hand newHand)
{
    if (m_hand == newHand)
        return;
    m_hand = newHand;
    emit handChanged();
}

QT_END_NAMESPACE
