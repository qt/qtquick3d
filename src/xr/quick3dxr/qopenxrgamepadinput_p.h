// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QOPENXRGAMEPADINPUT_H
#define QOPENXRGAMEPADINPUT_H

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
#include <QVector2D>
#include <QtQml/qqml.h>

QT_BEGIN_NAMESPACE

class QOpenXRGamepadInput : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool buttonMenu READ buttonMenu NOTIFY buttonMenuChanged)
    Q_PROPERTY(bool buttonView READ buttonView NOTIFY buttonViewChanged)
    Q_PROPERTY(bool buttonA READ buttonA NOTIFY buttonAChanged)
    Q_PROPERTY(bool buttonB READ buttonB NOTIFY buttonBChanged)
    Q_PROPERTY(bool buttonX READ buttonX NOTIFY buttonXChanged)
    Q_PROPERTY(bool buttonY READ buttonY NOTIFY buttonYChanged)
    Q_PROPERTY(bool dpadDown READ dpadDown NOTIFY dpadDownChanged)
    Q_PROPERTY(bool dpadRight READ dpadRight NOTIFY dpadRightChanged)
    Q_PROPERTY(bool dpadUp READ dpadUp NOTIFY dpadUpChanged)
    Q_PROPERTY(bool dpadLeft READ dpadLeft NOTIFY dpadLeftChanged)
    Q_PROPERTY(bool shoulderLeft READ shoulderLeft NOTIFY shoulderLeftChanged)
    Q_PROPERTY(bool shoulderRight READ shoulderRight NOTIFY shoulderRightChanged)
    Q_PROPERTY(bool buttonThumbstickLeft READ buttonThumbstickLeft NOTIFY buttonThumbstickLeftChanged)
    Q_PROPERTY(bool buttonThumbstickRight READ buttonThumbstickRight NOTIFY buttonThumbstickRightChanged)
    Q_PROPERTY(float triggerLeft READ triggerLeft NOTIFY triggerLeftChanged)
    Q_PROPERTY(float triggerRight READ triggerRight NOTIFY triggerRightChanged)
    Q_PROPERTY(QVector2D thumbstickLeft READ thumbstickLeft NOTIFY thumbstickLeftChanged)
    Q_PROPERTY(QVector2D thumbstickRight READ thumbstickRight NOTIFY thumbstickRightChanged)

    QML_NAMED_ELEMENT(XrGamepadInput)
    QML_UNCREATABLE("Created by XrView")

public:
    explicit QOpenXRGamepadInput(QObject *parent = nullptr);

    bool buttonMenu() const;
    bool buttonView() const;
    bool buttonA() const;
    bool buttonB() const;
    bool buttonX() const;
    bool buttonY() const;
    bool dpadDown() const;
    bool dpadUp() const;
    bool dpadRight() const;
    bool dpadLeft() const;
    bool shoulderLeft() const;
    bool shoulderRight() const;
    bool buttonThumbstickLeft() const;
    bool buttonThumbstickRight() const;
    float triggerLeft() const;
    float triggerRight() const;
    QVector2D thumbstickLeft() const;
    QVector2D thumbstickRight() const;

    void setButtonMenu(bool value);
    void setButtonView(bool value);
    void setButtonA(bool value);
    void setButtonB(bool value);
    void setButtonX(bool value);
    void setButtonY(bool value);
    void setDpadDown(bool value);
    void setDpadUp(bool value);
    void setDpadRight(bool value);
    void setDpadLeft(bool value);
    void setShoulderRight(bool value);
    void setShoulderLeft(bool value);
    void setButtonThumbstickLeft(bool value);
    void setButtonThumbstickRight(bool value);
    void setTriggerLeft(float value);
    void setTriggerRight(float value);
    void setThumbstickLeftX(float value);
    void setThumbstickLeftY(float value);
    void setThumbstickLeft(const QVector2D &value);
    void setThumbstickRightX(float value);
    void setThumbstickRightY(float value);
    void setThumbstickRight(const QVector2D &value);

Q_SIGNALS:
    void buttonMenuChanged();
    void buttonViewChanged();
    void buttonAChanged();
    void buttonBChanged();
    void buttonXChanged();
    void buttonYChanged();
    void dpadDownChanged();
    void dpadUpChanged();
    void dpadRightChanged();
    void dpadLeftChanged();
    void shoulderLeftChanged();
    void shoulderRightChanged();
    void buttonThumbstickLeftChanged();
    void buttonThumbstickRightChanged();
    void triggerLeftChanged();
    void triggerRightChanged();
    void thumbstickLeftChanged();
    void thumbstickRightChanged();

private:
    bool m_buttonMenu = false;
    bool m_buttonView = false;
    bool m_buttonA = false;
    bool m_buttonB = false;
    bool m_buttonX = false;
    bool m_buttonY = false;
    bool m_dpadDown = false;
    bool m_dpadUp = false;
    bool m_dpadRight = false;
    bool m_dpadLeft = false;
    bool m_shoulderLeft = false;
    bool m_shoulderRight = false;
    bool m_buttonThumbstickLeft = false;
    bool m_buttonThumbstickRight = false;
    float m_triggerLeft = 0.0f;
    float m_triggerRight = 0.0f;
    QVector2D m_thumbstickLeft;
    QVector2D m_thumbstickRight;
};

QT_END_NAMESPACE

#endif // QOPENXRGAMEPADINPUT_H
