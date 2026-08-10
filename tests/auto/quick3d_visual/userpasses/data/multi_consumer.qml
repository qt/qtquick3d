// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// A producer nested inside consumerA (depth 1) renders before ALL depth-0
// passes, so consumerB, which also samples the producer's output, sees fresh
// results without expressing the dependency a second time.

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers

Item {
    width: 400
    height: 400

    View3D {
        anchors.fill: parent
        renderOverrides: View3D.DisableInternalPasses

        PerspectiveCamera { position: Qt.vector3d(0, 0, 300) }
        DirectionalLight {}

        RenderPassTexture { id: producerTexture; format: RenderPassTexture.RGBA8 }
        RenderPassTexture { id: consumerATexture; format: RenderPassTexture.RGBA8 }
        RenderPassTexture { id: consumerBTexture; format: RenderPassTexture.RGBA8 }

        RenderPass {
            id: consumerA
            objectName: "consumerA"
            materialMode: RenderPass.OriginalMaterial
            clearColor: "black"
            commands: [
                ColorAttachment { target: consumerATexture },
                DepthStencilAttachment {},
                RenderablesFilter { layerMask: ContentLayer.Layer1 }
            ]

            RenderPass {
                id: producerPass
                objectName: "producerPass"
                materialMode: RenderPass.OriginalMaterial
                clearColor: "yellow"
                commands: [
                    ColorAttachment { target: producerTexture },
                    DepthStencilAttachment {},
                    RenderablesFilter { layerMask: ContentLayer.Layer0 }
                ]
            }
        }

        RenderPass {
            id: consumerB
            objectName: "consumerB"
            materialMode: RenderPass.OriginalMaterial
            clearColor: "black"
            commands: [
                ColorAttachment { target: consumerBTexture },
                DepthStencilAttachment {},
                RenderablesFilter { layerMask: ContentLayer.Layer2 }
            ]
        }

        // Display consumerB's output: it contains the producer's result even
        // though the producer is nested in consumerA.
        SimpleQuadRenderer {
            texture: Texture {
                textureProvider: RenderOutputProvider {
                    textureSource: RenderOutputProvider.UserPassTexture
                    renderPass: consumerB
                }
            }
        }

        // Red cube rendered by the producer.
        Model {
            layers: ContentLayer.Layer0
            source: "#Cube"
            materials: PrincipledMaterial {
                baseColor: "red"
                lighting: PrincipledMaterial.NoLighting
            }
        }

        // Sphere for consumerA, textured with the producer's output.
        Model {
            layers: ContentLayer.Layer1
            source: "#Sphere"
            scale: Qt.vector3d(2, 2, 2)
            materials: PrincipledMaterial {
                lighting: PrincipledMaterial.NoLighting
                baseColorMap: Texture {
                    textureProvider: RenderOutputProvider {
                        textureSource: RenderOutputProvider.UserPassTexture
                        renderPass: producerPass
                    }
                }
            }
        }

        // Sphere for consumerB, also textured with the producer's output.
        Model {
            layers: ContentLayer.Layer2
            source: "#Sphere"
            scale: Qt.vector3d(2, 2, 2)
            materials: PrincipledMaterial {
                lighting: PrincipledMaterial.NoLighting
                baseColorMap: Texture {
                    textureProvider: RenderOutputProvider {
                        textureSource: RenderOutputProvider.UserPassTexture
                        renderPass: producerPass
                    }
                }
            }
        }
    }
}
