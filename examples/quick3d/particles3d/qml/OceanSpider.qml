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

    readonly property real tentacleEmitRate: 120
    readonly property real cameraDistance: sliderCameraDistance.sliderValue
    property real cameraDistanceSmoothed: cameraDistance
    property real tentacleWideness: 40

    anchors.fill: parent

    // Animate tentacle movement
    SequentialAnimation on tentacleWideness {
        loops: Animation.Infinite
        NumberAnimation {
            to: 80
            duration: 6000
            easing.type: Easing.InOutQuad
        }
        NumberAnimation {
            to: 30
            duration: 6000
            easing.type: Easing.InOutQuad
        }
    }

    Behavior on cameraDistanceSmoothed {
        SmoothedAnimation {
            velocity: 100
            duration: 1000
        }
    }

    // Background ocean gradient
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop {
                color: "#005060"
                position: 0.0
            }
            GradientStop {
                color: "#000000"
                position: 1.0
            }
        }
    }

    View3D {
        anchors.fill: parent

        environment: SceneEnvironment {
            backgroundMode: SceneEnvironment.Transparent
            antialiasingMode: SceneEnvironment.MSAA
            antialiasingQuality: SceneEnvironment.High
        }

        // Camera rotating the spider
        Node {
            PerspectiveCamera {
                id: camera
                position: Qt.vector3d(0, 0, cameraDistanceSmoothed)
            }

            NumberAnimation on eulerRotation.y {
                loops: Animation.Infinite
                from: 0
                to: 360
                duration: 20000
            }

            SequentialAnimation on eulerRotation.x {
                loops: Animation.Infinite
                NumberAnimation {
                    to: -50
                    duration: 6000
                    easing.type: Easing.InOutQuad
                }
                NumberAnimation {
                    to: 0
                    duration: 6000
                    easing.type: Easing.InOutQuad
                }
            }
        }

        PointLight {
            position: Qt.vector3d(0, 400, 0)
            brightness: 200
        }

        // Body of the spider
        Model {
            id: spiderBody
            source: "#Sphere"
            scale: Qt.vector3d(0.6, 0.8, 1.2)

            PrincipledMaterial {
                id: spiderMaterial
                baseColor: tentacleParticle.color
                metalness: 1.0
                roughness: 0.6
                normalMap: Texture {
                    source: "images/leather_n.png"
                }
                normalStrength: 0.8
            }
            materials: spiderMaterial
            Model {
                source: "#Cone"
                scale: Qt.vector3d(0.4, 0.5, 0.5)
                position: Qt.vector3d(20, 10, -20)
                eulerRotation: Qt.vector3d(-40, -80, 0)
                materials: spiderMaterial
            }
            Model {
                source: "#Cone"
                scale: Qt.vector3d(0.4, 0.5, 0.5)
                position: Qt.vector3d(-20, 10, -20)
                eulerRotation: Qt.vector3d(-40, 80, 0)
                materials: spiderMaterial
            }
            SequentialAnimation on y {
                loops: Animation.Infinite
                NumberAnimation {
                    to: 100
                    duration: 2500
                    easing.type: Easing.InOutQuad
                }
                NumberAnimation {
                    to: 0
                    duration: 2500
                    easing.type: Easing.InOutQuad
                }
            }
        }

        // System for dust/smoke particles
        ParticleSystem3D {
            id: psystemDust
            SpriteParticle3D {
                id: smokeParticle
                sprite: Texture {
                    source: "images/smoke_sprite.png"
                }
                maxAmount: 200
                frameCount: 15
                interpolate: true
                billboard: true
                color: tentacleParticle.color
                fadeInEffect: SpriteParticle3D.FadeNone
                fadeOutEffect: Particle3D.FadeOpacity
                fadeOutDuration: 1500
                blendMode: SpriteParticle3D.Screen
            }

            ParticleEmitter3D {
                id: smokeEmitter
                particle: smokeParticle
                position: spiderBody.position
                particleScale: 10
                particleScaleVariation: 5
                particleEndScale: 80
                particleRotationVariation: Qt.vector3d(0, 0, 180)
                particleRotationVelocityVariation: Qt.vector3d(0, 0, 40)
                emitRate: sliderDustEmitRate.sliderValue
                lifeSpan: 3000
                lifeSpanVariation: 1000
                velocity: VectorDirection3D {
                    direction: Qt.vector3d(0, 0, 200)
                    directionVariation: Qt.vector3d(50, 50, 50)
                }
            }

            SpriteParticle3D {
                id: dustParticle
                sprite: Texture {
                    source: "images/dust.png"
                }
                maxAmount: 100
                color: "#ffffff"
                colorVariation: Qt.vector4d(0.4, 0.4, 0.4, 0.4)
                billboard: true
                fadeInDuration: 200
                fadeOutDuration: 1500
                blendMode: SpriteParticle3D.Screen
            }

            ParticleEmitter3D {
                id: dustEmitter
                particle: dustParticle
                position: spiderBody.position
                emitRate: sliderDustEmitRate.sliderValue * 0.5
                lifeSpan: 3000
                lifeSpanVariation: 1000
                particleScale: 10.0
                particleEndScale: 30.0
                particleScaleVariation: 5.0
                particleRotationVariation: Qt.vector3d(20, 20, 180)
                particleRotationVelocityVariation: Qt.vector3d(5, 5, 50)
                velocity: VectorDirection3D {
                    direction: Qt.vector3d(0, 0, 100)
                    directionVariation: Qt.vector3d(20, 20, 20)
                }
            }
        }

        // System for bubbles particles
        ParticleSystem3D {
            id: psystemBubbles
            position: Qt.vector3d(0, 100, -200)
            SpriteParticle3D {
                id: bubbleParticle
                sprite: Texture {
                    source: "images/sphere.png"
                }
                maxAmount: 300
                color: "#80ffffff"
                colorVariation: Qt.vector4d(0.0, 0.0, 0.0, 0.2)
                fadeInDuration: 1000
                fadeOutDuration: 1000
                blendMode: SpriteParticle3D.Screen
                billboard: true
            }

            ParticleEmitter3D {
                particle: bubbleParticle
                scale: Qt.vector3d(12, 12, 12)
                particleScale: 2.0
                particleScaleVariation: 1.0
                particleEndScale: 0.5
                emitRate: 100
                lifeSpan: 3000
                shape: ParticleShape3D {
                    type: ParticleShape3D.Sphere
                }
                velocity: VectorDirection3D {
                    direction: Qt.vector3d(0, 0, 200)
                    directionVariation: Qt.vector3d(20, 20, 100)
                }
            }
        }

        // System for tentacles particles
        ParticleSystem3D {
            id: psystemTentacles
            y: spiderBody.y - 25

            SpriteParticle3D {
                id: tentacleParticle
                sprite: Texture {
                    source: "images/dot.png"
                }
                maxAmount: 8 * tentacleEmitRate * 1.5
                color: "#000000"
                colorVariation: Qt.vector4d(0.0, 0.0, 0.0, 0.2)
                fadeInEffect: SpriteParticle3D.FadeNone
                fadeOutDuration: 800
                billboard: true
                SequentialAnimation on color {
                    loops: Animation.Infinite
                    ColorAnimation {
                        to: "#202000"
                        duration: 2000
                    }
                    ColorAnimation {
                        to: "#200000"
                        duration: 2000
                    }
                    ColorAnimation {
                        to: "#000020"
                        duration: 2000
                    }
                    ColorAnimation {
                        to: "#002000"
                        duration: 2000
                    }
                }
                blendMode: SpriteParticle3D.Screen
            }

            // Emitters for all 8 tentacles
            component TentacleEmitter: ParticleEmitter3D {
                particle: tentacleParticle
                velocity: VectorDirection3D {
                    direction: Qt.vector3d(0, 300, 0)
                }
                particleScale: 6.0
                particleEndScale: 0.5
                emitRate: tentacleEmitRate
                lifeSpan: 1500
            }
            TentacleEmitter {
                eulerRotation: Qt.vector3d(tentacleWideness, 0.5 * 45, 0)
            }
            TentacleEmitter {
                eulerRotation: Qt.vector3d(tentacleWideness, 1.5 * 45, 0)
            }
            TentacleEmitter {
                eulerRotation: Qt.vector3d(tentacleWideness, 2.5 * 45, 0)
            }
            TentacleEmitter {
                eulerRotation: Qt.vector3d(tentacleWideness, 3.5 * 45, 0)
            }
            TentacleEmitter {
                eulerRotation: Qt.vector3d(tentacleWideness, 4.5 * 45, 0)
            }
            TentacleEmitter {
                eulerRotation: Qt.vector3d(tentacleWideness, 5.5 * 45, 0)
            }
            TentacleEmitter {
                eulerRotation: Qt.vector3d(tentacleWideness, 6.5 * 45, 0)
            }
            TentacleEmitter {
                eulerRotation: Qt.vector3d(tentacleWideness, 7.5 * 45, 0)
            }

            Gravity3D {
                direction: Qt.vector3d(0, 1, 0)
                magnitude: -300
            }

            Wander3D {
                globalAmount: Qt.vector3d(15, 15, 15)
                globalPace: Qt.vector3d(1.0, 1.0, 1.0)
                PropertyAnimation on globalPaceStart {
                    loops: Animation.Infinite
                    duration: 8000
                    from: Qt.vector3d(0, 0, 0)
                    to: Qt.vector3d(5 * Math.PI * 2, 3 * Math.PI * 2, Math.PI * 2)
                }
            }
        }
    }

    SettingsView {
        CustomLabel {
            text: "Camera Distance"
        }
        CustomSlider {
            id: sliderCameraDistance
            sliderValue: 400
            fromValue: 200
            toValue: 800
        }
        CustomLabel {
            text: "Mystical Dust"
        }
        CustomSlider {
            id: sliderDustEmitRate
            sliderValue: 20
            fromValue: 0
            toValue: 50
        }
    }

    LoggingView {
        anchors.bottom: parent.bottom
        particleSystems: [psystemTentacles, psystemBubbles, psystemDust]
    }
}
