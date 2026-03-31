// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// CullMode override in SubRenderPass with OverrideMaterial.
// A sphere with a blue material is rendered through a SubRenderPass that
// uses OverrideMaterial (red NoLighting DefaultMaterial) and a PipelineStateOverride.
// Phase 1 (useFrontCulling=false): Back culling -> red rectangle visible at center.
// Phase 2 (useFrontCulling=true): Front culling override -> rectangle culled -> black center.

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers

Rectangle {
    id: root
    width: 400
    height: 400
    color: "black"

    property bool useFrontCulling: false

    View3D {
        anchors.fill: parent
        renderOverrides: View3D.DisableInternalPasses

        camera: cam

        PerspectiveCamera {
            id: cam
            position: Qt.vector3d(0, 0, 600)
        }

        DefaultMaterial {
            id: redMat
            lighting: DefaultMaterial.NoLighting
            diffuseColor: "red"
        }

        RenderPassTexture {
            id: colorBuffer
            format: RenderPassTexture.RGBA16F
        }

        RenderPass {
            id: mainPass
            clearColor: Qt.rgba(0, 0, 0, 1)
            commands: [
                ColorAttachment { target: colorBuffer },
                DepthStencilAttachment {},
                RenderablesFilter { renderableTypes: RenderablesFilter.None },
                SubRenderPass {
                    renderPass: RenderPass {
                        materialMode: RenderPass.OverrideMaterial
                        overrideMaterial: redMat
                        commands: [
                            PipelineStateOverride {
                                objectName: "pso"
                                cullMode: root.useFrontCulling ? PipelineStateOverride.Front
                                                              : PipelineStateOverride.Back
                            }
                        ]
                    }
                }
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
            source: "#Rectangle"
            scale: Qt.vector3d(5, 5, 5)
            materials: PrincipledMaterial {
                baseColor: "blue"
                lighting: PrincipledMaterial.NoLighting
            }
        }
    }
}
