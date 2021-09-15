/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
import QtQuick3D.Helpers
import QtQuick.Controls
import QtQuick.Layouts

Window {
    width: 1024
    height: 768
    visible: true
    title: qsTr("Screen Space Reflections")

    View3D {
        id: screenSpaceReflectionsView
        anchors.fill: parent
        property double modelRotation: 0
        property double modelHeight: 0

        NumberAnimation on modelRotation {
            running: true
            from: 0
            to: 360
            duration: 10000
            loops: Animation.Infinite
        }

        SequentialAnimation on modelHeight {
            running: true
            loops: Animation.Infinite
            NumberAnimation {
                from: -5
                to: 20
                duration: 1000
            }
            NumberAnimation {
                from: 20
                to: -5
                duration: 1000
            }
        }

        environment: SceneEnvironment {
            backgroundMode: SceneEnvironment.SkyBox
            probeExposure: 2
            lightProbe: Texture {
                source: "maps/OpenfootageNET_lowerAustria01-1024.hdr"
            }
        }

        //! [scene]
        Node {

            Model {
                source: "#Cube"
                eulerRotation.y: 0
                scale: Qt.vector3d(1, 1, 1)
                position: Qt.vector3d(50.0, 40.0, 50.0)
                materials:  DefaultMaterial {
                    diffuseMap: Texture {
                        source: "qt_logo_rect.png"
                    }
                }
            }

            Node{

                Model {
                    source: "#Sphere"
                    position: Qt.vector3d(-400.0, screenSpaceReflectionsView.modelHeight, 0.0)
                    materials: DefaultMaterial {
                        diffuseColor: "magenta"
                    }
                }
            }

            Node{
                eulerRotation.y: screenSpaceReflectionsView.modelRotation
                position.y: screenSpaceReflectionsView.modelHeight

                Model {
                    source: "#Sphere"
                    pivot: Qt.vector3d(0, 0.0, 0.0)
                    position: Qt.vector3d(200.0, 0.0, 0.0)
                    materials: DefaultMaterial {
                        diffuseColor: "green"
                    }
                }
            }

            Node{
                eulerRotation.y: screenSpaceReflectionsView.modelRotation
                position.y: screenSpaceReflectionsView.modelHeight

                Model {
                    source: "#Sphere"
                    eulerRotation.y: 45
                    position: Qt.vector3d(0.0, 0.0, -200.0)
                    materials: DefaultMaterial {
                        diffuseColor: "blue"
                    }
                }
            }

            Node{
                eulerRotation.y: screenSpaceReflectionsView.modelRotation
                position.y: screenSpaceReflectionsView.modelHeight

                Model {
                    source: "#Sphere"
                    position: Qt.vector3d(0.0, 0.0, 200.0)
                    materials: DefaultMaterial {
                        diffuseColor: "red"
                    }
                }
            }
            //! [scene]

            //! [reflectingsurface]
            Model {
                source: "#Rectangle"
                scale: Qt.vector3d(5, 5, 5)
                eulerRotation.x: -90
                eulerRotation.z: 180
                position: Qt.vector3d(0.0, -50.0, 0.0)
                materials: ScreenSpaceReflections {
                    depthBias: depthBiasSlider.value
                    rayMaxDistance: distanceSlider.value
                    marchSteps: marchSlider.value
                    refinementSteps: refinementStepsSlider.value
                    specular: specularSlider.value
                    materialColor: materialColorCheckBox.checked ? "transparent" : "dimgray"
                }
            }
            //! [reflectingsurface]

            PerspectiveCamera {
                id: camera
                position: Qt.vector3d(0.0, 40.0, 500)
            }

            DirectionalLight {
                y: 0
                castsShadow: false
            }
        }

        WasdController {
            id: wasdController
            controlledObject: camera
        }

        MouseArea {
            anchors.fill: parent
            onClicked: {
                wasdController.forceActiveFocus()
            }
        }
    }

    DebugView {
        anchors.right: parent.right
        source: screenSpaceReflectionsView
    }


    Frame {
        background: Rectangle {
            color: "#c0c0c0"
            border.color: "#202020"
        }
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 10
        Column {
            id: settingsArea
            spacing: 5

            CheckBox {
                id: materialColorCheckBox
                text: "Transparent Material"
            }

            Row {

                Slider {
                    id: depthBiasSlider
                    from: 0.0
                    to: 5
                    value: 0.79
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: "Depth Bias: " + depthBiasSlider.value.toFixed(2);
                }
            }

            Row {

                Slider {
                    id: distanceSlider
                    from: 0.0
                    to: 400
                    value: 200
                    stepSize: 1
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: "Ray Distance: " + distanceSlider.value
                }
            }

            Row {

                Slider {
                    id: marchSlider
                    Layout.alignment: Qt.AlignHCenter
                    from: 0
                    to: 2000
                    value: 300
                    stepSize: 1
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: "March Steps: " + marchSlider.value
                }
            }

            Row {
                Slider {
                    id: refinementStepsSlider
                    Layout.alignment: Qt.AlignHCenter
                    from: 0
                    to: 50
                    value: 10
                    stepSize: 1
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: "Refinement Steps: " + refinementStepsSlider.value
                }
            }

            Row {
                Slider {
                    id: specularSlider
                    Layout.alignment: Qt.AlignHCenter
                    from: 0
                    to: 1
                    value: 1.0
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: "Specular: " + specularSlider.value.toFixed(2);
                }
            }
        }
    }
}
