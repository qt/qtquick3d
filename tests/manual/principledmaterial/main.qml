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
import QtQuick.Window 2.15
import QtQuick3D 1.15

Window {
    id: window
    width: 1280
    height: 720
    visible: true
    title: "Principled Materials Example"
    color: "#848895"

    MaterialControl {
        id: materialCtrl
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
    }

    View3D {
        anchors.fill: parent
        camera: camera
        renderMode: View3D.Underlay

        //! [rotating light]
        // Rotate the light direction
        DirectionalLight {
            eulerRotation.y: -100
            brightness: 100
            SequentialAnimation on eulerRotation.y {
                loops: Animation.Infinite
                PropertyAnimation {
                    duration: 5000
                    to: 360
                    from: 0
                }
            }
        }        //! [rotating light]

        //! [environment]
        environment: SceneEnvironment {
            probeBrightness: 250
            clearColor: window.color

            backgroundMode: SceneEnvironment.Color
            lightProbe: Texture {
                source: "maps/OpenfootageNET_garage-1024.hdr"
            }
        }
        //! [environment]

        PerspectiveCamera {
            id: camera
            position: Qt.vector3d(0, 0, 600)
        }

        //! [basic principled]
        Model {
            position: Qt.vector3d(-250, -30, 0)
            scale: Qt.vector3d(4, 4, 4)
            source: "#Sphere"
            materials: [ PrincipledMaterial {
                    baseColor: "#41cd52"
                    metalness: materialCtrl.metalness
                    roughness: materialCtrl.roughness
                    specularAmount: materialCtrl.specular
                    indexOfRefraction: materialCtrl.ior
                    specularTint: materialCtrl.specularTint
                    opacity: materialCtrl.opacityValue
                }
            ]
        }
        //! [basic principled]

        //! [textured principled]
        Model {
            position: Qt.vector3d(250, -30, 0)
            scale: Qt.vector3d(4, 4, 4)
            source: "#Sphere"
            materials: [ PrincipledMaterial {
                    metalness: materialCtrl.metalness
                    roughness: materialCtrl.roughness
                    specularAmount: materialCtrl.specular
                    indexOfRefraction: materialCtrl.ior
                    opacity: materialCtrl.opacityValue
                    Texture {
                        id: basemetal
                        source: "maps/metallic/basemetal.astc"
                    }
                    Texture {
                        id: normalrough
                        source: "maps/metallic/normalrough.astc"
                    }
                    baseColorMap: basemetal
                    metalnessMap: basemetal
                    roughnessMap: normalrough
                    normalMap: normalrough
                    metalnessChannel: Material.A
                    roughnessChannel: Material.A
                }
            ]
            //! [textured principled]

            SequentialAnimation on eulerRotation {
                loops: Animation.Infinite
                PropertyAnimation {
                    duration: 5000
                    from: Qt.vector3d(0, 0, 0)
                    to: Qt.vector3d(360, 360, 360)
                }
            }
        }
    }
}
