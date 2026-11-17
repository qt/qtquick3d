// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// QSSGFrameData::getShaderImageTexture()/getShaderStorageBuffer() test.
//
// A ComputePass writes two ShaderImages (differing in both size and format)
// and two ShaderStorageBuffers (one provided by the extension itself, one
// allocated by the engine). A ComputeResultExtension in the same View3D then
// looks each one up by its resource id through QSSGFrameData's
// getShaderImageTexture() and getShaderStorageBuffer(), which is how a C++
// RenderExtension gets at the QRhiTexture/QRhiBuffer behind them, and checks
// that every id resolved to the right resource. It also reads the provided
// buffer back to the CPU to confirm the handle really points at what the
// compute shader wrote. computeresult_multiview.qml covers the same lookups
// from a different View3D than the one running the pass.
//
// The pass writes a green full-screen ShaderImage too, displayed by an Effect,
// so the test can tell "the pass never ran" apart from
// "the lookups did not find what the pass published".

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
                    property ShaderImage sourceImage: displayImageRes
                    passes: Pass {
                        shaders: Shader { stage: Shader.Fragment; shader: "displayimage.frag" }
                    }
                }
            ]
        }

        PerspectiveCamera {
            position: Qt.vector3d(0, 0, 300)
        }

        ComputeResultExtension {
            id: probe
            objectName: "probe"
            image: smallImageRes
            wideImage: wideImageRes
            writtenBuffer: providedBufferRes
            engineBuffer: engineBufferRes
            providesWrittenBuffer: true
        }

        // Backed by a QRhiBuffer the probe extension creates and registers
        // itself, so the probe can compare the handle it gets back from
        // getShaderStorageBuffer() against the one it handed in.
        ShaderStorageBuffer {
            id: providedBufferRes
            bufferProvider: probe
        }

        ShaderStorageBuffer {
            id: engineBufferRes
            size: 256
        }

        ShaderImage {
            id: smallImageRes
            width: 8
            height: 8
            format: ShaderImage.RGBA8
        }

        ShaderImage {
            id: wideImageRes
            width: 32
            height: 4
            format: ShaderImage.R32UI
        }

        ShaderImage {
            id: displayImageRes
            width: 400
            height: 400
            format: ShaderImage.RGBA8
        }

        // The property names have to match the SHADER_IMAGE/STORAGE_BUFFER
        // declarations in computeresult_fill.comp.
        ComputePass {
            id: fillPass
            computeShader: "computeresult_fill.comp"
            property ShaderImage probeImage: smallImageRes
            property ShaderImage probeWideImage: wideImageRes
            property ShaderImage displayImage: displayImageRes
            property ShaderStorageBuffer probeWrittenBuffer: providedBufferRes
            property ShaderStorageBuffer probeEngineBuffer: engineBufferRes
            commands: [
                Dispatch {
                    groupCountX: (displayImageRes.width + 7) / 8
                    groupCountY: (displayImageRes.height + 7) / 8
                    groupCountZ: 1
                }
            ]
        }
    }
}
