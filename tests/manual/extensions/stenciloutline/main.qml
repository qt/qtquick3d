// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QtQuick3D
import QtQuick3D.Helpers
import QtQuick3DTest.OutlineRenderExtension

Window {
    width: 640
    height: 480
    visible: true
    title: qsTr("Hello World")

    RowLayout {
        anchors.top: parent.top
        height: 100
        Label {
            text: "Color"
            Layout.fillWidth: true
        }
        ColorPicker {
            id: colorPicker
            color: "blue"
        }
        Slider {
            value: 1.05
            from: 0
            to: 3
            onValueChanged: outlineRenderer.outlineScale = value
        }
    }

    View3D {
        id: view3d
        anchors.topMargin: 100
        objectName: "view3d"
        anchors.fill: parent
        extensions: [ OutlineRenderExtension {
                id: outlineRenderer
                outlineMaterial: outlineMaterial
            }
        ]

        PerspectiveCamera {
            id: camera
            z: 600
        }

        DirectionalLight {

        }

        DirectionalLight {
            eulerRotation: Qt.vector3d(0, 180, 0)
            position.z: -600
        }
        PrincipledMaterial {
            id: outlineMaterial
            baseColor: colorPicker.color
            lighting: PrincipledMaterial.NoLighting
        }

        Model {
            id: modelA
            position.x: 100
            scale: Qt.vector3d(100, 100, 100)
            source: "models/distortedcube.mesh"
            pickable: true
            materials: PrincipledMaterial {
                id: modelAMaterial0
                baseColor: "green"
            }

            NumberAnimation on eulerRotation.y {
                running: true
                duration: 8000
                from: 0
                to: 360
                loops: Animation.Infinite
            }
        }

        Model {
            id: modelB
            position.x: -100
            source: "#Sphere"
            pickable: true
            materials: PrincipledMaterial {
                baseColor: "red"
            }
        }
    }

    Item {
        width: debugViewToggleText.implicitWidth
        height: debugViewToggleText.implicitHeight
        anchors.right: parent.right
        Label {
            id: debugViewToggleText
            text: "Click here " + (dbg.visible ? "to hide DebugView" : "for DebugView")
            anchors.right: parent.right
            anchors.top: parent.top
        }
        MouseArea {
            anchors.fill: parent
            onClicked: dbg.visible = !dbg.visible
            DebugView {
                y: debugViewToggleText.height * 2
                anchors.right: parent.right
                source: view3d
                id: dbg
                visible: false
            }
        }
    }

    WasdController {
        id: wasdController
        mouseEnabled: !colorPicker.popupVisible && !dbg.visible
        anchors.fill: view3d
        controlledObject: camera
    }

    MouseArea {
        enabled: !colorPicker.popupVisible && !dbg.visible
        anchors.fill: view3d
        onClicked: (mouse)=> {
            let hit = view3d.pick(mouse.x, mouse.y)
//            console.log("Pick-result: " + hit)
            outlineRenderer.target = hit.objectHit
        }
    }
}
