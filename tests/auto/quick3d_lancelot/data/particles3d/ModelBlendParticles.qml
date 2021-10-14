/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tests of the Qt Toolkit.
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
import QtQuick.Timeline

Item {
    id: mainWindow
    anchors.fill: parent

    View3D {
        anchors.fill: parent
        camera: camera

        environment: SceneEnvironment {
            clearColor: "#404040"
            backgroundMode: SceneEnvironment.Color
        }

        Node {
            id: cameraRotation
            property real rot: 0.0
            eulerRotation: Qt.vector3d(-45, 0, 0)
            PerspectiveCamera {
                id: camera
                position: Qt.vector3d(0, 0, 1250)
                clipFar: 2000
            }
        }

        DirectionalLight {
            brightness: 50
            eulerRotation: Qt.vector3d(-10, -90, 0)
            castsShadow: true
            shadowFactor: 25
            shadowMapQuality: Light.ShadowMapQualityVeryHigh
        }

        Model {
            source: "#Rectangle"
            eulerRotation: Qt.vector3d(-90, 0, 0)
            y: -30
            scale: Qt.vector3d(13, 13, 1)
            receivesShadows: true
            castsShadows: false
            materials: [
                DefaultMaterial {
                    diffuseColor: "#0c100c"
                }
            ]
        }

        ParticleSystem3D {
            id: psystem
            running: false

            NumberAnimation on time {
                    loops: 1
                    from:0
                    to: 8000
                    duration: 10
                }

            Component {
                id: modelComponent
                Model {
                    source: "#Cube"
                    scale: Qt.vector3d(1, 1, 1)
                    receivesShadows: false
                    eulerRotation: Qt.vector3d(90, 0, 0)
                    position: Qt.vector3d(-400, 50, 0)
                    materials: [
                        PrincipledMaterial {
                            baseColor: "#41cd52"
                            metalness: 0.8
                            roughness: 0.1
                            specularAmount: 1
                            cullMode: Material.NoCulling
                        }
                    ]
                }
            }

            Node {
                id: translateNode
                x: 150
            }

            ModelBlendParticle3D {
                id: particle
                delegate: modelComponent
                endNode: translateNode
                modelBlendMode: "Explode"
                endTime: 1500
            }

            ParticleEmitter3D {
                id: emitter
                system: psystem
                particle: particle
                lifeSpan: particle.modelBlendMode === ModelBlendParticle3D.Explode ? 40000 : 4000
                emitRate: particle.maxAmount / 10
                velocity: VectorDirection3D {
                    direction: Qt.vector3d(50, 50, 0)
                    directionVariation: Qt.vector3d(0, 10, 10)
                }
                particleRotation: Qt.vector3d(20, 0, 3)
                particleRotationVariation: Qt.vector3d(4, 0, 1)
                particleScale: 1.0
                particleEndScale: 1.0
            }
        }
    }
}
