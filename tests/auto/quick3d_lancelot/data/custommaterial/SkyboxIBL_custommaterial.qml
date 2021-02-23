/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
import QtQuick3D

Rectangle {
    width: 1024
    height: 600
    color: "black"

    View3D {
        anchors.fill: parent
        camera: camera

        DirectionalLight {
            eulerRotation.y: -100
            brightness: 1
        }

        environment: SceneEnvironment {
            clearColor: "black"
            backgroundMode: SceneEnvironment.SkyBox
            lightProbe: Texture {
                source: "../lighting/maps/OpenfootageNET_garage-1024.hdr"
            }
            probeOrientation: Qt.vector3d(0, -90, 0)
        }

        PerspectiveCamera {
            id: camera
            position: Qt.vector3d(0, 0, 600)
        }

        Model {
            position: Qt.vector3d(-250, -30, 0)
            scale: Qt.vector3d(4, 4, 4)
            source: "#Sphere"
            materials: [ CustomMaterial {
                    fragmentShader: "custommaterial_ibl.frag"
                }
            ]
        }

        Model {
            position: Qt.vector3d(250, -30, 0)
            scale: Qt.vector3d(4, 4, 4)
            source: "#Sphere"
            materials: [ CustomMaterial {
                    fragmentShader: "custommaterial_ibl2.frag"

                    property TextureInput baseColorMap: TextureInput {
                        enabled: true
                        texture: Texture {
                            source: "../lighting/maps/metallic/basecolor.jpg"
                            generateMipmaps: true
                            mipFilter: Texture.Linear
                        }
                    }
                    property TextureInput metalnessMap: TextureInput {
                        enabled: true
                        texture: Texture {
                            source: "../lighting/maps/metallic/metallic.jpg"
                            generateMipmaps: true
                            mipFilter: Texture.Linear
                        }
                    }
                    property TextureInput roughnessMap: TextureInput {
                        enabled: true
                        texture: Texture {
                            source: "../lighting/maps/metallic/roughness.jpg"
                            generateMipmaps: true
                            mipFilter: Texture.Linear
                        }
                    }
                    property TextureInput normalMap: TextureInput {
                        enabled: true
                        texture: Texture {
                            source: "../lighting/maps/metallic/normal.jpg"
                            generateMipmaps: true
                            mipFilter: Texture.Linear
                        }
                    }
                }
            ]
        }
    }
}
