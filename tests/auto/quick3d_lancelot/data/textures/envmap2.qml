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

Item {
    width: 800
    height: 600

    Item {
        id: quickScene
        visible: false
        width: 1024
        height: 1024
        Image {
            source: "../shared/maps/00455_OpenfootageNET_field_low_resized.png"
            anchors.fill: parent
        }
        Text {
            text: "Background"
            font.pointSize: 36
            anchors.horizontalCenter: parent.horizontalCenter
            color: "red"
            y: 200
        }
    }

    View3D {
        anchors.fill: parent
        PerspectiveCamera {
            id: camera
            // Positioned in a way so the "Background" text on top of the
            // background image is reflected on the top face of the cube.
            z: 450
        }
        DirectionalLight {
        }
        Model {
            source: "../shared/models/InvertedSphere.mesh"
            scale: Qt.vector3d(4000, 4000, 4000)
            eulerRotation.y: 90
            materials: PrincipledMaterial {
                lighting: PrincipledMaterial.NoLighting
                baseColorMap: Texture {
                    flipU: true
                    sourceItem: quickScene
                }
            }
        }
        Model {
            source: "#Cube"
            scale: Qt.vector3d(3, 3, 3)
            eulerRotation: Qt.vector3d(30, 30, 0)
            materials: PrincipledMaterial {
                specularReflectionMap: Texture {
                    sourceItem: quickScene
                    mappingMode: Texture.Environment
                }
            }
        }
    }
}
