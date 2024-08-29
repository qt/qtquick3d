// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick3D
import QtQuick3D.Helpers
import QtQuick3D.Xr

XrView {
    id: xrView

    XrErrorDialog { id: err }
    onInitializeFailed: (errorString) => err.run("XRView", errorString)
    referenceSpace: XrView.ReferenceSpaceLocal

    environment: SceneEnvironment {
        clearColor: "black"
        backgroundMode: SceneEnvironment.Color
    }

    XrOrigin {
        objectName: "xrorigin"
        XrController {
            id: leftController
            controller: XrController.ControllerLeft
            Component.onCompleted: handInput.poseSpace = XrHandInput.AimPose
            Lazer {
                enableBeam: true
            }
            Node {
                y: rect.height
                x: -rect.width / 2
                Rectangle {
                    id: rect
                    opacity: 0.7
                    width: uiLayout.implicitWidth + 4
                    height: uiLayout.implicitHeight + 4
                    color: "white"
                    radius: 2
                    ColumnLayout {
                        id: uiLayout
                        spacing: 0
                        anchors.fill: parent
                        anchors.margins: 2
                        Text {
                            text: xrView.runtimeInfo.runtimeName + " " + xrView.runtimeInfo.runtimeVersion + "\n" + xrView.runtimeInfo.graphicsApiName
                            font.pixelSize: 2
                            color: "black"
                        }
                        Text {
                            visible: xrView.multiViewRenderingEnabled
                            text: "Multiview rendering enabled"
                            font.pixelSize: 2
                            color: "green"
                        }
                    }
                }
            }
        }
        XrController {
            id: rightController
            controller: XrController.ControllerRight
            Component.onCompleted: handInput.poseSpace = XrHandInput.AimPose
            ActionMapper { }
            Lazer {
                enableBeam: true
            }
        }
    }

    XrVirtualMouse {
        view: xrView
        source: rightController
        enabled: true
        leftMouseButton: rightController.actionMapper.triggerPressed
        rightMouseButton: rightController.actionMapper.button1Pressed
        middleMouseButton: rightController.actionMapper.button2Pressed
    }
}
