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

import QtQuick 2.12
import QtQuick.Window 2.11
import QtQuick3D 1.0
import QtQuick3D.MaterialLibrary 1.0
import QtQuick.Controls 2.5
import QtQuick3D.Helpers 1.0

Window {
    id: window
    width: 1280
    height: 720
    visible: true

    WasdController {
        id: wasdController
        controlledObject: camera
    }

    MaterialControl
    {
        id: materialCtrl
        anchors.top: parent.top
        width: 100
        height: 100
    }

    View3D {
        id: layer1
        anchors.fill: parent
        camera: camera
        renderMode: View3D.Underlay

        // Light always points the same direction as camera
        Light {
            id: directionalLight
            lightType: Light.Directional
            rotation: Qt.vector3d(0, 100, 0)
            brightness: 100
            SequentialAnimation on rotation {
                loops: Animation.Infinite
                PropertyAnimation { duration: 5000; to: Qt.vector3d(0, 360, 0); from: Qt.vector3d(0, 0, 0) }
            }
        }

        environment: SceneEnvironment {
            probeBrightness: 200
            clearColor: "gray"

            backgroundMode: SceneEnvironment.Color
            lightProbe: Texture {
                source: "maps/OpenfootageNET_garage-1024.hdr"
            }
        }


        Camera {
            id: camera
            position: Qt.vector3d(0, 0, -600)
        }

        Model {
            position: Qt.vector3d(-300, 0, 0)
            scale: Qt.vector3d(3, 3, 3)
            source: "#Sphere"
            materials: [ PrincipledMaterial {
                    baseColor: "#00ff00"
                    metalness: materialCtrl.metalness
                    roughness: materialCtrl.roughness
                    specularAmount: materialCtrl.specular
                    indexOfRefraction: materialCtrl.ior
                    specularTint: materialCtrl.specularTint
                    opacity: materialCtrl.opacityValue
                }
            ]
        }


        Model {
            position: Qt.vector3d(300, 0, 0)
            scale: Qt.vector3d(3, 3, 3)
            source: "#Sphere"
            materials: [ DefaultMaterial {
                    diffuseColor: "#ffff00"
                    specularRoughness: 0.0
                    indexOfRefraction: 1.450
                    specularAmount: 1.0
                    fresnelPower: 1.0
                }
            ]
        }

        Model {
            position: Qt.vector3d(0, 0, 0)
            scale: Qt.vector3d(4, 4, 4)
            source: "#Sphere"
            materials: [ PrincipledMaterial {
                    metalness: materialCtrl.metalness
                    roughness: materialCtrl.roughness
                    specularAmount: materialCtrl.specular
                    indexOfRefraction: materialCtrl.ior
                    opacity: materialCtrl.opacityValue
                    baseColorMap: Texture { source: "maps/metallic/basecolor.jpg" }
                    metalnessMap: Texture { source: "maps/metallic/metallic.jpg" }
                    roughnessMap: Texture { source: "maps/metallic/roughness.jpg" }
                    normalMap: Texture { source: "maps/metallic/normal.jpg" }
                }
            ]
        }
    }
}
