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

import QtQuick 2.0
import QtQuick3D 1.0
import MouseArea3D 0.1

Model {
    id: arrow
    rotationOrder: Node.XYZr
    source: "meshes/Arrow.mesh"

    property alias color: material.emissiveColor
    property Node targetNode: null

    readonly property bool hovering: mouseAreaYZ.hovering || mouseAreaXZ.hovering

    property var _pointerPosPressed
    property var _targetStartPos

    materials: DefaultMaterial {
        id: material
        emissiveColor: mouseAreaFront.hovering ? "white" : Qt.rgba(1.0, 0.0, 0.0, 1.0)
        lighting: DefaultMaterial.NoLighting
    }

    function handlePressed(mouseArea, pointerPosition)
    {
        if (!targetNode)
            return;

        var maskedPosition = Qt.vector3d(pointerPosition.x, 0, 0)
        _pointerPosPressed = mouseArea.mapPositionToScene(maskedPosition)
        var sp = targetNode.positionInScene
        _targetStartPos = Qt.vector3d(sp.x, sp.y, sp.z);
    }

    function handleDragged(mouseArea, pointerPosition)
    {
        if (!targetNode)
            return;

        var maskedPosition = Qt.vector3d(pointerPosition.x, 0, 0)
        var scenePointerPos = mouseArea.mapPositionToScene(maskedPosition)
        var sceneRelativeDistance = Qt.vector3d(
                    scenePointerPos.x - _pointerPosPressed.x,
                    scenePointerPos.y - _pointerPosPressed.y,
                    scenePointerPos.z - _pointerPosPressed.z)

        var newScenePos = Qt.vector3d(
                    _targetStartPos.x + sceneRelativeDistance.x,
                    _targetStartPos.y + sceneRelativeDistance.y,
                    _targetStartPos.z + sceneRelativeDistance.z)

        var posInParent = targetNode.parent.mapPositionFromScene(newScenePos)
        targetNode.position = posInParent
    }

    MouseArea3D {
        id: mouseAreaYZ
        view3D: overlayView
        x: 0
        y: -1.5
        width: 12
        height: 3
        rotation: Qt.vector3d(0, 90, 0)
        grabsMouse: targetNode
        onPressed: arrow.handlePressed(mouseAreaYZ, pointerPosition)
        onDragged: arrow.handleDragged(mouseAreaYZ, pointerPosition)
    }

    MouseArea3D {
        id: mouseAreaXZ
        view3D: overlayView
        x: 0
        y: -1.5
        width: 12
        height: 3
        rotation: Qt.vector3d(90, 90, 0)
        grabsMouse: targetNode
        onPressed: arrow.handlePressed(mouseAreaXZ, pointerPosition)
        onDragged: arrow.handleDragged(mouseAreaXZ, pointerPosition)
    }

}

