// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Array texture binding in an AugmentMaterial pass.
//
// A sampler2DArray is declared by giving the texture a provider with a
// Sampler2DArray sampler hint. The provider has no source, so it never hands
// out a texture and the sampler stays unbound, which means the pass has to
// fill it with the engine's dummy texture. That dummy has to be a texture
// array too: binding a plain 2D texture to a sampler2DArray is a view type
// mismatch, which graphics APIs either reject or read back as garbage.
//
// The sampler hint is only honoured for CustomMaterial properties, so the
// check lives in the material's own shader and the augment shader forwards
// the verdict. The bindings still go through rhiPrepareAugmentedUserPass().

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
            augmentShader: "augment_array_texture_binding_augment.glsl"
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
                fragmentShader: "augment_array_texture_binding_material.frag"

                // Declared as a sampler2DArray because of the hint, but with
                // textureSource left at None the provider produces nothing, so
                // there is no texture to bind.
                property TextureInput arrayMap: TextureInput {
                    texture: Texture {
                        textureProvider: RenderOutputProvider {
                            samplerHint: TextureProviderExtension.Sampler2DArray
                        }
                    }
                }
            }
        }
    }
}
