// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers

// Creates 17 SubRenderPasses in a single main pass, exceeding the maximum of
// 16 user pass slots (QSSGUserRenderPassManager::maxUserPassSlots()).
// The 17th pass should be silently skipped with a warning emitted via qWarning.
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

        DefaultMaterial { id: mat; lighting: DefaultMaterial.NoLighting; diffuseColor: "green" }

        RenderPassTexture { id: colorBuffer; format: RenderPassTexture.RGBA16F }

        RenderPass {
            id: mainPass
            clearColor: Qt.rgba(0, 0, 0, 1)
            commands: [
                ColorAttachment { target: colorBuffer },
                DepthStencilAttachment { },
                RenderablesFilter { renderableTypes: RenderablesFilter.None },

                SubRenderPass { renderPass: RenderPass { materialMode: RenderPass.OverrideMaterial; overrideMaterial: mat } },
                SubRenderPass { renderPass: RenderPass { materialMode: RenderPass.OverrideMaterial; overrideMaterial: mat } },
                SubRenderPass { renderPass: RenderPass { materialMode: RenderPass.OverrideMaterial; overrideMaterial: mat } },
                SubRenderPass { renderPass: RenderPass { materialMode: RenderPass.OverrideMaterial; overrideMaterial: mat } },
                SubRenderPass { renderPass: RenderPass { materialMode: RenderPass.OverrideMaterial; overrideMaterial: mat } },
                SubRenderPass { renderPass: RenderPass { materialMode: RenderPass.OverrideMaterial; overrideMaterial: mat } },
                SubRenderPass { renderPass: RenderPass { materialMode: RenderPass.OverrideMaterial; overrideMaterial: mat } },
                SubRenderPass { renderPass: RenderPass { materialMode: RenderPass.OverrideMaterial; overrideMaterial: mat } },
                SubRenderPass { renderPass: RenderPass { materialMode: RenderPass.OverrideMaterial; overrideMaterial: mat } },
                SubRenderPass { renderPass: RenderPass { materialMode: RenderPass.OverrideMaterial; overrideMaterial: mat } },
                SubRenderPass { renderPass: RenderPass { materialMode: RenderPass.OverrideMaterial; overrideMaterial: mat } },
                SubRenderPass { renderPass: RenderPass { materialMode: RenderPass.OverrideMaterial; overrideMaterial: mat } },
                SubRenderPass { renderPass: RenderPass { materialMode: RenderPass.OverrideMaterial; overrideMaterial: mat } },
                SubRenderPass { renderPass: RenderPass { materialMode: RenderPass.OverrideMaterial; overrideMaterial: mat } },
                SubRenderPass { renderPass: RenderPass { materialMode: RenderPass.OverrideMaterial; overrideMaterial: mat } },
                SubRenderPass { renderPass: RenderPass { materialMode: RenderPass.OverrideMaterial; overrideMaterial: mat } },
                SubRenderPass { renderPass: RenderPass { materialMode: RenderPass.OverrideMaterial; overrideMaterial: mat } }
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
