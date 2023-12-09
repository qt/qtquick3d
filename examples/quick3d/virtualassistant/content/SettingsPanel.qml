// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import VirtualAssistant.Constants

Pane {
    id: root

    topPadding: 10
    leftPadding: 40
    rightPadding: 40
    bottomPadding: 40

    required property var camera

    property alias cameraControllerEnabled: enableCamera.checked
    property alias cameraFov: fov.value
    property alias skyboxRotation: skyboxRot.value

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        Label {
            text: qsTr("Scene settings")
            font.pixelSize: 24
            Layout.alignment: Qt.AlignHCenter
        }

        CheckBox {
            id: enableCamera

            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Enable Camera Controller")
        }

        ColumnLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 5

            Label {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("Camera Fov: ") + fov.value
                font.pixelSize: 14
            }

            Slider {
                id: fov

                from: 35
                to: 125
                value: 80
                stepSize: 1
            }
        }

        ColumnLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 5

            Label {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("Skybox rotation: ") + skyboxRot.value
                font.pixelSize: 14
            }

            Slider {
                id: skyboxRot

                from: 0
                to: 360
                value: 180
                stepSize: 1
            }
        }

        ColumnLayout {
            spacing: 4
            Layout.alignment: Qt.AlignHCenter

            Label {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("Camera position: ")
            }

            Row {
                Layout.alignment: Qt.AlignHCenter
                spacing: 5
                Label {
                    text: "x: " + root.camera.x.toFixed(2)
                }
                Label {
                    text: "y: " + root.camera.y.toFixed(2)
                }
                Label {
                    text: "z: " + root.camera.z.toFixed(2)
                }
            }
        }

        Button {
            Layout.alignment: Qt.AlignHCenter

            text: qsTr("Reset default settings")
            onClicked: {
                fov.value = Constants.defaultFov;
                enableCamera.checked = false;
                skyboxRot.value = Constants.defaultRotation;
                root.camera.position = Constants.defaultCameraPosition
            }
        }

        Item {
            Layout.fillHeight: true
            Layout.fillWidth: true
        }
    }
}
