// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick3D
import QtQuick3D.AssetUtils

// Three overlapping quads using a fully extended principled material
// (clearcoat, transmission, volume, ior, emissive strength, specular,
// alpha mask), a specular glossy material, and an unlit material.
Rectangle {
    width: 400
    height: 400
    color: "lightgray"

    View3D {
        anchors.fill: parent
        renderMode: View3D.Offscreen

        PerspectiveCamera {
            position: Qt.vector3d(0, 0, 350)
        }

        DirectionalLight {}

        RuntimeLoader {
            scale: Qt.vector3d(80, 80, 80)
            eulerRotation.y: 30
            source: "extmats.gltf"
        }
    }
}
