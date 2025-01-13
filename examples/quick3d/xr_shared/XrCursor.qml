// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick3D

Model {
    id: root
    source: sphere ? "#Sphere" : "#Rectangle"
    property real size: 2

    property Node cameraNode: null
    property real scaleDistance: 100 // Reference distance

    property alias radius: targetMaterial.radius // TODO: handle antialiasing in vertex shader

    property real sf: size / 100 // private property

    opacity: 0.5

    scale: sphere ? Qt.vector3d(sf * radius * 2, sf * radius * 2, sf * radius * 2) : Qt.vector3d(sf, sf, sf)

    property bool sphere: false

    CustomMaterial {
        id: targetMaterial
        shadingMode: CustomMaterial.Unshaded
        cullMode: Material.BackFaceCulling
        vertexShader: "shaders/cursor.vert"
        fragmentShader: "shaders/cursor.frag"

        property real radius: 0.3
        property real opacity: root.opacity
    }

    PrincipledMaterial {
        id: testMaterial
        baseColor: "white"
    }

    materials: sphere ? testMaterial : targetMaterial

    function setPositionAndOrientation(scenePos : vector3d, sceneNormal : vector3d) {
        if (sphere) {
            position = scenePos
        } else {
            const newPos = scenePos.minus(sceneNormal.normalized().times(-0.1))
            rotation = Quaternion.lookAt(newPos, scenePos,
                                         Qt.vector3d(0, 0, -1), Qt.vector3d(0, 1, 0))
            position = newPos
        }
        if (cameraNode && scaleDistance) {
            const distance = scenePos.minus(cameraNode.position).length()
            const distanceScale = Math.max(1, distance / root.scaleDistance)
            sf = size * distanceScale / 100
        }
    }
}
