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
import QtQuick3D
import QtQuick.Controls

Window {
    width: 1280
    height: 720
    visible: true
    title: qsTr("Lights Example")

    View3D {
        anchors.fill: parent

        environment: SceneEnvironment {
            clearColor: "#808080"
            backgroundMode: SceneEnvironment.Color
            antialiasingMode: SceneEnvironment.MSAA
            antialiasingQuality: SceneEnvironment.High
        }

        PerspectiveCamera {
            position: Qt.vector3d(0, 400, 600)
            eulerRotation.x: -30
            clipFar: 2000
        }

        //! [directional light]
        DirectionalLight {
            id: light1
            color: Qt.rgba(1.0, 0.1, 0.1, 1.0)
            ambientColor: Qt.rgba(0.1, 0.1, 0.1, 1.0)
            position: Qt.vector3d(0, 200, 0)
            rotation: Quaternion.fromEulerAngles(-135, -90, 0)
            shadowMapQuality: Light.ShadowMapQualityHigh
            visible: checkBox1.checked
            castsShadow: checkBoxShadows.checked
            brightness: slider1.sliderValue
            SequentialAnimation on rotation {
                loops: Animation.Infinite
                QuaternionAnimation {
                    to: Quaternion.fromEulerAngles(-45, -90, 0)
                    duration: 2000
                    easing.type: Easing.InOutQuad
                }
                QuaternionAnimation {
                    to: Quaternion.fromEulerAngles(-135, -90, 0)
                    duration: 2000
                    easing.type: Easing.InOutQuad
                }
            }
        }
        //! [directional light]

        //! [point light]
        PointLight {
            id: light2
            color: Qt.rgba(0.1, 1.0, 0.1, 1.0)
            ambientColor: Qt.rgba(0.1, 0.1, 0.1, 1.0)
            position: Qt.vector3d(0, 300, 0)
            shadowMapFar: 2000
            shadowMapQuality: Light.ShadowMapQualityHigh
            visible: checkBox2.checked
            castsShadow: checkBoxShadows.checked
            brightness: slider2.sliderValue
            SequentialAnimation on x {
                loops: Animation.Infinite
                NumberAnimation {
                    to: 400
                    duration: 2000
                    easing.type: Easing.InOutQuad
                }
                NumberAnimation {
                    to: 0
                    duration: 2000
                    easing.type: Easing.InOutQuad
                }
            }
        }
        //! [point light]

        //! [spot light]
        SpotLight {
            id: light4
            color: Qt.rgba(1.0, 0.9, 0.7, 1.0)
            ambientColor: Qt.rgba(0.0, 0.0, 0.0, 0.0)
            position: Qt.vector3d(0, 250, 0)
            eulerRotation.x: -45
            shadowMapFar: 2000
            shadowMapQuality: Light.ShadowMapQualityHigh
            visible: checkBox4.checked
            castsShadow: checkBoxShadows.checked
            brightness: slider4.sliderValue
            coneAngle: 50
            innerConeAngle: 30
            PropertyAnimation on eulerRotation.y {
                loops: Animation.Infinite
                from: 0
                to: -360
                duration: 10000
            }
        }
        //! [spot light]

        //! [rectangle models]
        Model {
            source: "#Rectangle"
            y: -200
            scale: Qt.vector3d(15, 15, 15)
            eulerRotation.x: -90
            materials: [
                DefaultMaterial {
                    diffuseColor: Qt.rgba(0.8, 0.6, 0.4, 1.0)
                }
            ]
        }
        Model {
            source: "#Rectangle"
            z: -400
            scale: Qt.vector3d(15, 15, 15)
            materials: [
                DefaultMaterial {
                    diffuseColor: Qt.rgba(0.8, 0.8, 0.9, 1.0)
                }
            ]
        }
        //! [rectangle models]

        RotatingTeaPot {
            visible: !checkBoxCustomMaterial.checked
            material: DefaultMaterial {
                diffuseColor: Qt.rgba(0.9, 0.9, 0.9, 1.0)
            }
            animate: checkBoxAnimate.checked
        }

        RotatingTeaPot {
            visible: checkBoxCustomMaterial.checked
            material: CustomMaterial {
                vertexShader: "custom.vert"
                property real uAmplitude: 0.5
                property real uTime: 0.0
                SequentialAnimation on uTime {
                    loops: -1
                    NumberAnimation { from: 0.0; to: 10.0; duration: 10000 }
                    NumberAnimation { from: 10.0; to: 0.0; duration: 10000 }
                }
            }
            animate: checkBoxAnimate.checked
        }

        //! [light models]
        Model {
            source: "#Cube"
            position: light1.position
            rotation: light1.rotation
            property real size: slider1.highlight ? 0.2 : 0.1
            scale: Qt.vector3d(size, size, size)
            materials: [
                DefaultMaterial {
                    diffuseColor: light1.color
                    opacity: 0.4
                }
            ]
        }
        Model {
            source: "#Cube"
            position: light2.position
            rotation: light2.rotation
            property real size: slider2.highlight ? 0.2 : 0.1
            scale: Qt.vector3d(size, size, size)
            materials: [
                DefaultMaterial {
                    diffuseColor: light2.color
                    opacity: 0.4
                }
            ]
        }
        Model {
            source: "#Cube"
            position: light4.position
            rotation: light4.rotation
            property real size: slider4.highlight ? 0.2 : 0.1
            scale: Qt.vector3d(size, size, size)
            materials: [
                DefaultMaterial {
                    diffuseColor: light4.color
                    opacity: 0.4
                }
            ]
        }
        //! [light models]
    }

    Button {
        x: settingsDrawer.visible ? (settingsDrawer.x + settingsDrawer.width) : 0
        anchors.top: parent.top
        width: 50
        height: width
        icon.width: width * 0.5
        icon.height: height * 0.5
        icon.source: "icon_settings.png"
        icon.color: "transparent"
        background: Rectangle {
            color: "transparent"
        }
        onClicked: {
            inTransition.duration = 400
            settingsDrawer.visible = !settingsDrawer.visible;
        }
    }

    Drawer {
        id: settingsDrawer
        edge: Qt.LeftEdge
        interactive: false
        modal: false
        background: Rectangle {
            color: "#e0e0e0"
            opacity: 0.8
        }
        visible: (Qt.platform.os === ("android" || "ios") ? false : true)

        enter: Transition {
            NumberAnimation {
                id: inTransition
                property: "position"
                to: 1.0
                duration: 0
                easing.type: Easing.InOutQuad
            }
        }

        exit: Transition {
            NumberAnimation {
                property: "position"
                to: 0.0
                duration: 400
                easing.type: Easing.InOutQuad
            }
        }

        ScrollView {
            anchors.fill: parent
            padding: 10

            Flickable{
                clip: true
                contentWidth: settingsArea.width
                contentHeight: settingsArea.height

                Column {
                    id: settingsArea
                    CustomCheckBox {
                        id: checkBoxShadows
                        text: qsTr("Enable Shadows")
                        checked: true
                    }
                    Item { width: 1; height: 20 }
                    CustomCheckBox {
                        id: checkBoxAnimate
                        text: qsTr("Rotate Teapot")
                        checked: true
                    }
                    Item { width: 1; height: 20 }
                    CustomCheckBox {
                        id: checkBoxCustomMaterial
                        text: qsTr("Custom Material")
                        checked: false
                    }
                    Item { width: 1; height: 40 }
                    CustomCheckBox {
                        id: checkBox1
                        text: qsTr("Directional Light")
                        checked: true
                    }
                    CustomSlider {
                        id: slider1
                        sliderValue: 0.5
                        fromValue: 0
                        toValue: 1
                    }
                    Item { width: 1; height: 40 }
                    CustomCheckBox {
                        id: checkBox2
                        text: qsTr("Point Light")
                        checked: false
                    }
                    CustomSlider {
                        id: slider2
                        sliderValue: 6
                        fromValue: 0
                        toValue: 10
                    }
                    Item { width: 1; height: 40 }
                    CustomCheckBox {
                        id: checkBox4
                        text: qsTr("Spot Light")
                        checked: false
                    }
                    CustomSlider {
                        id: slider4
                        sliderValue: 10
                        fromValue: 0
                        toValue: 30
                    }
                }
            }
        }
    }
}
