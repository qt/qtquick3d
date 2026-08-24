// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers

// Doc image: the axes and unit of a Qt Quick 3D scene under an XrView.
// Everything is drawn from real geometry at real coordinates, so the picture
// cannot disagree with the numbers in the text.
Diagram {
    id: win
    width: 1000
    height: 620
    imageName: "qt3dxr-coordinate-systems"
    grabTarget: root

    readonly property int step: 100
    readonly property int extent: 250
    readonly property int axisLen: 330

    Item {
        id: root
        anchors.fill: parent

        View3D {
            id: view
            anchors.fill: parent
            camera: cam
            environment: SceneEnvironment {
                clearColor: "white"
                backgroundMode: SceneEnvironment.Color
                antialiasingMode: SceneEnvironment.MSAA
                antialiasingQuality: SceneEnvironment.VeryHigh
            }

            Node { id: subject; position: Qt.vector3d(0, 60, 0) }

            LookAtNode {
                position: Qt.vector3d(520, 470, 640)
                target: subject
                PerspectiveCamera { id: cam; fieldOfView: 40 }
            }

            DirectionalLight { eulerRotation: Qt.vector3d(-40, -35, 0); brightness: 1.1 }
            DirectionalLight { eulerRotation: Qt.vector3d(30, 140, 0); brightness: 0.45 }

            // Floor grid, one square per 100 units.
            Repeater3D {
                model: (2 * win.extent / win.step) + 1
                Model {
                    required property int index
                    readonly property int at: -win.extent + index * win.step
                    source: "#Cube"
                    position: Qt.vector3d(0, 0, at)
                    scale: Qt.vector3d(2 * win.extent / 100, 0.004, 0.004)
                    materials: PrincipledMaterial {
                        baseColor: at === 0 ? "#aeb4ba" : "#dce0e4"
                        lighting: PrincipledMaterial.NoLighting
                    }
                }
            }
            Repeater3D {
                model: (2 * win.extent / win.step) + 1
                Model {
                    required property int index
                    readonly property int at: -win.extent + index * win.step
                    source: "#Cube"
                    position: Qt.vector3d(at, 0, 0)
                    scale: Qt.vector3d(0.004, 0.004, 2 * win.extent / 100)
                    materials: PrincipledMaterial {
                        baseColor: at === 0 ? "#aeb4ba" : "#dce0e4"
                        lighting: PrincipledMaterial.NoLighting
                    }
                }
            }

            // Axes. A #Cube is 100 units across, so a rod from the origin to
            // len needs scale len/100 and its center at len/2.
            component Axis : Model {
                required property vector3d dir
                required property color tint
                property int len: win.axisLen
                source: "#Cube"
                position: Qt.vector3d(dir.x * len / 2, dir.y * len / 2, dir.z * len / 2)
                scale: Qt.vector3d(dir.x !== 0 ? len / 100 : 0.016,
                                   dir.y !== 0 ? len / 100 : 0.016,
                                   dir.z !== 0 ? len / 100 : 0.016)
                materials: PrincipledMaterial { baseColor: tint; lighting: PrincipledMaterial.NoLighting }
            }
            Axis { dir: Qt.vector3d(1, 0, 0); tint: "#d0353a" }
            Axis { dir: Qt.vector3d(0, 1, 0); tint: "#2f9e44" }
            Axis { dir: Qt.vector3d(0, 0, 1); tint: "#1c6fb8" }

            component Tip : Model {
                required property vector3d dir
                required property color tint
                source: "#Cone"
                position: Qt.vector3d(dir.x * win.axisLen, dir.y * win.axisLen, dir.z * win.axisLen)
                scale: Qt.vector3d(0.13, 0.26, 0.13)
                eulerRotation: dir.y !== 0 ? Qt.vector3d(0, 0, 0)
                             : dir.x !== 0 ? Qt.vector3d(0, 0, -90)
                                           : Qt.vector3d(90, 0, 0)
                materials: PrincipledMaterial { baseColor: tint; lighting: PrincipledMaterial.NoLighting }
            }
            Tip { dir: Qt.vector3d(1, 0, 0); tint: "#d0353a" }
            Tip { dir: Qt.vector3d(0, 1, 0); tint: "#2f9e44" }
            Tip { dir: Qt.vector3d(0, 0, 1); tint: "#1c6fb8" }

//! [unit cube]
            // One built-in cube at scale 1. The mesh is 100 units across, so
            // this is a 1 m cube, and its center has to sit at y = 50 for the
            // cube to rest on the floor.
            Model {
                source: "#Cube"
                position: Qt.vector3d(0, 50, 0)
                materials: PrincipledMaterial { baseColor: "#41cd52"; roughness: 0.45 }
            }
//! [unit cube]

            Model {
                source: "#Sphere"
                scale: Qt.vector3d(0.13, 0.13, 0.13)
                materials: PrincipledMaterial { baseColor: "#17181a"; lighting: PrincipledMaterial.NoLighting }
            }
        }

        // 2D labels pinned to projected 3D positions.
        component Tag : Text {
            required property vector3d at
            property real dx: 0
            property real dy: 0
            property color tint: "#17181a"
            property bool strong: true
            readonly property vector3d p: (win.tick, view.mapFrom3DScene(at))
            x: p.x - implicitWidth / 2 + dx
            y: p.y - implicitHeight / 2 + dy
            color: tint
            font.pixelSize: 20
            font.bold: strong
            font.family: "Segoe UI"
        }

        Tag { at: Qt.vector3d(win.axisLen, 0, 0); dx: 34; dy: 14; text: "+x"; tint: "#d0353a" }
        Tag { at: Qt.vector3d(0, win.axisLen, 0); dy: -42; text: "+y"; tint: "#2f9e44" }
        Tag { at: Qt.vector3d(0, 0, win.axisLen); dx: -30; dy: 16; text: "+z"; tint: "#1c6fb8" }

        Tag {
            at: Qt.vector3d(90, 105, 0); dx: 96
            text: "#Cube at scale 1\n100 units = 1 m"
            horizontalAlignment: Text.AlignLeft
        }
        Tag {
            at: Qt.vector3d(-120, 0, 210); dy: 16
            text: "one grid square = 100 units = 1 m"
            tint: "#5c6066"; strong: false
        }
        Tag {
            at: Qt.vector3d(215, 0, -95); dx: 20; dy: -8
            text: "y = 0 is the floor"
            tint: "#5c6066"; strong: false
        }

        Text {
            x: 18; y: win.height - 54
            font { pixelSize: 23; bold: true; family: "Segoe UI" }
            color: "#17181a"
            text: "3D scene under an XrView: right-handed, +y up, 1 unit = 1 cm"
        }
        Text {
            x: 18; y: win.height - 30
            font { pixelSize: 18; family: "Segoe UI" }
            color: "#5c6066"
            text: "The viewer looks along \u2212z."
        }
    }
}
