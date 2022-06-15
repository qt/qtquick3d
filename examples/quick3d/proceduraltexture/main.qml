// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick3D
import QtQuick.Controls
import QtQuick.Layouts

import ProceduralTextureExample

Window {
    id: window
    width: 480
    height: 320
    visible: true

    View3D {
        anchors.fill: parent

        DirectionalLight {
        }

        PerspectiveCamera {
            z: 300
        }

        //! [model]
        Model {
            source: "#Cube"

            materials: DefaultMaterial {
                diffuseMap: Texture {
                    textureData: GradientTexture {
                        id: gradientTexture
                        startColor: "#00dbde"
                        endColor: "#fc00ff"
                        width: size256.checked ? 256 : 16
                        height: width
                    }

                    minFilter: size256.checked ? Texture.Linear : Texture.Nearest
                    magFilter: size256.checked ? Texture.Linear : Texture.Nearest
                }
            }
        //! [model]

            PropertyAnimation on eulerRotation.y {
                from: 0
                to: 360
                duration: 5000
                loops: -1
                running: true
            }
        }
    }

    function randomColor() {
        return Qt.rgba(Math.random(),
                       Math.random(),
                       Math.random(),
                       1.0);
    }

    ColumnLayout {
        Label {
            text: "Set texture size:"
        }

        ButtonGroup  {
            id: sizeGroup
        }
        RadioButton {
            id: size256
            text: "256x256"
            checked: true
            ButtonGroup.group: sizeGroup
        }
        RadioButton {
            id: size512
            text: "16x16"
            checked: false
            ButtonGroup.group: sizeGroup
        }
        Button {
            text: "Random Start Color"
            onClicked: gradientTexture.startColor = randomColor();
        }
        Button {
            text: "Random End Color"
            onClicked: gradientTexture.endColor = randomColor();
        }
    }
}
