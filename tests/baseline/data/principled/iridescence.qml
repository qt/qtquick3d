// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick3D

Rectangle {
    width: 400
    height: 400
    color: "lightgray"

    View3D {
        anchors.fill: parent

        environment: SceneEnvironment {
            lightProbe: Texture {
                source: "../shared/maps/OpenfootageNET_lowerAustria01-1024.hdr"
            }
        }

        camera: PerspectiveCamera {
            z: 400
        }

        DirectionalLight {
            eulerRotation.x: -20
        }

        PointLight {
            z: 200
            y: 100
        }

        // Row 0: the factor, from the reference material to a full thin film.
        // A metal base is used because the interference is far more visible
        // against a high reflectance than against a dielectric's few percent.
        Model {
            source: "#Sphere"
            y: 110
            x: -110
            scale: Qt.vector3d(0.9, 0.9, 0.9)
            materials: PrincipledMaterial {
                baseColor: "#ffd4af37"
                metalness: 1.0
                roughness: 0.15
            }
        }
        Model {
            source: "#Sphere"
            y: 110
            scale: Qt.vector3d(0.9, 0.9, 0.9)
            materials: PrincipledMaterial {
                baseColor: "#ffd4af37"
                metalness: 1.0
                roughness: 0.15
                iridescenceFactor: 0.5
            }
        }
        Model {
            source: "#Sphere"
            y: 110
            x: 110
            scale: Qt.vector3d(0.9, 0.9, 0.9)
            materials: PrincipledMaterial {
                baseColor: "#ffd4af37"
                metalness: 1.0
                roughness: 0.15
                iridescenceFactor: 1.0
            }
        }

        // Row 1: thickness selects the hue, so this sweep has to produce three
        // visibly different colors
        Model {
            source: "#Sphere"
            x: -110
            scale: Qt.vector3d(0.9, 0.9, 0.9)
            materials: PrincipledMaterial {
                baseColor: "#ff909090"
                metalness: 1.0
                roughness: 0.15
                iridescenceFactor: 1.0
                iridescenceThicknessMaximum: 200
            }
        }
        Model {
            source: "#Sphere"
            scale: Qt.vector3d(0.9, 0.9, 0.9)
            materials: PrincipledMaterial {
                baseColor: "#ff909090"
                metalness: 1.0
                roughness: 0.15
                iridescenceFactor: 1.0
                iridescenceThicknessMaximum: 550
            }
        }
        // A higher film index of refraction as well as a thicker film
        Model {
            source: "#Sphere"
            x: 110
            scale: Qt.vector3d(0.9, 0.9, 0.9)
            materials: PrincipledMaterial {
                baseColor: "#ff909090"
                metalness: 1.0
                roughness: 0.15
                iridescenceFactor: 1.0
                iridescenceIndexOfRefraction: 2.0
                iridescenceThicknessMaximum: 900
            }
        }

        // Row 2: the two maps, and iridescence over a smooth dielectric, which
        // is the soap bubble case and deliberately much subtler
        Model {
            source: "#Sphere"
            y: -110
            x: -110
            scale: Qt.vector3d(0.9, 0.9, 0.9)
            materials: PrincipledMaterial {
                baseColor: "#ff909090"
                metalness: 1.0
                roughness: 0.15
                iridescenceFactor: 1.0
                iridescenceThicknessMinimum: 200
                iridescenceThicknessMaximum: 900
                iridescenceThicknessMap: Texture {
                    source: "../shared/maps/RoughnessStripes.png"
                    generateMipmaps: true
                    mipFilter: Texture.Linear
                }
            }
        }
        Model {
            source: "#Sphere"
            y: -110
            scale: Qt.vector3d(0.9, 0.9, 0.9)
            materials: PrincipledMaterial {
                baseColor: "#ff909090"
                metalness: 1.0
                roughness: 0.15
                iridescenceFactor: 1.0
                iridescenceThicknessMaximum: 550
                iridescenceMap: Texture {
                    source: "../shared/maps/rgba.png"
                    generateMipmaps: true
                    mipFilter: Texture.Linear
                }
            }
        }
        Model {
            source: "#Sphere"
            y: -110
            x: 110
            scale: Qt.vector3d(0.9, 0.9, 0.9)
            materials: PrincipledMaterial {
                baseColor: "#ff303030"
                roughness: 0.1
                iridescenceFactor: 1.0
                iridescenceThicknessMaximum: 550
                clearcoatAmount: 1.0
            }
        }
    }
}
