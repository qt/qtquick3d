/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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
import QtQuick.Timeline
import QtQuick3D
import HelloExample

Window {
    id: window
    width: 1280
    height: 720
    visible: true

    View3D {
        id: view
        anchors.fill: parent

        environment: SceneEnvironment {
            clearColor: "black"
            backgroundMode: SceneEnvironment.Color
        }

        Node {
            id: cameraNode

            PerspectiveCamera {
                id: camera
                position: Qt.vector3d(0, 0, 650)
            }

            property bool isRotating: false
            PropertyAnimation on eulerRotation.x {
                from: 0
                to: 360
                duration: 20000
                loops: Animation.Infinite
                running: cameraNode.isRotating
            }

            Timeline {
                id: timeline0
                startFrame: 0
                endFrame: 1000
                currentFrame: 0
                enabled: true

                SequentialAnimation on currentFrame {
                    NumberAnimation {
                        from: 0
                        to: 500
                        duration: 4000
                    }
                    NumberAnimation {
                        from: 500
                        to: 1000
                        duration: 10000
                        loops: Animation.Infinite
                    }
                }

                KeyframeGroup {
                    target: cameraNode
                    property: "isRotating"

                    Keyframe {
                        frame: 0
                        value: false
                    }
                    Keyframe {
                        frame: 500
                        value: true
                    }
                }

                KeyframeGroup {
                    target: oldLogo
                    property: "isRotating"

                    Keyframe {
                        frame: 0
                        value: false
                    }
                    Keyframe {
                        frame: 100
                        value: true
                    }
                }

                KeyframeGroup {
                    target: camera
                    property: "z"

                    Keyframe {
                        frame: 0
                        value: 2000
                    }
                    Keyframe {
                        frame: 100
                        value: 2000
                    }
                    Keyframe {
                        frame: 400
                        value: 100
                    }
                    Keyframe {
                        frame: 500
                        value: 200
                    }
                    Keyframe {
                        frame: 550
                        value: 200
                    }
                    Keyframe {
                        frame: 700
                        value: 1000
                        easing.type: Easing.OutElastic
                    }
                    Keyframe {
                        frame: 750
                        value: 1000
                    }
                    Keyframe {
                        frame: 800
                        value: 1000
                    }
                    Keyframe {
                        frame: 950
                        value: 200
                        easing.type: Easing.OutElastic
                    }
                    Keyframe {
                        frame: 1000
                        value: 200
                    }

                }
            }
        }

        DirectionalLight {
            eulerRotation.x: 250
            eulerRotation.y: -30
            brightness: 1.0
            ambientColor: "#7f7f7f"
        }

        Model {
            id: oldLogo
            property real s: 4.75
            property real r: 0
            property bool isRotating: false
            scale: Qt.vector3d(s, s, s);
            source: "oldqtlogo.mesh"
            instancing: ImageInstanceTable {
                gridSize: 40
                gridSpacing: 20
                image: ":/qt_logo.png"
            }
            materials: DefaultMaterial { diffuseColor: "white" }
            NumberAnimation on r {
                running: oldLogo.isRotating
                from: 0
                to: 360
                duration: 3000
                loops: Animation.Infinite
            }
            onRChanged: rotate(2.5, Qt.vector3d(1, -1, 0), Node.LocalSpace)
        }
    }
}
