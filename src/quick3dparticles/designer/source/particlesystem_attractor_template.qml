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
    id: attractorSystem
    ParticleEmitter3D {
        velocity: attractorDirection
        particle: attractorParticle
        emitRate: 200
        lifeSpan: 2000

        SpriteParticle3D {
            id: attractorParticle
            maxAmount: 1000
        }

        VectorDirection3D {
            id: attractorDirection
            direction.y: 40
            directionVariation.y: 10
            directionVariation.z: 100
            directionVariation.x: 100
        }
    }

    Attractor3D {
        id: particleAttractor
        y: 100
        duration: 1000
        particles: attractorParticle
    }
}
