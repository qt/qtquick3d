// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// SkyboxPass test scene.
// A custom render pipeline with DisableInternalPasses renders the skybox
// through a SubRenderPass with passMode: RenderPass.SkyboxPass.
// The ProceduralSkyTextureData light probe provides a coloured sky gradient.
// The test verifies that non-black pixels appear (the skybox rendered), and
// that toggling the lightProbe at runtime changes the output.

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers

Item {
    id: root
    width: 400
    height: 400

    View3D {
        id: view3D
        anchors.fill: parent
        renderOverrides: View3D.DisableInternalPasses

        environment: SceneEnvironment {
            id: sceneEnv
            backgroundMode: SceneEnvironment.SkyBox
            lightProbe: Texture {
                id: skyProbe
                objectName: "skyProbe"
                textureData: ProceduralSkyTextureData {}
            }
        }

        PerspectiveCamera {
            position: Qt.vector3d(0, 0, 300)
        }

        RenderPassTexture {
            id: colorTex
            format: RenderPassTexture.RGBA8
        }

        // Main pass: only renders via SubRenderPasses
        RenderPass {
            id: mainPass
            clearColor: "black"
            commands: [
                ColorAttachment { target: colorTex },
                DepthStencilAttachment {},
                RenderablesFilter { renderableTypes: RenderablesFilter.None },
                SubRenderPass {
                    renderPass: RenderPass {
                        id: skyPass
                        objectName: "skyPass"
                        passMode: RenderPass.SkyboxPass
                    }
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
    }
}
