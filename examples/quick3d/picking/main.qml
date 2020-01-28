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
import QtQuick.Controls 2.14
import QtQuick3D 1.15

Window {
    visible: true
    width: 1280
    height: 720
    title: qsTr("Picking Example")

    Row {
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 20
        Label {
            id: pickName
            color: "#222840"
            font.pointSize: 14
            text: "Last Pick: None"
        }
        Label {
            id: pickPosition
            color: "#222840"
            font.pointSize: 14
            text: "Screen Position: (0, 0)"
        }
        Label {
            id: uvPosition
            color: "#222840"
            font.pointSize: 14
            text: "UV Position: (0.00, 0.00)"
        }
        Label {
            id: distance
            color: "#222840"
            font.pointSize: 14
            text: "Distance: 0.00"
        }
        Label {
            id: scenePosition
            color: "#222840"
            font.pointSize: 14
            text: "World Position: (0.00, 0.00)"
        }
    }

    View3D {
        id: view
        anchors.fill: parent
        renderMode: View3D.Underlay

        PointLight {
            x: -200
            y: 200
            z: 300
            quadraticFade: 0
            brightness: 150
        }

        PerspectiveCamera {
            z: 500
        }

        environment: SceneEnvironment {
            clearColor: "#848895"
            backgroundMode: SceneEnvironment.Color
        }

        //! [pickable model]
        Model {
            id: cubeModel
            objectName: "Cube"
            source: "#Cube"
            pickable: true
            property bool isPicked: false
            //! [pickable model]

            scale.x: 1.5
            scale.y: 2
            scale.z: 1.5

            //! [picked color]
            materials: DefaultMaterial {
                diffuseColor: cubeModel.isPicked ? "#41cd52" : "#09102b"
                //! [picked color]
                specularAmount: 0.25
                specularRoughness: 0.2
                roughnessMap: Texture { source: "maps/roughness.jpg" }
            }

            //! [picked animation]
            SequentialAnimation on eulerRotation {
                running: !cubeModel.isPicked
                //! [picked animation]
                loops: Animation.Infinite
                PropertyAnimation {
                    duration: 2500
                    from: Qt.vector3d(0, 0, 0)
                    to: Qt.vector3d(360, 360, 360)
                }
            }
        }

        Model {
            id: coneModel
            objectName: "Cone"
            source: "#Cone"
            pickable: true
            property bool isPicked: false

            x: 200
            z: 100

            scale.x: 2
            scale.y: 1.5
            scale.z: 2

            materials: DefaultMaterial {
                diffuseColor: coneModel.isPicked ? "#53586b" : "#21be2b"
                specularAmount: 1
                specularRoughness: 0.1
                roughnessMap: Texture { source: "maps/roughness.jpg" }
            }

            SequentialAnimation on eulerRotation {
                running: !coneModel.isPicked
                loops: Animation.Infinite
                PropertyAnimation {
                    duration: 10000
                    from: Qt.vector3d(0, 0, 0)
                    to: Qt.vector3d(-360, 360, 0)
                }
            }
        }

        Model {
            id: sphereModel
            objectName: "Sphere"
            source: "#Sphere"
            pickable: true
            property bool isPicked: false

            x: -100
            y: -100
            z: -100

            scale.x: 5
            scale.y: 3
            scale.z: 1

            materials: DefaultMaterial {
                diffuseColor: sphereModel.isPicked ? "#17a81a" : "#9d9faa"
                specularAmount: 0.25
                specularRoughness: 0.2
                roughnessMap: Texture { source: "maps/roughness.jpg" }
            }

            SequentialAnimation on eulerRotation.x {
                running: !sphereModel.isPicked
                loops: Animation.Infinite
                PropertyAnimation {
                    duration: 5000
                    from: 0
                    to: 360
                }
            }
        }
    }

    //! [mouse area]
    MouseArea {
        anchors.fill: view
        //! [mouse area]

        onClicked: {
            // Get screen coordinates of the click
            pickPosition.text = "Screen Position: (" + mouse.x + ", " + mouse.y + ")"
            //! [pick result]
            var result = view.pick(mouse.x, mouse.y);
            //! [pick result]
            //! [pick specifics]
            if (result.objectHit) {
                var pickedObject = result.objectHit;
                // Toggle the isPicked property for the model
                pickedObject.isPicked = !pickedObject.isPicked;
                // Get picked model name
                pickName.text = "Last Pick: " + pickedObject.objectName;
                // Get other pick specifics
                uvPosition.text = "UV Position: ("
                        + result.uvPosition.x.toFixed(2) + ", "
                        + result.uvPosition.y.toFixed(2) + ")";
                distance.text = "Distance: " + result.distance.toFixed(2);
                scenePosition.text = "World Position: ("
                        + result.scenePosition.x.toFixed(2) + ", "
                        + result.scenePosition.y.toFixed(2) + ")";
                //! [pick specifics]
            } else {
                pickName.text = "Last Pick: None";
            }
        }
    }
}
