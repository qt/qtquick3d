// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick3D

Window {
    id: window
    width: 1280
    height: 720
    visible: true
    color: "black"
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

        onClicked: shapeSpawner.addShape()
    }

    Label {
        id: countLabel
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.margins: 20
        font.pointSize: 20
        font.bold: true
        color: "#848895"

        text: "Models in Scene: " + shapeSpawner.count
    }

    Button {
        id: removeButton
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 20
        text: "Remove Model"
        implicitWidth: 150
        enabled: shapeSpawner.count > 0

        background: Rectangle {
            implicitWidth: 150
            implicitHeight: 40
            opacity: enabled ? 1 : 0.3
            color: parent.down ? "#6b7080" : "#848895"
            border.color: "#222840"
            border.width: 1
            radius: 5
        }

        onClicked: shapeSpawner.removeShape()
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
            brightness: 2.5
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
            property int count
        //! [spawner node]

            //! [adding]
            function addShape()
            {
                var xPos = (2 * Math.random() * range) - range;
                var yPos = (2 * Math.random() * range) - range;
                var zPos = (2 * Math.random() * range) - range;
                var shapeComponent = Qt.createComponent("WeirdShape.qml");
                let instance = shapeComponent.createObject(shapeSpawner,
                    { "x": xPos, "y": yPos, "z": zPos, "scale": Qt.vector3d(0.25, 0.25, 0.25)});
                instances.push(instance);
                count = instances.length
            }
            //! [adding]

            //! [removing]
            function removeShape()
            {
                if (instances.length > 0) {
                    let instance = instances.pop();
                    instance.destroy();
                    count = instances.length
                }
            }
            //! [removing]
        }

        //! [startup]
        Component.onCompleted: {
            for (var i = 0; i < 10; ++i)
                shapeSpawner.addShape()
        }
        //! [startup]
    }
}
