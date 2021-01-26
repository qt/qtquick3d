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
    // These properties names need to match the ones in the shader code!
    property bool uEnvironmentMappingEnabled: false
    property bool uShadowMappingEnabled: false
    property real bump_amount: 0.5
    property real uTranslucentFalloff: 0.0
    property real uDiffuseLightWrap: 0.0
    property real uOpacity: 100.0
    property real transmission_weight: 0.2
    property real reflection_weight: 0.8
    property vector2d texture_tiling: Qt.vector2d(5.0, 5.0)

    shaderInfo: ShaderInfo {
        version: "330"
        type: "GLSL"
        shaderKey: ShaderInfo.Transmissive | ShaderInfo.Diffuse
    }

    property TextureInput uEnvironmentTexture: TextureInput {
            enabled: uEnvironmentMappingEnabled
            texture: Texture {
                id: envImage
                source: "maps/spherical_checker.png"
            }
    }
    property TextureInput uBakedShadowTexture: TextureInput {
            enabled: uShadowMappingEnabled
            texture: Texture {
                id: shadowImage
                source: "maps/shadow.png"
            }
    }
    property TextureInput diffuse_texture: TextureInput {
        enabled: true
        texture: Texture {
            tilingModeHorizontal: Texture.Repeat
            tilingModeVertical: Texture.Repeat
            source: "maps/paper_diffuse.png"
        }
    }
    property TextureInput bump_texture: TextureInput {
        enabled: true
        texture: Texture {
            tilingModeHorizontal: Texture.Repeat
            tilingModeVertical: Texture.Repeat
            source: "maps/art_paper_normal.png"
        }
    }
    property TextureInput transmission_texture: TextureInput {
        enabled: true
        texture: Texture {
            tilingModeHorizontal: Texture.Repeat
            tilingModeVertical: Texture.Repeat
            source: "maps/art_paper_trans.png"
        }
    }

    Shader {
        id: paperArtisticFragShader
        stage: Shader.Fragment
        shader: "shaders/paperArtistic.frag"
    }

    passes: [ Pass {
            shaders: paperArtisticFragShader
        }
    ]
}
