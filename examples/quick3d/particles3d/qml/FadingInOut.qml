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

        // Model shared between particles
        Component {
            id: particleComponent
            Model {
                source: "#Cube"
                scale: Qt.vector3d(0.4, 0.4, 0.4)
                materials: DefaultMaterial {
                }
            }
        }

        ParticleSystem3D {
            id: psystem

            // Particles
            ModelParticle3D {
                id: particleNone
                delegate: particleComponent
                maxAmount: 40
                color: "#ffffff"
                fadeInEffect: ModelParticle3D.FadeNone
                fadeOutEffect: ModelParticle3D.FadeNone
                hasTransparency: false
            }
            ModelParticle3D {
                id: particleOpacity
                delegate: particleComponent
                maxAmount: 40
                color: "#ffffff"
                fadeInEffect: ModelParticle3D.FadeOpacity
                fadeOutEffect: ModelParticle3D.FadeOpacity
                fadeInDuration: sliderFadeInDuration.sliderValue
                fadeOutDuration: sliderFadeOutDuration.sliderValue
            }
            ModelParticle3D {
                id: particleScale
                delegate: particleComponent
                maxAmount: 40
                color: "#ffffff"
                fadeInEffect: ModelParticle3D.FadeScale
                fadeOutEffect: ModelParticle3D.FadeScale
                fadeInDuration: sliderFadeInDuration.sliderValue
                fadeOutDuration: sliderFadeOutDuration.sliderValue
            }
            ModelParticle3D {
                id: particleScaleOpacity
                delegate: particleComponent
                maxAmount: 40
                color: "#ffffff"
                fadeInEffect: ModelParticle3D.FadeScale
                fadeOutEffect: ModelParticle3D.FadeOpacity
                fadeInDuration: sliderFadeInDuration.sliderValue
                fadeOutDuration: sliderFadeOutDuration.sliderValue
            }

            // Emitters, one per particle
            ParticleEmitter3D {
                particle: particleNone
                position: Qt.vector3d(300, 200, 0)
                particleScaleVariation: 0.5
                particleRotationVariation: Qt.vector3d(180, 180, 180)
                particleRotationVelocityVariation: Qt.vector3d(200, 200, 200);
                velocity: VectorDirection3D {
                    direction: Qt.vector3d(-200, 0, 0)
                }
                emitRate: 10
                lifeSpan: 4000
                Node {
                    x: 80
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "NONE"
                        font.pointSize: settings.fontSizeLarge
                        color: "#ffffff"
                    }
                }
            }
            ParticleEmitter3D {
                particle: particleOpacity
                position: Qt.vector3d(300, 100, 0)
                particleScaleVariation: 0.5
                particleRotationVariation: Qt.vector3d(180, 180, 180)
                particleRotationVelocityVariation: Qt.vector3d(200, 200, 200);
                velocity: VectorDirection3D {
                    direction: Qt.vector3d(-200, 0, 0)
                }
                emitRate: 10
                lifeSpan: 4000
                Node {
                    x: 80
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "OPACITY"
                        font.pointSize: settings.fontSizeLarge
                        color: "#ffffff"
                    }
                }
            }
            ParticleEmitter3D {
                particle: particleScale
                position: Qt.vector3d(300, 0, 0)
                particleScaleVariation: 0.5
                particleRotationVariation: Qt.vector3d(180, 180, 180)
                particleRotationVelocityVariation: Qt.vector3d(200, 200, 200);
                velocity: VectorDirection3D {
                    direction: Qt.vector3d(-200, 0, 0)
                }
                emitRate: 10
                lifeSpan: 4000
                Node {
                    x: 80
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "SCALE"
                        font.pointSize: settings.fontSizeLarge
                        color: "#ffffff"
                    }
                }
            }
            ParticleEmitter3D {
                particle: particleScaleOpacity
                position: Qt.vector3d(300, -100, 0)
                particleScaleVariation: 0.5
                particleRotationVariation: Qt.vector3d(180, 180, 180)
                particleRotationVelocityVariation: Qt.vector3d(200, 200, 200);
                velocity: VectorDirection3D {
                    direction: Qt.vector3d(-200, 0, 0)
                }
                emitRate: 10
                lifeSpan: 4000
                Node {
                    x: 80
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "SCALE -> OPACITY"
                        font.pointSize: settings.fontSizeLarge
                        color: "#ffffff"
                    }
                }
            }
        }
    }

    SettingsView {
        CustomLabel {
            text: "Fade In Duration (ms)"
        }
        CustomSlider {
            id: sliderFadeInDuration
            sliderValue: 1500
            fromValue: 0
            toValue: 4000
        }
        CustomLabel {
            text: "Fade Out Duration (ms)"
        }
        CustomSlider {
            id: sliderFadeOutDuration
            sliderValue: 1500
            fromValue: 0
            toValue: 4000
        }
    }

    LoggingView {
        anchors.bottom: parent.bottom
        particleSystems: [psystem]
    }
}
