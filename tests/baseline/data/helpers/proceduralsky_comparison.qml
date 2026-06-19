// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Side-by-side comparison of ProceduralSkyMaterial (left) and
// ProceduralSkyTextureData (right) for equal parameter sets.
// Top row: all defaults. Bottom row: every property tweaked.
// PSTD uses VeryHigh quality to minimise resolution differences.

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers

Rectangle {
    width: 800
    height: 600
    color: "black"

    component SkyView: View3D {
        required property SceneEnvironment env
        environment: env
        camera: PerspectiveCamera {
            position: Qt.vector3d(0, 150, 600)
            eulerRotation: Qt.vector3d(-10, 0, 0)
        }
        Model {
            source: "#Sphere"
            y: 125
            scale: Qt.vector3d(2.5, 2.5, 2.5)
            materials: PrincipledMaterial {
                baseColor: "lightblue"
                roughness: 0.05
                metalness: 1.0
            }
        }
    }

    // ── Top row: defaults ──────────────────────────────────────────────────

    SkyView {
        x: 0; y: 0; width: 400; height: 300
        env: SceneEnvironment {
            backgroundMode: SceneEnvironment.SkyMaterial
            skyMaterial: ProceduralSkyMaterial {}
        }
    }

    SkyView {
        x: 400; y: 0; width: 400; height: 300
        env: SceneEnvironment {
            backgroundMode: SceneEnvironment.SkyBox
            lightProbe: Texture {
                textureData: ProceduralSkyTextureData {}
            }
        }
    }

    // ── Bottom row: all properties tweaked ────────────────────────────────

    SkyView {
        x: 0; y: 300; width: 400; height: 300
        env: SceneEnvironment {
            backgroundMode: SceneEnvironment.SkyMaterial
            skyMaterial: ProceduralSkyMaterial {
                skyTopColor:      "#1A2E5E"
                skyHorizonColor:  "#D4784A"
                skyCurve:         0.04
                skyEnergy:        0.8
                groundBottomColor:  "#0A0C10"
                groundHorizonColor: "#3A2818"
                groundCurve:      0.04
                groundEnergy:     0.4
                sunColor:         "#FF8C30"
                sunLatitude:      8
                sunLongitude:     160
                sunAngleMin:      2.0
                sunAngleMax:      60.0
                sunCurve:         0.1
                sunEnergy:        2.5
            }
        }
    }

    SkyView {
        x: 400; y: 300; width: 400; height: 300
        env: SceneEnvironment {
            backgroundMode: SceneEnvironment.SkyBox
            lightProbe: Texture {
                textureData: ProceduralSkyTextureData {
                    skyTopColor:      "#1A2E5E"
                    skyHorizonColor:  "#D4784A"
                    skyCurve:         0.04
                    skyEnergy:        0.8
                    groundBottomColor:  "#0A0C10"
                    groundHorizonColor: "#3A2818"
                    groundCurve:      0.04
                    groundEnergy:     0.4
                    sunColor:         "#FF8C30"
                    sunLatitude:      8
                    sunLongitude:     160
                    sunAngleMin:      2.0
                    sunAngleMax:      60.0
                    sunCurve:         0.1
                    sunEnergy:        2.5
                }
            }
        }
    }
}
