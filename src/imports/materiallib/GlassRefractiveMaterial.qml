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
    property bool uEnvironmentMappingEnabled: true
    property bool uShadowMappingEnabled: false
    property real uFresnelPower: 1.0
    property real reflectivity_amount: 1.0
    property real glass_ior: 1.5
    property real roughness: 0.0
    property vector3d glass_color: Qt.vector3d(0.9, 0.9, 0.9)
    hasTransparency: true

    shaderInfo: ShaderInfo {
        version: "330"
        type: "GLSL"
        shaderKey: ShaderInfo.Refraction | ShaderInfo.Glossy
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

    Shader {
        id: simpleGlassRefractiveFragShader
        stage: Shader.Fragment
        shader: "shaders/simpleGlassRefractive.frag"
    }

    Buffer {
        id: tempBuffer
        name: "temp_buffer"
        format: Buffer.Unknown
        textureFilterOperation: Buffer.Linear
        textureCoordOperation: Buffer.ClampToEdge
        sizeMultiplier: 1.0
        bufferFlags: Buffer.None // aka frame
    }

    passes: [ Pass {
            shaders: simpleGlassRefractiveFragShader
            commands: [ BufferBlit {
                    destination: tempBuffer
                }, BufferInput {
                    buffer: tempBuffer
                    param: "refractiveTexture"
                }, Blending {
                    srcBlending: Blending.SrcAlpha
                    destBlending: Blending.OneMinusSrcAlpha
                }
            ]
        }
    ]
}
