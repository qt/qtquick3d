// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Same scene as shadow_uniforms.qml, with a compute shader that uses only
// the *_LIGHT_SHADOW macros and none of the light identifiers. See
// shadow_only.comp.

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
                    property ShaderImage sourceImage: outputImageRes
                    passes: Pass {
                        shaders: Shader { stage: Shader.Fragment; shader: "displayimage.frag" }
                    }
                }
            ]
        }

        PerspectiveCamera {
            position: Qt.vector3d(0, 300, 600)
            eulerRotation: Qt.vector3d(-20, 0, 0)
            clipFar: 2000
        }

        Model {
            source: "#Cube"
            scale: Qt.vector3d(20, 0.01, 20)
            castsShadows: false
            receivesShadows: true
            materials: DefaultMaterial { diffuseColor: "gray" }
        }

        Model {
            source: "#Cube"
            position: Qt.vector3d(0, 150, 0)
            castsShadows: true
            receivesShadows: false
            materials: DefaultMaterial { diffuseColor: "white" }
        }

        DirectionalLight {
            castsShadow: true
            softShadowQuality: Light.PCF8
            shadowMapFar: 2000
            eulerRotation: Qt.vector3d(-60, -20, 0)
        }

        PointLight {
            castsShadow: true
            softShadowQuality: Light.PCF8
            position: Qt.vector3d(300, 300, 300)
        }

        SpotLight {
            castsShadow: true
            softShadowQuality: Light.PCF8
            position: Qt.vector3d(-300, 300, 300)
            coneAngle: 40
        }

        ShaderImage {
            id: outputImageRes
            width: 400
            height: 400
            format: ShaderImage.RGBA8
        }

        ComputePass {
            computeShader: "shadow_only.comp"
            property ShaderImage targetImage: outputImageRes
            commands: [
                Dispatch {
                    groupCountX: (outputImageRes.width + 7) / 8
                    groupCountY: (outputImageRes.height + 7) / 8
                    groupCountZ: 1
                }
            ]
        }
    }
}
