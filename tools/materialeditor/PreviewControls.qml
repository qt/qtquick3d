// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts
import QtCore
import QtQuick3D

Item {
    id: previewControls
    required property View3D targetView
    property alias modelSource: modelComboBox.currentValue
    property alias enableIBL: iblEnableButton.checked
    property alias enableDirectionalLight: directionalLightEnabledButton.checked


    Settings {
        property alias enableIbl: previewControls.enableIBL
        property alias enableDirectionalLight: previewControls.enableDirectionalLight
        property alias environmentOrientationSliderValue: environmentOrientationSlider.value
    }

    FrostedGlass {
        width: parent.width
        height: layout.implicitHeight
        backgroundItem: targetView
        backgroundRect: Qt.rect(0, 0, width, height)
//        range: 0.05
//        blur: 0.005
        range: 0.05
        blur: 0.05
        //color: "pink"
    }

    RowLayout {
        id: layout
        anchors.left: parent.left
        anchors.leftMargin: 10
        Label {
            text: "Model"
        }
        ComboBox {
            id: modelComboBox
            textRole: "text"
            valueRole: "value"
            model: ListModel {
                ListElement {
                    text: "Sphere"
                    value: "#Sphere"
                }
                ListElement {
                    text: "Cube"
                    value: "#Cube"
                }
                ListElement {
                    text: "Plane"
                    value: "#Rectangle"
                }
                ListElement {
                    text: "Suzanne"
                    value: "assets/meshes/suzanne.mesh"
                }
            }
        }
        Button {
            text: "Reset View"
            onClicked: {
                targetView.cameraOrigin.rotation = Qt.quaternion(1, 0, 0, 0)
                targetView.camera.rotation = Qt.quaternion(1, 0, 0, 0)
                targetView.camera.position = Qt.vector3d(0, 0, 300)
                environmentOrientationSlider.value = 0
            }
        }
        ToolButton {
            id: iblEnableButton
            icon.source: "assets/icons/texture.png"
            checkable: true
            checked: true
            hoverEnabled: true
            ToolTip.delay: 1000
            ToolTip.timeout: 5000
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Toggle the use of IBL")
        }

        Label {
            visible: enableIBL
            text: "Environment Orientation"
        }
        Slider {
            visible: enableIBL
            id: environmentOrientationSlider
            Layout.fillWidth: true
            from: -180
            to: 180
            value: 0
            onValueChanged: {
                targetView.environment.probeOrientation = Qt.vector3d(0, value, 0)
            }
        }
        ToolButton {
            id: directionalLightEnabledButton
            icon.source: "assets/icons/lightdirectional.png"
            checkable: true
            checked: true
            hoverEnabled: true
            ToolTip.delay: 1000
            ToolTip.timeout: 5000
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Toggle a Directional Light")
        }
    }
}
