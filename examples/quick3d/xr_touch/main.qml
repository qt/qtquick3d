// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import QtQuick3D
import QtQuick3D.Helpers
import QtQuick3D.Xr

XrView {
    id: xrView
    referenceSpace: XrView.ReferenceSpaceStage

    depthSubmissionEnabled: true

    xrOrigin: theOrigin

    environment: SceneEnvironment {
        id: sceneEnvironment
        lightProbe: Texture {
            textureData: ProceduralSkyTextureData {
            }
        }
        antialiasingMode: SceneEnvironment.MSAA
        antialiasingQuality: SceneEnvironment.High
        backgroundMode: SceneEnvironment.Color
        clearColor: "skyblue"
        probeHorizon: 0.5
    }

    DirectionalLight {
        eulerRotation.x: -30
        eulerRotation.y: -70
    }

    component Hand : XrHandModel {
        id: handComponentRoot
        property color color: "#ddaa88"
        required property int touchId
        XrController {
            id: handController
            controller: handComponentRoot.hand
        }

        property vector3d touchPos: handController.pokePosition
        onTouchPosChanged: {
            const scenePos = theOrigin.mapPositionToScene(touchPos)
            const touchOffset = xrView.processTouch(scenePos, handComponentRoot.touchId)
            position = touchOffset
            buttons.handleTouch(scenePos)
        }
        materials: PrincipledMaterial {
            baseColor: handComponentRoot.color
            roughness: 0.5
        }
    }

    XrOrigin {
        id: theOrigin
        z: 25

        Hand {
            id: rightHandModel
            hand: XrHandModel.RightHand
            touchId: 0
        }
        Hand {
            id: leftHandModel
            hand: XrHandModel.LeftHand
            touchId: 1
        }
    }

    Model {
        id: floor
        source: "#Rectangle"
        eulerRotation.x: -90
        scale: Qt.vector3d(5,5,5)
        materials: [ PrincipledMaterial {
                baseColor: "green"
            }
        ]
    }

    Model {
        id: table
        property real height: 70
        position: Qt.vector3d(0, height - 2.5, 0)
        source: "#Cube"
        scale: Qt.vector3d(1.5, 0.05, 0.6)
        materials: PrincipledMaterial {
            baseColor: "white"
        }
    }

    Node {
        id: buttons
        y: table.height
        z: 20
        property ExampleButton activeButton: midButton

        function handleTouch(touchPos : vector3d) {
            for (const b of [leftButton, midButton, rightButton]) {
                if (b.hit(touchPos)) {
                    if (buttons.activeButton !== b) {
                        buttons.activeButton = b
                        monitor.monitorRotation = b.degrees
                    }
                    break
                }
            }
        }

        ExampleButton {
            id: leftButton
            x: -20
            on: buttons.activeButton === this
            objectName: "Button 1"
            degrees: -20
        }
        ExampleButton {
            id: midButton
            x: 0
            on: buttons.activeButton === this
            objectName: "Button 2"
            degrees: 0
        }
        ExampleButton {
            id: rightButton
            x: 20
            on: buttons.activeButton === this
            objectName: "Button 3"
            degrees: 20
        }
    }

    Monitor {
        id: monitor
        y: table.height
        eulerRotation.y: monitorRotation
        property real monitorRotation: 0
        Behavior on monitorRotation {
            NumberAnimation { duration: 500 }
        }

        XrItem {
            id: theScreen
            y: monitor.yOffset + height
            x: -width / 2
            width: monitor.width
            height: monitor.height
            contentItem: ScreenContent {}
        }
    }
}