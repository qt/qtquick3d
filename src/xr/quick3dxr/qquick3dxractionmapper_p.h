// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default


#ifndef QQUICK3DXRACTIONMAPPER_H
#define QQUICK3DXRACTIONMAPPER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QObject>
#include <QQmlEngine>
#include <QQuick3DObject>
#include <QMultiHash>
#include <QPointer>
#include "qquick3dxrabstracthapticeffect_p.h"
#include <QtQuick3DXr/private/qtquick3dxrglobal_p.h>

QT_BEGIN_NAMESPACE

class QQuick3DXrController;
class QQuick3DXrInputAction;
class QQuick3DXrActionMapper;

class QQuick3DXrInputAction : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    QML_NAMED_ELEMENT(XrInputAction)
    QML_ADDED_IN_VERSION(6, 8)

    Q_PROPERTY(float value READ value NOTIFY valueChanged FINAL)
    Q_PROPERTY(bool pressed READ pressed NOTIFY pressedChanged FINAL)
    Q_PROPERTY(QString actionName READ actionName WRITE setActionName NOTIFY actionNameChanged FINAL)
    Q_PROPERTY(QList<Action> actionId READ actionId WRITE setActionId NOTIFY actionIdChanged FINAL)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged FINAL REVISION(6, 9))

    Q_PROPERTY(Controller hand READ hand WRITE setHand NOTIFY handChanged FINAL)
    Q_PROPERTY(Controller controller READ controller WRITE setController NOTIFY controllerChanged FINAL REVISION(6, 10))

public:

    enum class Controller {
        LeftController = uint(QtQuick3DXr::Controller::LeftController),
        RightController = uint(QtQuick3DXr::Controller::RightController),
        UnknownController = uint(QtQuick3DXr::Controller::UnknownController),
        LeftHand = uint(QtQuick3DXr::Controller::LeftHand),
        RightHand = uint(QtQuick3DXr::Controller::RightHand),
        ControllerLeft = LeftController,
        ControllerRight = RightController,
        ControllerNone = UnknownController,
    };
    Q_ENUM(Controller)

    enum Action : qint16 {
        CustomAction = -1,
        Button1Pressed,
        Button1Touched,
        Button2Pressed,
        Button2Touched,
        ButtonMenuPressed,
        ButtonMenuTouched,
        ButtonSystemPressed,
        ButtonSystemTouched,
        SqueezeValue,
        SqueezeForce,
        SqueezePressed,
        TriggerValue,
        TriggerPressed,
        TriggerTouched,
        ThumbstickX,
        ThumbstickY,
        ThumbstickPressed,
        ThumbstickTouched,
        ThumbrestTouched,
        TrackpadX,
        TrackpadY,
        TrackpadForce,
        TrackpadTouched,
        TrackpadPressed,
        IndexFingerPinch,
        MiddleFingerPinch,
        RingFingerPinch,
        LittleFingerPinch,
        HandTrackingMenuPress,
        NumHandActions,
        NumActions
    };
    Q_ENUM(Action)

    explicit QQuick3DXrInputAction(QObject *parent = nullptr);
    ~QQuick3DXrInputAction() override;
    float value() const;
    void setValue(float newValue);
    bool pressed() const;
    void setPressed(bool newPressed);

    QString actionName() const;
    void setActionName(const QString &newActionName);

    QList<Action> actionId() const;
    void setActionId(const QList<Action> &newActionId);

    void classBegin() override;
    void componentComplete() override;

    Controller hand() const;
    void setHand(Controller newHand);

    bool enabled() const;
    void setEnabled(bool newEnabled);

    Controller controller() const;
    void setController(Controller newController);

Q_SIGNALS:
    void valueChanged();
    void pressedChanged();
    void triggered();

    void actionNameChanged();
    void actionIdChanged();

    void handChanged();

    void enabledChanged();

    void controllerChanged();

