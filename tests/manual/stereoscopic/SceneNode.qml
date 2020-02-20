/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

import QtQuick 2.11
import QtQuick3D 1.15

Node {
    id: sceneRoot

    property alias mainCamera: mainCamera

    DirectionalLight {
        brightness: 40
        color: "#f0e0a0"
    }
    DirectionalLight {
        brightness: 40
        position: Qt.vector3d(-200,-200,-500)
        eulerRotation: Qt.vector3d(20,-20,20)
        color: "#f0e0a0"
    }

    PerspectiveCamera {
        id: mainCamera
        x: 0
        z: 600
        SequentialAnimation on position.y {
            loops: Animation.Infinite
            NumberAnimation {
                duration: 6000
                easing.type: Easing.InOutQuad
                to: -80
            }
            NumberAnimation {
                duration: 3000
                easing.type: Easing.InOutQuad
                to: 80
            }
        }
    }

    Model {
        id: cube
        source: "#Cube"
        materials: DefaultMaterial {
        }

        NumberAnimation {
            target: cube
            property: "eulerRotation.x"
            duration: 3000
            easing.type: Easing.InOutQuad
            loops: Animation.Infinite
            running: true
            from: 0
            to: 360
        }

        SequentialAnimation on position.z {
            running: true
            loops: Animation.Infinite
            NumberAnimation {
                duration: 3000
                easing.type: Easing.InOutQuad
                to: 300
            }
            NumberAnimation {
                duration: 6000
                easing.type: Easing.InOutQuad
                to: 0
            }
            PauseAnimation {
                duration: 3000
            }
        }
    }

    Model {
        id: cone
        source: "#Cone"
        z: -250
        x: -150
        materials: DefaultMaterial {
        }
        SequentialAnimation on x {
            running: true
            loops: Animation.Infinite

            NumberAnimation {
                duration: 5000
                easing.type: Easing.InOutQuad
                to: 250
            }
            NumberAnimation {
                duration: 5000
                easing.type: Easing.InOutQuad
                to: -250
            }
        }
    }

    Model {
        id: cylinder
        source: "#Cylinder"
        z: 250
        x: 150
        materials: DefaultMaterial {
        }

        SequentialAnimation on x {
            running: true
            loops: Animation.Infinite

            NumberAnimation {
                duration: 5000
                easing.type: Easing.InOutQuad
                to: -150
            }
            NumberAnimation {
                duration: 5000
                easing.type: Easing.InOutQuad
                to: 150
            }
        }
    }
    Node {
        y: 260
        z: -100
        Text {
            color: "white"
            font.pixelSize: 36
            font.bold: true
            text: "Press 0..4 to change the stereoscopic mode"
        }
    }
    Node {
        y: -260
        z: -100
        Text {
            color: "white"
            font.pixelSize: 36
            font.bold: true
            text: "Press -/+ to change the eye separation"
        }
    }
}
