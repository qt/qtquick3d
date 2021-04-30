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

        ParticleSystem3D {
            id: psystem

            // Particles
            SpriteParticle3D {
                id: animated
                sprite: Texture {
                    source: "images/bear_black.png"
                }
                spriteSequence: SpriteSequence3D {
                    frameCount: 13
                    interpolate: false
                    duration: 2000
                    durationVariation: 1500
                }
                maxAmount: 4
                billboard: true
                blendMode: SpriteParticle3D.Screen
            }

            SpriteParticle3D {
                id: blended
                sprite: Texture {
                    source: "images/explosion_01_strip13.png"
                }
                spriteSequence: SpriteSequence3D {
                    frameCount: 13
                    interpolate: true
                }
                maxAmount: 26
                billboard: true
                blendMode: SpriteParticle3D.Screen
            }

            SpriteParticle3D {
                id: numberParticle
                sprite: Texture {
                    source: "images/sprite_09.png"
                }
                spriteSequence: SpriteSequence3D {
                    id: numberParticleSequence
                    frameCount: 10
                    frameIndex: sliderFrameIndex.sliderValue
                    animationDirection: sliderAnimationDirection.sliderValue
                    interpolate: checkBoxInterpolate.checked
                    randomStart: checkBoxRandom.checked
                    duration: sliderDuration.sliderValue
                    durationVariation: sliderDurationVariation.sliderValue
                }
                maxAmount: 10
                billboard: true
                blendMode: SpriteParticle3D.Screen
            }

            // Emitters, one per particle
            ParticleEmitter3D {
                id: emitter1
                particle: animated
                particleScale: 10
                position: Qt.vector3d(-300, 0, 0)
                emitRate: 1
                lifeSpan: 4000

                shape: ParticleShape3D {
                    fill: true
                }
                Node {
                    y: -140
                    x: -100
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "Animated + durationVariation"
                        font.pointSize: settings.fontSizeLarge
                        color: "#ffffff"
                    }
                }
            }

            ParticleEmitter3D {
                id: emitter2
                particle: blended
                position: Qt.vector3d(300, -100, 0)
                particleScale: 20
                velocity: VectorDirection3D {
                    direction: Qt.vector3d(0, 100, 0)
                    directionVariation: Qt.vector3d(50, 50, 50)
                }

                emitRate: 12
                lifeSpan: 2000
                lifeSpanVariation: 200
                Node {
                    y: -40
                    x: -100
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "Animated + interpolated"
                        font.pointSize: settings.fontSizeLarge
                        color: "#ffffff"
                    }
                }
            }

            ParticleEmitter3D {
                id: emitter3
                particle: numberParticle
                particleScale: 8
                position: Qt.vector3d(0, 350, 0)
                emitRate: 2
                lifeSpan: 5000
                velocity: VectorDirection3D {
                    direction: Qt.vector3d(0, -100, 0)
                }
                Node {
                    y: 40
                    x: -50
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "Controlled"
                        font.pointSize: settings.fontSizeLarge
                        color: "#ffffff"
                    }
                }
            }
        }
    }

    SettingsView {
        CustomLabel {
            property var enumStrings: ["Normal", "Reverse", "Alternate", "AlternateReverse", "SingleFrame"]
            text: "Animation Direction: " + enumStrings[numberParticleSequence.animationDirection]
        }
        CustomSlider {
            id: sliderAnimationDirection
            sliderValue: 0
            fromValue: 0
            toValue: 4
            sliderStepSize: 1
        }
        CustomLabel {
            text: "Duration"
        }
        CustomSlider {
            id: sliderDuration
            sliderValue: -1
            fromValue: -1
            toValue: 8000
        }
        CustomLabel {
            text: "Duration Variation"
        }
        CustomSlider {
            id: sliderDurationVariation
            sliderValue: 0
            fromValue: 0
            toValue: 2000
        }
        CustomCheckBox {
            id: checkBoxInterpolate
            text: "Interpolate"
            checked: false
        }
        CustomCheckBox {
            id: checkBoxRandom
            text: "Random Start"
            checked: false
        }
        CustomLabel {
            text: "Frame Index"
            opacity: checkBoxRandom.checked ? 0.4 : 1.0
        }
        CustomSlider {
            id: sliderFrameIndex
            sliderValue: 0
            fromValue: 0
            toValue: 9
            sliderEnabled: !checkBoxRandom.checked
            sliderStepSize: 1
        }
    }

    LoggingView {
        anchors.bottom: parent.bottom
        particleSystems: [psystem]
    }
}
