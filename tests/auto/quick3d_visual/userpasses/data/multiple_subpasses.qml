// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers

// Two SubRenderPasses in the same main pass, each with a different override
// material and targeting a different layer:
//   SubPass1: red NoLighting override for Layer0 (cube at x=-150)
//   SubPass2: blue NoLighting override for Layer1 (sphere at x=+150)
// If both sub-passes acquire unique userPassData slots the left side should
// be red and the right side blue. If they share the same slot, the second
// pass would overwrite the first and both sides would be the same color.

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
