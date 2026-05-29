// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers

SkyMaterial {
    id: skyMaterial
    required property DirectionalLight skyLight
    fragmentShader: "advancedsky.frag"

    property int lutSize: 256
    property int lutSizeTransmit: 256

    property real aerosol_base_density: 1.3681e20
    property real aerosol_background_density: 2e6
    property real aerosol_height_scale: 0.73
    property real aerosol_background_divided_by_base_density: aerosol_background_density / aerosol_base_density
    property real ozone_mean_monthly_dobson: 347
    property real aerosol_turbidity: 1

    property vector3d sun_direction: Qt.vector3d(skyLight.forward.x, skyLight.forward.z, -skyLight.forward.y)
    property vector4d aerosol_absorption_cross_section: Qt.vector4d(2.8722e-24, 4.6168e-24, 7.9706e-24, 1.3578e-23)
    property vector4d aerosol_scattering_cross_section: Qt.vector4d(1.5908e-22, 1.7711e-22, 2.0942e-22, 2.4033e-22)
    property vector4d ground_albedo: Qt.vector4d(0.3, 0.3, 0.3, 0.3)

    property int aerosol_type: 8
    property real eye_altitude: 0.05
    property real sunEnergy: skyLight.brightness
    property vector3d sunColor: skyLight.color
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
            width: skyMaterial.lutSize
            height: skyMaterial.lutSize

            property real aerosol_base_density: skyMaterial.aerosol_base_density
            property real aerosol_height_scale: skyMaterial.aerosol_height_scale
            property real aerosol_background_divided_by_base_density: skyMaterial.aerosol_background_divided_by_base_density
            property real ozone_mean_monthly_dobson: skyMaterial.ozone_mean_monthly_dobson
            property real aerosol_turbidity: skyMaterial.aerosol_turbidity

            property vector3d sun_direction: skyMaterial.sun_direction
            property vector4d aerosol_absorption_cross_section: skyMaterial.aerosol_absorption_cross_section
            property vector4d aerosol_scattering_cross_section: skyMaterial.aerosol_scattering_cross_section
            property vector4d ground_albedo: skyMaterial.ground_albedo

            property int aerosol_type: skyMaterial.aerosol_type
            property real eye_altitude: skyMaterial.eye_altitude
            property real sunEnergy: skyMaterial.sunEnergy
            property vector3d sunColor: skyMaterial.sunColor
            property vector3d sunDirection: skyMaterial.sunDirection
            property real sunDiskInnerAngle: skyMaterial.sunDiskInnerAngle
            property real sunDiskOuterAngle: skyMaterial.sunDiskOuterAngle
            property real sunDiskFalloff: skyMaterial.sunDiskFalloff
            property real sunAlpha: skyMaterial.sunAlpha

            property Texture transmittanceBuffer: Texture {
                tilingModeVertical: Texture.ClampToEdge
                tilingModeHorizontal: Texture.ClampToEdge

                textureProvider: QuadTextureProvider {
                    fragmentShader: "transmittance.frag"
                    format: Buffer.RGBA16F
                    width: skyMaterial.lutSizeTransmit
                    height: skyMaterial.lutSizeTransmit

                    property real aerosol_base_density: skyMaterial.aerosol_base_density
                    property real aerosol_background_density: skyMaterial.aerosol_background_density
                    property real aerosol_height_scale: skyMaterial.aerosol_height_scale
                    property real aerosol_background_divided_by_base_density: skyMaterial.aerosol_background_divided_by_base_density
                    property real ozone_mean_monthly_dobson: skyMaterial.ozone_mean_monthly_dobson
                    property real aerosol_turbidity: skyMaterial.aerosol_turbidity

                    property vector3d sun_direction: skyMaterial.sun_direction
                    property vector4d aerosol_absorption_cross_section: skyMaterial.aerosol_absorption_cross_section
                    property vector4d aerosol_scattering_cross_section: skyMaterial.aerosol_scattering_cross_section
                    property vector4d ground_albedo: skyMaterial.ground_albedo

                    property int aerosol_type: skyMaterial.aerosol_type
                    property real eye_altitude: skyMaterial.eye_altitude
                    property real sunEnergy: skyMaterial.sunEnergy
                    property vector3d sunColor: skyMaterial.sunColor
                    property vector3d sunDirection: skyMaterial.sunDirection
                    property real sunDiskInnerAngle: skyMaterial.sunDiskInnerAngle
                    property real sunDiskOuterAngle: skyMaterial.sunDiskOuterAngle
                    property real sunDiskFalloff: skyMaterial.sunDiskFalloff
                    property real sunAlpha: skyMaterial.sunAlpha
                }
            }
        }
    }
}
