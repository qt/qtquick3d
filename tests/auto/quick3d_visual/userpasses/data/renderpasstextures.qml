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
            clearColor: "black"  // Black environment
        }

        PerspectiveCamera {
            position: Qt.vector3d(0, 0, 300)
        }

        DirectionalLight {}

        // Test different texture formats
        RenderPassTexture {
            id: rgba16f
            format: RenderPassTexture.RGBA16F
        }

        RenderPassTexture {
            id: depth24
            format: RenderPassTexture.Depth24Stencil8
        }

        RenderPass {
            id: texturePass
            materialMode: RenderPass.OriginalMaterial
            clearColor: "darkBlue"  // Distinctive clear color to prove pass is used

            commands: [
                ColorAttachment { target: rgba16f },
                DepthTextureAttachment { target: depth24 }
            ]
        }

        SimpleQuadRenderer {
            texture: Texture {
                textureProvider: RenderOutputProvider {
                    textureSource: RenderOutputProvider.UserPassTexture
                    renderPass: texturePass
                    attachmentSelector: RenderOutputProvider.Attachment0
                }
            }
        }

        Model {
            source: "#Sphere"
            materials: PrincipledMaterial {
                baseColor: "yellow"
                metalness: 0.5
                roughness: 0.3
            }
        }
    }
}
