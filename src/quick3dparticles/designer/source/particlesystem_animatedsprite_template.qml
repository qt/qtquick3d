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
    id: animatedSpriteSystem
    ParticleEmitter3D {
        id: animatedSpriteEmitter
        velocity: animatedSpriteDirection
        particle: animatedSpriteParticle
        lifeSpan: 1000
        emitRate: 1
        SpriteParticle3D {
            id: animatedSpriteParticle
            particleScale: 25
            billboard: true
            sprite: animatedTexture
            spriteSequence: animatedSequence
            maxAmount: 10

            SpriteSequence3D {
                id: animatedSequence
                duration: -1
                interpolate: false
            }

            Texture {
                id: animatedTexture
            }
        }

        VectorDirection3D {
            id: animatedSpriteDirection
        }
    }
}
