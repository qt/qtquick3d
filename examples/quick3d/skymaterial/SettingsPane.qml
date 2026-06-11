// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: root

    required property var sun
    required property var advancedSky
    required property var sunAnimator

    property bool animateSun: true
    property real sunSweepSpeed: 0.4

    header: ToolBar {
        Label {
            anchors.centerIn: parent
            text: "Settings"
            font.pointSize: 17
        }
    }

    ScrollView {
        id: scrollView
        anchors.fill: parent
        ScrollBar.vertical.policy: ScrollBar.AsNeeded
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        padding: 20
        contentHeight: settingsColumn.implicitHeight
        contentWidth: skyGroup.implicitWidth

        ColumnLayout {
            id: settingsColumn
            width: parent.width
            spacing: 8

            component SliderWithValue : RowLayout {
                id: sliderWithValue
                property alias value: slider.value
                property alias from: slider.from
                property alias to: slider.to
                property alias stepSize: slider.stepSize
                property alias snapMode: slider.snapMode
                property int numDecimals: 0
                signal moved(real v)
                Slider {
                    id: slider
                    onMoved: sliderWithValue.moved(slider.value)
                }
                Label {
                    text: slider.value.toFixed(sliderWithValue.numDecimals)
                    Layout.minimumWidth: 40
                    horizontalAlignment: Text.AlignRight
                }
            }

            GroupBox {
                id: skyGroup
                Layout.fillWidth: true
                title: "Sky"

                ColumnLayout {
                    width: parent.width
                    spacing: 5

                    Switch {
                        Layout.fillWidth: true
                        text: "Animate sun"
                        checked: root.animateSun
                        onToggled: root.animateSun = checked
                    }

                    Label { text: "Cycle speed" }
                    SliderWithValue {
                        from: 0.01; to: 1.0; stepSize: 0.01; snapMode: Slider.SnapAlways
                        numDecimals: 2
                        value: root.sunSweepSpeed
                        onMoved: (v) => root.sunSweepSpeed = v
                    }

                    Label {
                        text: "Sun elevation"
                    }
                    SliderWithValue {
                        from: 0; to: 180; stepSize: 1; snapMode: Slider.SnapAlways
                        value: -root.sun.eulerRotation.x
                        onMoved: (v) => {
                            root.sun.eulerRotation.x = -v
                            root.sunAnimator.updatePhaseFromSun()
                        }
                    }

                    Switch {
                        Layout.fillWidth: true
                        text: "Enable clouds"
                        checked: root.advancedSky.cloudsEnabled
                        onToggled: root.advancedSky.cloudsEnabled = checked
                    }

                    Label {
                        text: "Cloud coverage"
                        enabled: root.advancedSky.cloudsEnabled
                    }
                    SliderWithValue {
                        enabled: root.advancedSky.cloudsEnabled
                        from: 0.0; to: 1.0; stepSize: 0.01
                        numDecimals: 2
                        value: root.advancedSky.cloudCoverage
                        onMoved: (v) => root.advancedSky.cloudCoverage = v
                    }

                    Switch {
                        id: animateWindSwitch
                        Layout.fillWidth: true
                        text: "Animate wind"
                        checked: true
                        enabled: root.advancedSky.cloudsEnabled
                    }
                }
            }

            FrameAnimation {
                running: animateWindSwitch.checked && root.advancedSky.cloudsEnabled
                onTriggered: {
                    const speed = 0.1
                    const off = root.advancedSky.cloudWindOffset
                    root.advancedSky.cloudWindOffset = Qt.vector2d(
                        off.x + speed * frameTime,
                        off.y + speed * frameTime * 0.3)
                    root.advancedSky.cloudTimeOffset += speed * frameTime * 0.05
                }
            }

            GroupBox {
                Layout.fillWidth: true
                title: "Rendering"

                ColumnLayout {
                    width: parent.width
                    spacing: 5

                    Label {
                        text: "IBL render frames"
                        enabled: root.advancedSky.enableIBL
                    }
                    SliderWithValue {
                        enabled: root.advancedSky.enableIBL
                        from: 0; to: 6; stepSize: 1; snapMode: Slider.SnapAlways
                        value: root.advancedSky.iblRenderFrames
                        onMoved: (v) => root.advancedSky.iblRenderFrames = Math.round(v)
                    }

                    Switch {
                        Layout.fillWidth: true
                        text: "Enable IBL"
                        checked: root.advancedSky.enableIBL
                        onToggled: root.advancedSky.enableIBL = checked
                    }

                    Label {
                        text: "IBL sample count"
                        enabled: root.advancedSky.enableIBL
                    }
                    SliderWithValue {
                        enabled: root.advancedSky.enableIBL
                        from: 1; to: 256; stepSize: 1; snapMode: Slider.SnapAlways
                        value: root.advancedSky.iblSampleCount
                        onMoved: (v) => root.advancedSky.iblSampleCount = Math.round(v)
                    }

                    Label { text: "Radiance map size" }
                    ComboBox {
                        Layout.fillWidth: true
                        model: [
                            { value: 256, text: "256" },
                            { value: 512, text: "512" },
                            { value: 1024, text: "1024" },
                        ]
                        textRole: "text"
                        valueRole: "value"
                        currentValue: 512
                        onActivated: root.advancedSky.radianceMapSize = currentValue
                    }
                }
            }
        }
    }
}
