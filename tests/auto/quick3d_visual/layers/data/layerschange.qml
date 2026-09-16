// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick3D

Item {
    width: 200
    height: 200

    View3D {
        anchors.fill: parent
        environment: SceneEnvironment {
            backgroundMode: SceneEnvironment.Color
            clearColor: "black"
        }
        // The explicit camera's layers is the view's mask: only Layer0.
        camera: PerspectiveCamera {
            z: 300
            layers: ContentLayer.Layer0
        }
        DirectionalLight {}

        // Starts outside the mask; the test moves it in and out at runtime.
        Model {
            objectName: "box"
            source: "#Cube"
            scale: Qt.vector3d(3, 3, 3)
            layers: ContentLayer.Layer1
            materials: PrincipledMaterial {
                lighting: PrincipledMaterial.NoLighting
                baseColor: "red"
            }
        }
    }
}
