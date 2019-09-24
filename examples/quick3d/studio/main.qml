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

import QtQuick 2.12
import QtQuick.Window 2.12

import QtQuick3D 1.0
import QtQuick3D.Helpers 1.0

import QtGraphicalEffects 1.0

import QtQuick.Controls 2.5
import QtQuick.Layouts 1.12

import MouseArea3D 0.1

ApplicationWindow {
    id: window
    width: 640
    height: 480
    visible: true

    property Node nodeBeingManipulated: pot1
    signal firstFrameReady

    Timer {
        // Work-around the fact that the projection matrix for the camera is not
        // calculated until the first frame is rendered, so any initial calls to
        // mapToViewport() etc will fail.
        interval: 1
        running: true
        onTriggered: firstFrameReady()
    }

    Node {
        id: mainScene

        Camera {
            id: camera1
            y: 200
            z: -300
            clipFar: 100000
            projectionMode: perspectiveControl.checked ? Camera.Perspective : Camera.Orthographic
        }

        Light {
            id: light
            y: 400
            diffuseColor: Qt.rgba(0.4, 0.5, 0.0, 1.0)
            rotation: Qt.vector3d(60, 0, 0)
            brightness: 80
        }

        AxisHelper {
            id: axisGrid
            enableXZGrid: true
            enableAxisLines: false
        }

        Model {
            id: pot1
            objectName: "First pot"
            y: 200
            rotation: Qt.vector3d(0, 0, 45)
            source: "meshes/Teapot.mesh"
            scale: Qt.vector3d(20, 20, 20)
            materials: DefaultMaterial {
                diffuseColor: "salmon"
            }
        }

        Model {
            id: pot2
            objectName: "Second pot"
            x: 200
            y: 200
            z: 300
            rotation: Qt.vector3d(45, 45, 0)
            source: "meshes/Teapot.mesh"
            scale: Qt.vector3d(20, 20, 20)
            materials: DefaultMaterial {
                diffuseColor: "salmon"
            }
        }
    }

    Node {
        id: overlayScene

        Camera {
            id: overlayCamera
            projectionMode: perspectiveControl.checked ? Camera.Perspective : Camera.Orthographic
            clipFar: camera1.clipFar
            position: camera1.position
            rotation: camera1.rotation
        }

        MoveGizmo {
            id: targetGizmo
            scale: autoScaleControl.checked ? autoScale.getScale(Qt.vector3d(5, 5, 5)) : Qt.vector3d(5, 5, 5)
            highlightOnHover: true
            targetNode: window.nodeBeingManipulated
            position: window.nodeBeingManipulated.positionInScene
            rotation: globalControl.checked ? Qt.vector3d(0, 0, 0) : window.nodeBeingManipulated.rotationInScene
        }

        AutoScaleHelper {
            id: autoScale
            view3D: overlayView
            position: targetGizmo.positionInScene
        }
    }

    RadialGradient {
        id: sceneBg
        anchors.fill: parent

        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.rgba(0.7, 0.7, 0.8, 1) }
            GradientStop { position: 0.5; color: Qt.rgba(0.5, 0.5, 0.5, 1) }
        }

        View3D {
            id: mainView
            anchors.fill: parent
            camera: camera1
            scene: mainScene
        }

        View3D {
            id: overlayView
            anchors.fill: parent
            camera: overlayCamera
            scene: overlayScene
        }

        CameraGizmo {
            targetCamera: camera1
            anchors.right: parent.right
            width: 100
            height: 100
        }

        Overlay2D {
            id: overlayLabelPot1
            targetNode: pot1
            targetView: mainView
            offsetY: 100
            visible: showLabelsControl.checked

            Rectangle {
                color: "white"
                x: -width / 2
                y: -height
                width: pot1Text.width + 4
                height: pot1Text.height + 4
                border.width: 1
                Text {
                    id: pot1Text
                    text: pot1.objectName
                    anchors.centerIn: parent
                }
            }
        }

        Overlay2D {
            id: overlayLabelPot2
            targetNode: pot2
            targetView: mainView
            offsetY: 100
            visible: showLabelsControl.checked

            Rectangle {
                color: "white"
                x: -width / 2
                y: -height
                width: pot2Text.width + 4
                height: pot2Text.height + 4
                border.width: 1
                Text {
                    id: pot2Text
                    text: pot2.objectName
                    anchors.centerIn: parent
                }
            }
        }

        WasdController {
            id: wasd
            controlledObject: mainView.camera
            acceptedButtons: Qt.RightButton
        }

        TapHandler {
            onTapped: {
                // Pick a pot, and use it as target for the gizmo
                var pickResult = mainView.pick(point.position.x, point.position.y)
                print("picked in mainView:", pickResult.objectHit)

                if (pickResult.objectHit && pickResult.objectHit !== axisGrid)
                    nodeBeingManipulated = pickResult.objectHit

                // Dummy test for now, just to show that it currently doesn't work to pick models
                // from two different views at the same time. On of the calls will fail.
                var pickResult2 = overlayView.pick(point.position.x, point.position.y)
                print("picked in overlayView:", pickResult2.objectHit)
            }
        }
    }

    Item {
        id: menu
        anchors.fill: parent

        ColumnLayout {
            anchors.fill: parent

            CheckBox {
                id: globalControl
                checked: true
                onCheckedChanged: wasd.forceActiveFocus()
                Text {
                    anchors.left: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Use global orientation")
                }
            }

            CheckBox {
                id: perspectiveControl
                checked: true
                onCheckedChanged: wasd.forceActiveFocus()
                Text {
                    anchors.left: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Use perspective")
                }
            }

            CheckBox {
                id: autoScaleControl
                checked: true
                onCheckedChanged: wasd.forceActiveFocus()
                Text {
                    anchors.left: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Use fixed-sized gizmo")
                }
            }

            CheckBox {
                id: showLabelsControl
                checked: true
                onCheckedChanged: wasd.forceActiveFocus()
                Text {
                    anchors.left: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Show labels")
                }
            }

            Item {
                Layout.fillHeight: true
            }
        }
    }

    Text {
        text: "Camera: W,A,S,D,R,F,right mouse drag"
        anchors.bottom: parent.bottom
    }

}
