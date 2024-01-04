// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers
import QtQuick3D.Xr

XrView {
    id: xrView
    XrErrorDialog { id: err }
    onInitializeFailed: (errorString) => err.run("XRView", errorString)

    environment: SceneEnvironment {
        clearColor: "black"
        backgroundMode: SceneEnvironment.Color
    }

    XrActor {
        id: actor
        position: Qt.vector3d(50, 2, 50)

        LeftHand {
            id: left
        }

        RightHand {
            id: right
        }
    }

    DirectionalLight {

    }

    Model {
        source: "#Cube"
        materials: DefaultMaterial {
            diffuseColor: "red"
        }

        y: -10

        scale: Qt.vector3d(0.1, 0.1, 0.1)


        NumberAnimation  on eulerRotation.y {
            duration: 10000
            easing.type: Easing.InOutQuad
            from: 0
            to: 360
            running: true
            loops: -1
        }
    }

    Node {
        x: 10
        y: 20
        Text {
            text: "Qt 6 in VR"
            font.pointSize: 12
            color: "white"
        }
        Text {
            text: "On " + xrView.runtimeInfo.runtimeName + " " + xrView.runtimeInfo.runtimeVersion + " with " + xrView.runtimeInfo.graphicsApiName
            font.pointSize: 4
            color: "white"
            y: 20
        }
    }

}


