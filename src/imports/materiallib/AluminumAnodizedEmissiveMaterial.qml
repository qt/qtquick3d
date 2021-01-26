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

    property bool uEnvironmentMappingEnabled: true
    property bool uShadowMappingEnabled: false
    property real roughness: 0.3
    property vector3d base_color: Qt.vector3d(0.7, 0.7, 0.7)
    property real intensity: 1.0
    property vector3d emission_color: Qt.vector3d(0, 0, 0)


    shaderInfo: ShaderInfo {
        version: "330"
        type: "GLSL"
        shaderKey: ShaderInfo.Glossy
    }

    property TextureInput emissive_texture: TextureInput {
            id: emissiveTexture
            enabled: true
            texture: Texture {
                id: emissiveImage
                tilingModeHorizontal: Texture.Repeat
                tilingModeVertical: Texture.Repeat
                source: "maps/emissive.png"
            }
    }

    property TextureInput emissive_mask_texture: TextureInput {
            id: emissiveMaskTexture
            enabled: true
            texture: Texture {
                id: emissiveMaskImage
                tilingModeHorizontal: Texture.Repeat
                tilingModeVertical: Texture.Repeat
                source: "maps/emissive_mask.png"
            }
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

    Shader {
        id: aluminumAnodizedEmissiveShader
        stage: Shader.Fragment
        shader: "shaders/aluminumAnodizedEmissive.frag"
    }

    passes: [
        Pass {
            shaders: aluminumAnodizedEmissiveShader
        }
    ]

}
