// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// CustomMaterial writing to a ShaderStorageBuffer/ShaderImage -- the
// geometry-rendered counterpart to effect_write.qml's full-screen Effect
// write test.
//
// resetPass (a bare ComputePass, always scheduled before the main
// opaque/transparent render) zero-initializes a counter buffer and
// pre-clears a marker image to a "not written" sentinel (red). The
// rectangle's CustomMaterial atomically increments the counter and writes
// a green marker for every fragment it covers. verifyEffect -- an Effect,
// which always runs after the main render within the same frame -- reads
// both back: green where the rectangle wrote, red where it didn't
// (background), or magenta everywhere if suspiciously few fragments wrote
// at all.

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
                        shaders: Shader { stage: Shader.Fragment; shader: "custommaterial_verify.frag" }
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
            computeShader: "reset_marker.comp"
            property ShaderStorageBuffer counterBuffer: counterBufferRes
            property ShaderImage markerImage: markerImageRes
            commands: [
                Dispatch {
                    groupCountX: (markerImageRes.width + 7) / 8
                    groupCountY: (markerImageRes.height + 7) / 8
                    groupCountZ: 1
                }
            ]
        }

        Model {
            source: "#Rectangle"
            scale: Qt.vector3d(2.5, 2.5, 2.5)
            materials: CustomMaterial {
                shadingMode: CustomMaterial.Unshaded
                vertexShader: "custommaterial_write.vert"
                fragmentShader: "custommaterial_write.frag"
                property ShaderStorageBuffer counterBuffer: counterBufferRes
                property ShaderImage markerImage: markerImageRes
            }
        }
    }
}
