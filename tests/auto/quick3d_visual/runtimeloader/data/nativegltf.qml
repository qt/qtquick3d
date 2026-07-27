// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick3D
import QtQuick3D.AssetUtils

Item {
    id: root
    width: 200
    height: 200

    readonly property var sources: [
        "texquad.gltf", // textures, sampler, KHR_texture_transform
        "extmats.gltf", // material extensions incl. specular glossy and unlit
        "simpleskin.gltf", // skinning and skeletal animation
        "morphquad.gltf", // morph targets and weight animation
        "animquad.gltf", // node TRS animation with LINEAR and STEP sampling
        "instquad.gltf" // EXT_mesh_gpu_instancing
    ]
    readonly property int totalCount: sources.length
    property int loadedCount: 0
    property int doneCount: 0
    property string errors: ""
    readonly property bool settled: doneCount === totalCount

    View3D {
        anchors.fill: parent

        DirectionalLight {}
        PerspectiveCamera { position: Qt.vector3d(0, 0, 600) }

        Repeater3D {
            model: root.sources
            RuntimeLoader {
                required property string modelData
                required property int index
                x: (index - root.sources.length / 2) * 150
                scale: Qt.vector3d(50, 50, 50)
                source: Qt.resolvedUrl(modelData)
                onStatusChanged: {
                    if (status === RuntimeLoader.Success) {
                        root.loadedCount++
                        root.doneCount++
                    } else if (status === RuntimeLoader.Error) {
                        root.errors += modelData + ": " + errorString + "\n"
                        root.doneCount++
                    }
                }
            }
        }
    }
}
