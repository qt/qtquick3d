// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Procedural texture generation test.
// A ComputePass writes a fixed 2x2 pattern (red/green/blue/yellow, one
// colour per texel) into a ShaderImage. A CustomMaterial applied to a flat
// rectangle samples that image directly via imageLoad() in its fragment
// shader (SHADER_IMAGE/ShaderImage property) -- unlike the screentexture_invert
// and chained_passes scenes, this never goes through an Effect/full-screen
// quad: the compute-generated data is consumed as an ordinary material's
// surface colour on real 3D geometry. The C++ test checks that all four
// generated colours are visible somewhere on the rectangle (not asserting
// exact corner placement, since the primitive's UV origin/winding isn't
// verified here).

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
        }

        PerspectiveCamera {
            position: Qt.vector3d(0, 0, 300)
        }

        ShaderImage {
            id: proceduralImageRes
            width: 2
            height: 2
            format: ShaderImage.RGBA8
        }

        ComputePass {
            computeShader: "procedural_pattern.comp"
            property ShaderImage targetImage: proceduralImageRes
            commands: [
                Dispatch { groupCountX: 2; groupCountY: 2; groupCountZ: 1 }
            ]
        }

        Model {
            source: "#Rectangle"
            scale: Qt.vector3d(2.5, 2.5, 2.5)
            materials: CustomMaterial {
                shadingMode: CustomMaterial.Unshaded
                vertexShader: "procedural_material.vert"
                fragmentShader: "procedural_material.frag"
                property ShaderImage proceduralImage: proceduralImageRes
            }
        }
    }
}
