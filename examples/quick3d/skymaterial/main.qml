// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [import]
import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts

import QtQuick3D
import QtQuick3D.Helpers
//! [import]

Window {
    id: windowRoot
    visible: true
    width: 1280
    height: 720
    title: "Qt Quick 3D - Sky Material Example"

    View3D {
        id: view3D

        anchors {
            top: parent.top
            bottom: parent.bottom
            left: sidePanel.right
            right: parent.right
        }

        // Which SkyMaterial subclass to use: 0 = BasicSky (gradient + sun disk),
        // 1 = AdvancedSky (Hillaire-style physical atmospheric scattering).
        property int skyVariant: 0
        property SkyMaterial activeSky: skyVariant === 0 ? basicSky : advancedSky
        property bool animateSun: true
        property real sunSweepSpeedDegPerSec: 40.0
        property real sunSweepMinRot: -180
        property real sunSweepMaxRot: 0
        property real sunSweepDirection: 1.0

        property int iblRenderFrames: 2
        property int iblSampleCount: 32
        property bool enableIBL: true
        property int radianceMapSize: 512

        //! [sky-materials]
        // Two SkyMaterial subclasses are instantiated up front. The toggle in
        // the side panel swaps which one is assigned to SceneEnvironment.skyMaterial.
        BasicSky {
            id: basicSky
            iblRenderFrames: view3D.iblRenderFrames
            iblSampleCount: view3D.iblSampleCount
            enableIBL: view3D.enableIBL
            radianceMapSize: view3D.radianceMapSize
            skyLight: sun
        }

        AdvancedSky {
            id: advancedSky
            iblRenderFrames: view3D.iblRenderFrames
            iblSampleCount: view3D.iblSampleCount
            enableIBL: view3D.enableIBL
            radianceMapSize: view3D.radianceMapSize
            skyLight: sun
        }
        //! [sky-materials]

        //! [environment]
        // SkyMaterial produces both the visible sky (rendered as the background)
        // and the image-based lighting cubemap used for reflections and ambient
        // light on the PBR materials in the scene.
        environment: SceneEnvironment {
            backgroundMode: SceneEnvironment.SkyMaterial
            skyMaterial: view3D.activeSky
        }
        //! [environment]

        PerspectiveCamera {
            id: cam
            position: Qt.vector3d(400, 600, -800)
            eulerRotation: Qt.vector3d(-5, -180, 0)
            Component.onCompleted: lookAt(Qt.vector3d(400, 325, 0))
        }
        camera: cam

        Node {
            id: sun
            // -90 = zenith (noon); 0 / -180 = horizon. Start at noon.
            eulerRotation.x: -90
        }

        //! [sun-animation]
        // Ping-pongs the sun back and forth across the horizon while
        // view3D.animateSun is true. With time-sliced IBL the cubemap
        // progressively re-converges as the sun moves.
        FrameAnimation {
            id: sunAnimator
            running: view3D.animateSun

            property real t: 0
            property real lastT: 0
            property int direction: 1

            onTriggered: {
                const min = view3D.sunSweepMinRot
                const max = view3D.sunSweepMaxRot
                lastT = t
                t += direction * frameTime * view3D.sunSweepSpeedDegPerSec * 0.01
                sun.eulerRotation.x = min + (Math.sin(t) * 0.5 + 0.5) * (max - min)
            }

            function updatePhaseFromSun() {
                const min = view3D.sunSweepMinRot
                const max = view3D.sunSweepMaxRot
                let norm = Math.max(0.0, Math.min(1.0, (sun.eulerRotation.x - min) / (max - min)))
                t = Math.asin(norm * 2.0 - 1.0)
                direction = lastT < t ? 1 : -1
            }
        }
        //! [sun-animation]

        // Demo scene: four spheres of increasing roughness. The spheres show
        // the IBL response — the smoothest one mirrors the procedural sky
        // almost perfectly, the  rougher ones blur it through the GGX prefilter chain.
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
            controlledObject: cam
        }
    }

    Pane {
        id: sidePanel

        width: 256
        height: parent.height
        anchors.top: parent.top
        anchors.left: parent.left

        ScrollView {
            anchors.fill: parent
            anchors.margins: 8

            ColumnLayout {
                width: parent.width
                spacing: 6

                Label {
                    Layout.fillWidth: true
                    text: "Sky Material Controls"
                    font.bold: true
                }

                Label {
                    Layout.fillWidth: true
                    text: "Sky variant"
                }
                ComboBox {
                    Layout.fillWidth: true
                    model: ["Basic", "Advanced"]
                    currentIndex: view3D.skyVariant
                    onActivated: view3D.skyVariant = currentIndex
                }

                Switch {
                    Layout.fillWidth: true
                    text: "Animate sun"
                    checked: view3D.animateSun
                    onToggled: view3D.animateSun = checked
                }

                Label {
                    Layout.fillWidth: true
                    text: "Sun elevation: " + sun.eulerRotation.x.toFixed(0) + "°"
                    enabled: !view3D.animateSun
                }
                Slider {
                    Layout.fillWidth: true
                    from: -180
                    to: 0
                    stepSize: 1
                    enabled: !view3D.animateSun
                    value: sun.eulerRotation.x
                    onMoved: {
                        sun.eulerRotation.x = value
                        sunAnimator.updatePhaseFromSun()
                    }
                }

                Label {
                    Layout.fillWidth: true
                    text: "IBL render frames: " + view3D.iblRenderFrames
                }
                Slider {
                    Layout.fillWidth: true
                    from: 1
                    to: 32
                    stepSize: 1
                    value: view3D.iblRenderFrames
                    onMoved: view3D.iblRenderFrames = Math.round(value)
                }

                Label {
                    Layout.fillWidth: true
                    text: "IBL sample count: " + view3D.iblSampleCount
                }
                Slider {
                    Layout.fillWidth: true
                    from: 1
                    to: 1024
                    stepSize: 1
                    value: view3D.iblSampleCount
                    onMoved: view3D.iblSampleCount = Math.round(value)
                }

                Label {
                    Layout.fillWidth: true
                    text: "Radiance map size:"
                }
                ComboBox {
                    Layout.fillWidth: true
                    model: [
                        { value: 256, text: qsTr("256") },
                        { value: 512, text: qsTr("512") },
                        { value: 1024, text: qsTr("1024") },
                    ]
                    textRole: "text"
                    valueRole: "value"
                    currentValue: 512
                    onActivated: view3D.radianceMapSize = currentValue
                }
            }
        }
    }
}
