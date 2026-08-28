// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// A ShaderImage of the given shape, written and read back by a compute
// shader declaring the matching image type (shape_<type>.comp). The whole
// window is green if the texel read back is the one written.

import QtQuick
import QtQuick3D

Item {
    id: root
    width: 400
    height: 400

    property int imageShape: ShaderImage.Image2D
    property string computeShader: "shape_image2D.comp"

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
            z: 300
        }

        ShaderImage {
            id: shapedImage
            shape: root.imageShape
            width: 4
            height: 4
            depth: 4
            arraySize: 4
            format: ShaderImage.R32F
        }

        ShaderImage {
            id: outputImage
            width: 400
            height: 400
            format: ShaderImage.RGBA8
        }

        ComputePass {
            computeShader: root.computeShader
            property ShaderImage shapedImage: shapedImage
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
