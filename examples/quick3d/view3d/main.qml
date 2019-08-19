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
import QtQuick.Window 2.12
import QtQuick3D 1.0
import QtQuick.Controls 2.4

Window {
    id: window
    width: 1280
    height: 720
    visible: true

    Node {
        id: standAloneScene

        Node {
            id: orbitingCamera
            Camera {
                id: camera1
                z: -600
            }
            SequentialAnimation on rotation {
                loops: Animation.Infinite
                PropertyAnimation { duration: 5000; to: Qt.vector3d(360, 0, 0); from: Qt.vector3d(0, 0, 0) }
            }
        }

        Camera {
            id: camera2
            z: -600
        }

        Node {
            id: orbitingCamera2

            Camera {
                id: camera3

                x: 500
                rotation: Qt.vector3d(0, -90, 0)
            }
            SequentialAnimation on rotation {
                loops: Animation.Infinite
                PropertyAnimation { duration: 5000; to: Qt.vector3d(0, 0, 0); from: Qt.vector3d(0, 360, 0) }
            }
        }


        Light {
            id: light2
            ambientColor: Qt.rgba(0.1, 0.1, 0.1, 1.0);
        }

        Model {
            source: "teapot.mesh"
            y: -100
            scale: Qt.vector3d(100, 100, 100)
            materials: [
                DefaultMaterial {
                    id: cubeMaterial2
                    diffuseColor: "salmon"
                }
            ]

            SequentialAnimation on rotation {
                loops: Animation.Infinite
                PropertyAnimation { duration: 5000; to: Qt.vector3d(0, 0, 0); from: Qt.vector3d(0, 360, 0) }
            }
        }
    }

    Rectangle {
        id: topLeft
        anchors.top: parent.top
        anchors.left: parent.left
        width: parent.width * 0.5
        height: parent.height * 0.5
        color: "grey"
        border.color: "black"

        View3D {
            id: topLeftView
            anchors.fill: parent
            scene: standAloneScene
            camera: camerafront

            Camera {
                id: camerafront
                z: -600
                projectionMode: Camera.Orthographic
                rotation: Qt.vector3d(0, 0, 0)
            }
        }

        Label {
            text: "Front"
            anchors.top: parent.top
            anchors.right: parent.right
            color: "limegreen"
            font.pointSize: 14
        }

    }

    Rectangle {
        id: topRight
        anchors.top: parent.top
        anchors.right: parent.right
        width: parent.width * 0.5
        height: parent.height * 0.5
        color: "transparent"
        border.color: "black"

        Label {
            text: "Perspective"
            anchors.top: parent.top
            anchors.right: parent.right
            color: "limegreen"
            font.pointSize: 14
        }

        View3D {
            id: topRightView
            anchors.top: controlsContainer.top
            anchors.right: parent.right
            anchors.left: parent.left
            anchors.bottom: parent.bottom;
            camera: camera1
            scene: standAloneScene
            renderMode: View3D.Underlay

            environment: SceneEnvironment {
                clearColor: "grey"
                backgroundMode: SceneEnvironment.Color
            }
        }

        Row {
            id: controlsContainer
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.left: parent.left
            Button {
                text: "Camera 1"
                onClicked: {
                    topRightView.camera = camera1
                }
            }
            Button {
                text: "Camera 2"
                onClicked: {
                    topRightView.camera = camera2
                }
            }

            Button {
                text: "Camera 3"
                onClicked: {
                    topRightView.camera = camera3
                }
            }
        }
    }

    Rectangle {
        id: bottomLeft
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        width: parent.width * 0.5
        height: parent.height * 0.5
        color: "grey"
        border.color: "black"

        View3D {
            id: bottomLeftView
            anchors.fill: parent
            scene: standAloneScene
            camera: cameratop

            Camera {
                id: cameratop
                y: 600
                projectionMode: Camera.Orthographic
                rotation: Qt.vector3d(90, 0, 0)
            }
        }

        Label {
            text: "Top"
            anchors.top: parent.top
            anchors.right: parent.right
            color: "limegreen"
            font.pointSize: 14
        }
    }

    Rectangle {
        id: bottomRight
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        width: parent.width * 0.5
        height: parent.height * 0.5
        color: "grey"
        border.color: "black"

        View3D {
            id: bottomRightView
            anchors.fill: parent
            scene: standAloneScene
            camera: cameratop

            Camera {
                id: cameraLeft
                x: -600
                projectionMode: Camera.Orthographic
                rotation: Qt.vector3d(0, 90, 0)
            }
        }

        Label {
            text: "Left"
            anchors.top: parent.top
            anchors.right: parent.right
            color: "limegreen"
            font.pointSize: 14
        }
    }
}
