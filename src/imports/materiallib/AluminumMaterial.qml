/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
****************************************************************************/

import QtQuick 2.15
import QtQuick3D 1.15
import QtQuick3D.Materials 1.15

CustomMaterial {
    property real reflection_map_offset: 0.5
    property real reflection_map_scale: 0.3
    property real roughness_map_offset: 0.16
    property real roughness_map_scale: 0.4
    property real bump_amount: 0.5
    property bool uEnvironmentMappingEnabled: true
    property bool uShadowMappingEnabled: false
    property vector3d tiling: Qt.vector3d(1, 1, 1)
    property vector3d metal_color: Qt.vector3d(0.95, 0.95, 0.95)

    shaderInfo: ShaderInfo {
        version: "330"
        type: "GLSL"
        shaderKey: ShaderInfo.Glossy
    }

    property TextureInput uEnvironmentTexture: TextureInput {
            id: uEnvironmentTexture
            enabled: uEnvironmentMappingEnabled
            texture: Texture {
                id: envImage
                tilingModeHorizontal: Texture.Repeat
                tilingModeVertical: Texture.Repeat
                source: "maps/spherical_checker.png"
            }
    }
    property TextureInput uBakedShadowTexture: TextureInput {
            enabled: uShadowMappingEnabled
            texture: Texture {
                id: shadowImage
                tilingModeHorizontal: Texture.Repeat
                tilingModeVertical: Texture.Repeat
                source: "maps/shadow.png"
            }
    }
    property TextureInput reflection_texture: TextureInput {
            enabled: true
            texture: Texture {
                id: reflectionTexture
                tilingModeHorizontal: Texture.Repeat
                tilingModeVertical: Texture.Repeat
                source: "maps/grunge_b.png"
            }
    }
    property TextureInput roughness_texture: TextureInput {
            enabled: true
            texture: Texture {
                id: roughnessTexture
                tilingModeHorizontal: Texture.Repeat
                tilingModeVertical: Texture.Repeat
                source: "maps/grunge_d.png"
            }
    }
    property TextureInput bump_texture: TextureInput {
            enabled: true
            texture: Texture {
                id: bumpTexture
                tilingModeHorizontal: Texture.Repeat
                tilingModeVertical: Texture.Repeat
                source: "maps/grunge_d.png"
            }
    }

    Shader {
        id: aluminumFragShader
        stage: Shader.Fragment
        shader: "shaders/aluminum.frag"
    }

    passes: [
        Pass {
            shaders: aluminumFragShader
        }
    ]
}
