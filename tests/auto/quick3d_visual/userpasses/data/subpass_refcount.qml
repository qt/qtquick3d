// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// A pass referenced by SubRenderPass commands from TWO containers is a
// sub-pass as long as at least one reference remains: the classification
// counts references, so dropping one of two references must not flip the
// role back to top-level.

import QtQuick
import QtQuick3D

Item {
    id: root
    width: 200
    height: 200

    property bool refA: true
    property bool refB: true

    View3D {
        anchors.fill: parent

        environment: SceneEnvironment {
            backgroundMode: SceneEnvironment.Color
            clearColor: "black"
        }

        PerspectiveCamera { position: Qt.vector3d(0, 0, 300) }
        DirectionalLight {}

        RenderPass {
            id: sharedLeaf
            objectName: "sharedLeaf"
            commands: [
                RenderablesFilter { renderableTypes: RenderablesFilter.None }
            ]
        }

        RenderPass {
            id: containerA
            objectName: "containerA"
            commands: [
                RenderablesFilter { renderableTypes: RenderablesFilter.None },
                SubRenderPass { renderPass: root.refA ? sharedLeaf : null }
            ]
        }

        RenderPass {
            id: containerB
            objectName: "containerB"
            commands: [
                RenderablesFilter { renderableTypes: RenderablesFilter.None },
                SubRenderPass { renderPass: root.refB ? sharedLeaf : null }
            ]
        }

        Model {
            source: "#Cube"
            materials: PrincipledMaterial {
                baseColor: "red"
                lighting: PrincipledMaterial.NoLighting
            }
        }
    }
}
