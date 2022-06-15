// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick3D

//! [implementation]
CustomMaterial {
    property real time: 0.0
    property real amplitude: 5.0
    property real alpha: 1.0

    property bool texturing: false
    property bool textureFromItem: false
    property Item texSrc
    Texture {
        id: texFromFile
        source: "qt_logo.png"
    }
    Texture {
        id: texFromItem
        sourceItem: texSrc
    }
    property TextureInput tex: TextureInput {
        enabled: texturing
        texture: textureFromItem ? texFromItem : texFromFile
    }

    shadingMode: CustomMaterial.Unshaded
    sourceBlend: alpha < 1.0 ? CustomMaterial.SrcAlpha : CustomMaterial.NoBlend
    destinationBlend: alpha < 1.0 ? CustomMaterial.OneMinusSrcAlpha : CustomMaterial.NoBlend
    cullMode: CustomMaterial.BackFaceCulling

    vertexShader: "example.vert"
    fragmentShader: texturing ? "example_tex.frag" : "example.frag"
}
//! [implementation]
