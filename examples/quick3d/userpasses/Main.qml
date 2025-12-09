// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick3D
import QtQuick3D.Helpers

ApplicationWindow {
    width: 1280
    height: 720
    visible: true
    title: qsTr("User Passes")



    // ![disable internal passes]
    View3D {
        id: view3D
        anchors.fill: parent
        renderOverrides: View3D.DisableInternalPasses
        environment: SceneEnvironment {
            lightProbe: Texture {
                textureData: ProceduralSkyTextureData {
                }
            }
            backgroundMode: SceneEnvironment.SkyBox
        }
    // ![disable internal passes]

        //! [main texture]
        SimpleQuadRenderer {
            texture: Texture {
                textureProvider: mainColorTextureProvider
            }
        }

        RenderPassTexture {
            id: mainColorTexture
            format: RenderPassTexture.RGBA16F
        }

        RenderOutputProvider {
            id: mainColorTextureProvider
            textureSource: RenderOutputProvider.UserPassTexture
            renderPass: deferredLightingPass
            attachmentSelector: RenderOutputProvider.Attachment0
        }
        //! [main texture]

        //! [depth texture]
        RenderPassTexture {
            id: mainDepthStencilTexture
            format: RenderPassTexture.Depth24Stencil8
        }
        //! [depth texture]

        Node {
            id: originNode
            y: 180

            PerspectiveCamera {
                id: cameraNode
                z: 300
            }
        }

        DirectionalLight {
            eulerRotation.x: -45
            castsShadow: true
        }

        // ![deferred lighting quad]
        Model {
            id: deferredLightingQuad
            layers: ContentLayer.Layer13
            castsShadows: false
            receivesShadows: false
            geometry: PlaneGeometry {
                // geometry doesn't matter, just need 4 verts
                plane: PlaneGeometry.XY
            }
            materials: [
                CustomMaterial {
                    id: lightingPassMaterial
                    property TextureInput gbuffer0: TextureInput {
                        enabled: true
                        texture: Texture {
                            textureProvider: gbuffer0Provider
                        }
                    }
                    property TextureInput gbuffer1: TextureInput {
                        enabled: true
                        texture: Texture {
                            textureProvider: gbuffer1Provider
                        }
                    }
                    property TextureInput gbuffer2: TextureInput {
                        enabled: true
                        texture: Texture {
                            textureProvider: gbuffer2Provider
                        }
                    }
                    shadingMode: CustomMaterial.Unshaded
                    fragmentShader: "lighting.frag"
                    vertexShader: "lighting.vert"
                }
            ]
        }

        RenderPass {
            id: deferredLightingPass

            readonly property Texture gbuffer0: Texture { textureProvider: gbuffer0Provider }
            readonly property Texture gbuffer1: Texture { textureProvider: gbuffer1Provider }
            readonly property Texture gbuffer2: Texture { textureProvider: gbuffer2Provider }

            materialMode: RenderPass.OriginalMaterial

            commands: [
                ColorAttachment { target: mainColorTexture },
                DepthStencilAttachment {},
                RenderablesFilter { layerMask: ContentLayer.Layer13 }
            ]

        }
        // ![deferred lighting quad]

        // ![GBufferPass usage]
        GBufferPass {
            id: gbufferPass
            layerMask: ContentLayer.Layer0 | ContentLayer.Layer1
            depthTexture: mainDepthStencilTexture
        }

        RenderOutputProvider {
            id: gbuffer0Provider
            textureSource: RenderOutputProvider.UserPassTexture
            renderPass: gbufferPass
            attachmentSelector: RenderOutputProvider.Attachment0
        }

        RenderOutputProvider {
            id: gbuffer1Provider
            textureSource: RenderOutputProvider.UserPassTexture
            renderPass: gbufferPass
            attachmentSelector: RenderOutputProvider.Attachment1
        }

        RenderOutputProvider {
            id: gbuffer2Provider
            textureSource: RenderOutputProvider.UserPassTexture
            renderPass: gbufferPass
            attachmentSelector: RenderOutputProvider.Attachment2
        }
        // ![GBufferPass usage]

        Model {
            id: donut
            layers: ContentLayer.Layer1
            y: 100
            z: 4
            geometry: TorusGeometry {
                radius: 100
                tubeRadius: 30
                segments: 64
                rings: 32
            }
            materials: [
                PrincipledMaterial {
                    baseColor: "#ffcc00"
                    metalness: 0.0
                    roughness: 0.5
                }
            ]

            NumberAnimation on eulerRotation.y {
                from: 0
                to: 360
                duration: 8000
                loops: Animation.Infinite
                running: true
            }
        }

        Model {
            id: ground
            layers: ContentLayer.Layer0
            geometry: PlaneGeometry {
                width: 1000
                height: 1000
                plane: PlaneGeometry.XZ
            }
            materials: [
                PrincipledMaterial {
                    baseColor: "brown"
                    metalness: 0.0
                    roughness: 0.5
                }

            ]
        }

        OrbitCameraController {
            origin: originNode
            camera: cameraNode
        }
    }

}
