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
import QtQuick3D 1.15

View3D {
    id: rootView

    property var target: null
    property View3D targetView: null
    property var space: Node.LocalSpace

    property color xColor: "red"
    property color yColor: "blue"
    property color zColor: "green"

    property int hoverAxis: -1
    property bool dragging: false

    property alias rootView: rootView
    property alias gizmoRootNode: gizmoRootNode

    camera: (targetView.camera instanceof OrthographicCamera) ?
                orthographicCamera : perspectiveCamera

    implicitWidth: targetView.width
    implicitHeight: targetView.height

    Item {
        id: dragAPI
        anchors.fill: parent
        property var hoverNode: null

        HoverHandler {
            onPointChanged: {
                var mousePressed = point.pressedButtons === Qt.LeftButton;

                if (!mousePressed)
                    dragging = false

                if (dragging) {
                    dragAPI.hoverNode.continueDrag(point.position)
                } else {
                    // Start a new drag?
                    var pickResult = rootView.pick(point.position.x, point.position.y)
                    var nodeUnderMouse = pickResult.objectHit

                    if (!nodeUnderMouse || !nodeUnderMouse.visible) {
                        hoverAxis = -1
                    } else {
                        var gizmoPart = nodeUnderMouse.gizmoAxisRoot
                        if (!gizmoPart || !gizmoPart.gizmoRoot.visible)
                            return;

                        dragAPI.hoverNode = gizmoPart
                        hoverAxis = gizmoPart.axis

                        if (mousePressed) {
                            dragging = true
                            dragAPI.hoverNode.startDrag(nodeBeingManipulated, rootView, point.position)
                        }
                    }
                }
            }
        }
    }

    Node {

        PerspectiveCamera {
            id: perspectiveCamera
            position: targetView.camera.position
            rotation: targetView.camera.rotation
        }

        OrthographicCamera {
            id: orthographicCamera
            position: targetView.camera.position
            rotation: targetView.camera.rotation
        }

        Node {
            id: gizmoRootNode
            position: target.scenePosition
            rotation: space === Node.SceneSpace ? Qt.quaternion(0, 0, 0, 0) : target.sceneRotation

            GizmoScaleHelper {
                id: scaleHelper
                target: gizmoRootNode
                targetView: rootView
            }

            // Gizmo axis children will be added here!
        }
    }
}
