// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers

Item {
    id: root
    width: 200
    height: 200

    property int depthFormat: RenderPassTexture.Depth24Stencil8
    property int textureSource: RenderOutputProvider.UserPassDepthTexture

    View3D {
        anchors.fill: parent
        renderOverrides: View3D.DisableInternalPasses
        // The default tonemapping would remap the depth values on their way to the window.
        environment: SceneEnvironment {
            tonemapMode: SceneEnvironment.TonemapModeNone
        }

        // The near and far planes bracket the sphere so that its front
        // (distance 250) lands at depth 0.4 instead of close to 1.0.
        PerspectiveCamera {
            position: Qt.vector3d(0, 0, 300)
            clipNear: 200
            clipFar: 400
        }

        RenderPassTexture {
            id: colorTexture
            format: RenderPassTexture.RGBA8
        }

        RenderPassTexture {
            id: depthTexture
            format: root.depthFormat
        }

        RenderPass {
            id: depthPass
            clearColor: "darkBlue"
            commands: [
                ColorAttachment { target: colorTexture },
                DepthTextureAttachment { target: depthTexture }
            ]
        }

        SimpleQuadRenderer {
            texture: Texture {
                textureProvider: RenderOutputProvider {
                    textureSource: root.textureSource
                    renderPass: depthPass
                }
            }
        }

        Model {
            source: "#Sphere"
            materials: PrincipledMaterial {
                baseColor: "yellow"
                lighting: PrincipledMaterial.NoLighting
            }
        }
    }
}
