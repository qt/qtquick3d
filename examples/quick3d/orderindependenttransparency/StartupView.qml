// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers
import QtQuick.Controls

Item {
    id: mainView

    required property Loader loader

    readonly property real listItemWidth: 180
    readonly property real listItemHeight: 40

    anchors.fill: parent

    ListModel {
        id: demosModel
        ListElement {
            name: "Particles"
            file: "Particles.qml"
        }
        ListElement {
            name: "Instancing"
            file: "Instancing.qml"
        }
        ListElement {
            name: "Blend Value correctness"
            file: "BlendValueTest.qml"
        }
    }


    View3D {
        id: view3D
        anchors.fill: parent
//! [scene environment]
        environment: SceneEnvironment {
            clearColor: "#000000"
            backgroundMode: SceneEnvironment.Color
            antialiasingMode: AppSettings.antialiasingMode
            antialiasingQuality: AppSettings.antialiasingQuality
            oitMethod: SceneEnvironment.OITWeightedBlended
        }
//! [scene environment]
        camera: PerspectiveCamera {
            position: Qt.vector3d(0, 0, 600)
            clipFar: 2000
        }

        PointLight {
            id: pointLight
            position: Qt.vector3d(400, 200, 400)
            brightness: 50
            ambientColor: Qt.rgba(0.5, 0.3, 0.1, 1.0)
            SequentialAnimation {
                loops: Animation.Infinite
                running: true
                NumberAnimation {
                    target: pointLight
                    property: "brightness"
                    to: 400
                    duration: 2000
                    easing.type: Easing.OutElastic
                }
                NumberAnimation {
                    target: pointLight
                    property: "brightness"
                    to: 50
                    duration: 6000
                    easing.type: Easing.InOutQuad
                }
            }
        }

        component QtCubeModel : Model {
            property alias xRotationDuration: cubeXAnim.duration
            property alias yRotationDuration: cubeYAnim.duration
            property alias materialOpacity: cubeMaterial.opacity

            source: "#Cube"
            position: Qt.vector3d(0, 0, 100)
            NumberAnimation on eulerRotation.y {
                id: cubeYAnim
                loops: Animation.Infinite
                from: 0
                to: 360
            }
            NumberAnimation on eulerRotation.x {
                id: cubeXAnim
                loops: Animation.Infinite
                from: 0
                to: 360
            }
            materials: PrincipledMaterial {
                id: cubeMaterial
                baseColorMap: Texture {
                    source: "images/qt_logo2.png"
                }
                normalMap: Texture {
                    source: "images/qt_logo2_n.png"
                }
                cullMode: Material.NoCulling
            }
        }
        QtCubeModel {
            scale: Qt.vector3d(2.0, 2.0, 2.0)
            xRotationDuration: 8000
            yRotationDuration: 10000
            materialOpacity: 0.5
        }
        QtCubeModel {
            scale: Qt.vector3d(1.0, 1.0, 1.0)
            xRotationDuration: 14000
            yRotationDuration: 20000
            materialOpacity: 0.7
        }
    }

    Component {
        id: listComponent
        Button {
            id: button
            required property string name
            required property string file

            width: mainView.listItemWidth
            height: mainView.listItemHeight
            background: Rectangle {
                id: buttonBackground
                border.width: 0.5
                border.color: "#d0808080"
                color: "#d0404040"
                opacity: button.hovered ? 1.0 : 0.5
            }
            contentItem: Text {
                anchors.centerIn: parent
                color: "#f0f0f0"
                font.pointSize: AppSettings.fontSizeSmall
                text: button.name
            }

            onClicked: {
                mainView.loader.source = button.file
            }
        }
    }

    Text {
        id: topLabel
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 20
        text: qsTr("Qt Quick 3D - Order Independent Transparency")
        color: "#f0f0f0"
        font.pointSize: AppSettings.fontSizeLarge
    }

    Row {
        anchors.top: topLabel.bottom
        anchors.topMargin: 20
        anchors.rightMargin: 50
        anchors.right: parent.right
        spacing: 20
        ListView {
            id: demosListView
            width: mainView.listItemWidth
            height: count * mainView.listItemHeight
            model: demosModel
            delegate: listComponent
        }
    }
    Pane {
        anchors.top: view3D.top
        anchors.right: parent.right
        Label {
            id: debugViewToggleText
            text: dbg.visible ? "Hide DebugView" : "Show DebugView"
            anchors.right: parent.right
            anchors.top: parent.top
            MouseArea {
                anchors.fill: parent
                onClicked: dbg.visible = !dbg.visible
                DebugView {
                    y: debugViewToggleText.height * 2
                    anchors.right: parent.right
                    source: view3D
                    id: dbg
                    visible: false
                }
            }
        }
    }
}
