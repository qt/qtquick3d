// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers

Rectangle {
    width: 800
    height: 480
    color: Qt.rgba(1, 1, 1, 1)

    View3D {
        anchors.fill: parent

        environment: SceneEnvironment {
            backgroundMode: SceneEnvironment.SkyMaterial
            skyMaterial: ProceduralSkyMaterial {
                sunLatitude: 35
                sunLongitude: 30
            }
        }

        camera: PerspectiveCamera {
            position: Qt.vector3d(0, 200, 800)
            eulerRotation: Qt.vector3d(-10, 0, 0)
        }

        Model {
            source: "#Rectangle"
            scale: Qt.vector3d(30, 30, 30)
            eulerRotation.x: -90
            receivesShadows: true
            materials: PrincipledMaterial {
                baseColor: "lightgray"
                roughness: 0.9
            }
        }

        Model {
            source: "#Sphere"
            x: -300
            y: 125
            scale: Qt.vector3d(2.5, 2.5, 2.5)
            materials: PrincipledMaterial {
                baseColor: "lightblue"
                roughness: 0.05
                metalness: 1.0
            }
        }

        Model {
            source: "#Sphere"
            x: 0
            y: 125
            scale: Qt.vector3d(2.5, 2.5, 2.5)
            materials: PrincipledMaterial {
                baseColor: "lightblue"
                roughness: 0.3
                metalness: 1.0
            }
        }

        Model {
            source: "#Sphere"
            x: 300
            y: 125
            scale: Qt.vector3d(2.5, 2.5, 2.5)
            materials: PrincipledMaterial {
                baseColor: "lightblue"
                roughness: 0.7
                metalness: 1.0
            }
        }
    }
}
