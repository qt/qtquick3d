/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tests of the Qt Toolkit.
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

import QtQuick3D 1.15
import QtQuick 2.15

Rectangle {
    width: 800
    height: 480
    color: Qt.rgba(1, 1, 0.5, 1)

    View3D {
        id: layer
        anchors.fill: parent
        environment: SceneEnvironment {
            clearColor: Qt.rgba(0, 0, 0, 1)
            aoDither: true
            depthPrePassEnabled: true
        }

        PerspectiveCamera {
            position: Qt.vector3d(0, 0, 600)
        }

        DirectionalLight {
            eulerRotation.x: -30
            eulerRotation.y: -70
        }

        Node {
            x: -200
            y: -200
            /*NumberAnimation on z {
                running: true
                to: 100
                duration: 1500
            }*/
            Text {
                text: "Text before Rectangle"
                color: "red"
                font.pixelSize: 40
            }
            Rectangle {
                width: 100
                height: 200
                color: "green"
            }
        }

        Node {
            x: -200
            y: 200
            Rectangle {
                width: 100
                height: 200
                color: "blue"
            }
            Text {
                text: "Text after Rectangle"
                color: "red"
                font.pixelSize: 40
            }
        }

        Node {
            x: 200
            y: 200
            Rectangle {
                width: 200
                height: 200
                color: "green"
                // rotation and transform ignored
                // because Rectangle parent is Node
                rotation: 30
                transform: Rotation {
                    origin.x: 30
                    origin.y: 30
                    axis { x: 0; y: 1; z: 0 }
                    angle: 45
                }
            }
        }

        Node {
            x: 200
            y: 200
            z: -10
            Text {
                text: "Node with Text further away"
                color: "red"
                font.pixelSize: 30
            }
        }

        Node {
            x: 200
            y: -200
            z: -10
            Rectangle {
                width: 200
                height: 200
                color: "black"
            }
        }

        Node {
            x: 200
            y: -200
            Item {
                width: 300
                height: 300
                Rectangle {
                    anchors.centerIn: parent
                    width: 200
                    height: 200
                    color: "green"
                    // rotation and transform respected
                    // because Rectangle parent is Item
                    rotation: 30
                    transform: Rotation {
                        origin.x: 40
                        origin.y: 40
                        axis { x: 0; y: 1; z: 0 }
                        angle: 25
                    }
                }
            }
        }

        Node {
            z: 100
            Image {
                width: 100
                height: 100
                source: "../shared/maps/checkerboard_1.png"
            }
        }
    }
}
