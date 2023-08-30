// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Rectangle {
    width: 256
    height: 256
    color: "white"
    Image {
        anchors.centerIn: parent
        width: 200
        height: 200
        source: "qtlogo.png"
        NumberAnimation on rotation {
            to: 45
            duration: 200
            easing.type: Easing.InOutQuad
        }
    }
}
