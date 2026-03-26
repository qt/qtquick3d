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
        id: tex0
        textureProvider: QuadTextureProvider {
            width: 64
            height: 64
            fragmentShaderCode: `
            void MAIN() {
            vec4 src = texture(src, INPUT_UV);
            FRAGCOLOR = vec4(1, src.gb, 1);
            }`
            property Texture src: tex1
        }
        tilingModeHorizontal: Texture.ClampToEdge
        tilingModeVertical: Texture.ClampToEdge
    }
    Texture {
        id: tex1
        textureProvider: QuadTextureProvider {
            width: 64
            height: 64
            fragmentShaderCode: `
            void MAIN() {
            vec4 src = texture(src, INPUT_UV);
            FRAGCOLOR = vec4(0, 0.65, src.b, 1);
            }`
            property Texture src: tex2
        }
        tilingModeHorizontal: Texture.ClampToEdge
        tilingModeVertical: Texture.ClampToEdge
    }
    Texture {
        id: tex2
        textureProvider: QuadTextureProvider {
            id: provider2
            property real t: 0
            width: 64
            height: 64
            fragmentShaderCode: `
            void MAIN() {
            FRAGCOLOR = vec4(0, 0, t, 1);
            }`
        }
        tilingModeHorizontal: Texture.ClampToEdge
        tilingModeVertical: Texture.ClampToEdge
    }
    NumberAnimation {
        target: provider2
        property: "t"
        duration: 200
        from: 1
        to: 0.25
        running: true
    }
}
