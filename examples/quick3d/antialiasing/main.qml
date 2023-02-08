// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers
import QtQuick.Controls
import QtQuick.Layouts

Window {
    visible: true
    width: 800
    height: 600
    title: qsTr("Quick3D Antialiasing Example")
    color: "black"

    View3D {
        id: view3D
        property real animationValue: 0.0
        anchors.fill: parent

        SequentialAnimation on animationValue {
            id: modelAnimation
            running: false
            NumberAnimation {
                from: 0.0
                to: 1.0
                duration: 1000
                easing.type: Easing.InOutQuad
            }
            NumberAnimation {
                from: 1.0
                to: 0.0
                duration: 1000
                easing.type: Easing.InOutQuad
            }
        }

        PerspectiveCamera {
            z: 500
        }

        DirectionalLight {
            eulerRotation.x: -30
        }
   //! [scene environment]
        environment: SceneEnvironment {
            id: sceneEnvironment
            clearColor: "#f0f0f0"
            backgroundMode: SceneEnvironment.Color

            antialiasingMode: modeButton1.checked ? SceneEnvironment.NoAA : modeButton2.checked
                                                    ? SceneEnvironment.SSAA : modeButton3.checked
                                                      ? SceneEnvironment.MSAA : SceneEnvironment.ProgressiveAA

            antialiasingQuality: qualityButton1.checked ? SceneEnvironment.Medium : qualityButton2.checked
                                                          ? SceneEnvironment.High : SceneEnvironment.VeryHigh
            temporalAAEnabled: temporalModeButton.checked
            temporalAAStrength: temporalStrengthSlider.value
        }
   //! [scene environment]

        Node {
            id: scene

            Model {
                source: "#Cube"
                eulerRotation.y: 45
                eulerRotation.x: 30 + view3D.animationValue * 100
                scale: Qt.vector3d(2, 2, 2)
                materials: DefaultMaterial {
                    diffuseColor: "#4aee45"
                }
            }

            Model {
                source: "#Cube"
                x: 200
                y: 150 + view3D.animationValue * 10
                eulerRotation.y: 5
                eulerRotation.x: 5
                scale: Qt.vector3d(0.5, 0.5, 0.5)
                materials: DefaultMaterial {
                    diffuseColor: "#faee45"
                }
            }

            Model {
                source: "#Sphere"
                x: 120
                y: -40
                z: 160 + view3D.animationValue * 40
                scale: Qt.vector3d(1.5, 1.5, 1.5)
                materials: DefaultMaterial {
                    diffuseColor: Qt.rgba(0.8, 0.8, 0.8, 1.0)
                }
            }
        }
    }


    Frame {
        background: Rectangle {
            color: "#c0c0c0"
            border.color: "#202020"
        }
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 10
        ColumnLayout {
            id: settingsArea
            Text {
                Layout.alignment: Qt.AlignHCenter
                font.bold: true
                text: "antialiasingMode"
            }
            RadioButton {
                id: modeButton1
                checked: true
                text: qsTr("NoAA")
            }
            RadioButton {
                id: modeButton2
                text: qsTr("SSAA")
            }
            RadioButton {
                id: modeButton3
                text: qsTr("MSAA")
            }
            RadioButton {
                id: modeButton4
                text: qsTr("ProgressiveAA")
            }
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: "#909090"
            }
            Text {
                Layout.alignment: Qt.AlignHCenter
                font.bold: true
                text: "antialiasingQuality"
            }
            ButtonGroup {
                buttons: antialiasingQualityColumn.children
            }
            ColumnLayout {
                id: antialiasingQualityColumn
                RadioButton {
                    id: qualityButton1
                    text: qsTr("Medium")
                }
                RadioButton {
                    id: qualityButton2
                    checked: true
                    text: qsTr("High")
                }
                RadioButton {
                    id: qualityButton3
                    text: qsTr("VeryHigh")
                }
            }
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: "#909090"
            }
            CheckBox {
                id: temporalModeButton
                text: "temporalAAEnabled"
            }
            Item { width: 1; height: 10 }
            Slider {
                id: temporalStrengthSlider
                from: 0.0
                to: 2.0
                value: 0.3
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.bottom: parent.verticalCenter
                    anchors.bottomMargin: 16
                    text: "temporalAAStrength: " + temporalStrengthSlider.value.toFixed(2);
                    z: 10
                }
            }
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: "#909090"
            }
            Button {
                id: animationButton
                Layout.alignment: Qt.AlignHCenter
                text: "Animate!"
                onClicked: {
                    modelAnimation.restart();
                }
            }
        }
    }

    Item {
        width: debugViewToggleText.implicitWidth
        height: debugViewToggleText.implicitHeight
        anchors.right: parent.right
        Label {
            id: debugViewToggleText
            text: "Click here " + (dbg.visible ? "to hide DebugView" : "for DebugView")
            anchors.right: parent.right
            anchors.top: parent.top
        }
        MouseArea {
            anchors.fill: parent
            onClicked: dbg.visible = !dbg.visible
            DebugView {
                y: debugViewToggleText.height * 2
                anchors.right: parent.right
                source: view3D
                id: dbg
                visible: false
            }
        }
    }
}
