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

Node {
    id: overlayNode

    property View3D view3D
    property Node target: parent
    property bool autoScale: true

    // Read-only
    property real relativeScale: 1

    onGlobalTransformChanged: updateScale()
    onAutoScaleChanged: updateScale()
    Connections {
        target: view3D.camera
        onGlobalTransformChanged: updateScale()
    }

    Connections {
        target: window
        onFirstFrameReady: updateScale()
    }

    function getScale(baseScale)
    {
        return Qt.vector3d(baseScale.x * relativeScale, baseScale.y * relativeScale, baseScale.z * relativeScale)
    }

    function updateScale()
    {
        if (!autoScale) {
            target.scale = Qt.vector3d(1, 1, 1)
        } else {
            // Calculate the distance independent scale by first mapping the targets position to
            // the view. We then measure up a distance on the view (100px) that we use as an "anchor"
            // distance. Map the two positions back to the target node, and measure the distance
            // between them now, in the 3D scene. The difference of the two distances, view and scene, will
            // tell us what the distance independent scale should be.
            var posInView1 = view3D.mapFrom3DScene(positionInScene)
            var posInView2 = Qt.vector3d(posInView1.x + 100, posInView1.y, posInView1.z)

            var rayPos1 = view3D.mapTo3DScene(Qt.vector3d(posInView2.x, posInView2.y, 0))
            var rayPos2 = view3D.mapTo3DScene(Qt.vector3d(posInView2.x, posInView2.y, 10))

            var planeNormal = view3D.camera.forward
            var rayHitPos = helper.rayIntersectsPlane(rayPos1, rayPos2, positionInScene, planeNormal)

            relativeScale = positionInScene.minus(rayHitPos).length() / 100
        }
    }

    MouseArea3D {
        id: helper
        view3D: overlayNode.view3D
    }
}
