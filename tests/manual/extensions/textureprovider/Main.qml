// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Window

import QtQuick3D
import QtQuick3DTest.Extensions.TextureProvider

Window {
    id: window
    width: 640
    height: 480
    visible: true
    title: qsTr("Texture provider")

    View3D {
        id: view
        property int api: GraphicsInfo.api
        anchors.fill: parent
        extensions: [ RenderExtension {
                id: renderer
            } ]

        Texture {
            id: texture
            textureProvider: renderer
            flipV: view.api !== GraphicsInfo.OpenGL
        }

        camera: PerspectiveCamera {
            z: 600
        }

        DirectionalLight {

        }

        Model {
            source: "#Cube"
            materials: [ PrincipledMaterial {
                    baseColorMap: texture
            } ]

            SequentialAnimation on eulerRotation.y {
                running: true
                NumberAnimation { from: 0; to: 360; duration: 4000 }
                loops: -1
            }
        }
    }
}
