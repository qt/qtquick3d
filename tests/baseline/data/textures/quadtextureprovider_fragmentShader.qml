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
            id: rect
            source: "#Rectangle"
            property bool useFrag1: true

            materials: [ PrincipledMaterial {
                    lighting: DefaultMaterial.NoLighting
                    baseColorMap: Texture {
                        magFilter: Texture.Nearest
                        textureProvider: QuadTextureProvider {
                            fragmentShaderCode: rect.useFrag1 ? `
                            void MAIN() {
                                FRAGCOLOR = vec4(INPUT_UV, 0, 1);
                            }` : `
                            void MAIN() {
                                FRAGCOLOR = vec4(0, INPUT_UV, 1);
                            }`
                        }
                        tilingModeHorizontal: Texture.ClampToEdge
                        tilingModeVertical: Texture.ClampToEdge
                    }
            } ]
        }
    }

    FrameAnimation {
        running: true
        onTriggered: {
            rect.useFrag1 = !rect.useFrag1
            running = false
        }
    }
}
