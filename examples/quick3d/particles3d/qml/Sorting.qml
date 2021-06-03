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
    anchors.fill: parent

    View3D {
        id: view
        anchors.fill: parent

        PointLight {
            color: "white"
            ambientColor: "gray"
            brightness: 2.0
        }

        Model {
            source: "#Rectangle"
            eulerRotation: Qt.vector3d(-90, 0, 0)
            scale: Qt.vector3d(10, 10, 1)
            materials: [ DefaultMaterial {
                    diffuseColor: "red"
                }
            ]
        }

        Model {
            source: "#Cube"
            scale: Qt.vector3d(0.3, 0.3, 0.3)
            materials: [DefaultMaterial {
                    diffuseColor: "white"
                }
            ]
        }

        Component {
            id: particleComponent
            Model {
                source: "#Cube"
                scale: Qt.vector3d(0.1, 0.1, 0.1)
                materials: PrincipledMaterial {
                    metalness: 0.5
                    roughness: 0
                    specularAmount: 1.0
                }
                opacity: 0.75
            }
        }

        ParticleSystem3D {
            id: psystem

            ModelParticle3D {
                id: modelParticle
                delegate: particleComponent
                maxAmount: 60
                color: Qt.rgba(0.5, 0.5, 0.5, 0.5)
                colorVariation: Qt.vector4d(0.5, 0.5, 0.5, 0.15)
                sortMode: sortModeSelectionBox.selection
            }

            SpriteParticle3D {
                id: spriteParticle
                color: Qt.rgba(0.75, 0.75, 0.75, 0.75)
                colorVariation: Qt.vector4d(0.5, 0.5, 0.5, 0.05)
                maxAmount: 60
                billboard: true
                sortMode: sortModeSelectionBox.selection
            }

            // Emitters, one per particle
            ParticleEmitter3D {
                x: 35
                particle: spriteParticle
                particleScaleVariation: 0.3
                velocity: VectorDirection3D {
                    direction: Qt.vector3d(0, 5, 15)
                    directionVariation: Qt.vector3d(0, 0, 5)
                }
                emitRate: 10
                lifeSpan: 6000
            }
            ParticleEmitter3D {
                x: -35
                particle: modelParticle
                particleScaleVariation: 0.2
                velocity: VectorDirection3D {
                    direction: Qt.vector3d(0, 5, 15)
                }
                emitRate: 10
                lifeSpan: 6000
            }
        }

        Node {
            property real rot: 0.0
            PropertyAnimation on rot {
                from: 0.0
                to: 360.0
                duration: 10000
                loops: Animation.Infinite
                running: true
                paused: !checkBoxRotateCamera.checked
            }
            eulerRotation: Qt.vector3d(0, rot, 0)
            PerspectiveCamera {
                y: 20
                z: 100
            }
        }

        environment: SceneEnvironment {
            clearColor: "darkGray"
            backgroundMode: SceneEnvironment.Color
        }
    }
    SettingsView {
        id: settingsView
        CustomSelectionBox {
            id: sortModeSelectionBox
            text: "Mode"
            values: ["SortNone", "SortNewest", "SortOldest", "SortDistance"]
            parentWidth: settingsView.width
        }
        CustomCheckBox {
            id: checkBoxRotateCamera
            text: "Rotate camera"
            checked: false
        }
    }
    LoggingView {
        anchors.bottom: parent.bottom
        particleSystems: [psystem]
    }
}
