// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// A RenderPass declared under a Node is an active top-level pass; the Node is
// transparent for ordering and does not contribute to the nesting depth.

import QtQuick
import QtQuick3D

Item {
    width: 400
    height: 400

    View3D {
        anchors.fill: parent

        environment: SceneEnvironment {
            backgroundMode: SceneEnvironment.Color
            clearColor: "black"
        }

        PerspectiveCamera { position: Qt.vector3d(0, 0, 300) }
        DirectionalLight {}

        Node {
            RenderPass {
                id: groupedPass
                objectName: "groupedPass"
                commands: [
                    RenderablesFilter { layerMask: ContentLayer.Layer0 }
                ]
            }
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
