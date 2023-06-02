// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick3D

//! [teapot model]
Model {
    id: teapotDefault
    source: "teapot.mesh"
    y: -100
    scale: Qt.vector3d(75, 75, 75)

    property variant material
    materials: [ material ]

    property bool animate: true
    NumberAnimation on eulerRotation.y {
        running: teapotDefault.animate
        loops: Animation.Infinite
        duration: 5000
        from: 0
        to: -360
    }
}
//! [teapot model]
