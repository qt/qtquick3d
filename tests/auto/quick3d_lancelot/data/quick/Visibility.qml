/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

import QtQuick3D 1.15
import QtQuick 2.15

Rectangle {
    width: 800
    height: 480
    color: Qt.rgba(1, 1, 1, 1)

    View3D {
        id: layer
        anchors.fill: parent
        environment: SceneEnvironment {
            clearColor: Qt.rgba(0, 0, 0, 1)
            aoDither: true
            depthPrePassEnabled: true
        }

        PerspectiveCamera {
            position: Qt.vector3d(0, 0, 600)
        }

        DirectionalLight {
            eulerRotation.x: -30
            eulerRotation.y: -70
        }

        Node {
            x: -300
            y: 250
            Text {
                text: "Text1 - visible: false"
                visible: false
                font.pixelSize: 40
            }
        }
        Node {
            x: 300
            y: 250
            Text {
                text: "Text2 - visible -> false"
                font.pixelSize: 40
                visible: true
                Component.onCompleted: {
                    visible = false;
                }
            }
        }
        Node {
            x: -300
            y: 150
            Text {
                text: "Text3 - visible -> true"
                font.pixelSize: 40
                visible: false
                Component.onCompleted: {
                    visible = true;
                }
            }
        }
        Node {
            x: 300
            y: 150
            Text {
                opacity: 0
                text: "Text4 - opacity: 0"
                font.pixelSize: 40
            }
        }
        Node {
            x: -300
            y: 50
            Text {
                NumberAnimation on opacity {
                    running: true
                    to: 0
                    duration: 500
                }
                text: "Text5 - opacity -> 0"
                font.pixelSize: 40
            }
        }
        Node {
            x: 300
            y: 50
            Text {
                NumberAnimation on opacity {
                    running: true
                    to: 1
                    duration: 500
                }
                text: "Text6 - opacity -> 1"
                opacity: 0
                font.pixelSize: 40
            }
        }
        Node {
            x: -300
            y: -50
            visible: false
            Text {
                text: "Text7 - Node visible: false"
                font.pixelSize: 40
            }
        }
        Node {
            x: 300
            y: -50
            visible: true
            Component.onCompleted: {
                visible = false;
            }
            Text {
                text: "Text8 - Node visible -> false"
                font.pixelSize: 40
            }
        }
        Node {
            x: -300
            y: -150
            visible: false
            Component.onCompleted: {
                visible = true;
            }
            Text {
                text: "Text9 - Node visible -> true"
                font.pixelSize: 40
            }
        }
        Node {
            x: 300
            y: -150
            opacity: 0
            Text {
                text: "Text10 - Node opacity: 0"
                font.pixelSize: 40
            }
        }
        Node {
            x: -300
            y: -250
            opacity: 1
            NumberAnimation on opacity {
                running: true
                to: 0
                duration: 500
            }
            Text {
                text: "Text11 - Node opacity -> 0"
                font.pixelSize: 40
            }
        }
        Node {
            x: 300
            y: -250
            opacity: 0
            NumberAnimation on opacity {
                running: true
                to: 1
                duration: 500
            }
            Text {
                text: "Text12 - Node opacity -> 1"
                font.pixelSize: 40
            }
        }
    }
}
