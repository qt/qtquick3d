// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// QSSGFrameData::getShaderImageTexture()/getShaderStorageBuffer() across
// View3Ds.
//
// The ComputePass and the resources it writes live in the left View3D, while
// the ComputeResultExtension doing the lookups lives in the right one. The
// GPU resources behind a ShaderImage or ShaderStorageBuffer belong to the
// window, not to a View3D, so the lookups have to resolve no matter which
// View3D the extension is in. The left View3D also displays a green
// full-screen image written by the pass, so the test can tell "the pass never
// ran" apart from "the lookups did not find what the pass wrote".

import QtQuick
import QtQuick3D
import io.qt.tests.auto.Quick3DComputePasses

Item {
    width: 400
    height: 200

    View3D {
        id: producerView
        x: 0
        y: 0
        width: 200
        height: 200

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

        ShaderStorageBuffer {
            id: writtenBufferRes
            size: 16
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
            width: 200
            height: 200
            format: ShaderImage.RGBA8
        }

        // The property names have to match the SHADER_IMAGE/STORAGE_BUFFER
        // declarations in computeresult_fill.comp.
        ComputePass {
            computeShader: "computeresult_fill.comp"
            property ShaderImage probeImage: smallImageRes
            property ShaderImage probeWideImage: wideImageRes
            property ShaderImage displayImage: displayImageRes
            property ShaderStorageBuffer probeWrittenBuffer: writtenBufferRes
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

    View3D {
        id: consumerView
        x: 200
        y: 0
        width: 200
        height: 200

        environment: SceneEnvironment {
            backgroundMode: SceneEnvironment.Color
            clearColor: "black"
        }

        PerspectiveCamera {
            position: Qt.vector3d(0, 0, 300)
        }

        ComputeResultExtension {
            id: probe
            objectName: "probe"
            image: smallImageRes
            wideImage: wideImageRes
            writtenBuffer: writtenBufferRes
            engineBuffer: engineBufferRes
        }
    }
}
