// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// A producer pass declared as a QML child of its consumer. The nested pass
// gets nesting depth 1 and renders first, into its own render target; the
// consumer samples the result through a RenderOutputProvider.

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
        RenderPassTexture { id: consumerTexture; format: RenderPassTexture.RGBA8 }

        RenderPass {
            id: consumerPass
            objectName: "consumerPass"
            materialMode: RenderPass.OriginalMaterial
            clearColor: "black"
            commands: [
                ColorAttachment { target: consumerTexture },
                DepthStencilAttachment {},
                RenderablesFilter { layerMask: ContentLayer.Layer1 }
            ]

            // Nested producer: renders before the consumer.
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

        SimpleQuadRenderer {
            texture: Texture {
                textureProvider: RenderOutputProvider {
                    textureSource: RenderOutputProvider.UserPassTexture
                    renderPass: consumerPass
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

        // Sphere rendered by the consumer, textured with the producer's output.
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
    }
}
