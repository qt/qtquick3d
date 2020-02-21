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

import QtQuick 2.14
import QtQuick3D 1.15

Rectangle {
    width: 400
    height: 400
    color: "lightgray"

    View3D {
        anchors.fill: parent
        camera: camera
        renderMode: View3D.Offscreen

        OrthographicCamera {
            id: camera
            z: 500
        }

        DirectionalLight {
            id: dirLightX
            ambientColor: Qt.rgba(0.5, 0.5, 0.5, 1.0);
        }

        Texture {
            id: tex_rgba
            source: "../shared/maps/rgba.png"
        }

        Texture {
            id: tex_photo
            source: "../shared/maps/oulu_2.jpeg"
        }

        Model {
            source: "#Rectangle"
            materials: [ DefaultMaterial {
                    lighting: DefaultMaterial.NoLighting
                    diffuseMap: Texture {
                        source: "../shared/maps/checkerboard_1.png"
                        tilingModeHorizontal: Texture.Repeat
                        tilingModeVertical: Texture.Repeat
                        scaleU: 8
                        scaleV: 8
                    }
                } ]
            z: -500
            scale: Qt.vector3d(8, 12, 1)
        }

        Node {
            id: row1

            // Row 1
            Model {
                source: "#Sphere"
                scale: Qt.vector3d(0.8, 0.8, 0.8)
                position: Qt.vector3d(-125, 125, 0)
                materials: [ PrincipledMaterial {
                        baseColor: "#4080A0"
                        metalness: 0
                        roughness: .1
                        indexOfRefraction: .1
                        specularAmount: 0
                    } ]
            }

            Model {
                source: "#Sphere"
                scale: Qt.vector3d(0.8, 0.8, 0.8)
                position: Qt.vector3d(0, 125, 0)
                materials: [ PrincipledMaterial {
                        baseColor: "#4080A0"
                        metalness: 0
                        roughness: .1
                        indexOfRefraction: .1
                        specularAmount: .3
                    } ]
            }

            Model {
                source: "#Sphere"
                scale: Qt.vector3d(0.8, 0.8, 0.8)
                position: Qt.vector3d(125, 125, 0)
                materials: [ PrincipledMaterial {
                        baseColor: "#4080A0"
                        metalness: 0
                        roughness: .1
                        indexOfRefraction: .1
                        specularAmount: 1
                    } ]
            }
        }

        Node {
            id: row2

            // Row 2
            Model {
                source: "#Cube"
                scale: Qt.vector3d(0.8, 0.8, 0.8)
                position: Qt.vector3d(-125, 0, 0)
                eulerRotation.y: 350
                materials: [ PrincipledMaterial {
                        baseColor: "#4080A0"
                        metalness: 0
                        roughness: 1
                        indexOfRefraction: .1
                        specularAmount: 1
                        specularMap: tex_rgba
                    } ]
            }

            Model {
                source: "#Cube"
                scale: Qt.vector3d(0.8, 0.8, 0.8)
                position: Qt.vector3d(0, 0, 0)
                eulerRotation.y: 340
                materials: [ PrincipledMaterial {
                        baseColor: "#4080A0"
                        metalness: 0
                        roughness: 1
                        indexOfRefraction: .1
                        specularAmount: .5
                        specularMap: tex_rgba
                    } ]
            }

            Model {
                source: "#Cube"
                scale: Qt.vector3d(0.8, 0.8, 0.8)
                position: Qt.vector3d(125, 0, 0)
                eulerRotation.y: 320
                materials: [ PrincipledMaterial {
                        baseColor: "#4080A0"
                        metalness: 0
                        roughness: 1
                        indexOfRefraction: .1
                        specularAmount: 0
                        specularMap: tex_rgba
                    } ]
            }
        }

        // Row 3
        Model {
            source: "#Sphere"
            scale: Qt.vector3d(0.8, 0.8, 0.8)
            position: Qt.vector3d(-125, -125, 0)
            materials: [ PrincipledMaterial {
                    baseColor: "#4080A0"
                    metalness: 0
                    roughness: .1
                    specularAmount: 1
                } ]
        }

        Model {
            source: "#Sphere"
            scale: Qt.vector3d(0.8, 0.8, 0.8)
            position: Qt.vector3d(0, -125, 0)
            eulerRotation.y: 350
            materials: [ PrincipledMaterial {
                    baseColor: "#4080A0"
                    metalness: 0
                    roughness: 1
                    specularAmount: 1
                } ]
        }

        Model {
            source: "#Sphere"
            scale: Qt.vector3d(0.8, 0.8, 0.8)
            position: Qt.vector3d(125, -125, 0)
            eulerRotation.y: 90
            materials: [ PrincipledMaterial {
                    baseColor: "#4080A0"
                    metalness: 0
                    roughness: 1
                    indexOfRefraction: 0
                    specularAmount: 1
                } ]
        }
    }
}
