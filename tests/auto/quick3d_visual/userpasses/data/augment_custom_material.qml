// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// An AugmentMaterial pass over a CustomMaterial.
//
// The augmentation is folded into the custom material's shader path key, and
// that key is also emitted as the "//Shader name -" comment of the generated
// shader. An augment shader is always more than one line long -- the license
// header alone sees to that -- so appending it to the key verbatim used to
// push everything after its first line out of the comment and into the shader
// as stray code, which then failed to compile.
//
// The material paints itself red and the augment shader overrides that with
// green, so a sphere that is not green means the augmentation never ran.

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

        RenderPassTexture {
            id: outputTex
            format: RenderPassTexture.RGBA8
        }

        RenderPass {
            id: mainPass
            materialMode: RenderPass.AugmentMaterial
            augmentShader: "augment_custom_material_augment.glsl"
            clearColor: "black"

            commands: [
                ColorAttachment { target: outputTex },
                DepthStencilAttachment {}
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

        Model {
            source: "#Sphere"
            scale: Qt.vector3d(3, 3, 3)
            materials: CustomMaterial {
                fragmentShader: "augment_custom_material_material.frag"
            }
        }
    }
}
