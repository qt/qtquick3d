// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Minimal two-ComputePass chain test.
// producerPass is declared inside consumerPass, which is how the ordering is
// expressed: a nested pass renders before its parent. The relative order of
// passes at the same nesting depth is not defined, so the chain cannot rely on
// declaration order alone.
// producerPass writes a fixed green colour into a ShaderStorageBuffer (a single
// vec4 SSBO entry). consumerPass reads that buffer, inverts the colour, and
// writes the result into every texel of a ShaderImage, which is then
// displayed full-screen via an Effect. If pass ordering/synchronization is
// broken and consumerPass reads the buffer before producerPass writes it,
// the screen shows white (inverted black, the buffer's zero-initialized
// content) instead of magenta (inverted green), giving an unambiguous
// pass/fail signal.

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

        ShaderStorageBuffer {
            id: chainBuffer
            size: 16
        }

        ShaderImage {
            id: outputImage
            width: 400
            height: 400
            format: ShaderImage.RGBA8
        }

        ComputePass {
            id: consumerPass
            computeShader: "consumer.comp"
            property ShaderStorageBuffer sharedBuffer: chainBuffer
            property ShaderImage targetImage: outputImage
            commands: [
                Dispatch {
                    groupCountX: (outputImage.width + 7) / 8
                    groupCountY: (outputImage.height + 7) / 8
                    groupCountZ: 1
                }
            ]

            // Nested in its consumer, so it runs first.
            ComputePass {
                id: producerPass
                computeShader: "producer.comp"
                property ShaderStorageBuffer sharedBuffer: chainBuffer
                commands: [ Dispatch { } ]
            }
        }
    }
}
