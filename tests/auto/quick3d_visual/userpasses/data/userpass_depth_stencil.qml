// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers

Item {
    width: 200
    height: 200

    View3D {
        anchors.fill: parent
        renderOverrides: View3D.DisableInternalPasses

        PerspectiveCamera {
            position: Qt.vector3d(0, 0, 300)
        }

        RenderPassTexture {
            id: colorTexture
            format: RenderPassTexture.RGBA8
        }

        // A depth-stencil renderbuffer is not a texture, so the provider has
        // nothing to expose.
        RenderPass {
            id: depthStencilPass
            clearColor: "darkBlue"
            commands: [
                ColorAttachment { target: colorTexture },
                DepthStencilAttachment {}
            ]
        }

        SimpleQuadRenderer {
            texture: Texture {
                textureProvider: RenderOutputProvider {
                    textureSource: RenderOutputProvider.UserPassDepthTexture
                    renderPass: depthStencilPass
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
