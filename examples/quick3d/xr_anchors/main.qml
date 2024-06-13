// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

import QtQuick3D
import QtQuick3D.Helpers

import QtQuick3D.Xr
pragma ComponentBehavior: Bound

XrView {
    id: xrView
    referenceSpace: XrView.ReferenceSpaceStage

    enableDepthSubmission: true

    property list<color> colorTable: [Qt.rgba(1, 0, 1, 1), // magenta
                                      Qt.rgba(0, 1, 0, 1), // green
                                      Qt.rgba(0, 0, 1, 1), // blue
                                      Qt.rgba(1, 0, 0, 1), // red
                                      Qt.rgba(0.5, 0.5, 0, 1), // darkYellow
                                      Qt.rgba(0.5, 0, 0, 1), // darkRed
                                      Qt.rgba(0, 0.5, 0.5, 1), // darkCyan
                                      Qt.rgba(1, 1, 0, 1)] // yellow

    function colorForClassification(name) {
        const index = ["door", "window", "ceiling", "floor", "wall", "table", "seat", "unknown"].indexOf(name.toLowerCase())
        return colorTable[index > 0 ? index : 0]
    }

    property bool preferPassthrough: true
    enablePassthrough: passthroughSupported && preferPassthrough

    XrErrorDialog { id: err }
    onInitializeFailed: (errorString) => {
                            console.log("===================>>>>>>" + errorString)
                            err.run("XRView", errorString)
                        }

    environment: SceneEnvironment {
        id: sceneEnvironment
        lightProbe: Texture {
            textureData: ProceduralSkyTextureData {
            }
        }
        antialiasingMode: SceneEnvironment.MSAA
        antialiasingQuality: SceneEnvironment.High
        backgroundMode: xrView.enablePassthrough ? SceneEnvironment.Transparent : SceneEnvironment.Color
        clearColor: "skyblue"
        probeHorizon: 0.5
    }

    XrOrigin {
        id: theOrigin

        property vector3d headPos: camera.position

        Component.onCompleted: {
            camera.setClipFar(10000)
            camera.setClipNear(10)
        }

        XrController {
            id: rightController
            controller: XrController.ControllerRight

            Component.onCompleted: handInput.poseSpace = XrHandInput.AimPose

            XrActionMapper {
                XrInputAction {
                    actionId: [XrActionMapper.Button2Pressed, XrActionMapper.MiddleFingerPinch]
                    onTriggered: xrView.preferPassthrough = !xrView.preferPassthrough
                }
            }

            onRotationChanged: {
                const pickResult = xrView.rayPick(scenePosition, forward)
                if (pickResult.hitType === PickResult.Model) {
                    pickRay.hit = true
                    pickRay.length = pickResult.distance
                    movableObject.move(pickResult.scenePosition, pickResult.sceneNormal)
                    movableObject.visible = true
                    const info = pickResult.objectHit?.anchorInfo
                    labelNode.anchorInfo = info ? info : "(unknown)"
                } else {
                    pickRay.hit = false
                    pickRay.length = 50
                    labelNode.anchorInfo = "(no anchor)"
                }
            }
            Node {
                id: pickRay
                property real length: 50
                property bool hit: false

                z: -length/2
                Model {
                    eulerRotation.x: 90
                    scale: Qt.vector3d(0.02, pickRay.length/100, 0.02)
                    source: "#Cylinder"
                    materials: PrincipledMaterial { baseColor: pickRay.hit ? "green" : "gray" }
                    opacity: 0.5
                }
            }
        }
    }

    Node {
        id: movableObject

        // This object is positioned on anchors where the ray hits

        visible: false

        function move(pos: vector3d, normal: vector3d) {
            const rot = Quaternion.lookAt(pos, pos.plus(normal));
            position = pos
            rotation = rot
        }

        Model {
            scale: Qt.vector3d(0.5, 0.05, 0.5)
            eulerRotation.x: 90
            z: -2.5
            source: "#Cylinder"
            materials: PrincipledMaterial {
                baseColor: "red"
                roughness: 0.5
            }
        }
    }

    Node {
        id: labelNode
        position: rightController.position
        rotation: rightController.rotation

        property int numAnchors: spatialAnchors.count
        property string anchorInfo: "(no anchor)"

        Node {
            y: 15
            x: -15
            scale: Qt.vector3d(0.1, 0.1, 0.1)
            Rectangle {
                width: 300
                height: 100
                color: Qt.rgba(1,0.9,0.8,0.7)
                radius: 10
                border.width: 2
                border.color: "blue"
                Text {
                    anchors.fill: parent
                    anchors.margins: 10
                    textFormat: Text.StyledText
                    text: "Total anchors: " + labelNode.numAnchors + "<br>" + "Selected: " + labelNode.anchorInfo
                }
            }
        }
    }

    Repeater3D {
        id: spatialAnchors
        model: XrSpatialAnchorModel {
        }
        Node {
            id: anchorNode
            required property XrSpatialAnchor anchor
            required property int index
            position: anchor.position
            rotation: anchor.rotation

            Model {
                pickable: true
                z: anchorNode.anchor.has3DBounds ? anchorNode.anchor.offset3D.z / 2 * 100 : 0 // Position is center of 2D surface also for 3D anchors
                scale: anchorNode.anchor.has3DBounds ? anchorNode.anchor.extent3D : Qt.vector3d(anchorNode.anchor.extent2D.x, anchorNode.anchor.extent2D.y, 0.01)
                materials: PrincipledMaterial {
                    // Make anchor objects invisible in passthrough mode
                    baseColor: xrView.enablePassthrough ? Qt.rgba(0, 0, 0, 0) : colorForClassification(anchor.semanticLabels.split(",")[0])
                    alphaMode: xrView.enablePassthrough ? PrincipledMaterial.Blend : PrincipledMaterial.Opaque
                    roughness: 0.7
                }
                source: anchorNode.anchor.has3DBounds ? "#Cube" : "#Rectangle"
                property string anchorInfo: "anchor #" + anchorNode.index + ", " + anchorNode.anchor.semanticLabels
            }

            Model {
                // Visualize anchor orientation
                materials: PrincipledMaterial {
                    baseColor: anchorNode.anchor.has3DBounds ? anchorNode.anchor.has2DBounds ? "green" : "red" : "blue"
                }
                scale: Qt.vector3d(0.05, 0.05, 0.05)
                source: "#Cube"

                Model {
                    materials: PrincipledMaterial {
                        baseColor: "black"
                    }
                    scale: Qt.vector3d(0.1, 3, 0.1)
                    source: "#Cube"
                    y: 150
                }
                Model {
                    materials: PrincipledMaterial {
                        baseColor: "white"
                    }
                    scale: Qt.vector3d(3, 0.1, 0.1)
                    source: "#Cube"
                    x: 150
                }
            }
            visible: anchor.has2DBounds || anchor.has3DBounds
        }
    }
}
