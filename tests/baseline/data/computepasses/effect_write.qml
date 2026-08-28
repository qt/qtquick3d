// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Effect writing to a ShaderStorageBuffer/ShaderImage (as opposed to every
// other scene in this suite, where only a ComputePass ever writes and a
// material/Effect only ever reads via imageLoad).
//
// resetPass (a bare ComputePass, always scheduled before the Effects chain
// within a frame) zero-initializes a counter buffer. writeEffect atomically
// increments that buffer and stores a marker into a ShaderImage once per
// output pixel. verifyEffect -- the next stage in the same Effects chain,
// guaranteed to run only after writeEffect's pass has completed for the
// whole framebuffer -- reads both back and shows green only if the counter
// reached exactly the output pixel count and the marker is present
// everywhere; red otherwise.

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
                    property ShaderStorageBuffer counterBuffer: counterBufferRes
                    property ShaderImage markerImage: markerImageRes
                    passes: Pass {
                        shaders: Shader { stage: Shader.Fragment; shader: "effect_write.frag" }
                    }
                },
                Effect {
                    property ShaderStorageBuffer counterBuffer: counterBufferRes
                    property ShaderImage markerImage: markerImageRes
                    passes: Pass {
                        shaders: Shader { stage: Shader.Fragment; shader: "effect_verify.frag" }
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
            id: markerImageRes
            width: 400
            height: 400
            format: ShaderImage.RGBA8
        }

        ComputePass {
            id: resetPass
            computeShader: "reset_counter.comp"
            property ShaderStorageBuffer counterBuffer: counterBufferRes
            commands: [ Dispatch { } ]
        }
    }
}
