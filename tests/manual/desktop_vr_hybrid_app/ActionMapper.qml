// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick3D
import QtQuick3D.Xr

XrActionMapper {
    id: actionMapper

    property bool triggerPressed: false
    property bool gripPressed: false
    property real trackpadY: 0
    property real trackpadX: 0
    property bool trackpadPressed: false
    property bool button1Pressed: false
    property bool button2Pressed: false

    onInputValueChange: (id, name, value) => {
        switch (id) {
            case XrActionMapper.TrackpadY:
                trackpadY = value
            break
            case XrActionMapper.TrackpadX:
                trackpadX = value
            break
            case XrActionMapper.TrackpadPressed:
                trackpadPressed = value > 0.8
            break
            case XrActionMapper.TriggerValue:
            case XrActionMapper.TriggerPressed:
                triggerPressed = value > 0.8
            break
            case XrActionMapper.SqueezeValue:
            case XrActionMapper.SqueezePressed:
                gripPressed = value > 0.8
            break
            case XrActionMapper.Button1Pressed:
            case XrActionMapper.ButtonMenuPressed:
                button1Pressed = value > 0.8
            break
            case XrActionMapper.Button2Pressed:
                button2Pressed = value > 0.8
            break
        }
    }

    onTrackpadPressedChanged: {
        button1Pressed = trackpadPressed && trackpadX < -0.1
        button2Pressed = trackpadPressed && trackpadX > 0.1
    }
}
