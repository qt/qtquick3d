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

    property int burstCount: 300

    anchors.fill: parent

    View3D {
        id: view3D
        anchors.fill: parent
        camera: camera

        environment: SceneEnvironment {
            clearColor: "#202020"
            backgroundMode: SceneEnvironment.Color
            antialiasingMode: SceneEnvironment.MSAA
            antialiasingQuality: SceneEnvironment.High
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
                scale: Qt.vector3d(0.2, 0.2, 0.2)
                materials: DefaultMaterial {
                }
            }
        }

        Component {
            id: particleComponentSpark
            Model {
                source: "#Rectangle"
                scale: Qt.vector3d(0.1, 0.1, 0.0)
                opacity: 0.5
                materials: DefaultMaterial {
                    diffuseMap: Texture {
                        source: "images/star.png"
                    }
                }
            }
        }

        ParticleSystem3D {
            id: psystem

            useRandomSeed: checkBoxRandomize.checked

            // Particles
            ModelParticle3D {
                id: particleSpark
                delegate: particleComponentSpark
                maxAmount: 2000
                fadeInDuration: 200
                fadeOutDuration: 500
                color: "#ffe000"
                colorVariation: Qt.vector4d(0.2, 0.2, 0.0, 0.5)
            }
            ModelParticle3D {
                id: particleWhite
                delegate: particleComponent
                maxAmount: 16
                color: "#ffffff"
            }
            ModelParticle3D {
                id: particleRed
                delegate: particleComponent
                maxAmount: burstCount * 3
                color: "#ff0000"
            }

            // Emitters, one per particle
            ParticleEmitter3D {
                particle: particleWhite
                position: Qt.vector3d(0, -150, 0)
                particleScaleVariation: 0.6
                particleRotationVariation: Qt.vector3d(180, 180, 180)
                particleRotationVelocityVariation: Qt.vector3d(200, 200, 200);
                velocity: VectorDirection3D {
                    direction: Qt.vector3d(0, 400, 0)
                    directionVariation: Qt.vector3d(80, 40, 20)
                }
                emitRate: 4
                lifeSpan: 4000
            }

            TrailEmitter3D {
                id: sparkEmitter
                particle: particleSpark
                follow: particleWhite
                particleScaleVariation: 0.5
                particleRotationVariation: Qt.vector3d(0, 0, 180)
                particleRotationVelocityVariation: Qt.vector3d(100, 100, 100);
                velocity: VectorDirection3D {
                    directionVariation: Qt.vector3d(20, 20, 20)
                }
                emitRate: 100
                lifeSpan: 1000
            }

            ParticleEmitter3D {
                id: burstEmitter
                particle: particleRed
                scale: Qt.vector3d(0.5, 0.5, 0.5)
                particleScale: 0.2
                particleEndScale: 0.4
                particleRotationVariation: Qt.vector3d(180, 180, 180)
                particleRotationVelocityVariation: Qt.vector3d(200, 200, 200);
                shape: ParticleShape3D {
                    type: ParticleShape3D.Sphere
                    fill: false
                }
                velocity: TargetDirection3D {
                    position: burstEmitter.position
                    magnitude: -4.0
                }
                lifeSpan: 1000
                lifeSpanVariation: 500
            }

            Gravity3D {
                direction: Qt.vector3d(0, 1, 0)
                magnitude: -200
                particles: [particleWhite]
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            var pos = view3D.mapTo3DScene(Qt.vector3d(mouseX, mouseY, camera.z));
            burstEmitter.setPosition(pos);
            burstEmitter.burst(burstCount);
        }
    }

    Row {
        y: 10
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 10
        Button {
            text: psystem.running ? qsTr("Stop") : qsTr("Start")
            onClicked: {
                psystem.running = !psystem.running;
            }
        }
        Button {
            text: psystem.paused ? qsTr("Continue") : qsTr("Pause")
            enabled: psystem.running
            onClicked: {
                psystem.paused = !psystem.paused;
            }
        }
    }

    SettingsView {
        CustomLabel {
            text: "ParticleSystem seed: " + psystem.seed
        }
        CustomCheckBox {
            id: checkBoxRandomize
            text: "Use random seed"
            checked: true
        }
        CustomLabel {
            text: "Custom seed"
            opacity: psystem.useRandomSeed ? 0.4 : 1.0
        }
        CustomSlider {
            id: sliderTimelineTime
            sliderValue: 0
            sliderEnabled: !psystem.useRandomSeed
            fromValue: 0
            toValue: 99
            sliderStepSize: 1
            onSliderValueChanged: psystem.setSeed(sliderValue);
        }
    }

    LoggingView {
        anchors.bottom: parent.bottom
        particleSystems: [psystem]
    }
}
