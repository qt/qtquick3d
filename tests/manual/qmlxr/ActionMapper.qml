// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick3D
import QtQuick3D.Xr

XrActionMapper {
    id: actionMapper

    property alias triggerPressed: trigger.pressed
    property bool button1Pressed: button1.pressed || trackpad.rightPressed
    property bool button2Pressed: button2.pressed || trackpad.leftPressed

    XrInputAction {
        id: trigger
        actionId: [XrActionMapper.TriggerValue, XrActionMapper.TriggerPressed, XrActionMapper.IndexFingerPinch]
    }

    XrInputAction {
        id: button1
        actionId: [XrActionMapper.Button1Pressed, XrActionMapper.MiddleFingerPinch]
    }

    XrInputAction {
        id: button2
        actionId: [XrActionMapper.Button2Pressed, XrActionMapper.ButtonMenuPressed, XrActionMapper.RingFingerPinch]
    }

    XrInputAction {
        id: trackpadX
        actionId: [XrActionMapper.TrackpadX]
    }

    XrInputAction {
        id: trackpad
        actionId: [XrActionMapper.TrackpadPressed]
        property bool rightPressed: pressed && trackpadX > 0.1
        property bool leftPressed: pressed && trackpadX < -0.1
    }
}
