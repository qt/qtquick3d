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
import QtQuick3D.Helpers 1.15 as Helpers

Model {
    id: arrow
    source: "qrc:///meshes/gizmoarrow.mesh"
    pickable: true

    property Node gizmoRoot
    property Node gizmoAxisRoot: arrow
    property int axis: -1

    property color color: "white"

    property var _pointerStartPos
    property var _targetStartPos
    property var _target
    property var _view

    materials: DefaultMaterial {
        id: material
        emissiveColor: color
        lighting: DefaultMaterial.NoLighting
    }

    function startDrag(targetNode, view, pointerPosition)
    {
        _target = targetNode
        _view = view
        _pointerStartPos = getSceneIntersectPos(pointerPosition)
        var sp = _target.scenePosition
        _targetStartPos = Qt.vector3d(sp.x, sp.y, sp.z)
    }

    function continueDrag(pointerPosition)
    {
        var scenePointerPos = getSceneIntersectPos(pointerPosition)
        var sceneRelativeDistance = Qt.vector3d(
                    scenePointerPos.x - _pointerStartPos.x,
                    scenePointerPos.y - _pointerStartPos.y,
                    scenePointerPos.z - _pointerStartPos.z)

        var newScenePos = Qt.vector3d(
                    _targetStartPos.x + sceneRelativeDistance.x,
                    _targetStartPos.y + sceneRelativeDistance.y,
                    _targetStartPos.z + sceneRelativeDistance.z)

        var posInParent = _target.parent.mapPositionFromScene(newScenePos)
        _target.position = posInParent
    }

    function getSceneIntersectPos(pointerPosition)
    {
        var scenePos = plane.getIntersectPosFromView(_view, pointerPosition)
        if (scenePos.x === 0 && scenePos.y === 0 && scenePos.z === 0) {
            // The viewport is perpendicular to the plane. Tilt the plane and try again
            plane.rotate(45, Qt.vector3d(1, 0, 0), Node.LocalSpace)
            scenePos = plane.getIntersectPosFromView(_view, pointerPosition)
        }

        // Get the local distance along the x axis from the origin to to the
        // pointer position. This is the only value we care about, since
        // you can only drag the arrow along it's local x axis.
        var localPos = plane.mapPositionFromScene(scenePos)
        var maskedLocalPos = Qt.vector3d(localPos.x, 0, 0)

        return plane.mapPositionToScene(maskedLocalPos)
    }

    Node {
        eulerRotation: Qt.vector3d(0, 90, 0)
        Helpers.PointerPlane {
            id: plane
        }
    }
}

