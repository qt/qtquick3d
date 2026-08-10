// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// A RenderPass without ColorAttachment commands gets one implicit default
// color attachment. Regression scene: building the pass's render target used
// to index the empty command list, and the compatibility check on subsequent
// frames both forced a rebuild and did the same. The provider consumes the
// pass's output, which schedules the pass and displays the implicitly
// created attachment.

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers

Item {
    width: 200
    height: 200

    View3D {
        anchors.fill: parent
        renderOverrides: View3D.DisableInternalPasses

        PerspectiveCamera { position: Qt.vector3d(0, 0, 300) }
        DirectionalLight {}

        RenderPass {
            id: bareRenderPass
            clearColor: "black"
            commands: [
                DepthStencilAttachment {},
                RenderablesFilter { layerMask: ContentLayer.Layer0 }
            ]
        }

        SimpleQuadRenderer {
            texture: Texture {
                textureProvider: RenderOutputProvider {
                    textureSource: RenderOutputProvider.UserPassTexture
                    renderPass: bareRenderPass
                }
            }
        }

        Model {
            layers: ContentLayer.Layer0
            source: "#Cube"
            materials: PrincipledMaterial {
                baseColor: "red"
                lighting: PrincipledMaterial.NoLighting
            }
        }
    }
}
