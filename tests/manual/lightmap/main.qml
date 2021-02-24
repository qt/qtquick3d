/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtShaderTools module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    property url modelSrc: "meshes/cube.mesh"

    Rectangle {
        color: "lightGray"
        width: parent.width / 2
        height: parent.height

        View3D {
            id: lightmapSource
            renderMode: View3D.Offscreen // so a texture provider as well
            anchors.fill: parent
            anchors.margins: 4

            environment: SceneEnvironment {
                clearColor: "transparent"
                backgroundMode: SceneEnvironment.Color
            }

            DirectionalLight {
            }

            PerspectiveCamera {
                id: camera1
                z: 200
            }

            Texture {
                id: baseTex
                source: "hskin.png"
                generateMipmaps: true
                mipFilter: Texture.Linear
            }

            PrincipledMaterial {
                id: principledMat
                roughness: 0.4
                baseColorMap: baseTex
            }

            CustomMaterial {
                id: customMat
                shadingMode: CustomMaterial.Shaded
                fragmentShader: "lightmapgen.frag"
                property TextureInput tex: TextureInput {
                    texture: baseTex
                }
            }

            CustomMaterial {
                id: lightmapGenMat
                shadingMode: CustomMaterial.Shaded
                vertexShader: "lightmapgen.vert"
                fragmentShader: "lightmapgen.frag"
                property bool useUV1: radioLightmapGen1.checked
                property TextureInput tex: TextureInput {
                    texture: baseTex
                }
                cullMode: Material.NoCulling
                // sourceBlend: CustomMaterial.SrcAlpha
                // destinationBlend: CustomMaterial.OneMinusSrcAlpha
            }

            Model {
                source: root.modelSrc
                scale: Qt.vector3d(10, 10, 10)
                NumberAnimation on eulerRotation.y {
                    running: cbRotate.checked
                    from: 0; to: 360; duration: 10000; loops: -1
                }
                materials: radioPrincipled.checked
                    ? principledMat
                    : (radioCustom.checked ? customMat : lightmapGenMat)
            }

            WasdController {
                controlledObject: camera1
            }
        }
    }

    View3D {
        width: parent.width / 2
        height: parent.height
        x: parent.width / 2

        environment: SceneEnvironment {
            clearColor: "white"
            backgroundMode: SceneEnvironment.Color
        }

        camera: camera1

        Model {
            source: root.modelSrc
            scale: Qt.vector3d(10, 10, 10)
            NumberAnimation on eulerRotation.y {
                running: cbRotate.checked
                from: 0; to: 360; duration: 10000; loops: -1
            }
            materials: CustomMaterial {
                shadingMode: CustomMaterial.Unshaded
                vertexShader: "lightmapuse.vert"
                fragmentShader: "lightmapuse.frag"
                property TextureInput tex: TextureInput {
                    texture: Texture {
                        sourceItem: lightmapSource
                        minFilter: Texture.Nearest
                        magFilter: Texture.Nearest
                    }
                }
            }
        }
    }

    Rectangle {
        id: controlPanelBackground
        x: 4
        y: 4
        color: "white"
        opacity: 0.5
        width: controlPanel.implicitWidth
        height: controlPanel.implicitHeight
    }
    Button {
        anchors.top: controlPanelBackground.top
        anchors.right: controlPanelBackground.right
        text: "Toggle"
        onClicked: {
            controlPanelBackground.visible = !controlPanelBackground.visible;
            controlPanel.visible = !controlPanel.visible;
        }
        focusPolicy: Qt.NoFocus
    }
    ColumnLayout {
        id: controlPanel
        x: 4
        y: 4
        RowLayout {
            Label {
                text: "Model"
            }
            ComboBox {
                model: [ "Cube",
                         "Sphere",
                         "Torus",
                         "Suzanne",
                         "Animal" ]
                onCurrentIndexChanged: {
                    var meshes = [ "meshes/cube.mesh",
                                   "meshes/sphere.mesh",
                                   "meshes/torus.mesh",
                                   "meshes/suzanne.mesh",
                                   "meshes/animal.mesh" ];
                    root.modelSrc = meshes[currentIndex]
                }
                focusPolicy: Qt.NoFocus
            }
        }
        RadioButton {
            id: radioPrincipled
            text: "PrincipledMaterial"
            checked: true
            focusPolicy: Qt.NoFocus
        }
        RadioButton {
            id: radioCustom
            text: "Custom material\nJust for verification; should look identical to Principled"
            checked: false
            focusPolicy: Qt.NoFocus
        }
        RadioButton {
            id: radioLightmapGen0
            text: "Lightmap generator custom material\nusing UV0"
            checked: false
            focusPolicy: Qt.NoFocus
        }
        RadioButton {
            id: radioLightmapGen1
            text: "Lightmap generator custom material\nusing UV1"
            checked: false
            focusPolicy: Qt.NoFocus
        }
        CheckBox {
            id: cbRotate
            text: "Rotate model\nto demo how the lightmap changes"
            checked: false
            focusPolicy: Qt.NoFocus
        }
    }

    Text {
        x: parent.width / 2
        text: "DefaultMaterial with no lighting,\njust a diffuseMap with UV1 and the texture of the left view.\nCamera is the same (use WASD to move)."
    }

    Text {
        anchors.right: parent.right
        anchors.margins: 10
        color: "red"
        style: Text.Outline
        font.pixelSize: 28
        text: {
            if (GraphicsInfo.api === GraphicsInfo.OpenGL)
                "OpenGL";
            else if (GraphicsInfo.api === GraphicsInfo.Direct3D11)
                "D3D11";
            else if (GraphicsInfo.api === GraphicsInfo.Vulkan)
                "Vulkan";
            else if (GraphicsInfo.api === GraphicsInfo.Metal)
                "Metal";
            else if (GraphicsInfo.api === GraphicsInfo.Null)
                "Null";
            else
                "Unknown";
        }
    }
}
