// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// AddDefine test scene.
// An AugmentMaterial pass uses AddDefine to inject #define MY_DEFINE 1 into
// the augment shader.  The shader writes green to MYCOLOR when MY_DEFINE is
// defined and red otherwise.
// The C++ test changes the define at runtime to verify dynamic re-evaluation.

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers

Item {
    id: root
    width: 400
    height: 400

    View3D {
        anchors.fill: parent
        renderOverrides: View3D.DisableInternalPasses

        environment: SceneEnvironment {
            backgroundMode: SceneEnvironment.Color
            clearColor: "black"
        }

        PerspectiveCamera {
            position: Qt.vector3d(0, 0, 300)
        }

        RenderPassTexture {
            id: outputTex
            format: RenderPassTexture.RGBA8
        }

        RenderPass {
            id: mainPass
            materialMode: RenderPass.AugmentMaterial
            augmentShader: "adddefine_augment.glsl"
            clearColor: "black"

            commands: [
                ColorAttachment { target: outputTex },
                DepthStencilAttachment {},
                AddDefine {
                    id: testDefine
                    objectName: "testDefine"
                    name: "MY_DEFINE"
                    value: 1
                }
            ]
        }

        SimpleQuadRenderer {
            texture: Texture {
                textureProvider: RenderOutputProvider {
                    textureSource: RenderOutputProvider.UserPassTexture
                    renderPass: mainPass
                    attachmentSelector: RenderOutputProvider.Attachment0
                }
            }
        }

        // Large sphere to fill most of the screen
        Model {
            source: "#Sphere"
            scale: Qt.vector3d(3, 3, 3)
            materials: PrincipledMaterial {
                baseColor: "white"
                lighting: PrincipledMaterial.NoLighting
            }
        }
    }
}
