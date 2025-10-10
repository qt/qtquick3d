// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import QtQuick.Dialogs
import QtQuick3D
import QtQuick3D.Helpers

import QtQuick3D.lightmapviewer
import LightmapFile 1.0

ColumnLayout {
    id: root
    anchors.fill: parent

    Pane {
        Layout.alignment: Qt.AlignLeft
        Layout.preferredHeight: 30
        padding: 8

        RowLayout {
            anchors.fill: parent
            spacing: 8

            CheckBox {
                id: cbWasd
                text: "WASD controller"
            }
            CheckBox {
                id: cbBackfaceCulling
                text: "Backface Culling"
                checked: true
            }
            CheckBox {
                id: cbDebugUv
                text: "Debug UV"
            }
            CheckBox {
                id: cbIgnoreScale
                text: "Ignore scale"
                onCheckedChanged: view.recomputeBounds()
            }
        }
    }

    View3D {
        id: view
        Layout.fillWidth: true
        Layout.fillHeight: true

        camera: cbWasd.checked ? wasdCamera : arcballCamera

        property var origBounds: null
        property real boundsDiameter: 0
        property vector3d boundsCenter
        property vector3d boundsSize

        function scaledBounds(orig, scaleVec) {
            if (!orig)
                return

            function mul(v, s) {
                return Qt.vector3d(v.x * s.x, v.y * s.y, v.z * s.z)
            }
            const a = mul(orig.minimum, scaleVec)
            const b = mul(orig.maximum, scaleVec)
            const min = Qt.vector3d(Math.min(a.x, b.x), Math.min(a.y, b.y),
                                    Math.min(a.z, b.z))
            const max = Qt.vector3d(Math.max(a.x, b.x), Math.max(a.y, b.y),
                                    Math.max(a.z, b.z))
            return {
                "minimum": min,
                "maximum": max
            }
        }

        function currentScale() {
            return cbIgnoreScale.checked ? Qt.vector3d(
                                               1, 1,
                                               1) : LightmapFile.getAppliedScaleFor(
                                               selectedKey)
        }

        function recomputeBounds() {
            if (!origBounds)
                return

            const s = currentScale()
            const sb = scaledBounds(origBounds, s)

            boundsSize = Qt.vector3d(sb.maximum.x - sb.minimum.x,
                                     sb.maximum.y - sb.minimum.y,
                                     sb.maximum.z - sb.minimum.z)
            boundsDiameter = Math.max(boundsSize.x, boundsSize.y, boundsSize.z)
            boundsCenter = Qt.vector3d((sb.maximum.x + sb.minimum.x) / 2,
                                       (sb.maximum.y + sb.minimum.y) / 2,
                                       (sb.maximum.z + sb.minimum.z) / 2)

            model.position = Qt.vector3d(-boundsCenter.x, -boundsCenter.y,
                                         -boundsCenter.z)

            arcballCamera.clipNear = Math.max(0.001, boundsDiameter / 100)
            arcballCamera.clipFar = Math.max(arcballCamera.clipNear + 1.0,
                                             boundsDiameter * 10)
            wasdCamera.clipNear = Math.max(0.001, boundsDiameter / 100)
            wasdCamera.clipFar = Math.max(wasdCamera.clipNear + 1.0,
                                          boundsDiameter * 10)

            wasdController.speed = boundsDiameter / 1000.0
            wasdController.shiftSpeed = 3 * wasdController.speed

            resetCamera()
        }

        function updateBounds(bounds) {
            origBounds = bounds
            recomputeBounds()
        }

        function resetCamera() {
            arcballCamera.position = Qt.vector3d(0, 0,
                                                 2 * Math.max(0.001,
                                                              boundsDiameter))
            arcballCamera.eulerRotation = Qt.vector3d(0, 0, 0)
            wasdCamera.position = Qt.vector3d(0, 0,
                                              2 * Math.max(0.001,
                                                           boundsDiameter))
            wasdCamera.eulerRotation = Qt.vector3d(0, 0, 0)
        }

        environment: SceneEnvironment {
            backgroundMode: SceneEnvironment.Color
            clearColor: "darkslategray"
        }

        PerspectiveCamera {
            id: arcballCamera
            z: 300
        }

        PerspectiveCamera {
            id: wasdCamera
            z: 300
        }

        Node {
            id: modelNode

            Model {
                id: model

                scale: view.currentScale()
                onScaleChanged: view.recomputeBounds()

                geometry: LightmapMesh {
                    source: LightmapFile.source
                    key: meshKey
                    onBoundsChanged: view.updateBounds(bounds)
                }
                materials: CustomMaterial {
                    shadingMode: CustomMaterial.Unshaded
                    cullMode: cbBackfaceCulling.checked ? Material.BackFaceCulling : Material.NoCulling

                    property TextureInput baseMap: TextureInput {
                        texture: Texture {
                            tilingModeHorizontal: Texture.ClampToEdge
                            tilingModeVertical: Texture.ClampToEdge

                            textureData: LightmapFile.textureDataFor(
                                             selectedKey, selectedTextureTag)
                        }
                    }
                    property bool debugUV: cbDebugUv.checked

                    vertexShader: "mesh.vert"
                    fragmentShader: "mesh.frag"
                }
            }
        }

        ArcballController {
            id: arcballController
            controlledObject: modelNode
            camera: arcballCamera
            enabled: !cbWasd.checked

            function jumpToAxis(axis) {
                cameraRotation.from = arcballController.controlledObject.rotation
                cameraRotation.to = originGizmo.quaternionForAxis(
                            axis, arcballController.controlledObject.rotation)
                cameraRotation.duration = 200
                cameraRotation.start()
            }

            function jumpToRotation(qRotation) {
                cameraRotation.from = arcballController.controlledObject.rotation
                cameraRotation.to = qRotation
                cameraRotation.duration = 200
                cameraRotation.start()
            }

            QuaternionAnimation {
                id: cameraRotation
                target: arcballController.controlledObject
                property: "rotation"
                type: QuaternionAnimation.Slerp
                running: false
                loops: 1
            }
        }

        WasdController {
            id: wasdController
            controlledObject: wasdCamera
            enabled: cbWasd.checked
        }

        OriginGizmo {
            id: originGizmo
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.margins: 10
            width: 120
            height: 120
            targetNode: modelNode
            onAxisClicked: axis => {
                               arcballController.jumpToAxis(axis)
                           }
        }

        Keys.onPressed: event => {
                            if (!arcballController.enabled) return

                            if (event.key === Qt.Key_Space) {
                                let rotation = originGizmo.quaternionAlign(
                                    arcballController.controlledObject.rotation)
                                arcballController.jumpToRotation(rotation)
                            } else if (event.key === Qt.Key_A) {
                                let rotation = originGizmo.rotateYawLeft(
                                    arcballController.controlledObject.rotation)
                                arcballController.jumpToRotation(rotation)
                            } else if (event.key === Qt.Key_D) {
                                let rotation = originGizmo.rotateYawRight(
                                    arcballController.controlledObject.rotation)
                                arcballController.jumpToRotation(rotation)
                            } else if (event.key === Qt.Key_W) {
                                let rotation = originGizmo.rotatePitchUp(
                                    arcballController.controlledObject.rotation)
                                arcballController.jumpToRotation(rotation)
                            } else if (event.key === Qt.Key_S) {
                                let rotation = originGizmo.rotatePitchDown(
                                    arcballController.controlledObject.rotation)
                                arcballController.jumpToRotation(rotation)
                            } else if (event.key === Qt.Key_Q) {
                                let rotation = originGizmo.rotateRollLeft(
                                    arcballController.controlledObject.rotation)
                                arcballController.jumpToRotation(rotation)
                            } else if (event.key === Qt.Key_E) {
                                let rotation = originGizmo.rotateRollRight(
                                    arcballController.controlledObject.rotation)
                                arcballController.jumpToRotation(rotation)
                            }
                        }
    }
}
