// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers

Rectangle {
    id: primitives
    width: 800
    height: 480
    color: Qt.rgba(0, 0, 0, 1)

    View3D {
        id: layer
        anchors.fill: parent
        visible: true

        environment: SceneEnvironment {
            clearColor:  Qt.rgba(0, 0, 0, 1)
            backgroundMode: SceneEnvironment.Color
        }

        PointLight {
            shadowFactor: 50
            castsShadow: true
            position: Qt.vector3d(200, 500, -500)
            brightness: 10
        }

        PointLight {
            shadowFactor: 50
            castsShadow: true
            position: Qt.vector3d(-200, 500, 500)
            brightness: 10
        }

        camera : PerspectiveCamera {
            id: cam
            position.z: 1000
            position.y: 65
            levelOfDetailBias: 0.25
        }

        Model {
            source: "../shared/models/lod_mixed_shapes.mesh"
            position: Qt.vector3d(-400, 0, 300)
            materials: [uvdebug]
        }

        Model {
            source: "../shared/models/lod_mixed_shapes.mesh"
            position: Qt.vector3d(0, 0, -300)
            materials: PrincipledMaterial { baseColor: "red" }
        }

        Model {
            source: "../shared/models/lod_mixed_shapes.mesh"
            position: Qt.vector3d(400, 0, -1000)
            materials: PrincipledMaterial { baseColor: "cyan" }
        }

        Model {
            source: "#Cube"
            position: Qt.vector3d(0, -200, -500)
            scale: Qt.vector3d(20, 0.05, 20)
            materials: PrincipledMaterial { baseColor: "white" }
        }
    }

    CustomMaterial {
        id: uvdebug
        fragmentShader: "uvmaterial.frag"
    }

    WasdController {
        controlledObject: cam
    }
}
