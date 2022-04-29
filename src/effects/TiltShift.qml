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
    readonly property TextureInput sourceSampler: TextureInput {
        texture: Texture {}
    }
    property real focusPosition: 0.5    // 0 - 1
    property real focusWidth: 0.2       // 0 - 1
    property real blurAmount: 4         // 0 - 10
    property bool isVertical: false
    property bool isInverted: false

    Shader {
        id: downsampleVert
        stage: Shader.Vertex
        shader: "qrc:/qtquick3deffects/shaders/downsample.vert"
    }
    Shader {
        id: downsampleFrag
        stage: Shader.Fragment
        shader: "qrc:/qtquick3deffects/shaders/downsampletiltshift.frag"
    }

    Shader {
        id: blurVert
        stage: Shader.Vertex
        shader: "qrc:/qtquick3deffects/shaders/poissonblurtiltshift.vert"
    }
    Shader {
        id: blurFrag
        stage: Shader.Fragment
        shader: "qrc:/qtquick3deffects/shaders/poissonblurtiltshift.frag"
    }

    Buffer {
        id: downsampleBuffer
        name: "downsampleBuffer"
        format: Buffer.RGBA8
        textureFilterOperation: Buffer.Linear
        textureCoordOperation: Buffer.ClampToEdge
        bufferFlags: Buffer.None
        sizeMultiplier: 0.5
    }

    passes: [
        Pass {
            shaders: [ downsampleVert, downsampleFrag ]
            output: downsampleBuffer
        },
        Pass {
            shaders: [ blurVert, blurFrag ]
            commands: [
                // INPUT is the texture for downsampleBuffer
                BufferInput {
                    buffer: downsampleBuffer
                },
                // the pass' input texture is exposed as sourceSampler
                BufferInput {
                    sampler: "sourceSampler"
                }
            ]
        }
    ]
}
