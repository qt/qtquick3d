// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick3D

Item {
    id: root
    width: 200
    height: 200

    // renderMode: Inline drives rendering through a QQuick3DSGRenderNode, which is the
    // code path that regressed in QTBUG-148707. The reflection probe and the shadow
    // casting light make sure the reflection and shadow map managers touched by
    // QQuick3DSceneRenderer::releaseCachedResources() are actually created.
    View3D {
        anchors.fill: parent
        renderMode: View3D.Inline

        environment: SceneEnvironment {
            backgroundMode: SceneEnvironment.Color
            clearColor: "black"
        }

        PerspectiveCamera { z: 300 }

        DirectionalLight { castsShadow: true }

        ReflectionProbe {
            position: Qt.vector3d(0, 0, 0)
            boxSize: Qt.vector3d(400, 400, 400)
        }

        Model {
            source: "#Sphere"
            materials: PrincipledMaterial {
                baseColor: "white"
                metalness: 1.0
                roughness: 0.1
            }
        }
    }
}
