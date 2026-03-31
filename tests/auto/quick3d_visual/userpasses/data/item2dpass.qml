// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Item2DPass test scene.
// A Qt Quick 2D Rectangle (red) is embedded inside a 3D Node.
// A SubRenderPass with passMode: RenderPass.Item2DPass renders the 2D item
// into the 3D scene.  The test verifies that red pixels appear (the 2D item
// rendered), and that changing the item colour at runtime changes the output.

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers

Item {
    id: root
    width: 400
    height: 400

    // Color for the embedded 2D item, changeable at runtime from C++ test
    property color item2DColor: "red"

    View3D {
        id: view3D
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
            id: colorTex
            format: RenderPassTexture.RGBA8
        }

        RenderPass {
            id: mainPass
            clearColor: "black"
            commands: [
                ColorAttachment { target: colorTex },
                DepthStencilAttachment {},
                RenderablesFilter { renderableTypes: RenderablesFilter.None },
                SubRenderPass {
                    renderPass: RenderPass {
                        id: item2DPass
                        objectName: "item2DPass"
                        passMode: RenderPass.Item2DPass
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

        // 2D item embedded in the 3D scene via a Node
        Node {
            position: Qt.vector3d(0, 0, 0)

            Item {
                width: 200
                height: 200

                Rectangle {
                    id: coloredRect
                    objectName: "coloredRect"
                    anchors.fill: parent
                    color: root.item2DColor
                }
            }
        }
    }
}
