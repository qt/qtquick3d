// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// PipelineStateOverride depthTestEnabled: false test.
// Two overlapping spheres: blue closer (z=100), red farther (z=-100).
// Opaque objects are sorted front-to-back: blue draws first, red draws second.
// With depthTestEnabled: false the last drawn (red) overwrites the first (blue)
// at the centre → expected centre pixel ≈ red.

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers

Rectangle {
    width: 400
    height: 400
    color: "black"

    View3D {
        anchors.fill: parent
        renderOverrides: View3D.DisableInternalPasses

        environment: SceneEnvironment {
            backgroundMode: SceneEnvironment.Color
            clearColor: "black"
        }

        PerspectiveCamera {
            position: Qt.vector3d(0, 0, 300)
        }

        RenderPassTexture {
            id: colorTex
            format: RenderPassTexture.RGBA8
        }

        RenderPass {
            id: mainPass
            materialMode: RenderPass.OriginalMaterial
            clearColor: "black"

            commands: [
                ColorAttachment { target: colorTex },
                DepthStencilAttachment {},
                PipelineStateOverride {
                    depthTestEnabled: false
                }
            ]
        }

        SimpleQuadRenderer {
            texture: Texture {
                textureProvider: RenderOutputProvider {
                    textureSource: RenderOutputProvider.UserPassTexture
                    renderPass: mainPass
                    attachmentSelector: RenderOutputProvider.Attachment0
                }
            }
        }

        // Blue sphere closer to camera (drawn first, front-to-back sort)
        Model {
            source: "#Sphere"
            position: Qt.vector3d(0, 0, 100)
            scale: Qt.vector3d(3, 3, 3)
            materials: PrincipledMaterial {
                baseColor: "blue"
                lighting: PrincipledMaterial.NoLighting
            }
        }

        // Red sphere farther from camera (drawn second, overwrites blue without depth test)
        Model {
            source: "#Sphere"
            position: Qt.vector3d(0, 0, -100)
            scale: Qt.vector3d(3, 3, 3)
            materials: PrincipledMaterial {
                baseColor: "red"
                lighting: PrincipledMaterial.NoLighting
            }
        }
    }
}
