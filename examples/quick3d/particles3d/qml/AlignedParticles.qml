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

            SequentialAnimation {
                running: true
                loops: Animation.Infinite
                NumberAnimation {
                    target: camera
                    property: "eulerRotation.x"
                    to: -90
                    duration: 20000
                    easing.type: Easing.Linear
                }
                NumberAnimation {
                    target: camera
                    property: "eulerRotation.x"
                    to: 0
                    duration: 2000
                    easing.type: Easing.Linear
                }
            }

            SequentialAnimation {
                running: true
                loops: Animation.Infinite
                NumberAnimation {
                    target: camera
                    property: "position.y"
                    to: 1400
                    duration: 20000
                    easing.type: Easing.Linear
                }
                NumberAnimation {
                    target: camera
                    property: "position.y"
                    to: 0
                    duration: 2000
                    easing.type: Easing.Linear
                }
            }

            SequentialAnimation {
                running: true
                loops: Animation.Infinite
                NumberAnimation {
                    target: camera
                    property: "position.z"
                    to: 0
                    duration: 20000
                    easing.type: Easing.Linear
                }
                NumberAnimation {
                    target: camera
                    property: "position.z"
                    to: 600
                    duration: 2000
                    easing.type: Easing.Linear
                }
            }
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
                source: "#Cone"
                scale: Qt.vector3d(0.1, 0.1, 0.1)
                materials: DefaultMaterial {
                }
            }
        }

        ParticleSystem3D {
            id: psystem

            // Particles
            ModelParticle3D {
                id: particleVelocity
                delegate: particleComponent
                maxAmount: 250
                color: "#ff0000"
                alignMode: Particle3D.AlignTowardsStartVelocity
            }

            ModelParticle3D {
                id: particleCamera
                delegate: particleComponent
                maxAmount: 250
                color: "#00ff00"
                alignMode: Particle3D.AlignTowardsTarget
                alignTargetPosition: camera.position
            }

            ModelParticle3D {
                id: particleNoAlign
                delegate: particleComponent
                maxAmount: 250
                color: "#0000ff"
            }

            SpriteParticle3D {
                id: particleBillboard
                sprite: Texture {
                    source: "images/snowflake.png"
                }
                maxAmount: 250
                color: "#ffffff"
                billboard: true
            }

            SpriteParticle3D {
                id: particleNoBillboard
                sprite: Texture {
                    source: "images/snowflake.png"
                }
                maxAmount: 250
                color: "#aaaaff"
            }

            // Emitters, one per particle
            ParticleEmitter3D {
                particle: particleVelocity
                position: Qt.vector3d(400, 50, 0)
                particleScale: 2
                particleRotation: Qt.vector3d(90, 0, 0)
                velocity: VectorDirection3D {
                    direction: Qt.vector3d(0, 100, 0)
                    directionVariation: Qt.vector3d(30, 30, 30)
                }
                emitRate: 10
                lifeSpan: 4000
                Node {
                    x: 20
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "StartVelocity"
                        font.pointSize: settings.fontSizeLarge
                        color: "#ffffff"
                    }
                }
            }

            ParticleEmitter3D {
                particle: particleCamera
                position: Qt.vector3d(200, 50, 0)
                particleScale: 2
                particleRotation: Qt.vector3d(90, 0, 0)
                velocity: VectorDirection3D {
                    direction: Qt.vector3d(0, 100, 0)
                    directionVariation: Qt.vector3d(30, 30, 30)
                }
                emitRate: 10
                lifeSpan: 4000
                Node {
                    x: 20
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "Camera"
                        font.pointSize: settings.fontSizeLarge
                        color: "#ffffff"
                    }
                }
            }

            ParticleEmitter3D {
                particle: particleNoAlign
                position: Qt.vector3d(0, 50, 0)
                particleScale: 2
                particleRotation: Qt.vector3d(90, 0, 0)
                velocity: VectorDirection3D {
                    direction: Qt.vector3d(0, 100, 0)
                    directionVariation: Qt.vector3d(30, 30, 30)
                }
                emitRate: 10
                lifeSpan: 4000
                Node {
                    x: 20
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "NoAlign"
                        font.pointSize: settings.fontSizeLarge
                        color: "#ffffff"
                    }
                }
            }

            ParticleEmitter3D {
                particle: particleBillboard
                position: Qt.vector3d(-200, 50, 0)
                particleScale: 8
                velocity: VectorDirection3D {
                    direction: Qt.vector3d(0, 100, 0)
                    directionVariation: Qt.vector3d(30, 30, 30)
                }
                emitRate: 10
                lifeSpan: 4000
                Node {
                    x: 20
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "Billboard"
                        font.pointSize: settings.fontSizeLarge
                        color: "#ffffff"
                    }
                }
            }

            ParticleEmitter3D {
                particle: particleNoBillboard
                position: Qt.vector3d(-400, 50, 0)
                particleScale: 8
                velocity: VectorDirection3D {
                    direction: Qt.vector3d(0, 100, 0)
                    directionVariation: Qt.vector3d(30, 30, 30)
                }
                emitRate: 10
                lifeSpan: 4000
                Node {
                    x: 20
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "NoBillboard"
                        font.pointSize: settings.fontSizeLarge
                        color: "#ffffff"
                    }
                }
            }
        }
    }

    LoggingView {
        anchors.bottom: parent.bottom
        particleSystems: [psystem]
    }
}
