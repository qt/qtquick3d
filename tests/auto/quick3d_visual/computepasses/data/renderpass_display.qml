// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Displaying a ComputePass-generated ShaderImage via a RenderPass instead
// of an Effect.
//
// Reuses the exact same ComputePass/ShaderImage/CustomMaterial trio as
// procedural_texture.qml (a compute-generated 2x2 red/green/blue/yellow
// pattern sampled via imageLoad() in a CustomMaterial's fragment shader),
// but instead of assigning that CustomMaterial directly to a Model's
// `materials` property, it's applied as a SubRenderPass's OverrideMaterial
// (the exact pattern proven in
// tests/auto/quick3d_visual/userpasses/data/subpass_overridematerial.qml):
// the RenderPass renders the rectangle into an offscreen RenderPassTexture
// with the pattern material substituted in, and a SimpleQuadRenderer then
// displays that captured texture full-screen via
// RenderOutputProvider.UserPassTexture. No Effect is involved anywhere in
// this scene.

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers

Item {
    width: 400
    height: 400

    View3D {
        anchors.fill: parent
        renderOverrides: View3D.DisableInternalPasses

        environment: SceneEnvironment {
            backgroundMode: SceneEnvironment.Color
            clearColor: "black"
            tonemapMode: SceneEnvironment.TonemapModeNone
        }

        PerspectiveCamera {
            position: Qt.vector3d(0, 0, 300)
        }

        ShaderImage {
            id: proceduralImageRes
            width: 2
            height: 2
            format: ShaderImage.RGBA8
        }

        CustomMaterial {
            id: patternMaterial
            shadingMode: CustomMaterial.Unshaded
            vertexShader: "procedural_material.vert"
            fragmentShader: "procedural_material.frag"
            property ShaderImage proceduralImage: proceduralImageRes
        }

        RenderPassTexture {
            id: colorBuffer
            format: RenderPassTexture.RGBA8
        }

        RenderPass {
            id: mainPass
            clearColor: "black"
            commands: [
                ColorAttachment { target: colorBuffer },
                DepthStencilAttachment { },
                SubRenderPass {
                    renderPass: RenderPass {
                        materialMode: RenderPass.OverrideMaterial
                        overrideMaterial: patternMaterial
                    }
                }
            ]

            // Nested in the pass that displays its output, so it runs first.
            ComputePass {
                computeShader: "procedural_pattern.comp"
                property ShaderImage targetImage: proceduralImageRes
                commands: [
                    Dispatch { groupCountX: 2; groupCountY: 2; groupCountZ: 1 }
                ]
            }
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
            source: "#Rectangle"
            scale: Qt.vector3d(2.5, 2.5, 2.5)
            materials: PrincipledMaterial {
                baseColor: "white"
            }
        }
    }
}
