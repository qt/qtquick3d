// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers

// Tests that two SubRenderPasses in the same main pass each acquire a unique
// userPassData slot. The cube on Layer0 should appear red (SubPass1 override)
// and the sphere on Layer1 should appear blue (SubPass2 override).
// If the index fix is broken both would use the same slot and one color would
// overwrite the other.

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
            position: Qt.vector3d(0, 0, 400)
            fieldOfView: 90
        }

        DefaultMaterial { id: redMat; lighting: DefaultMaterial.NoLighting; diffuseColor: "red" }
        DefaultMaterial { id: blueMat; lighting: DefaultMaterial.NoLighting; diffuseColor: "blue" }

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
                        materialMode: RenderPass.OverrideMaterial
                        overrideMaterial: redMat
                        commands: [
                            RenderablesFilter { layerMask: ContentLayer.Layer0 }
                        ]
                    }
                },

                SubRenderPass {
                    renderPass: RenderPass {
                        materialMode: RenderPass.OverrideMaterial
                        overrideMaterial: blueMat
                        commands: [
                            RenderablesFilter { layerMask: ContentLayer.Layer1 }
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
            source: "#Cube"
            x: -150
            scale: Qt.vector3d(2, 2, 2)
            eulerRotation.y: 30
            layers: ContentLayer.Layer0
            materials: PrincipledMaterial { baseColor: "yellow" }
        }

        Model {
            source: "#Sphere"
            x: 150
            scale: Qt.vector3d(2, 2, 2)
            layers: ContentLayer.Layer1
            materials: PrincipledMaterial { baseColor: "yellow" }
        }
    }
}
