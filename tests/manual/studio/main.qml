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

import QtQuick3D 1.15
import QtQuick3D.Helpers 1.15 as Helpers

import QtGraphicalEffects 1.14

import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14

import "gizmos"

ApplicationWindow {
    id: window
    width: 800
    height: 600
    visible: true

    property color xAxisGizmoColor: Qt.rgba(1, 0, 0, 1)
    property color yAxisGizmoColor: Qt.rgba(0, 0, 1, 1)
    property color zAxisGizmoColor: Qt.rgba(0, 0.8, 0, 1)

    property Node nodeBeingManipulated: pot1

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
            camera: perspectiveControl.checked ? perspectiveCamera : orthographicCamera

            Node {
                id: mainScene

                PerspectiveCamera {
                    id: perspectiveCamera
                    y: 200
                    z: 300
                    clipFar: 100000
                }

                OrthographicCamera {
                    id: orthographicCamera
                    position: perspectiveCamera.position
                    rotation: perspectiveCamera.rotation
                    clipFar: perspectiveCamera.clipFar
                }

                DirectionalLight {
                    id: light
                    y: 400
                    eulerRotation: Qt.vector3d(60, 0, 0)
                }

                Helpers.AxisHelper {
                    id: axisGrid
                    enableXZGrid: true
                    enableAxisLines: false
                }

                Model {
                    id: pot1
                    objectName: "First pot"
                    y: 200
                    pickable: true
                    eulerRotation: Qt.vector3d(0, 0, 45)
                    source: "meshes/teapot.mesh"
                    scale: Qt.vector3d(20, 20, 20)
                    materials: DefaultMaterial {
                        diffuseColor: "salmon"
                    }
                }
                Node {
                    x: pot1.x
                    y: pot1.y + 100
                    z: pot1.z
                    rotation: mainView.camera.rotation
                    visible: showLabelsControl.checked
                    Text {
                        visible: false
                        text: pot1.objectName
                    }
                }
                Model {
                    id: pot2
                    objectName: "Second pot"
                    x: 200
                    y: 200
                    z: -300
                    pickable: true
                    eulerRotation: Qt.vector3d(45, 45, 0)
                    source: "meshes/teapot.mesh"
                    scale: Qt.vector3d(20, 40, 20)
                    materials: DefaultMaterial {
                        diffuseColor: "darkkhaki"
                    }
                }
                Node {
                    x: pot2.x
                    y: pot2.y + 120
                    z: pot2.z
                    rotation: mainView.camera.rotation
                    visible: showLabelsControl.checked
                    Text {
                        text: pot2.objectName
                    }
                }
            }
        }

        CameraGizmo {
            id: cameraGizmo
            targetCamera: perspectiveCamera
            anchors.bottom: parent.bottom
            width: 80
            height: 80
            xColor: xAxisGizmoColor
            yColor: yAxisGizmoColor
            zColor: zAxisGizmoColor
        }

        TranslateGizmo {
            id: translateGizmo
            visible: toolGroup.checkedButton == translateButton
            target: nodeBeingManipulated
            targetView: mainView
            space: spaceControl.checked ? Node.SceneSpace : Node.LocalSpace
            xColor: xAxisGizmoColor
            yColor: yAxisGizmoColor
            zColor: zAxisGizmoColor
        }

        RotateGizmo {
            id: rotateGizmo
            visible: toolGroup.checkedButton == rotateButton
            target: nodeBeingManipulated
            targetView: mainView
            space: spaceControl.checked ? Node.SceneSpace : Node.LocalSpace
            xColor: xAxisGizmoColor
            yColor: yAxisGizmoColor
            zColor: zAxisGizmoColor
        }

        Helpers.WasdController {
            id: wasd
            controlledObject: perspectiveCamera
            acceptedButtons: Qt.RightButton
        }

        TapHandler {
            onTapped: {
                var pickResult = mainView.pick(point.position.x, point.position.y)
                if (pickResult.objectHit)
                    nodeBeingManipulated = pickResult.objectHit
            }
        }
    }

    Item {
        id: menu
        anchors.fill: parent

        ButtonGroup {
            id: toolGroup
            buttons: toolRow.children
        }

        Row {
            id: toolRow
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.margins: 4
            spacing: 1

            Button {
                id: translateButton
                text: "Translate"
                checkable: true
                onClicked: wasd.forceActiveFocus()
            }

            Button {
                id: rotateButton
                text: "Rotate"
                checkable: true
                checked: true
                onClicked: wasd.forceActiveFocus()
            }
        }

        Row {
            id: cameraRow
            spacing: 1
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.margins: 4
            Button {
                text: "Front"
                onClicked: {
                    var dist = perspectiveCamera.scenePosition.minus(nodeBeingManipulated.scenePosition).length()
                    perspectiveCamera.eulerRotation = Qt.vector3d(0, 0, 0)
                    perspectiveCamera.position = nodeBeingManipulated.position.plus(Qt.vector3d(0, 0, dist))
                    wasd.forceActiveFocus()
                }
            }

            Button {
                text: "Right"
                onClicked: {
                    var dist = perspectiveCamera.scenePosition.minus(nodeBeingManipulated.scenePosition).length()
                    perspectiveCamera.eulerRotation = Qt.vector3d(0, 90, 0)
                    perspectiveCamera.position = nodeBeingManipulated.position.plus(Qt.vector3d(dist, 0, 0))
                    wasd.forceActiveFocus()
                }
            }

            Button {
                text: "Top"
                onClicked: {
                    var dist = perspectiveCamera.scenePosition.minus(nodeBeingManipulated.scenePosition).length()
                    perspectiveCamera.eulerRotation = Qt.vector3d(-90, 0, 0)
                    perspectiveCamera.position = nodeBeingManipulated.position.plus(Qt.vector3d(0, dist, 0))
                    wasd.forceActiveFocus()
                }
            }
        }

        ColumnLayout {
            anchors.top: toolRow.bottom
            x: -4

            CheckBox {
                id: spaceControl
                checked: false
                onCheckedChanged: wasd.forceActiveFocus()
                Text {
                    anchors.left: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Use scene orientation")
                }
            }

            CheckBox {
                id: perspectiveControl
                checked: true
                onCheckedChanged: {
                    wasd.forceActiveFocus()
                }
                Text {
                    anchors.left: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Use perspective")
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
        text: "Camera: W,A,S,D,R,F,right mouse drag "
        anchors.bottom: parent.bottom
        anchors.right: parent.right
    }
}
