// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// A container pass references one of two leaf passes through a SubRenderPass.
// Toggling root.useLeafAAsSubPass flips which leaf is the sub-pass, exercising
// the runtime re-derivation of the top-level/sub-pass classification.

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers

Item {
    id: root
    width: 200
    height: 200

    property bool useLeafAAsSubPass: false

    View3D {
        anchors.fill: parent
        renderOverrides: View3D.DisableInternalPasses

        PerspectiveCamera { position: Qt.vector3d(0, 0, 300) }

        RenderPassTexture { id: texA; format: RenderPassTexture.RGBA8 }
        RenderPassTexture { id: texB; format: RenderPassTexture.RGBA8 }
        RenderPassTexture { id: texMain; format: RenderPassTexture.RGBA8 }

        RenderPass {
            id: leafA
            objectName: "leafA"
            commands: [
                ColorAttachment { target: texA },
                DepthStencilAttachment {},
                RenderablesFilter { renderableTypes: RenderablesFilter.None }
            ]
        }
        RenderPass {
            id: leafB
            objectName: "leafB"
            commands: [
                ColorAttachment { target: texB },
                DepthStencilAttachment {},
                RenderablesFilter { renderableTypes: RenderablesFilter.None }
            ]
        }
        RenderPass {
            id: container
            objectName: "container"
            commands: [
                ColorAttachment { target: texMain },
                DepthStencilAttachment {},
                RenderablesFilter { renderableTypes: RenderablesFilter.None },
                SubRenderPass { renderPass: root.useLeafAAsSubPass ? leafA : leafB }
            ]
        }

        SimpleQuadRenderer {
            texture: Texture {
                textureProvider: RenderOutputProvider {
                    textureSource: RenderOutputProvider.UserPassTexture
                    renderPass: container
                }
            }
        }
    }
}
