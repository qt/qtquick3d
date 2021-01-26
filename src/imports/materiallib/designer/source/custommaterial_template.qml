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
import QtQuick3D.Materials 1.15

CustomMaterial {
    shaderInfo: shaderInformation
    passes: renderPass

    ShaderInfo {
        id: shaderInformation
        type: "GLSL"
        version: "330"
    }

    Pass {
        id: renderPass
        shaders: [vertShader, fragShader]
    }

    Shader {
        id: vertShader
        stage: Shader.Vertex
        shader: "custom_material_default_shader.vert"
    }

    Shader {
        id: fragShader
        stage: Shader.Fragment
        shader: "custom_material_default_shader.frag"
    }
}
