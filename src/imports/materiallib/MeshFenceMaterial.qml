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
    property bool uEnvironmentMappingEnabled: false
    property bool uShadowMappingEnabled: false
    property real material_ior: 15.0
    property real glossy_weight: 0.5
    property real roughness: 0.25
    property real bump_amount: 2.0
    property vector2d texture_tiling: Qt.vector2d(3.0, 3.0)

    shaderInfo: CustomMaterialShaderInfo {
        version: "330"
        type: "GLSL"
        shaderKey: CustomMaterialShaderInfo.Cutout | CustomMaterialShaderInfo.Glossy | CustomMaterialShaderInfo.Diffuse
        layers: 2
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
    property CustomMaterialTexture diffuse_texture: CustomMaterialTexture {
        type: CustomMaterialTexture.Diffuse
        enabled: true
        image: Texture {
            tilingModeHorizontal: Texture.Repeat
            tilingModeVertical: Texture.Repeat
            source: "maps/cyclone_mesh_fencing.png"
        }
    }

    property CustomMaterialTexture bump_texture: CustomMaterialTexture {
        type: CustomMaterialTexture.Bump
        enabled: true
        image: Texture {
            tilingModeHorizontal: Texture.Repeat
            tilingModeVertical: Texture.Repeat
            source: "maps/cyclone_mesh_fencing_normal.png"
        }
    }

    CustomMaterialShader {
        id: meshFenceFragShader
        stage: CustomMaterialShader.Fragment
        shader: "shaders/meshFence.frag"
    }

    passes: [ CustomMaterialPass {
            shaders: meshFenceFragShader
        }
    ]
}
