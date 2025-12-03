// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers.impl

SsrEnvEffect {
    id: ssrEffect

    // Global
    property real roughnessCut: 0.65

    // Main pass
    property real stepSize: 0.01
    property real minRayStep: 0.01
    property int binarySteps: 8
    property int maxSteps: 512
    property real baseThickness: 20

    property View3D view3d: null

    readonly property TextureInput f0Texture: TextureInput {
        id: f0TextureInput
        texture: Texture {
            sourceItem: Item {
                width: ssrEffect.view3d !== null ? ssrEffect.view3d.width : 0
                height: ssrEffect.view3d !== null ? ssrEffect.view3d.height : 0
                View3D {
                    anchors.fill: parent
                    importScene: ssrEffect.view3d !== null ? ssrEffect.view3d.scene : null
                    environment.tonemapMode: SceneEnvironment.TonemapModeNone
                    environment.debugSettings.materialOverride: DebugSettings.F0
                }
            }
        }
    }

    readonly property TextureInput ssrSampler: TextureInput { texture: Texture {} }
    readonly property TextureInput ssrMaskSampler: TextureInput { texture: Texture {} }
    readonly property TextureInput ssrReflConfSampler: TextureInput { texture: Texture {} }

    Buffer {
        id: ssrBufferMask
        name: "ssrBufferMask"
        sizeMultiplier: 1.0
        format: Buffer.RGBA32F
    }

    Buffer {
        id: ssrBufferMainReflColorConf
        name: "ssrBufferMainReflColorConf"
        sizeMultiplier: 1.0
        format: Buffer.RGBA32F
    }

    Pass {
        id: ssrMaskPass
        output: ssrBufferMask
        shaders: Shader { stage: Shader.Fragment; shader: "qrc:/qtquick3d_helpers/shaders/ssr_mask.frag" }
    }

    Pass {
        id: ssrMainPass
        output: ssrBufferMainReflColorConf
        shaders: [
            Shader {
                stage: Shader.Fragment
                shader: "qrc:/qtquick3d_helpers/shaders/ssr_main.frag"
            }
        ]
        commands: [
            BufferInput { buffer: ssrBufferMask; sampler: "ssrMaskSampler" }
        ]
    }

    Pass {
        id: ssrCompositionPass
        shaders: [
        Shader {
                stage: Shader.Fragment
                shader: "qrc:/qtquick3d_helpers/shaders/ssr_composition.frag"
            }
        ]
        commands: [
            BufferInput { buffer: ssrBufferMainReflColorConf; sampler: "ssrSampler" }
        ]
    }

    passes: [
        ssrMaskPass, ssrMainPass, ssrCompositionPass
    ]

}
