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
    id: mainWindow

    anchors.fill: parent

    View3D {
        id: view3D
        anchors.fill: parent

        environment: SceneEnvironment {
            clearColor: "#202020"
            backgroundMode: SceneEnvironment.Color
            antialiasingMode: settings.antialiasingMode
            antialiasingQuality: settings.antialiasingQuality
        }

        PerspectiveCamera {
            id: camera
            z: 600
            y: -20
        }

        PointLight {
            position: Qt.vector3d(0, 400, 300)
            brightness: 40
            ambientColor: Qt.rgba(0.1, 0.1, 0.1, 1.0)
        }

        // Model shared between particles
        Component {
            id: particleComponent
            Model {
                source: "#Cube"
                scale: Qt.vector3d(0.2, 0.2, 0.2)
                materials: DefaultMaterial {
                }
            }
        }

        Node {
            SequentialAnimation on eulerRotation.x {
                loops: Animation.Infinite
                NumberAnimation {
                    to: -80
                    duration: 2500
                    easing.type: Easing.InOutQuad
                }
                NumberAnimation {
                    to: 0
                    duration: 2500
                    easing.type: Easing.InOutQuad
                }
            }
            ParticleSystem3D {
                id: psystem

                // Particles
                ModelParticle3D {
                    id: particleWhite
                    delegate: particleComponent
                    maxAmount: 1200
                    color: "#ffffff"
                }

                ParticleEmitter3D {
                    id: emitter1
                    particle: particleWhite
                    position: Qt.vector3d(-300, -200, 0)
                    particleScale: 0.2
                    particleEndScale: 1.0
                    particleRotationVariation: Qt.vector3d(180, 180, 180)
                    velocity: VectorDirection3D {
                        direction: Qt.vector3d(0, 200, 0)
                        directionVariation: Qt.vector3d(20, 0, 20)
                    }
                    lifeSpan: 3000
                    emitRate: 100
                    Timer {
                        running: true
                        repeat: true
                        interval: 500
                        onTriggered: {
                            emitter1.enabled = !emitter1.enabled
                        }
                    }
                }
                Node {
                    position: emitter1.position
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        horizontalAlignment: Text.AlignHCenter
                        text: "Enabling\nEmitter"
                        font.pointSize: settings.fontSizeLarge
                        color: "#ffffff"
                    }
                }

                ParticleEmitter3D {
                    id: emitter2
                    particle: particleWhite
                    position: Qt.vector3d(-100, -200, 0)
                    particleScale: 0.2
                    particleEndScale: 1.0
                    particleRotationVariation: Qt.vector3d(180, 180, 180)
                    velocity: VectorDirection3D {
                        direction: Qt.vector3d(0, 200, 0)
                        directionVariation: Qt.vector3d(20, 0, 20)
                    }
                    lifeSpan: 3000
                    emitRate: 0.1
                    SequentialAnimation on emitRate {
                        running: true
                        loops: Animation.Infinite
                        NumberAnimation {
                            duration: 2000
                            easing.type: Easing.InOutQuad
                            to: 100
                        }
                        NumberAnimation {
                            duration: 2000
                            easing.type: Easing.InOutQuad
                            to: 0.1
                        }
                    }
                }
                Node {
                    position: emitter2.position
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        horizontalAlignment: Text.AlignHCenter
                        text: "Animated\nemitRate"
                        font.pointSize: settings.fontSizeLarge
                        color: "#ffffff"
                    }
                }

                ParticleEmitter3D {
                    id: emitter3
                    particle: particleWhite
                    position: Qt.vector3d(100, -200, 0)
                    particleScale: 0.2
                    particleEndScale: 1.0
                    particleRotationVariation: Qt.vector3d(180, 180, 180)
                    velocity: VectorDirection3D {
                        direction: Qt.vector3d(0, 200, 0)
                        directionVariation: Qt.vector3d(20, 0, 20)
                    }
                    lifeSpan: 3000
                    Timer {
                        running: true
                        repeat: true
                        interval: 500
                        onTriggered: {
                            emitter3.burst(50);
                        }
                    }
                }
                Node {
                    position: emitter3.position
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        horizontalAlignment: Text.AlignHCenter
                        text: "Burst"
                        font.pointSize: settings.fontSizeLarge
                        color: "#ffffff"
                    }
                }

                ParticleEmitter3D {
                    id: emitter4
                    particle: particleWhite
                    position: Qt.vector3d(300, -200, 0)
                    particleScale: 0.2
                    particleEndScale: 1.0
                    particleRotationVariation: Qt.vector3d(180, 180, 180)
                    velocity: VectorDirection3D {
                        direction: Qt.vector3d(0, 200, 0)
                        directionVariation: Qt.vector3d(20, 0, 20)
                    }
                    lifeSpan: 3000
                    emitRate: 10
                    Timer {
                        running: true
                        repeat: true
                        interval: 500
                        onTriggered: {
                            emitter4.burst(50);
                        }
                    }
                }
                Node {
                    position: emitter4.position
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        horizontalAlignment: Text.AlignHCenter
                        text: "Emit and\nBurst"
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
