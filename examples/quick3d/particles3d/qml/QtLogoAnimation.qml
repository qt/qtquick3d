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

    property real cubeRotation: 0

    anchors.fill: parent

    NumberAnimation on cubeRotation {
        loops: Animation.Infinite
        from: 0
        to: 360
        duration: 6000
    }

    SequentialAnimation {
        id: mainAnimation
        running: true
        loops: Animation.Infinite
        ScriptAction {
            script: {
                // Burst at time 0
                psystem1.time = 0;
                emitter1.burst(2000);
            }
        }
        NumberAnimation {
            target: psystem1
            property: "time"
            from: 3000
            to: 0
            duration: 6000
            easing.type: Easing.OutQuad
        }
        ScriptAction {
            script: {
                emitter3.burst(500);
            }
        }
        PauseAnimation {
            duration: 500
        }
        ScriptAction {
            script: {
                emitter2.burst(50);
            }
        }
        ParallelAnimation {
            NumberAnimation {
                target: qtCube
                property: "opacity"
                to: 0.9
                duration: 1000
            }
            NumberAnimation {
                target: psystem1
                property: "time"
                from: 0
                to: 3000
                duration: 1000
                easing.type: Easing.InQuad
            }
        }
        PauseAnimation {
            duration: 3000
        }
        NumberAnimation {
            target: qtCube
            property: "opacity"
            to: 0.0
            duration: 1000
        }
    }

    View3D {
        id: view3D
        anchors.fill: parent

        environment: SceneEnvironment {
            clearColor: "#202020"
            backgroundMode: SceneEnvironment.Color
            antialiasingMode: SceneEnvironment.MSAA
            antialiasingQuality: SceneEnvironment.High
        }

        PerspectiveCamera {
            id: camera
            position.z: 600
        }

        PointLight {
            position: Qt.vector3d(200, 400, 200)
            brightness: 10
            ambientColor: Qt.rgba(0.2, 0.2, 0.2, 1.0)
        }

        // Particle models
        Component {
            id: dotParticleComponent
            Model {
                source: "#Cube"
                scale: Qt.vector3d(0.02, 0.02, 0.02)
                materials: DefaultMaterial {
                    lighting: DefaultMaterial.NoLighting
                }
            }
        }
        Component {
            id: smokeParticleComponent
            Model {
                source: "#Rectangle"
                scale: Qt.vector3d(6, 6, 6)
                materials: DefaultMaterial {
                    diffuseMap: Texture { source: "images/smoke.png" }
                    lighting: DefaultMaterial.NoLighting
                }
                opacity: 0.2
            }
        }

        Component {
            id: starParticleComponent
            Model {
                source: "#Rectangle"
                scale: Qt.vector3d(0.5, 0.5, 0.5)
                materials: DefaultMaterial {
                    diffuseMap: Texture { source: "images/star.png" }
                    lighting: DefaultMaterial.NoLighting
                    cullMode: DefaultMaterial.NoCulling
                }
                opacity: 0.2
            }
        }

        Node {
            eulerRotation: Qt.vector3d(20, -40 + cubeRotation, -10 + cubeRotation)

            Model {
                id: qtCube
                source: "#Cube"
                scale: Qt.vector3d(2.0, 2.0, 2.0)
                opacity: 0
                materials: DefaultMaterial {
                    diffuseMap: Texture { source: "images/qt_logo.png" }
                }
            }

            ParticleSystem3D {
                id: psystem1
                // We animate this system time manually
                running: false

                ModelParticle3D {
                    id: particleWhite
                    delegate: dotParticleComponent
                    maxAmount: 2000
                    color: "#ffffff"
                    colorVariation: Qt.vector4d(0, 0, 0, 0.8)
                    fadeInEffect: ModelParticle3D.FadeNone
                    fadeOutEffect: ModelParticle3D.FadeOpacity
                    fadeOutDuration: 3000
                }

                ParticleEmitter3D {
                    id: emitter1
                    particle: particleWhite
                    scale: Qt.vector3d(2.0, 2.0, 2.0)
                    shape: ParticleShape3D {
                        type: ParticleShape3D.Cube
                        fill: false
                    }
                    velocity: TargetDirection3D {
                        magnitude: -0.6
                        magnitudeVariation: 0.4
                    }
                    lifeSpan: 3000
                }

                Wander3D {
                    uniqueAmount: Qt.vector3d(40.0, 40.0, 40.0)
                    uniquePace: Qt.vector3d(0.2, 0.2, 0.2)
                    uniqueAmountVariation: 0.5
                    uniquePaceVariation: 0.5
                }
            }
        }

        ParticleSystem3D {
            id: psystem2

            running: true

            ModelParticle3D {
                id: smokeParticle
                delegate: smokeParticleComponent
                maxAmount: 50
                color: "#ffffff"
                colorVariation: Qt.vector4d(0, 0, 0, 0.6)
                fadeInEffect: ModelParticle3D.FadeScale
                fadeOutEffect: ModelParticle3D.FadeOpacity
                fadeOutDuration: 2000
            }
            ModelParticle3D {
                id: starParticle
                delegate: starParticleComponent
                maxAmount: 500
                color: "#ffff00"
                colorVariation: Qt.vector4d(0, 0.2, 0, 0)
                fadeInEffect: ModelParticle3D.FadeScale
                fadeOutEffect: ModelParticle3D.FadeOpacity
                fadeOutDuration: 2000
            }

            ParticleEmitter3D {
                id: emitter2
                particle: smokeParticle
                scale: Qt.vector3d(0.1, 0.1, 0.1)
                shape: ParticleShape3D {
                    type: ParticleShape3D.Sphere
                }
                particleRotationVariation: Qt.vector3d(20, 20, 180)
                particleRotationVelocityVariation: Qt.vector3d(0, 0, 100)
                velocity: TargetDirection3D {
                    normalized: true
                    magnitude: -200.0
                    magnitudeVariation: 0.5
                }
                lifeSpan: 4000
            }
            ParticleEmitter3D {
                id: emitter3
                particle: starParticle
                scale: Qt.vector3d(0.1, 0.1, 0.1)
                shape: ParticleShape3D {
                    type: ParticleShape3D.Sphere
                }
                particleScale: 2.0
                particleScaleVariation: 1.0
                particleEndScale: 4.0
                particleRotationVariation: Qt.vector3d(0, 0, 180)
                particleRotationVelocityVariation: Qt.vector3d(0, 0, 200)
                velocity: TargetDirection3D {
                    normalized: true
                    magnitudeVariation: 150
                }
                lifeSpan: 2500
            }
        }
    }
}
