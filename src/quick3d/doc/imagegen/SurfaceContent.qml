// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

// Stand-in for whatever 2D user interface an application puts on an XrItem.
// Sized and laid out in pixels, with no knowledge of the surface it lands on.
Column {
    spacing: 16

    Text {
        text: "contentItem"
        font { pixelSize: 40; bold: true; family: "Segoe UI" }
        color: "#17181a"
    }
    Text {
        text: "an ordinary Qt Quick Item,\n500 x 300 pixels"
        font { pixelSize: 26; family: "Segoe UI" }
        color: "#5c6066"
        lineHeight: 1.2
    }
    Rectangle {
        width: 210
        height: 54
        radius: 8
        color: "#41cd52"
        Text {
            anchors.centerIn: parent
            text: "Button"
            font { pixelSize: 26; bold: true; family: "Segoe UI" }
            color: "white"
        }
    }
}
