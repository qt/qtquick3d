// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick3D

Node {
    property color textColor: "black"
    property string textLabel: "TEXT"
    Text {
        text: textLabel
        font.pixelSize: 40
        color: textColor
    }
}
