// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

import QtQuick
import QtQuick3D
import QtQuick3D.Effects

Effect {
    property real amount: 0.5

    Shader {
        id: desaturate
        stage: Shader.Fragment
        shader: "qrc:/qtquick3deffects/shaders/desaturate.frag"
    }

    passes: [
        Pass {
            shaders: desaturate
        }
    ]
}
