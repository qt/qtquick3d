// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick3D
import QtQuick3D.Helpers
import Example

ApplicationWindow {
    id: window
    visible: true
    width: 1200
    height: 720
    title: qsTr("Volumetric Fog Example")

    //! [scene]
    View3D {
        id: view
        x: settings.viewX
        y: 0
        width: parent.width - settings.viewX
        height: parent.height
        camera: camera
        environment: SceneEnvironment {
            id: se
            clearColor: "darkgray"
            backgroundMode: SceneEnvironment.Color
            antialiasingMode: SceneEnvironment.MSAA
            antialiasingQuality: SceneEnvironment.High
            property real time: 0.0
            NumberAnimation on time {
                from: 0.0
                to: 100000.0
                duration: 100000
                loops: NumberAnimation.Infinite
                running: true
            }

            effects: [
                Effect {
                    property ShaderImage froxelImage: froxelImage

                    property real jitterIntensity : 0.1
                    property vector3d gridDimensions : Qt.vector3d(froxelImage.width, froxelImage.height, froxelImage.depth)

                    property TextureInput blueNoise: TextureInput {
                            texture: Texture {
                                minFilter: Texture.Linear
                                magFilter: Texture.Linear
                                tilingModeHorizontal: Texture.Repeat
                                tilingModeVertical: Texture.Repeat
                                generateMipmaps: false
                                source: "blue_noise.png"
                            }
                        }

                    passes: Pass {
                        shaders: Shader {
                            stage: Shader.Fragment
                            shader: "volumetricLight.frag"
                        }
                    }
                }
            ]
        }

        ShaderImage {

            function nextMultipleOf8(n) {
                return Math.ceil(n / 8) * 8
            }

            id: froxelImage
            // settings.quality is in range [0.0, 1.0]
            // width is in range [80, 240]
            // height is in range [48, 144]
            // depth is in range [32, 96]
            width: nextMultipleOf8(160 * (settings.quality + 0.5))
            height: nextMultipleOf8(96 * (settings.quality + 0.5))
            depth: Math.ceil(64 * (settings.quality + 0.5))
            shape: ShaderImage.Image3D
            format: ShaderImage.RGBA32F
        }

        ComputePass {
            id: compute
            commands: [
                Dispatch {
                    // The local size is 8x8x1, and width and height are
                    // multiples of 8.
                    groupCountX: froxelImage.width / 8
                    groupCountY: froxelImage.height / 8
                    groupCountZ: froxelImage.depth
                }
            ]
            property ShaderImage froxelTexture : froxelImage
            property vector3d gridDimensions : Qt.vector3d(froxelImage.width, froxelImage.height, froxelImage.depth)
            property real useExponentialDepth : 0.0
            property vector3d fogPos : Qt.vector3d(se.time * 0.1 * settings.fogSpeed, 0, 0)
            property real fogScale : settings.fogScale
            property real fogDensity : settings.fogDensity
            computeShader: "compute_shader.comp"
        }

        PerspectiveCamera {
            id: camera
            position: Qt.vector3d(0, 300, 0)
            eulerRotation: Qt.vector3d(0, 0, 0)
            clipFar: settings.cameraFar
        }

        DirectionalLight {
            eulerRotation: Qt.vector3d(0, 180, 0)
            brightness: 100
            visible: true
            castsShadow: true
            shadowFactor: 75
            csmSplit1: 0.15
            csmSplit2: 0.5
            csmSplit3: 0.75
            csmNumSplits: 2
            shadowMapQuality: Light.ShadowMapQualityVeryHigh
            csmBlendRatio: 0.05
            shadowBias: 10
            pcfFactor: 3
            softShadowQuality: Light.PCF8
            shadowMapFar: camera.clipFar
            lockShadowmapTexels: false
        }

        Model {
            id: ground
            source: "#Cube"
            scale: Qt.vector3d(25, 0.01, 135)
            z: -5500
            materials: PrincipledMaterial {
                baseColor: "gray"
            }
            castsShadows: false
        }

        // Animated boxes
        Path3D {
            id: boxes
            z: -1100
            Repeater3D {
                model: 16
                Model {
                    source: "#Cube"
                    position: Qt.vector3d(0, 0, 0)
                    materials: [DefaultMaterial {
                            diffuseColor: "darkGray"
                    }]
                    castsShadows: true
                }
            }
            property int segmentCount: 128
            pathDuration: 8000 * 2
            positions: circlePath(Qt.vector3d(0, 500, 0), 500, Qt.vector3d(0,0,0), boxes.segmentCount)
        }
        // Spheres
        Path3D {
            id: spheres
            z: -1100
            y: -500
            Repeater3D {
                model: 8
                Model {
                    source: "#Sphere"
                    position: Qt.vector3d(0, 0, 0)
                    materials: [DefaultMaterial {
                            diffuseColor: "gray"
                    }]
                    castsShadows: true
                }
            }
            property int segmentCount: 64
            pathDuration: 4000 * 2
            positions: circlePath(Qt.vector3d(0, 500, 0), 250, Qt.vector3d(0,180,0), spheres.segmentCount)
        }

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton
            propagateComposedEvents: true
            onClicked: {
                settings.drawerVisible = false
                wasd.forceActiveFocus() // focus WASD controller for keyboard input
            }
        }
    }
    //! [scene]

    WasdController {
        id: wasd
        controlledObject: view.camera
        speed: 5
        shiftSpeed: 10
        enabled: !settings.drawerOpen
    }


    SettingsPane {
        id: settings
    }

}
