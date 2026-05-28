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
    property int materialCount: 0
    property int modelCount: 0
    property int cameraCount: 0
    property int lightCount: 0
    property bool queryNullForMissing: false

    View3D {
        anchors.fill: parent

        DirectionalLight {}

        RuntimeLoader {
            id: loader
            source: Qt.resolvedUrl("torus_and_cone.glb")

            onStatusChanged: {
                if (status === RuntimeLoader.Error) {
                    root.loadError = true
                    return
                }
                if (status !== RuntimeLoader.Success)
                    return
                root.materialCount = loader.queryAll(RuntimeLoader.Materials).length
                root.modelCount    = loader.queryAll(RuntimeLoader.Models).length
                root.cameraCount   = loader.queryAll(RuntimeLoader.Cameras).length
                root.lightCount    = loader.queryAll(RuntimeLoader.Lights).length
                root.queryNullForMissing = (loader.query("__nonexistent__") === null)
                root.loaded = true
            }
        }
    }
}
