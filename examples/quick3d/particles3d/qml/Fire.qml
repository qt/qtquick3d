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

    readonly property real fireStrength: sliderFireStrength.sliderValue
    readonly property real smokeStrength: sliderSmokeStrength.sliderValue
    readonly property real sparklesStrength: sliderSparklesStrength.sliderValue

    anchors.fill: parent

    View3D {
        anchors.fill: parent

        environment: SceneEnvironment {
            clearColor: "#000000"
            backgroundMode: SceneEnvironment.Color
            antialiasingMode: settings.antialiasingMode
            antialiasingQuality: settings.antialiasingQuality
        }

        PerspectiveCamera {
            id: camera
            position: Qt.vector3d(0, 100, 300)
        }

        // Light following the fire
        PointLight {
            property real animatedBrightness: 0.1
            position: fireEmitter.position
            brightness: 0.04 * smokeStrength + 0.4 * fireStrength + animatedBrightness * sparklesStrength;
            // Add some liveness to the light
            SequentialAnimation on animatedBrightness {
                loops: Animation.Infinite
                NumberAnimation {
                    to: 0.12
                    duration: 1000
                    easing.type: Easing.OutElastic
                }
                NumberAnimation {
                    to: 0.05
                    duration: 1500
                    easing.type: Easing.InOutQuad
                }
                NumberAnimation {
                    to: 0.1
                    duration: 2500
                    easing.type: Easing.InElastic
                }
            }
        }

        // Background walls & floor
        Node {
            id: background
            visible: checkBoxBackground.checked
            Model {
                source: "#Rectangle"
                position: Qt.vector3d(200, 0, -300)
                scale: Qt.vector3d(10.0, 10.0, 0.0)
                materials: DefaultMaterial {
                    diffuseColor: "#204020"
                }
            }
            Model {
                source: "#Rectangle"
                position: Qt.vector3d(200, -100, -300)
                eulerRotation.x: -90
                scale: Qt.vector3d(10.0, 10.0, 0.0)
                materials: DefaultMaterial {
                    diffuseColor: "#204020"
                }
            }
            Model {
                source: "#Rectangle"
                position: Qt.vector3d(-300, 0, -200)
                eulerRotation.y: 90
                scale: Qt.vector3d(10.0, 10.0, 0.0)
                materials: DefaultMaterial {
                    diffuseColor: "#204020"
                }
            }
        }

        ParticleSystem3D {
            id: psystem

            SpriteParticle3D {
                id: particleFire
                sprite: Texture {
                    source: "images/sphere.png"
                }
                colorTable: Texture {
                    source: "images/colorTable.png"
                }
                maxAmount: 300
                color: "#ffffff"
                colorVariation: Qt.vector4d(0.0, 0.6, 0.8, 0.0)
                billboard: true
                blendMode: SpriteParticle3D.Screen
                fadeInDuration: 100
            }

            ParticleEmitter3D {
                id: fireEmitter
                particle: particleFire
                particleScale: 4
                particleEndScale: 12
                particleScaleVariation: 3
                particleEndScaleVariation: 5
                velocity: VectorDirection3D {
                    direction: Qt.vector3d(0, 20 + fireStrength, 0)
                    directionVariation: Qt.vector3d(10 + fireStrength * 0.2, 10, 0)
                }
                emitRate: fireStrength * 2
                lifeSpan: 1000
                lifeSpanVariation: 500
                // Animate the fire position
                SequentialAnimation on x {
                    paused: !checkBoxAnimate.checked
                    loops: Animation.Infinite
                    NumberAnimation {
                        to: 100
                        duration: 2500
                        easing.type: Easing.InOutQuad
                    }
                    NumberAnimation {
                        to: -100
                        duration: 2500
                        easing.type: Easing.InOutQuad
                    }
                }
            }

            SpriteParticle3D {
                id: particleSparkle
                sprite: Texture {
                    source: "images/sphere.png"
                }
                colorTable: Texture {
                    source: "images/color_table3.png"
                }
                maxAmount: 400
                colorVariation: Qt.vector4d(0.0, 0.0, 0.0, 0.4)
                blendMode: SpriteParticle3D.Screen
            }

            ParticleEmitter3D {
                id: sparklesEmitter
                particle: particleSparkle
                position: fireEmitter.position
                particleScale: 1.0
                particleEndScale: 0
                particleScaleVariation: 0.5
                particleRotationVariation: Qt.vector3d(180, 180, 0)
                particleRotationVelocityVariation: Qt.vector3d(400, 400, 0)
                velocity: VectorDirection3D {
                    direction: Qt.vector3d(0, 50 + sparklesStrength, 0)
                    directionVariation: Qt.vector3d(50, 10 + sparklesStrength * 0.5, 0)
                }
                emitRate: sparklesStrength * 4
                lifeSpan: 500
                lifeSpanVariation: 500
            }

            SpriteParticle3D {
                id: smokeParticle
                sprite: Texture {
                    source: "images/smoke_sprite.png"
                }
                maxAmount: 200
                spriteSequence: SpriteSequence3D {
                    frameCount: 15
                    interpolate: true
                }
                billboard: true
                color: "#40ffffff"
                colorVariation: Qt.vector4d(0.5, 0.5, 0.5, 0.2)
                unifiedColorVariation: true
                fadeOutEffect: Particle3D.FadeOpacity
                fadeOutDuration: 1500
            }

            ParticleEmitter3D {
                id: smokeEmitter
                particle: smokeParticle
                // Smoke always behind the fire & sparkles
                position: Qt.vector3d(fireEmitter.position.x, fireEmitter.position.y, fireEmitter.position.z - 2)
                particleScale: 6
                particleScaleVariation: 4
                particleEndScale: 35
                particleEndScaleVariation: 15
                particleRotationVariation: Qt.vector3d(0, 0, 180)
                particleRotationVelocityVariation: Qt.vector3d(0, 0, 40)
                emitRate: smokeStrength * 0.5
                lifeSpan: 3000
                lifeSpanVariation: 1000
                velocity: VectorDirection3D {
                    direction: Qt.vector3d(0, 50 + smokeStrength * 0.1, 0)
                    directionVariation: Qt.vector3d(20, 20, 0)
                }
            }

            Gravity3D {
                // Add gravity to sparkles
                particles: particleSparkle
                magnitude: 200
            }
        }
    }

    SettingsView {
        CustomLabel {
            text: "Fire Strength"
        }
        CustomSlider {
            id: sliderFireStrength
            sliderValue: 50
            fromValue: 0
            toValue: 100
        }
        CustomLabel {
            text: "Smoke Strength"
        }
        CustomSlider {
            id: sliderSmokeStrength
            sliderValue: 50
            fromValue: 0
            toValue: 100
        }
        CustomLabel {
            text: "Sparkles Strength"
        }
        CustomSlider {
            id: sliderSparklesStrength
            sliderValue: 50
            fromValue: 0
            toValue: 100
        }
        CustomCheckBox {
            id: checkBoxBackground
            text: "Show Background"
            checked: true
        }
        CustomCheckBox {
            id: checkBoxAnimate
            text: "Animate Position"
            checked: true
        }
    }

    LoggingView {
        anchors.bottom: parent.bottom
        particleSystems: [psystem]
    }
}
