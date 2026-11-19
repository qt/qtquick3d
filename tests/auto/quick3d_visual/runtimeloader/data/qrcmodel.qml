// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick3D
import QtQuick3D.AssetUtils

Item {
    id: root
    width: 200
    height: 200

    property bool loaded: false
    property bool loadError: false
    property var textureSources: []

    View3D {
        anchors.fill: parent

        environment: SceneEnvironment {
            backgroundMode: SceneEnvironment.Color
            clearColor: "black"
        }
        DirectionalLight {}
        PerspectiveCamera { z: 300 }

        RuntimeLoader {
            id: loader
            scale: Qt.vector3d(100, 100, 100)
            source: "qrc:/qrcmodel/texquad.gltf"

            onStatusChanged: {
                if (status === RuntimeLoader.Error) {
                    root.loadError = true
                    return
                }
                if (status !== RuntimeLoader.Success)
                    return
                root.textureSources = loader.queryAll(RuntimeLoader.Textures).map(t => t.source.toString())
                root.loaded = true
            }
        }
    }
}
