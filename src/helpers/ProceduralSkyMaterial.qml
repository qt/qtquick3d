// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick3D

/*!
    \qmltype ProceduralSkyMaterial
    \inqmlmodule QtQuick3D.Helpers
    \since 6.13
    \brief A gradient-based sky material compatible with ProceduralSkyTextureData.

    ProceduralSkyMaterial is a GPU-based sky rendered via \l SkyMaterial that
    provides the same gradient-and-sun-disk sky model as \l ProceduralSkyTextureData,
    but with real-time updates and no CPU texture baking.

    It can be used as a drop-in replacement for ProceduralSkyTextureData: set
    \l{SceneEnvironment::backgroundMode}{backgroundMode} to
    \c SceneEnvironment.SkyMaterial and assign this type to
    \l{SceneEnvironment::skyMaterial}{skyMaterial} instead of using a Texture
    with ProceduralSkyTextureData as its texture data.

    \section1 Example Usage

    \qml
    import QtQuick3D
    import QtQuick3D.Helpers

    View3D {
        environment: SceneEnvironment {
            backgroundMode: SceneEnvironment.SkyMaterial
            skyMaterial: ProceduralSkyMaterial {
                sunLatitude: 35
                sunLongitude: 45
                skyTopColor: "#1a3a6a"
            }
        }
    }
    \endqml

    \sa ProceduralSkyTextureData, SkyMaterial
*/
SkyMaterial {
    fragmentShader: Qt.resolvedUrl("proceduralsky.frag")
    iblSampleCount: 256
    radianceMapSize: 256

    /*!
        \qmlproperty color ProceduralSkyMaterial::skyTopColor
        \since 6.13
        \default "#A5D6F1"
        The color at the top of the sky dome.
    */
    property color skyTopColor: "#A5D6F1"

    /*!
        \qmlproperty color ProceduralSkyMaterial::skyHorizonColor
        \since 6.13
        \default "#D6EAFA"
        The color of the sky at the horizon.
    */
    property color skyHorizonColor: "#D6EAFA"

    /*!
        \qmlproperty real ProceduralSkyMaterial::skyCurve
        \since 6.13
        \default 0.09
        Controls the curvature of the sky gradient blend from horizon to top.
        Values below 1 curve toward the horizon, values above 1 toward the top.
    */
    property real skyCurve: 0.09

    /*!
        \qmlproperty real ProceduralSkyMaterial::skyEnergy
        \since 6.13
        \default 1.0
        Brightness multiplier for the sky hemisphere.
    */
    property real skyEnergy: 1.0

    /*!
        \qmlproperty color ProceduralSkyMaterial::groundBottomColor
        \since 6.13
        \default "#282F36"
        The color at the bottom of the ground hemisphere.
    */
    property color groundBottomColor: "#282F36"

    /*!
        \qmlproperty color ProceduralSkyMaterial::groundHorizonColor
        \since 6.13
        \default "#6C655F"
        The color of the ground at the horizon.
    */
    property color groundHorizonColor: "#6C655F"

    /*!
        \qmlproperty real ProceduralSkyMaterial::groundCurve
        \since 6.13
        \default 0.02
        Controls the curvature of the ground gradient blend from horizon to bottom.
    */
    property real groundCurve: 0.02

    /*!
        \qmlproperty real ProceduralSkyMaterial::groundEnergy
        \since 6.13
        \default 1.0
        Brightness multiplier for the ground hemisphere.
    */
    property real groundEnergy: 1.0

    /*!
        \qmlproperty color ProceduralSkyMaterial::sunColor
        \since 6.13
        \default "#ffffff"
        The color of the sun disk.
    */
    property color sunColor: "#ffffff"

    /*!
        \qmlproperty real ProceduralSkyMaterial::sunLatitude
        \since 6.13
        \default 35.0
        The elevation of the sun above the horizon in degrees. 0 places the sun on
        the horizon; 90 places it directly overhead.
    */
    property real sunLatitude: 35.0

    /*!
        \qmlproperty real ProceduralSkyMaterial::sunLongitude
        \since 6.13
        \default 0.0
        The horizontal rotation of the sun around the vertical axis in degrees.
    */
    property real sunLongitude: 0.0

    /*!
        \qmlproperty real ProceduralSkyMaterial::sunAngleMin
        \since 6.13
        \default 1.0
        The angular radius of the solid sun disk in degrees.
    */
    property real sunAngleMin: 1.0

    /*!
        \qmlproperty real ProceduralSkyMaterial::sunAngleMax
        \since 6.13
        \default 100.0
        The outer angular radius of the sun halo in degrees.
        The halo fades out from \l sunAngleMin to this value.
    */
    property real sunAngleMax: 100.0

    /*!
        \qmlproperty real ProceduralSkyMaterial::sunCurve
        \since 6.13
        \default 0.05
        Controls the falloff curve of the sun halo between \l sunAngleMin and
        \l sunAngleMax. Values below 1 produce a sharp-edged halo; values above
        1 produce a soft glow.
    */
    property real sunCurve: 0.05

    /*!
        \qmlproperty real ProceduralSkyMaterial::sunEnergy
        \since 6.13
        \default 1.0
        Brightness multiplier for the sun disk and halo.
    */
    property real sunEnergy: 1.0

    // Computed sun direction passed as vec3 uniform to the shader.
    // Matches the visual output of ProceduralSkyTextureData used as a SkyBox lightProbe.
    readonly property vector3d sunDirection: {
        const latRad = sunLatitude * Math.PI / 180.0
        const lonRad = sunLongitude * Math.PI / 180.0
        return Qt.vector3d(
            -Math.cos(lonRad) * Math.cos(latRad),
             Math.sin(latRad),
            -Math.sin(lonRad) * Math.cos(latRad)
        )
    }
}
