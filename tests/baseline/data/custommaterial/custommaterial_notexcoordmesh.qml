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

Rectangle {
    width: 400
    height: 400
    color: "black"

    View3D {
        anchors.fill: parent
        camera: camera

        PerspectiveCamera {
            id: camera
            position: Qt.vector3d(0, 200, 300)
            eulerRotation.x: -30
        }

        DirectionalLight {
            ambientColor: Qt.rgba(0.1, 0.1, 0.1, 1.0)
        }

        Model {
            scale: Qt.vector3d(40, 40, 40)
            // use a mesh that does not have attr_uv0
            // (the teapot here is good for this test because it only has attr_pos and attr_norm)
            source: "../shared/models/teapot_without_texcoords.mesh"
            x: -100
            materials: [
                CustomMaterial {
                    TextureInput {
                        id: texInput
                        enabled: true
                        texture: Texture {
                            source: "../shared/maps/oulu_2.jpeg"
                        }
                    }

                    vertexShader: "custommaterial_notexcoordmesh.vert"
                    fragmentShader: "custommaterial_notexcoordmesh.frag"
                    property real selector: 0.0
                    property TextureInput tex: texInput
                }
            ]
        }

        Model {
            scale: Qt.vector3d(40, 40, 40)
            source: "../shared/models/teapot_without_texcoords.mesh"
            x: 100
            materials: [
                CustomMaterial {
                    vertexShader: "custommaterial_notexcoordmesh.vert"
                    fragmentShader: "custommaterial_notexcoordmesh.frag"
                    property real selector: 1.0
                    property TextureInput tex: texInput
                }
            ]
        }
    }
}
