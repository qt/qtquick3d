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
import QtQuick.Layouts
import QtQuick3D

Rectangle {
    width: 400
    height: 400
    color: "lightgray"

    // Row 1 - Quick
    Image {
        height: 130
        width: parent.width
        source: "../shared/maps/checkers1.png"
        fillMode: Image.Tile

        RowLayout {
            anchors.fill: parent

            Image {
                id: o1_bc1
                source: "../shared/maps/o1_bc1.ktx"
                Layout.alignment: Qt.AlignCenter
            }

            Image {
                id: t2_bc2
                source: "../shared/maps/t2_bc2.ktx"
                Layout.alignment: Qt.AlignCenter
            }

            Image {
                id: t2_png
                source: "../shared/maps/t2.png"
                Layout.alignment: Qt.AlignCenter

            }
        }
    }

    View3D {
        y: 140
        width: parent.width
        height: parent.height - y
        renderMode: View3D.Offscreen

        OrthographicCamera {
            z: 500
        }

        DirectionalLight {
            eulerRotation.y: 180
            ambientColor: Qt.rgba(1.0, 1.0, 1.0, 1.0);
        }

        Model {
            source: "#Rectangle"
            materials: [ DefaultMaterial {
                    lighting: DefaultMaterial.NoLighting
                    diffuseMap: Texture {
                        source: "../shared/maps/checkers1.png"
                        tilingModeHorizontal: Texture.Repeat
                        tilingModeVertical: Texture.Repeat
                        scaleU: 100
                        scaleV: 100
                    }
                } ]
            z: -500
            scale: Qt.vector3d(10, 10, 1)
        }

        // Row 2 - Quick3D loading texture files
        Model {
            source: "#Rectangle"
            scale: Qt.vector3d(0.7, 0.7, 1)
            position: Qt.vector3d(-125, 65, 0)
            materials: [ DefaultMaterial {
                    diffuseMap: Texture { source: o1_bc1.source }
                } ]
        }

        Model {
            source: "#Rectangle"
            scale: Qt.vector3d(0.7, 0.7, 1)
            position: Qt.vector3d(0, 65, 0)
            materials: [ DefaultMaterial {
                    diffuseMap: Texture { source: t2_bc2.source }
                } ]
        }

        Model {
            source: "#Rectangle"
            scale: Qt.vector3d(0.7, 0.7, 1)
            position: Qt.vector3d(125, 65, 0)
            materials: [ DefaultMaterial {
                    diffuseMap: Texture { source: t2_png.source }
                } ]
        }

        // Row 3 - Quick3D loading textures from Quick items
        Model {
            source: "#Rectangle"
            scale: Qt.vector3d(0.7, 0.7, 1)
            position: Qt.vector3d(-125, -65, 0)
            materials: [ DefaultMaterial {
                    diffuseMap: Texture { sourceItem: o1_bc1 }
                } ]
        }

        Model {
            source: "#Rectangle"
            scale: Qt.vector3d(0.7, 0.7, 1)
            position: Qt.vector3d(0, -65, 0)
            materials: [ DefaultMaterial {
                    diffuseMap: Texture { sourceItem: t2_bc2 }
                } ]
        }

        Model {
            source: "#Rectangle"
            scale: Qt.vector3d(0.7, 0.7, 1)
            position: Qt.vector3d(125, -65, 0)
            materials: [ DefaultMaterial {
                    diffuseMap: Texture { sourceItem: t2_png }
                } ]
        }
    }
}
