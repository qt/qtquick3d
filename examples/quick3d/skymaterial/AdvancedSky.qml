// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers

SkyMaterial {
    id: skyMaterial
    required property Node skyLight
    fragmentShader: "advancedsky.frag"

    property int lutSize: 256
    property int lutSizeTransmit: 256

    property real aerosolBaseDensity: 1.3681e20
    property real aerosolBackgroundDensity: 2e6
    property real aerosolHeightScale: 0.73
    property real aerosolBackgroundDividedByBaseDensity: aerosolBackgroundDensity / aerosolBaseDensity
    property real ozoneMeanMonthlyDobson: 347
    property real aerosolTurbidity: 1

    property vector4d aerosolAbsorptionCrossSection: Qt.vector4d(2.8722e-24, 4.6168e-24, 7.9706e-24, 1.3578e-23)
    property vector4d aerosolScatteringCrossSection: Qt.vector4d(1.5908e-22, 1.7711e-22, 2.0942e-22, 2.4033e-22)
    property vector4d groundAlbedo: Qt.vector4d(0.3, 0.3, 0.3, 0.3)

    property int aerosolType: 8
    property real eyeAltitude: 0.05
    property real sunEnergy: 1.0
    property vector3d sunColor: Qt.vector3d(1, 1, 1)
    // Atmosphere coords: x stays, swap y/z, flip y. Both the advanced-sky frag and the
    // LUT shaders (skytexture/transmittance) sample this in the same convention.
    property vector3d sunDirection: Qt.vector3d(skyLight.forward.x, skyLight.forward.z, -skyLight.forward.y)
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

            property real aerosolBaseDensity: skyMaterial.aerosolBaseDensity
            property real aerosolHeightScale: skyMaterial.aerosolHeightScale
            property real aerosolBackgroundDividedByBaseDensity: skyMaterial.aerosolBackgroundDividedByBaseDensity
            property real ozoneMeanMonthlyDobson: skyMaterial.ozoneMeanMonthlyDobson
            property real aerosolTurbidity: skyMaterial.aerosolTurbidity

            property vector4d aerosolAbsorptionCrossSection: skyMaterial.aerosolAbsorptionCrossSection
            property vector4d aerosolScatteringCrossSection: skyMaterial.aerosolScatteringCrossSection
            property vector4d groundAlbedo: skyMaterial.groundAlbedo

            property int aerosolType: skyMaterial.aerosolType
            property real eyeAltitude: skyMaterial.eyeAltitude
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

                    property real aerosolBaseDensity: skyMaterial.aerosolBaseDensity
                    property real aerosolBackgroundDensity: skyMaterial.aerosolBackgroundDensity
                    property real aerosolHeightScale: skyMaterial.aerosolHeightScale
                    property real aerosolBackgroundDividedByBaseDensity: skyMaterial.aerosolBackgroundDividedByBaseDensity
                    property real ozoneMeanMonthlyDobson: skyMaterial.ozoneMeanMonthlyDobson
                    property real aerosolTurbidity: skyMaterial.aerosolTurbidity

                    property vector4d aerosolAbsorptionCrossSection: skyMaterial.aerosolAbsorptionCrossSection
                    property vector4d aerosolScatteringCrossSection: skyMaterial.aerosolScatteringCrossSection
                    property vector4d groundAlbedo: skyMaterial.groundAlbedo

                    property int aerosolType: skyMaterial.aerosolType
                    property real eyeAltitude: skyMaterial.eyeAltitude
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
