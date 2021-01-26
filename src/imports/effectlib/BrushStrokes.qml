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
    property TextureInput noiseSample: TextureInput {
        texture: Texture {
            tilingModeHorizontal: Texture.Repeat
            tilingModeVertical: Texture.Repeat
            source: "maps/brushnoise.png"
        }
    }
    property real brushLength: 1.0  // 0 - 3
    property real brushSize: 100.0  // 10 - 200
    property real brushAngle: 45.0
    readonly property real sinAlpha: Math.sin(degrees_to_radians(brushAngle))
    readonly property real cosAlpha: Math.cos(degrees_to_radians(brushAngle))

    function degrees_to_radians(degrees) {
        var pi = Math.PI;
        return degrees * (pi/180);
    }

    Shader {
        id: brushstrokes
        stage: Shader.Fragment
        shader: "shaders/brushstrokes.frag"
    }

    passes: [
        Pass {
            shaders: brushstrokes
        }
    ]
}
