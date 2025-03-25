// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Pane {
    id: root
    anchors.fill: parent

    property double totalProgress: 0
    property double totalTimeRemaining: 0
    property double totalTimeElapsed: 0

    function clearText() {
        textArea.clear();
    }

    function update(payload) {
        if (payload["message"]) {
            textArea.insert(textArea.length, payload["message"] + "\n")
        }
        if (payload["totalProgress"]) {
            root.totalProgress = payload["totalProgress"];
        }
        if (payload["totalTimeRemaining"]) {
            root.totalTimeRemaining = payload["totalTimeRemaining"];
        }
        if (payload["totalTimeElapsed"]) {
            root.totalTimeElapsed = payload["totalTimeElapsed"]
        }
    }

    function formatDuration(milliseconds, showMilliseconds = true) {
        if (milliseconds < 0)
            return " Estimating..."
        const partSeconds = Math.floor(milliseconds / 1000) % 60;
        const partMinutes = Math.floor(milliseconds / 60000) % 60;
        const partHours = Math.floor(milliseconds / 3600000) % 60;

        if (partHours > 0) {
            return partHours + "h " + partMinutes + "m " + partSeconds + "s";
        }
        if (partMinutes > 0) {
            return partMinutes + "m " + partSeconds + "s";
        }
        if (partSeconds > 0) {
            return partSeconds + "s";
        }
        return "0s";
    }

    ColumnLayout {
        anchors.fill: parent

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            TextArea {
                id: textArea
                readOnly: true
                placeholderText: qsTr("Qt Lightmapper")
                font.pixelSize: 12
                wrapMode: Text.WordWrap
            }
        }

        ProgressBar {
            Layout.fillWidth: true
            Layout.preferredHeight: 25
            value: root.totalProgress
        }

        RowLayout {
            //Layout.fillWidth: true
            Item {
                Layout.preferredWidth: 10
            }
            Label {
                text: (root.totalProgress * 100).toFixed(0) + "% complete"
            }
            Item {
                Layout.fillWidth: true
            }
            Label {
                text: "Remaining: " + root.formatDuration(root.totalTimeRemaining)
            }
            Item {
                Layout.fillWidth: true
            }
            Label {
                Layout.alignment: Qt.AlignRight
                text: "Elapsed: " + root.formatDuration(root.totalTimeElapsed)
            }
            Item {
                Layout.preferredWidth: 10
            }
        }

        Button {
            objectName: "cancelButton"
            Layout.fillWidth: true
            text: "Cancel"
        }
    }
}
