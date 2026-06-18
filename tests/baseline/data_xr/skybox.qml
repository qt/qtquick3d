// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
//
import QtQuick
import QtQuick.Layouts
import QtQuick3D.Helpers
import QtQuick3D
import QtQuick3D.Xr

Node {
    property SceneEnvironment qmlxr_environment: SceneEnvironment {
        backgroundMode: SceneEnvironment.SkyBox
        lightProbe: Texture {
            textureData: ProceduralSkyTextureData { }
        }
    }

    DirectionalLight {}

    Model {
        source: "#Sphere"
        materials: PrincipledMaterial {
            baseColor: "green"
        }
        position: Qt.vector3d(25, 0, 0)
        scale: Qt.vector3d(0.5, 0.5, 0.5)
    }
}
