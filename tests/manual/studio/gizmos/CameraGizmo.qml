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

/*
    The gizmo showing the rotation of the camera differs from other
    gizmos by the fact that it doesn't overlay the camera in the
    scene, but rather stays in a corner of the view. Still, the
    perspective of the gizmo should reflect the perspective as if
    it was located on the same spot as the camera. For that reason, we
    draw it in a separate View3D, which means that the user can position
    it wherever he wants on the screen without affecting how it looks.
 */
View3D {
    id: root
    property PerspectiveCamera targetCamera
    implicitWidth: 60
    implicitHeight: 60
    camera: localCamera

    property color xColor: "red"
    property color yColor: "blue"
    property color zColor: "green"

    Connections {
        target: targetCamera
        onSceneTransformChanged: updateGizmo()
        Component.onCompleted: updateGizmo()
    }

    function updateGizmo()
    {
        sceneGizmo.position = root.mapTo3DScene(Qt.vector3d(root.width / 2, root.height / 2, 180))
    }

    Node {
        PerspectiveCamera {
            id: localCamera
            position: targetCamera.scenePosition
            rotation: targetCamera.sceneRotation
        }

        Node {
            id: sceneGizmo
            scale: Qt.vector3d(7, 7, 7)

            Model {
                id: arrowX
                eulerRotation: Qt.vector3d(0, 90, 0)
                source: "qrc:///meshes/gizmoarrow.mesh"
                materials: DefaultMaterial {
                    id: materialX
                    emissiveColor: xColor
                    lighting: DefaultMaterial.NoLighting
                }
            }
            Node {
                x: 14
                scale: Qt.vector3d(0.4, 0.4, 0.4)
                rotation: camera.rotation
                Text {
                    color: xColor
                    text: "x"
                }
            }
            Model {
                id: arrowY
                eulerRotation: Qt.vector3d(-90, 0, 0)
                source: "qrc:///meshes/gizmoarrow.mesh"
                materials: DefaultMaterial {
                    emissiveColor: yColor
                    lighting: DefaultMaterial.NoLighting
                }
            }
            Node {
                y: 14
                scale: Qt.vector3d(0.4, 0.4, 0.4)
                rotation: camera.rotation
                Text {
                    color: yColor
                    text: "y"
                }
            }
            Model {
                id: arrowZ
                eulerRotation: Qt.vector3d(0, -180, 0)
                source: "qrc:///meshes/gizmoarrow.mesh"
                materials: DefaultMaterial {
                    emissiveColor: zColor
                    lighting: DefaultMaterial.NoLighting
                }
            }
            Node {
                z: -14
                scale: Qt.vector3d(0.4, 0.4, 0.4)
                rotation: camera.rotation
                Text {
                    color: zColor
                    text: "z"
                }
            }
        }
    }
}
