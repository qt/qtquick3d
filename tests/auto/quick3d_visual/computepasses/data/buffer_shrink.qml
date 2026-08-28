// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// CPU-uploaded lookup buffer test.
// LutBufferData (a QQuick3DShaderStorageBufferData subclass, C++-side) uploads a fixed
// 4-entry RGBA colour table into a ShaderStorageBuffer via bufferData. A
// ComputePass reads that buffer as a read-only SSBO and paints one of the
// four colours into each vertical quarter of a ShaderImage, based on the
// pixel's x coordinate; the image is then displayed full-screen via an
// Effect. This exercises the bufferData/dataModified CPU->GPU upload path
// with no GPU-side data generation at all.

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers
import io.qt.tests.auto.Quick3DComputePasses

Item {
    property alias count: bufferData.count
    property alias expectedCount: lengthPass.expectedCount
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
            id: sizedBuffer
            bufferData: ResizableBufferData { id: bufferData; count: 8 }
        }

        ShaderImage {
            id: outputImage
            width: 400
            height: 400
            format: ShaderImage.RGBA8
        }

        ComputePass {
            id: lengthPass
            property int expectedCount: 8
            computeShader: "length_check.comp"
            property ShaderStorageBuffer checkBuffer: sizedBuffer
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
