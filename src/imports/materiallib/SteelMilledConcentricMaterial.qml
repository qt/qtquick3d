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
    property real material_ior: 2.5
    property real anisotropy: 0.8
    property vector2d texture_tiling: Qt.vector2d(8, 5)

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
    property TextureInput diffuse_texture: TextureInput {
            enabled: true
            texture: Texture {
                id: diffuseTexture
                tilingModeHorizontal: Texture.Repeat
                tilingModeVertical: Texture.Repeat
                source: "maps/concentric_milled_steel.png"
            }
    }
    property TextureInput anisotropy_rot_texture: TextureInput {
            enabled: true
            texture: Texture {
                id: anisoTexture
                tilingModeHorizontal: Texture.Repeat
                tilingModeVertical: Texture.Repeat
                source: "maps/concentric_milled_steel_aniso.png"
            }
    }

    Shader {
        id: steelMilledConcentricFragShader
        stage: Shader.Fragment
        shader: "shaders/steelMilledConcentric.frag"
    }

    passes: [
        Pass {
            shaders: steelMilledConcentricFragShader
        }
    ]
}
