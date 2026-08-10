// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Scene for runtime reparenting: the test moves passA between the viewport
// root, passB, and the Node, and asserts depth/validity re-derive each time.

import QtQuick
import QtQuick3D

Item {
    width: 200
    height: 200

    View3D {
        objectName: "view"
        anchors.fill: parent

        environment: SceneEnvironment {
            backgroundMode: SceneEnvironment.Color
            clearColor: "black"
        }

        PerspectiveCamera { position: Qt.vector3d(0, 0, 300) }
        DirectionalLight {}

        RenderPass {
            id: passA
            objectName: "passA"
            commands: [
                RenderablesFilter { renderableTypes: RenderablesFilter.None }
            ]
        }

        RenderPass {
            id: passB
            objectName: "passB"
            commands: [
                RenderablesFilter { renderableTypes: RenderablesFilter.None }
            ]
        }

        Node {
            objectName: "nodeParent"
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
