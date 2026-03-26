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
            z: 250
        }
        DirectionalLight {
            eulerRotation.y: 180
            ambientColor: Qt.rgba(1.0, 1.0, 1.0, 1.0);
        }

        Model {
            x: -75
            source: "#Rectangle"
            materials: [ PrincipledMaterial {
                    lighting: DefaultMaterial.NoLighting
                    baseColorMap: Texture {
                        magFilter: Texture.Nearest
                        textureProvider: QuadTextureProvider {
                            id: quad
                            fragmentShaderCode: `
                            void MAIN() {
                                if (OUTPUT_SIZE.x > 8) {
                                    FRAGCOLOR = vec4(INPUT_UV, 0, 1);
                                } else {
                                    FRAGCOLOR = vec4(0, INPUT_UV, 1);
                                }
                            }`
                        }
                        tilingModeHorizontal: Texture.ClampToEdge
                        tilingModeVertical: Texture.ClampToEdge
                    }
            } ]
        }

        Model {
            x: 75
            source: "#Rectangle"
            materials: [ PrincipledMaterial {
                    lighting: DefaultMaterial.NoLighting
                    baseColorMap: Texture {
                        magFilter: Texture.Nearest
                        textureProvider: QuadTextureProvider {
                            id: quad2
                            fragmentShaderCode: `
                            void MAIN() {
                                if (OUTPUT_SIZE.x > 8) {
                                    FRAGCOLOR = vec4(0, INPUT_UV, 1);
                                } else {
                                    FRAGCOLOR = vec4(INPUT_UV, 0, 1);
                                }
                            }`
                        }
                        tilingModeHorizontal: Texture.ClampToEdge
                        tilingModeVertical: Texture.ClampToEdge
                    }
            } ]
        }
    }

    NumberAnimation {
        target: quad
        property: "width"
        duration: 200
        from: 1
        to: 16
        running: true
    }

    NumberAnimation {
        target: quad
        property: "height"
        duration: 200
        from: 1
        to: 16
        running: true
    }

    NumberAnimation {
        target: quad2
        property: "width"
        duration: 200
        from: 1
        to: 16
        running: true
    }

    NumberAnimation {
        target: quad2
        property: "height"
        duration: 200
        from: 1
        to: 16
        running: true
    }
}
