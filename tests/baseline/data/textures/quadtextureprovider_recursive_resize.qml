// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers

Rectangle {
    width: 400
    height: 400
    color: "lightgray"

    View3D {
        anchors.fill: parent

        camera: PerspectiveCamera {
            z: 100
        }

        DirectionalLight {
            eulerRotation.y: 180
            ambientColor: Qt.rgba(1.0, 1.0, 1.0, 1.0)
        }

        Model {
            source: "#Rectangle"
            materials: [
                PrincipledMaterial {
                    lighting: DefaultMaterial.NoLighting
                    baseColorMap: tex0
                }
            ]
        }
    }

    Texture {
        id: tex2
        textureProvider: QuadTextureProvider {
            id: quad2
            width: 32 * factor
            height: 32 * factor
            property real factor: 0
            fragmentShaderCode: `
            void MAIN() {
            FRAGCOLOR = vec4(INPUT_UV*0.25, 0.25, 1);
            }`
        }
        magFilter: Texture.Nearest
        tilingModeHorizontal: Texture.ClampToEdge
        tilingModeVertical: Texture.ClampToEdge
    }
    Texture {
        id: tex1
        textureProvider: QuadTextureProvider {
            id: quad1
            width: 32 * factor
            height: 32 * factor
            property real factor: 0
            fragmentShaderCode: `
            void MAIN() {
            vec4 src = texture(src, INPUT_UV);
            FRAGCOLOR = vec4(src.r, src.g + 0.65, src.b, 1);
            }`
            property Texture src: tex2
        }
        magFilter: Texture.Nearest
        tilingModeHorizontal: Texture.ClampToEdge
        tilingModeVertical: Texture.ClampToEdge
    }
    Texture {
        id: tex0
        textureProvider: QuadTextureProvider {
            width: 64
            height: 64
            fragmentShaderCode: `
            void MAIN() {
            vec4 src = texture(src, INPUT_UV);
            FRAGCOLOR = vec4(src.r + 0.65, src.g, src.b, 1);
            }`
            property Texture src: tex1
        }
        magFilter: Texture.Nearest
        tilingModeHorizontal: Texture.ClampToEdge
        tilingModeVertical: Texture.ClampToEdge
    }

    NumberAnimation {
        target: quad2
        property: "factor"
        duration: 200
        from: 0
        to: 1
        running: true
    }

    NumberAnimation {
        target: quad1
        property: "factor"
        duration: 200
        from: 0
        to: 1
        running: true
    }
}
