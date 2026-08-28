// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Compute-based post-processing filter test.
// A ComputePass reads SCREEN_TEXTURE (the already-rendered scene: a red
// sphere on a black background), inverts each pixel's colour, and writes
// the result into a ShaderImage. An Effect then displays that ShaderImage
// full-screen via imageLoad(). The C++ test verifies that:
//   (1) The background (originally black) appears inverted to white.
//   (2) The sphere (originally red) appears inverted to cyan.

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
                    property ShaderImage sourceImage: outputImage
                    passes: Pass {
                        shaders: Shader { stage: Shader.Fragment; shader: "displayimage.frag" }
                    }
                }
            ]
        }

        PerspectiveCamera {
            position: Qt.vector3d(0, 0, 300)
        }

        Model {
            source: "#Sphere"
            scale: Qt.vector3d(2, 2, 2)
            materials: PrincipledMaterial {
                baseColor: "red"
                lighting: PrincipledMaterial.NoLighting
            }
        }

        ShaderImage {
            id: outputImage
            width: 400
            height: 400
            format: ShaderImage.RGBA8
        }

        ComputePass {
            computeShader: "invert.comp"
            property ShaderImage targetImage: outputImage
            commands: [
                Dispatch {
                    groupCountX: (outputImage.width + 7) / 8
                    groupCountY: (outputImage.height + 7) / 8
                    groupCountZ: 1
                }
            ]
        }
    }
}
