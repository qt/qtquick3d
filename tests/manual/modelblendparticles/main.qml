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
import ModelBlendParticlesExample

Window {
    id: mainWindow

    width: 1280
    height: 720
    visible: true
    title: qsTr("Test")
    color: "#000000"

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
                Keyframe { frame: 100; value: -180 }
            }
        ]
    }

    View3D {
        anchors.fill: parent
        camera: camera

        environment: SceneEnvironment {
            clearColor: "#c0c0c0"
            backgroundMode: SceneEnvironment.SkyBox
        }

        Node {
            id: cameraRotation
            property real rot: 0.0
            eulerRotation: Qt.vector3d(0, rot, 0)
            PerspectiveCamera {
                id: camera
                position: Qt.vector3d(0, 0, 150)
            }
        }

        DirectionalLight {
            brightness: 100
        }
        DirectionalLight {
            eulerRotation: Qt.vector3d(180, 0, 0)
            brightness: 100
        }
        PointLight {
            y: 300
            brightness: 200
        }

        Model {
            source: "#Rectangle"
            eulerRotation: Qt.vector3d(-90, 0, 0)
            y: -50
            scale: Qt.vector3d(10, 10, 1)
            materials: [
                DefaultMaterial {
                    diffuseColor: "green"
                }
            ]
        }

        ParticleSystem3D {
            id: psystem
            running: false
            x: -100

            Component {
                id: modelComponent
                Model {
                    geometry: TestGeometry {}
                    scale: Qt.vector3d(10, 10, 10)

                    materials: [
                        PrincipledMaterial {
                            baseColor: "#41cd52"
                            metalness: 1
                            roughness: 0.1
                            specularAmount: 0
                            cullMode: cullingModelBox.checked ? Material.BackFaceCulling : Material.NoCulling
                        }
                    ]
                }
            }

            Node {
                id: translateNode
                x: 100
            }

            ModelBlendParticle3D {
                id: particle
                delegate: modelComponent
                endNode: translateNode
                modelBlendMode: blendModeSelectionBox.selection
                endTime: 2500
            }

            ParticleEmitter3D {
                id: emitter
                system: psystem
                particle: particle
                lifeSpan: particle.modelBlendMode === ModelBlendParticle3D.Explode ? 40000 : 4000
                emitRate: particle.maxAmount / 10
                velocity: VectorDirection3D {
                    direction: Qt.vector3d(40, 10, 0)
                    directionVariation: Qt.vector3d(0, 10, 10)
                }
                particleRotationVelocity: Qt.vector3d(30, 0, 10)
                particleRotationVelocityVariation: Qt.vector3d(10, 0, 5)
                particleScale: 1.0
                particleEndScale: 1.0
            }
        }
    }

    SettingsView {
        CustomCheckBox {
            id: cullingModelBox
            text: "Enable culling"
            checked: false
        }
        CustomSelectionBox {
            id: blendModeSelectionBox
            text: "Mode"
            values: ["Explode", "Construct", "Transfer"]
            onSelectionChanged: {
                timeline.currentFrame = 0
                psystem.reset()
            }
        }
    }

    Item {
        id: toolbar
        anchors.left: parent.left
        anchors.leftMargin: 20
        anchors.right: parent.right
        anchors.rightMargin: 20
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 80
        height: 60
        Rectangle {
            anchors.fill: parent
            color: "#ffffff"
            radius: 4
            opacity: 0.2
        }
        Image {
            id: playButton
            anchors.left: parent.left
            anchors.leftMargin: 14
            anchors.verticalCenter: parent.verticalCenter
            height: parent.height - 10
            width: height
            source: timelineAnimation.running ? "images/icon_pause.png" : "images/icon_play.png"
            MouseArea {
                anchors.fill: parent
                anchors.margins: -10
                onClicked: {
                    // If we are close to end, start from the beginning
                    if (timeline.currentFrame >= timeline.endFrame - 1.0)
                        timeline.currentFrame = 0;

                    timelineAnimation.running = !timelineAnimation.running;
                }
            }
        }

        CustomSlider {
            id: sliderTimelineTime
            anchors.left: playButton.right
            anchors.leftMargin: 8
            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            sliderValue: timeline.currentFrame
            sliderEnabled: !timelineAnimation.running || timelineAnimation.paused
            fromValue: 0.0
            toValue: 100.0
            onSliderValueChanged: timeline.currentFrame = sliderValue;
        }
    }
}
