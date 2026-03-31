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

        // Output texture
        RenderPassTexture {
            id: outputTexture
            format: RenderPassTexture.RGBA8
        }

        RenderPassTexture {
            id: depthTexture
            format: RenderPassTexture.Depth24Stencil8
        }

        // Render pass with a DIFFERENT clear color than environment
        // This proves the user pass is actually being used
        RenderPass {
            id: basicPass
            objectName: "basicPass"
            materialMode: RenderPass.OriginalMaterial
            clearColor: "blue"  // Different from environment's black

            commands: [
                ColorAttachment { target: outputTexture },
                DepthStencilAttachment {}
            ]
        }

        // Display the output
        SimpleQuadRenderer {
            texture: Texture {
                textureProvider: RenderOutputProvider {
                    textureSource: RenderOutputProvider.UserPassTexture
                    renderPass: basicPass
                    attachmentSelector: RenderOutputProvider.Attachment0
                }
            }
        }

        Model {
            source: "#Cube"
            materials: PrincipledMaterial {
                baseColor: "red"
            }
        }
    }
}
