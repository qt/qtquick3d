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

        // G-buffer textures
        RenderPassTexture {
            id: gbuffer0
            format: RenderPassTexture.RGBA16F
        }

        RenderPassTexture {
            id: gbuffer1
            format: RenderPassTexture.RGBA16F
        }

        RenderPassTexture {
            id: gbuffer2
            format: RenderPassTexture.RGBA16F
        }

        RenderPassTexture {
            id: depthTexture
            format: RenderPassTexture.Depth24Stencil8
        }

        // Multiple render targets test - ONE pass writing to MULTIPLE attachments
        // Uses AugmentMaterial to write different colors to each attachment
        RenderPass {
            id: mrtPass
            objectName: "mrtPass"
            materialMode: RenderPass.AugmentMaterial
            augmentShader: "mrt_augment.glsl"
            clearColor: "black"

            commands: [
                ColorAttachment { target: gbuffer0; name: "GBUFFER0" },
                ColorAttachment { target: gbuffer1; name: "GBUFFER1" },
                ColorAttachment { target: gbuffer2; name: "GBUFFER2" },
                DepthTextureAttachment { target: depthTexture }
            ]
        }

        // Display attachment 1 (GBUFFER1 = yellow) to prove MRT works
        // If MRT wasn't working, we'd see attachment 0 (red) instead
        SimpleQuadRenderer {
            texture: Texture {
                textureProvider: RenderOutputProvider {
                    textureSource: RenderOutputProvider.UserPassTexture
                    renderPass: mrtPass
                    attachmentSelector: RenderOutputProvider.Attachment1  // Display GBUFFER1
                }
            }
        }

        Model {
            source: "#Cube"
            materials: PrincipledMaterial {
                baseColor: "white"
                lighting: PrincipledMaterial.NoLighting
            }
        }
    }
}
