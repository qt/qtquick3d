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
    property real fontSize: width * 0.012

    View3D {
        id: view
        anchors.fill: parent

        environment: SceneEnvironment {
            clearColor: "#202020"
            backgroundMode: SceneEnvironment.Color
            antialiasingMode: SceneEnvironment.MSAA
            antialiasingQuality: SceneEnvironment.High
        }

        PerspectiveCamera {
            id: camera
            property real cameraAnim: 0
            SequentialAnimation on cameraAnim {
                running: true
                loops: Animation.Infinite
                NumberAnimation {
                    to: 1
                    duration: 4000
                    easing.type: Easing.InOutQuad
                }
                NumberAnimation {
                    to: 0
                    duration: 4000
                    easing.type: Easing.InOutQuad
                }
            }
            position: Qt.vector3d(500 * Math.sin(cameraAnim * Math.PI * 2), 0, 500 * Math.cos(cameraAnim * Math.PI * 2))
            eulerRotation: Qt.vector3d(0, cameraAnim * 360, 0)
        }

        Timer {
            running: true
            repeat: true
            interval: 4000
            onTriggered: {
                if (shape1.delegate === cube) {
                    shape1.delegate = suzanne;
                    shape2.delegate = suzanne;
                } else if (shape1.delegate === suzanne) {
                    shape1.delegate = cube;
                    shape2.delegate = cube;
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
                maxAmount: 6000
                color: "#ffffff"
                billboard: true
                blendMode: SpriteParticle3D.Screen
            }

            Component {
                id: suzanne
                Model {
                    source: "meshes/suzanne.mesh"
                    scale: Qt.vector3d(100, 100, 100)
                    materials: DefaultMaterial { diffuseColor: "red" }
                }
            }

            Component {
                id: cube
                Model {
                    source: "#Cube"
                    scale: Qt.vector3d(2, 2, 2)
                    materials: DefaultMaterial { diffuseColor: "red" }
                }
            }

            ParticleEmitter3D {
                particle: particleFire
                position: Qt.vector3d(-150, 0, 0)
                particleScale: 1
                particleScaleVariation: 1
                velocity: VectorDirection3D {
                    direction: Qt.vector3d(0, 60, 0)
                    directionVariation: Qt.vector3d(6, 6, 6)
                }
                emitRate: 3000
                lifeSpan: 1000
                shape: ParticleModelShape3D {
                    id: shape1
                    delegate: suzanne
                }
                Node {
                    x: -30
                    y: 150
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "Filled"
                        font.pointSize: mainWindow.fontSize
                        color: "#ffffff"
                    }
                }
            }

            ParticleEmitter3D {
                particle: particleFire
                position: Qt.vector3d(150, 0, 0)
                particleScale: 1
                particleScaleVariation: 1
                velocity: VectorDirection3D {
                    direction: Qt.vector3d(0, 60, 0)
                    directionVariation: Qt.vector3d(6, 6, 6)
                }
                emitRate: 3000
                lifeSpan: 1000
                shape: ParticleModelShape3D {
                    id: shape2
                    delegate: suzanne
                    fill: false
                }
                Node {
                    x: -30
                    y: 150
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "Surface"
                        font.pointSize: mainWindow.fontSize
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
