// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// PipelineStateOverride blendEnabled + targetBlend0 test.
// A white sphere is rendered with an AugmentMaterial augment shader that outputs
// vec4(1.0, 1.0, 1.0, 0.5) — white at 50 % alpha.
// With blendEnabled: true and SrcAlpha/OneMinusSrcAlpha blending against the blue
// clear colour the expected result is a mix of white and blue.

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
            materialMode: RenderPass.AugmentMaterial
            augmentShader: "blendenabled_augment.glsl"
            clearColor: "blue"

            commands: [
                ColorAttachment { target: colorTex },
                DepthStencilAttachment {},
                PipelineStateOverride {
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

        // Large white sphere to fill most of the screen
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
