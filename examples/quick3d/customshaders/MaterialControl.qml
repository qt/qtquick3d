// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    property real time: time.sliderValue
    property real amplitude: amplitude.sliderValue
    property real alpha: alpha.sliderValue
    property bool animateRotation: animControl.checkBoxSet
    property bool texturing: texControl.checkBoxSet
    property bool textureFromItem : texControl2.checkBoxSet

    color: "#6b7080"
    width: parent.width
    height: 75

    Component {
        id: propertySlider
        RowLayout {
            Label {
                id: propText
                text: name
                color: "#222840"
                font.pointSize: 12
            }
            Slider {
                id: slider
                from: fromValue
                to: toValue
                value: sliderValue
                stepSize: 0.01
                onValueChanged: sliderValue = value
                Layout.minimumWidth: 100
                Layout.maximumWidth: 200
                background: Rectangle {
                    x: slider.leftPadding
                    y: slider.topPadding + slider.availableHeight / 2 - height / 2
                    implicitWidth: 120
                    implicitHeight: 4
                    width: slider.availableWidth
                    height: implicitHeight
                    radius: 1
                    color: "#222840"

                    Rectangle {
                        width: slider.visualPosition * parent.width
                        height: parent.height
                        color: "#848895"
                        radius: 1
                    }
                }
                handle: Rectangle {
                    x: slider.leftPadding + slider.visualPosition * (slider.availableWidth - width)
                    y: slider.topPadding + slider.availableHeight / 2 - height / 2
                    implicitWidth: 20
                    implicitHeight: 20
                    radius: 5
                    color: slider.pressed ? "#222840" : "#6b7080"
                    border.color: "#848895"
                }
            }
            Label {
                id: valueText
                text: slider.value.toFixed(2)
                color: "#222840"
                font.pointSize: 12
                font.bold: true
                Layout.minimumWidth: 40
                Layout.maximumWidth: 40
            }
        }
    }

    Component {
        id: propertyCheckBox
        RowLayout {
            Label {
                text: checkBoxText
                font.pointSize: 12
                font.bold: true
            }
            CheckBox {
                checked: false
                onCheckedChanged: checkBoxSet = checked
            }
        }
    }

    ScrollBar {
        id: hbar
        policy: parent.width < columnLayout.width ? ScrollBar.AlwaysOn : ScrollBar.AsNeeded
        orientation: Qt.Horizontal
        size: parent.width / columnLayout.width
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
    }

    ColumnLayout {
        id: columnLayout
        readonly property int leftMargin: 25
        x: leftMargin - hbar.position * width

        RowLayout {
            spacing: 10
            Loader {
                id: time
                property real sliderValue: 0.0
                property string name: "Time"
                property real fromValue: 0.0
                property real toValue: 1.0
                sourceComponent: propertySlider
            }
            Loader {
                id: amplitude
                property real sliderValue: 5.0
                property string name: "Amplitude"
                property real fromValue: 1.0
                property real toValue: 20.0
                sourceComponent: propertySlider
            }
            Loader {
                id: alpha
                property real sliderValue: 1.0
                property string name: "Alpha"
                property real fromValue: 0.0
                property real toValue: 1.0
                sourceComponent: propertySlider
            }
            Loader {
                id: animControl
                property string checkBoxText: "Rotate"
                property bool checkBoxSet: false
                sourceComponent: propertyCheckBox
            }
            Loader {
                id: texControl
                property string checkBoxText: "Texture"
                property bool checkBoxSet: false
                sourceComponent: propertyCheckBox
            }
            Layout.rightMargin: texControl.checkBoxSet ? 0 : texControl2.width + 10
            Loader {
                id: texControl2
                visible: texControl.checkBoxSet
                property string checkBoxText: "Texture with Item"
                property bool checkBoxSet: false
                sourceComponent: texControl.checkBoxSet ? propertyCheckBox : undefined
            }
        }
    }
}
