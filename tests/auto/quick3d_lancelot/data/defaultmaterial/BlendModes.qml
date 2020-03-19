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

import QtQuick3D 1.15
import QtQuick 2.15

Rectangle {
    id: defaultmaterial_screen

    property var blendModesModel: [
        DefaultMaterial.SourceOver,
        DefaultMaterial.Screen,
        DefaultMaterial.Multiply,
        DefaultMaterial.Overlay,
        DefaultMaterial.ColorBurn,
        DefaultMaterial.ColorDodge
    ]
    property int itemsWidth: 800
    property int itemsHeight: 400
    property int modelSeparation: itemsWidth / (blendModesModel.length - 1)

    width: 800
    height: 480
    color: "white"

    View3D {
        id: layer
        anchors.fill: parent
        environment: SceneEnvironment {
            clearColor: Qt.rgba(0, 0, 0, 1)
            aoDither: true
            depthPrePassEnabled: true
        }

        PerspectiveCamera {
            id: camera
            position: Qt.vector3d(0, 0, 800)
            clipFar: 5000
        }

        DirectionalLight {
        }

        Repeater3D {
            model: blendModesModel.length
            Node {
                x: -itemsWidth / 2 + index * modelSeparation
                y: -itemsHeight / 2 + index % 2 * itemsHeight
                BlendComponent {
                    id: modeComponent
                    blendMode: blendModesModel[index]
                }
                Node {
                    z: 200
                    Rectangle {
                        color: "white"
                        width: textItem.width + 20
                        height: textItem.height + 10
                    }
                    Text {
                        id: textItem
                        font.pixelSize: 20
                        color: "black"
                        text: "blendMode:" + modeComponent.blendMode
                    }
                }
            }
        }
    }
}
