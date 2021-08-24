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
import QtQuick.Timeline
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: mainWindow

    anchors.fill: parent

    Timeline {
        id: timeline
        enabled: true
        startFrame: 0
        endFrame: 100
        animations: [
            TimelineAnimation {
                id: timelineAnimation
                running: true
                duration: (100 - timeline.currentFrame) * 100
                from: timeline.currentFrame
                to: 100
            }
        ]
        keyframeGroups: [
            KeyframeGroup {
                target: psystem
                property: "time"
                Keyframe { frame: 0; value: 0 }
                Keyframe { frame: 100; value: 15000 }
            },
            KeyframeGroup {
                target: cameraRotation
                property: "rot"
                Keyframe { frame: 0; value: 0 }
                Keyframe { frame: 100; value: 180 }
            },
            KeyframeGroup {
                target: actNode
                property: "y"
                Keyframe { frame: 0; value: 50 }
                Keyframe { frame: 100; value: -100 }
            }
        ]
    }

    View3D {
        anchors.fill: parent
        camera: camera

        environment: SceneEnvironment {
            clearColor: "#404040"
            backgroundMode: SceneEnvironment.Color
            antialiasingMode: settings.antialiasingMode
            antialiasingQuality: settings.antialiasingQuality
        }

        Node {
            id: cameraRotation
            property real rot: 0.0
            eulerRotation: Qt.vector3d(0, rot, 0)
            PerspectiveCamera {
                id: camera
                position: Qt.vector3d(0, 0, 150)
                clipFar: 2000
            }
        }

        DirectionalLight {
            brightness: 50
            eulerRotation: Qt.vector3d(-90, -90, 0)
            castsShadow: true
            shadowFactor: 25
            shadowMapQuality: Light.ShadowMapQualityHigh
        }
        DirectionalLight {
            eulerRotation: Qt.vector3d(180, 0, 0)
            brightness: 25
        }
        DirectionalLight {
            brightness: 25
        }

        Model {
            source: "#Rectangle"
            eulerRotation: Qt.vector3d(-90, 0, 0)
            y: -30
            scale: Qt.vector3d(3, 3, 1)
            receivesShadows: true
            materials: [
                DefaultMaterial {
                    diffuseColor: "#0c100c"
                }
            ]
        }

        ParticleSystem3D {
            id: psystem
            running: false
            x: -100

            Node {
                id: actNode
                eulerRotation: Qt.vector3d(-90, 0, 0)

                Model {
                    visible: enableActivationPlane.checked
                    source: "#Rectangle"
                    scale: Qt.vector3d(0.5, 0.5, 1)
                    receivesShadows: false
                    materials: [
                        DefaultMaterial {
                            lighting: DefaultMaterial.NoLighting
                            cullMode: Material.NoCulling
                            opacity: 0.25
                        }
                    ]
                }
            }

            Component {
                id: modelComponent
                Model {
                    source: "meshes/oldqtlogo.mesh"
                    scale: Qt.vector3d(10, 10, 10)
                    receivesShadows: false
                    materials: [
                        PrincipledMaterial {
                            baseColor: "#41cd52"
                            metalness: 0.8
                            roughness: 0.1
                            specularAmount: 1
                            cullMode: cullingModelBox.checked ? Material.BackFaceCulling : Material.NoCulling
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
                modelBlendMode: blendModeSelectionBox.selection
                endTime: 1500
                activationNode: enableActivationPlane.checked ? actNode : null
                random: enableRandom.checked
            }

            ParticleEmitter3D {
                id: emitter
                system: psystem
                particle: particle
                lifeSpan: particle.modelBlendMode === ModelBlendParticle3D.Explode ? 40000 : 4000
                emitRate: particle.maxAmount / 10
                velocity: VectorDirection3D {
                    direction: Qt.vector3d(50, 10, 0)
                    directionVariation: Qt.vector3d(0, 10, 10)
                }
                particleRotation: Qt.vector3d(20, 0, 3)
                particleRotationVariation: Qt.vector3d(4, 0, 1)
                particleScale: 1.0
                particleEndScale: 1.0
            }
        }
    }

    SettingsView {
        id: settingsView
        CustomCheckBox {
            id: enableActivationPlane
            text: "Enable activation plane"
            checked: false
            onCheckedChanged: {
                timeline.currentFrame = 0
                psystem.reset()
            }
        }
        CustomCheckBox {
            id: enableRandom
            text: "Random"
            checked: false
            enabled: !enableActivationPlane.checked
            onCheckedChanged: {
                timeline.currentFrame = 0
                psystem.reset()
            }
        }
        CustomCheckBox {
            id: cullingModelBox
            text: "Enable culling"
            checked: false
        }
        CustomSelectionBox {
            id: blendModeSelectionBox
            text: "Mode"
            values: ["Explode", "Construct", "Transfer"]
            parentWidth: settingsView.width
            onSelectionChanged: {
                timeline.currentFrame = 0
                psystem.reset()
            }
        }
    }

    Frame {
        id: toolbar
        anchors.left: parent.left
        anchors.leftMargin: 20
        anchors.right: parent.right
        anchors.rightMargin: 20
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 80
        height: 60
        padding: 0
        background: Rectangle {
            color: "#ffffff"
            radius: 4
            opacity: 0.2
        }

        RowLayout {
            anchors.fill: parent
            Button {
                id: playButton
                Layout.leftMargin: 14
                icon.source: timelineAnimation.running ? "qrc:/qml/images/icon_pause.png" : "qrc:/qml/images/icon_play.png"
                icon.width: toolbar.height - 10
                icon.height: toolbar.height - 10
                icon.color: "transparent"
                background: Rectangle {
                    color: "transparent"
                }
                onClicked: {
                    // If we are close to end, start from the beginning
                    if (timeline.currentFrame >= timeline.endFrame - 1.0)
                        timeline.currentFrame = 0;

                    timelineAnimation.running = !timelineAnimation.running;
                }
            }

            CustomSlider {
                id: sliderTimelineTime
                Layout.fillWidth: true
                sliderValue: timeline.currentFrame
                sliderEnabled: !timelineAnimation.running || timelineAnimation.paused
                fromValue: 0.0
                toValue: 100.0
                onSliderValueChanged: timeline.currentFrame = sliderValue;
            }
        }
    }

    LoggingView {
        id: loggingView
        anchors.bottom: parent.bottom
        particleSystems: [psystem]
    }
}
