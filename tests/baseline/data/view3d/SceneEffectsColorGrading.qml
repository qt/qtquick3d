// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Layouts
import QtQuick3D
import QtQuick3D.Helpers

Item {
    id: rootItem
    width: 460
    height: 460

    PerspectiveCamera {
        id: cam
        z: 600
    }

    Node {
        id: scene

        DirectionalLight {
            eulerRotation.x: -30
        }

        Model {
            source: "#Sphere"
            x: -80
            materials: PrincipledMaterial {
                baseColor: "red"
            }
        }

        Model {
            source: "#Sphere"
            x: 80
            materials: PrincipledMaterial {
                baseColor: "green"
            }
        }

        Model {
            source: "#Sphere"
            y: -80
            materials: PrincipledMaterial {
                baseColor: "blue"
            }
        }

        Model {
            source: "#Sphere"
            y: 80
            materials: PrincipledMaterial {
                baseColor: "yellow"
            }
        }
    }

    GridLayout {
        columns: 2
        anchors.fill: parent

        // No LUT — baseline reference
        View3D {
            implicitHeight: rootItem.height / 2
            implicitWidth: rootItem.width / 2
            environment: ExtendedSceneEnvironment {
                clearColor: "#334466"
                backgroundMode: SceneEnvironment.Color
                lutEnabled: false
            }
            camera: cam
            importScene: scene
        }

        // Identity LUT at full strength — should look identical to no-LUT
        View3D {
            implicitHeight: rootItem.height / 2
            implicitWidth: rootItem.width / 2
            environment: ExtendedSceneEnvironment {
                clearColor: "#334466"
                backgroundMode: SceneEnvironment.Color
                lutEnabled: true
                lutFilterAlpha: 1.0
            }
            camera: cam
            importScene: scene
        }

        // Inverted LUT at full strength
        View3D {
            implicitHeight: rootItem.height / 2
            implicitWidth: rootItem.width / 2
            environment: ExtendedSceneEnvironment {
                clearColor: "#334466"
                backgroundMode: SceneEnvironment.Color
                lutEnabled: true
                lutTexture: Texture {
                    source: "../shared/maps/inverted_lut.png"
                }
                lutFilterAlpha: 1.0
            }
            camera: cam
            importScene: scene
        }

        // Grayscale LUT blended at 50%
        View3D {
            implicitHeight: rootItem.height / 2
            implicitWidth: rootItem.width / 2
            environment: ExtendedSceneEnvironment {
                clearColor: "#334466"
                backgroundMode: SceneEnvironment.Color
                lutEnabled: true
                lutTexture: Texture {
                    source: "../shared/maps/grayscale_lut.png"
                }
                lutFilterAlpha: 0.5
            }
            camera: cam
            importScene: scene
        }
    }
}
