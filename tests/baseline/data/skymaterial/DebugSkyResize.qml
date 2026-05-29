// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers

Rectangle {
    id: root
    width: 800
    height: 480
    color: Qt.rgba(1, 1, 1, 1)

    SkyMaterial {
        id: mySkyMaterial
    }

    View3D {
        id: view3D
        anchors.fill: parent

        environment: SceneEnvironment {
            id: scene
            backgroundMode: SceneEnvironment.SkyMaterial
            skyMaterial: mySkyMaterial
        }

        camera: PerspectiveCamera {
            id: cam
            position: Qt.vector3d(750, 460, -1128)
            eulerRotation: Qt.vector3d(-13, -203, 0)
        }

        DirectionalLight {
            id: mySkyLight
            eulerRotation.x: -90
            castsShadow: true
            softShadowQuality: Light.PCF16
            shadowMapQuality: Light.ShadowMapQualityVeryHigh
            pcfFactor: 3
            use32BitShadowmap: true
            property real customBrightness: 1.0
            brightness: eulerRotation.x > 0.0 || eulerRotation.x < -180 ? 0 : customBrightness
        }

        /// Models
        Model {
            source: "#Rectangle"
            scale: Qt.vector3d(30, 30, 30)
            eulerRotation.x: -90
            receivesShadows: true
            materials: PrincipledMaterial {
                baseColor: "lightgray"
                roughness: 0.9
                metalness: 0.0
            }
        }

        Model {
            source: "#Cube"
            x: -375
            y: 100
            scale: Qt.vector3d(2, 2, 2)
            castsShadows: true
            receivesShadows: true
            materials: PrincipledMaterial {
                baseColor: "steelblue"
                roughness: 0.25
                metalness: 0.0
            }
        }

        Model {
            source: "#Sphere"
            x: -75
            y: 125
            scale: Qt.vector3d(2.5, 2.5, 2.5)
            castsShadows: true
            receivesShadows: true
            materials: PrincipledMaterial {
                baseColor: "lightblue"
                roughness: 0.05
                metalness: 1.0
            }
        }

        Model {
            source: "#Sphere"
            x: 275
            y: 125
            scale: Qt.vector3d(2.5, 2.5, 2.5)
            castsShadows: true
            receivesShadows: true
            materials: PrincipledMaterial {
                baseColor: "lightblue"
                roughness: 0.3
                metalness: 1.0
            }
        }

        Model {
            source: "#Sphere"
            x: 575
            y: 125
            scale: Qt.vector3d(2.5, 2.5, 2.5)
            castsShadows: true
            receivesShadows: true
            materials: PrincipledMaterial {
                baseColor: "lightblue"
                roughness: 0.55
                metalness: 1.0
            }
        }

        Model {
            source: "#Sphere"
            x: 875
            y: 125
            scale: Qt.vector3d(2.5, 2.5, 2.5)
            castsShadows: true
            receivesShadows: true
            materials: PrincipledMaterial {
                baseColor: "lightblue"
                roughness: 0.8
                metalness: 1.0
            }
        }
    }

    NumberAnimation {
        target: mySkyMaterial
        property: "radianceMapSize"
        duration: 200
        from: 8
        to: 64
        running: true
    }

    WasdController {
        controlledObject: cam
    }
}
