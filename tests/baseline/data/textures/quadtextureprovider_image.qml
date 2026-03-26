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
                            id: quad
                            width: 64
                            height: 64
                            fragmentShaderCode: `
                            void MAIN() {
                                vec2 uv = INPUT_UV;
                                vec4 c = texture(checkers, uv);
                                FRAGCOLOR = mix(c, vec4(uv, 1, 1), mixT);
                            }`

                            property real mixT : 0
                            property Texture checkers : Texture {
                                source: "../shared/maps/checkers2.png"
                            }
                        }
                        tilingModeHorizontal: Texture.ClampToEdge
                        tilingModeVertical: Texture.ClampToEdge
                    }
            } ]
        }
    }

    NumberAnimation {
        target: quad
        property: "mixT"
        duration: 200
        from: 0
        to: 0.5
        running: true
    }
}
