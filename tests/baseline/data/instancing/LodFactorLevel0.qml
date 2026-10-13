// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick3D

// lod_digits.mesh has an extruded digit per level: the original is a "0" and
// levels 1 to 9 are the digits "1" to "9". The leftmost model is not instanced
// with LOD disabled, so it always draws the original. The others are instanced
// with instancingLodFactor stepping from 0 to 1 in ninths, and are expected
// to read 0 to 9 in order.
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

        PerspectiveCamera {
            position.z: 1000
        }

        InstanceList {
            id: singleInstance
            instances: [
                InstanceListEntry {
                    position: Qt.vector3d(0, 0, 0)
                }
            ]
        }

        Model {
            source: "../shared/models/lod_digits.mesh"
            position: Qt.vector3d(-750, 0, 0)
            scale: Qt.vector3d(0.8, 0.8, 0.8)
            levelOfDetailBias: 0
            materials: PrincipledMaterial { baseColor: "white"; lighting: PrincipledMaterial.NoLighting }
        }

        Repeater3D {
            model: 10
            Model {
                required property int index
                source: "../shared/models/lod_digits.mesh"
                position: Qt.vector3d(-600 + index * 150, 0, 0)
                scale: Qt.vector3d(0.8, 0.8, 0.8)
                instancing: singleInstance
                instancingLodFactor: index / 9
                materials: PrincipledMaterial { baseColor: Qt.hsla(index / 10, 1, 0.6, 1); lighting: PrincipledMaterial.NoLighting }
            }
        }
    }
}
