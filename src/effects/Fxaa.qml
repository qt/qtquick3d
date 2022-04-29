/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

import QtQuick
import QtQuick3D
import QtQuick3D.Effects

Effect {
    readonly property TextureInput sprite: TextureInput {
        texture: Texture {}
    }

    Shader {
        id: rgbl
        stage: Shader.Fragment
        shader: "qrc:/qtquick3deffects/shaders/fxaaRgbl.frag"
    }
    Shader {
        id: blur
        stage: Shader.Fragment
        shader: "qrc:/qtquick3deffects/shaders/fxaaBlur.frag"
    }
    Buffer {
        id: rgblBuffer
        name: "rgbl_buffer"
        format: Buffer.RGBA8
        textureFilterOperation: Buffer.Linear
        textureCoordOperation: Buffer.ClampToEdge
        bufferFlags: Buffer.None // aka frame
    }

    passes: [
        Pass {
            shaders: rgbl
            output: rgblBuffer
        },
        Pass {
            shaders: blur
            commands: [
                // INPUT is the texture for rgblBuffer
                BufferInput {
                    buffer: rgblBuffer
                },
                // the actual input texture is exposed as sprite
                BufferInput {
                    sampler: "sprite"
                }
            ]
        }
    ]
}
