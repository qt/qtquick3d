// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick3D
import QtQuick3D.AssetUtils

Rectangle {
    width: 400
    height: 400
    color: "lightgray"

    View3D {
        anchors.fill: parent
        renderMode: View3D.Offscreen

        camera: PerspectiveCamera {
            z: 500
        }

        DirectionalLight {
        }

        // Checkerboard so transparency from the opacity map is visible
        Model {
            source: "#Rectangle"
            materials: [ PrincipledMaterial {
                lighting: DefaultMaterial.NoLighting
                baseColorMap: Texture {
                    source: "../shared/maps/checkers1.png"
                    tilingModeHorizontal: Texture.Repeat
                    tilingModeVertical: Texture.Repeat
                    scaleU: 20
                    scaleV: 20
                }
            }]
            z: -500
            scale: Qt.vector3d(10, 10, 1)
        }

        RuntimeLoader {
            source: "../shared/obj/plane_prb_opacity.obj"
        }
    }
}
