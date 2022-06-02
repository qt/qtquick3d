/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
import QtQuick3D.Helpers

Item {
    anchors.fill: parent

    View3D {
        anchors.fill: parent

        environment: SceneEnvironment {
            clearColor: "#101010"
            backgroundMode: SceneEnvironment.Color
            antialiasingMode: settings.antialiasingMode
            antialiasingQuality: settings.antialiasingQuality
        }
        PerspectiveCamera {
            id: camera
            position: Qt.vector3d(0, 100, 600)
            clipFar: 2000
        }
        WasdController {
            controlledObject: camera
        }
        PointLight {
            id: pointLight
            position.y: 100
            brightness: 100
            quadraticFade: 50
        }

        Model {
            source: "#Cube"
            position.y: -100
            scale: Qt.vector3d(20, 1, 20)
            materials: PrincipledMaterial {
                baseColor: Qt.rgba(1.0, 0.8, 0.8, 1.0)
            }
        }
        Node {
            property real angle: 0.0
            NumberAnimation on angle {
                from: 0.0
                to: 360.0
                duration: 5000
                loops: Animation.Infinite
                running: true
            }
            eulerRotation.y: angle
            Model {
                id: sphere1
                source: "#Sphere"
                position.y: -50
                position.x: -100
                scale: Qt.vector3d(1.8, 1.8, 1.8)
                materials: PrincipledMaterial {
                    baseColor: Qt.rgba(0.8, 1.0, 0.8, 1.0)
                }
            }
            Model {
                id: sphere2
                source: "#Sphere"
                position.y: 0
                position.x: 300
                scale: Qt.vector3d(0.9, 0.9, 0.9)
                materials: PrincipledMaterial {
                    baseColor: Qt.rgba(0.8, 1.0, 0.8, 1.0)
                }
            }
        }

        //! [particle system]
        ParticleSystem3D {
            id: psystem

            property real particleScaleFactor : 1.0 + Math.abs(Math.cos(2.0 * Math.PI * scaleTemp))
            property real scaleTemp: 0.0
            NumberAnimation on scaleTemp {
                from: 0.0
                to: 1.0
                duration: 400
                loops: Animation.Infinite
                running: true
            }

            //! [particle system]

            //! [sprite particle]
            LineParticle3D {
                id: particle
                sprite: Texture {
                    source: "images/qt_logo.png"
                }
                maxAmount: 200 + emitter.emitRate * emitter.lifeSpan / 1000
                color: Qt.rgba(1.0, 1.0, 1.0, sliderOpacity.sliderValue * 0.01)
                colorVariation: Qt.vector4d(0.6, 0.6, 0.6, 0.0)
                fadeInDuration: 500
                fadeOutDuration: 500
                segmentCount: sliderSegmentCount.sliderValue
                alphaFade: sliderOpacityFade.sliderValue * 0.01
                scaleMultiplier: sliderSizeFactor.sliderValue
                texcoordMultiplier: sliderTexcoordFactor.sliderValue
                particleScale: sliderSize.sliderValue
                eolFadeOutDuration: 500
                texcoordMode: LineParticle3D.Relative
                billboard: billboardCheckBox.checked
                length: lengthCheckBox.checked ? lengthSlider.sliderValue : -1.0
                lengthVariation: lengthVariationSlider.sliderValue
                lengthDeltaMin: lengthDeltaSlider.sliderValue
                sortMode: LineParticle3D.SortDistance
            }
            //! [sprite particle]

            //! [particle emitter]
            ParticleEmitter3D {
                id: emitter
                particle: particle
                position: Qt.vector3d(-550, 0, 0)
                scale: Qt.vector3d(2.0, 0.5, 2.0)
                shape: ParticleShape3D {
                    type: ParticleShape3D.Sphere
                }
                velocity: VectorDirection3D {
                    direction: Qt.vector3d(sliderVelocityY.sliderValue * 40, 0, 0)
                    directionVariation: Qt.vector3d(sliderVelocityY.sliderValue * 20, 15, 0)
                }
                emitRate: sliderEmitRate.sliderValue
                lifeSpan: 500 * 1000 / (sliderVelocityY.sliderValue * 40)
            }

            ParticleEmitter3D {
                id: emitter2
                particle: particle
                position: Qt.vector3d(-550, 0, 60)
                scale: Qt.vector3d(2.0, 0.5, 2.0)
                shape: ParticleShape3D {
                    type: ParticleShape3D.Sphere
                }
                velocity: VectorDirection3D {
                    direction: Qt.vector3d(sliderVelocityY.sliderValue * 40, 0, 0)
                    directionVariation: Qt.vector3d(sliderVelocityY.sliderValue * 20, 15, 0)
                }
                emitRate: sliderEmitRate.sliderValue
                lifeSpan: 500 * 1000 / (sliderVelocityY.sliderValue * 40)
            }
            ParticleEmitter3D {
                id: emitter3
                particle: particle
                position: Qt.vector3d(-550, 0, 120)
                scale: Qt.vector3d(2.0, 0.5, 2.0)
                shape: ParticleShape3D {
                    type: ParticleShape3D.Sphere
                }
                velocity: VectorDirection3D {
                    direction: Qt.vector3d(sliderVelocityY.sliderValue * 40, 0, 0)
                    directionVariation: Qt.vector3d(sliderVelocityY.sliderValue * 20, 15, 0)
                }
                emitRate: sliderEmitRate.sliderValue
                lifeSpan: 500 * 1000 / (sliderVelocityY.sliderValue * 40)
            }
            //! [particle emitter]

            Repeller3D {
                particles: [particle]
                position: sphere1.scenePosition
                outerRadius: sliderRadius.sliderValue
                strength: sliderRepelStrength.sliderValue
            }
            Repeller3D {
                particles: [particle]
                position: sphere2.scenePosition
                outerRadius: sliderRadius.sliderValue * 0.5
                strength: sliderRepelStrength.sliderValue
            }
        }
    }

    SettingsView {
        CustomLabel {
            text: "Billboard particle"
        }
        CustomCheckBox {
            id: billboardCheckBox
            checked: true
        }
        CustomLabel {
            text: "Line Segment Count"
        }
        CustomSlider {
            id: sliderSegmentCount
            sliderValue: 10
            fromValue: 1
            toValue: 250
            sliderStepSize: 1
        }
        CustomLabel {
            text: "Line minimum length"
        }
        CustomSlider {
            id: lengthDeltaSlider
            sliderValue: 10.0
            fromValue: 0.0
            toValue: 50.0
        }
        CustomLabel {
            text: "Line Opacity Fade"
        }
        CustomSlider {
            id: sliderOpacityFade
            sliderValue: 0
            fromValue: 0
            toValue: 100
            sliderStepSize: 1
        }
        CustomLabel {
            text: "Size"
        }
        CustomSlider {
            id: sliderSize
            sliderValue: 1
            fromValue: 0.1
            toValue: 20.0
        }
        CustomLabel {
            text: "Line Size Modifier"
        }
        CustomSlider {
            id: sliderSizeFactor
            sliderValue: 1
            fromValue: 0.1
            toValue: 2.0
        }
        CustomLabel {
            text: "Line Texcoord Modifier"
        }
        CustomSlider {
            id: sliderTexcoordFactor
            sliderValue: 1
            fromValue: 0.01
            toValue: 20.0
        }
        CustomLabel {
            text: "Emit Rate"
        }
        CustomSlider {
            id: sliderEmitRate
            sliderValue: 50
            fromValue: 1
            toValue: 200
        }
        CustomLabel {
            text: "Start Velocity"
        }
        CustomSlider {
            id: sliderVelocityY
            sliderValue: 10
            fromValue: 0.1
            toValue: 50
        }
        CustomLabel {
            text: "Opacity"
        }
        CustomSlider {
            id: sliderOpacity
            sliderValue: 30.0
            fromValue: 0.0
            toValue: 100.0
        }
        CustomLabel {
            text: "Repel Radius"
        }
        CustomSlider {
            id: sliderRadius
            sliderValue: 1000
            fromValue: 1
            toValue: 2000
        }
        CustomLabel {
            text: "Repel Strength"
        }
        CustomSlider {
            id: sliderRepelStrength
            sliderValue: 50.0
            fromValue: 0.0
            toValue: 400.0
        }
        CustomLabel {
            text: "Fixed Length"
        }
        CustomCheckBox {
            id: lengthCheckBox
            checked: false
        }
        CustomLabel {
            text: "Line Length"
        }
        CustomSlider {
            id: lengthSlider
            sliderValue: 50.0
            fromValue: 10.0
            toValue: 200.0
        }
        CustomLabel {
            text: "Line Length Variation"
        }
        CustomSlider {
            id: lengthVariationSlider
            sliderValue: 0.0
            fromValue: 0.0
            toValue: 100.0
        }
    }

    LoggingView {
        anchors.bottom: parent.bottom
        particleSystems: [psystem]
    }
}
