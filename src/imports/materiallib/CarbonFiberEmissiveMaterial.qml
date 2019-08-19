/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.12
import QtQuick3D 1.0

CustomMaterial {

    property bool uEnvironmentMappingEnabled: true
    property bool uShadowMappingEnabled: false
    property real coat_ior: 1.24
    property real coat_weight: 1
    property real coat_roughness: 0
    property real base_ior: 1.65
    property real base_weight: 0.5
    property real base_roughness: 0.1
    property real anisotropy: 0.8
    property vector2d texture_tiling: Qt.vector2d(12, 12)
    property real bump_amount: 1.0

    property real intensity: 1.0
    property vector3d emission_color: Qt.vector3d(0, 0, 0)

    shaderInfo: CustomMaterialShaderInfo {
        version: "330"
        type: "GLSL"
        shaderKey: CustomMaterialShaderInfo.Glossy | CustomMaterialShaderInfo.Diffuse
        layers: 3
    }

    property CustomMaterialTexture uEnvironmentTexture: CustomMaterialTexture {
            id: uEnvironmentTexture
            type: CustomMaterialTexture.Environment
            enabled: uEnvironmentMappingEnabled
            image: Texture {
                id: envImage
                tilingModeHorizontal: Texture.Repeat
                tilingModeVertical: Texture.Repeat
                source: "maps/spherical_checker.png"
            }
    }
    property CustomMaterialTexture uBakedShadowTexture: CustomMaterialTexture {
            type: CustomMaterialTexture.LightmapShadow
            enabled: uShadowMappingEnabled
            image: Texture {
                id: shadowImage
                tilingModeHorizontal: Texture.Repeat
                tilingModeVertical: Texture.Repeat
                source: "maps/shadow.png"
            }
    }

    property CustomMaterialTexture anisotropy_rotation_texture: CustomMaterialTexture {
            type: CustomMaterialTexture.Anisotropy
            enabled: true
            image: Texture {
                id: anisoTexture
                tilingModeHorizontal: Texture.Repeat
                tilingModeVertical: Texture.Repeat
                source: "maps/carbon_fiber_aniso.png"
            }
    }

    property CustomMaterialTexture reflect_texture: CustomMaterialTexture {
            type: CustomMaterialTexture.Specular
            enabled: true
            image: Texture {
                id: reflectionTexture
                tilingModeHorizontal: Texture.Repeat
                tilingModeVertical: Texture.Repeat
                source: "maps/carbon_fiber_spec.png"
            }
    }

    property CustomMaterialTexture diffuse_texture: CustomMaterialTexture {
            type: CustomMaterialTexture.Diffuse
            enabled: true
            image: Texture {
                id: diffuseTexture
                tilingModeHorizontal: Texture.Repeat
                tilingModeVertical: Texture.Repeat
                source: "maps/carbon_fiber.png"
            }
    }

    property CustomMaterialTexture bump_texture: CustomMaterialTexture {
            type: CustomMaterialTexture.Bump
            enabled: true
            image: Texture {
                id: bumpTexture
                tilingModeHorizontal: Texture.Repeat
                tilingModeVertical: Texture.Repeat
                source: "maps/carbon_fiber_bump.png"
            }
    }

    property CustomMaterialTexture emissive_texture: CustomMaterialTexture {
            id: emissiveTexture
            type: CustomMaterialTexture.Emissive
            enabled: true
            image: Texture {
                id: emissiveImage
                tilingModeHorizontal: Texture.Repeat
                tilingModeVertical: Texture.Repeat
                source: "maps/emissive.png"
            }
    }

    property CustomMaterialTexture emissive_mask_texture: CustomMaterialTexture {
            id: emissiveMaskTexture
            type: CustomMaterialTexture.Unknown
            enabled: true
            image: Texture {
                id: emissiveMaskImage
                tilingModeHorizontal: Texture.Repeat
                tilingModeVertical: Texture.Repeat
                source: "maps/emissive_mask.png"
            }
    }


    CustomMaterialShader {
        id: carbonFiberEmissiveFragShader
        stage: CustomMaterialShader.Fragment
        shader: "shaders/carbonFiberEmissive.frag"
    }

    passes: [
        CustomMaterialPass {
            shaders: carbonFiberEmissiveFragShader
        }
    ]
}
