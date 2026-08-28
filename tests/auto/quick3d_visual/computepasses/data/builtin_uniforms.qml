// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Built-in uniform/texture coverage test.
// A ComputePass checks that every internally-defined (non-shadow) uniform
// and texture it can reference -- camera matrices/properties, RHI sentinel
// properties, the frame counter, per-light-type counts/diffuse/specular
// colours/positions/attenuation/cones, and the screen/depth/AO/normal-
// roughness textures -- carries a value consistent with this scene's known
// setup (one light of each type with distinct colour/brightness/position, a camera
// at a known position/clip range, a lit sphere at the origin). See
// builtin_uniforms.comp for the exact checks. Paints the screen green if
// every check passes, red otherwise.

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers

Item {
    width: 400
    height: 400

    View3D {
        anchors.fill: parent

        environment: SceneEnvironment {
            backgroundMode: SceneEnvironment.Color
            clearColor: "black"
            tonemapMode: SceneEnvironment.TonemapModeNone
            effects: [
                Effect {
                    property ShaderImage sourceImage: outputImageRes
                    passes: Pass {
                        shaders: Shader { stage: Shader.Fragment; shader: "displayimage.frag" }
                    }
                }
            ]
        }

        PerspectiveCamera {
            position: Qt.vector3d(0, 0, 300)
            clipNear: 10
            clipFar: 1000
        }

        DirectionalLight {
            color: "red"
            brightness: 2.0
        }

        PointLight {
            // Pure primary, not the CSS/SVG named colour "green" (which is
            // the darker (0, 0.502, 0) "web green", not (0, 1, 0)).
            color: "#00ff00"
            brightness: 3.0
            position: Qt.vector3d(50, 0, 0)
            constantFade: 1
            linearFade: 0
            quadraticFade: 0
        }

        SpotLight {
            color: "blue"
            brightness: 4.0
            position: Qt.vector3d(-50, 0, 0)
            constantFade: 1
            linearFade: 0
            quadraticFade: 0
            coneAngle: 40
            innerConeAngle: 20
        }

        Model {
            source: "#Sphere"
            materials: PrincipledMaterial {
                baseColor: "white"
            }
        }

        ShaderImage {
            id: outputImageRes
            width: 400
            height: 400
            format: ShaderImage.RGBA8
        }

        ComputePass {
            computeShader: "builtin_uniforms.comp"
            property ShaderImage targetImage: outputImageRes
            commands: [
                Dispatch {
                    groupCountX: (outputImageRes.width + 7) / 8
                    groupCountY: (outputImageRes.height + 7) / 8
                    groupCountZ: 1
                }
            ]
        }
    }
}
