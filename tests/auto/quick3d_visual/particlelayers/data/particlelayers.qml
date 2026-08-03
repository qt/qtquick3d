// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick3D
import QtQuick3D.Particles3D

Item {
    id: root
    width: 200
    height: 200

    View3D {
        anchors.fill: parent

        PerspectiveCamera { z: 600 }
        DirectionalLight {}

        // A running system assigned to a non-default content layer. The
        // particles it produces should end up on the same layer.
        ParticleSystem3D {
            id: psystem
            objectName: "psystem"
            layers: ContentLayer.Layer2
            // Pre-warm so particles are alive from the first rendered frame.
            startTime: 1000

            SpriteParticle3D {
                id: sprite
                objectName: "sprite"
                color: "red"
                maxAmount: 100
            }

            ParticleEmitter3D {
                particle: sprite
                emitRate: 50
                lifeSpan: 5000
                velocity: VectorDirection3D {
                    direction: Qt.vector3d(0, 100, 0)
                }
            }

            // A compound delegate; the delegate is user content, so the
            // system leaves its layers alone. Each model keeps its own
            // assignment: one explicit, one the default.
            ModelParticle3D {
                id: modelParticle
                objectName: "modelParticle"
                maxAmount: 20
                delegate: Node {
                    Model {
                        source: "#Cube"
                        layers: ContentLayer.Layer5
                        scale: Qt.vector3d(0.1, 0.1, 0.1)
                        materials: PrincipledMaterial { baseColor: "blue" }
                    }
                    Model {
                        source: "#Sphere"
                        // No explicit layers; stays on the default layer.
                        scale: Qt.vector3d(0.1, 0.1, 0.1)
                        materials: PrincipledMaterial { baseColor: "green" }
                    }
                }
            }

            ParticleEmitter3D {
                particle: modelParticle
                emitRate: 10
                lifeSpan: 5000
                velocity: VectorDirection3D {
                    direction: Qt.vector3d(0, 100, 0)
                }
            }
        }
    }
}
