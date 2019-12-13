/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

import QtQuick3D 1.14
import QtQuick 2.14
import QtQuick.Timeline 1.0
import "../components"

Rectangle {
    id: qmlstreamlayer
    width: 1200
    height: 800
    color: Qt.rgba(0, 0, 0, 1)

    Layer {
        id: qmlstream
        anchors.left: parent.left
        anchors.leftMargin: parent.width * 0
        width: 512
        anchors.top: parent.top
        anchors.topMargin: 288
        height: 512
    }

    View3D {
        id: layer
        anchors.left: parent.left
        anchors.leftMargin: parent.width * 0
        width: parent.width * 1
        anchors.top: parent.top
        anchors.topMargin: parent.height * 0
        height: parent.height * 1
        environment: SceneEnvironment {
            clearColor: Qt.rgba(0, 0, 0, 1)
            aoDither: true
            depthPrePassEnabled: true
        }

        Camera {
            id: camera
            position: Qt.vector3d(0, 0, -600)
            rotationOrder: Node.YZX
            clipFar: 5000
        }

        DirectionalLight {
            id: light
            rotationOrder: Node.YZX
            shadowFactor: 10
        }

        Model {
            id: cube
            position: Qt.vector3d(-7.21698, 4.33, 0)
            rotation: Qt.vector3d(-7, -72, 127)
            rotationOrder: Node.YZX
            source: "#Cube"
            edgeTess: 4
            innerTess: 4

            DefaultMaterial {
                id: material
                lighting: DefaultMaterial.FragmentLighting
                diffuseMap: material_diffusemap
                indexOfRefraction: 1.5
                specularAmount: 0
                specularRoughness: 0
                bumpAmount: 0.5
                translucentFalloff: 1
                displacementAmount: 20

                Texture {
                    id: material_diffusemap
                    sourceItem: Red_fill { }
                }
            }
            materials: [material]
        }
    }

    Timeline {
        id: rotating_CubeTimeline
        startFrame: 0
        endFrame: 2
        currentFrame: 0
        enabled: false
        animations: [
            TimelineAnimation {
                id: rotating_CubeTimelineAnimation
                duration: 2000
                from: 0
                to: 2
                running: true
                loops: -1
                pingPong: true
            }
        ]
    }

    states: [
        State {
            name: "Rotating_Cube"
            PropertyChanges {
                target: rotating_CubeTimeline
                enabled: true
                currentFrame: 0
            }
            PropertyChanges {
                target: rotating_CubeTimelineAnimation
                running: true
            }
        }
    ]
    state: "Rotating_Cube"
}
