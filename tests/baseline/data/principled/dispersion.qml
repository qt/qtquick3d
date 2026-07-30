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

        // High contrast bars behind the glass. Dispersion shows up as colored
        // edges where the three refracted rays land on different sides of a
        // luminance step, so the scene needs sharp edges to separate.
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

        // Reference: refracting glass with no dispersion at all
        Model {
            source: "#Sphere"
            x: -120
            y: 90
            scale: Qt.vector3d(0.8, 0.8, 0.8)
            materials: PrincipledMaterial {
                baseColor: "white"
                roughness: 0.0
                transmissionFactor: 1.0
                thicknessFactor: 300
                indexOfRefraction: 2.0
            }
        }

        // The largest value ordinary materials reach, which is about an Abbe
        // number of 20. Realistic dispersion is a thin colored fringe rather
        // than a broad rainbow.
        Model {
            source: "#Sphere"
            x: 120
            y: 90
            scale: Qt.vector3d(0.8, 0.8, 0.8)
            materials: PrincipledMaterial {
                baseColor: "white"
                roughness: 0.0
                transmissionFactor: 1.0
                thicknessFactor: 300
                indexOfRefraction: 2.0
                dispersion: 1.0
            }
        }

        // A thicker volume gives the three rays further to diverge, so the same
        // dispersion separates more
        Model {
            source: "#Sphere"
            x: -120
            y: -110
            scale: Qt.vector3d(0.8, 0.8, 0.8)
            materials: PrincipledMaterial {
                baseColor: "white"
                roughness: 0.0
                transmissionFactor: 1.0
                thicknessFactor: 900
                indexOfRefraction: 2.0
                dispersion: 1.0
            }
        }

        // Deliberately beyond any real material. This one is here so the test
        // is actually sensitive: at physical values the difference from the
        // reference is a few pixels of colored edge, which is a weak thing to
        // regress against, whereas this separates the channels unmistakably.
        Model {
            source: "#Sphere"
            x: 120
            y: -110
            scale: Qt.vector3d(0.8, 0.8, 0.8)
            materials: PrincipledMaterial {
                baseColor: "white"
                roughness: 0.0
                transmissionFactor: 1.0
                thicknessFactor: 900
                indexOfRefraction: 2.0
                dispersion: 20.0
            }
        }
    }
}
