// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    property alias sliderValue: slider.value
    property alias fromValue: slider.from
    property alias toValue: slider.to
    readonly property bool highlight: slider.hovered || slider.pressed

    width: rowLayout.width
    height: rowLayout.height

    RowLayout {
        id: rowLayout
        Slider {
            id: slider
            from: fromValue
            to: toValue
            stepSize: 0.01
            Layout.minimumWidth: 200
            Layout.maximumWidth: 200
            background: Rectangle {
                x: slider.leftPadding
                y: slider.topPadding + slider.availableHeight / 2 - height / 2
                implicitWidth: 200
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
            Layout.minimumWidth: 80
            Layout.maximumWidth: 80
        }
    }
}
