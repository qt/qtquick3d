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

Item {
    id: root
    property int api: GraphicsInfo.api
    width: 800
    height: 400

    View3D {
        id: src
        renderMode: View3D.Offscreen
        width: parent.width / 2
        height: parent.height
        environment: SceneEnvironment {
            clearColor: "lightGray"
            backgroundMode: SceneEnvironment.Color
        }
        PerspectiveCamera {
            position: Qt.vector3d(0, 0, 600);
        }
        DirectionalLight {
        }
        Model {
            source: "#Rectangle"
            scale: Qt.vector3d(4, 4, 4)
            materials: PrincipledMaterial {
                baseColorMap: Texture {
                    source: "../shared/maps/oulu_2.jpeg"
                }
            }
        }
    }

    View3D {
        width: parent.width / 2
        height: parent.height
        x: parent.width / 2
        environment: SceneEnvironment {
            clearColor: "black"
            backgroundMode: SceneEnvironment.Color
        }
        PerspectiveCamera {
            position: Qt.vector3d(0, 0, 600);
        }
        Model {
            source: "../shared/models/suzanne.mesh"
            scale:  Qt.vector3d(120, 120, 120)
            materials: DefaultMaterial {
                lighting: DefaultMaterial.NoLighting
                diffuseMap: Texture {
                    sourceItem: src
                    // To get identical results on-screen with all graphics APIs.
                    // The other View3D renders into a texture as-is, and texture have Y up in OpenGL.
                    // There is nothing that would correct for this, so apply a V coordinate flip when using the texture.
                    flipV: root.api === GraphicsInfo.OpenGL
                }
            }
        }
    }
}
