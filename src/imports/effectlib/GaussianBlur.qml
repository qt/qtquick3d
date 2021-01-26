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
    property real amount: 2 // 0 - 10
    Shader {
        id: vertical
        stage: Shader.Vertex
        shader: "shaders/blurvertical.vert"
    }
    Shader {
        id: horizontal
        stage: Shader.Vertex
        shader: "shaders/blurhorizontal.vert"
    }
    Shader {
        id: gaussianblur
        stage: Shader.Fragment
        shader: "shaders/gaussianblur.frag"
    }

    Buffer {
        id: tempBuffer
        name: "tempBuffer"
        format: Buffer.RGBA8
        textureFilterOperation: Buffer.Linear
        textureCoordOperation: Buffer.ClampToEdge
        bufferFlags: Buffer.None // aka frame
    }

    passes: [
        Pass {
            shaders: [ horizontal, gaussianblur ]
            output: tempBuffer
        },
        Pass {
            shaders: [ vertical, gaussianblur ]
            commands: [
                BufferInput {
                    buffer: tempBuffer
                }
            ]
        }
    ]
}
