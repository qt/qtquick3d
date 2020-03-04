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
    width: 800
    height: 480
    color: Qt.rgba(0, 0, 0, 1)

    View3D {
        id: layer
        anchors.fill: parent
        environment: SceneEnvironment {
            clearColor: Qt.rgba(0, 0, 0, 1)
            aoDither: true
            depthPrePassEnabled: true
        }

        PerspectiveCamera {
            position: Qt.vector3d(0, 0, 600)
        }

        DirectionalLight {
            eulerRotation.x: -30
            eulerRotation.y: -70
        }

        Component.onCompleted: {
            // At the end there should be 5 red and 5 blue items
            for (var i = 0; i < 8; ++i)
                shapeSpawner.addItem("TEXT " + i, "red");
            for (var i = 0; i < 3; ++i)
                shapeSpawner.removeItem();
            for (var i = 0; i < 15; ++i)
                shapeSpawner.addItem("TEXT " + i, "blue");
            for (var i = 0; i < 10; ++i)
                shapeSpawner.removeItem();
        }

        Node {
            id: shapeSpawner
            property real range: 300
            property var instances: []

            function addItem(textLabel, textColor) {
                var yPos = range - (2 * (0.1 * instances.length) * range);
                var itemComponent = Qt.createComponent("Component.qml");
                let instance = itemComponent.createObject(
                        shapeSpawner, { "y": yPos, "textLabel": textLabel, "textColor": textColor});
                instances.push(instance);
            }
            function removeItem() {
                let instance = instances.pop();
                instance.destroy();
            }
        }

        Node {
            y: 200
            x: -200
            Loader {
                anchors.fill: parent
                sourceComponent: rect
            }
        }
        Node {
            y: 0
            x: -200
            Loader {
                id: loaderItem
            }
            Component.onCompleted: {
                loaderItem.sourceComponent = rect;
            }
        }
        Node {
            y: -200
            x: -200
            Loader {
                id: loaderItem2
                sourceComponent: rect
            }
            Component.onCompleted: {
                // Remove component, so shouldn't be visible
                loaderItem2.sourceComponent = undefined;
            }
        }

        Component {
            id: rect
            Rectangle {
                width: 200
                height: 150
                color: "green"
                border.color: "white"
                border.width: 5
                Text {
                    anchors.centerIn: parent
                    color: "white"
                    font.pixelSize: 20
                    text: "Loader Component"
                }
            }
        }
    }
}
