// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick3D

Item {
    id: root
    width: 640
    height: 480
    visible: true

    // Toggled from the test: loadView destroys and recreates the whole
    // View3D, loadPass only the ComputePass inside a View3D that stays alive.
    property bool loadView: true
    property bool loadPass: true

    // Bumped by the test before every frame. A destroyed View3D's layer and
    // nodes are only released when another Quick3D scene in the window
    // synchronizes, so this keeps one syncing while the other is churned.
    property int frameCounter: 0

    View3D {
        width: 1
        height: 1
        PerspectiveCamera {
            z: 300 + root.frameCounter % 2
        }
    }

    Loader {
        id: viewLoader
        anchors.fill: parent
        sourceComponent: root.loadView ? viewComponent : null
    }

    Component {
        id: viewComponent
        View3D {
            anchors.fill: parent

            environment: SceneEnvironment {
                backgroundMode: SceneEnvironment.Color
                clearColor: "black"
            }

            PerspectiveCamera {
                z: 300
            }

            ShaderImage {
                id: churnImageRes
                width: 4
                height: 4
                format: ShaderImage.RGBA8
            }

            // The compute pass owns a uniform buffer and an srb in its draw
            // call data, which must go away together with the pass.
            Loader3D {
                active: root.loadPass
                sourceComponent: ComputePass {
                    computeShader: "computeChurn.comp"
                    property ShaderImage churnImage: churnImageRes
                    commands: [
                        Dispatch { groupCountX: 4; groupCountY: 4; groupCountZ: 1 }
                    ]
                }
            }
        }
    }
}
