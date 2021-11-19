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
import QtQuick3D
import QtQuick3D.Particles3D
import QtQuick.Controls

Item {
    anchors.fill: parent

    Timer {
        id: lightsUpdateTimer
        interval: 0
        triggeredOnStart: true
        onTriggered: updateLightsArray();
    }

    // Update the lights array of the particles
    function updateLightsArray() {
        var newLights = [];
        if (checkBoxDirectionalLightUse.checked)
            newLights.push(lightDirectional);
        if (checkBoxPointLightUse.checked)
            newLights.push(lightPoint);
        if (checkBoxSpotLightUse.checked)
            newLights.push(lightSpot);
        spriteParticle.lightsArray = newLights;
    }

    View3D {
        anchors.fill: parent

        environment: SceneEnvironment {
            clearColor: "#000000"
            backgroundMode: SceneEnvironment.Color
            antialiasingMode: SceneEnvironment.MSAA
            antialiasingQuality: SceneEnvironment.High
        }

        PerspectiveCamera {
            position: Qt.vector3d(0, 100, 700)
            eulerRotation.x: -10
        }

        DirectionalLight {
            id: lightDirectional
            color: Qt.rgba(1.0, 0.1, 0.1, 1.0)
            position: Qt.vector3d(0, 200, 0)
            rotation: Quaternion.fromEulerAngles(-70, 0, 0)
            visible: checkBoxDirectionalLight.checked
            brightness: sliderDirectionalLight.sliderValue
        }

        PointLight {
            id: lightPoint
            color: Qt.rgba(0.1, 1.0, 0.1, 1.0)
            position: Qt.vector3d(0, 300, 0)
            visible: checkBoxPointLight.checked
            brightness: sliderPointLight.sliderValue
            constantFade: sliderPointLightConstantFade.sliderValue
            linearFade: sliderPointLightLinearFade.sliderValue
            quadraticFade: sliderPointLightQuadraticFade.sliderValue
            SequentialAnimation on x {
                loops: Animation.Infinite
                NumberAnimation {
                    to: 400
                    duration: 2000
                    easing.type: Easing.InOutQuad
                }
                NumberAnimation {
                    to: -400
                    duration: 2000
                    easing.type: Easing.InOutQuad
                }
            }
        }

        SpotLight {
            id: lightSpot
            color: Qt.rgba(0.0, 0.0, 1.0, 1.0)
            position: Qt.vector3d(0, 250, 0)
            eulerRotation.x: -45
            visible: checkBoxSpotLight.checked
            brightness: sliderSpotLight.sliderValue
            coneAngle: 50
            innerConeAngle: 20
            constantFade: sliderSpotLightConstantFade.sliderValue
            linearFade: sliderSpotLightLinearFade.sliderValue
            quadraticFade: sliderSpotLightQuadraticFade.sliderValue
            PropertyAnimation on eulerRotation.y {
                loops: Animation.Infinite
                from: 0
                to: 360
                duration: 10000
            }
        }

        Model {
            source: "#Rectangle"
            y: -200
            scale: Qt.vector3d(15, 15, 15)
            eulerRotation.x: -90
            materials: [
                DefaultMaterial {
                    diffuseColor: Qt.rgba(0.4, 0.4, 0.4, 1.0)
                }
            ]
        }
        Model {
            source: "#Rectangle"
            z: -400
            scale: Qt.vector3d(15, 15, 15)
            materials: [
                DefaultMaterial {
                    diffuseColor: Qt.rgba(0.4, 0.4, 0.4, 1.0)
                }
            ]
        }

        ParticleSystem3D {
            id: psystem

            startTime: 5000

            SpriteParticle3D {
                id: spriteParticle
                property var lightsArray: []
                sprite: Texture {
                    source: "images/sphere.png"
                }
                maxAmount: 10000
                color: "#ffffff"
                colorVariation: Qt.vector4d(0.0, 0.0, 0.0, 0.5);
                fadeInDuration: 1000
                fadeOutDuration: 1000
                billboard: true
                // Particles to use the enabled lights
                lights: lightsArray
                // Disable this to see the unlit particles
                blendMode: SpriteParticle3D.Screen
            }

            ParticleEmitter3D {
                id: emitter
                particle: spriteParticle
                position: Qt.vector3d(0, 350, 0)
                depthBias: -100
                scale: Qt.vector3d(8.0, 0.0, 8.0)
                shape: ParticleShape3D {
                    type: ParticleShape3D.Sphere
                }
                particleScale: 2.0
                particleScaleVariation: 1.0;
                velocity: VectorDirection3D {
                    direction: Qt.vector3d(0, -100, 0)
                    directionVariation: Qt.vector3d(20, 50, 20)
                }
                emitRate: 2000
                lifeSpan: 5000
            }

            PointRotator3D {
                pivotPoint: Qt.vector3d(0, 0, 0)
                direction: Qt.vector3d(0, 1, 0)
                magnitude: 20
            }
        }
    }

    SettingsView {
        CustomCheckBox {
            id: checkBoxDirectionalLight
            text: qsTr("Directional Light")
            checked: true
        }
        CustomSlider {
            id: sliderDirectionalLight
            sliderValue: 0.4
            fromValue: 0.0
            toValue: 1
        }
        CustomCheckBox {
            id: checkBoxDirectionalLightUse
            text: qsTr("Use also for particles")
            checked: true
            onCheckedChanged: lightsUpdateTimer.restart();
        }
        Item { width: 1; height: 40 }
        CustomCheckBox {
            id: checkBoxPointLight
            text: qsTr("Point Light")
            checked: true
        }
        CustomSlider {
            id: sliderPointLight
            sliderValue: 6
            fromValue: 0.0
            toValue: 10
        }
        CustomCheckBox {
            id: checkBoxPointLightUse
            text: qsTr("Use also for particles")
            checked: true
            onCheckedChanged: lightsUpdateTimer.restart();
        }
        CustomLabel {
            text: qsTr("constantFade")
        }
        CustomSlider {
            id: sliderPointLightConstantFade
            sliderValue: 1.0
            fromValue: 0.1
            toValue: 20.0
        }
        CustomLabel {
            text: qsTr("linearFade")
        }
        CustomSlider {
            id: sliderPointLightLinearFade
            sliderValue: 0.0
            fromValue: 0.0
            toValue: 20.0
        }
        CustomLabel {
            text: qsTr("quadraticFade")
        }
        CustomSlider {
            id: sliderPointLightQuadraticFade
            sliderValue: 1.0
            fromValue: 0.1
            toValue: 20.0
        }

        Item { width: 1; height: 40 }
        CustomCheckBox {
            id: checkBoxSpotLight
            text: qsTr("Spot Light")
            checked: true
        }
        CustomSlider {
            id: sliderSpotLight
            sliderValue: 40
            fromValue: 0.0
            toValue: 100
        }
        CustomCheckBox {
            id: checkBoxSpotLightUse
            text: qsTr("Use also for particles")
            checked: true
            onCheckedChanged: lightsUpdateTimer.restart();
        }
        CustomLabel {
            text: qsTr("constantFade")
        }
        CustomSlider {
            id: sliderSpotLightConstantFade
            sliderValue: 1.0
            fromValue: 0.1
            toValue: 20.0
        }
        CustomLabel {
            text: qsTr("linearFade")
        }
        CustomSlider {
            id: sliderSpotLightLinearFade
            sliderValue: 0.0
            fromValue: 0.0
            toValue: 20.0
        }
        CustomLabel {
            text: qsTr("quadraticFade")
        }
        CustomSlider {
            id: sliderSpotLightQuadraticFade
            sliderValue: 1.0
            fromValue: 0.1
            toValue: 20.0
        }
    }

    LoggingView {
        anchors.bottom: parent.bottom
        particleSystems: [psystem]
    }

}
