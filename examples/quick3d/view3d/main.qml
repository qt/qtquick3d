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

import QtQuick 2.15
import QtQuick.Window 2.14
import QtQuick3D 1.15
import QtQuick.Controls 2.14

Window {
    id: window
    width: 1280
    height: 720
    visible: true
    title: "View3Ds with Different Cameras"

    // The root scene
    //! [rootnode]
    Node {
        id: standAloneScene
        //! [rootnode]

        DirectionalLight {
            ambientColor: Qt.rgba(1.0, 1.0, 1.0, 1.0)
        }

        Model {
            source: "#Cube"
            y: -104
            scale: Qt.vector3d(3, 3, 0.1)
            eulerRotation.x: -90
            materials: [
                DefaultMaterial {
                    diffuseColor: Qt.rgba(0.8, 0.8, 0.8, 1.0)
                }
            ]
        }

        Model {
            source: "teapot.mesh"
            y: -100
            scale: Qt.vector3d(50, 50, 50)
            materials: [
                PrincipledMaterial {
                    baseColor: "#41cd52"
                    metalness: 0.75
                    roughness: 0.1
                    specularAmount: 1.0
                    indexOfRefraction: 2.5
                    opacity: 1.0
                }
            ]

            PropertyAnimation on eulerRotation.y {
                loops: Animation.Infinite
                duration: 5000
                to: 0
                from: -360
            }
        }

        //! [cameras start]
        // The predefined cameras. They have to be part of the scene, i.e. inside the root node.
        // Animated perspective camera
        Node {
            PerspectiveCamera {
                id: cameraPerspectiveOne
                z: 600
            }

            PropertyAnimation on eulerRotation.x {
                loops: Animation.Infinite
                duration: 5000
                to: -360
                from: 0
            }
        }

        // Stationary perspective camera
        PerspectiveCamera {
            id: cameraPerspectiveTwo
            z: 600
        }
        //! [cameras start]

        // Second animated perspective camera
        Node {
            PerspectiveCamera {
                id: cameraPerspectiveThree
                x: 500
                eulerRotation.y: 90
            }
            PropertyAnimation on eulerRotation.y {
                loops: Animation.Infinite
                duration: 5000
                to: 0
                from: -360
            }
        }

        // Stationary orthographic camera viewing from the top
        OrthographicCamera {
            id: cameraOrthographicTop
            y: 600
            eulerRotation.x: -90
        }

        // Stationary orthographic camera viewing from the front
        OrthographicCamera {
            id: cameraOrthographicFront
            z: 600
        }

        //! [cameras end]
        // Stationary orthographic camera viewing from left
        OrthographicCamera {
            id: cameraOrthographicLeft
            x: -600
            eulerRotation.y: -90
        }
    }
    //! [cameras end]

    //! [views]
    // The views
    Rectangle {
        id: topLeft
        anchors.top: parent.top
        anchors.left: parent.left
        width: parent.width * 0.5
        height: parent.height * 0.5
        color: "#848895"
        border.color: "black"

        View3D {
            id: topLeftView
            anchors.fill: parent
            importScene: standAloneScene
            camera: cameraOrthographicFront
        }

        Label {
            text: "Front"
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.margins: 10
            color: "#222840"
            font.pointSize: 14
        }
    }
    //! [views]

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
            anchors.margins: 10
            color: "#222840"
            font.pointSize: 14
        }

        View3D {
            id: topRightView
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.left: parent.left
            anchors.bottom: parent.bottom;
            camera: cameraPerspectiveOne
            importScene: standAloneScene
            renderMode: View3D.Underlay

            environment: SceneEnvironment {
                clearColor: "#848895"
                backgroundMode: SceneEnvironment.Color
            }
        }

        Row {
            id: controlsContainer
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 10
            padding: 10

            //! [buttons]
            RoundButton {
                text: "Camera 1"
                highlighted: topRightView.camera == cameraPerspectiveOne
                onClicked: {
                    topRightView.camera = cameraPerspectiveOne
                }
            }
            //! [buttons]
            RoundButton {
                text: "Camera 2"
                highlighted: topRightView.camera == cameraPerspectiveTwo
                onClicked: {
                    topRightView.camera = cameraPerspectiveTwo
                }
            }
            RoundButton {
                text: "Camera 3"
                highlighted: topRightView.camera == cameraPerspectiveThree
                onClicked: {
                    topRightView.camera = cameraPerspectiveThree
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
        color: "#848895"
        border.color: "black"

        View3D {
            id: bottomLeftView
            anchors.fill: parent
            importScene: standAloneScene
            camera: cameraOrthographicTop
        }

        Label {
            text: "Top"
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.margins: 10
            color: "#222840"
            font.pointSize: 14
        }
    }

    Rectangle {
        id: bottomRight
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        width: parent.width * 0.5
        height: parent.height * 0.5
        color: "#848895"
        border.color: "black"

        View3D {
            id: bottomRightView
            anchors.fill: parent
            importScene: standAloneScene
            camera: cameraOrthographicLeft
        }

        Label {
            text: "Left"
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.margins: 10
            color: "#222840"
            font.pointSize: 14
        }
    }
}
