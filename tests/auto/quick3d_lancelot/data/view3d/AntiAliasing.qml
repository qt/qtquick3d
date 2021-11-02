/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

import QtQuick
import QtQuick3D
import QtQuick.Controls

Rectangle {
    id: window
    width: 640
    height: 360
    visible: true
    color: "green"

    Node {
        id: standAloneScene

        DirectionalLight {
            brightness: 1.0
            eulerRotation.x: -25
        }

        Model {
            source: "#Cube"
            y: 0
            scale: Qt.vector3d(2, 2, 2)
            eulerRotation : Qt.vector3d(45,45,45)
            materials: [
                DefaultMaterial {
                    diffuseColor: "#FF00FF"
                }
            ]
        }

        OrthographicCamera {
            id: cameraOrthographic
            z: 600
            eulerRotation: Qt.vector3d(0, 0, 0)
        }
    }

    Rectangle {
        id: topLeft
        anchors.top: parent.top
        anchors.left: parent.left
        width: parent.width * 0.5
        height: parent.height
        color: window.color
        border.color: "black"

        View3D {
            id: topLeftView
            anchors.fill: parent
            importScene: standAloneScene
            camera: cameraOrthographic
            environment: SceneEnvironment
            {
                clearColor: window.color
                antialiasingMode: SceneEnvironment.MSAA
                antialiasingQuality: SceneEnvironment.VeryHigh
            }
        }

        Label {
            text: "MSAA VeryHigh"
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.margins: 10
            color: "#222840"
            font.pointSize: 14
        }
    }

    Rectangle {
        id: topRight
        anchors.top: parent.top
        anchors.right: parent.right
        width: parent.width * 0.5
        height: parent.height
        color: window.color
        border.color: "black"

        View3D {
            id: topRightView
            anchors.fill: parent
            importScene: standAloneScene
            camera: cameraOrthographic
            environment: SceneEnvironment
            {
                clearColor: window.color
                antialiasingMode: SceneEnvironment.SSAA
                antialiasingQuality: SceneEnvironment.VeryHigh
            }
        }

        Label {
            text: "SSAA VeryHigh"
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.margins: 10
            color: "#222840"
            font.pointSize: 14
        }
    }
}
