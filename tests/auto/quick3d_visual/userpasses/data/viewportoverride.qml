// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// PipelineStateOverride viewport test.
// A large yellow sphere fills the entire view under normal rendering.
// The C++ test sets PipelineStateOverride.viewport at runtime to the RIGHT half
// of the render target (using device-pixel coordinates), so the sphere only
// appears in the right half while the left half stays black (clear colour).
// The test then switches the viewport to the LEFT half and verifies the result
// flips accordingly.

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers

Item {
    id: root
    width: 400
    height: 400

    View3D {
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
            materialMode: RenderPass.OriginalMaterial
            clearColor: "black"

            commands: [
                ColorAttachment { target: colorTex },
                DepthStencilAttachment {},
                // viewport set to an empty rect initially;
                // the C++ test calls pso->setProperty("viewport", ...) with
                // device-pixel coordinates computed from the window's DPR.
                PipelineStateOverride {
                    objectName: "pso"
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

        // Large yellow sphere that would fill the entire view without a viewport override
        Model {
            source: "#Sphere"
            scale: Qt.vector3d(4, 4, 4)
            materials: PrincipledMaterial {
                baseColor: "yellow"
                lighting: PrincipledMaterial.NoLighting
            }
        }
    }
}
