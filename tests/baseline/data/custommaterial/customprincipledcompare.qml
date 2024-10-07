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

import QtQuick3D
import QtQuick

Item {
    width: 400
    height: 400

    View3D {
        id: v3d
        anchors.fill: parent
        renderMode: View3D.Overlay

        environment: SceneEnvironment {
            clearColor: "#444845"
            backgroundMode: SceneEnvironment.Color
        }

        camera: camera

        PerspectiveCamera {
            id: camera
            position: Qt.vector3d(0, 0, 600)
        }

        DirectionalLight {
            position: Qt.vector3d(-500, 500, -100)
            color: Qt.rgba(0.2, 0.2, 0.2, 1.0)
            ambientColor: Qt.rgba(0.1, 0.1, 0.1, 1.0)
        }

        Model {
            source: "#Sphere"
            scale: Qt.vector3d(2, 2, 2)
            y: 200
            x: -100
            materials: PrincipledMaterial {
                baseColor: "#41cd52"
                metalness: 0.0
                roughness: 0.2
                specularAmount: 0.5
            }
        }

        Model {
            source: "#Sphere"
            scale: Qt.vector3d(2, 2, 2)
            y: 200
            x: 100
            materials: CustomMaterial {
                property color uBaseColor: "#41cd52"
                property real uMetalness: 0.0
                property real uRoughness: 0.2
                property real uSpecularAmount: 0.5
                fragmentShader: "customprincipledcompare.frag"
            }
        }

        Model {
            source: "#Sphere"
            scale: Qt.vector3d(2, 2, 2)
            x: -100
            materials: PrincipledMaterial {
                baseColor: "#41cd52"
                metalness: 0.3
                roughness: 0.2
                specularAmount: 0.0
            }
        }

        Model {
            source: "#Sphere"
            scale: Qt.vector3d(2, 2, 2)
            x: 100
            materials: CustomMaterial {
                property color uBaseColor: "#41cd52"
                property real uMetalness: 0.3
                property real uRoughness: 0.2
                property real uSpecularAmount: 0.0
                fragmentShader: "customprincipledcompare.frag"
            }
        }

        // Here the color is passed in as a non-color property (would be the
        // same if the shader hardcoded the value), so now it is up to the
        // shader to linearize.
        Model {
            source: "#Sphere"
            scale: Qt.vector3d(2, 2, 2)
            y: -200
            materials: CustomMaterial {
                property vector4d uBaseColorSrgb: Qt.vector4d(0.2549, 0.80392, 0.32157, 1.0)
                property real uMetalness: 0.3
                property real uRoughness: 0.2
                property real uSpecularAmount: 0.0
                fragmentShader: "customprincipledcompare2.frag"
            }
        }
    }
}
