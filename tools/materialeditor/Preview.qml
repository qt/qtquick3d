/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.settings
import QtQuick3D

ColumnLayout {
    id: previewRoot

    property url skyBoxTexturePath: "qrc:/assets/skybox/OpenfootageNET_lowerAustria01-1024.hdr"
    property CustomMaterial currentMaterial: CustomMaterial {

    }

    property PrincipledMaterial fallbackMaterial: PrincipledMaterial {
        baseColor: "magenta"
    }

    property alias modelInstance: model
    property alias rootNode: resourceRoot

    Settings {
        property alias cameraOriginRotation: originNode.rotation
        property alias cameraRotation: sceneCamera.rotation
        property alias cameraPosition: sceneCamera.position
    }

    View3D {
        id: view
        Layout.preferredHeight: previewRoot.height
        Layout.preferredWidth: previewRoot.width
        environment: SceneEnvironment {
            id: sceneEnvironment
            backgroundMode: previewControls.enableIBL ? SceneEnvironment.SkyBox : SceneEnvironment.Transparent
            lightProbe: previewControls.enableIBL ? skyboxTexture : null
        }

        Texture {
            id: skyboxTexture
            source: previewRoot.skyBoxTexturePath
        }

        Node {
            id: resourceRoot
        }

        property alias cameraOrigin: originNode

        Node {
            id: originNode
            PerspectiveCamera {
                id: sceneCamera
                z: 300
            }
        }

        camera: sceneCamera

        DirectionalLight {
            id: light
            z: 600
            eulerRotation: Qt.vector3d(30, 0, 0)
            visible: previewControls.enableDirectionalLight
        }

        Model {
            id: model
            source: previewControls.modelSource
            materials: [ currentMaterial, fallbackMaterial ]
        }

        OrbitCameraController {
            origin: originNode
            camera: sceneCamera
        }
    }

    PreviewControls {
        id: previewControls
        width: parent.width
        targetView: view
    }
}
