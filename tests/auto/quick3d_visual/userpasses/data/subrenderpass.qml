// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers

Item {
    width: 400
    height: 400

    View3D {
        id: view3D
        anchors.fill: parent
        renderOverrides: View3D.DisableInternalPasses

        environment: SceneEnvironment {
            backgroundMode: SceneEnvironment.Color
            clearColor: "yellow"  // Yellow environment
        }

        PerspectiveCamera {
            position: Qt.vector3d(0, 0, 300)
        }

        DirectionalLight {}

        RenderPassTexture {
            id: outputTexture
            format: RenderPassTexture.RGBA8
        }

        // Parent render pass with magenta clear color that calls a sub-render pass
        // Parent filters to nothing, child pass actually renders the cone
        // This proves SubRenderPass executes
        RenderPass {
            id: parentPass
            objectName: "parentPass"
            materialMode: RenderPass.OriginalMaterial
            clearColor: "magenta"  // Magenta background

            commands: [
                ColorAttachment { target: outputTexture },
                DepthStencilAttachment {},
                RenderablesFilter {
                    layerMask: ContentLayer.Layer0  // Parent filters to Layer0 (nothing)
                },
                SubRenderPass {
                    // Child pass renders Layer1 (the cone)
                    // This proves SubRenderPass executes
                    renderPass: RenderPass {
                        objectName: "childPass"
                        materialMode: RenderPass.OriginalMaterial
                        commands: [
                            RenderablesFilter {
                                layerMask: ContentLayer.Layer1  // Child renders Layer1
                            }
                        ]
                    }
                }
            ]
        }

        SimpleQuadRenderer {
            texture: Texture {
                textureProvider: RenderOutputProvider {
                    textureSource: RenderOutputProvider.UserPassTexture
                    renderPass: parentPass
                    attachmentSelector: RenderOutputProvider.Attachment0
                }
            }
        }

        Model {
            layers: ContentLayer.Layer1  // Rendered by child pass
            source: "#Cone"
            materials: PrincipledMaterial {
                baseColor: "cyan"
                lighting: PrincipledMaterial.NoLighting
            }
        }
    }
}
