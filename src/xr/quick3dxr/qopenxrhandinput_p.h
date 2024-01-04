// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QOPENXRHANDINPUT_H
#define QOPENXRHANDINPUT_H

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
#include <QVector3D>
#include <QQuaternion>
#include <QtQml/qqml.h>

QT_BEGIN_NAMESPACE

class QOpenXRHandInput : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isActive READ isActive NOTIFY isActiveChanged)
    Q_PROPERTY(bool button1Pressed READ button1Pressed NOTIFY button1PressedChanged)
    Q_PROPERTY(bool button1Touched READ button1Touched NOTIFY button1TouchedChanged)
    Q_PROPERTY(bool button2Pressed READ button2Pressed NOTIFY button2PressedChanged)
    Q_PROPERTY(bool button2Touched READ button2Touched NOTIFY button2TouchedChanged)
    Q_PROPERTY(bool buttonMenuPressed READ buttonMenuPressed NOTIFY buttonMenuPressedChanged)
    Q_PROPERTY(bool buttonMenuTouched READ buttonMenuTouched NOTIFY buttonMenuTouchedChanged)
    Q_PROPERTY(bool buttonSystemPressed READ buttonSystemPressed NOTIFY buttonSystemPressedChanged)
    Q_PROPERTY(bool buttonSystemTouched READ buttonSystemTouched NOTIFY buttonSystemTouchedChanged)
    Q_PROPERTY(float squeezeValue READ squeezeValue NOTIFY squeezeValueChanged)
    Q_PROPERTY(float squeezeForce READ squeezeForce NOTIFY squeezeForceChanged)
    Q_PROPERTY(bool squeezePressed READ squeezePressed NOTIFY squeezePressedChanged)
    Q_PROPERTY(float triggerValue READ triggerValue NOTIFY triggerValueChanged)
    Q_PROPERTY(bool triggerPressed READ triggerPressed NOTIFY triggerPressedChanged)
    Q_PROPERTY(bool triggerTouched READ triggerTouched NOTIFY triggerTouchedChanged)
    Q_PROPERTY(QVector2D thumbstickValues READ thumbstickValues NOTIFY thumbstickValuesChanged)
    Q_PROPERTY(bool thumbstickPressed READ thumbstickPressed NOTIFY thumbstickPressedChanged)
    Q_PROPERTY(bool thumbstickTouched READ thumbstickTouched NOTIFY thumbstickTouchedChanged)
    Q_PROPERTY(bool thumbrestTouched READ thumbrestTouched NOTIFY thumbrestTouchedChanged)
    Q_PROPERTY(QVector2D trackpadValues READ trackpadValues NOTIFY trackpadValuesChanged)
    Q_PROPERTY(float trackpadForce READ trackpadForce NOTIFY trackpadForceChanged)
    Q_PROPERTY(bool trackpadTouched READ trackpadTouched NOTIFY trackpadTouchedChanged)
    Q_PROPERTY(bool trackpadPressed READ trackpadPressed NOTIFY trackpadPressedChanged)
    Q_PROPERTY(HandPoseSpace poseSpace READ poseSpace WRITE setPoseSpace NOTIFY poseSpaceChanged)

    Q_PROPERTY(QVector3D posePosition READ posePosition NOTIFY posePositionChanged)
    Q_PROPERTY(QQuaternion poseRotation READ poseRotation NOTIFY poseRotationChanged)

    QML_NAMED_ELEMENT(XrHandInput)
    QML_UNCREATABLE("Created by XrView")

