// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick3D
import io.qt.tests.auto.BufferManager

View3D {
    id: view

    property bool customTextures: true
    property list<url> textureSources
    property real cameraZ: 600

    width: 640
    height: 480

    PerspectiveCamera {
        z: view.cameraZ
    }

    Repeater3D {
        model: view.customTextures ? 64 : view.textureSources.length
        delegate: Model {
            id: quad

            required property int index

            source: "#Rectangle"
            x: (index % 8 - 3.5) * 40
            y: (Math.floor(index / 8) - 3.5) * 40
            scale: Qt.vector3d(0.3, 0.3, 1)
            materials: PrincipledMaterial {
                lighting: PrincipledMaterial.NoLighting
                baseColorMap: Texture {
                    source: view.customTextures ? "" : view.textureSources[quad.index]
                    textureData: view.customTextures ? gradient : null

                    // Identical metadata gives distinct cache entries the same hash.
                    GradientTexture {
                        id: gradient
                        width: 16
                        height: 16
                    }
                }
            }
        }
    }
}
