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

        // Row 0: white sheen over a dark base, increasing sheen roughness
        Model {
            source: "#Sphere"
            y: 110
            x: -110
            scale: Qt.vector3d(0.9, 0.9, 0.9)
            materials: PrincipledMaterial {
                baseColor: "#ff101018"
                roughness: 0.5
                sheenColor: "white"
                sheenRoughness: 0.0
            }
        }
        Model {
            source: "#Sphere"
            y: 110
            scale: Qt.vector3d(0.9, 0.9, 0.9)
            materials: PrincipledMaterial {
                baseColor: "#ff101018"
                roughness: 0.5
                sheenColor: "white"
                sheenRoughness: 0.5
            }
        }
        Model {
            source: "#Sphere"
            y: 110
            x: 110
            scale: Qt.vector3d(0.9, 0.9, 0.9)
            materials: PrincipledMaterial {
                baseColor: "#ff101018"
                roughness: 0.5
                sheenColor: "white"
                sheenRoughness: 1.0
            }
        }

        // Row 1: colored sheen, and the reference material with sheen off
        Model {
            source: "#Sphere"
            x: -110
            scale: Qt.vector3d(0.9, 0.9, 0.9)
            materials: PrincipledMaterial {
                baseColor: "#ff101018"
                roughness: 0.5
            }
        }
        Model {
            source: "#Sphere"
            scale: Qt.vector3d(0.9, 0.9, 0.9)
            materials: PrincipledMaterial {
                baseColor: "#ff101018"
                roughness: 0.5
                sheenColor: "#ff3060ff"
                sheenRoughness: 0.3
            }
        }
        Model {
            source: "#Sphere"
            x: 110
            scale: Qt.vector3d(0.9, 0.9, 0.9)
            materials: PrincipledMaterial {
                baseColor: "#ffb02010"
                metalness: 1.0
                roughness: 0.4
                sheenColor: "#ffffc060"
                sheenRoughness: 0.6
            }
        }

        // Row 2: the sheen maps, including a channel-selected roughness map
        Model {
            source: "#Sphere"
            y: -110
            x: -110
            scale: Qt.vector3d(0.9, 0.9, 0.9)
            materials: PrincipledMaterial {
                baseColor: "#ff101018"
                roughness: 0.5
                sheenColor: "white"
                sheenRoughness: 0.3
                sheenColorMap: Texture {
                    source: "../shared/maps/PartialCoating.png"
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
                baseColor: "#ff101018"
                roughness: 0.5
                sheenColor: "white"
                sheenRoughness: 1.0
                sheenRoughnessMap: Texture {
                    source: "../shared/maps/RoughnessStripes.png"
                    generateMipmaps: true
                    mipFilter: Texture.Linear
                }
                sheenRoughnessChannel: Material.G
            }
        }
        Model {
            source: "#Sphere"
            y: -110
            x: 110
            scale: Qt.vector3d(0.9, 0.9, 0.9)
            materials: PrincipledMaterial {
                baseColor: "#ff101018"
                roughness: 0.5
                sheenColor: "white"
                sheenRoughness: 0.4
                sheenColorMap: Texture {
                    source: "../shared/maps/rgba.png"
                    generateMipmaps: true
                    mipFilter: Texture.Linear
                }
                sheenRoughnessMap: Texture {
                    source: "../shared/maps/rgba.png"
                    generateMipmaps: true
                    mipFilter: Texture.Linear
                }
                sheenRoughnessChannel: Material.A
                clearcoatAmount: 1.0
                clearcoatRoughnessAmount: 0.1
            }
        }
    }
}
