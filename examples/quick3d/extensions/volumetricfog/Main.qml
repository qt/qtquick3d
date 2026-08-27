// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick3D
import QtQuick3D.Helpers

import VFExtension

ApplicationWindow {
    id: window
    visible: true
    width: 1200
    height: 720
    title: qsTr("Volumetric Fog")

    //! [fog extension]
    VolumetricFogExtension {
        id: froxelExtension
        froxelWidth: 160
        froxelHeight: 96
        froxelDepth: 96
        nearPlane: 1.0
        farPlane: 4000.0
        fogVolumes: [ fogSphere ]

        iesTexture: settings.iesLights ? iesAtlasTexture : null
        iesCount: iesTextureData.sources.length
        iesLightProfiles: [
            IESLightProfileIndex { light: pinkSpot; index: settings.iesLightIndex; intensity: 0.5}
        ]
    }
    //! [fog extension]

    View3D {
        id: view
        x: settings.viewX
        y: 0
        width: parent.width - settings.viewX
        height: parent.height
        camera: camera

        extensions: [ froxelExtension ]

        //! [ies texture]
        Texture {
            id: iesAtlasTexture
            textureData: IESTextureData {
                id: iesTextureData
                sources: [
                    "qrc:/assets/l0.ies",
                    "qrc:/assets/l1.ies",
                    "qrc:/assets/l2.ies",
                    "qrc:/assets/l3.ies",
                    "qrc:/assets/l4.ies",
                    "qrc:/assets/l5.ies",
                ]
            }

            tilingModeHorizontal: Texture.ClampToEdge
            tilingModeVertical: Texture.ClampToEdge
            minFilter: Texture.Linear
            magFilter: Texture.Linear
            generateMipmaps: false
        }
        //! [ies texture]

        environment: ExtendedSceneEnvironment {
            id: se
            clearColor: "black"
            backgroundMode: SceneEnvironment.Color
            antialiasingMode: settings.enableMotionVectorTAA ? SceneEnvironment.NoAA : SceneEnvironment.MSAA
            antialiasingQuality: SceneEnvironment.High
            temporalAAEnabled: true
            tonemapMode: SceneEnvironment.TonemapModeAces
            temporalAAMode: SceneEnvironment.TAAMotionVector

            //! [fog effect]
            effects: [
                VolumetricFogEffect {
                    froxelTexture: froxelExtension.froxelTexture
                    cameraPosition: camera.scenePosition
                    cameraForward: camera.forward
                    invViewMatrix: camera.sceneTransform
                    marchSteps: froxelExtension.froxelDepth
                    nearPlane: froxelExtension.nearPlane
                    farPlane: froxelExtension.farPlane
                    frameBaseJitter: se.temporalAAEnabled &&
                                     se.temporalAAMode === SceneEnvironment.TAAMotionVector &&
                                     se.antialiasingMode !== SceneEnvironment.MSAA ? 1.0 : 0.0
                    jitterIntensity: settings.jitterIntensity
                }
            ]
            //! [fog effect]
        }

        FrameAnimation {
            running: true
            onTriggered: {
                baseNode.eulerRotationY += 12 * frameTime
                if (baseNode.eulerRotationY >= 360)
                    baseNode.eulerRotationY = 0

                baseNode.time += 1000 * frameTime
                if (baseNode.time >= 100000)
                    baseNode.time = 0
            }
        }

        Node {
            id: camOrigin
            PerspectiveCamera {
                id: camera
                position: Qt.vector3d(0, 900, 1800)
                clipFar: settings.clipFar
                Component.onCompleted: {
                    lookAt(Qt.vector3d(0, 500, 0))
                }
            }
        }

        SpotLight {
            id: pinkSpot
            visible: settings.iesLights
            brightness: 20 * baseNode.lightsBrightnessMultiplier
            eulerRotation: Qt.vector3d(settings.eulerRotation.x + 80,
                                       settings.eulerRotation.y,
                                       settings.eulerRotation.z)
            innerConeAngle: 160
            coneAngle: 170
            position: Qt.vector3d(0, 1500, 0)
            color: "white"
            castsShadow: settings.iesLightsShadow
            shadowFactor: settings.shadowFactor
            shadowBias: settings.shadowBias
            shadowMapQuality: settings.shadowMapQuality
        }

        Node {
            id: baseNode
            property real lightsBrightnessMultiplier: 1.0
            property real time: 0.0
            property real eulerRotationY: 0
            eulerRotation.y: eulerRotationY + settings.eulerRotation.y

            SpotLight {
                id: yellowSpot
                visible: settings.spotLightEnabled
                brightness: 10 * baseNode.lightsBrightnessMultiplier
                eulerRotation: Qt.vector3d(settings.eulerRotation.x + 30,
                                           settings.eulerRotation.y + 90,
                                           settings.eulerRotation.z)
                innerConeAngle: 40
                coneAngle: 45
                position: Qt.vector3d(200, 1200, 0)
                color: "yellow"
                castsShadow: settings.spotLightShadow
                shadowFactor: settings.shadowFactor
                shadowBias: settings.shadowBias
                shadowMapQuality: settings.shadowMapQuality
            }

            SpotLight {
                id: lightBlueSpot
                visible: settings.spotLightEnabled
                brightness: 20 * baseNode.lightsBrightnessMultiplier
                eulerRotation: Qt.vector3d(settings.eulerRotation.x + 20,
                                           settings.eulerRotation.y - 90,
                                           settings.eulerRotation.z)
                innerConeAngle: 30
                coneAngle: 50
                position: Qt.vector3d(200, 1200, 0)
                color: "lightblue"
                castsShadow: settings.spotLightShadow
                shadowFactor: settings.shadowFactor
                shadowBias: settings.shadowBias
                shadowMapQuality: settings.shadowMapQuality
            }

            DirectionalLight {
                visible: settings.dirLightEnabled
                brightness: 4 * baseNode.lightsBrightnessMultiplier
                castsShadow: settings.dirLightShadow
                shadowFactor: settings.shadowFactor
                eulerRotation: Qt.vector3d(settings.eulerRotation.x - 5,
                                           settings.eulerRotation.y - 90,
                                           settings.eulerRotation.z)
                csmSplit1: settings.csmSplit1
                csmSplit2: settings.csmSplit2
                csmSplit3: settings.csmSplit3
                csmNumSplits: settings.csmNumSplits
                shadowMapQuality: settings.shadowMapQuality
                csmBlendRatio: settings.csmBlendRatio
                shadowBias: settings.shadowBias
                pcfFactor: settings.pcfFactor
                softShadowQuality: settings.softShadowQuality
                shadowMapFar: settings.shadowMapFar
                lockShadowmapTexels: settings.lockShadowmapTexels
            }

            PointLight {
                id: redPoint
                visible: settings.pointLightEnabled
                position: Qt.vector3d(200, 200, -200)
                color: "red"
                brightness: 50 * baseNode.lightsBrightnessMultiplier
                castsShadow: settings.pointLightShadow
                shadowFactor: settings.shadowFactor
                shadowBias: settings.shadowBias
                shadowMapQuality: settings.shadowMapQuality
            }

            PointLight {
                id: limePoint
                visible: settings.pointLightEnabled
                position: Qt.vector3d(-300, 300, 300)
                color: "lime"
                brightness: 50 * baseNode.lightsBrightnessMultiplier
                castsShadow: settings.pointLightShadow
                shadowFactor: settings.shadowFactor
                shadowBias: settings.shadowBias
                shadowMapQuality: settings.shadowMapQuality
            }

            PointLight {
                id: bluePoint
                visible: settings.pointLightEnabled
                position: Qt.vector3d(-300, 600, -300)
                color: "blue"
                brightness: 50 * baseNode.lightsBrightnessMultiplier
                castsShadow: settings.pointLightShadow
                shadowFactor: settings.shadowFactor
                shadowBias: settings.shadowBias
                shadowMapQuality: settings.shadowMapQuality
            }
        }

        //! [fog volume]
        Fog3DVolume {
            id: fogSphere
            type: Fog3DVolume.Sphere
            extents: Qt.vector3d(5000, 5000, 5000)
            color: settings.fogVolumeColor
            density: settings.fogVolumeDensity
            noiseOffset: Qt.vector3d(baseNode.time * 0.1 * settings.fogSpeed,
                                     baseNode.time * 0.02 * settings.fogSpeed,
                                     baseNode.time * 0.01 * settings.fogSpeed)
            noiseScale: settings.fogVolumeNoiseScale
            heightEnabled: settings.fogVolumeHeightEnabled
            leastIntenseY: settings.fogVolumeHeightLeastY
            mostIntenseY: settings.fogVolumeHeightMostY
            heightCurve: settings.fogVolumeHeightCurve
        }
        //! [fog volume]

        Room { id: room; visible: settings.sceneVisible }

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton
            propagateComposedEvents: true
            onClicked: {
                settings.drawerVisible = false
            }
        }
    }

    OrbitCameraController {
        origin: camOrigin
        camera: camera
        ySpeed: 0.05
    }

    SettingsPane {
        id: settings
    }
}
