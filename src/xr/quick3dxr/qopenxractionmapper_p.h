// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QOPENXRACTIONMAPPER_H
#define QOPENXRACTIONMAPPER_H

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

QT_BEGIN_NAMESPACE

class QOpenXRController;
class QOpenXrInputAction;
class QOpenXrActionMapper;

// Maybe combine this with controller?
class QOpenXRActionMapper : public QQuick3DObject
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    QML_NAMED_ELEMENT(XrActionMapper)
    QML_ADDED_IN_VERSION(6, 8)
public:
    explicit QOpenXRActionMapper(QQuick3DObject *parent = nullptr);

    enum InputAction {
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
    Q_ENUM(InputAction)

    void handleInput(InputAction id, const char *shortName, float value);
    void registerAction(QOpenXrInputAction *action);

    void classBegin() override;
    void componentComplete() override;

Q_SIGNALS:
    void inputValueChange(InputAction id, QString shortName, float value);

private:
    QOpenXRController *m_controller = nullptr;

    // Data structure: array of pointers, with overflow for the (rare) case of multiple actions for one id.

    bool insertAction(QOpenXrInputAction *action, InputAction id);
    void removeAction(QOpenXrInputAction *action);
    QOpenXrInputAction* m_actionMap[QOpenXRActionMapper::NumActions] = {};
    quint64 m_actionMapOverflow = 0;
    QList<QOpenXrInputAction *> m_extraActions;
    void setOverflow(InputAction id) { m_actionMapOverflow |= (quint64(1) << id); }
    void clearOverflow(InputAction id) { m_actionMapOverflow &= ~(quint64(1) << id); }
    bool isOverflow(InputAction id) { return m_actionMapOverflow & (quint64(1) << id); }
};

class QOpenXrInputAction : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    QML_NAMED_ELEMENT(XrInputAction)
    QML_ADDED_IN_VERSION(6, 8)

    Q_PROPERTY(float value READ value NOTIFY valueChanged FINAL)
    Q_PROPERTY(bool pressed READ pressed NOTIFY pressedChanged FINAL)
    Q_PROPERTY(QString actionName READ actionName WRITE setActionName NOTIFY actionNameChanged FINAL)
    Q_PROPERTY(QList<QOpenXRActionMapper::InputAction> actionId READ actionId WRITE setActionId NOTIFY actionIdChanged FINAL)

public:
    explicit QOpenXrInputAction(QObject *parent = nullptr);
    float value() const;
    void setValue(float newValue);
    bool pressed() const;
    void setPressed(bool newPressed);

    QString actionName() const;
    void setActionName(const QString &newActionName);

    QList<QOpenXRActionMapper::InputAction> actionId() const;
    void setActionId(const QList<QOpenXRActionMapper::InputAction> &newActionId);

    void classBegin() override;
    void componentComplete() override;

signals:
    void valueChanged();
    void pressedChanged();
    void triggered();

    void actionNameChanged();
    void actionIdChanged();

private:
    QString m_actionName;
    float m_value = 0;
    bool m_pressed = false;

    QList<QOpenXRActionMapper::InputAction> m_actionIds;
};

QT_END_NAMESPACE

#endif // QOPENXRACTIONMAPPER_H
