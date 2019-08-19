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
    property real coating_ior: 1.5
    property real coat_weight: 1.0
    property real coat_roughness: 0
    property vector3d coat_color: Qt.vector3d(1, 1, 1)

    property real flake_intensity: 0.5
    property real flake_size: 0.002
    property real flake_amount: 0.22
    property real flake_roughness: 0.2
    property vector3d flake_color: Qt.vector3d(1, 0.7, 0.02)

    property vector3d base_color: Qt.vector3d(0.1, 0.001, 0.001)
    property real flake_bumpiness: 0.6
    property real peel_size: 1.0
    property real peel_amount: 0.1

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
        id: carpaint2LayerFragShader
        stage: CustomMaterialShader.Fragment
        shader: "shaders/carPaintColorPeel2Layer.frag"
    }

    passes: [
        CustomMaterialPass {
            shaders: carpaint2LayerFragShader
        }
    ]
}
