// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick3D
import QtQuick3D.Xr

XrController {
    id: root
    property Item view3d
    controller: XrController.ControllerRight
    handInput.poseSpace: XrHandInput.AimPose
    Lazer {
        enableBeam: true
    }
    Node {
        x: 10
        y: 10
        Rectangle {
            radius: 8
            color: "lightGray"
            width: 50
            height: 30
            Text {
                font.pointSize: 8
                text: root.view3d.renderStats.fps + " FPS"
                anchors.centerIn: parent
            }
        }
    }
}
