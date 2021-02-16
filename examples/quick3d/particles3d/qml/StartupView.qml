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
    id: mainView

    readonly property real listItemWidth: 300
    readonly property real listItemHeight: 40

    anchors.fill: parent

    ListModel {
        id: testsModel
        ListElement {
            name: "Snowing"
            file: "Snowing.qml"
        }
        ListElement {
            name: "HeartTrail"
            file: "HeartTrail.qml"
        }
        ListElement {
            name: "Colorful Particles"
            file: "ColorfulParticles.qml"
        }
        ListElement {
            name: "Emitter Shapes"
            file: "EmitterShapes.qml"
        }
        ListElement {
            name: "Attractor Shapes"
            file: "AttractorShapes.qml"
        }
        ListElement {
            name: "Fading In/Out"
            file: "FadingInOut.qml"
        }
        ListElement {
            name: "Emit And Burst"
            file: "EmitAndBurst.qml"
        }
        ListElement {
            name: "Qt Cube Burst"
            file: "QtLogoAnimation.qml"
        }
        ListElement {
            name: "ParticleSystem"
            file: "SystemPlayPause.qml"
        }
        ListElement {
            name: "Aligned Particles"
            file: "AlignedParticles.qml"
        }
        ListElement {
            name: "Fire And Smoke"
            file: "Fire.qml"
        }
        ListElement {
            name: "TrailEmitter Burst"
            file: "TrailEmitterBurst.qml"
        }
        ListElement {
            name: "Animated Sprite"
            file: "AnimatedSprite.qml"
        }
    }

    View3D {
        anchors.fill: parent

        environment: SceneEnvironment {
            clearColor: "#000000"
            backgroundMode: SceneEnvironment.Color
            antialiasingMode: SceneEnvironment.MSAA
            antialiasingQuality: SceneEnvironment.High
        }

        PerspectiveCamera {
            position: Qt.vector3d(0, 0, 600)
            clipFar: 2000
        }

        PointLight {
            position: Qt.vector3d(200, 200, 400)
            brightness: 50
            ambientColor: Qt.rgba(0.5, 0.3, 0.1, 1.0)
            SequentialAnimation on brightness {
                loops: Animation.Infinite
                NumberAnimation {
                    to: 400
                    duration: 2000
                    easing.type: Easing.OutElastic
                }
                NumberAnimation {
                    to: 50
                    duration: 6000
                    easing.type: Easing.InOutQuad
                }
            }
        }

        // Qt Cube model
        Model {
            source: "#Cube"
            position: Qt.vector3d(-250, 150, 100)
            scale: Qt.vector3d(1.0, 1.0, 1.0)
            NumberAnimation on eulerRotation.y {
                loops: Animation.Infinite
                from: 0
                to: 360
                duration: 10000
            }
            NumberAnimation on eulerRotation.x {
                loops: Animation.Infinite
                from: 0
                to: 360
                duration: 6000
            }
            materials: PrincipledMaterial {
                baseColorMap: Texture {
                    source: "images/qt_logo2.png"
                }
                normalMap: Texture {
                    source: "images/qt_logo2_n.png"
                }
            }
        }

        ParticleSystem3D {
            id: psystem
            startTime: 10000
            SpriteParticle3D {
                id: spriteParticle
                sprite: Texture {
                    source: "images/dot.png"
                }
                maxAmount: 200
                color: "#80ff7000"
                colorVariation: Qt.vector4d(0.6, 0.2, 0.0, 0.4)
                unifiedColorVariation: true
                fadeInDuration: 1000
                fadeOutDuration: 3000
            }
            ParticleEmitter3D {
                particle: spriteParticle
                emitRate: 20
                lifeSpan: 10000
                scale: Qt.vector3d(8, 8, 0)
                shape: ParticleShape3D {
                    type: ParticleShape3D.Cube
                }
                particleScale: 2.4
                particleEndScale: 0.2
                particleScaleVariation: 1.8
                velocity: TargetDirection3D {
                    magnitudeVariation: magnitude
                    positionVariation: Qt.vector3d(180, 180, 180)
                    SequentialAnimation on magnitude {
                        loops: Animation.Infinite
                        NumberAnimation {
                            to: 1.0
                            duration: 3000
                            easing.type: Easing.InOutQuad
                        }
                        NumberAnimation {
                            to: 0.1
                            duration: 5000
                            easing.type: Easing.InOutQuad
                        }
                    }
                }
            }
        }
    }

    Component {
        id: listComponent
        Item {
            width: mainView.listItemWidth
            height: mainView.listItemHeight
            Rectangle {
                anchors.fill: parent
                border.width: 0.5
                border.color: "#d0808080"
                color: "#d0404040"
                opacity: mouseArea.containsMouse ? 1.0 : 0.5
            }
            Text {
                anchors.centerIn: parent
                color: "#f0f0f0"
                text: name
            }
            MouseArea {
                id: mouseArea
                anchors.fill: parent
                onClicked: {
                    loader.source = file
                }
                hoverEnabled: true
            }
        }
    }

    Text {
        anchors.horizontalCenter: examplesListView.horizontalCenter
        anchors.bottom: examplesListView.top
        anchors.bottomMargin: 20
        text: qsTr("Qt Quick 3D - Particles3D")
        color: "#f0f0f0"
        font.pointSize: 18
    }

    ListView {
        id: examplesListView
        anchors.centerIn: parent
        width: mainView.listItemWidth
        height: count * mainView.listItemHeight
        model: testsModel
        delegate: listComponent
    }
}
