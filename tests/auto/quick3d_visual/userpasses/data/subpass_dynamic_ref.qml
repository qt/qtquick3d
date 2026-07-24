// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Scene for dynamic reference-lifetime tests: the test creates and destroys
// passes at runtime and drives the SubRenderPass reference through the
// staticSubCmd command, asserting that the referenced pass's classification
// follows the reference count.

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
            id: staticLeaf
            objectName: "staticLeaf"
            commands: [
                RenderablesFilter { renderableTypes: RenderablesFilter.None }
            ]
        }

        RenderPass {
            id: staticContainer
            objectName: "staticContainer"
            commands: [
                RenderablesFilter { renderableTypes: RenderablesFilter.None },
                SubRenderPass { objectName: "staticSubCmd" }
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
