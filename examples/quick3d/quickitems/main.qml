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

import QtQuick 2.15
import QtQuick.Window 2.12
import QtQuick3D 1.15

Window {
    width: 800
    height: 480
    visible: true
    title: qsTr("Quick Items Example")

    View3D {
        anchors.fill: parent
        environment: SceneEnvironment {
            clearColor: "#808080"
            backgroundMode: SceneEnvironment.Color
            antialiasingMode: SceneEnvironment.MSAA
            antialiasingQuality: SceneEnvironment.High
        }

        PerspectiveCamera {
            id: camera
            property real cameraAnimation: 1
            SequentialAnimation on cameraAnimation {
                loops: Animation.Infinite
                NumberAnimation {
                    to: -1
                    duration: 5000
                    easing.type: Easing.InOutQuad
                }
                NumberAnimation {
                    to: 1
                    duration: 5000
                    easing.type: Easing.InOutQuad
                }
            }
            position: Qt.vector3d(200 * cameraAnimation, 300, 500)
            eulerRotation.x: -20
            eulerRotation.y: 20 * cameraAnimation
        }

        DirectionalLight {
            eulerRotation: Qt.vector3d(-135, -110, 0)
            brightness: 100
        }

        SpotLight {
            position: Qt.vector3d(0, 500, 600)
            eulerRotation.x: -45
            brightness: 3000
        }

        //! [walls models]
        Model {
            source: "#Rectangle"
            y: -500
            scale: Qt.vector3d(12, 12, 12)
            eulerRotation.x: -90
            materials: DefaultMaterial {
                diffuseColor: Qt.rgba(0.5, 0.5, 0.5, 1.0)
            }
        }
        Model {
            source: "#Rectangle"
            x: -600
            scale: Qt.vector3d(12, 10, 12)
            eulerRotation.y: 90
            materials: DefaultMaterial {
                diffuseColor: Qt.rgba(0.8, 0.8, 0.6, 1.0)
            }
        }
        Model {
            source: "#Rectangle"
            z: -600
            scale: Qt.vector3d(12, 10, 12)
            materials: DefaultMaterial {
                diffuseColor: Qt.rgba(0.8, 0.8, 0.6, 1.0)
            }
        }
        //! [walls models]

        //! [background item]
        Node {
            position: Qt.vector3d(0, 100, -120)
            Item {
                width: 400
                height: 400
                Rectangle {
                    anchors.fill: parent
                    opacity: 0.4
                    color: "#202020"
                    radius: 10
                    border.width: 2
                    border.color: "#f0f0f0"
                }
                Text {
                    anchors.top: parent.top
                    anchors.topMargin: 10
                    anchors.horizontalCenter: parent.horizontalCenter
                    font.pixelSize: 20
                    color: "#e0e0e0"
                    style: Text.Raised
                    text: qsTr("Background Item")
                }
                Image {
                    anchors.centerIn: parent
                    source: "Built_with_Qt_RGB_logo_vertical"
                }
            }
        }
        //! [background item]

        //! [circles item]
        Node {
            position: Qt.vector3d(0, 150, 100)
            SequentialAnimation on x {
                loops: Animation.Infinite
                NumberAnimation {
                    to: -200
                    duration: 1500
                    easing.type: Easing.InOutQuad
                }
                NumberAnimation {
                    to: 200
                    duration: 1500
                    easing.type: Easing.InOutQuad
                }
            }
            NumberAnimation on eulerRotation.z {
                loops: Animation.Infinite
                from: 0
                to: 360
                duration: 4000
                easing.type: Easing.InOutBack
            }
            Item {
                width: 400
                height: 400
                // This allows rendering into offscreen surface and caching it.
                layer.enabled: true
                Rectangle {
                    x: 150
                    y: 100
                    width: 100
                    height: 100
                    radius: 50
                    color: "#80808020"
                    border.color: "black"
                    border.width: 2
                }
                Rectangle {
                    x: 90
                    y: 200
                    width: 100
                    height: 100
                    radius: 50
                    color: "#80808020"
                    border.color: "black"
                    border.width: 2
                }
                Rectangle {
                    x: 210
                    y: 200
                    width: 100
                    height: 100
                    radius: 50
                    color: "#80808020"
                    border.color: "black"
                    border.width: 2
                }
            }
        }
        //! [circles item]

        //! [text item]
        Node {
            position: Qt.vector3d(0, 80, 250)
            Text {
                width: 300
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignJustify
                font.pixelSize: 14
                color: "#e0e0e0"
                style: Text.Raised
                text: "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod " +
                      "tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim " +
                      "veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea " +
                      "commodo consequat."
                SequentialAnimation on opacity {
                    loops: Animation.Infinite
                    NumberAnimation {
                        to: 0
                        duration: 1500
                        easing.type: Easing.InOutQuad
                    }
                    NumberAnimation {
                        to: 1
                        duration: 1500
                        easing.type: Easing.InOutQuad
                    }
                }
            }
        }
        //! [text item]

        //! [wrecking ball model]
        Node {
            position: Qt.vector3d(0, 800, 0)
            SequentialAnimation on eulerRotation.x {
                loops: Animation.Infinite
                NumberAnimation {
                    to: 20
                    duration: 3500
                    easing.type: Easing.InOutQuad
                }
                NumberAnimation {
                    to: -20
                    duration: 3500
                    easing.type: Easing.InOutQuad
                }
            }
            Model {
                source: "#Cylinder"
                y: -300
                scale: Qt.vector3d(0.1, 6.1, 0.1)
                materials: DefaultMaterial {
                    diffuseColor: Qt.rgba(0.9, 0.9, 0.9, 1.0)
                }
            }
            Model {
                source: "#Sphere"
                y: -700
                scale: Qt.vector3d(2, 2, 2)
                materials: DefaultMaterial {
                    diffuseColor: Qt.rgba(0.4, 0.4, 0.4, 1.0)
                }
            }
        }
        //! [wrecking ball model]
    }
}
