// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick3D

Item {
    width: 400
    height: 200

    // Shared between both views through importScene. The box starts hidden
    // and on Layer1; the test toggles its visibility and layers at runtime.
    Node {
        id: sharedScene
        Model {
            objectName: "box"
            source: "#Cube"
            scale: Qt.vector3d(3, 3, 3)
            layers: ContentLayer.Layer1
            visible: false
            materials: PrincipledMaterial {
                lighting: PrincipledMaterial.NoLighting
                baseColor: "red"
            }
        }
    }

    component SharedView : View3D {
        width: 200
        height: 200
        importScene: sharedScene
        environment: SceneEnvironment {
            backgroundMode: SceneEnvironment.Color
            clearColor: "black"
        }
    }

    Row {
        // The left view is declared first, so its layer is prepared first and is the
        // one that consumes the shared node's dirty flags. Its camera does not see
        // Layer1, so the right view depends on the change being passed on.
        SharedView {
            objectName: "layer2View"
            camera: PerspectiveCamera { z: 300; layers: ContentLayer.Layer2 }
        }
        SharedView {
            objectName: "layer1View"
            camera: PerspectiveCamera { z: 300; layers: ContentLayer.Layer1 }
        }
    }
}
