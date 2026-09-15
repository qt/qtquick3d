// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    width: settingsDrawer.width
    height: parent.height
    property alias drawerVisible: settingsDrawer.visible
    property bool drawerOpen: settingsDrawer.position === 1.0

    property real fogDensity: fogDensitySlider.value
    property real fogScale: fogScaleSlider.value
    property real fogSpeed: fogSpeedSlider.value
    property real quality: qualitySlider.value
    property real cameraFar : cameraFarSlider.value

    property real viewX: settingsDrawer.visible ? (settingsDrawer.x + settingsDrawer.width) : 0

    RoundButton {
        id: iconOpen
        icon.width: 25
        icon.height: 25
        padding: 10
        x: padding + root.viewX
        y: padding
        onClicked: {
            settingsDrawer.visible = !settingsDrawer.visible;
        }
    }

    Drawer {
        id: settingsDrawer
        edge: Qt.LeftEdge
        interactive: false
        modal: false
        height: parent.height

        enter: Transition {
            NumberAnimation {
                property: "position"
                to: 1.0
                duration: 400
                easing.type: Easing.InOutQuad
            }
        }

        exit: Transition {
            NumberAnimation {
                property: "position"
                to: 0.0
                duration: 400
                easing.type: Easing.InOutQuad
            }
        }

        Page {
            anchors.fill: parent

            header: ToolBar {
                width: parent.width
                Label {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "Settings"
                    font.pointSize: 17
                }
            }

            ScrollView {
                anchors.fill: parent
                ScrollBar.vertical.policy: ScrollBar.AlwaysOn
                padding: 10
                contentHeight: settingsArea.implicitHeight

                ColumnLayout {
                    id: settingsArea
                    width: parent.width
                    spacing: 5

                    component SliderWithValue : RowLayout {
                        id: sliderWithValue
                        property alias value: slider.value
                        property alias from: slider.from
                        property alias to: slider.to
                        property alias stepSize: slider.stepSize
                        property int numDecimals: 3
                        readonly property bool highlight: slider.hovered || slider.pressed
                        Slider {
                            id: slider
                            stepSize: 0.01
                            Layout.minimumWidth: 200
                            Layout.maximumWidth: 200
                        }
                        Label {
                            id: valueText
                            text: slider.value.toFixed(sliderWithValue.numDecimals)
                            Layout.minimumWidth: 80
                            Layout.maximumWidth: 80
                        }
                    }

                    Label {
                        text: "Fog density"
                    }

                    SliderWithValue {
                        id: fogDensitySlider
                        value: 1
                        from: 0
                        to: 1
                    }
                    Label {
                        text: "Fog scale"
                    }

                    SliderWithValue {
                        id: fogScaleSlider
                        value: 0.5
                        from: 0
                        to: 1
                    }
                    Label {
                        text: "Fog speed"
                    }

                    SliderWithValue {
                        id: fogSpeedSlider
                        value: 1
                        from: 0
                        to: 1
                    }
                    Label {
                        text: "Quality"
                    }

                    SliderWithValue {
                        id: qualitySlider
                        value: 0.5
                        from: 0
                        to: 1
                    }
                    Label {
                        text: "Camera Far"
                    }

                    SliderWithValue {
                        id: cameraFarSlider
                        value: 2000
                        from: 1000
                        to: 5000
                    }
                }
            }
        }
    }
}

