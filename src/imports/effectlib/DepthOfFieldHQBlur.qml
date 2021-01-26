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
import QtQuick3D.Effects 1.15

Effect {
    readonly property TextureInput sourceSampler: TextureInput {
        texture: Texture {}
    }
    readonly property TextureInput depthSampler: TextureInput {
        texture: Texture {}
    }
    property real focusDistance: 600
    property real focusRange: 100
    property real blurAmount: 4

    Shader {
        id: downsampleVert
        stage: Shader.Vertex
        shader: "shaders/downsample.vert"
    }
    Shader {
        id: downsampleFrag
        stage: Shader.Fragment
        shader: "shaders/downsample.frag"
    }

    Shader {
        id: blurVert
        stage: Shader.Vertex
        shader: "shaders/depthoffieldblur.vert"
    }
    Shader {
        id: blurFrag
        stage: Shader.Fragment
        shader: "shaders/depthoffieldblur.frag"
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
            commands: BufferInput {
                param: "depthSampler"
            }
            output: downsampleBuffer
        },
        Pass {
            shaders: [ blurVert, blurFrag ]
            commands: [
                BufferInput {
                    buffer: downsampleBuffer
                },
                BufferInput {
                    param: "sourceSampler"
                },
                DepthInput {
                    param: "depthSampler"
                }
            ]
        }
    ]
}
