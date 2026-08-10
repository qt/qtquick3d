// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// While the container references leafPass it is a sub-pass: it produces no
// output of its own and the provider-driven quad stays empty. When the
// reference is dropped the leaf must return to being a top-level pass and
// render into its own (implicit default) attachment again.

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers

Item {
    id: root
    width: 200
    height: 200

    property bool useAsSubPass: true

    View3D {
        anchors.fill: parent
        renderOverrides: View3D.DisableInternalPasses

        PerspectiveCamera { position: Qt.vector3d(0, 0, 300) }
        DirectionalLight {}

        // No attachment commands: as a top-level pass it renders into the
        // implicit default color attachment, and as a sub-pass there are no
        // ignored-attachment warnings.
        RenderPass {
            id: leafPass
            objectName: "leafPass"
            clearColor: "black"
            commands: [
                RenderablesFilter { layerMask: ContentLayer.Layer0 }
            ]
        }

        RenderPass {
            id: containerPass
            objectName: "containerPass"
            clearColor: "black"
            commands: [
                RenderablesFilter { renderableTypes: RenderablesFilter.None },
                SubRenderPass { renderPass: root.useAsSubPass ? leafPass : null }
            ]
        }

        // Displays the leaf's own output: empty while the leaf is a sub-pass,
        // the green cube once it is a top-level pass again.
        SimpleQuadRenderer {
            texture: Texture {
                textureProvider: RenderOutputProvider {
                    textureSource: RenderOutputProvider.UserPassTexture
                    renderPass: leafPass
                }
            }
        }

        Model {
            layers: ContentLayer.Layer0
            source: "#Cube"
            materials: PrincipledMaterial {
                baseColor: "green"
                lighting: PrincipledMaterial.NoLighting
            }
        }
    }
}
