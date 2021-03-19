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
        id: view3D
        anchors.fill: parent
        camera: camera

        environment: SceneEnvironment {
            clearColor: "#202020"
            backgroundMode: SceneEnvironment.Color
            antialiasingMode: settings.antialiasingMode
            antialiasingQuality: settings.antialiasingQuality
        }

        PerspectiveCamera {
            id: camera
            position: Qt.vector3d(0, 0, 600)
        }

        PointLight {
            position: Qt.vector3d(400, 600, 400)
            brightness: 80
            ambientColor: Qt.rgba(0.3, 0.3, 0.3, 1.0)
        }

        Component {
            id: particleComponent
            Model {
                source: "#Cube"
                scale: Qt.vector3d(0.5, 0.5, 0.5)
                materials: DefaultMaterial {
                }
            }
        }

        Component {
            id: particleComponentStar
            Model {
                source: "#Rectangle"
                scale: Qt.vector3d(0.1, 0.1, 0.0)
                materials: DefaultMaterial {
                    diffuseMap: Texture {
                        source: "images/star2.png"
                    }
                }
            }
        }

        ParticleSystem3D {
            id: psystem

            // Particles
            ModelParticle3D {
                id: particleCube
                delegate: particleComponent
                maxAmount: 4
                color: "#ffffff"
                colorVariation: Qt.vector4d(0.4, 0.4, 0.4, 0.0)
                unifiedColorVariation: true
            }
            ModelParticle3D {
                id: particleSpark
                delegate: particleComponentStar
                maxAmount: 600
                fadeInDuration: 200
                fadeOutDuration: 500
                color: "#ffffff"
                colorVariation: Qt.vector4d(0.4, 0.4, 0.4, 0.8)
                unifiedColorVariation: true

            }
            ModelParticle3D {
                id: particleSpark2
                delegate: particleComponentStar
                maxAmount: 1000
                fadeInDuration: 200
                fadeOutDuration: 500
                color: "#ff6060"
                colorVariation: Qt.vector4d(0.5, 0.2, 0.2, 0.5)
                unifiedColorVariation: true
            }
            ModelParticle3D {
                id: particleStar
                delegate: particleComponentStar
                maxAmount: 1000
                color: "#ffee60"
                colorVariation: Qt.vector4d(0.6, 0.6, 0.2, 0.5)
                unifiedColorVariation: true
            }

            ParticleEmitter3D {
                particle: particleCube
                position: Qt.vector3d(-550, -300, 0)
                particleScaleVariation: 0.4
                particleRotationVariation: Qt.vector3d(180, 180, 180)
                particleRotationVelocityVariation: Qt.vector3d(200, 200, 200);
                velocity: VectorDirection3D {
                    direction: Qt.vector3d(250, 400, 0)
                    directionVariation: Qt.vector3d(50, 50, 50)
                }
                emitRate: 1
                lifeSpan: 4000
            }

            TrailEmitter3D {
                id: trailEmitter
                particle: particleSpark
                follow: particleCube
                particleScale: 1.5
                particleScaleVariation: 0.5
                particleRotationVariation: Qt.vector3d(20, 20, 180)
                particleRotationVelocityVariation: Qt.vector3d(100, 100, 100);
                velocity: VectorDirection3D {
                    directionVariation: Qt.vector3d(20, 20, 20)
                }
                emitRate: sliderEmitRate.sliderValue
                lifeSpan: 1000
            }

            TrailEmitter3D {
                id: trailEmitter2
                particle: particleSpark2
                follow: particleCube
                particleScale: 2.5
                particleScaleVariation: 0.5
                particleRotationVariation: Qt.vector3d(20, 20, 180)
                particleRotationVelocityVariation: Qt.vector3d(100, 100, 100);
                velocity: VectorDirection3D {
                    directionVariation: Qt.vector3d(100, 100, 100)
                }
                lifeSpan: 1000
            }

            ParticleEmitter3D {
                id: starEmitter
                particle: particleStar
                particleScale: 5.0
                particleScaleVariation: 3.0
                particleRotationVariation: Qt.vector3d(0, 0, 180)
                particleRotationVelocityVariation: Qt.vector3d(100, 100, 100);
                velocity: VectorDirection3D {
                    direction: Qt.vector3d(0, 500, 0)
                    directionVariation: Qt.vector3d(80, 80, 80)
                }
                lifeSpan: 1000
            }

            Gravity3D {
                direction: Qt.vector3d(0, 1, 0)
                magnitude: -200
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            trailEmitter2.burst(50);
            var pos = view3D.mapTo3DScene(Qt.vector3d(mouseX, mouseY, camera.z));
            starEmitter.burst(sliderBurstAmount.sliderValue,
                              sliderBurstDuration.sliderValue,
                              pos);

        }
    }

    Text {
        anchors.centerIn: parent
        font.pointSize: settings.fontSizeLarge
        color: "#ffffff"
        text: qsTr("Click to burst!")
    }

    SettingsView {
        CustomLabel {
            text: "Burst amount"
        }
        CustomSlider {
            id: sliderBurstAmount
            sliderValue: 100
            fromValue: 10
            toValue: 200
        }
        CustomLabel {
            text: "Burst duration"
        }
        CustomSlider {
            id: sliderBurstDuration
            sliderValue: 100
            fromValue: 0
            toValue: 1000
        }
        CustomLabel {
            text: "Trail emitRate"
        }
        CustomSlider {
            id: sliderEmitRate
            sliderValue: 100
            fromValue: 0
            toValue: 150
        }
    }

    LoggingView {
        anchors.bottom: parent.bottom
        particleSystems: [psystem]
    }
}
