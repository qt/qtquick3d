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
        fragmentShader: "basicsky.frag"
        enableIBL: false

        // Sky Settings
        property vector3d skyTopColor: Qt.vector3d(m_skyTopColor.r, m_skyTopColor.g, m_skyTopColor.b)
        property vector3d skyHorizonColor: Qt.vector3d(m_skyHorizonColor.r, m_skyHorizonColor.g, m_skyHorizonColor.b)
        property real skyCurve: 0.09
        property real skyEnergy: 1.0

        // Ground Settings
        property vector3d groundBottomColor: Qt.vector3d(m_groundBottomColor.r, m_groundBottomColor.g, m_groundBottomColor.b)
        property vector3d groundHorizonColor: Qt.vector3d(m_groundHorizonColor.r, m_groundHorizonColor.g, m_groundHorizonColor.b)
        property real groundCurve: 0.02
        property real groundEnergy: 1.0

        property vector3d sunColor: Qt.vector3d(1.0, 1.0, 1.0)
        // Sun Settings
        property real sunEnergy: mySkyLight.brightness
        property vector3d sunDirection:Qt.vector3d(-mySkyLight.forward.x, -mySkyLight.forward.y, -mySkyLight.forward.z)
        property real sunDiskInnerAngle: 2.2
        property real sunDiskOuterAngle: 9.9
        property real sunDiskFalloff: 0.32
        property real sunAlpha: 1.0

        // Default colors
        readonly property color m_skyTopColor: "#A5D6F1"
        readonly property color m_skyHorizonColor: "#D6EAFA"
        readonly property color m_groundBottomColor: "#282F36"
        readonly property color m_groundHorizonColor: "#6C655F"
    }

    NumberAnimation {
        target: mySkyLight
        property: "eulerRotation.x"
        duration: 200
        easing.type: Easing.InOutQuad
        from: -90
        to: -10
        running: true
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

    WasdController {
        controlledObject: cam
    }
}
