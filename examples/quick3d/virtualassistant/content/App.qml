// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import VirtualAssistant.Constants

Window {
    minimumWidth: Constants.width
    minimumHeight: Constants.height

    visible: true
    title: "Qt Robot Assistant"

    Screen01 {
        id: mainScreen

        anchors.fill: parent
    }
}
