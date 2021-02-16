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

Window {
    id: mainWindow

    property real listItemWidth: 300
    property real listItemHeight: 40

    width: 1280
    height: 720
    visible: true
    title: qsTr("Particles Testbed")
    color: "#404040"

    ListModel {
        id: testsModel
        ListElement {
            group: "ParticleSystem"
            name: "Snowing (Wander, PointRotator)"
            file: "Snowing.qml"
        }
        ListElement {
            group: "ParticleSystem"
            name: "ColorfulParticles (ModelParticle)"
            file: "ColorfulParticles.qml"
        }
        ListElement {
            group: "ParticleSystem"
            name: "EmitterShapes"
            file: "EmitterShapes.qml"
        }
        ListElement {
            group: "ParticleSystem"
            name: "AttractorShapes"
            file: "AttractorShapes.qml"
        }
        ListElement {
            group: "ParticleSystem"
            name: "FadingInOut"
            file: "FadingInOut.qml"
        }
        ListElement {
            group: "ParticleSystem"
            name: "EmitAndBurst"
            file: "EmitAndBurst.qml"
        }
        ListElement {
            group: "ParticleSystem"
            name: "Qt Logo Animation (ParticleSystem time, burst)"
            file: "QtLogoAnimation.qml"
        }
        ListElement {
            group: "ParticleSystem"
            name: "Qt Logo Animation 2 (Timeline, EmitBursts)"
            file: "QtLogoAnimation2.qml"
        }
        ListElement {
            group: "ParticleSystem"
            name: "ParticleSystem (pause, stop)"
            file: "SystemPlayPause.qml"
        }
        ListElement {
            group: "ParticleSystem"
            name: "AlignedParticles"
            file: "AlignedParticles.qml"
        }
        ListElement {
            group: "ParticleSystem"
            name: "Fire (colorTable)"
            file: "Fire.qml"
        }
        ListElement {
            group: "ParticleSystem"
            name: "TrailEmitterBurst"
            file: "TrailEmitterBurst.qml"
        }
        ListElement {
            group: "ParticleSystem"
            name: "HeartTrail"
            file: "HeartTrail.qml"
        }
    }

    Component {
        id: listComponent
        Item {
            width: listItemWidth
            height: listItemHeight
            Rectangle {
                anchors.fill: parent
                border.width: 1
                border.color: "#202020"
                Text {
                    anchors.centerIn: parent
                    text: name
                }
            }
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    loader.source = file
                }
            }
        }
    }

    Component {
        id: sectionHeadingComponent
        Item {
            required property string section
            width: listItemWidth
            height: listItemHeight
            Rectangle {
                anchors.fill: parent
                anchors.topMargin: 10
                border.width: 1
                color: "#D0D0D0"
                border.color: "#202020"
                Text {
                    anchors.centerIn: parent
                    text: parent.parent.section
                }
            }
        }
    }

    ListView {
        anchors.centerIn: parent
        width: listItemWidth
        height: count * listItemHeight
        visible: loader.source == ""
        model: testsModel
        delegate: listComponent
        section.property: "group"
        section.criteria: ViewSection.FullString
        section.delegate: sectionHeadingComponent
    }

    Loader {
        anchors.fill: parent
        id: loader
    }

    Image {
        anchors.left: parent.left
        anchors.leftMargin: width
        anchors.top: parent.top
        anchors.topMargin: width
        width: parent.width * 0.02
        height: width
        source: "images/arrow_icon.png"
        visible: loader.source != ""
        MouseArea {
            anchors.fill: parent
            anchors.margins: -20
            onClicked: {
                loader.source = "";
            }
        }
    }
}
