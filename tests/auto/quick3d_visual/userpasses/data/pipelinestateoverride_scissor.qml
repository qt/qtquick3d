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
            clearColor: "black"
        }

        PerspectiveCamera {
            position: Qt.vector3d(0, 0, 300)
        }

        DirectionalLight {
            eulerRotation.x: -30
        }

        RenderPassTexture {
            id: outputTexture
            format: RenderPassTexture.RGBA16F
        }

        RenderPassTexture {
            id: depthTexture
            format: RenderPassTexture.Depth24Stencil8
        }

        // Test PipelineStateOverride with scissor
        // Rendering is clipped to a 200x200 rectangle
        // The cube should only be visible within the scissor rect (bottom-left region)
        // Without scissor working, the cube would fill the entire view
        RenderPass {
            id: scissorPass
            objectName: "scissorPass"
            materialMode: RenderPass.OriginalMaterial
            clearColor: "black"

            commands: [
                ColorAttachment { target: outputTexture },
                DepthTextureAttachment { target: depthTexture },
                PipelineStateOverride {
                    usesScissor: true
                    scissor: Qt.rect(100, 100, 200, 200)
                }
            ]
        }

        SimpleQuadRenderer {
            texture: Texture {
                textureProvider: RenderOutputProvider {
                    textureSource: RenderOutputProvider.UserPassTexture
                    renderPass: scissorPass
                    attachmentSelector: RenderOutputProvider.Attachment0
                }
            }
        }

        // Large cyan cube that would normally fill the view
        // With scissor, only a portion should be visible
        Model {
            source: "#Cube"
            scale: Qt.vector3d(2, 2, 2)
            materials: PrincipledMaterial {
                baseColor: "cyan"
                lighting: PrincipledMaterial.NoLighting
            }
        }
    }
}
