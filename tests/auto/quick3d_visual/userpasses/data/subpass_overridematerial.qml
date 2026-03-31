// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers

// A sphere with a blue PrincipledMaterial rendered through a SubRenderPass
// whose renderPass uses OverrideMaterial with a red NoLighting DefaultMaterial.
// The center pixel should be red, not blue.

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
            id: redMat
            lighting: DefaultMaterial.NoLighting
            diffuseColor: "red"
        }

        RenderPassTexture { id: colorBuffer; format: RenderPassTexture.RGBA16F }

        RenderPass {
            id: mainPass
            clearColor: Qt.rgba(0, 0, 0, 1)
            commands: [
                ColorAttachment { target: colorBuffer },
                DepthStencilAttachment { },
                SubRenderPass {
                    renderPass: RenderPass {
                        materialMode: RenderPass.OverrideMaterial
                        overrideMaterial: redMat
                        commands: [
                            PipelineStateOverride { depthTestEnabled: false }
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
