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
            clearColor: "#000000"
            backgroundMode: SceneEnvironment.Color
            antialiasingMode: settings.antialiasingMode
            antialiasingQuality: settings.antialiasingQuality
        }

        PerspectiveCamera {
            id: camera
            property real cameraAnim: 0
            NumberAnimation on cameraAnim {
                running: true
                loops: Animation.Infinite
                from: 0
                to: 2 * Math.PI
                duration: 40000
            }
            position: Qt.vector3d(Math.sin(cameraAnim * 3.0) * 400, 200, 600 + Math.cos(cameraAnim) * 200)
            eulerRotation: Qt.vector3d(-20, Math.sin(cameraAnim * 3.0) * 30, 0)
        }

        PointLight {
            position: Qt.vector3d(0, 400, 0)
            brightness: 10
            ambientColor: Qt.rgba(0.3, 0.3, 0.3, 1.0)
        }

        // Background stars
        ParticleSystem3D {
            id: psystem2
            startTime: 10000
            SpriteParticle3D {
                id: particle3
                sprite: Texture {
                    source: "images/star3.png"
                }
                maxAmount: 1000
                color: "#40ffffff"
                colorVariation: Qt.vector4d(0.1, 0.1, 0.1, 0.2)
                fadeInEffect: SpriteParticle3D.FadeScale
                fadeInDuration: 2000
                fadeOutEffect: SpriteParticle3D.FadeScale
                fadeOutDuration: 2000
                alignMode: SpriteParticle3D.AlignTowardsTarget
                alignTargetPosition: camera.position
            }
            ParticleEmitter3D {
                particle: particle3
                shape: ParticleShape3D {
                    fill: true
                    type: ParticleShape3D.Sphere
                }
                position: Qt.vector3d(0, -500, -400)
                scale: Qt.vector3d(30, 30, 15)
                emitRate: 100
                lifeSpan: 10000
                particleRotationVariation: Qt.vector3d(0, 0, 180)
                particleRotationVelocityVariation: Qt.vector3d(0, 0, 50)
                particleScale: 5.0
                particleScaleVariation: 3.0
                velocity: VectorDirection3D {
                    direction: Qt.vector3d(0, 0, 0)
                    directionVariation: Qt.vector3d(10, 10, 10)
                }
            }
        }

        ParticleSystem3D {
            id: psystem
            running: false
            SequentialAnimation on time {
                loops: Animation.Infinite
                PauseAnimation {
                    duration: 1500
                }
                NumberAnimation {
                    to: 5000
                    duration: 5000
                    easing.type: Easing.InOutQuad
                }
                PauseAnimation {
                    duration: 1500
                }
                NumberAnimation {
                    to: 0
                    duration: 5000
                    easing.type: Easing.InOutQuad
                }
            }

            SpriteParticle3D {
                id: particle1
                sprite: Texture {
                    source: "images/star3.png"
                }
                maxAmount: 4096
                colorTable: Texture {
                    source: "images/color_table5.png"
                }
                color: "#d0ffffff"
                colorVariation: Qt.vector4d(0.0, 0.0, 0.0, 0.4)
                particleScale: 15.0
                billboard: true
                fadeInEffect: SpriteParticle3D.FadeNone
                fadeOutEffect: SpriteParticle3D.FadeNone
            }

            SpriteParticle3D {
                id: particle2
                sprite: Texture {
                    source: "images/dot.png"
                }
                maxAmount: 4096
                colorTable: Texture {
                    source: "images/color_table4.png"
                }
                color: "#60ffffff"
                colorVariation: Qt.vector4d(0.0, 0.0, 0.0, 0.4)
                particleScale: 6.0
                billboard: true
                fadeInEffect: SpriteParticle3D.FadeNone
                fadeOutEffect: SpriteParticle3D.FadeNone
            }

            ParticleEmitter3D {
                particle: particle1
                scale: Qt.vector3d(5.0, 5.0, 5.0)
                shape: ParticleCustomShape3D {
                    source: "data/qt_logo_in_4096.cbor"
                }
                lifeSpan: 5001
                emitBursts: [
                    EmitBurst3D {
                        time: 0
                        amount: 4096
                    }
                ]
                depthBias: -200
                particleRotationVariation: Qt.vector3d(0, 0, 180)
                particleRotationVelocityVariation: Qt.vector3d(80, 80, 80)
                particleScaleVariation: 0.5
                particleEndScale: 4.0
                particleEndScaleVariation: 2.0
                velocity: VectorDirection3D {
                    direction: Qt.vector3d(-150, 100, 0)
                    directionVariation: Qt.vector3d(150, 100, 100)
                }
            }

            ParticleEmitter3D {
                particle: particle2
                scale: Qt.vector3d(5.0, 5.0, 5.0)
                shape: ParticleCustomShape3D {
                    source: "data/qt_logo_out_4096.cbor"
                }
                lifeSpan: 5001
                emitBursts: [
                    EmitBurst3D {
                        time: 0
                        amount: 4096
                    }
                ]
                particleScale: 2.0
                particleEndScale: 1.0
                particleScaleVariation: 1.5
                particleEndScaleVariation: 0.8
                velocity: VectorDirection3D {
                    direction: Qt.vector3d(0, 200, 0)
                    directionVariation: Qt.vector3d(50, 50, 50)
                }
            }

            Attractor3D {
                particles: [particle1]
                position: Qt.vector3d(-200, 0, 0)
                scale: Qt.vector3d(4.0, 4.0, 4.0)
                shape: ParticleCustomShape3D {
                    source: "data/heart_4096.cbor"
                    randomizeData: true
                }
                duration: 4000
                durationVariation: 1000
            }
            Attractor3D {
                particles: [particle2]
                position: Qt.vector3d(200, 0, 0)
                scale: Qt.vector3d(6.0, 6.0, 6.0)
                shape: ParticleCustomShape3D {
                    source: "data/qt_logo_in_4096.cbor"
                }
                duration: 4000
                durationVariation: 1000
            }
        }
    }

    LoggingView {
        anchors.bottom: parent.bottom
        particleSystems: [psystem, psystem2]
    }
}
