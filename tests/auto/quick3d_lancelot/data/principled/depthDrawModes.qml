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

import QtQuick
import QtQuick3D

Rectangle {
    width: 700
    height: 400
    color: "lightgray"

    Rectangle {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 200
        color: "green"
    }

    View3D {
        anchors.fill: parent

        Texture {
            id: diffuseMap
            source: "../shared/maps/texture_withAlpha.png"
            generateMipmaps: true
            mipFilter: Texture.Linear
        }

        environment: SceneEnvironment {
            depthPrePassEnabled: false
        }

        Node {

            Model {
                id: car1
                x: -200
                y: 100
                scale: Qt.vector3d(50, 50, 50)
                eulerRotation.y: 45
                source: "../shared/models/carCombined.mesh"
                materials: PrincipledMaterial {
                    baseColorMap: diffuseMap
                    opacityChannel: Material.A
                    roughness: 0.240909
                    alphaMode: PrincipledMaterial.Blend
                    depthDrawMode: Material.OpaqueOnlyDepthDraw
                }
            }
            Model {
                id: car2
                x: 200
                y: 100
                scale: Qt.vector3d(50, 50, 50)
                eulerRotation.y: 45
                source: "../shared/models/carCombined.mesh"
                materials: PrincipledMaterial {
                    baseColorMap: diffuseMap
                    opacityChannel: Material.A
                    roughness: 0.240909
                    alphaMode: PrincipledMaterial.Blend
                    depthDrawMode: Material.AlwaysDepthDraw
                }
            }
            Model {
                id: car3
                x: -200
                y: -100
                scale: Qt.vector3d(50, 50, 50)
                eulerRotation.y: 45
                source: "../shared/models/carCombined.mesh"
                materials: PrincipledMaterial {
                    baseColorMap: diffuseMap
                    opacityChannel: Material.A
                    roughness: 0.240909
                    alphaMode: PrincipledMaterial.Blend
                    depthDrawMode: Material.NeverDepthDraw
                }
            }
            Model {
                id: car4
                x: 200
                y: -100
                scale: Qt.vector3d(50, 50, 50)
                eulerRotation.y: 45
                source: "../shared/models/carCombined.mesh"
                materials: PrincipledMaterial {
                    baseColorMap: diffuseMap
                    opacityChannel: Material.A
                    roughness: 0.240909
                    alphaMode: PrincipledMaterial.Blend
                    depthDrawMode: Material.OpaquePrePassDepthDraw
                }
            }
            Model {
                id: cube1
                scale: Qt.vector3d(2, 2, 2)
                x: 600
                y: 200
                source: "#Cube"
                materials: PrincipledMaterial {
                    baseColorMap: diffuseMap
                    opacityChannel: Material.A
                    alphaMode: PrincipledMaterial.Blend
                    depthDrawMode: Material.AlwaysDepthDraw
                }
            }
            Model {
                id: cube2
                scale: Qt.vector3d(2, 2, 2)
                x: 600
                y: -200
                source: "#Cube"
                materials: PrincipledMaterial {
                    baseColorMap: diffuseMap
                    opacityChannel: Material.A
                    alphaMode: PrincipledMaterial.Blend
                    depthDrawMode: Material.OpaquePrePassDepthDraw
                }
            }
        }

        DirectionalLight {
            eulerRotation.x: -20
        }

        PerspectiveCamera {
            id: camera1
            z: 600
            y: 250
            x: 200
            eulerRotation.x: -20
        }
    }
}
