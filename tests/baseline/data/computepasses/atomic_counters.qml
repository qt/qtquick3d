// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Atomic buffer/image access test, exercising the coherent, volatile, and
// restrict GLSL memory qualifiers alongside readonly/writeonly.
//
// Pipeline (three chained ComputePasses):
//   resetPass  - zero-initializes a ShaderStorageBuffer counter and a 4-bin
//                r32ui ShaderImage histogram (plain writes, no atomics).
//   countPass  - dispatched once per output pixel (400x400 = 160000
//                invocations). Every invocation atomically increments the
//                buffer counter and its bucket in the histogram image
//                (bucketed by coord.x / 100, 4 buckets of 100 columns
//                each), both declared "coherent restrict".
//   verifyPass - reads both back declared "volatile readonly" and paints
//                the whole screen green if the buffer counter is exactly
//                160000 and every histogram bucket is exactly 40000, or
//                red otherwise. Any lost atomic update undercounts and
//                turns the screen red.

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers

Item {
    width: 400
    height: 400

    // imageAtomic*() does not work on Metal (see testAtomicAccess in
    // tests/auto/quick3d_visual/computepasses/tst_computepasses.cpp), so
    // the result there would be undefined. Show a fixed colour instead to
    // keep the baseline deterministic.
    Rectangle {
        anchors.fill: parent
        color: "gray"
        visible: GraphicsInfo.api === GraphicsInfo.Metal
    }

    Loader {
        anchors.fill: parent
        active: GraphicsInfo.api !== GraphicsInfo.Unknown && GraphicsInfo.api !== GraphicsInfo.Metal
        sourceComponent: Component {
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
                }

                ShaderStorageBuffer {
                    id: counterBufferRes
                    size: 4
                }

                ShaderImage {
                    id: histogramImageRes
                    width: 4
                    height: 1
                    format: ShaderImage.R32UI
                }

                ShaderImage {
                    id: outputImageRes
                    width: 400
                    height: 400
                    format: ShaderImage.RGBA8
                }

                // Nesting expresses the order: reset (deepest) runs first, then count,
                // then verify.
                ComputePass {
                    id: verifyPass
                    computeShader: "verify.comp"
                    property ShaderStorageBuffer counterBuffer: counterBufferRes
                    property ShaderImage histogramImage: histogramImageRes
                    property ShaderImage targetImage: outputImageRes
                    commands: [
                        Dispatch {
                            groupCountX: (outputImageRes.width + 7) / 8
                            groupCountY: (outputImageRes.height + 7) / 8
                            groupCountZ: 1
                        }
                    ]

                    ComputePass {
                        id: countPass
                        computeShader: "count.comp"
                        property ShaderStorageBuffer counterBuffer: counterBufferRes
                        property ShaderImage histogramImage: histogramImageRes
                        commands: [
                            Dispatch {
                                groupCountX: (outputImageRes.width + 7) / 8
                                groupCountY: (outputImageRes.height + 7) / 8
                                groupCountZ: 1
                            }
                        ]

                        ComputePass {
                            id: resetPass
                            computeShader: "reset.comp"
                            property ShaderStorageBuffer counterBuffer: counterBufferRes
                            property ShaderImage histogramImage: histogramImageRes
                            commands: [ Dispatch { } ]
                        }
                    }
                }
            }
        }
    }
}
