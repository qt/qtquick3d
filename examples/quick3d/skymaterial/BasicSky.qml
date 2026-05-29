// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick3D

SkyMaterial {
    required property Node skyLight
    fragmentShader: "basicsky.frag"
    // Sky Settings
    readonly property color skyTopColor: "#A5D6F1"
    readonly property color skyHorizonColor: "#D6EAFA"
    property real skyCurve: 0.09
    property real skyEnergy: 1.0

    // Ground Settings
    readonly property color groundBottomColor: "#282F36"
    readonly property color groundHorizonColor: "#6C655F"
    property real groundCurve: 0.02
    property real groundEnergy: 1.0

    property vector3d sunColor: Qt.vector3d(1.0, 1.0, 1.0)
    // Sun Settings
    property real sunEnergy: 1.0
    property vector3d sunDirection:Qt.vector3d(-skyLight.forward.x, -skyLight.forward.y, -skyLight.forward.z)
    property real sunDiskInnerAngle: 2.2
    property real sunDiskOuterAngle: 9.9
    property real sunDiskFalloff: 0.32
    property real sunAlpha: 1.0
}
