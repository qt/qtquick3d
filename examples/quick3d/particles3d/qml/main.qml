// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick3D
import QtQuick3D.Particles3D
import QtQuick.Controls

Window {
    id: rootWindow

    readonly property url startupView: "StartupView.qml"

    QtObject {
        id: settings
        // Antialiasing mode & quality used in all examples.
        property var antialiasingMode: SceneEnvironment.NoAA
        property var antialiasingQuality: SceneEnvironment.High
        // Toggle default visibility of these views
        property bool showSettingsView: true
        property bool showLoggingView: false
        // Fonts in pointSizes
        // These are used mostly on examples in 3D side
        property real fontSizeLarge: 16
        // These are used mostly on settings
        property real fontSizeSmall: 10
    }

    readonly property real iconSize: 16 + Math.max(width, height) * 0.05

    width: 1280
    height: 720
    visible: true
    title: qsTr("Qt Quick 3D Particles3D Testbed")
    color: "#000000"

    Loader {
        id: loader
        anchors.fill: parent
        source: startupView
    }

    Button {
        id: backButton
        anchors.left: parent.left
        anchors.top: parent.top
        width: rootWindow.iconSize
        height: width
        opacity: loader.source !== startupView
        visible: opacity
        icon.source: "qrc:/qml/images/arrow_icon.png"
        icon.width: backButton.width * 0.3
        icon.height: backButton.height * 0.3
        icon.color: "transparent"
        background: Rectangle {
            color: "transparent"
        }
        onClicked: {
            loader.source = startupView;
        }
        Behavior on opacity {
            NumberAnimation {
                duration: 400
                easing.type: Easing.InOutQuad
            }
        }
    }

}
