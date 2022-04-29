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
    id: modelShapeSystem
    ParticleEmitter3D {
        id: modelShapeEmitter
        shape: targetShape
        velocity: modelShapeDirection
        emitRate: 100
        lifeSpanVariation: 100
        lifeSpan: 4000
        particle: modelShapeParticle
        particleRotationVelocityVariation.x: 200
        particleRotationVariation.z: 180
        particleRotationVelocityVariation.y: 200

        SpriteParticle3D {
            id: modelShapeParticle
            color: "#ffffff"
            fadeInDuration: 1500
            fadeOutDuration: 1500
            particleScale: 2
            maxAmount: 2000

            VectorDirection3D {
                id: modelShapeDirection
                directionVariation.z: 2
                direction.y: 2
                directionVariation.x: 2
                direction.z: 0
                directionVariation.y: 2
            }
        }
        particleRotationVelocityVariation.z: 200
        particleEndScale: 1.5
        particleRotationVariation.y: 180
        particleRotationVariation.x: 180
    }
    ParticleModelShape3D {
        id: targetShape
        fill: false
        delegate: Model {
            source: "#Cube"
        }
    }
}
