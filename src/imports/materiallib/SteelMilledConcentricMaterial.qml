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
    property real material_ior: 20.0
    property real anisotropy: 0.8
    property vector2d texture_tiling: Qt.vector2d(8, 5)

    shaderInfo: CustomMaterialShaderInfo {
        version: "330"
        type: "GLSL"
        shaderKey: CustomMaterialShaderInfo.Glossy
        layers: 1
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
    property CustomMaterialTexture diffuse_texture: CustomMaterialTexture {
            type: CustomMaterialTexture.Diffuse
            enabled: true
            image: Texture {
                id: diffuseTexture
                tilingModeHorizontal: Texture.Repeat
                tilingModeVertical: Texture.Repeat
                source: "maps/concentric_milled_steel.png"
            }
    }
    property CustomMaterialTexture anisotropy_rot_texture: CustomMaterialTexture {
            type: CustomMaterialTexture.Anisotropy
            enabled: true
            image: Texture {
                id: anisoTexture
                tilingModeHorizontal: Texture.Repeat
                tilingModeVertical: Texture.Repeat
                source: "maps/concentric_milled_steel_aniso.png"
            }
    }

    CustomMaterialShader {
        id: steelMilledConcentricFragShader
        stage: CustomMaterialShader.Fragment
        shader: "shaders/steelMilledConcentric.frag"
    }

    passes: [
        CustomMaterialPass {
            shaders: steelMilledConcentricFragShader
        }
    ]
}
