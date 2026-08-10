// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Regression scene for QTBUG-149007: swapping to a SkyMaterial whose
// texture isn't ready yet, while its radianceMapSize also differs, used to
// leave QSSGRenderLayer::skyBoxSrb pointing at a QRhiTexture that
// ensureTextures() had just deleted while resizing the environment cube.

import QtQuick
import QtQuick3D

Item {
    id: root
    width: 64
    height: 64

    property bool useReadyMaterial: true

    // Shading is irrelevant to the bug; kept trivial.
    property string skyFragmentShaderCode: `
        void MAIN()
        {
            vec3 d = normalize(qt_eyeDir);
            FRAGCOLOR = vec4(d * 0.5 + 0.5, 1.0);
        }
    `

    View3D {
        id: view3D
        anchors.fill: parent
        camera: camera

        environment: SceneEnvironment {
            id: sceneEnvironment
            backgroundMode: SceneEnvironment.SkyMaterial
            skyMaterial: root.useReadyMaterial ? readyMaterial : stuckMaterial
        }

        PerspectiveCamera {
            id: camera
            position: Qt.vector3d(0, 0, 600)
        }

        DirectionalLight {
        }

        Model {
            source: "#Cube"
            materials: [ DefaultMaterial { } ]
        }

        // No texture property, so it's always ready.
        SkyMaterial {
            id: readyMaterial
            skyboxMode: SkyMaterial.Cubemap
            enableIBL: true
            radianceMapSize: 256
            fragmentShaderCode: root.skyFragmentShaderCode
        }

        // noiseTex never gets a source, so this material never becomes
        // ready; its different radianceMapSize forces the env cube resize.
        SkyMaterial {
            id: stuckMaterial
            skyboxMode: SkyMaterial.Cubemap
            enableIBL: true
            radianceMapSize: 512
            fragmentShaderCode: root.skyFragmentShaderCode
            property Texture noiseTex: Texture { }
        }
    }
}
