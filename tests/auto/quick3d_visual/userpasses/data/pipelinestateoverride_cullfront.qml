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
            eulerRotation.y: 45
        }

        RenderPassTexture {
            id: outputTexture
            format: RenderPassTexture.RGBA16F
        }

        RenderPassTexture {
            id: depthTexture
            format: RenderPassTexture.Depth24Stencil8
        }

        // Test PipelineStateOverride with cullMode = Front
        // This culls front faces and shows back faces (inside of cube)
        // With lighting, back faces will appear darker/different than front faces
        // Without PipelineStateOverride working, we'd see normal front faces
        RenderPass {
            id: cullFrontPass
            objectName: "cullFrontPass"
            materialMode: RenderPass.OriginalMaterial
            clearColor: "black"

            commands: [
                ColorAttachment { target: outputTexture },
                DepthTextureAttachment { target: depthTexture },
                PipelineStateOverride {
                    cullMode: PipelineStateOverride.Front
                }
            ]
        }

        SimpleQuadRenderer {
            texture: Texture {
                textureProvider: RenderOutputProvider {
                    textureSource: RenderOutputProvider.UserPassTexture
                    renderPass: cullFrontPass
                    attachmentSelector: RenderOutputProvider.Attachment0
                }
            }
        }

        // White cube with lighting
        // Back faces will be darker than front faces due to lighting direction
        Model {
            source: "#Cube"
            materials: PrincipledMaterial {
                baseColor: "white"
                metalness: 0.0
                roughness: 0.8
            }
        }
    }
}
