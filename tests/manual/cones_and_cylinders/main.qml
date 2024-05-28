// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick3D
import QtQuick3D.Helpers

Window {
    id: window
    visible: true
    width: 1200
    height: 720
    title: qsTr("Cones and Cylinders")

    View3D {
        id: view
        anchors.fill: parent
        camera: camera1
        environment: SceneEnvironment {
            clearColor: "lightblue"
            backgroundMode: SceneEnvironment.Color
            antialiasingMode: SceneEnvironment.MSAA
            antialiasingQuality: SceneEnvironment.High
        }

        DirectionalLight {
            visible: checkBoxDirectionalLight.checked
            castsShadow: true
            shadowFactor: sliderDirectionaLightShadowFactor.value
            eulerRotation: Qt.vector3d(sliderDirectionalLightRotX.value,
                                       sliderDirectionalLightRotY.value,
                                       0)
            csmSplit1: sliderCSMSplit1.value
            csmSplit2: sliderCSMSplit2.value
            csmSplit3: sliderCSMSplit3.value
            csmNumSplits: sliderNumSplits.currentIndex
            shadowMapQuality: Light.ShadowMapQualityHigh
            csmBlendRatio: sliderBlendRatio.value
            shadowBias: sliderShadowBias.value
            shadowMapFar: sliderShadowMapFar.value
        }

        Model {
            id: ground
            source: "#Cube"
            scale: Qt.vector3d(25, 0.01, 135)
            z: -5500
            materials: DefaultMaterial {
                diffuseColor: "gray"
            }
            castsShadows: false
        }

        PerspectiveCamera {
            id: camera1
            position: Qt.vector3d(458, 300, 515)
            eulerRotation: Qt.vector3d(-14, 19, 0)
            clipFar: sliderCameraClipFar.value
        }

        Node {
            id: shapeSpawner
            Component.onCompleted: {
                var conesAndCylinderTrio = Qt.createComponent("ConesAndCylinderTrio.qml")
                var z_pos = 0
                for (var i = 0; i < 25; i++) {
                    conesAndCylinderTrio.incubateObject(shapeSpawner, {
                                                        "z_positions": [
                                                                z_pos,
                                                                z_pos - 125,
                                                                z_pos - 250
                                                            ]})
                    z_pos -= 450
                }
            }
        }
    }

    WasdController {
        controlledObject: view.camera
        speed: 5
        shiftSpeed: 10
    }

    ScrollView {
        anchors.fill: parent
        padding: 10

        component SliderWithValue : RowLayout {
            property alias value: slider.value
            property alias from: slider.from
            property alias to: slider.to
            property alias stepSize: slider.stepSize
            readonly property bool highlight: slider.hovered || slider.pressed
            Slider {
                id: slider
                stepSize: 0.01
                Layout.minimumWidth: 200
                Layout.maximumWidth: 200
            }
            Label {
                id: valueText
                text: slider.value.toFixed(3)
                Layout.minimumWidth: 80
                Layout.maximumWidth: 80
            }
        }

        ColumnLayout {
            id: contentLayout

            Label {
                text: "Settings"
                font.pointSize: 20
            }

            Label {
                // spacer
            }

            Label {
                text: "Camera"
                font.pointSize: 15
            }

            Label {
                text: "Clip Far"
            }
            SliderWithValue {
                id: sliderCameraClipFar
                value: 15000
                from: 0
                to: 30000
            }

            Label {
                text: "Shadowmap far"
                font.pointSize: 12
            }
            SliderWithValue {
                id: sliderShadowMapFar
                value: 5000
                from: 0
                to: 30000
            }

            Label {
                // spacer
            }

            Label {
                text: "Directional light"
                font.pointSize: 15
            }

            CheckBox {
                id: checkBoxDirectionalLight
                text: "Visible"
                checked: true
            }
            Label {
                text: "EulerRotation XY"
                font.pointSize: 12
            }
            SliderWithValue {
                id: sliderDirectionalLightRotX
                value: -40
                from: -180
                to: 180
                enabled: checkBoxDirectionalLight.checked
            }
            SliderWithValue {
                id: sliderDirectionalLightRotY
                value: -120
                from: -180
                to: 180
                enabled: checkBoxDirectionalLight.checked
            }
            Label {
                text: "Shadow Factor"
                font.pointSize: 12
            }
            SliderWithValue {
                id: sliderDirectionaLightShadowFactor
                value: 100
                from: 0
                to: 100
                enabled: checkBoxDirectionalLight.checked
            }
            Label {
                text: "Num Splits"
                font.pointSize: 12
            }
            ComboBox {
                id: sliderNumSplits
                model: ListModel {
                    id: model
                    ListElement { text: "0" }
                    ListElement { text: "1" }
                    ListElement { text: "2" }
                    ListElement { text: "3" }
                }
                enabled: checkBoxDirectionalLight.checked
            }
            Label {
                text: "CSM Split 1"
                font.pointSize: 12
            }
            SliderWithValue {
                id: sliderCSMSplit1
                value: 0.25
                from: 0.01
                to: 0.99
                enabled: checkBoxDirectionalLight.checked
            }
            Label {
                text: "CSM Split 2"
                font.pointSize: 12
            }
            SliderWithValue {
                id: sliderCSMSplit2
                value: 0.5
                from: 0.01
                to: 0.99
                enabled: checkBoxDirectionalLight.checked
            }
            Label {
                text: "CSM Split 3"
                font.pointSize: 12
            }
            SliderWithValue {
                id: sliderCSMSplit3
                value: 0.75
                from: 0.01
                to: 0.99
                enabled: checkBoxDirectionalLight.checked
            }
            Label {
                text: "Blend ratio"
                font.pointSize: 12
            }
            SliderWithValue {
                id: sliderBlendRatio
                value: 0.05
                from: 0
                to: 1
                enabled: checkBoxDirectionalLight.checked
            }
            Label {
                text: "Shadow bias"
                font.pointSize: 12
            }
            SliderWithValue {
                id: sliderShadowBias
                value: 0.001
                from: -0.01
                to: 0.01
                stepSize: 0.001
                enabled: checkBoxDirectionalLight.checked
            }
        }
    }

    DebugView {
        anchors.top: parent.top
        anchors.right: parent.right
        source: view
        visible: true
    }
}
