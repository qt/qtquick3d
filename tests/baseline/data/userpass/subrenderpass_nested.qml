// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers

// Tests that nested SubRenderPasses (a SubRenderPass whose renderPass itself
// contains a SubRenderPass) each acquire unique userPassData slots.
// The outer pass renders the sphere with its original blue material.
// The inner pass re-renders it with a cyan NoLighting override on top
// (depth write disabled so it always composites over the outer output).
// Expected result: sphere appears cyan, proving the inner sub-pass
// got a unique slot and its override material was applied correctly.

Rectangle {
    width: 400
    height: 400
    color: "black"

    View3D {
        anchors.fill: parent
        renderOverrides: View3D.DisableInternalPasses

        camera: camera

        PerspectiveCamera {
            id: camera
            position: Qt.vector3d(0, 0, 600)
        }

        DefaultMaterial {
            id: cyanMat
            lighting: DefaultMaterial.NoLighting
            diffuseColor: "cyan"
        }

        RenderPassTexture { id: colorBuffer; format: RenderPassTexture.RGBA16F }

        RenderPass {
            id: mainPass
            clearColor: Qt.rgba(0, 0, 0, 1)
            commands: [
                ColorAttachment { target: colorBuffer },
                DepthStencilAttachment { },
                RenderablesFilter { renderableTypes: RenderablesFilter.None },

                SubRenderPass {
                    renderPass: RenderPass {
                        // Outer pass: render sphere with its original blue material
                        materialMode: RenderPass.OriginalMaterial
                        commands: [
                            // Inner pass: override with cyan, no depth write so it
                            // always draws on top of the outer pass output
                            SubRenderPass {
                                renderPass: RenderPass {
                                    materialMode: RenderPass.OverrideMaterial
                                    overrideMaterial: cyanMat
                                    commands: [
                                        PipelineStateOverride {
                                            depthTestEnabled: false
                                            depthWriteEnabled: false
                                        }
                                    ]
                                }
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
                    renderPass: mainPass
                }
            }
        }

        Model {
            source: "#Sphere"
            scale: Qt.vector3d(1.5, 1.5, 1.5)
            materials: PrincipledMaterial { baseColor: "blue" }
        }
    }
}
