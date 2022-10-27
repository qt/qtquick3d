// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    property string title: ""
    property bool isExpanded: true
    default property alias content: container.children
    color: "transparent"
    border.width: 2
    border.color: palette.alternateBase

    Layout.fillWidth: true
    height: outsideLayout.implicitHeight
    implicitHeight: height

    ColumnLayout {
        id: outsideLayout
        width: parent.width
        Rectangle {
            id: sectionHeader
            Layout.fillWidth: true
            height: 24
            color: palette.alternateBase

            RowLayout {
                anchors.verticalCenter: parent.verticalCenter
                Image {
                    fillMode: Image.PreserveAspectFit
                    sourceSize: Qt.size(titleText.implicitHeight, titleText.implicitHeight)
                    source: root.isExpanded ? "qrc:/images/TreeExpanded.png" : "qrc:/images/TreeUnexpanded.png"
                }

                Text {
                    id: titleText
                    Layout.fillWidth: true
                    text: root.title
                }
            }

            MouseArea {
                anchors.fill: parent
                onClicked: root.isExpanded = !root.isExpanded
            }
        }

        ColumnLayout {
            id: container
            Layout.margins: root.border.width * 2
            visible: root.isExpanded
            Layout.fillWidth: true
        }

    }
}
