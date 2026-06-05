// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers

Item {
    id: root
    property string augmentFile: "augment_feature_flags_worldnormal_augment.glsl"
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
            objectName: "augmentPass"
            materialMode: RenderPass.AugmentMaterial
            augmentShader: root.augmentFile
            clearColor: "black"

            commands: [
                ColorAttachment { target: outputTex },
                DepthStencilAttachment {}
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
