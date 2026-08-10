// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Regression scene for QTBUG-148554.
// The sub-pass is declared as a QML child of the top-level pass and referenced
// via SubRenderPass. A sub-pass must carry no ordering state (nesting depth
// stays 0 even though it has a RenderPass parent), otherwise it could sort
// ahead of genuine top-level passes if it ever entered the scheduled list.

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

        // Top-level pass that invokes the nested pass below as a sub-pass.
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

            RenderPass {
                id: subPass
                objectName: "subPass"
                materialMode: RenderPass.OriginalMaterial
                commands: [
                    RenderablesFilter { layerMask: ContentLayer.Layer1 }
                ]
            }
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
