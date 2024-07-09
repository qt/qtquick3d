// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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

    Q_PROPERTY(Hand hand READ hand WRITE setHand NOTIFY handChanged FINAL)

public:

    // Same values as XrController and XrHandModel enums
    enum Hand : quint8 {
        LeftHand = 0,
        RightHand,
        Unknown,
    };
    Q_ENUM(Hand)

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

    Hand hand() const;
    void setHand(Hand newHand);

signals:
    void valueChanged();
    void pressedChanged();
    void triggered();

    void actionNameChanged();
    void actionIdChanged();

    void handChanged();

private:
    QString m_actionName;
    float m_value = 0;
    bool m_pressed = false;
    bool m_componentComplete = false;
    Hand m_hand;

    QList<Action> m_actionIds;
};

class QQuick3DXrActionMapper : public QObject
{
    Q_OBJECT
public:
    static QQuick3DXrActionMapper *instance();

    static void handleInput(QQuick3DXrInputAction::Action id, QQuick3DXrInputAction::Hand hand, const char *shortName, float value);
    static void registerAction(QQuick3DXrInputAction *action);
    static void removeAction(QQuick3DXrInputAction *action);

private:
    explicit QQuick3DXrActionMapper(QObject *parent = nullptr);

    QMultiHash<quint32, QQuick3DXrInputAction *> m_actions;
    QMultiHash<QString, QQuick3DXrInputAction *> m_customActions;
};

QT_END_NAMESPACE

#endif // QQUICK3DXRACTIONMAPPER_H
