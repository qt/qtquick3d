// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers

// One reference-space panel. The scene origin is always the render origin, so
// the physical world is what moves: floorY says how far below the origin the
// real floor sits. That is exactly what choosing a reference space does.
Item {
    id: panel

    required property string title
    required property string subtitle
    required property string footnote
    // Height of the real floor in scene coordinates. 0 puts the origin on the
    // floor; -eyeHeight puts it at the user's default view position.
    required property real floorY
    property real userX: 0
    property bool showPlayArea: false
    // In ReferenceSpaceLocal the origin coincides with the eye position, so the
    // two labels would sit on top of each other.
    property bool showEyeLabel: true
    property string originLabel: "(0, 0, 0)"
    property real originDx: -46
    property real originDy: 0
    property real areaHalf: 110
    property int tick: 0

    // Mannequin proportions in centimeters, 175 cm tall.
    readonly property real eyeHeight: 163
    readonly property int extent: 200
    readonly property int step: 50

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

        Node { id: subject; position: Qt.vector3d(0, panel.floorY + 85, 0) }

        LookAtNode {
            position: Qt.vector3d(150, panel.floorY + 190, 520)
            target: subject
            PerspectiveCamera { id: cam; fieldOfView: 46 }
        }

        DirectionalLight { eulerRotation: Qt.vector3d(-35, -30, 0); brightness: 1.15 }
        DirectionalLight { eulerRotation: Qt.vector3d(20, 150, 0); brightness: 0.4 }

        // The real floor, drawn as a grid at 50 cm spacing.
        Node {
            y: panel.floorY
            Repeater3D {
                model: (2 * panel.extent / panel.step) + 1
                Model {
                    required property int index
                    readonly property int at: -panel.extent + index * panel.step
                    source: "#Cube"
                    position: Qt.vector3d(0, 0, at)
                    scale: Qt.vector3d(2 * panel.extent / 100, 0.005, 0.005)
                    materials: PrincipledMaterial { baseColor: "#d3d8dc"; lighting: PrincipledMaterial.NoLighting }
                }
            }
            Repeater3D {
                model: (2 * panel.extent / panel.step) + 1
                Model {
                    required property int index
                    readonly property int at: -panel.extent + index * panel.step
                    source: "#Cube"
                    position: Qt.vector3d(at, 0, 0)
                    scale: Qt.vector3d(0.005, 0.005, 2 * panel.extent / 100)
                    materials: PrincipledMaterial { baseColor: "#d3d8dc"; lighting: PrincipledMaterial.NoLighting }
                }
            }

            // Play area outline, a square of side 2 * areaHalf.
            Repeater3D {
                model: panel.showPlayArea ? [ -panel.areaHalf, panel.areaHalf ] : []
                Node {
                    required property real modelData
                    Model {
                        source: "#Cube"
                        position: Qt.vector3d(0, 1, modelData)
                        scale: Qt.vector3d(2 * panel.areaHalf / 100, 0.012, 0.012)
                        materials: PrincipledMaterial { baseColor: "#1c6fb8"; lighting: PrincipledMaterial.NoLighting }
                    }
                    Model {
                        source: "#Cube"
                        position: Qt.vector3d(modelData, 1, 0)
                        scale: Qt.vector3d(0.012, 0.012, 2 * panel.areaHalf / 100)
                        materials: PrincipledMaterial { baseColor: "#1c6fb8"; lighting: PrincipledMaterial.NoLighting }
                    }
                }
            }
        }

        // The user: a blocky mannequin standing on the floor, 175 cm tall.
        Node {
            x: panel.userX
            y: panel.floorY
            component Part : Model {
                required property real bottom
                required property real top
                required property real wide
                required property real deep
                source: "#Cube"
                position: Qt.vector3d(0, (bottom + top) / 2, 0)
                scale: Qt.vector3d(wide / 100, (top - bottom) / 100, deep / 100)
                materials: PrincipledMaterial { baseColor: "#c9cfd5"; roughness: 0.6 }
            }
            Part { bottom: 0;   top: 85;  wide: 32; deep: 22 }   // legs
            Part { bottom: 85;  top: 145; wide: 42; deep: 24 }   // torso
            Part { bottom: 145; top: 175; wide: 22; deep: 22 }   // head

            // Eye line, marking the default view position.
            Model {
                source: "#Cube"
                position: Qt.vector3d(0, panel.eyeHeight, 12)
                scale: Qt.vector3d(0.24, 0.015, 0.015)
                materials: PrincipledMaterial { baseColor: "#41cd52"; lighting: PrincipledMaterial.NoLighting }
            }
        }

        // The scene origin, and a 20 cm content cube sitting at scene y = 0.
        Model {
            source: "#Sphere"
            scale: Qt.vector3d(0.16, 0.16, 0.16)
            materials: PrincipledMaterial { baseColor: "#17181a"; lighting: PrincipledMaterial.NoLighting }
        }
        Model {
            source: "#Cube"
            position: Qt.vector3d(90, 0, 0)
            scale: Qt.vector3d(0.2, 0.2, 0.2)
            materials: PrincipledMaterial { baseColor: "#41cd52"; roughness: 0.45 }
        }
    }

    component Tag : Text {
        required property vector3d at
        property real dx: 0
        property real dy: 0
        property color tint: "#17181a"
        property bool strong: true
        readonly property vector3d p: (panel.tick, view.mapFrom3DScene(at))
        x: p.x - implicitWidth / 2 + dx
        y: p.y - implicitHeight / 2 + dy
        color: tint
        font.pixelSize: 19
        font.bold: strong
        font.family: "Segoe UI"
    }

    Tag { at: Qt.vector3d(0, 0, 0); dx: panel.originDx; dy: panel.originDy; text: panel.originLabel }
    Tag {
        at: Qt.vector3d(90, 0, 0); dx: 6; dy: -34
        text: "model at y = 0"; tint: "#2f9e44"
    }
    Tag {
        at: Qt.vector3d(panel.userX, panel.floorY + panel.eyeHeight, 12); dx: -62
        text: "eye height"; tint: "#5c6066"; strong: false
        visible: panel.showEyeLabel
    }

    Text {
        x: panel.width - implicitWidth - 14
        y: 12
        font { pixelSize: 16; family: "Segoe UI" }
        color: "#8b9198"
        text: "grid 50 cm"
    }

    Text {
        x: 14; y: 10
        font { pixelSize: 21; bold: true; family: "Segoe UI" }
        color: "#17181a"
        text: panel.title
    }
    Text {
        x: 14; y: 30
        font { pixelSize: 16; family: "Segoe UI" }
        color: "#5c6066"
        text: panel.subtitle
    }
    Text {
        x: 14; y: panel.height - 26
        width: panel.width - 28
        wrapMode: Text.WordWrap
        font { pixelSize: 16; italic: true; family: "Segoe UI" }
        color: "#5c6066"
        text: panel.footnote
    }
}
