// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// PipelineStateOverride depthTestEnabled test.
// Two overlapping spheres: blue (closer, z=100) and red (farther, z=-100).
// Opaque objects are sorted front-to-back, so blue draws first, red draws second.
// With depthTestEnabled: false (no GPU depth test), the last-drawn object wins
// → red (drawn second) is visible at the center.
// With depthTestEnabled: true (default), the depth buffer keeps the closer object
// → blue (z=100, closer to camera at z=300) is visible at the center.
// The C++ test changes depthTestEnabled at runtime to verify the transition.

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers

Item {
    id: root
    width: 400
    height: 400

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
                    objectName: "pso"
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

        // Blue sphere closer to camera (z=100, distance to camera=200)
        // Front-to-back sort: drawn FIRST
        Model {
            source: "#Sphere"
            position: Qt.vector3d(0, 0, 100)
            scale: Qt.vector3d(3, 3, 3)
            materials: PrincipledMaterial {
                baseColor: "blue"
                lighting: PrincipledMaterial.NoLighting
            }
        }

        // Red sphere farther from camera (z=-100, distance to camera=400)
        // Front-to-back sort: drawn SECOND
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
