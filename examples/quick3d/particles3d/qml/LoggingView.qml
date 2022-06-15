// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick3D.Particles3D
import QtQuick.Controls

Frame {
    property var particleSystems
    readonly property bool loggingEnabled: settings.showLoggingView
    property bool intervalInstant: false
    property real itemWidth: (width - loggingButton.width - intervalButton.width) / 7

    width: parent.width
    height: tableContent.height + 30

    Component.onCompleted: {
        for (const psystem of particleSystems)
            psystem.logging = settings.showLoggingView;
    }

    // Background
    background: Rectangle {
        color: "#80000000"
        visible: loggingEnabled
    }

    Button {
        id: loggingButton
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        anchors.rightMargin: 10
        opacity: loggingEnabled ? 1.0 : 0.4
        icon.source: "qrc:/qml/images/icon_logging.png"
        icon.width: 32
        icon.height: 32
        icon.color: "transparent"
        background: Rectangle {
            color: "transparent"
        }
        onClicked: {
            settings.showLoggingView = !settings.showLoggingView
            for (const psystem of particleSystems) {
                psystem.logging = settings.showLoggingView;
            }
        }
    }

    Button {
        id: intervalButton
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: loggingButton.left
        anchors.rightMargin: 0
        visible: loggingEnabled
        opacity: intervalInstant ? 1.0 : 0.2
        icon.source: "qrc:/qml/images/icon_interval.png"
        icon.width: 32
        icon.height: 32
        icon.color: "transparent"
        background: Rectangle {
            color: "transparent"
        }
        onClicked: {
            intervalInstant = !intervalInstant;
            var interval = intervalInstant ? 0 : 1000;
            for (const psystem of particleSystems)
                psystem.loggingData.loggingInterval = interval;
        }
    }

    Component {
        id: systemItem
        Row {
            Text {
                width: itemWidth
                horizontalAlignment: Text.AlignHCenter
                color: "#ffffff"
                font.pointSize: settings.fontSizeSmall
                text: modelData.seed
            }
            Text {
                width: itemWidth
                horizontalAlignment: Text.AlignHCenter
                color: "#ffffff"
                font.pointSize: settings.fontSizeSmall
                text: modelData.loggingData.updates
            }
            Text {
                width: itemWidth
                horizontalAlignment: Text.AlignHCenter
                color: "#ffffff"
                font.pointSize: settings.fontSizeSmall
                text: modelData.loggingData.particlesMax
            }
            Text {
                width: itemWidth
                horizontalAlignment: Text.AlignHCenter
                color: "#ffffff"
                font.pointSize: settings.fontSizeSmall
                text: modelData.loggingData.particlesUsed
            }
            Text {
                width: itemWidth
                horizontalAlignment: Text.AlignHCenter
                color: "#ffffff"
                font.pointSize: settings.fontSizeSmall
                text: modelData.loggingData.time.toFixed(4)
            }
            Text {
                width: itemWidth
                horizontalAlignment: Text.AlignHCenter
                color: "#ffffff"
                font.pointSize: settings.fontSizeSmall
                text: modelData.loggingData.timeAverage.toFixed(4)
            }
            Text {
                width: itemWidth
                horizontalAlignment: Text.AlignHCenter
                color: "#ffffff"
                font.pointSize: settings.fontSizeSmall
                text: modelData.loggingData.timeDeviation.toFixed(4)
            }
        }
    }

    Column {
        id: tableContent
        width: parent.width
        anchors.verticalCenter: parent.verticalCenter
        visible: loggingEnabled
        Row {
            Text {
                width: itemWidth
                horizontalAlignment: Text.AlignHCenter
                color: "#ffffff"
                font.pointSize: settings.fontSizeSmall
                text: qsTr("SEED")
            }
            Text {
                width: itemWidth
                horizontalAlignment: Text.AlignHCenter
                color: "#ffffff"
                font.pointSize: settings.fontSizeSmall
                text: qsTr("UPDATES")
            }
            Text {
                width: itemWidth
                horizontalAlignment: Text.AlignHCenter
                color: "#ffffff"
                font.pointSize: settings.fontSizeSmall
                text: qsTr("P. MAX")
            }
            Text {
                width: itemWidth
                horizontalAlignment: Text.AlignHCenter
                color: "#ffffff"
                font.pointSize: settings.fontSizeSmall
                text: qsTr("P. USED")
            }
            Text {
                width: itemWidth
                horizontalAlignment: Text.AlignHCenter
                color: "#ffffff"
                font.pointSize: settings.fontSizeSmall
                text: qsTr("TIME")
            }
            Text {
                width: itemWidth
                horizontalAlignment: Text.AlignHCenter
                color: "#ffffff"
                font.pointSize: settings.fontSizeSmall
                text: qsTr("TIME AVG.")
            }
            Text {
                width: itemWidth
                horizontalAlignment: Text.AlignHCenter
                color: "#ffffff"
                font.pointSize: settings.fontSizeSmall
                text: qsTr("TIME DEV.")
            }
        }
        Repeater {
            model: particleSystems
            delegate: systemItem
        }
    }
}
