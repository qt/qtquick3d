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
    property real radius: 0.25              // 0 - 1
    property real distortionStrength: 1.0   // -10 - 10
    property vector2d center: Qt.vector2d(0.5, 0.5)

    Shader {
        id: distortionVert
        stage: Shader.Vertex
        shader: "shaders/distortion.vert"
    }

    Shader {
        id: distortionFrag
        stage: Shader.Fragment
        shader: "shaders/distortionspiral.frag"
    }

    passes: [
        Pass {
            shaders: [ distortionVert, distortionFrag ]
        }
    ]
}
