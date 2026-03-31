// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers

Item {
    width: 400
    height: 400

    View3D {
        id: view3D
        anchors.fill: parent
        renderOverrides: View3D.DisableInternalPasses

        environment: SceneEnvironment {
            backgroundMode: SceneEnvironment.Color
            clearColor: "red"  // Red environment
        }

        PerspectiveCamera {
            position: Qt.vector3d(0, 0, 300)
        }

        DirectionalLight {
            eulerRotation.x: -30
        }

        RenderPassTexture {
            id: mainTexture
            format: RenderPassTexture.RGBA8
        }

        // Custom render pass with green clear color
        // With DisableInternalPasses, this green should show, not the environment's red
        RenderPass {
            id: customPass
            materialMode: RenderPass.OriginalMaterial
            clearColor: "green"  // Green pass, environment is red

            commands: [
                ColorAttachment { target: mainTexture },
                DepthStencilAttachment {}
            ]
        }

        SimpleQuadRenderer {
            texture: Texture {
                textureProvider: RenderOutputProvider {
                    textureSource: RenderOutputProvider.UserPassTexture
                    renderPass: customPass
                    attachmentSelector: RenderOutputProvider.Attachment0
                }
            }
        }

        Model {
            source: "#Cube"
            materials: PrincipledMaterial {
                baseColor: "cyan"
            }
        }
    }
}
