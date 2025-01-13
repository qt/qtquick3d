// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers
import QtQuick3D.Xr

import xr_shared

XrView {
    id: xrView
    XrErrorDialog { id: err }
    onInitializeFailed: (errorString) => err.run("XR freeform teleportation", errorString)
    referenceSpace: XrView.ReferenceSpaceStage

    environment: ExtendedSceneEnvironment {
        id: extendedSceneEnvironment
        clearColor: "black"
        backgroundMode: SceneEnvironment.Color
        antialiasingMode: SceneEnvironment.MSAA
        antialiasingQuality: SceneEnvironment.High
        lightProbe: Texture {
            textureData: ProceduralSkyTextureData {}
        }
        exposure: teleporter.screenVisibility
        specularAAEnabled: true
    }

    xrOrigin: XrOrigin {
        id: xrOrigin

        camera: XrCamera {
            id: trackingCamera
        }

        Component.onCompleted: {
            teleporter.teleportTo(Qt.vector3d(1000, 200, -300))
        }

        XrController {
            id: xrRightController
            controller: XrController.RightController
            poseSpace: XrController.AimPose

            XrInputAction {
                id: thumbstickX
                hand: XrInputAction.RightHand
                actionId: [XrInputAction.ThumbstickX]
            }

            XrInputAction {
                id: thumbstickY
                hand: XrInputAction.RightHand
                actionId: [XrInputAction.ThumbstickY]
            }

            XrInputAction {
                id: trackpadX
                hand: XrInputAction.RightHand
                actionId: [XrInputAction.TrackpadX]
            }

            XrInputAction {
                id: trackpadY
                hand: XrInputAction.RightHand
                actionId: [XrInputAction.TrackpadY]
            }

            XrInputAction {
                id: trackpadPressed
                hand: XrInputAction.RightHand
                actionId: [XrInputAction.TrackpadPressed]
            }

            property real xValue: trackpadPressed.pressed ? trackpadX.value : thumbstickX.value
            property real yValue: trackpadPressed.pressed ? trackpadY.value : thumbstickY.value


            Model {
                source: "#Cube"
                scale: Qt.vector3d(0.1, 0.1, 0.1)
                materials: PrincipledMaterial {
                    baseColor: "white"
                }
            }
        }
    }

    //! [FreeformTeleporter component]
    FreeformTeleporter {
        id: teleporter

        view: xrView
        originNode: xrOrigin
        cameraNode: xrOrigin.camera
        beamHandle: xrRightController

        xStickValue: xrRightController.xValue
        yStickValue: xrRightController.yValue

        onDoTeleportation: (cameraOriginPosition)=> {
                               xrOrigin.position = cameraOriginPosition
                           }
        onDoRotation: (cameraOriginRotation, cameraOriginPosition)=> {
                          xrOrigin.rotation = cameraOriginRotation
                          xrOrigin.position = cameraOriginPosition
                      }
    }
    //! [FreeformTeleporter component]

    Building {}
}


