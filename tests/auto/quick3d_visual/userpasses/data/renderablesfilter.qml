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
            clearColor: "yellow"  // Yellow environment (should not be seen)
        }

        PerspectiveCamera {
            position: Qt.vector3d(0, 0, 300)
        }

        DirectionalLight {}

        RenderPassTexture {
            id: outputTexture
            format: RenderPassTexture.RGBA8
        }

        // Render pass that only renders opaque objects
        // Uses blue background to prove pass is active
        RenderPass {
            id: opaquePass
            materialMode: RenderPass.OriginalMaterial
            clearColor: "blue"  // Blue background (proves pass is used)

            commands: [
                ColorAttachment { target: outputTexture },
                DepthStencilAttachment {},
                RenderablesFilter {
                    renderableTypes: RenderablesFilter.Opaque
                }
            ]
        }

        SimpleQuadRenderer {
            texture: Texture {
                textureProvider: RenderOutputProvider {
                    textureSource: RenderOutputProvider.UserPassTexture
                    renderPass: opaquePass
                    attachmentSelector: RenderOutputProvider.Attachment0
                }
            }
        }

        // This should be visible (opaque)
        Model {
            source: "#Cube"
            materials: PrincipledMaterial {
                baseColor: "red"
                alphaMode: PrincipledMaterial.Opaque
            }
        }

        // This should NOT be visible (transparent, filtered out)
        Model {
            source: "#Sphere"
            position: Qt.vector3d(150, 0, 0)
            materials: PrincipledMaterial {
                baseColor: Qt.rgba(0, 1, 0, 0.5)
                alphaMode: PrincipledMaterial.Blend
            }
        }
    }
}
