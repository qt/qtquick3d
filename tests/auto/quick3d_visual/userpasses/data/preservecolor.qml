// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// RenderPass.renderTargetFlags: PreserveColorContents test.
// Two render passes share one colour attachment texture (sharedTex).
// Pass 1 clears sharedTex to red but renders no geometry.
// Pass 2 has renderTargetFlags: RenderPass.PreserveColorContents which means it
// does NOT clear the colour buffer.  It renders a blue sphere on top of the
// red background left by pass 1.
// Expected result: background area = red, sphere area = blue.
// The C++ test clears renderTargetFlags at runtime → pass 2 then clears the
// buffer (background becomes black).

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

        // Shared colour attachment written by both passes
        RenderPassTexture {
            id: sharedTex
            format: RenderPassTexture.RGBA8
        }

        // Pass 1: clear sharedTex to red, do not render any geometry
        RenderPass {
            id: pass1
            clearColor: "red"
            commands: [
                ColorAttachment { target: sharedTex },
                DepthStencilAttachment {},
                RenderablesFilter { renderableTypes: RenderablesFilter.None }
            ]
        }

        // Pass 2: preserve the red from pass 1, then render blue sphere on top
        RenderPass {
            id: pass2
            objectName: "pass2"
            materialMode: RenderPass.OriginalMaterial
            renderTargetFlags: RenderPass.PreserveColorContents
            commands: [
                ColorAttachment { target: sharedTex },
                DepthStencilAttachment {}
            ]
        }

        SimpleQuadRenderer {
            texture: Texture {
                textureProvider: RenderOutputProvider {
                    textureSource: RenderOutputProvider.UserPassTexture
                    renderPass: pass2
                    attachmentSelector: RenderOutputProvider.Attachment0
                }
            }
        }

        // Blue sphere in the centre of the view
        Model {
            source: "#Sphere"
            scale: Qt.vector3d(2, 2, 2)
            materials: PrincipledMaterial {
                baseColor: "blue"
                lighting: PrincipledMaterial.NoLighting
            }
        }
    }
}
