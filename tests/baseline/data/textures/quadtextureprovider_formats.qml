// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers

Rectangle {
    width: 800
    height: 400
    color: "lightgray"

    View3D {
        id: root
        anchors.fill: parent

        camera: PerspectiveCamera {
            z: 100
        }

        DirectionalLight {
            eulerRotation.y: 180
            ambientColor: Qt.rgba(1, 1, 1, 1)
        }

        property var formats: [
            TextureData.RGBA8,
            TextureData.RGBA16F,
            TextureData.RGBA32F,
            TextureData.RGBE8,
            TextureData.R8,
            TextureData.R16,
            TextureData.R16F,
            TextureData.R32F
        ]

        Repeater3D {
            model: root.formats.length

            Model {
                source: "#Rectangle"

                x: (index % 4) * 60 - 90
                y: Math.floor(index / 4) * -60 + 30

                scale: Qt.vector3d(0.5, 0.5, 0.5)

                materials: PrincipledMaterial {
                    lighting: DefaultMaterial.NoLighting

                    baseColorMap: Texture {
                        magFilter: Texture.Nearest
                        textureProvider: QuadTextureProvider {
                            format: root.formats[index]
                            width: 16
                            height: 16
                            fragmentShaderCode: `
                                void MAIN() {
                                    FRAGCOLOR = vec4(INPUT_UV, 0.0, 1.0);
                                }
                            `
                        }

                        tilingModeHorizontal: Texture.ClampToEdge
                        tilingModeVertical: Texture.ClampToEdge
                    }
                }
            }
        }
    }
}
