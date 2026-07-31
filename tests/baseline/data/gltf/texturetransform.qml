// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick3D
import QtQuick3D.AssetUtils

// A textured quad with a KHR_texture_transform combining offset, rotation,
// and scale. Renders identically through the Assimp and native glTF
// importers; the baseline guards the importer switch-over.
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
            scale: Qt.vector3d(100, 100, 100)
            source: "texquad.gltf"
        }
    }
}
