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
    // emit that->inputValueChange(id, name, value); // TODO: emit a signal from public class

    QList<QQuick3DXrInputAction *> actions;
    if (id == QQuick3DXrInputAction::CustomAction) {
        actions = that->m_customActions.values(actionStringKey(name, hand));
    } else {
        actions = that->m_actions.values(actionIntKey(id, hand));
    }

    for (const auto &action : std::as_const(actions))
        set(action, value);
}

// Note: it is the responsibility of the caller to call removeAction() before the action is destroyed or actionId/actionName is changed
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
    \inherits QtObject
    \inqmlmodule QtQuick3D.Xr
    \brief Represents an action from an input controller.

    Actions can be boolean, such as a button press, or analog, such as a joystick axis.

    Use the \l pressed property or the \l triggered signal to react to a boolean action. An analog action will set the \l value property.

    \note For convenience, an analog property will also set the \c pressed property and emit the \c triggered signal,
    while a boolean property will set \c value to 1.0 when pressed.

    The following shows how to react to either the right hand grip being pressed or to a right hand pinch gesture from hand tracking:

    \qml
    XrInputAction {
        hand: XrInputAction.RightHand
        actionId: [XrInputAction.SqueezePressed, XrInputAction.SqueezeValue, XrInputAction.IndexFingerPinch]
        onTriggered: console.log("Do action here.")
    }
    \endqml

    The reason for specifying both \c SqueezePressed and \c SqueezeValue is that some controllers have an analog grip button,
    and some controllers just have an on/off grip switch.
 */

/*!
    \qmlsignal XrInputAction::triggered()

    This signal is emitted when a boolean action is activated. This happens at the same time as the \l pressed property is set to \c true.
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
    \qmlproperty string XrInputAction::actionName
    \brief The name of the input action.

    Use this property to specify the name of the custom input action you want to react to. This property does not have an effect if \l actionId is set.
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

QQuick3DXrInputAction::~QQuick3DXrInputAction()
{
    QQuick3DXrActionMapper::removeAction(this);
}

/*!
    \qmlproperty real XrInputAction::value
    \brief The analog value of the input action.

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
    \brief Specifies the action(s) to react to

    Holds a List of IDs that can be of the following values:

    \value XrInputAction.Button1Pressed Button 1 is pressed. \e Boolean.
    \value XrInputAction.Button1Touched Button 1 is touched. \e Boolean.
    \value XrInputAction.Button2Pressed Button 2 is pressed. \e Boolean.
    \value XrInputAction.Button2Touched Button 2 is touched. \e Boolean.
    \value XrInputAction.ButtonMenuPressed The menu button is pressed. \e Boolean.
    \value XrInputAction.ButtonMenuTouched The menu button is touched. \e Boolean.
    \value XrInputAction.ButtonSystemPressed The system button is pressed. \e Boolean.
    \value XrInputAction.ButtonSystemTouched The system button is touched. \e Boolean.
    \value XrInputAction.SqueezeValue How far the grip button is pressed. \e Analog.
    \value XrInputAction.SqueezeForce The force applied to the grip button. \e Analog.
    \value XrInputAction.SqueezePressed The grip button is pressed.  \e Boolean.
    \value XrInputAction.TriggerValue How far the trigger button is pressed. \e Analog.
    \value XrInputAction.TriggerPressed The trigger is pressed. \e Boolean.
    \value XrInputAction.TriggerTouched The trigger is touched. \e Boolean.
    \value XrInputAction.ThumbstickX The X-axis value of the thumbstick. \e Analog.
    \value XrInputAction.ThumbstickY The Y-axis value of the thumbstick. \e Analog.
    \value XrInputAction.ThumbstickPressed The thumbstick is pressed. \e Boolean.
    \value XrInputAction.ThumbstickTouched The thumbstick is touched. \e Boolean.
    \value XrInputAction.ThumbrestTouched The thumbrest is touched. \e Boolean.
    \value XrInputAction.TrackpadX The X-axis position on the trackpad. \e Analog.
    \value XrInputAction.TrackpadY The Y-axis position on the trackpad. \e Analog.
    \value XrInputAction.TrackpadForce The force applied on the trackpad. \e Analog.
    \value XrInputAction.TrackpadTouched The trackpad is touched. \e Boolean.
    \value XrInputAction.TrackpadPressed The trackpad is pressed. \e Boolean.
    \value XrInputAction.IndexFingerPinch Thumb to index finger pinch gesture. \e Boolean.
    \value XrInputAction.MiddleFingerPinch Thumb to middle finger pinch gesture. \e Boolean.
    \value XrInputAction.RingFingerPinch Thumb to ring finger pinch gesture. \e Boolean.
    \value XrInputAction.LittleFingerPinch Thumb to little finger pinch gesture. \e Boolean.
    \value XrInputAction.HandTrackingMenuPress Hand tracking menu gesture. \e Boolean.
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

/*!
    \qmlproperty enumeration QtQuick3D.Xr::XrInputAction::hand
    \brief The Hand that this input action will apply to.

    Specifies the hand ro react to.

    It can be one of:

    \value XrInputAction.LeftHand
    \value XrInputAction.RightHand
    \value XrInputAction.Unknown
 */

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
