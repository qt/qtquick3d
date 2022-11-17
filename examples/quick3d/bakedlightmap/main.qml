// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick3D
import QtQuick3D.Helpers

Window {
    width: 1280
    height: 720
    title: "Qt Quick 3D Baked Lightmap Example"
    visible: true

    View3D {
        id: root
        anchors.fill: parent

        environment: SceneEnvironment {
            backgroundMode: SceneEnvironment.Color
            clearColor: "black"
        }

        PerspectiveCamera {
            id: camera
            z: 300
            y: 100
        }

        property bool lmEnabled: lmToggle.checked
        property int lightBakeMode: lmToggle.checked ? Light.BakeModeAll : Light.BakeModeDisabled

        //! [light]
        PointLight {
            bakeMode: root.lightBakeMode
            y: 190
            brightness: 5
            castsShadow: true
            shadowFactor: 75
        }
        //! [light]

        //! [model]
        Box {
            usedInBakedLighting: true
            lightmapBaseResolution: 256
            bakedLightmap: BakedLightmap {
                enabled: root.lmEnabled
                key: "box"
                loadPrefix: "qrc:/"
            }
            scale: Qt.vector3d(100, 100, 100)
        }
        //! [model]

        Rectangle {
            color: "lightGray"
            width: 300
            height: 80
            ColumnLayout {
                anchors.centerIn: parent
                CheckBox {
                    id: lmToggle
                    text: "Use lightmaps\n(fully baked direct+indirect)"
                    checked: true
                    focusPolicy: Qt.NoFocus
                }
                Text {
                    text: "Run with --bake-lightmaps to rebake"
                }
            }
        }
    }

    Item {
        width: Math.min(100, dbg.width)
        height: Math.min(100, dbg.height)
        anchors.right: parent.right
        Text {
            text: "Click here for DebugView"
            font.pointSize: 8
            color: "white"
            anchors.right: parent.right
            anchors.top: parent.top
            visible: !dbg.visible
        }
        MouseArea {
            anchors.fill: parent
            onClicked: dbg.visible = !dbg.visible
            DebugView {
                anchors.right: parent.right
                source: root
                id: dbg
                visible: false
                resourceDetailsVisible: true
            }
        }
    }
}
