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
    property real vignetteStrength: 15 // 0 - 15
    property vector3d vignetteColor: Qt.vector3d(0.5, 0.5, 0.5)
    property real vignetteRadius: 0.35 // 0 - 5

    Shader {
        id: vignette
        stage: Shader.Fragment
        shader: "qrc:/qtquick3deffects/shaders/vignette.frag"
    }

    passes: [
        Pass {
            shaders: vignette
        }
    ]
}
