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
            ambientColor: Qt.rgba(1.0, 1.0, 1.0, 1.0);
        }

        Model {
            source: "#Rectangle"
            materials: [ PrincipledMaterial {
                    lighting: DefaultMaterial.NoLighting
                    baseColorMap: Texture {
                        textureProvider: QuadTextureProvider {
                            width: 64
                            height: 64
                            fragmentShaderCode: `
                            void MAIN() {
                                vec2 uv = INPUT_UV;
                                FRAGCOLOR = vec4(uv.x, uv.y, 1, 1);
                            }`
                        }
                        tilingModeHorizontal: Texture.ClampToEdge
                        tilingModeVertical: Texture.ClampToEdge
                    }
            } ]
        }
    }
}
