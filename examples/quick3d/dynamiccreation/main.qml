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
import QtQuick.Window 2.14
import QtQuick.Controls 2.14
import QtQuick3D 1.15

Window {
    id: window
    width: 1280
    height: 720
    visible: true
    title: "Dynamic Model Creation example"

    Button {
        id: addButton
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: 20
        text: "Add Model"
        implicitWidth: 150

        background: Rectangle {
            implicitWidth: 150
            implicitHeight: 40
            opacity: enabled ? 1 : 0.3
            color: parent.down ? "#6b7080" : "#848895"
            border.color: "#222840"
            border.width: 1
            radius: 5
        }

        onClicked: {
            if (shapeSpawner.instances.length < shapeSpawner.maxInstances)
                shapeSpawner.addOrRemove(true);
        }
    }

    Label {
        id: countLabel
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.margins: 20
        font.pointSize: 20
        font.bold: true
        color: "#848895"
    }

    Button {
        id: removeButton
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 20
        text: "Remove Model"
        implicitWidth: 150

        background: Rectangle {
            implicitWidth: 150
            implicitHeight: 40
            opacity: enabled ? 1 : 0.3
            color: parent.down ? "#6b7080" : "#848895"
            border.color: "#222840"
            border.width: 1
            radius: 5
        }

        onClicked: {
            if (shapeSpawner.instances.length > 0)
                shapeSpawner.addOrRemove(false);
        }
    }

    View3D {
        anchors.fill: parent
        camera: camera
        renderMode: View3D.Underlay

        environment: SceneEnvironment {
            clearColor: "black"
            backgroundMode: SceneEnvironment.Color
            antialiasingMode: SceneEnvironment.MSAA
            antialiasingQuality: SceneEnvironment.High
        }

        PointLight {
            position: Qt.vector3d(0, 0, 0);
            brightness: 1500
        }

        Node {
            position: Qt.vector3d(0, 0, 0);

            PerspectiveCamera {
                position: Qt.vector3d(0, 0, 600)
            }

            eulerRotation.y: -90

            SequentialAnimation on eulerRotation.y {
                loops: Animation.Infinite
                PropertyAnimation {
                    duration: 5000
                    to: 360
                    from: 0
                }
            }
        }

        //! [spawner node]
        Node {
            id: shapeSpawner
            property real range: 300
            property var instances: []
            readonly property int maxInstances: 100

            function addOrRemove(add) {
                //! [spawner node]
                if (add) {
                    //! [adding]
                    // Create a new weirdShape at random postion
                    var xPos = (2 * Math.random() * range) - range;
                    var yPos = (2 * Math.random() * range) - range;
                    var zPos = (2 * Math.random() * range) - range;
                    var weirdShapeComponent = Qt.createComponent("WeirdShape.qml");
                    let instance = weirdShapeComponent.createObject(
                            shapeSpawner, { "x": xPos, "y": yPos, "z": zPos,
                                "scale": Qt.vector3d(0.25, 0.25, 0.25)});
                    instances.push(instance);
                    //! [adding]
                    if (instances.length === maxInstances)
                        addButton.enabled = false;
                    else if (instances.length > 0)
                        removeButton.enabled = true;
                } else {
                    //! [removing]
                    // Remove last item in instances list
                    let instance = instances.pop();
                    instance.destroy();
                    //! [removing]
                    if (instances.length === 0)
                        removeButton.enabled = false;
                    else if (instances.length < maxInstances)
                        addButton.enabled = true;
                }
                countLabel.text = "Models in Scene: " + instances.length;
            }
        }

        //! [startup]
        Component.onCompleted: {
            // Create 10 instances to get started
            for (var i = 0; i < 10; ++i)
                shapeSpawner.addOrRemove(true);
        }
        //! [startup]
    }
}
