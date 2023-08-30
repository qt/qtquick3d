/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

Rectangle {
    id: pointlight
    width: 800
    height: 480
    color: Qt.rgba(0, 0, 0, 1)

    View3D {
        anchors.fill: parent

        PerspectiveCamera {
            id: camera_u2473
            position: Qt.vector3d(-75, 75, -75)
            eulerRotation: Qt.vector3d(-35, -135, 0)
            clipFar: 5000
        }

        Texture {
            id: diffuseTexture
            source: "../shared/maps/texture_withAlpha.png"
        }

        Node {
            id: group

            Model {
                id: cube
                property real rotationy: 0.0
                position: Qt.vector3d(-19.6299, 18.21, -9.59015)
                scale: Qt.vector3d(0.25, 0.25, 0.25)
                eulerRotation: Qt.vector3d(0, rotationy, 0)
                source: "#Cube"

                materials: PrincipledMaterial {
                    baseColorMap: diffuseTexture
                    depthDrawMode: Material.OpaquePrePassDepthDraw
                    cullMode: Material.NoCulling
                }
            }

            Model {
                id: car_u46272
                x: 35
                eulerRotation: Qt.vector3d(0, -180, 0)
                scale: Qt.vector3d(10, 10, 10)
                source: "../shared/models/carCombined.mesh"
                materials: PrincipledMaterial {
                    baseColorMap: diffuseTexture
                    depthDrawMode: Material.OpaquePrePassDepthDraw
                }
            }

            Model {
                id: ground_u62818
                eulerRotation: Qt.vector3d(-90, 0, 0)
                scale: Qt.vector3d(7, 7, 1)
                source: "#Rectangle"
                castsShadows: false
                receivesShadows: true
                materials: PrincipledMaterial {
                    baseColor: "#ff717171"
                }
            }
        }

        PointLight {
            id: light_u23253
            x: -23.3305
            y: 11.4227
            z: -8.09776
            color: "#ff38ffa6"
            ambientColor: "#ff191919"
            brightness: 1
            quadraticFade: 0
            castsShadow: true
            shadowBias: -0.1
            shadowFactor: 50
            shadowMapQuality: Light.ShadowMapQualityVeryHigh
            shadowMapFar: 200
            shadowFilter: 0
        }
    }
}
