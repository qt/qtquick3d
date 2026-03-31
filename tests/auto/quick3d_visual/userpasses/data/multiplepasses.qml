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
            clearColor: "blue"
        }

        PerspectiveCamera {
            position: Qt.vector3d(0, 0, 300)
        }

        DirectionalLight {}

        RenderPassTexture {
            id: pass1Texture
            format: RenderPassTexture.RGBA8
        }

        RenderPassTexture {
            id: pass2Texture
            format: RenderPassTexture.RGBA8
        }

        // First pass renders a red cube to pass1Texture with yellow background
        RenderPass {
            id: pass1
            objectName: "pass1"
            materialMode: RenderPass.OriginalMaterial
            clearColor: "yellow"  // Yellow background
            commands: [
                ColorAttachment { target: pass1Texture },
                DepthStencilAttachment {},
                RenderablesFilter {
                    layerMask: ContentLayer.Layer0  // Only render cube
                }
            ]
        }

        // Second pass renders a sphere with pass1's output as its texture
        // This proves the passes are chaining correctly
        RenderPass {
            id: pass2
            objectName: "pass2"
            materialMode: RenderPass.OriginalMaterial
            clearColor: "cyan"  // Cyan background
            commands: [
                ColorAttachment { target: pass2Texture },
                DepthStencilAttachment {},
                RenderablesFilter {
                    layerMask: ContentLayer.Layer1  // Only render sphere
                }
            ]
        }

        // Display the output from pass2
        SimpleQuadRenderer {
            texture: Texture {
                textureProvider: RenderOutputProvider {
                    textureSource: RenderOutputProvider.UserPassTexture
                    renderPass: pass2
                    attachmentSelector: RenderOutputProvider.Attachment0
                }
            }
        }

        // Red cube on Layer0, rendered in pass1
        Model {
            layers: ContentLayer.Layer0
            source: "#Cube"
            position: Qt.vector3d(-50, 0, 0)
            materials: PrincipledMaterial {
                baseColor: "red"
            }
        }

        // Sphere on Layer1, rendered in pass2 with pass1's output as texture
        Model {
            layers: ContentLayer.Layer1
            source: "#Sphere"
            position: Qt.vector3d(50, 0, 0)
            materials: PrincipledMaterial {
                baseColorMap: Texture {
                    textureProvider: RenderOutputProvider {
                        textureSource: RenderOutputProvider.UserPassTexture
                        renderPass: pass1
                        attachmentSelector: RenderOutputProvider.Attachment0
                    }
                }
            }
        }
    }
}
