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
    property var variants: []
    property string defaultMaterial: ""
    property string blueMaterial: ""
    property string redMaterial: ""
    property string restoredMaterial: ""

    View3D {
        anchors.fill: parent

        DirectionalLight {}

        RuntimeLoader {
            id: loader
            source: Qt.resolvedUrl("variants.gltf")

            onStatusChanged: {
                if (status === RuntimeLoader.Error) {
                    root.loadError = true
                    return
                }
                if (status !== RuntimeLoader.Success)
                    return

                root.variants = loader.materialVariants
                const model = loader.queryAll(RuntimeLoader.Models)[0]
                root.defaultMaterial = model.materials[0].objectName
                loader.materialVariant = "Blue"
                root.blueMaterial = model.materials[0].objectName
                loader.materialVariant = "Red"
                root.redMaterial = model.materials[0].objectName
                loader.materialVariant = ""
                root.restoredMaterial = model.materials[0].objectName
                root.loaded = true
            }
        }
    }
}
