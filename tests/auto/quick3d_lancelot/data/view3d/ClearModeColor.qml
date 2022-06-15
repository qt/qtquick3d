// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick3D

Rectangle {
    width: 320
    height: 480
    color: "blue"

    View3D {
        anchors.fill: parent

        environment: SceneEnvironment {
            clearColor: "#848895"
            backgroundMode: SceneEnvironment.Color
        }
        PerspectiveCamera {
            id: dummyCamera
        }
    }
}
