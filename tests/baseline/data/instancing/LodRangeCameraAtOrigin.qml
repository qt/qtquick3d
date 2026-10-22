// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick3D

// Two models share an instance table and each show the instances in a range of
// distances from the camera. The near ones have to be red, drawn by the first
// model only, and the far ones blue, drawn by the second one only. The camera
// is at the origin, where the position that the distance culling remembers
// also starts out, so the culling has to be done without the camera moving.

Rectangle {
    width: 800
    height: 480
    color: Qt.rgba(0, 0, 0, 1)

    View3D {
        anchors.fill: parent

        environment: SceneEnvironment {
            clearColor: Qt.rgba(0, 0, 0, 1)
            backgroundMode: SceneEnvironment.Color
        }

        camera: PerspectiveCamera {
            position: Qt.vector3d(0, 0, 0)
            clipFar: 10000
        }

        InstanceList {
            id: instances
            instances: [
                InstanceListEntry { position: Qt.vector3d(-250, 0, -500); },
                InstanceListEntry { position: Qt.vector3d(-100, 0, -500); },
                InstanceListEntry { position: Qt.vector3d(300, 0, -2000); scale: Qt.vector3d(4, 4, 4) },
                InstanceListEntry { position: Qt.vector3d(900, 0, -2000); scale: Qt.vector3d(4, 4, 4) }
            ]
        }

        Model {
            source: "#Cube"
            instancing: instances
            instancingLodMin: 0
            instancingLodMax: 1000
            materials: DefaultMaterial {
                diffuseColor: "red"
                lighting: DefaultMaterial.NoLighting
            }
        }

        Model {
            source: "#Cube"
            instancing: instances
            instancingLodMin: 1000
            materials: DefaultMaterial {
                diffuseColor: "blue"
                lighting: DefaultMaterial.NoLighting
            }
        }
    }
}
