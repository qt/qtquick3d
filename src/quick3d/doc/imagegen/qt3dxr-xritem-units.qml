// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers

// Doc image: how an XrItem relates centimeters to pixels.
//
// XrItem parents a Rectangle to a Node, sets its transform origin to TopLeft,
// sizes it width*ppu by height*ppu pixels and scales it by 1/ppu. This scene
// does the same thing by hand, so the render shows the real behaviour: a
// 500x300 px content item on a 50x30 cm surface, hanging right and down from
// the node origin.
Diagram {
    id: win
    width: 1080
    height: 560
    imageName: "qt3dxr-xritem-units"
    grabTarget: root

    readonly property real surfaceW: 50      // cm
    readonly property real surfaceH: 30      // cm
    readonly property real ppu: 10           // pixels per unit
    readonly property real nodeY: 130        // cm above the floor

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

            Node { id: subject; position: Qt.vector3d(win.surfaceW / 2, win.nodeY - win.surfaceH / 2, 0) }

            LookAtNode {
                position: Qt.vector3d(0, win.nodeY + 3, 74)
                target: subject
                PerspectiveCamera { id: cam; fieldOfView: 38; clipNear: 1 }
            }

            DirectionalLight { eulerRotation: Qt.vector3d(-35, -25, 0); brightness: 1.1 }
            DirectionalLight { eulerRotation: Qt.vector3d(15, 150, 0); brightness: 0.5 }

            // Floor, at 50 cm spacing, purely for a sense of real-world scale.
            Repeater3D {
                model: 9
                Model {
                    required property int index
                    readonly property int at: -200 + index * 50
                    source: "#Cube"
                    position: Qt.vector3d(50, 0, at)
                    scale: Qt.vector3d(4.0, 0.006, 0.006)
                    materials: PrincipledMaterial { baseColor: "#dfe3e7"; lighting: PrincipledMaterial.NoLighting }
                }
            }
            Repeater3D {
                model: 9
                Model {
                    required property int index
                    readonly property int at: -150 + index * 50
                    source: "#Cube"
                    position: Qt.vector3d(at, 0, -0)
                    scale: Qt.vector3d(0.006, 0.006, 4.0)
                    materials: PrincipledMaterial { baseColor: "#dfe3e7"; lighting: PrincipledMaterial.NoLighting }
                }
            }

            // A Node with a 2D child is what XrItem builds internally: the
            // content item is sized in pixels, scaled by 1 / pixelsPerUnit,
            // and pinned by its top-left corner to the node's origin.
            Node {
                position: Qt.vector3d(0, win.nodeY, 0)

                Rectangle {
                    width: win.surfaceW * win.ppu     // 50 * 10 = 500 px
                    height: win.surfaceH * win.ppu    // 30 * 10 = 300 px
                    scale: 1 / win.ppu                // 0.1
                    transformOrigin: Item.TopLeft
                    color: "#ffffff"
                    border { width: 3; color: "#41cd52" }

                    SurfaceContent { anchors { fill: parent; margins: 26 } }
                }
            }

            // Edge rods marking the surface extents, and the node origin.
            component Edge : Model {
                required property vector3d from
                required property vector3d to
                source: "#Cube"
                position: Qt.vector3d((from.x + to.x) / 2, (from.y + to.y) / 2, (from.z + to.z) / 2)
                readonly property real len: Math.hypot(to.x - from.x, to.y - from.y, to.z - from.z)
                scale: Qt.vector3d(to.x !== from.x ? len / 100 : 0.008,
                                   to.y !== from.y ? len / 100 : 0.008,
                                   0.008)
                materials: PrincipledMaterial { baseColor: "#d81b60"; lighting: PrincipledMaterial.NoLighting }
            }
            // Top edge, from the node origin along +x.
            Edge {
                from: Qt.vector3d(0, win.nodeY + 6, 0)
                to: Qt.vector3d(win.surfaceW, win.nodeY + 6, 0)
            }
            // Left edge, from the node origin downwards.
            Edge {
                from: Qt.vector3d(-6, win.nodeY, 0)
                to: Qt.vector3d(-6, win.nodeY - win.surfaceH, 0)
            }

            Model {
                source: "#Sphere"
                position: Qt.vector3d(0, win.nodeY, 0)
                scale: Qt.vector3d(0.022, 0.022, 0.022)
                materials: PrincipledMaterial { baseColor: "#17181a"; lighting: PrincipledMaterial.NoLighting }
            }
        }

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

        Tag {
            at: Qt.vector3d(0, win.nodeY, 0); dx: -8; dy: -30
            text: "node origin"
        }
        Tag {
            at: Qt.vector3d(win.surfaceW / 2, win.nodeY + 6, 0); dy: -26
            text: "width: 50  \u2192  50 cm"; tint: "#d81b60"
        }
        Tag {
            at: Qt.vector3d(-6, win.nodeY - win.surfaceH / 2, 0); dx: -78
            text: "height: 30\n\u2192 30 cm"; tint: "#d81b60"
            horizontalAlignment: Text.AlignRight
        }

        Text {
            x: 18; y: win.height - 76
            font { pixelSize: 23; bold: true; family: "Segoe UI" }
            color: "#17181a"
            text: "XrItem: centimeters outside, pixels inside"
        }
        Text {
            x: 18; y: win.height - 52
            font { pixelSize: 18; family: "Segoe UI" }
            color: "#5c6066"
            text: "500 px over 50 units, so pixelsPerUnit = 10: one centimeter of surface holds ten pixels of content."
        }
        Text {
            x: 18; y: win.height - 30
            font { pixelSize: 18; family: "Segoe UI" }
            color: "#5c6066"
            text: "The surface hangs right and down from the node origin, because the content keeps Qt Quick's downward y axis."
        }
    }
}
