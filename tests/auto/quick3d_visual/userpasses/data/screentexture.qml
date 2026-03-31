// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// RenderOutputProvider.ScreenTexture test.
// A red sphere is rendered by the standard internal rendering pipeline.
// A user pass uses RenderOutputProvider.ScreenTexture to read the rendered scene
// texture and displays it via SimpleQuadRenderer, covering the full view.
// The C++ test verifies that:
//   (1) The ScreenTexture contains the rendered scene (red pixels from the sphere).
//   (2) Changing the sphere colour to blue causes the ScreenTexture output to change.

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers

Item {
    id: root
    width: 400
    height: 400

    // Exposed so the C++ test can change the sphere colour at runtime
    property color sphereColor: "red"

    View3D {
        anchors.fill: parent
        // Standard rendering is active (no DisableInternalPasses)

        environment: SceneEnvironment {
            backgroundMode: SceneEnvironment.Color
            clearColor: "black"
        }

        PerspectiveCamera {
            position: Qt.vector3d(0, 0, 300)
        }

        // User pass: display the ScreenTexture (rendered scene) full-screen
        RenderPassTexture {
            id: colorTex
            format: RenderPassTexture.RGBA8
        }

        RenderPass {
            id: screenPass
            clearColor: "black"
            commands: [
                ColorAttachment { target: colorTex },
                DepthStencilAttachment {},
                RenderablesFilter { renderableTypes: RenderablesFilter.None }
            ]
        }

        SimpleQuadRenderer {
            texture: Texture {
                textureProvider: RenderOutputProvider {
                    textureSource: RenderOutputProvider.ScreenTexture
                }
            }
        }

        // Large sphere rendered by the standard internal pipeline
        Model {
            source: "#Sphere"
            scale: Qt.vector3d(3, 3, 3)
            materials: PrincipledMaterial {
                baseColor: root.sphereColor
                lighting: PrincipledMaterial.NoLighting
            }
        }
    }
}
