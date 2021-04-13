/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
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

Window {
    width: 1280
    height: 720
    visible: true
    title: qsTr("Input in 3D")
    objectName: "root window"
    color: "green"

    Shortcut {
        sequence: StandardKey.Quit
        onActivated: Qt.quit()
    }

//    DebugView {
//        anchors.top: parent.top
//        anchors.left: parent.left
//        source: view3D
//    }

    View3D {
        id: view3D
        objectName: "Viewport3D"
        anchors.left: parent.left
        //anchors.leftMargin: 256
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right

        environment: SceneEnvironment {
            clearColor: "white"
            backgroundMode: SceneEnvironment.Color
        }

        PerspectiveCamera {
            z: 600
        }

        DirectionalLight {

        }

        DirectionalLight {
            eulerRotation.y: -90
        }

        Node {
            x: 0
            z: 300
            eulerRotation.y: 15
            Model {
                objectName: "middle cube in middle stack"
                source: "#Cube"
                pickable: true
                materials: DefaultMaterial {
                    diffuseMap: Texture {
                        sourceItem: InputDebugger {
                            objectName: "middle InputDebugger on middle stack of cubes"
                        }
                    }
                }
            }
            Model {
                objectName: "bottom cube in middle stack"
                y: -100
                source: "#Cube"
                pickable: true
                materials: PrincipledMaterial {
                    baseColorMap: Texture {
                        sourceItem: InputDebugger {
                            objectName: "bottom InputDebugger on middle stack of cubes"
                        }
                    }
                }
            }

            Model {
                objectName: "top cube in middle stack"
                y: 100
                source: "#Cube"
                pickable: true
                materials: CustomMaterial {
                    property TextureInput diffuse: TextureInput {
                        texture: Texture {
                            sourceItem: InputDebugger {
                                objectName: "top InputDebugger on middle stack of cubes"
                            }
                        }
                    }
                    fragmentShader: "custom.frag"
                }
            }
        }

        Node {
            x: 200
            z: 300
            Model {
                source: "#Cube"
                pickable: true
                objectName: "middle cube in right stack"
                materials: DefaultMaterial {
                    diffuseMap: Texture {
                        sourceItem: InputDebugger {
                            objectName: "lower-left InputDebugger, layer mapped onto right stack of cubes"
                            layer.enabled: true
                            layer.textureSize: Qt.size(512, 512)
                            id: sharedItem
                            x: 10; y: 400
                        }
                    }
                }
//                eulerRotation.z: 30
//                PropertyAnimation on eulerRotation.y {
//                    from: 0
//                    to: 360
//                    duration: 15000
//                    running: true
//                    loops: Animation.Infinite
//                }
            }
            Model {
                objectName: "bottom cube in right stack"
                y: -100
                source: "#Cube"
                pickable: true
                materials: PrincipledMaterial {
                    baseColorMap: Texture {
                        sourceItem: sharedItem
                    }
                }
            }

            Model {
                objectName: "top cube in right stack"
                y: 100
                source: "#Cube"
                pickable: true
                materials: CustomMaterial {
                    property TextureInput diffuse: TextureInput {
                        texture: Texture {
                            sourceItem: sharedItem
                        }
                    }
                    fragmentShader: "custom.frag"
                }
            }
        }



        Model {
            objectName: "rotating cube"
            source: "#Cube"
            x: 50
            z: 400
            scale: Qt.vector3d(0.2, 0.2, 0.2)
            materials: DefaultMaterial {
                diffuseColor: "tomato"
            }
            pickable: true
            eulerRotation.z: 20
            PropertyAnimation on eulerRotation.y {
                from: 0
                to: 360
                duration: 5000
                running: true
                loops: Animation.Infinite
            }
        }

//        Node {
//            x: -512
//            y: 256
//            eulerRotation.y: 65
//            InputDebugger {
//                objectName: "top-left rotated planar InputDebugger"
//            }
//        }
    }
}
