// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [import]
import QtQuick
import QtQuick.Controls

import QtQuick3D
import QtQuick3D.Helpers
//! [import]

ApplicationWindow {
    id: windowRoot
    visible: true
    width: 1280
    height: 720
    title: "Qt Quick 3D - Sky Material Example"

    property bool settingsVisible: true

    View3D {
        id: view3D
        x: settingsPane.x + settingsPane.width
        y: 0
        width: parent.width - x
        height: parent.height

        //! [advanced-sky]
        AdvancedSky {
            id: advancedSky
            skyLight: sun
        }
        //! [advanced-sky]

        //! [environment]
        environment: SceneEnvironment {
            backgroundMode: SceneEnvironment.SkyMaterial
            skyMaterial: advancedSky
            probeExposure: 0.25
            tonemapMode: SceneEnvironment.TonemapModeAces
        }
        //! [environment]

        PerspectiveCamera {
            id: cam
            position: Qt.vector3d(400, 600, -800)
            eulerRotation: Qt.vector3d(-5, -180, 0)
            Component.onCompleted: lookAt(Qt.vector3d(400, 550, 0))
        }
        camera: cam

        Node {
            id: sun
            // -90 = zenith (noon); 0 / -180 = horizon. Start at noon.
            eulerRotation.x: -90
        }

        //! [sun-animation]
        FrameAnimation {
            id: sunAnimator
            running: settingsPane.animateSun

            property real t: 0
            property real lastT: 0
            property int direction: 1

            onTriggered: {
                const min = -180
                const max = 0
                lastT = t
                t += direction * frameTime * settingsPane.sunSweepSpeed
                sun.eulerRotation.x = min + (Math.sin(t) * 0.5 + 0.5) * (max - min)
            }

            function updatePhaseFromSun() {
                const min = -180
                const max = 0
                let norm = Math.max(0.0, Math.min(1.0, (sun.eulerRotation.x - min) / (max - min)))
                t = Math.asin(norm * 2.0 - 1.0)
                direction = lastT < t ? 1 : -1
            }
        }
        //! [sun-animation]

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton
            propagateComposedEvents: true
            onClicked: wasd.forceActiveFocus()
        }

        Model {
            source: "#Sphere"
            x: -50
            y: 325
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
            x: 250
            y: 325
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
            x: 550
            y: 325
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
            x: 850
            y: 325
            scale: Qt.vector3d(2.5, 2.5, 2.5)
            castsShadows: true
            receivesShadows: true
            materials: PrincipledMaterial {
                baseColor: "lightblue"
                roughness: 0.8
                metalness: 1.0
            }
        }

        WasdController {
            id: wasd
            controlledObject: cam
        }
    }

    DebugView {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 10
        source: view3D
        visible: settingsPane.showDebugView
    }

    SettingsPane {
        id: settingsPane
        height: parent.height
        x: windowRoot.settingsVisible ? 0 : -width
        sun: sun
        advancedSky: advancedSky
        sunAnimator: sunAnimator

        Behavior on x {
            SmoothedAnimation { duration: 300 }
        }
    }

    RoundButton {
        id: settingsToggle
        icon.source: "sliders.svg"
        icon.width: 25
        icon.height: 25
        padding: 10
        x: settingsPane.x + settingsPane.width + padding
        y: padding
        onClicked: windowRoot.settingsVisible = !windowRoot.settingsVisible
    }
}
