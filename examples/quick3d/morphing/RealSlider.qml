// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Slider {
    property string description
    property int precision: 2
    from: 0.0
    to: 1.0
    value: 0.0
    Layout.fillWidth: true
    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.verticalCenter
        anchors.topMargin: 6
        text: (parent.description.length == 0 ? "" : parent.description + ": ")
                   + parent.value.toFixed(precision);
        z: 10
    }
}
