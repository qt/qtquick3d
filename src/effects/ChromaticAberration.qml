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
    property TextureInput maskTexture: TextureInput {
        texture: Texture {
            source: "qrc:/qtquick3deffects/maps/white.png"
            tilingModeHorizontal: Texture.Repeat
            tilingModeVertical: Texture.Repeat
        }
    }
    property real aberrationAmount: 50
    property real focusDepth: 600

    Shader {
        id: chromaticAberration
        stage: Shader.Fragment
        shader: "qrc:/qtquick3deffects/shaders/chromaticaberration.frag"
    }

    passes: [
        Pass {
            shaders: chromaticAberration
        }
    ]
}
