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

import QtQuick
import QtQuick.Window
import QtQuick3D

Window {
    visible: true
    width: 1280
    height: 720
    title: qsTr("Blend Modes Example")
    color: "#6b7080"

    Column {
        id: controlArea
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.margins: 20
        width: 220

        Text {
            text: qsTr("Background model")
            font.pixelSize: 20
            font.bold: true
        }
        ListView {
            id: modeList
            width: parent.width
            height: childrenRect.height
            model: modeModel
            delegate: Item {
                height: 26
                width: 140
                Text {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    text: mode
                    font.pixelSize: 20
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: modeList.currentIndex = index;
                }
            }
            highlight: Rectangle {
                color: "#53586b"
                radius: 4
            }
            focus: true
        }
        Item {
            width: 1
            height: 20
        }

        Text {
            text: qsTr("Foreground model")
            font.pixelSize: 20
            font.bold: true
        }
        ListView {
            id: modeList2
            width: parent.width
            height: childrenRect.height
            model: modeModel
            delegate: Item {
                height: 26
                width: 140
                Text {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    text: mode
                    font.pixelSize: 20
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: modeList2.currentIndex = index;
                }
            }
            highlight: Rectangle {
                color: "#53586b"
                radius: 4
            }
            focus: true
        }

    }

    ListModel {
        id: modeModel
        ListElement {
            mode: "SourceOver"
            modeType: DefaultMaterial.SourceOver
        }
        ListElement {
            mode: "Screen"
            modeType: DefaultMaterial.Screen
        }
        ListElement {
            mode: "Multiply"
            modeType: DefaultMaterial.Multiply
        }
    }

    Item {
        id: viewArea
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.left: controlArea.right

        View3D {
            anchors.fill: parent
            environment: SceneEnvironment {
                clearColor: "#848895"
                backgroundMode: SceneEnvironment.Color
            }

            DirectionalLight {
                brightness: 2
            }

            PerspectiveCamera {
                z: 500
            }

            // Background model
            Model {
                source: "#Cube"
                rotation: Quaternion.fromEulerAngles(-45, -45, 22.5)
                scale: Qt.vector3d(2,2,2)
                materials: DefaultMaterial {
                    diffuseColor: "#a8171a"
                    blendMode: modeModel.get(modeList.currentIndex).modeType
                }
            }

            // Foreground model
            Model {
                source: "#Cone"
                scale: Qt.vector3d(3,3,3)
                position.z: 50
                position.y: -100
                materials: DefaultMaterial {
                    diffuseColor: "#17a81a"
                    blendMode: modeModel.get(modeList2.currentIndex).modeType
                }
            }
        }
    }
}
