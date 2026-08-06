// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Regression scene for QTBUG-148554.
// The sub-pass is declared under a Node hierarchy (so it has a real spatial
// node parent) and is referenced by id from the top-level pass via
// SubRenderPass. Before the fix the dependency-index loop would assign the
// sub-pass its parent node's index; a sub-pass must instead carry no
// dependency index, otherwise it can sort ahead of genuine top-level passes.

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers

Item {
    width: 400
    height: 400

    View3D {
        anchors.fill: parent
        renderOverrides: View3D.DisableInternalPasses

        camera: cam
        PerspectiveCamera { id: cam; position: Qt.vector3d(0, 0, 300) }
        DirectionalLight {}

        RenderPassTexture { id: mainTex; format: RenderPassTexture.RGBA8 }
        RenderPassTexture { id: subTex; format: RenderPassTexture.RGBA8 }

        // Sub-pass under a Node -> non-null node parentItem with a non-zero index.
        Node {
            RenderPass {
                id: subPass
                objectName: "subPass"
                materialMode: RenderPass.OriginalMaterial
                commands: [
                    ColorAttachment { target: subTex },
                    DepthStencilAttachment {},
                    RenderablesFilter { layerMask: ContentLayer.Layer1 }
                ]
            }
        }

        // Top-level pass that invokes the sub-pass above.
        RenderPass {
            id: mainPass
            objectName: "mainPass"
            clearColor: "magenta"
            commands: [
                ColorAttachment { target: mainTex },
                DepthStencilAttachment {},
                RenderablesFilter { renderableTypes: RenderablesFilter.None },
                SubRenderPass { renderPass: subPass }
            ]
        }

        SimpleQuadRenderer {
            texture: Texture {
                textureProvider: RenderOutputProvider {
                    textureSource: RenderOutputProvider.UserPassTexture
                    renderPass: mainPass
                }
            }
        }

        Model {
            source: "#Cone"
            layers: ContentLayer.Layer1
            materials: PrincipledMaterial {
                baseColor: "cyan"
                lighting: PrincipledMaterial.NoLighting
            }
        }
    }
}