public:
    enum class HandPoseSpace {
        GripPose,
        AimPose
    };
    Q_ENUM(HandPoseSpace)


    explicit QOpenXRHandInput(QObject *parent = nullptr);

    bool isActive() const;
    bool button1Pressed() const;
    bool button1Touched() const;
    bool button2Pressed() const;
    bool button2Touched() const;
    bool buttonMenuPressed() const;
    bool buttonMenuTouched() const;
    bool buttonSystemPressed() const;
    bool buttonSystemTouched() const;
    float squeezeValue() const;
    float squeezeForce() const;
    bool squeezePressed() const;
    float triggerValue() const;
    bool triggerPressed() const;
    bool triggerTouched() const;
    QVector2D thumbstickValues() const;
    bool thumbstickPressed() const;
    bool thumbstickTouched() const;
    bool thumbrestTouched() const;
    QVector2D trackpadValues() const;
    float trackpadForce() const;
    bool trackpadTouched() const;
    bool trackpadPressed() const;
    HandPoseSpace poseSpace() const;
    void setIsActive(bool isActive);
    void setButton1Pressed(bool button1Pressed);
    void setButton1Touched(bool button1Touched);
    void setButton2Pressed(bool button2Pressed);
    void setButton2Touched(bool button2Touched);
    void setButtonMenuPressed(bool buttonMenuPressed);
    void setButtonMenuTouched(bool buttonMenuTouched);
    void setButtonSystemPressed(bool buttonSystemPressed);
    void setButtonSystemTouched(bool buttonSystemTouched);
    void setSqueezeValue(float squeezeValue);
    void setSqueezeForce(float squeezeForce);
    void setSqueezePressed(bool squeezePressed);
    void setTriggerValue(float triggerValue);
    void setTriggerPressed(bool triggerPressed);
    void setTriggerTouched(bool triggerTouched);
    void setThumbstickX(float thumbstickX);
    void setThumbstickY(float thumbstickY);
    void setThumbstickValues(const QVector2D &values);
    void setThumbstickPressed(bool thumbstickPressed);
    void setThumbstickTouched(bool thumbstickTouched);
    void setThumbrestTouched(bool thumbrestTouched);
    void setTrackpadX(float trackpadX);
    void setTrackpadY(float trackpadY);
    void setTrackpadValues(const QVector2D &values);
    void setTrackpadForce(float trackpadForce);
    void setTrackpadTouched(bool trackpadTouched);
    void setTrackpadPressed(bool trackpadPressed);
    void setPosePosition(const QVector3D &position);
    void setPoseRotation(const QQuaternion &rotation);

    const QVector3D &posePosition() const;

    const QQuaternion &poseRotation() const;

public Q_SLOTS:
    void setPoseSpace(HandPoseSpace poseSpace);

Q_SIGNALS:
    void isActiveChanged();
    void button1PressedChanged();
    void button1TouchedChanged();
    void button2PressedChanged();
    void button2TouchedChanged();
    void buttonMenuPressedChanged();
    void buttonMenuTouchedChanged();
    void buttonSystemPressedChanged();
    void buttonSystemTouchedChanged();
    void squeezeValueChanged();
    void squeezeForceChanged();
    void squeezePressedChanged();
    void triggerValueChanged();
    void triggerPressedChanged();
    void triggerTouchedChanged();
    void thumbstickValuesChanged();
    void thumbstickPressedChanged();
    void thumbstickTouchedChanged();
    void thumbrestTouchedChanged();
    void trackpadValuesChanged();
    void trackpadForceChanged();
    void trackpadTouchedChanged();
    void trackpadPressedChanged();
    void poseSpaceChanged();
    void posePositionChanged();
    void poseRotationChanged();

private:
    bool m_isActive = false;
    bool m_button1Pressed = false;
    bool m_button1Touched = false;
    bool m_button2Pressed = false;
    bool m_button2Touched = false;
    bool m_buttonMenuPressed = false;
    bool m_buttonMenuTouched = false;
    bool m_buttonSystemPressed = false;
    bool m_buttonSystemTouched = false;
    float m_squeezeValue = 0.0f;
    float m_squeezeForce = 0.0f;
    bool m_squeezePressed = false;
    float m_triggerValue = 0.0f;
    bool m_triggerPressed = false;
    bool m_triggerTouched = false;
    QVector2D m_thumbstickValues;
    bool m_thumbstickPressed = false;
    bool m_thumbstickTouched = false;
    bool m_thumbrestTouched = false;
    QVector2D m_trackpadValues;
    float m_trackpadForce = 0.0f;
    bool m_trackpadTouched = false;
    bool m_trackpadPressed = false;
    HandPoseSpace m_poseSpace = HandPoseSpace::GripPose;
    QVector3D m_posePosition;
    QQuaternion m_poseRotation;
};

QT_END_NAMESPACE

#endif // QOPENXRHANDINPUT_H
