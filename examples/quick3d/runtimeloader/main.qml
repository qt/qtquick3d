// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Window
import QtQuick.Controls

import Qt.labs.platform
import Qt.labs.settings

import QtQuick3D
import QtQuick3D.Helpers
import QtQuick3D.AssetUtils

Window {
    visible: true
    width: 800
    height: 600

    property url importUrl;

    //! [base scene]
    View3D {
        id: view3D
        anchors.fill: parent
        environment: SceneEnvironment {
            backgroundMode: SceneEnvironment.SkyBox
            lightProbe: Texture {
                textureData: ProceduralSkyTextureData{}
            }
        }

        DirectionalLight {
            eulerRotation: "-30, -20, -40"
            ambientColor: "#333"
        }

        PerspectiveCamera {
            id: camera
            position: "0, 150, 400"
            Component.onCompleted: view3D.resetView()
        }

    //! [base scene]


        function resetView() {
            camera.position = "0, 150, 400"
            camera.eulerRotation = "-15, 0, 0"
            importNode.eulerRotation = "0, 0, 0"
            helper.resetScale()
            helper.resetPosition()
        }

        //! [instancing]
        RandomInstancing {
            id: instancing
            instanceCount: 30
            position: InstanceRange {
                property alias boundsDiameter: helper.boundsDiameter
                from: Qt.vector3d(-3*boundsDiameter, -3*boundsDiameter, -3*boundsDiameter);
                to: Qt.vector3d(3*boundsDiameter, 3*boundsDiameter, 3*boundsDiameter)
            }
            color: InstanceRange { from: "black"; to: "white" }
        }
        //! [instancing]

        QtObject {
            id: helper
            property real boundsDiameter: 0
            property vector3d boundsCenter
            property vector3d boundsSize
            property alias scaleFactor: wheelHandler.factor
            property alias position: importNode.position

            function resetScale() {
                wheelHandler.factor = boundsDiameter ? 300 / boundsDiameter : 1.0
            }
            function resetPosition() {
                position = Qt.vector3d(-boundsCenter.x*scaleFactor, -boundsCenter.y*scaleFactor, -boundsCenter.z*scaleFactor)
            }

            function updateBounds(bounds) {
                boundsSize = Qt.vector3d(bounds.maximum.x - bounds.minimum.x,
                                         bounds.maximum.y - bounds.minimum.y,
                                         bounds.maximum.z - bounds.minimum.z)
                boundsDiameter = Math.max(boundsSize.x, boundsSize.y, boundsSize.z)
                boundsCenter = Qt.vector3d((bounds.maximum.x + bounds.minimum.x) / 2,
                                           (bounds.maximum.y + bounds.minimum.y) / 2,
                                           (bounds.maximum.z + bounds.minimum.z) / 2 )
                console.log("Bounds changed: ", bounds.minimum, bounds.maximum,
                            " center:", boundsCenter, "diameter:", boundsDiameter)
                view3D.resetView()
            }
        }

        //! [runtimeloader]
        RuntimeLoader {
            id: importNode
            scale: Qt.vector3d(helper.scaleFactor, helper.scaleFactor, helper.scaleFactor)
            source: importUrl
            instancing: instancingButton.checked ? instancing : null
            onBoundsChanged: helper.updateBounds(bounds)
        }
        //! [runtimeloader]

        //! [bounds]
        Model {
            parent: importNode
            source: "#Cube"
            materials: PrincipledMaterial {
                baseColor: "red"
            }
            opacity: 0.2
            visible: visualizeButton.checked && importNode.status === RuntimeLoader.Success
            position: helper.boundsCenter
            scale: Qt.vector3d(helper.boundsSize.x / 100,
                               helper.boundsSize.y / 100,
                               helper.boundsSize.z / 100)
        }
        //! [bounds]

        //! [status report]
        Rectangle {
            id: messageBox
            visible: importNode.status !== RuntimeLoader.Success
            color: "red"
            width: parent.width * 0.8
            height: parent.height * 0.8
            anchors.centerIn: parent
            radius: Math.min(width, height) / 10
            opacity: 0.6
            Text {
                anchors.fill: parent
                font.pixelSize: 36
                text: "Status: " + importNode.errorString + "\nPress \"Import...\" to import a model"
                color: "white"
                wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
        //! [status report]
    }

    //! [camera control]
    WasdController {
        controlledObject: camera
    }

    WheelHandler {
        id: wheelHandler
        property real factor: 10.0
        onWheel: (event)=> {
            if (event.angleDelta.y > 0)
                factor *= 1.1
            else
                factor /= 1.1
       }
    }

    PointHandler {
        id: rotateHandler
        acceptedButtons: Qt.MiddleButton
        onPointChanged: {
            if (Math.abs(point.velocity.x) >= Math.abs(point.velocity.y))
                importNode.eulerRotation.y += point.velocity.x / 2000
            else
                importNode.eulerRotation.x += point.velocity.y / 2000
        }
    }
    //! [camera control]

    Row {
        RoundButton {
            id: importButton
            text: "Import..."
            onClicked: fileDialog.open()
            focusPolicy: Qt.NoFocus
        }
        RoundButton {
            id: resetButton
            text: "Reset view"
            onClicked: view3D.resetView()
            focusPolicy: Qt.NoFocus
        }
        RoundButton {
            id: scaleButton
            text: "Scale: " + helper.scaleFactor.toPrecision(3)
            onClicked: helper.resetScale()
            focusPolicy: Qt.NoFocus
        }
        RoundButton {
            id: visualizeButton
            checkable: true
            text: "Visualize bounds"
            focusPolicy: Qt.NoFocus
        }
        RoundButton {
            id: instancingButton
            checkable: true
            text: "Instancing"
            focusPolicy: Qt.NoFocus
        }
    }
    FileDialog {
        id: fileDialog
        nameFilters: ["glTF files (*.gltf *.glb)", "All files (*)"]
        onAccepted: importUrl = file
        Settings {
            id: fileDialogSettings
            category: "QtQuick3D.Examples.RuntimeLoader"
            property alias folder: fileDialog.folder
        }
    }
}
