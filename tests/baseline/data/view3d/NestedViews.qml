// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers

View3D {
    anchors.fill: parent
    camera: PerspectiveCamera {
        z: 200
    }

    Model {
        id: lens
        source: "#Rectangle"
        materials: [
            PrincipledMaterial {
                lighting: PrincipledMaterial.NoLighting
                baseColorMap: Texture {
                    id: texture
                    sourceItem: sourceView
                }
            }
        ]
    }

    View3D {
        id: sourceView
        visible: false
        width: 512
        height: 512
        PerspectiveCamera {
            z: 300
        }

        Model {
            source: "#Cube"
            materials: [ PrincipledMaterial {
                    baseColor: "green"
                    lighting: PrincipledMaterial.NoLighting
                }
            ]

            // NumberAnimation  on eulerRotation.y {
            //     duration: 10000
            //     easing.type: Easing.InOutQuad
            //     from: 0
            //     to: 360
            //     running: true
            //     loops: -1
            // }
        }
    }
}
