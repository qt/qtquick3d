// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick3D
import QtQuick3D.AssetUtils

// A smooth-shaded indexed sphere: guards vertex data, normals, and default
// material handling across the importer switch-over.
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
        PointLight {
            position: Qt.vector3d(200, 200, 200)
            brightness: 2
        }

        RuntimeLoader {
            scale: Qt.vector3d(100, 100, 100)
            source: "sphere.gltf"
        }
    }
}
