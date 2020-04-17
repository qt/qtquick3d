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
    width: 480
    height: 480
    color: Qt.rgba(1, 1, 0.5, 1)

    View3D {
        id: layer
        anchors.fill: parent
        environment: SceneEnvironment {
            clearColor: Qt.rgba(0, 0, 0, 1)
            aoDither: true
            depthPrePassEnabled: true
            depthTestEnabled: true
        }

        PerspectiveCamera {
            position: Qt.vector3d(0, 0, 800)
        }

        DirectionalLight {
        }

        DirectionalLight {
            eulerRotation.x: -30
            eulerRotation.y: -70
        }

        // Model behind of everything else
        Model {
            position: Qt.vector3d(0, 100, -400)
            eulerRotation: Qt.vector3d(45, 45, 45)
            scale: Qt.vector3d(3.5, 3.5, 3.5)
            source: "#Cube"
            materials: DefaultMaterial {
                diffuseColor: Qt.rgba(1, 0, 1, 1)
            }
        }

        // Model in front of everything else
        Model {
            position: Qt.vector3d(0, -100, 450)
            eulerRotation: Qt.vector3d(45, 45, 45)
            scale: Qt.vector3d(1.0, 1.0, 1.0)
            source: "#Cube"
            opacity: 1.0
            materials: DefaultMaterial {
                diffuseColor: Qt.rgba(0, 1, 1, 1)
            }
        }

        // Pile #1
        Node {
            x: -200
            y: 250
            Rectangle {
                width: 200
                height: 200
                color: "red"
                opacity: 0.5
            }
        }
        Node {
            x: -200
            y: 250
            z: 1
            Rectangle {
                width: 150
                height: 150
                color: "green"
                opacity: 0.5
            }
        }
        Node {
            x: -200
            y: 250
            z: 2
            Rectangle {
                width: 100
                height: 100
                color: "blue"
                opacity: 0.5
            }
        }

        // Pile #2
        Node {
            x: 200
            y: 250
            Rectangle {
                width: 200
                height: 200
                color: "red"
                opacity: 0.5
            }
        }
        Node {
            x: 200
            y: 250
            z: 2
            Rectangle {
                width: 100
                height: 100
                color: "blue"
                opacity: 0.5
            }
        }
        Node {
            x: 200
            y: 250
            z: 1
            Rectangle {
                width: 150
                height: 150
                color: "green"
                opacity: 0.5
            }
        }

        // Pile #3
        Node {
            x: -200
            y: 0
            z: -1
            Rectangle {
                width: 100
                height: 100
                color: "blue"
                opacity: 0.5
            }
        }
        Node {
            x: -200
            y: 0
            z: -2
            Rectangle {
                width: 150
                height: 150
                color: "green"
                opacity: 0.5
            }
        }
        Node {
            x: -200
            y: 0
            z: -3
            Rectangle {
                width: 200
                height: 200
                color: "red"
                opacity: 0.5
            }
        }

        // Pile #4
        Node {
            x: 200
            y: 0
            z: -1
            Rectangle {
                width: 100
                height: 100
                color: "blue"
                opacity: 0.5
            }
        }
        Node {
            x: 200
            y: 0
            z: 20
            NumberAnimation on z {
                to: -2
                duration: 200
            }
            Rectangle {
                width: 150
                height: 150
                color: "green"
                opacity: 0.5
            }
        }
        Node {
            x: 200
            y: 0
            z: 3
            Component.onCompleted: {
                z = -3;
            }
            Rectangle {
                width: 200
                height: 200
                color: "red"
                opacity: 0.5
            }
        }

        // Pile #5 with 3D models
        Model {
            position: Qt.vector3d(-200, -250, -20)
            eulerRotation: Qt.vector3d(45, 45, 45)
            scale: Qt.vector3d(0.4, 0.4, 0.4)
            opacity: 0.5
            source: "#Cube"
            Component.onCompleted: {
                z = 20;
            }
            materials: DefaultMaterial {
                diffuseColor: Qt.rgba(1, 1, 1, 1)
            }
        }
        Model {
            position: Qt.vector3d(-200, -250, -1)
            scale: Qt.vector3d(2, 2, 0.01)
            opacity: 0.5
            source: "#Cube"
            materials: DefaultMaterial {
                diffuseColor: Qt.rgba(1, 0, 0, 1)
            }
        }
        Node {
            x: -200
            y: -250
            opacity: 0.5
            Rectangle {
                width: 150
                height: 150
                color: "green"
                opacity: 0.5
            }
        }
        Model {
            position: Qt.vector3d(-200, -250, 1)
            //eulerRotation: Qt.vector3d(-10,-10,-10)
            scale: Qt.vector3d(1, 1, 0.01)
            opacity: 0.5
            source: "#Cube"
            materials: DefaultMaterial {
                diffuseColor: Qt.rgba(0, 0, 1, 1)
            }
        }

        // Pile #6 with 3D models
        Node {
            x: 200
            y: -250
            z: 10
            Rectangle {
                width: 200
                height: 200
                color: "red"
                opacity: 0.5
            }
        }
        Node {
            x: 200
            y: -250
            z: 10
            Model {
                scale: Qt.vector3d(1.5, 1.5, 0.01)
                opacity: 0.5
                source: "#Cube"
                materials: DefaultMaterial {
                    diffuseColor: Qt.rgba(0, 1, 0, 1)
                }
            }
        }
        Node {
            x: 200
            y: -250
            z: 12
            Rectangle {
                width: 100
                height: 100
                color: "blue"
                opacity: 0.5
            }
        }
        Model {
            position: Qt.vector3d(200, -250, -20)
            eulerRotation: Qt.vector3d(45, 45, 45)
            scale: Qt.vector3d(0.4, 0.4, 0.4)
            source: "#Cube"
            NumberAnimation on z {
                to: 20
                duration: 200
            }
            materials: DefaultMaterial {
                diffuseColor: Qt.rgba(1, 1, 1, 1)
            }
        }
    }
}
