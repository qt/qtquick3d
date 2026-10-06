// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers

Item {
    id: root
    width: 640
    height: 480
    visible: true

    // Drives the Loader below. Toggled from the test to destroy and recreate
    // the importing View3D.
    property bool loadView: true

    // The shared content is rendered through the depth pre-pass, the shadow
    // map, the normal pass and a user pass. Each of these builds its srb
    // around the uniform buffer of its draw call data, so the srb must go
    // away together with the draw call data when the view is destroyed.
    Node {
        id: sharedScene

        PerspectiveCamera {
            z: 300
        }

        DirectionalLight {
            castsShadow: true
            eulerRotation.x: -45
        }

        Model {
            source: "#Cube"
            castsShadows: true
            receivesShadows: true
            materials: PrincipledMaterial { baseColor: "white" }
        }

        Model {
            source: "#Sphere"
            x: 100
            castsShadows: true
            materials: CustomMaterial {
                shadingMode: CustomMaterial.Unshaded
                fragmentShader: "normalroughness.frag"
            }
        }

        Model {
            source: "#Rectangle"
            y: -80
            scale: Qt.vector3d(5, 5, 5)
            eulerRotation.x: -90
            receivesShadows: true
            materials: PrincipledMaterial { baseColor: "gray" }
        }
    }

    Loader {
        id: viewLoader
        anchors.fill: parent
        sourceComponent: root.loadView ? viewComponent : null
    }

    Component {
        id: viewComponent
        View3D {
            anchors.fill: parent
            importScene: sharedScene

            environment: SceneEnvironment {
                backgroundMode: SceneEnvironment.Color
                clearColor: "black"
                depthPrePassEnabled: true
            }

            RenderPassTexture {
                id: userPassTexture
                format: RenderPassTexture.RGBA8
            }

            RenderPass {
                id: userPass
                materialMode: RenderPass.OriginalMaterial
                commands: [
                    ColorAttachment { target: userPassTexture },
                    DepthStencilAttachment {}
                ]
            }

            // Something has to consume the pass output for the pass to run.
            Model {
                source: "#Cube"
                x: -100
                materials: PrincipledMaterial {
                    baseColorMap: Texture {
                        textureProvider: RenderOutputProvider {
                            textureSource: RenderOutputProvider.UserPassTexture
                            renderPass: userPass
                        }
                    }
                }
            }
        }
    }
}
