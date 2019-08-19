/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

import QtQuick 2.12
import QtQuick.Window 2.11
import QtQuick3D 1.0
import QtQuick3D.MaterialLibrary 1.0

Window {
    id: window
    width: 1280
    height: 720
    visible: true

    View3D {
        id: layer1
        anchors.fill: parent
        camera: camera

        // Light always points the same direction as camera
//        Light {
//            id: directionalLight
//            lightType: Light.Directional
//            rotation: Qt.vector3d(0, 0, 0)
//            SequentialAnimation on rotation {
//                loops: Animation.Infinite
//                PropertyAnimation { duration: 5000; to: Qt.vector3d(0, 360, 0); from: Qt.vector3d(0, 0, 0) }
//            }
//        }

        environment: SceneEnvironment {
            probeBrightness: 1000
            clearColor: "green"
            backgroundMode: SceneEnvironment.Color
            lightProbe: Texture {
                source: "maps/OpenfootageNET_garage-1024.hdr"
            }
        }

        Node {
            id: cameraSpinner
            position: Qt.vector3d(0, 0, 0);


            Camera {
                id: camera
                position: Qt.vector3d(0, 0, -600)
            }

            rotation: Qt.vector3d(0, 90, 0)

            SequentialAnimation on rotation {
                loops: Animation.Infinite
                PropertyAnimation { duration: 5000; to: Qt.vector3d(0, 360, 0); from: Qt.vector3d(0, 0, 0) }
            }
        }

        Node {
            id: shapeSpawner
            Timer {
                property real range: 300
                property var instances: []
                property bool reverse: false

                running: true
                repeat: true
                interval: 500
                onTriggered: {
                    if (!reverse) {
                        // Create a new weirdShape at random postion
                        var xPos = Math.random() * (range - (-range)) + -range;
                        var yPos = Math.random() * (range - (-range)) + -range;
                        var zPos = Math.random() * (range - (-range)) + -range;
                        var weirdShapeComponent = Qt.createComponent("WeirdShape.qml");
                        let instance = weirdShapeComponent.createObject(shapeSpawner, {"x": xPos, "y": yPos, "z": zPos, "scale": Qt.vector3d(0.25, 0.25, 0.25)})
                        instances.push(instance);
                        //console.log("created WeirdShape[" + instances.length + "] at: (" + xPos + ", " + yPos + ", " + zPos + ")");
                        if (instances.length === 10)
                            reverse = true;
                    } else {
                        // remove last item in instances list
                        //console.log("removed WeirdShape[" + instances.length + "]");
                        let instance = instances.pop();
                        instance.destroy();
                        if (instances.length === 0) {
                            reverse = false;
                        }
                    }
                }
            }
        }



        WeirdShape {
            id: weirdShape1
            color: "red"
        }

        WeirdShape {
            id: weirdShape2
            color: "orange"
            x: 100
            y: 100
            z: 100
        }

        TexturedCube {
            z: -300
        }

        CopperCube {
            id: copperCube
            position: Qt.vector3d(300, 0, 0)
            SequentialAnimation on metalColor {
                loops: Animation.Infinite
                PropertyAnimation { duration: 2000; to: Qt.vector3d(0.805, 0.0, 0.305) }
                PropertyAnimation { duration: 2000; to: Qt.vector3d(0.805, 1.0, 0.305) }
            }
        }

        Model {
            position: Qt.vector3d(0, 0, 0)
            source: "#Cube"
            materials: [ GlassMaterial {
                }
            ]
        }

        Model {
            position: Qt.vector3d(0, 300, 0)
            source: "#Cube"
            materials: [ AluminumMaterial {
                }
            ]
        }

        Model {
            position: Qt.vector3d(0, -300, 0)
            source: "#Cube"
            materials: [ MeshFenceMaterial {
                }
            ]
        }

        Model {
            position: Qt.vector3d(-300, 0, 0)
            source: "#Cube"
            materials: [ FrostedGlassMaterial {
                }
            ]
        }
    }
}
