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

        // Test PipelineStateOverride with polygonMode set to Line (wireframe)
        // This should render only the edges of the cube, not filled faces
        // Without PipelineStateOverride working, it would render as a filled cube
        RenderPass {
            id: wireframePass
            objectName: "wireframePass"
            materialMode: RenderPass.OriginalMaterial
            clearColor: "black"  // Black background so edges stand out

            commands: [
                ColorAttachment { target: outputTexture },
                DepthTextureAttachment { target: depthTexture },
                PipelineStateOverride {
                    polygonMode: PipelineStateOverride.Line
                }
            ]
        }

        SimpleQuadRenderer {
            texture: Texture {
                textureProvider: RenderOutputProvider {
                    textureSource: RenderOutputProvider.UserPassTexture
                    renderPass: wireframePass
                    attachmentSelector: RenderOutputProvider.Attachment0
                }
            }
        }

        Model {
            source: "#Cube"
            scale: Qt.vector3d(1.5, 1.5, 1.5)
            materials: PrincipledMaterial {
                baseColor: "white"
                lighting: PrincipledMaterial.NoLighting
            }
        }
    }
}
