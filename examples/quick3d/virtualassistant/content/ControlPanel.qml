// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Pane {
    id: root

    topPadding: 10
    leftPadding: 40
    rightPadding: 40
    bottomPadding: 40

    readonly property list<string> animations: [qsTr("Entry Animation"), qsTr("Backflip"),
        qsTr("Low Body Bouncing"), qsTr("Right hand waving"),
        qsTr("Left hand waving"), qsTr("Exploring scene"),
        qsTr("Exit Animation"), qsTr("Face Animation")]

    signal clicked(index: int)

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        Label {
            text: qsTr("Select animation:")
            font.pixelSize: 24
            Layout.alignment: Qt.AlignHCenter
        }

        Repeater {
            model: root.animations
            Button {
                required property int index
                required property string modelData

                Layout.fillHeight: true
                Layout.fillWidth: true

                text: modelData
                onClicked: root.clicked(index)
            }
        }
    }
}
