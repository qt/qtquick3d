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
import QtQuick3D.Helpers
import QtQuick.Controls

Window {
    id: window
    width: 1024
    height: 768
    visible: true
    title: qsTr("Reflection Probes")

    WasdController {
        controlledObject: camera
    }

    View3D {
        id: view
        anchors.fill: parent
        environment: SceneEnvironment {
            clearColor: window.color
            backgroundMode: SceneEnvironment.SkyBox
            lightProbe: Texture {
                source: "res/OpenfootageNET_lowerAustria01-1024.hdr"
            }
            probeOrientation: Qt.vector3d(0, -90, 0)
        }

        PerspectiveCamera {
            id: camera
            position: Qt.vector3d(-100, 500, 600)
            eulerRotation.x: -30
        }

        DirectionalLight { }

        Model {
            property real angle: 0
            source: "#Sphere"
            x: Math.cos(angle) * 100
            z: Math.sin(angle) * 100
            y: 300
            receivesReflections: settingsPanel.sphereReceivesReflection
            materials: PrincipledMaterial {
                metalness: 1.0
                roughness: settingsPanel.sphereRoughness
            }

            NumberAnimation on angle {
                from: 0
                to: Math.PI * 2
                duration: 8000
                loops: Animation.Infinite
            }

            ReflectionProbe {
                timeSlicing: {
                    if (settingsPanel.timeSlicingIndex == 0) ReflectionProbe.None
                    else if (settingsPanel.timeSlicingIndex == 1) ReflectionProbe.AllFacesAtOnce
                    else ReflectionProbe.IndividualFaces
                }
                refreshMode: {
                    if (settingsPanel.refreshModeIndex == 0) ReflectionProbe.EveryFrame
                    else ReflectionProbe.FirstFrame
                }
                quality: {
                    if (settingsPanel.qualityIndex == 0) ReflectionProbe.VeryLow
                    else if (settingsPanel.qualityIndex == 1) ReflectionProbe.Low
                    else if (settingsPanel.qualityIndex == 2) ReflectionProbe.Medium
                    else if (settingsPanel.qualityIndex == 3) ReflectionProbe.High
                    else ReflectionProbe.VeryHigh
                }
                boxSize: Qt.vector3d(300, 300, 300)
            }
        }

        Model {
            position: Qt.vector3d(200, 300, 0)
            source: "#Sphere"
            materials: PrincipledMaterial {
                baseColor: "green"
            }
        }

        Model {
            position: Qt.vector3d(-200, 300, 0)
            source: "#Sphere"
            materials: PrincipledMaterial {
                baseColor: "green"
            }
        }

        Model {
            position: Qt.vector3d(0, 200, -285)
            scale: Qt.vector3d(6, 4, 0.3)
            source: "#Cube"
            materials: PrincipledMaterial {
                baseColor: "yellow"
                roughness: 0.1
                metalness: 1.0
            }
        }

        Model {
            scale: Qt.vector3d(6, 0.1, 6)
            source: "#Cube"
            receivesReflections: settingsPanel.floorReceivesReflection
            materials: PrincipledMaterial {
                baseColor: "lightBlue"
                roughness: 0.1
                metalness: 1.0
            }
        }

        ReflectionProbe {
            position: settingsPanel.probePosition
            timeSlicing: {
                if (settingsPanel.timeSlicingIndex == 0) ReflectionProbe.None
                else if (settingsPanel.timeSlicingIndex == 1) ReflectionProbe.AllFacesAtOnce
                else ReflectionProbe.IndividualFaces
            }
            refreshMode: {
                if (settingsPanel.refreshModeIndex == 0) ReflectionProbe.EveryFrame
                else ReflectionProbe.FirstFrame
            }
            quality: {
                if (settingsPanel.qualityIndex == 0) ReflectionProbe.VeryLow
                else if (settingsPanel.qualityIndex == 1) ReflectionProbe.Low
                else if (settingsPanel.qualityIndex == 2) ReflectionProbe.Medium
                else if (settingsPanel.qualityIndex == 3) ReflectionProbe.High
                else ReflectionProbe.VeryHigh
            }
            parallaxCorrection: settingsPanel.probeParallaxCorrection
            boxSize: settingsPanel.probeSize
        }

        ParticleSystem3D {
            position: Qt.vector3d(0, 300, 0)

            SpriteParticle3D {
                id: snowFlakeParticle
                sprite: Texture {
                    source: "res/snowflake.png"
                }
                maxAmount: 100
                particleScale: 20.0
                fadeOutDuration: 500
                billboard: true
            }

            ParticleEmitter3D {
                enabled: settingsPanel.spriteParticlesEnabled
                particle: snowFlakeParticle
                velocity: VectorDirection3D {
                    direction: Qt.vector3d(100, 200, 0)
                    directionVariation: Qt.vector3d(20, 20, 20)
                }
                particleScaleVariation: 0.4
                emitRate: 50
                lifeSpan: 4000
            }

            Gravity3D {
                magnitude: 100
            }
        }

        ParticleSystem3D {
            position: Qt.vector3d(0, 300, 0)

            Component {
                id: particleComponent
                Model {
                    source: "#Sphere"
                    scale: Qt.vector3d(0.8, 0.8, 0.8)
                    receivesReflections: true
                    materials: PrincipledMaterial {
                        baseColor: "red"
                        roughness: 0.1
                        metalness: 1.0
                    }
                }
            }

            ModelParticle3D {
                id: particleRed
                delegate: particleComponent
                maxAmount: 10
            }

            ParticleEmitter3D {
                enabled: settingsPanel.modelParticlesEnabled
                particle: particleRed
                velocity: VectorDirection3D {
                    direction: Qt.vector3d(-100, 200, 0)
                }
                emitRate: 1
                lifeSpan: 4000
            }

            Gravity3D {
                magnitude: 100
            }
        }
    }

    SettingsPanel {
        id: settingsPanel
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.bottom: parent.bottom
    }
}
