/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
******************************************************************************/

import QtQuick
import QtQuick3D
import QtQuick3D.Particles3D

ParticleSystem3D {
    id: modelBlendSystem
    Component {
        id: modelComponent
        Model {
            id: sphere
            source: "#Sphere"
            materials: defaultMaterial
            DefaultMaterial {
                id: defaultMaterial
                diffuseColor: "#4aee45"
            }
        }
    }

    Node {
        id: translateNode
        x: 150
    }
    ModelBlendParticle3D {
        id: modelBlendParticle
        modelBlendMode: ModelBlendParticle3D.Construct
        endNode: translateNode
        random: true
        delegate: modelComponent
        endTime: 1500
    }
    ParticleEmitter3D {
        id: emitter
        velocity: modelBlendDirection
        particle: modelBlendParticle
        lifeSpan: 4000
        emitRate: modelBlendParticle.maxAmount

        VectorDirection3D {
            id: modelBlendDirection
            directionVariation.z: 50
            directionVariation.x: 50
        }
    }
}
