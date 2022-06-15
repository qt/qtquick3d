// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    property alias text: propText.text
    property alias checked: checkBox.checked

    width: rowLayout.width
    height: rowLayout.height

    RowLayout {
        id: rowLayout
        CheckBox {
            id: checkBox
            indicator: Rectangle {
                anchors.centerIn: parent
                implicitWidth: 20
                implicitHeight: 20
                radius: 5
                color: checkBox.pressed ? "#222840" : "#6b7080"
                border.color: "#848895"
                Rectangle {
                    anchors.centerIn: parent
                    implicitWidth: 12
                    implicitHeight: 12
                    radius: 4
                    color: "#ffffff"
                    border.color: "#848895"
                    visible: checkBox.checked
                }
            }
        }
        Label {
            id: propText
            color: "#222840"
            font.pointSize: 12
            Layout.minimumWidth: 150
            Layout.maximumWidth: 150
            opacity: checkBox.checked ? 1.0 : 0.5
        }
    }
}
