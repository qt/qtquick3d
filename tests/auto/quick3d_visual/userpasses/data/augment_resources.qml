// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Augment shader consuming GPU resources.
//
// A ComputePass writes a known texel into a ShaderImage and a known vec4 into
// a ShaderStorageBuffer. Both are then exposed to an AugmentMaterial pass as
// ShaderImage / ShaderStorageBuffer properties of the RenderPass
// itself, and read back by the augment shader with imageLoad() and a plain
// buffer index. The augment shader paints the sphere green only if both
// resources hold the written values, so an unbound resource (which reads as
// zero) shows up as red.
//
// Unlike renderpass_display.qml, which reaches the same resources through a
// CustomMaterial used as a SubRenderPass override material, here the RenderPass
// carries the properties directly -- that is the path the augment shader's own
// SHADER_IMAGE/STORAGE_BUFFER declarations resolve against.

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
            tonemapMode: SceneEnvironment.TonemapModeNone
        }

        PerspectiveCamera {
            position: Qt.vector3d(0, 0, 300)
        }

        ShaderImage {
            id: sharedImage
            width: 1
            height: 1
            format: ShaderImage.RGBA8
        }

        ShaderStorageBuffer {
            id: sharedBuffer
            size: 16 // one vec4
        }

        RenderPassTexture {
            id: outputTex
            format: RenderPassTexture.RGBA8
        }

        RenderPass {
            id: mainPass
            materialMode: RenderPass.AugmentMaterial
            augmentShader: "augment_resources_augment.glsl"
            clearColor: "black"

            // The property names have to match the names the augment shader
            // declares with SHADER_IMAGE/STORAGE_BUFFER.
            property ShaderImage checkImage: sharedImage
            property ShaderStorageBuffer checkBuffer: sharedBuffer

            commands: [
                ColorAttachment { target: outputTex },
                DepthStencilAttachment {}
            ]

            // Nested in the pass that consumes the resources, so it runs first.
            ComputePass {
                computeShader: "augment_resources_fill.comp"
                property ShaderImage fillImage: sharedImage
                property ShaderStorageBuffer fillBuffer: sharedBuffer
                commands: [
                    Dispatch { groupCountX: 1; groupCountY: 1; groupCountZ: 1 }
                ]
            }
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
