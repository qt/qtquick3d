// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Blend mode override in SubRenderPass with OriginalMaterial.
// A semi-transparent red sphere (opacity 0.5) is rendered on a blue background.
// Phase 1 (useStandardBlend=true):  SrcAlpha/OneMinusSrcAlpha -> red+blue mix -> purplish center.
// Phase 2 (useStandardBlend=false): One/Zero override -> only red sphere, no background -> red center.

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers

Item {
    id: root
    width: 400
    height: 400

    // Toggle from C++ test: true = SrcAlpha/OneMinusSrcAlpha, false = One/Zero
    property bool useStandardBlend: true

    View3D {
        anchors.fill: parent
        renderOverrides: View3D.DisableInternalPasses

        environment: SceneEnvironment {
            backgroundMode: SceneEnvironment.Color
            clearColor: "black"
        }

        PerspectiveCamera {
            z: 300
        }

        RenderPassTexture {
            id: colorTex
            format: RenderPassTexture.RGBA8
        }

        RenderPass {
            id: mainPass
            clearColor: "blue"
            commands: [
                ColorAttachment { target: colorTex },
                DepthStencilAttachment {},
                RenderablesFilter { renderableTypes: RenderablesFilter.None },
                SubRenderPass {
                    renderPass: RenderPass {
                        materialMode: RenderPass.OriginalMaterial
                        commands: [
                            RenderablesFilter {
                                renderableTypes: RenderablesFilter.Transparent
                            },
                            PipelineStateOverride {
                                objectName: "pso"
                                blendEnabled: true
                                targetBlend0.enable: true
                                targetBlend0.srcColor: root.useStandardBlend
                                    ? RenderTargetBlend.SrcAlpha : RenderTargetBlend.One
                                targetBlend0.dstColor: root.useStandardBlend
                                    ? RenderTargetBlend.OneMinusSrcAlpha : RenderTargetBlend.Zero
                                targetBlend0.srcAlpha: RenderTargetBlend.One
                                targetBlend0.dstAlpha: root.useStandardBlend
                                    ? RenderTargetBlend.OneMinusSrcAlpha : RenderTargetBlend.Zero
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
                    attachmentSelector: RenderOutputProvider.Attachment0
                }
            }
        }

        Model {
            source: "#Sphere"
            materials: PrincipledMaterial {
                baseColor: "red"
                opacity: 0.5
                lighting: PrincipledMaterial.NoLighting
                alphaMode: PrincipledMaterial.Blend
            }
        }
    }
}
