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

QT_BEGIN_NAMESPACE

class QOpenXRController;

class QOpenXRActionMapper : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    QML_NAMED_ELEMENT(XrActionMapper)
public:
    explicit QOpenXRActionMapper(QObject *parent = nullptr);

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
        NumActions
    };
    Q_ENUM(InputAction)

Q_SIGNALS:
    void inputValueChange(InputAction id, QString shortName, float value);

    // QQmlParserStatus interface
public:
    void classBegin() override;
    void componentComplete() override;

private:
    QOpenXRController *m_controller = nullptr;
    friend class QOpenXRController;
};

QT_END_NAMESPACE

#endif // QOPENXRACTIONMAPPER_H
