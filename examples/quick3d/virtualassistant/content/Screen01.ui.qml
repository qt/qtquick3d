// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick3D
import QtQuick.Layouts
import QtQuick3D.Helpers
import VirtualAssistant.Constants
import Quick3DAssets.VirtualAssistant

Rectangle {
    width: Constants.width
    height: Constants.height

    View3D {
        id: view3D
        anchors.fill: parent

        environment: ExtendedSceneEnvironment {
            backgroundMode: SceneEnvironment.SkyBox
            lightProbe: Texture { source: Constants.sceneName }
            antialiasingMode: SceneEnvironment.MSAA
            antialiasingQuality: SceneEnvironment.VeryHigh
            fxaaEnabled: true
            probeExposure: 0.6
            probeOrientation: Qt.vector3d(0, settingsPanel.skyboxRotation, 0)
            specularAAEnabled: true
            tonemapMode: SceneEnvironment.TonemapModeLinear
            vignetteEnabled: true
            vignetteRadius: 0.15
        }

        Node {
            id: scene

            VirtualAssistant {
                id: virtualAssistant
            }
        }

        Node {
            id: cameraNode
            PerspectiveCamera {
                id: sceneCamera
                y: 5
                z: 15
                fieldOfView: settingsPanel.cameraFov
                clipNear: 1.0
            }
        }

        OrbitCameraController {
            anchors.fill: parent
            origin: cameraNode
            camera: sceneCamera
            enabled: settingsPanel.cameraControllerEnabled
        }

        MouseArea {
            anchors.fill: parent

            Connections {
                function onClicked(mouse) {
                    var result = view3D.pick(mouse.x, mouse.y);

                    if (!result.objectHit)
                        return

                    virtualAssistant.animateObject(result.objectHit.objectName)
                }
            }
        }
    }

    Item {
        id: __materialLibrary__
        DefaultMaterial {
            id: defaultMaterial
            objectName: "Default Material"
            diffuseColor: "#4aee45"
        }
    }

    TabBar {
        id: bar
        anchors.left: parent.left
        anchors.top: parent.top
        width: 300

        TabButton {
            text: qsTr("Animations")
        }
        TabButton {
            text: qsTr("Settings")
        }
    }

    StackLayout {
        anchors.top: bar.bottom
        anchors.bottom: parent.bottom
        width: bar.width

        currentIndex: bar.currentIndex

        ControlPanel {
            id: controlPanel

            Connections {
                target: controlPanel
                function onClicked(index: int) {
                    virtualAssistant.runAnimation(index);
                }
            }
        }

        SettingsPanel {
            id: settingsPanel

            camera: sceneCamera
        }
    }
}


