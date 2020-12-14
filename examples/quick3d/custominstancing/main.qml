/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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
import InstancingExample

Window {
    id: window
    width: 1280
    height: 720
    visible: true

    View3D {
        id: view
        anchors.fill: parent

        environment: SceneEnvironment {
            clearColor: "#8fafff"
            backgroundMode: SceneEnvironment.Color
        }

        Node {
            id: cameraNode
            state: "START"
            PerspectiveCamera {
                id: camera
                position: Qt.vector3d(0, 0, 650)
                eulerRotation.x: -20
            }
            // This would have been easier with a TimelineAnimation, but using
            // states and transitions avoids adding a dependency.
            property int zoomDuration: 4000
            property int rotationDuration: 7500
            SequentialAnimation {
                running: true
                PauseAnimation { duration: 1000 }
                ScriptAction { script: cameraNode.state = "ROTATED90" }
                PauseAnimation { duration: cameraNode.rotationDuration }
                ScriptAction { script: cameraNode.state = "HALF ZOOMED IN" }
                PauseAnimation { duration: cameraNode.zoomDuration }
                ScriptAction { script: cameraNode.state = "ZOOMED IN" }
                PauseAnimation { duration: cameraNode.zoomDuration + 1500 }
                ScriptAction { script: cameraNode.state = "ROTATED90"}
                PauseAnimation { duration: cameraNode.zoomDuration }

                ScriptAction { script: cameraNode.state = "ROTATED180"}
                PauseAnimation { duration: cameraNode.rotationDuration }
                ScriptAction { script: cameraNode.state = "HALF ZOOMED IN" }
                PauseAnimation { duration: cameraNode.zoomDuration }
                ScriptAction { script: cameraNode.state = "ZOOMED IN" }
                PauseAnimation { duration: cameraNode.zoomDuration + 1000 }
                ScriptAction { script: cameraNode.state = "ROTATED180"}
                PauseAnimation { duration: cameraNode.zoomDuration + 500}

                ScriptAction { script: cameraNode.state = "ROTATED360"}
                PauseAnimation { duration: cameraNode.rotationDuration }
                ScriptAction { script: cameraNode.state = "DIFFERENT ZOOM" }
                PauseAnimation { duration: cameraNode.zoomDuration + 1000 }
                ScriptAction { script: cameraNode.state = "ROTATED360"}
                PauseAnimation { duration: cameraNode.zoomDuration + 500}
                ScriptAction {
                    script: {
                        cameraTransition.enabled = false;
                        cameraNode.state = "START";
                        cameraTransition.enabled = true;
                    }
                }
                PauseAnimation { duration: 100 }
                loops: -1
            }
            transitions: Transition {
                id: cameraTransition
                NumberAnimation { target: camera; properties: "z"; duration: cameraNode.zoomDuration }
                NumberAnimation { target: camera; properties: "y"; duration: cameraNode.zoomDuration }
                NumberAnimation { target: camera; properties: "x"; duration: cameraNode.zoomDuration }
                NumberAnimation { target: cameraNode; properties: "eulerRotation.y"; duration: cameraNode.rotationDuration }
            }

            states:[
                State {
                    name: "START"
                    PropertyChanges {
                        target: cameraNode
                        restoreEntryValues: false
                        eulerRotation.y: 0
                    }
                },
                State {
                    name: "ROTATED90"
                    PropertyChanges {
                        target: cameraNode
                        restoreEntryValues: false
                        eulerRotation.y: 90
                    }
                },
                State {
                    name: "ROTATED180"
                    PropertyChanges {
                        target: cameraNode
                        restoreEntryValues: false
                        eulerRotation.y: 180
                    }
                },
                State {
                    name: "ROTATED360"
                    PropertyChanges {
                        target: cameraNode
                        restoreEntryValues: false
                        eulerRotation.y: 360
                    }
                },
                State {
                    name: "HALF ZOOMED IN"
                    PropertyChanges {
                        target: camera
                        z: 400
                        y: -300
                    }
                },
                State {
                    name: "ZOOMED IN"
                    PropertyChanges {
                        target: camera
                        z: 100
                        y: -300
                    }
                },
                State {
                    name: "DIFFERENT ZOOM"
                    PropertyChanges {
                        target: camera
                        z: 400
                        y: -300
                        x: -200
                    }
                }

            ]
        }

        DirectionalLight {
            eulerRotation.x: 250
            eulerRotation.y: -30
            brightness: 1.0
            ambientColor: "#7f7f7f"
        }

        CustomMaterial {
            id: cubeMaterial
            property real uTime: 0
            NumberAnimation on uTime {
                from: 0
                to: 1000000
                duration: 1000000
                loops: Animation.Infinite
            }
            vertexShader: "cubeMaterial.vert"
            fragmentShader: "cubeMaterial.frag"
        }

        Model {
            id: instancedCube
            property real cubeSize: 15
            scale: Qt.vector3d(cubeSize/100, cubeSize/100, cubeSize/100);
            source: "#Cube"
            instancing: CppInstanceTable {
                gridSize: 65
                gridSpacing: instancedCube.cubeSize
                randomSeed: 1522562186
            }
            materials: cubeMaterial
        }
    }
}
