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
    // These properties names need to match the ones in the shader code!
    property bool uEnvironmentMappingEnabled: true
    property bool uShadowMappingEnabled: false
    property real roughness: 0.25
    property real material_ior: 1.8
    property real intensity: 1.0
    property real texture_scaling: 0.1
    property real bump_factor: 0.4
    property vector3d diffuse_color: Qt.vector3d(0.451, 0.04, 0.035)
    property vector3d emission_color: Qt.vector3d(0.0, 0.0, 0.0)

    shaderInfo: CustomMaterialShaderInfo {
        version: "330"
        type: "GLSL"
        shaderKey: CustomMaterialShaderInfo.Glossy | CustomMaterialShaderInfo.Diffuse
        layers: 1
    }

    property CustomMaterialTexture uEnvironmentTexture: CustomMaterialTexture {
            enabled: uEnvironmentMappingEnabled
            type: CustomMaterialTexture.Environment
            image: Texture {
                id: envImage
                source: "maps/spherical_checker.png"
            }
    }
    property CustomMaterialTexture uBakedShadowTexture: CustomMaterialTexture {
            enabled: uShadowMappingEnabled
            type: CustomMaterialTexture.LightmapShadow
            image: Texture {
                id: shadowImage
                source: "maps/shadow.png"
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
    property CustomMaterialTexture randomGradient1D: CustomMaterialTexture {
            type: CustomMaterialTexture.Unknown; //Gradient
            image: Texture {
                tilingModeHorizontal: Texture.Repeat
                tilingModeVertical: Texture.Repeat
                source: "maps/randomGradient1D.png"
            }
    }
    property CustomMaterialTexture randomGradient2D: CustomMaterialTexture {
            type: CustomMaterialTexture.Unknown; //Gradient
            image: Texture {
                tilingModeHorizontal: Texture.Repeat
                tilingModeVertical: Texture.Repeat
                source: "maps/randomGradient2D.png"
            }
    }
    property CustomMaterialTexture randomGradient3D: CustomMaterialTexture {
        type: CustomMaterialTexture.Unknown; //Gradient
        image: Texture {
            tilingModeHorizontal: Texture.Repeat
            tilingModeVertical: Texture.Repeat
            source: "maps/randomGradient3D.png"
        }
    }
    property CustomMaterialTexture randomGradient4D: CustomMaterialTexture {
        type: CustomMaterialTexture.Unknown; //Gradient
        image: Texture {
            tilingModeHorizontal: Texture.Repeat
            tilingModeVertical: Texture.Repeat
            source: "maps/randomGradient4D.png"
        }
    }

    CustomMaterialShader {
        id: plasticStructuredRedEmissiveFragShader
        stage: CustomMaterialShader.Fragment
        shader: "shaders/plasticStructuredRedEmissive.frag"
    }

    passes: [ CustomMaterialPass {
            shaders: plasticStructuredRedEmissiveFragShader
        }
    ]
}
