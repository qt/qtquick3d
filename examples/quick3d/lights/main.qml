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
    width: 1680
    height: 1020
    visible: true

    View3D {
        id: topLeftView
        anchors.fill: parent

        environment: SceneEnvironment {
            clearColor: "darkGray"
            backgroundMode: SceneEnvironment.Color
            multisampleAAMode: SceneEnvironment.X4
        }

        Camera {
            id: camera
            z: -500
            projectionMode: Camera.Perspective
            rotation: Qt.vector3d(0, 0, 0)
            clipFar: 2000
        }

        DirectionalLight {
            id: light1
            color: Qt.rgba(1.0, 0.3, 0.3, 1.0)
            ambientColor: Qt.rgba(0.1, 0.1, 0.1, 1.0);
            rotation: Qt.vector3d(45, 80, 0)
        }

        PointLight {
            id: light2
            color: Qt.rgba(0.3, 1.0, 0.3, 1.0)
            ambientColor: Qt.rgba(0.1, 0.1, 0.1, 1.0);
            position: Qt.vector3d(0, 0, -300)
            quadraticFade: 1
        }

        AreaLight {
            id: light3
            color: Qt.rgba(0.3, 0.3, 1.0, 1.0)
            ambientColor: Qt.rgba(0.1, 0.1, 0.1, 1.0);
            position: Qt.vector3d(0, 100, -300)
            width: 1000
            height: 100
            brightness: 200
        }

        Model {
            source: "teapot.mesh"
            y: -200
            scale: Qt.vector3d(75, 75, 75)
            materials: [
                DefaultMaterial {
                    id: cubeMaterial
                    diffuseColor: Qt.rgba(0.8, 0.6, 0.4, 1.0)
                    specularTint: cubeMaterial.diffuseColor
                    specularModel: DefaultMaterial.KGGX
                    specularRoughness: 0.1
                    indexOfRefraction: 1.41
                    fresnelPower: 0.8
                }
            ]

            SequentialAnimation on rotation {
                loops: Animation.Infinite
                PropertyAnimation { duration: 5000; to: Qt.vector3d(0, 0, 0); from: Qt.vector3d(0, 360, 0) }
            }
        }
        Model {
            source: "#Rectangle"
            y: -200
            scale: Qt.vector3d(25, 25, 25)
            rotation: Qt.vector3d(90, 0, 0)
            materials: [
                DefaultMaterial {
                    id: planeMaterial
                    diffuseColor: Qt.rgba(0.8, 0.6, 0.4, 1.0)
                    specularTint: planeMaterial.diffuseColor
                    specularModel: DefaultMaterial.KGGX
                    specularRoughness: 0.1
                    indexOfRefraction: 1.41
                    fresnelPower: 0.8
                }
            ]
        }
    }
    Row {
        anchors.top: parent.top
        anchors.left: parent.left
        Column {
            CheckBox {
                text: "Directional Light"
                onClicked: light1.visible = !light1.visible
                checked: true
            }
            Slider {
                orientation: Qt.Vertical
                height: 400
                from: 0
                value: 100
                to: 500
                onValueChanged: light1.brightness = value
            }
        }
        Column {
            CheckBox {
                text: "Point Light"
                onClicked: light2.visible = !light2.visible
                checked: true
            }
            Slider {
                orientation: Qt.Vertical
                height: 400
                from: 0
                value: 100
                to: 500
                onValueChanged: light2.brightness = value
            }
        }
        Column {
            CheckBox {
                text: "Area Light"
                onClicked: light3.visible = !light3.visible
                checked: true
            }
            Slider {
                orientation: Qt.Vertical
                height: 400
                from: 0
                value: 200
                to: 500
                onValueChanged: light3.brightness = value
            }
        }
    }
}
