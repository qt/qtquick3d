// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick3D

Effect {
    property vector3d cameraPosition
    property vector3d cameraForward
    property matrix4x4 invViewMatrix
    property int marchSteps: 96
    property real nearPlane: 1.0
    property real farPlane: 4000.0
    property real frameBaseJitter: 0.0
    property real jitterIntensity: 0.019

    property Texture froxelTexture
    property TextureInput froxelGrid: TextureInput {
        texture: froxelTexture
    }

    property TextureInput blueNoise: TextureInput {
        texture: Texture {
            minFilter: Texture.Linear
            magFilter: Texture.Linear
            tilingModeHorizontal: Texture.Repeat
            tilingModeVertical: Texture.Repeat
            generateMipmaps: false
            source: Qt.resolvedUrl("qrc:/res/textures/blue_noise.png")
        }
    }

    passes: Pass {
        shaders: Shader {
            stage: Shader.Fragment
            shader: Qt.resolvedUrl("shaders/volumetricLight.frag")
        }
    }
}
