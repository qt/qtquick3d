// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qopenxractionmapper_p.h"
#include "qopenxrcontroller_p.h"

QT_BEGIN_NAMESPACE

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

void QOpenXrInputAction::setValue(float newValue)
{
    if (qFuzzyCompare(m_value, newValue))
        return;
    m_value = newValue;
    emit valueChanged();
}

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

QT_END_NAMESPACE
