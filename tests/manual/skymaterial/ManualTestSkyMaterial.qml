// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts

import QtQuick3D
import QtQuick3D.Helpers

Window {
    id: windowRoot
    visible: true
    width: 1280
    height: 720

    View3D {
        id: view3D
        anchors.fill: parent

        // IBL backend: 0=Legacy CPU, 1=SkyMaterial GPU
        property int iblMode: 1
        property int skyMaterialKind: 1
        property SkyMaterial activeSkyMaterial: skyMaterialKind === 0 ? basicSkyEffect : advancedSkyEffect
        property bool animateSunAcrossHorizon: false
        property real sunSweepSpeedDegPerSec: 20.0
        property real sunSweepMinRot: -180
        property real sunSweepMaxRot: 0
        property real sunSweepDirection: 1.0

        property Texture lightProbe: Texture {
            textureData: ProceduralSkyTextureData {
                id: procSkyTextureData
            }
        }

        // ProceduralSkyTextureData regenerates the entire CPU texture every time a
        // sun property changes, so only drive it when Legacy CPU IBL is actually in use.
        Binding {
            target: procSkyTextureData
            property: "sunLatitude"
            // Map DirectionalLight.eulerRotation.x (expected in [-180, 0])
            // to a sun elevation in [0, 90]: 0 = horizon, -90 = zenith, -180 = opposite horizon.
            value: 90 - Math.abs(mySkyLight.eulerRotation.x + 90)
            when: view3D.iblMode === 0
        }
        Binding {
            target: procSkyTextureData
            property: "sunLongitude"
            value: mySkyLight.eulerRotation.y
            when: view3D.iblMode === 0
        }

        AdvancedSky {
            id: advancedSkyEffect
            skyLight: mySkyLight
        }

        BasicSky {
            id: basicSkyEffect
            skyLight: mySkyLight
        }

        environment: SceneEnvironment {
            id: scene
            backgroundMode: view3D.iblMode === 0 ? SceneEnvironment.SkyBox : SceneEnvironment.SkyMaterial
            skyMaterial: view3D.activeSkyMaterial
            // Legacy CPU probe is mode 0; mode 1 uses GPU SkyMaterial capture.
            lightProbe: view3D.iblMode === 0 ? view3D.lightProbe : null
        }

        PerspectiveCamera {
            id: cam
            position: Qt.vector3d(1338.76, 251.537, -942.507)
            eulerRotation: Qt.vector3d(-5.44442, -225.778, 0)
        }
        camera: cam


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

        FrameAnimation {
            running: view3D.animateSunAcrossHorizon
            onTriggered: {
                let rot = mySkyLight.eulerRotation.x
                        + view3D.sunSweepDirection * view3D.sunSweepSpeedDegPerSec * frameTime

                if (rot > view3D.sunSweepMaxRot) {
                    rot = view3D.sunSweepMaxRot
                    view3D.sunSweepDirection = -1.0
                } else if (rot < view3D.sunSweepMinRot) {
                    rot = view3D.sunSweepMinRot
                    view3D.sunSweepDirection = 1.0
                }

                mySkyLight.eulerRotation.x = rot
            }
        }

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

        WasdController {
            id: wasdController
            controlledObject: cam
        }
        DebugView {
            anchors.top: parent.top
            anchors.right: parent.right
            source: view3D
        }
    }



    Pane {
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
                    text: "Sky Controls"
                    font.bold: true
                }

                ComboBox {
                    Layout.fillWidth: true
                    model: ["Legacy CPU", "SkyMaterial GPU"]
                    currentIndex: view3D.iblMode
                    onActivated: view3D.iblMode = currentIndex
                }

                ComboBox {
                    Layout.fillWidth: true
                    model: ["BasicSkyEffect", "AdvancedSkyEffect"]
                    currentIndex: view3D.skyMaterialKind
                    onActivated: view3D.skyMaterialKind = currentIndex
                }

                Switch {
                    Layout.fillWidth: true
                    text: "Animate Sun Across Horizon"
                    checked: view3D.animateSunAcrossHorizon
                    onToggled: view3D.animateSunAcrossHorizon = checked
                }

                Switch {
                    Layout.fillWidth: true
                    text: "Toggle IBL"
                    checked: view3D.activeSkyMaterial.enableIBL
                    onToggled: view3D.activeSkyMaterial.enableIBL = checked
                }

                Label {
                    Layout.fillWidth: true
                    text: "Sun X rot: " + mySkyLight.eulerRotation.x.toFixed(1)
                }
                Slider {
                    Layout.fillWidth: true
                    from: -180
                    to: 180
                    value: mySkyLight.eulerRotation.x
                    onMoved: mySkyLight.eulerRotation.x = value
                    stepSize: 1
                }

                Label {
                    Layout.fillWidth: true
                    text: "Sun Y rot: " + mySkyLight.eulerRotation.y.toFixed(1)
                }
                Slider {
                    Layout.fillWidth: true
                    from: -180
                    to: 180
                    value: mySkyLight.eulerRotation.y
                    onMoved: mySkyLight.eulerRotation.y = value
                }
                Label {
                    Layout.fillWidth: true
                    text: "GPU Radiance Size: " + view3D.activeSkyMaterial.radianceMapSize
                }
                Slider {
                    Layout.fillWidth: true
                    from: 0
                    to: 2048
                    stepSize: 1
                    value: view3D.activeSkyMaterial.radianceMapSize
                    enabled: view3D.iblMode === 1
                    onMoved: view3D.activeSkyMaterial.radianceMapSize = Math.round(value)
                }
                Label {
                    Layout.fillWidth: true
                    text: "IBL Sample Count: " + view3D.activeSkyMaterial.iblSampleCount
                }
                Slider {
                    Layout.fillWidth: true
                    from: 1
                    to: 1024
                    stepSize: 1
                    value: view3D.activeSkyMaterial.iblSampleCount
                    enabled: view3D.iblMode === 1
                    onMoved: view3D.activeSkyMaterial.iblSampleCount = Math.round(value)
                }
                Label {
                    Layout.fillWidth: true
                    text: "IBL render frames: " + view3D.activeSkyMaterial.iblRenderFrames
                }
                Slider {
                    Layout.fillWidth: true
                    from: 1
                    to: 32
                    stepSize: 1
                    value: view3D.activeSkyMaterial.iblRenderFrames
                    enabled: view3D.iblMode === 1
                    onMoved: view3D.activeSkyMaterial.iblRenderFrames = Math.round(value)
                }
                Label {
                    Layout.fillWidth: true
                    text: "Sun Brightness: " + mySkyLight.customBrightness.toFixed(2)
                }
                Slider {
                    Layout.fillWidth: true
                    from: 0
                    to: 20
                    value: mySkyLight.customBrightness
                    onMoved: mySkyLight.customBrightness = value
                }
                Label {
                    Layout.fillWidth: true
                    text: "Sun Disk Inner Angle: " + view3D.activeSkyMaterial.sunDiskInnerAngle.toFixed(2)
                }
                Slider {
                    Layout.fillWidth: true
                    from: 0.1
                    to: 10
                    value: view3D.activeSkyMaterial.sunDiskInnerAngle
                    onMoved: view3D.activeSkyMaterial.sunDiskInnerAngle = value
                }

                Label {
                    Layout.fillWidth: true
                    text: "Sun Disk Outer Angle: " + view3D.activeSkyMaterial.sunDiskOuterAngle.toFixed(2)
                }
                Slider {
                    Layout.fillWidth: true
                    from: 1
                    to: 150
                    value: view3D.activeSkyMaterial.sunDiskOuterAngle
                    onMoved: view3D.activeSkyMaterial.sunDiskOuterAngle = value
                }

                Label {
                    Layout.fillWidth: true
                    text: "Sun Disk Falloff: " + view3D.activeSkyMaterial.sunDiskFalloff.toFixed(3)
                }
                Slider {
                    Layout.fillWidth: true
                    from: 0.001
                    to: 1
                    value: view3D.activeSkyMaterial.sunDiskFalloff
                    onMoved: view3D.activeSkyMaterial.sunDiskFalloff = value
                }

                Label {
                    Layout.fillWidth: true
                    enabled: view3D.skyMaterialKind === 1
                    text: "LUT Size Transmittance: " + advancedSkyEffect.lutSizeTransmit
                }
                Slider {
                    Layout.fillWidth: true
                    from: 8
                    to: 1024
                    stepSize: 8
                    value: advancedSkyEffect.lutSizeTransmit
                    enabled: view3D.skyMaterialKind === 1 && view3D.iblMode === 1
                    onMoved: advancedSkyEffect.lutSizeTransmit = Math.round(value / 8) * 8
                }

                Label {
                    Layout.fillWidth: true
                    enabled: view3D.skyMaterialKind === 1
                    text: "LUT Size: " + advancedSkyEffect.lutSize
                }
                Slider {
                    Layout.fillWidth: true
                    from: 8
                    to: 1024
                    stepSize: 8
                    value: advancedSkyEffect.lutSize
                    enabled: view3D.skyMaterialKind === 1 && view3D.iblMode === 1
                    onMoved: advancedSkyEffect.lutSize = Math.round(value / 8) * 8
                }

                Label {
                    Layout.fillWidth: true
                    enabled: view3D.skyMaterialKind === 1
                    text: "Aerosol type"
                }
                ComboBox {
                    enabled: view3D.skyMaterialKind === 1
                    id: aerosolTypeCb
                    Layout.fillWidth: true
                    // 0=Background, 1=Desert Dust, 2=Maritime Clean, 3=Maritime Mineral,
                    // 4=Polar Antarctic, 5=Polar Arctic, 6=Remote Continental, 7=Rural, 8=Urban
                    model: [
                        { value: 0, text: qsTr("Background") },
                        { value: 1, text: qsTr("Desert Dust") },
                        { value: 2, text: qsTr("Maritime Clean") },
                        { value: 3, text: qsTr("Maritime Mineral") },
                        { value: 4, text: qsTr("Polar Antarctic") },
                        { value: 5, text: qsTr("Polar Arctic") },
                        { value: 6, text: qsTr("Remote Continental") },
                        { value: 7, text: qsTr("Rural") },
                        { value: 8, text: qsTr("Urban") }
                    ]
                    currentIndex: advancedSkyEffect.aerosol_type
                    textRole: "text"
                    valueRole: "value"
                    onActivated: advancedSkyEffect.aerosol_type = currentValue
                }
                Label {
                    Layout.fillWidth: true
                    enabled: view3D.skyMaterialKind === 1
                    text: "Eye altitude: " + eyeAltitudeSlider.value.toFixed(2)
                }
                Slider {
                    id: eyeAltitudeSlider
                    Layout.fillWidth: true
                    enabled: view3D.skyMaterialKind === 1
                    from: 0.0
                    to: 1.0
                    value: advancedSkyEffect.eye_altitude
                    stepSize: 0.01
                    onMoved: advancedSkyEffect.eye_altitude = value
                }
                Label {
                    Layout.fillWidth: true
                    enabled: view3D.skyMaterialKind === 1
                    text: "Ozone (Dobson units): " + ozoneSlider.value.toFixed(0)
                }
                Slider {
                    id: ozoneSlider
                    Layout.fillWidth: true
                    enabled: view3D.skyMaterialKind === 1
                    from: 100
                    to: 600
                    value: advancedSkyEffect.ozone_mean_monthly_dobson
                    stepSize: 10
                    onMoved: advancedSkyEffect.ozone_mean_monthly_dobson = value
                }
                Label {
                    Layout.fillWidth: true
                    enabled: view3D.skyMaterialKind === 1
                    text: "Aerosol Turbidity: " + turbiditySlider.value.toFixed(2)
                }
                Slider {
                    id: turbiditySlider
                    Layout.fillWidth: true
                    enabled: view3D.skyMaterialKind === 1
                    from: 0.0
                    to: 1.0
                    value: advancedSkyEffect.aerosol_turbidity
                    stepSize: 0.1
                    onMoved: advancedSkyEffect.aerosol_turbidity = value
                }
            }
        }
    }
}
