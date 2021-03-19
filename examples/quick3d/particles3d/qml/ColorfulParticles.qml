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

Item {
    id: mainWindow

    anchors.fill: parent

    View3D {
        anchors.fill: parent

        environment: SceneEnvironment {
            clearColor: "#202020"
            backgroundMode: SceneEnvironment.Color
            antialiasingMode: settings.antialiasingMode
            antialiasingQuality: settings.antialiasingQuality
        }

        PerspectiveCamera {
            id: camera
            position: Qt.vector3d(0, 100, 600)
        }

        PointLight {
            position: Qt.vector3d(0, 400, 100)
            brightness: 10
            ambientColor: Qt.rgba(0.3, 0.3, 0.3, 1.0)
        }

        // Models shared between particles
        Component {
            id: particleComponent
            Model {
                source: "#Cube"
                scale: Qt.vector3d(0.1, 0.1, 0.1)
                materials: DefaultMaterial {
                }
            }
        }

        Component {
            id: particleComponent2
            Model {
                source: "#Cylinder"
                scale: Qt.vector3d(0.1, 0.2, 0.1)
                materials: PrincipledMaterial {
                    metalness: 0.5
                    roughness: 0
                    specularAmount: 1.0
                }
            }
        }

        ParticleSystem3D {
            id: psystem

            SequentialAnimation on eulerRotation.y {
                running: true
                loops: Animation.Infinite
                NumberAnimation {
                    to: 180
                    duration: 5000
                    easing.type: Easing.InOutCirc
                }
                NumberAnimation {
                    to: 0
                    duration: 5000
                    easing.type: Easing.InOutCirc
                }
            }

            property vector4d cVar: Qt.vector4d(sliderRedVariation.sliderValue,
                                                sliderGreenVariation.sliderValue,
                                                sliderBlueVariation.sliderValue,
                                                sliderAlphaVariation.sliderValue)
            // Particles
            ModelParticle3D {
                id: particleRed
                delegate: particleComponent
                maxAmount: 250
                color: "#ff0000"
                colorVariation: psystem.cVar
                unifiedColorVariation: checkBoxUnified.checked
            }
            ModelParticle3D {
                id: particleGreen
                delegate: particleComponent
                maxAmount: 250
                color: "#00ff00"
                colorVariation: psystem.cVar
                unifiedColorVariation: checkBoxUnified.checked
            }
            ModelParticle3D {
                id: particleBlue
                delegate: particleComponent
                maxAmount: 250
                color: "#0000ff"
                colorVariation: psystem.cVar
                unifiedColorVariation: checkBoxUnified.checked
            }
            ModelParticle3D {
                id: particleWhite
                delegate: particleComponent2
                maxAmount: 250
                color: "#ffffff"
                colorVariation: psystem.cVar
                unifiedColorVariation: checkBoxUnified.checked
            }

            // Emitters, one per particle
            ParticleEmitter3D {
                particle: particleRed
                position: Qt.vector3d(400, 50, 0)
                particleScaleVariation: 0.8
                particleRotationVariation: Qt.vector3d(180, 180, 180)
                particleRotationVelocityVariation: Qt.vector3d(200, 200, 200);
                velocity: VectorDirection3D {
                    direction: Qt.vector3d(-350, 150, 0)
                    directionVariation: Qt.vector3d(30, 30, 30)
                }
                emitRate: 100
                lifeSpan: 2500
                Node {
                    x: 20
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "RED"
                        font.pointSize: settings.fontSizeLarge
                        color: "#ffffff"
                    }
                }
            }
            ParticleEmitter3D {
                particle: particleGreen
                position: Qt.vector3d(400, 0, 0)
                particleScaleVariation: 0.8
                particleRotationVariation: Qt.vector3d(180, 180, 180)
                particleRotationVelocityVariation: Qt.vector3d(200, 200, 200);
                velocity: VectorDirection3D {
                    direction: Qt.vector3d(-350, 150, 0)
                    directionVariation: Qt.vector3d(30, 30, 30)
                }
                emitRate: 100
                lifeSpan: 2500
                Node {
                    x: 20
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "GREEN"
                        font.pointSize: settings.fontSizeLarge
                        color: "#ffffff"
                    }
                }
            }
            ParticleEmitter3D {
                particle: particleBlue
                position: Qt.vector3d(400, -50, 0)
                particleScaleVariation: 0.8
                particleRotationVariation: Qt.vector3d(180, 180, 180)
                particleRotationVelocityVariation: Qt.vector3d(200, 200, 200);
                velocity: VectorDirection3D {
                    direction: Qt.vector3d(-350, 150, 0)
                    directionVariation: Qt.vector3d(30, 30, 30)
                }
                emitRate: 100
                lifeSpan: 2500
                Node {
                    x: 20
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "BLUE"
                        font.pointSize: settings.fontSizeLarge
                        color: "#ffffff"
                    }
                }
            }
            ParticleEmitter3D {
                particle: particleWhite
                position: Qt.vector3d(400, -100, 0)
                particleScaleVariation: 0.8
                particleRotationVariation: Qt.vector3d(180, 180, 180)
                particleRotationVelocityVariation: Qt.vector3d(200, 200, 200);
                velocity: VectorDirection3D {
                    direction: Qt.vector3d(-350, 150, 0)
                    directionVariation: Qt.vector3d(30, 30, 30)
                }
                emitRate: 100
                lifeSpan: 2500
                Node {
                    x: 20
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "WHITE"
                        font.pointSize: settings.fontSizeLarge
                        color: "#ffffff"
                    }
                }
            }

            Gravity3D {
                // Enable to affect only some of the particles
                //particles: [particleRed, particleBlue, particleGreen]
                direction: Qt.vector3d(0, 1, 0)
                magnitude: -100
            }
        }
    }

    SettingsView {
        CustomCheckBox {
            id: checkBoxUnified
            text: "Unified Variation"
            checked: false
        }
        CustomLabel {
            text: "Red Variation"
        }
        CustomSlider {
            id: sliderRedVariation
            sliderValue: 0
            fromValue: 0.0
            toValue: 1.0
        }
        CustomLabel {
            text: "Green Variation"
        }
        CustomSlider {
            id: sliderGreenVariation
            sliderValue: 0
            fromValue: 0.0
            toValue: 1.0
        }
        CustomLabel {
            text: "Blue Variation"
        }
        CustomSlider {
            id: sliderBlueVariation
            sliderValue: 0
            fromValue: 0.0
            toValue: 1.0
        }
        CustomLabel {
            text: "Alpha Variation"
        }
        CustomSlider {
            id: sliderAlphaVariation
            sliderValue: 0
            fromValue: 0.0
            toValue: 1.0
        }
    }

    LoggingView {
        anchors.bottom: parent.bottom
        particleSystems: [psystem]
    }
}