private:
    QString m_actionName;
    float m_value = 0;
    bool m_pressed = false;
    bool m_componentComplete = false;
    bool m_enabled = true;
    Controller m_controller;

    QList<Action> m_actionIds;
};

class QQuick3DXrHapticFeedback : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    QML_NAMED_ELEMENT(XrHapticFeedback)
    QML_ADDED_IN_VERSION(6, 9)

    Q_PROPERTY(Controller controller READ controller WRITE setController NOTIFY controllerChanged FINAL)
    Q_PROPERTY(QQuick3DXrAbstractHapticEffect *hapticEffect READ hapticEffect WRITE setHapticEffect NOTIFY hapticEffectChanged FINAL)
    Q_PROPERTY(bool trigger READ trigger WRITE setTrigger NOTIFY triggerChanged FINAL)
    Q_PROPERTY(Condition condition READ condition WRITE setCondition NOTIFY conditionChanged FINAL)

public:

    // Values here must match the QtQuick3DXr::Controller enum
    enum class Controller {
        LeftController = uint(QtQuick3DXr::Controller::LeftController),
        RightController = uint(QtQuick3DXr::Controller::RightController),
        UnknownController = uint(QtQuick3DXr::Controller::UnknownController),
        LeftHand = uint(QtQuick3DXr::Controller::LeftHand),
        RightHand = uint(QtQuick3DXr::Controller::RightHand),
        ControllerLeft = LeftController,
        ControllerRight = RightController,
        ControllerNone = UnknownController,
    };
    Q_ENUM(Controller)

    enum class Condition : quint8 {
        RisingEdge = 0,
        TrailingEdge,
    };
    Q_ENUM(Condition)

    explicit QQuick3DXrHapticFeedback(QObject *parent = nullptr);
    ~QQuick3DXrHapticFeedback() override;

    void classBegin() override;
    void componentComplete() override;

    QQuick3DXrAbstractHapticEffect *hapticEffect() const;
    void setHapticEffect(QQuick3DXrAbstractHapticEffect *newHapticEffect);

    Controller controller() const;
    void setController(Controller newController);

    bool trigger();
    void setTrigger(bool newTrigger);

    enum Condition condition()  const;
    void setCondition(enum Condition newCondition);

    bool testAndClear();

Q_SIGNALS:
    void controllerChanged();
    void hapticEffectChanged();
    void triggerChanged();
    void conditionChanged();

public Q_SLOTS:
    void start();
    void stop();

private:
    QMetaObject::Connection m_triggerConnection;
    Controller m_controller = Controller::UnknownController;
    Condition m_condition = Condition::RisingEdge;
    QPointer<QQuick3DXrAbstractHapticEffect> m_hapticEffect;
    bool m_trigger = false;
    bool m_componentComplete = false;
    bool m_pending = false;
};

class QQuick3DXrActionMapper : public QObject
{
    Q_OBJECT
public:
    static QQuick3DXrActionMapper *instance();

    static QList<QPointer<QQuick3DXrHapticFeedback>> getHapticEffects(QtQuick3DXr::Controller controller);

    static void handleInput(QQuick3DXrInputAction::Action id, QtQuick3DXr::Controller controller, const char *shortName, float value);
    static void registerAction(QQuick3DXrInputAction *action);
    static void registerHapticEffect(QPointer<QQuick3DXrHapticFeedback>);
    static void removeAction(QQuick3DXrInputAction *action);
    static void removeHapticEffect(QQuick3DXrHapticFeedback *action);

private:
    explicit QQuick3DXrActionMapper(QObject *parent = nullptr);

    struct HapticData
    {
        QList<QPointer<QQuick3DXrHapticFeedback>> m_hapticEffects;
    } m_hapticData[2];

    QMultiHash<quint32, QQuick3DXrInputAction *> m_actions;
    QMultiHash<QString, QQuick3DXrInputAction *> m_customActions;
};

QT_END_NAMESPACE

#endif // QQUICK3DXRACTIONMAPPER_H
