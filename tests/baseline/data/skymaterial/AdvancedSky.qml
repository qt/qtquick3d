// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers

Rectangle {
    id: root
    width: 800
    height: 480
    color: Qt.rgba(1, 1, 1, 1)

    SkyMaterial {
        id: mySkyMaterial
        fragmentShader: "advancedsky.frag"

        property int lutSize: 256
        property int lutSizeTransmit: 256

        property real aerosol_base_density: 1.3681e20
        property real aerosol_background_density: 2e6
        property real aerosol_height_scale: 0.73
        property real aerosol_background_divided_by_base_density: aerosol_background_density / aerosol_base_density
        property real ozone_mean_monthly_dobson: 347
        property real aerosol_turbidity: 1

        property vector3d sun_direction: Qt.vector3d(mySkyLight.forward.x, mySkyLight.forward.z, -mySkyLight.forward.y)
        property vector4d aerosol_absorption_cross_section: Qt.vector4d(2.8722e-24, 4.6168e-24, 7.9706e-24, 1.3578e-23)
        property vector4d aerosol_scattering_cross_section: Qt.vector4d(1.5908e-22, 1.7711e-22, 2.0942e-22, 2.4033e-22)
        property vector4d ground_albedo: Qt.vector4d(0.3, 0.3, 0.3, 0.3)

        property int aerosol_type: 8
        property real eye_altitude: 0.05
        property real sunEnergy: mySkyLight.brightness
        property vector3d sunColor: mySkyLight.color
        property vector3d sunDirection: sun_direction
        property real sunDiskInnerAngle: 1.5
        property real sunDiskOuterAngle: 10.0
        property real sunDiskFalloff: 0.05
        property real sunAlpha: 1.0

        property Texture skytextureBuffer: Texture {
            tilingModeVertical: Texture.ClampToEdge
            tilingModeHorizontal: Texture.ClampToEdge

            textureProvider: QuadTextureProvider {
                fragmentShader: "skytexture.frag"
                format: Buffer.RGBA16F
                width: mySkyMaterial.lutSize
                height: mySkyMaterial.lutSize

                property real aerosol_base_density: mySkyMaterial.aerosol_base_density
                property real aerosol_height_scale: mySkyMaterial.aerosol_height_scale
                property real aerosol_background_divided_by_base_density: mySkyMaterial.aerosol_background_divided_by_base_density
                property real ozone_mean_monthly_dobson: mySkyMaterial.ozone_mean_monthly_dobson
                property real aerosol_turbidity: mySkyMaterial.aerosol_turbidity

                property vector3d sun_direction: mySkyMaterial.sun_direction
                property vector4d aerosol_absorption_cross_section: mySkyMaterial.aerosol_absorption_cross_section
                property vector4d aerosol_scattering_cross_section: mySkyMaterial.aerosol_scattering_cross_section
                property vector4d ground_albedo: mySkyMaterial.ground_albedo

                property int aerosol_type: mySkyMaterial.aerosol_type
                property real eye_altitude: mySkyMaterial.eye_altitude
                property real sunEnergy: mySkyMaterial.sunEnergy
                property vector3d sunColor: mySkyMaterial.sunColor
                property vector3d sunDirection: mySkyMaterial.sunDirection
                property real sunDiskInnerAngle: mySkyMaterial.sunDiskInnerAngle
                property real sunDiskOuterAngle: mySkyMaterial.sunDiskOuterAngle
                property real sunDiskFalloff: mySkyMaterial.sunDiskFalloff
                property real sunAlpha: mySkyMaterial.sunAlpha

                property Texture transmittanceBuffer: Texture {
                    tilingModeVertical: Texture.ClampToEdge
                    tilingModeHorizontal: Texture.ClampToEdge

                    textureProvider: QuadTextureProvider {
                        fragmentShader: "transmittance.frag"
                        format: Buffer.RGBA16F
                        width: mySkyMaterial.lutSizeTransmit
                        height: mySkyMaterial.lutSizeTransmit

                        property real aerosol_base_density: mySkyMaterial.aerosol_base_density
                        property real aerosol_background_density: mySkyMaterial.aerosol_background_density
                        property real aerosol_height_scale: mySkyMaterial.aerosol_height_scale
                        property real aerosol_background_divided_by_base_density: mySkyMaterial.aerosol_background_divided_by_base_density
                        property real ozone_mean_monthly_dobson: mySkyMaterial.ozone_mean_monthly_dobson
                        property real aerosol_turbidity: mySkyMaterial.aerosol_turbidity

                        property vector3d sun_direction: mySkyMaterial.sun_direction
                        property vector4d aerosol_absorption_cross_section: mySkyMaterial.aerosol_absorption_cross_section
                        property vector4d aerosol_scattering_cross_section: mySkyMaterial.aerosol_scattering_cross_section
                        property vector4d ground_albedo: mySkyMaterial.ground_albedo

                        property int aerosol_type: mySkyMaterial.aerosol_type
                        property real eye_altitude: mySkyMaterial.eye_altitude
                        property real sunEnergy: mySkyMaterial.sunEnergy
                        property vector3d sunColor: mySkyMaterial.sunColor
                        property vector3d sunDirection: mySkyMaterial.sunDirection
                        property real sunDiskInnerAngle: mySkyMaterial.sunDiskInnerAngle
                        property real sunDiskOuterAngle: mySkyMaterial.sunDiskOuterAngle
                        property real sunDiskFalloff: mySkyMaterial.sunDiskFalloff
                        property real sunAlpha: mySkyMaterial.sunAlpha
                    }
                }
            }
        }
    }

    NumberAnimation {
        target: mySkyLight
        property: "eulerRotation.x"
        duration: 200
        easing.type: Easing.InOutQuad
        from: -90
        to: -1
        running: true
    }

    View3D {
        id: view3D
        anchors.fill: parent

        environment: SceneEnvironment {
            id: scene
            backgroundMode: SceneEnvironment.SkyMaterial
            skyMaterial: mySkyMaterial
        }

        camera: PerspectiveCamera {
            id: cam
            position: Qt.vector3d(750, 460, -1128)
            eulerRotation: Qt.vector3d(-13, -203, 0)
        }

        DirectionalLight {
            id: mySkyLight
            eulerRotation.x: -90
            castsShadow: true
            softShadowQuality: Light.PCF16
            shadowMapQuality: Light.ShadowMapQualityVeryHigh
            pcfFactor: 3
            use32BitShadowmap: true
            property real customBrightness: 1.0
            brightness: eulerRotation.x > 0.0 || eulerRotation.x < -180 ? 0 : customBrightness
        }

        /// Models
        Model {
            source: "#Rectangle"
            scale: Qt.vector3d(30, 30, 30)
            eulerRotation.x: -90
            receivesShadows: true
            materials: PrincipledMaterial {
                baseColor: "lightgray"
                roughness: 0.9
                metalness: 0.0
            }
        }

        Model {
            source: "#Cube"
            x: -375
            y: 100
            scale: Qt.vector3d(2, 2, 2)
            castsShadows: true
            receivesShadows: true
            materials: PrincipledMaterial {
                baseColor: "steelblue"
                roughness: 0.25
                metalness: 0.0
            }
        }

        Model {
            source: "#Sphere"
            x: -75
            y: 125
            scale: Qt.vector3d(2.5, 2.5, 2.5)
            castsShadows: true
            receivesShadows: true
            materials: PrincipledMaterial {
                baseColor: "lightblue"
                roughness: 0.05
                metalness: 1.0
            }
        }

        Model {
            source: "#Sphere"
            x: 275
            y: 125
            scale: Qt.vector3d(2.5, 2.5, 2.5)
            castsShadows: true
            receivesShadows: true
            materials: PrincipledMaterial {
                baseColor: "lightblue"
                roughness: 0.3
                metalness: 1.0
            }
        }

        Model {
            source: "#Sphere"
            x: 575
            y: 125
            scale: Qt.vector3d(2.5, 2.5, 2.5)
            castsShadows: true
            receivesShadows: true
            materials: PrincipledMaterial {
                baseColor: "lightblue"
                roughness: 0.55
                metalness: 1.0
            }
        }

        Model {
            source: "#Sphere"
            x: 875
            y: 125
            scale: Qt.vector3d(2.5, 2.5, 2.5)
            castsShadows: true
            receivesShadows: true
            materials: PrincipledMaterial {
                baseColor: "lightblue"
                roughness: 0.8
                metalness: 1.0
            }
        }
    }

    WasdController {
        controlledObject: cam
    }
}
