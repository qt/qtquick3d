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

    property bool fill: checkBoxFill.checked

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
            property real cameraAnim: 0
            SequentialAnimation on cameraAnim {
                running: true
                loops: Animation.Infinite
                NumberAnimation {
                    to: 1
                    duration: 2000
                    easing.type: Easing.InOutQuad
                }
                NumberAnimation {
                    to: 0
                    duration: 2000
                    easing.type: Easing.InOutQuad
                }
            }
            position: Qt.vector3d(0, 400 + cameraAnim * 400, 800 - cameraAnim * 800)
            eulerRotation: Qt.vector3d(-30 - cameraAnim * 60, 0, 0)
        }

        PointLight {
            position: Qt.vector3d(0, 400, 0)
            brightness: 10
            ambientColor: Qt.rgba(0.3, 0.3, 0.3, 1.0)
        }

        ParticleSystem3D {
            id: psystem

            NumberAnimation on eulerRotation.y {
                running: true
                loops: Animation.Infinite
                from: 0
                to: 360
                duration: 12000
            }

            // Particles
            SpriteParticle3D {
                id: particleWhite
                sprite: Texture {
                    source: "images/dot.png"
                }
                maxAmount: 4000 * 3
                color: "#ffffff"
                billboard: true
            }

            // Emitters, one per particle
            ParticleEmitter3D {
                particle: particleWhite
                position: Qt.vector3d(-300, 0, 0)
                scale: Qt.vector3d(2.0, 2.0, 3.0)
                shape: ParticleShape3D {
                    type: ParticleShape3D.Cube
                    fill: mainWindow.fill
                }
                emitRate: sliderEmitRate.sliderValue
                lifeSpan: 2000
                depthBias: depthBias.sliderValue
                Model {
                    source: "#Cube"
                    opacity: 0.4
                    materials: DefaultMaterial {
                    }
                }
            }
            ParticleEmitter3D {
                particle: particleWhite
                position: Qt.vector3d(0, 0, 0)
                scale: Qt.vector3d(2.0, 2.0, 3.0)
                shape: ParticleShape3D {
                    type: ParticleShape3D.Sphere
                    fill: mainWindow.fill
                }
                emitRate: sliderEmitRate.sliderValue
                lifeSpan: 2000
                depthBias: depthBias.sliderValue
                Model {
                    source: "#Sphere"
                    opacity: 0.4
                    materials: DefaultMaterial {
                    }
                }
            }
            ParticleEmitter3D {
                particle: particleWhite
                position: Qt.vector3d(300, 0, 0)
                scale: Qt.vector3d(2.0, 2.0, 3.0)
                shape: ParticleShape3D {
                    type: ParticleShape3D.Cylinder
                    fill: mainWindow.fill
                }
                emitRate: sliderEmitRate.sliderValue
                lifeSpan: 2000
                depthBias: depthBias.sliderValue
                Model {
                    source: "#Cylinder"
                    opacity: 0.4
                    materials: DefaultMaterial {
                    }
                }
            }
        }
    }

    SettingsView {
        CustomCheckBox {
            id: checkBoxFill
            text: "Fill"
            checked: false
        }
        CustomLabel {
            text: "Particles emitRate"
        }
        CustomSlider {
            id: sliderEmitRate
            sliderValue: 1000
            fromValue: 0
            toValue: 2000
        }
        CustomLabel {
            text: "Particle depthBias"
        }
        CustomSlider {
            id: depthBias
            sliderValue: -20
            fromValue: -20
            toValue: 20
        }
    }

    LoggingView {
        anchors.bottom: parent.bottom
        particleSystems: [psystem]
    }
}
