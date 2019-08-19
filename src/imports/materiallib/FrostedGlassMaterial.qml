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
    property real roughness: 1.0
    property real blur_size: 8.0
    property real refract_depth: 5
    property bool uEnvironmentMappingEnabled: true
    property bool uShadowMappingEnabled: false
    property real glass_bfactor: 0.0
    property bool glass_binside: false
    property real uFresnelPower: 1.0
    property real reflectivity_amount: 0.1
    property real glass_ior: 1.1
    property real intLightFall: 2.0
    property real intLightRot: 0.0
    property real intLightBrt: 0.0
    property real bumpScale: 0.5
    property int bumpBands: 1
    property vector3d bumpCoords: Qt.vector3d(1.0, 1.0, 1.0)
    property vector2d intLightPos: Qt.vector2d(0.5, 0.0)
    property vector3d glass_color: Qt.vector3d(0.9, 0.9, 0.9)
    property vector3d intLightCol: Qt.vector3d(0.9, 0.9, 0.9)
    hasTransparency: true

    shaderInfo: CustomMaterialShaderInfo {
        version: "330"
        type: "GLSL"
        shaderKey: CustomMaterialShaderInfo.Refraction | CustomMaterialShaderInfo.Glossy
        layers: 1
    }

    property CustomMaterialTexture glass_bump: CustomMaterialTexture {
        type: CustomMaterialTexture.Environment
        enabled: true
        image: Texture {
            id: glassBumpMap
            source: "maps/spherical_checker.png"
        }
    }

    property CustomMaterialTexture uEnvironmentTexture: CustomMaterialTexture {
            type: CustomMaterialTexture.Environment
            enabled: uEnvironmentMappingEnabled
            image: Texture {
                id: envImage
                source: "maps/spherical_checker.png"
            }
    }
    property CustomMaterialTexture uBakedShadowTexture: CustomMaterialTexture {
            type: CustomMaterialTexture.LightmapShadow
            enabled: uShadowMappingEnabled
            image: Texture {
                id: shadowImage
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
        id: mainShader
        stage: CustomMaterialShader.Fragment
        shader: "shaders/frostedThinGlass.frag"
    }
    CustomMaterialShader {
        id: noopShader
        stage: CustomMaterialShader.Fragment
        shader: "shaders/frostedThinGlassNoop.frag"
    }
    CustomMaterialShader {
        id: preBlurShader
        stage: CustomMaterialShader.Fragment
        shader: "shaders/frostedThinGlassPreBlur.frag"
    }
    CustomMaterialShader {
        id: blurXShader
        stage: CustomMaterialShader.Fragment
        shader: "shaders/frostedThinGlassBlurX.frag"
    }
    CustomMaterialShader {
        id: blurYShader
        stage: CustomMaterialShader.Fragment
        shader: "shaders/frostedThinGlassBlurY.frag"
    }

    CustomMaterialBuffer {
        id: frameBuffer
        name: "frameBuffer"
        format: CustomMaterialBuffer.Unknown
        magOp: CustomMaterialBuffer.Linear
        coordOp: CustomMaterialBuffer.ClampToEdge
        sizeMultiplier: 1.0
        bufferFlags: CustomMaterialBuffer.None // aka frame
    }

    CustomMaterialBuffer {
        id: dummyBuffer
        name: "dummyBuffer"
        format: CustomMaterialBuffer.RGBA8
        magOp: CustomMaterialBuffer.Linear
        coordOp: CustomMaterialBuffer.ClampToEdge
        sizeMultiplier: 1.0
        bufferFlags: CustomMaterialBuffer.None // aka frame
    }

    CustomMaterialBuffer {
        id: tempBuffer
        name: "tempBuffer"
        format: CustomMaterialBuffer.RGBA16F
        magOp: CustomMaterialBuffer.Linear
        coordOp: CustomMaterialBuffer.ClampToEdge
        sizeMultiplier: 0.5
        bufferFlags: CustomMaterialBuffer.None // aka frame
    }

    CustomMaterialBuffer {
        id: blurYBuffer
        name: "tempBlurY"
        format: CustomMaterialBuffer.RGBA16F
        magOp: CustomMaterialBuffer.Linear
        coordOp: CustomMaterialBuffer.ClampToEdge
        sizeMultiplier: 0.5
        bufferFlags: CustomMaterialBuffer.None // aka frame
    }

    CustomMaterialBuffer {
        id: blurXBuffer
        name: "tempBlurX"
        format: CustomMaterialBuffer.RGBA16F
        magOp: CustomMaterialBuffer.Linear
        coordOp: CustomMaterialBuffer.ClampToEdge
        sizeMultiplier: 0.5
        bufferFlags: CustomMaterialBuffer.None // aka frame
    }

    passes: [ CustomMaterialPass {
            shaders: noopShader
            output: dummyBuffer
            commands: [ CustomMaterialBufferBlit {
                    destination: frameBuffer
                }
            ]
        }, CustomMaterialPass {
            shaders: preBlurShader
            output: tempBuffer
            commands: [ CustomMaterialBufferInput {
                    buffer: frameBuffer
                    param: "OriginBuffer"
                }
            ]
        }, CustomMaterialPass {
            shaders: blurXShader
            output: blurXBuffer
            commands: [ CustomMaterialBufferInput {
                    buffer: tempBuffer
                    param: "BlurBuffer"
                }
            ]
        }, CustomMaterialPass {
            shaders: blurYShader
            output: blurYBuffer
            commands: [ CustomMaterialBufferInput {
                    buffer: blurXBuffer
                    param: "BlurBuffer"
                }, CustomMaterialBufferInput {
                    buffer: tempBuffer
                    param: "OriginBuffer"
                }
            ]
        }, CustomMaterialPass {
            shaders: mainShader
            commands: [CustomMaterialBufferInput {
                    buffer: blurYBuffer
                    param: "refractiveTexture"
                }, CustomMaterialBlending {
                    srcBlending: CustomMaterialBlending.SrcAlpha
                    destBlending: CustomMaterialBlending.OneMinusSrcAlpha
                }
            ]
        }
    ]
}
