// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// PipelineStateOverride blendEnabled test.
// The scene has a blue clear colour and a white sphere.
// An AugmentMaterial augment shader sets the sphere fragment output to
// vec4(1.0, 1.0, 1.0, 0.5) — white at 50% alpha.
// With blendEnabled: true and SrcAlpha blend mode, the fragment blends
// with the blue clear colour to produce a mix of white and blue (~cyan/light blue).
// With blendEnabled: false the sphere renders as fully opaque white regardless
// of the alpha value in the output (no blending occurs).

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
            materialMode: RenderPass.AugmentMaterial
            augmentShader: "blendenabled_augment.glsl"
            // Blue clear colour: the blend result must differ from both pure
            // white (blending off) and pure blue (clear only).
            clearColor: "blue"

            commands: [
                ColorAttachment { target: colorTex },
                DepthStencilAttachment {},
                PipelineStateOverride {
                    objectName: "pso"
                    blendEnabled: true
                    targetBlend0.enable: true
                    targetBlend0.srcColor: RenderTargetBlend.SrcAlpha
                    targetBlend0.dstColor: RenderTargetBlend.OneMinusSrcAlpha
                    targetBlend0.srcAlpha: RenderTargetBlend.One
                    targetBlend0.dstAlpha: RenderTargetBlend.OneMinusSrcAlpha
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

        // Large sphere to fill most of the screen
        Model {
            source: "#Sphere"
            scale: Qt.vector3d(3, 3, 3)
            materials: PrincipledMaterial {
                baseColor: "white"
                lighting: PrincipledMaterial.NoLighting
            }
        }
    }
}
