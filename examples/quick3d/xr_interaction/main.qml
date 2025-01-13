// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Shapes
import QtQuick.Layouts

import QtQuick3D
import QtQuick3D.Helpers

import QtQuick3D.Xr

import xr_shared

pragma ComponentBehavior: Bound

XrView {
    id: xrView
    referenceSpace: XrView.ReferenceSpaceStage

    depthSubmissionEnabled: true

    environment: SceneEnvironment {
        id: sceneEnvironment
        lightProbe: Texture {
            textureData: ProceduralSkyTextureData {
            }
        }
        antialiasingMode: SceneEnvironment.MSAA
        antialiasingQuality: SceneEnvironment.High
        backgroundMode: SceneEnvironment.Color
        clearColor: "skyblue"
        probeHorizon: 0.5
    }

    DirectionalLight {
        eulerRotation.x: -30
        eulerRotation.y: -70
    }

    //! [haptics]
    XrHapticFeedback {
        id: hapticFeedback
        controller: XrHapticFeedback.RightController
        hapticEffect: XrSimpleHapticEffect {
            amplitude: 0.5
            duration: 30
            frequency: 3000
        }
        property Model prevObj: null
        function handleHover(obj: Model) {
            if (obj && obj !== prevObj)
                start()
            prevObj = obj
        }
    }
    //! [haptics]

    xrOrigin: XrOrigin {
        id: theOrigin

        //! [connections]
        AimController {
            id: rightAim
            controller: XrController.RightController
            view: xrView

            onObjectPressed: (obj, pos, dir) => {
                gadgetBox.handlePress(obj, pos, dir)
            }
            onObjectHovered: (obj) => {
                gadgetBox.handleHover(obj)
                hapticFeedback.handleHover(obj)
            }
            onMoved: (pos, dir) => {
                gadgetBox.handleMove(pos, dir)
            }
            onReleased: {
                gadgetBox.handleRelease()
            }
            onObjectGrabbed: (obj) => {
                if (!(obj instanceof XrGadget))
                    startGrab(obj)
            }
            Model {
                source: "#Cylinder"
                scale: "0.05, 0.1, 0.05"
                z: 5
                eulerRotation.x: 90
                materials: PrincipledMaterial {
                    baseColor: "black"
                }
            }
            xrCursor: cursor
        }
        //! [connections]

        XrCamera {
            id: camera
        }

        Node {
            // Separate node to turn off cursor for gadgets. We can't bind to
            // XrCursor.visible, since AimController assigns to it.
            visible: !gadgetBox.gadgetActive
            XrCursor {
                id: cursor
                cameraNode: camera
                size: 2
                sphere: rightAim.pickStatus === PickResult.Model
            }
        }
    }

    GadgetBox {
        id: gadgetBox
    }

    Scene {}
}
