// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick3D

Rectangle {
    width: 400
    height: 400
    color: "black"

    View3D {
        anchors.fill: parent

        environment: SceneEnvironment {
            clearColor: "black"
            backgroundMode: SceneEnvironment.Color
        }

        camera: PerspectiveCamera {
            z: 400
        }

        DirectionalLight {
        }

        // Sharp edges behind the glass, for the color fringes to show on
        Repeater3D {
            model: 9
            Model {
                id: bar
                required property int index
                source: "#Rectangle"
                z: -260
                x: -400 + index * 100
                scale: Qt.vector3d(0.3, 8, 1)
                materials: PrincipledMaterial {
                    baseColor: bar.index % 2 === 0 ? "white" : "black"
                    lighting: PrincipledMaterial.NoLighting
                }
            }
        }

        // Transmission without dispersion, for reference
        Model {
            source: "#Sphere"
            x: -120
            y: 90
            scale: Qt.vector3d(0.8, 0.8, 0.8)
            materials: CustomMaterial {
                fragmentShader: "custom_dispersion.frag"
                property real uDispersion: 0.0
            }
        }

        Model {
            source: "#Sphere"
            x: 120
            y: 90
            scale: Qt.vector3d(0.8, 0.8, 0.8)
            // Far beyond any real material, so that the channels separate clearly
            materials: CustomMaterial {
                fragmentShader: "custom_dispersion.frag"
                property real uDispersion: 20.0
            }
        }

        // Dispersion without transmission has no effect, but must still compile
        Model {
            source: "#Sphere"
            y: -100
            scale: Qt.vector3d(0.8, 0.8, 0.8)
            materials: CustomMaterial {
                fragmentShader: "custom_dispersion_notransmission.frag"
            }
        }
    }
}
