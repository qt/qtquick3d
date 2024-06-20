// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick3D

Node {
    id: teleporter
    required property var rayPicker //any object that has implemented rayPick(pos, dir)
    required property Node cameraOrigin
    required property Node camera
    required property Node beamHandle
    property real cameraSnapRotation: 30
    property real xStickValue: 0
    property real yStickValue: 0
    property alias screenVisibility: screenValueFader.value
    property bool targetValid: false
    property color rayHitColor: "green"
    property color rayMissColor: "red"
    property bool teleporting: false
    property int blinkSpeed: 150

    function teleportTo(position) {
        teleporter.teleporting = true
        let offset = camera.scenePosition.minus(cameraOrigin.scenePosition)
        let cameraOriginPosition = position.minus(offset)
        cameraOriginPosition.y = position.y

        screenValueFader.blink(()=>{
                                   teleporter.doTeleportation(cameraOriginPosition)
                               },()=>{teleporter.teleporting = false}, teleporter.blinkSpeed)
    }

    function rotateBy(degrees) {
        let r = Quaternion.fromEulerAngles(0, degrees, 0)
        let origin = Qt.vector3d(camera.position.x, 0, camera.position.z)
        let mappedOrigin = cameraOrigin.rotation.times(origin).plus(cameraOrigin.position)
        let rotatedOrigin = r.times(origin)
        let mappedRO = cameraOrigin.rotation.times(rotatedOrigin).plus(cameraOrigin.position)
        let delta = mappedRO.minus(mappedOrigin)

        doRotation(cameraOrigin.rotation.times(r), cameraOrigin.position.minus(delta))
    }

    signal doTeleportation(var cameraOriginPosition)

    signal doRotation(var cameraOriginRotation, var cameraOriginPosition)

    readonly property bool xPlusRotation: xStickValue > 0.5
    onXPlusRotationChanged: {
        if (xPlusRotation)
            rotateBy(-cameraSnapRotation)
    }

    readonly property bool xMinusRotation: xStickValue < -0.5
    onXMinusRotationChanged: {
        if (xMinusRotation)
            rotateBy(cameraSnapRotation)
    }

    ValueFader {
        id: screenValueFader
    }

    TargetIndicator {
        id: targetIndicator
    }

    BeamModel {
        id: beamModel
        color: teleporter.targetValid ? teleporter.rayHitColor : teleporter.rayMissColor
    }

    FrameAnimation {
        running: teleporter.yStickValue > 0.7
        onTriggered: {
            teleporter.updateTarget()
        }
        onRunningChanged: {
            if (running) {
                beamModel.show()
            }else {
                beamModel.hide()
                targetIndicator.hide()
                if (teleporter.targetValid)
                    teleporter.teleportTo(targetIndicator.scenePosition)
            }
        }
    }

    function updateTarget() : bool {
        // Not a pure gravity parabola: We want a flatter curve

        let beamPositions = [];
        let pos = beamHandle.scenePosition
        const dx = beamHandle.forward.x
        const dz = beamHandle.forward.z
        const a = Qt.vector3d(dx * 2, -4, dz * 2)
        let d = beamHandle.forward.times(50)
        let index = 0
        let hit = false
        let pickResult = null
        beamPositions.push(Qt.vector3d(pos.x, pos.y, pos.z))
        for (let i = 0; !hit && i < 50; ++i) {
            pickResult = teleporter.rayPicker.rayPick(pos, d)
            pos = pos.plus(d)
            d = d.plus(a)
            hit = pickResult.objectHit && pickResult.distance < d.length()
            beamPositions.push(Qt.vector3d(pos.x, pos.y, pos.z))
        }
        beamModel.generate(beamPositions)

        if (pickResult.objectHit && pickResult.sceneNormal.y > 0.9) {
            teleporter.targetValid = true
            targetIndicator.moveTo(pickResult.scenePosition)
            targetIndicator.show()
        } else {
            teleporter.targetValid = false
            targetIndicator.hide()
        }

        return teleporter.targetValid
    }
}
