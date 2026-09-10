// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Texture binding in an AugmentMaterial pass.
//
// The RenderPass carries two Texture properties. Both are declared as samplers
// in the generated shader, because the property exists in either case, but
// only boundMap resolves to a GPU texture: unboundMap has no source, so
// nothing binds it and the pass has to fill it with the engine's dummy texture
// to keep the shader resource bindings complete.
//
// The augment shader samples both and only paints the sphere green if the
// bound one still reads its real colour and the unbound one reads the dummy.
// Getting that backwards -- filling a sampler that was already bound -- turns
// the sphere red.

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
            augmentShader: "augment_texture_binding_augment.glsl"
            clearColor: "black"

            // Resolves to a real texture.
            property Texture boundMap: Texture {
                sourceItem: Rectangle {
                    width: 8
                    height: 8
                    color: "#00ff00"
                }
            }

            // Declared, but with no source there is no GPU texture to bind.
            property Texture unboundMap: Texture { }

            // Same, but a cube map: the property type stays Texture so the
            // pass picks it up, while the CubeMapTexture instance is what makes
            // the generated shader declare a samplerCube. It has to end up with
            // the dummy cube map, all six faces of it.
            property Texture unboundCube: CubeMapTexture { }

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
            materials: PrincipledMaterial {
                baseColor: "white"
                lighting: PrincipledMaterial.NoLighting
            }
        }
    }
}
