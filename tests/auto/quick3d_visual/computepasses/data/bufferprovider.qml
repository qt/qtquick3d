// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// ShaderStorageBuffer.bufferProvider test.
//
// Identical to chained_passes.qml (producerPass writes green into a
// ShaderStorageBuffer, consumerPass reads it, inverts it, and writes the
// result into a ShaderImage displayed via an Effect), except the buffer's
// backing GPU resource comes from a BufferProviderExtension (a
// QQuick3DRenderExtension whose backend creates its own QRhiBuffer and
// registers it via QSSGRenderExtensionHelpers::registerRenderResultBuffer())
// instead of being allocated by the engine via ShaderStorageBuffer.size.
//
// If bufferProvider wiring were broken, the compute passes would either
// fail to bind the buffer at all, or read back the wrong/uninitialized
// content -- either way, the screen would not end up magenta.

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers
import io.qt.tests.auto.Quick3DComputePasses

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
        }

        BufferProviderExtension {
            id: providerExtension
        }

        ShaderStorageBuffer {
            id: providedBuffer
            bufferProvider: providerExtension
        }

        ShaderImage {
            id: outputImageRes
            width: 400
            height: 400
            format: ShaderImage.RGBA8
        }

        ComputePass {
            id: consumerPass
            computeShader: "consumer.comp"
            property ShaderStorageBuffer sharedBuffer: providedBuffer
            property ShaderImage targetImage: outputImageRes
            commands: [
                Dispatch {
                    groupCountX: (outputImageRes.width + 7) / 8
                    groupCountY: (outputImageRes.height + 7) / 8
                    groupCountZ: 1
                }
            ]

            // Nested in its consumer, so it runs first.
            ComputePass {
                id: producerPass
                computeShader: "producer.comp"
                property ShaderStorageBuffer sharedBuffer: providedBuffer
                commands: [ Dispatch { } ]
            }
        }
    }
}
