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

import QtQuick 2.15
import QtQuick3D 1.15

Item {
    width: 480
    height: 480

    // Rectangles below and above View3D items
    Rectangle {
        z: 1
        width: 200
        height: 200
        anchors.centerIn: parent
        color: "#808080"
    }
    Rectangle {
        z: 10
        width: 150
        height: 150
        anchors.centerIn: parent
        color: "#606060"
    }

    Node {
        id: sceneRoot
        PerspectiveCamera {
            position: Qt.vector3d(0, 0, 350)
        }
        DirectionalLight {
            brightness: 100
        }
        Model {
            source: "#Cube"
            scale: Qt.vector3d(2,2,2)
            materials: DefaultMaterial {
                diffuseColor: "green"
            }
            eulerRotation: Qt.vector3d(45, 45, 45)
        }
    }

    View3D {
        z: 2
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: 20
        width: 200
        height: 200
        environment: SceneEnvironment {
            backgroundMode: SceneEnvironment.Color
            clearColor: "red"
        }
        renderMode: View3D.Offscreen
        importScene: sceneRoot
        Node {
            z: 220
            Text {
                font.pixelSize: 22
                text: "Offscreen"
            }
        }
    }
    View3D {
        z: 3
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 20
        width: 200
        height: 200
        environment: SceneEnvironment {
            backgroundMode: SceneEnvironment.Transparent
        }
        renderMode: View3D.Inline
        importScene: sceneRoot
        Node {
            z: 220
            Text {
                font.pixelSize: 22
                text: "Inline"
            }
        }
    }
    View3D {
        z: 4
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: 20
        width: 200
        height: 200
        // Must match the window's default clear color (white). Only here for
        // Qt 5 compatibility. Not effective in Qt 6!
        environment: SceneEnvironment {
            backgroundMode: SceneEnvironment.Color
            clearColor: "white"
        }
        renderMode: View3D.Underlay
        importScene: sceneRoot
        Node {
            z: 220
            Text {
                font.pixelSize: 22
                text: "Underlay"
            }
        }
    }
    View3D {
        z: 5
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 20
        width: 200
        height: 200
        environment: SceneEnvironment {
            backgroundMode: SceneEnvironment.Transparent
        }
        renderMode: View3D.Overlay
        importScene: sceneRoot
        Node {
            z: 220
            Text {
                font.pixelSize: 22
                text: "Overlay"
            }
        }
    }
}
