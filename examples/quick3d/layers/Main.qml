// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick3D
import QtQuick3D.Helpers
import QtQuick3D.Particles3D

ApplicationWindow {
    id: window
    width: 1280
    height: 720
    visible: true
    title: qsTr("Layers Example")

    property bool isLandscape: width > height

    View3D {
        id: v3d

        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.left: window.isLandscape ? settingsDrawer.right : parent.left
        anchors.top: window.isLandscape ? parent.top : settingsDrawer.bottom

        environment: SceneEnvironment {
            antialiasingMode: SceneEnvironment.MSAA
            antialiasingQuality: SceneEnvironment.High
        }

        //! [content layers]
        readonly property int redModels: ContentLayer.Layer1
        readonly property int greenModels: ContentLayer.Layer2
        readonly property int blueModels: ContentLayer.Layer3
        //! [content layers]

        //! [camera filter]
        property int cameraFilter: ContentLayer.LayerAll

        camera: PerspectiveCamera {
            layers: v3d.cameraFilter
            position: Qt.vector3d(0, 400, 500)
            eulerRotation.x: -30
            clipFar: 2000
        }
        //! [camera filter]

        DirectionalLight {
            ambientColor: Qt.rgba(0.2, 0.2, 0.2, 1)
        }

        //! [red models]
        Node {
            position: Qt.vector3d(0, 200, 0)
            Model {
                source: "qtlogo.mesh"
                layers: v3d.redModels
                scale: Qt.vector3d(5000, 5000, 5000)
                materials:[ PrincipledMaterial {
                    baseColor: "red"
                    roughness: 0.5
                } ]

                NumberAnimation on eulerRotation.y {
                    from: 0
                    to: 360
                    duration: 15000
                    loops: Animation.Infinite
                }
            }

            Model {
                position: Qt.vector3d(0, -50, 0)
                source: "#Cylinder"
                materials:[ PrincipledMaterial {
                    baseColor: "red"
                    roughness: 0.5
                } ]
            }

            ParticleSystem3D {
                visible: particlesCheckbox.checked
                running: particlesCheckbox.checked
                layers: v3d.redModels
                SpriteParticle3D {
                    id: redParticle
                    color: "red"
                    maxAmount: 200
                    particleScale: 8
                    fadeInDuration: 500
                    fadeOutDuration: 500
                }
                ParticleEmitter3D {
                    particle: redParticle
                    position: Qt.vector3d(0, -50, 0)
                    emitRate: 40
                    lifeSpan: 2000
                    velocity: VectorDirection3D {
                        direction: Qt.vector3d(0, 120, 0)
                        directionVariation: Qt.vector3d(40, 40, 40)
                    }
                }
            }
        }

        //! [red models]

        //! [green models]
        Node {
            position: Qt.vector3d(0, 100, 0)
            Model {
                source: "qtlogo.mesh"
                layers: v3d.greenModels
                scale: Qt.vector3d(5000, 5000, 5000)
                materials:[ PrincipledMaterial {
                    baseColor: "green"
                    roughness: 0.5
                } ]

                NumberAnimation on eulerRotation.y {
                    from: 0
                    to: 360
                    duration: 20000
                    loops: Animation.Infinite
                }
            }
            Model {
                position: Qt.vector3d(0, -50, 0)
                source: "#Cylinder"
                materials:[ PrincipledMaterial {
                    baseColor: "green"
                    roughness: 0.5
                } ]
            }

            ParticleSystem3D {
                visible: particlesCheckbox.checked
                running: particlesCheckbox.checked
                layers: v3d.greenModels
                SpriteParticle3D {
                    id: greenParticle
                    color: "green"
                    maxAmount: 200
                    particleScale: 8
                    fadeInDuration: 500
                    fadeOutDuration: 500
                }
                ParticleEmitter3D {
                    particle: greenParticle
                    position: Qt.vector3d(0, -50, 0)
                    emitRate: 40
                    lifeSpan: 2000
                    velocity: VectorDirection3D {
                        direction: Qt.vector3d(0, 120, 0)
                        directionVariation: Qt.vector3d(40, 40, 40)
                    }
                }
            }
        }
        //! [green models]

        //! [blue models]
        Node {
            Model {
                source: "qtlogo.mesh"
                layers: v3d.blueModels
                scale: Qt.vector3d(5000, 5000, 5000)
                materials:[ PrincipledMaterial {
                    baseColor: "blue"
                    roughness: 0.5
                } ]

                NumberAnimation on eulerRotation.y {
                    from: 0
                    to: 360
                    duration: 12500
                    loops: Animation.Infinite
                }
            }
            Model {
                position: Qt.vector3d(0, -50, 0)
                source: "#Cylinder"
                materials:[ PrincipledMaterial {
                    baseColor: "blue"
                    roughness: 0.5
                } ]
            }

            ParticleSystem3D {
                visible: particlesCheckbox.checked
                running: particlesCheckbox.checked
                layers: v3d.blueModels
                SpriteParticle3D {
                    id: blueParticle
                    color: "blue"
                    maxAmount: 200
                    particleScale: 8
                    fadeInDuration: 500
                    fadeOutDuration: 500
                }
                ParticleEmitter3D {
                    particle: blueParticle
                    position: Qt.vector3d(0, -50, 0)
                    emitRate: 40
                    lifeSpan: 2000
                    velocity: VectorDirection3D {
                        direction: Qt.vector3d(0, 120, 0)
                        directionVariation: Qt.vector3d(40, 40, 40)
                    }
                }
            }
        }
        //! [blue models]

        // A model particle with a compound delegate; each model in the
        // delegate sub-tree assigns its own content layer.
        ParticleSystem3D {
            visible: particlesCheckbox.checked
            running: particlesCheckbox.checked

            ModelParticle3D {
                id: multiLayerParticle
                maxAmount: 30
                delegate: Node {
                    Model {
                        source: "#Cube"
                        layers: v3d.redModels
                        position: Qt.vector3d(-40, 0, 0)
                        scale: Qt.vector3d(0.25, 0.25, 0.25)
                        materials: [ PrincipledMaterial {
                            baseColor: "red"
                            roughness: 0.5
                        } ]
                    }
                    Model {
                        source: "#Sphere"
                        layers: v3d.greenModels
                        scale: Qt.vector3d(0.25, 0.25, 0.25)
                        materials: [ PrincipledMaterial {
                            baseColor: "green"
                            roughness: 0.5
                        } ]
                    }
                    Model {
                        source: "#Cone"
                        layers: v3d.blueModels
                        position: Qt.vector3d(40, 0, 0)
                        scale: Qt.vector3d(0.25, 0.25, 0.25)
                        materials: [ PrincipledMaterial {
                            baseColor: "blue"
                            roughness: 0.5
                        } ]
                    }
                }
            }

            ParticleEmitter3D {
                particle: multiLayerParticle
                position: Qt.vector3d(0, -50, 0)
                emitRate: 4
                lifeSpan: 5000
                velocity: VectorDirection3D {
                    direction: Qt.vector3d(0, 80, 0)
                    directionVariation: Qt.vector3d(20, 20, 20)
                }
            }
        }

        DebugView {
            id: dbg
            anchors.top: parent.top
            anchors.right: parent.right
            source: v3d
            visible: debugViewCheckbox.checked
        }
    }

    RowLayout {
        anchors.bottom: v3d.bottom
        anchors.left: v3d.left
        anchors.margins: 10
        Item {
            id: settingsButton
            implicitWidth: 64
            implicitHeight: 64
            Image {
                anchors.centerIn: parent
                source: "icon_settings.png"
            }

            HoverHandler {
                id: hoverHandler
            }
        }
        Text {
            Layout.alignment: Qt.AlignVCenter
            text: settingsDrawer.title
            visible: !settingsDrawer.isOpen
            color: "white"
            font.pointSize: 16
        }
        TapHandler {
            // qmllint disable signal-handler-parameters
            onTapped: settingsDrawer.isOpen = !settingsDrawer.isOpen;
            // qmllint enable signal-handler-parameters
        }
    }

    SettingsDrawer {
        id: settingsDrawer
        title: qsTr("Settings")
        isLandscape: window.isLandscape
        width: window.isLandscape ? implicitWidth : window.width
        height: window.isLandscape ? window.height : window.height * 0.33

        //! [checkbox red layer]
        CheckBox {
            id: checkBoxRedLayer
            text: qsTr("Red Layer")
            checked: true
            onCheckedChanged: {
                v3d.cameraFilter = checked ? v3d.cameraFilter | v3d.redModels : v3d.cameraFilter & ~v3d.redModels;
            }
        }
        //! [checkbox red layer]
        CheckBox {
            id: checkBoxGreenLayer
            text: qsTr("Green Layer")
            checked: true
            onCheckedChanged: {
                v3d.cameraFilter = checked ? v3d.cameraFilter | v3d.greenModels : v3d.cameraFilter & ~v3d.greenModels;
            }
        }
        CheckBox {
            id: checkBoxBlueLayer
            text: qsTr("Blue Layer")
            checked: true
            onCheckedChanged: {
                v3d.cameraFilter = checked ? v3d.cameraFilter | v3d.blueModels : v3d.cameraFilter & ~v3d.blueModels;
            }
        }

        CheckBox {
            id: particlesCheckbox
            text: qsTr("Enable Particles")
            checked: false
        }

        Label {
            // spacer
        }

        CheckBox {
            id: debugViewCheckbox
            text: qsTr("Enable Debug View")
            checked: false
        }
    }
}
