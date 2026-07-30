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

        // Row 0: increasing strength on a metal, from the isotropic reference
        // through to a fully stretched highlight
        Model {
            source: "#Sphere"
            y: 110
            x: -110
            scale: Qt.vector3d(0.9, 0.9, 0.9)
            materials: PrincipledMaterial {
                baseColor: "#ffcccccc"
                metalness: 1.0
                roughness: 0.35
            }
        }
        Model {
            source: "#Sphere"
            y: 110
            scale: Qt.vector3d(0.9, 0.9, 0.9)
            materials: PrincipledMaterial {
                baseColor: "#ffcccccc"
                metalness: 1.0
                roughness: 0.35
                anisotropyStrength: 0.5
            }
        }
        Model {
            source: "#Sphere"
            y: 110
            x: 110
            scale: Qt.vector3d(0.9, 0.9, 0.9)
            materials: PrincipledMaterial {
                baseColor: "#ffcccccc"
                metalness: 1.0
                roughness: 0.35
                anisotropyStrength: 1.0
            }
        }

        // Row 1: the rotation, which has to turn the highlight within the
        // tangent plane
        Model {
            source: "#Sphere"
            x: -110
            scale: Qt.vector3d(0.9, 0.9, 0.9)
            materials: PrincipledMaterial {
                baseColor: "#ffcccccc"
                metalness: 1.0
                roughness: 0.35
                anisotropyStrength: 0.9
                anisotropyRotation: 45
            }
        }
        Model {
            source: "#Sphere"
            scale: Qt.vector3d(0.9, 0.9, 0.9)
            materials: PrincipledMaterial {
                baseColor: "#ffcccccc"
                metalness: 1.0
                roughness: 0.35
                anisotropyStrength: 0.9
                anisotropyRotation: 90
            }
        }
        // A rough dielectric rather than a metal, and combined with sheen so
        // that the layer composite is exercised together with anisotropy
        Model {
            source: "#Sphere"
            x: 110
            scale: Qt.vector3d(0.9, 0.9, 0.9)
            materials: PrincipledMaterial {
                baseColor: "#ff202030"
                roughness: 0.5
                anisotropyStrength: 0.9
                sheenColor: "#ff80a0ff"
                sheenRoughness: 0.4
            }
        }

        // Row 2: the map, which supplies direction in RG and strength in B,
        // and the interaction with normal mapping and with clearcoat
        Model {
            source: "#Sphere"
            y: -110
            x: -110
            scale: Qt.vector3d(0.9, 0.9, 0.9)
            materials: PrincipledMaterial {
                baseColor: "#ffcccccc"
                metalness: 1.0
                roughness: 0.35
                anisotropyStrength: 1.0
                anisotropyMap: Texture {
                    source: "../shared/maps/rgba.png"
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
                baseColor: "#ffcccccc"
                metalness: 1.0
                roughness: 0.35
                anisotropyStrength: 0.9
                normalMap: Texture {
                    source: "../shared/maps/RibsNormal.png"
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
                baseColor: "#ffcccccc"
                metalness: 1.0
                roughness: 0.35
                anisotropyStrength: 0.9
                anisotropyRotation: 30
                clearcoatAmount: 1.0
                clearcoatRoughnessAmount: 0.1
            }
        }
    }
}
