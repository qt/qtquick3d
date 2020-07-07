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

import QtQuick 2.15
import QtQuick.Window 2.14
import QtQuick3D 1.15
import QtQuick3D.Materials 1.15

Window {
    width: 1280
    height: 720
    visible: true
    title: "Custom Materials Example"

    View3D {
        width: parent.width / 2
        height: parent.height

        camera: camera

        environment: SceneEnvironment {
            clearColor: "#848895"
            backgroundMode: SceneEnvironment.Color
            probeBrightness: 1000
            lightProbe: Texture {
                source: "maps/OpenfootageNET_garage-1024.hdr"
            }
        }

        PerspectiveCamera {
            id: camera
            position: Qt.vector3d(0, 0, 600)
        }

//        WeirdShape {
//            customMaterial: CopperMaterial {}
//            position: Qt.vector3d(-150, -150, -100)
//        }

//        Model {
//            position: Qt.vector3d(100, 100, 100)
//            eulerRotation.x: 30
//            NumberAnimation on eulerRotation.y {
//                from: 0; to: 360; duration: 5000; loops: -1
//            }
//            source: "#Cube"
//            materials: [ PlasticStructuredRedMaterial {
//                    material_ior: 1.55
//                    bump_factor: 0.1
//                }
//            ]
//        }
    }
    Text {
        text: "Light probe based scene"
    }

    View3D {
        x: parent.width / 2
        width: parent.width / 2
        height: parent.height;

        camera: camera

        environment: SceneEnvironment {
            clearColor: "#444845"
            backgroundMode: SceneEnvironment.Color
        }

        DirectionalLight {
            position: Qt.vector3d(-500, 500, -100)
            color: Qt.rgba(0.2, 0.2, 0.2, 1.0)
            ambientColor: Qt.rgba(0.1, 0.1, 0.1, 1.0)
        }

        PointLight {
            position: Qt.vector3d(0, 500, 0)
            //rotation: Quaternion.fromEulerAngles(-135, -90, 0)
            color: Qt.rgba(0.1, 1.0, 0.1, 1.0)
            ambientColor: Qt.rgba(0.2, 0.2, 0.2, 1.0)
            brightness: 500
            castsShadow: true
            shadowMapQuality: Light.ShadowMapQualityHigh
        }

        Model {
            source: "#Rectangle"
            y: -200
            scale: Qt.vector3d(5, 5, 5)
            eulerRotation.x: -90
            materials: [
                CustomMaterial {
                    shadingMode: CustomMaterial.Shaded
                    vertexShader: "mat1.vert"
                    fragmentShader: "mat1.frag"
                    property color uDiffuseColorFactor: "white"
                }
            ]
        }

        WeirdShape {
            customMaterial: CustomMaterial {
                shadingMode: CustomMaterial.Shaded
                vertexShader: "mat1.vert"
                fragmentShader: "mat1.frag"
                property color uDiffuseColorFactor: "white"
            }
            position: Qt.vector3d(150, 150, -100)
        }

        Model {
            position: Qt.vector3d(-100, 0, -50)
            eulerRotation.x: 30
            NumberAnimation on eulerRotation.y {
                from: 0; to: 360; duration: 5000; loops: -1
            }
            scale: Qt.vector3d(1.5, 1.5, 1.5)
            source: "#Cylinder"
            materials: [
                CustomMaterial {
                    shadingMode: CustomMaterial.Shaded
                    vertexShader: "mat1.vert"
                    fragmentShader: "mat1.frag"
                    property color uDiffuseColorFactor: "yellow"
                }
            ]
        }
    }
    Text {
        text: "Light based scene"
        x: parent.width / 2
    }
}
