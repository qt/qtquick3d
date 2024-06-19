// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Window
import QtCore
import QtQuick3D
import QtQuick3D.Helpers

Item {
    id: previewRoot

    property url skyBoxTexturePath: "assets/skybox/OpenfootageNET_lowerAustria01-1024.hdr"
    property CustomMaterial currentMaterial: CustomMaterial {

    }

    property PrincipledMaterial fallbackMaterial: PrincipledMaterial {
        baseColor: "magenta"
    }

    property alias modelInstance: model
    property alias rootNode: resourceRoot

    Settings {
        property alias cameraOriginRotation: originNode.rotation
        property alias cameraRotation: sceneCamera.rotation
        property alias cameraPosition: sceneCamera.position
    }

    View3D {
        id: view
        anchors.fill: parent
        environment: SceneEnvironment {
            id: sceneEnvironment
            backgroundMode: previewControls.enableIBL ? SceneEnvironment.SkyBox : SceneEnvironment.Transparent
            lightProbe: previewControls.enableIBL ? skyboxTexture : null
        }

        Texture {
            id: skyboxTexture
            source: previewRoot.skyBoxTexturePath
        }

        Node {
            id: resourceRoot
        }

        property alias cameraOrigin: originNode

        Node {
            id: originNode
            PerspectiveCamera {
                id: sceneCamera
                z: 300
            }
        }

        camera: sceneCamera

        DirectionalLight {
            id: light
            z: 600
            eulerRotation: Qt.vector3d(30, 0, 0)
            visible: previewControls.enableDirectionalLight
        }

        Model {
            id: model
            source: previewControls.modelSource
            materials: [ previewRoot.currentMaterial, previewRoot.fallbackMaterial ]
        }

        OrbitCameraController {
            id: cameraController
            origin: originNode
            camera: sceneCamera
            panEnabled: false
        }
    }

    PreviewControls {
        id: previewControls
        width: parent.width
        targetView: view
        orbitCamera: cameraController
    }
}
