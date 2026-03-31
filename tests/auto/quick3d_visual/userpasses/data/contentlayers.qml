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
            clearColor: "white"  // White environment (should not be seen)
        }

        PerspectiveCamera {
            position: Qt.vector3d(0, 0, 500)
        }

        DirectionalLight {}

        RenderPassTexture {
            id: outputTexture
            format: RenderPassTexture.RGBA8
        }

        // Render pass that filters by layer (only Layer0 and Layer1)
        // Uses gray background to prove pass is active
        RenderPass {
            id: layerPass
            materialMode: RenderPass.OriginalMaterial
            clearColor: "gray"  // Gray background (proves pass is used)

            commands: [
                ColorAttachment { target: outputTexture },
                DepthStencilAttachment {},
                RenderablesFilter {
                    layerMask: ContentLayer.Layer0 | ContentLayer.Layer1
                }
            ]
        }

        SimpleQuadRenderer {
            texture: Texture {
                textureProvider: RenderOutputProvider {
                    textureSource: RenderOutputProvider.UserPassTexture
                    renderPass: layerPass
                    attachmentSelector: RenderOutputProvider.Attachment0
                }
            }
        }

        // This should be visible (Layer0)
        Model {
            layers: ContentLayer.Layer0
            source: "#Cube"
            position: Qt.vector3d(-100, 0, 0)
            materials: PrincipledMaterial {
                baseColor: "red"
            }
        }

        // This should be visible (Layer1)
        Model {
            layers: ContentLayer.Layer1
            source: "#Sphere"
            position: Qt.vector3d(100, 0, 0)
            materials: PrincipledMaterial {
                baseColor: "green"
            }
        }

        // This should NOT be visible (Layer2, filtered out)
        Model {
            layers: ContentLayer.Layer2
            source: "#Cone"
            position: Qt.vector3d(0, 100, 0)
            materials: PrincipledMaterial {
                baseColor: "blue"
            }
        }
    }
}
