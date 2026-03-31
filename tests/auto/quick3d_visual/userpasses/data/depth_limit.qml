// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers

// Creates 17 levels of nested SubRenderPasses, exceeding MAX_SUBPASS_DEPTH (16).
// The 17th level should be silently skipped with a warning emitted via qWarning.
// The test verifies no crash occurs.

Rectangle {
    width: 100
    height: 100
    color: "black"

    View3D {
        anchors.fill: parent
        renderOverrides: View3D.DisableInternalPasses

        camera: camera

        PerspectiveCamera {
            id: camera
            position: Qt.vector3d(0, 0, 600)
        }

        DefaultMaterial { id: mat; lighting: DefaultMaterial.NoLighting; diffuseColor: "cyan" }

        RenderPassTexture { id: colorBuffer; format: RenderPassTexture.RGBA16F }

        // depth 17 (innermost, should be warned and skipped)
        RenderPass {
            id: pass17
            materialMode: RenderPass.OverrideMaterial
            overrideMaterial: mat
        }
        RenderPass {
            id: pass16
            commands: [ SubRenderPass { renderPass: pass17 } ]
        }
        RenderPass {
            id: pass15
            commands: [ SubRenderPass { renderPass: pass16 } ]
        }
        RenderPass {
            id: pass14
            commands: [ SubRenderPass { renderPass: pass15 } ]
        }
        RenderPass {
            id: pass13
            commands: [ SubRenderPass { renderPass: pass14 } ]
        }
        RenderPass {
            id: pass12
            commands: [ SubRenderPass { renderPass: pass13 } ]
        }
        RenderPass {
            id: pass11
            commands: [ SubRenderPass { renderPass: pass12 } ]
        }
        RenderPass {
            id: pass10
            commands: [ SubRenderPass { renderPass: pass11 } ]
        }
        RenderPass {
            id: pass9
            commands: [ SubRenderPass { renderPass: pass10 } ]
        }
        RenderPass {
            id: pass8
            commands: [ SubRenderPass { renderPass: pass9 } ]
        }
        RenderPass {
            id: pass7
            commands: [ SubRenderPass { renderPass: pass8 } ]
        }
        RenderPass {
            id: pass6
            commands: [ SubRenderPass { renderPass: pass7 } ]
        }
        RenderPass {
            id: pass5
            commands: [ SubRenderPass { renderPass: pass6 } ]
        }
        RenderPass {
            id: pass4
            commands: [ SubRenderPass { renderPass: pass5 } ]
        }
        RenderPass {
            id: pass3
            commands: [ SubRenderPass { renderPass: pass4 } ]
        }
        RenderPass {
            id: pass2
            commands: [ SubRenderPass { renderPass: pass3 } ]
        }

        RenderPass {
            id: mainPass
            clearColor: Qt.rgba(0, 0, 0, 1)
            commands: [
                ColorAttachment { target: colorBuffer },
                DepthStencilAttachment { },
                SubRenderPass { renderPass: pass2 }   // depth 1
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
            materials: PrincipledMaterial { baseColor: "yellow" }
        }
    }
}
