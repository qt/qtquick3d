// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick3D

// Three instanced cubes at clearly different distances from the camera, each in
// its own part of the viewport. The LOD thresholds decide which ones are drawn.
Item {
    id: root
    width: 400
    height: 400

    property real lodMin: -1
    property real lodMax: -1
    property bool depthSorting: false

    View3D {
        anchors.fill: parent
        camera: lodCamera
        environment: SceneEnvironment {
            clearColor: "black"
            backgroundMode: SceneEnvironment.Color
        }

        // Magnification 1 maps one world unit to one logical pixel, so screen
        // positions do not depend on the distance to the camera.
        OrthographicCamera { id: lodCamera; z: 600 }

        InstanceList {
            id: instanceList
            depthSortingEnabled: root.depthSorting
            instances: [
                // distance to the camera: 612
                InstanceListEntry {
                    position: Qt.vector3d(-120, 0, 0)
                    scale: Qt.vector3d(0.5, 0.5, 0.5)
                },
                // distance to the camera: 900
                InstanceListEntry {
                    position: Qt.vector3d(0, 0, -300)
                    scale: Qt.vector3d(0.5, 0.5, 0.5)
                },
                // distance to the camera: 1206
                InstanceListEntry {
                    position: Qt.vector3d(120, 0, -600)
                    scale: Qt.vector3d(0.5, 0.5, 0.5)
                }
            ]
        }

        Model {
            source: "#Cube"
            instancing: instanceList
            instancingLodMin: root.lodMin
            instancingLodMax: root.lodMax
            materials: PrincipledMaterial {
                baseColor: "white"
                lighting: PrincipledMaterial.NoLighting
            }
        }
    }
}
