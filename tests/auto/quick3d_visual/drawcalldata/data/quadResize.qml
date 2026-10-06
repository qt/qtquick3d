// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers

Item {
    width: 640
    height: 480
    visible: true

    View3D {
        anchors.fill: parent

        environment: SceneEnvironment {
            backgroundMode: SceneEnvironment.Color
            clearColor: "black"
        }

        PerspectiveCamera {
            z: 300
        }

        DirectionalLight {}

        Model {
            source: "#Cube"
            materials: PrincipledMaterial { baseColor: "red" }
        }

        // The pass texture follows the size of the view, so every resize
        // hands the quad renderer a new texture to bind.
        RenderPassTexture {
            id: passTexture
            format: RenderPassTexture.RGBA8
        }

        RenderPass {
            id: colorPass
            materialMode: RenderPass.OriginalMaterial
            commands: [
                ColorAttachment { target: passTexture },
                DepthStencilAttachment {}
            ]
        }

        SimpleQuadRenderer {
            texture: Texture {
                textureProvider: RenderOutputProvider {
                    textureSource: RenderOutputProvider.UserPassTexture
                    renderPass: colorPass
                    attachmentSelector: RenderOutputProvider.Attachment0
                }
            }
        }
    }
}
