/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquick3dparticleattractor_p.h"
#include "qquick3dparticlerandomizer_p.h"

QT_BEGIN_NAMESPACE

// Minimum duration in seconds
const float MIN_DURATION = 0.001f;

QQuick3DParticleAttractor::QQuick3DParticleAttractor(QObject *parent)
    : QQuick3DParticleAffector(parent)
    , m_position(0, 0, 0)
    , m_duration(-1)
    , m_durationVariation(0)
{
}

QVector3D QQuick3DParticleAttractor::position() const
{
    return m_position;
}

void QQuick3DParticleAttractor::setPosition(const QVector3D &position)
{
    if (m_position == position)
        return;

    m_position = position;
    Q_EMIT positionChanged();
    update();
}

QVector3D QQuick3DParticleAttractor::positionVariation() const
{
    return m_positionVariation;
}

void QQuick3DParticleAttractor::setPositionVariation(const QVector3D &positionVariation)
{
    if (m_positionVariation == positionVariation)
        return;

    m_positionVariation = positionVariation;
    Q_EMIT positionVariationChanged();
    update();
}

QQuick3DParticleShapeNode *QQuick3DParticleAttractor::shapeNode() const
{
    return m_shapeNode;
}

void QQuick3DParticleAttractor::setShapeNode(QQuick3DParticleShapeNode *shapeNode)
{
    if (m_shapeNode == shapeNode)
        return;

    m_shapeNode = shapeNode;
    m_shapeDirty = true;
    Q_EMIT shapeNodeChanged();
    update();
}

int QQuick3DParticleAttractor::duration() const
{
    return m_duration;
}

void QQuick3DParticleAttractor::setDuration(int duration)
{
    if (m_duration == duration)
        return;

    m_duration = duration;
    Q_EMIT durationChanged();
    update();
}

int QQuick3DParticleAttractor::durationVariation() const
{
    return m_durationVariation;
}

void QQuick3DParticleAttractor::setDurationVariation(int durationVariation)
{
    if (m_durationVariation == durationVariation)
        return;

    m_durationVariation = durationVariation;
    Q_EMIT durationVariationChanged();
    update();
}

bool QQuick3DParticleAttractor::hideAtEnd() const
{
    return m_hideAtEnd;
}

void QQuick3DParticleAttractor::setHideAtEnd(bool hideAtEnd)
{
    if (m_hideAtEnd == hideAtEnd)
        return;

    m_hideAtEnd = hideAtEnd;
    Q_EMIT hideAtEndChanged();
    update();
}

void QQuick3DParticleAttractor::updateShapePositions()
{
    m_shapePositionList.clear();
    if (!m_shapeNode || !m_shapeNode->shape())
        return;

    // Get count of particles positions needed
    int pCount = 0;
    if (!m_particles.isEmpty()) {
        for (auto p : m_particles) {
            auto pp = qobject_cast<QQuick3DParticle *>(p);
            pCount += pp->maxAmount();
        }
    } else {
        pCount = m_system->particleCount();
    }

    m_shapePositionList.reserve(pCount);
    for (int i = 0; i < pCount; i++)
        m_shapePositionList << m_shapeNode->shape()->randomPosition(i);

    m_shapeDirty = false;
}

void QQuick3DParticleAttractor::affectParticle(const QQuick3DParticleData &sd, QQuick3DParticleDataCurrent *d, float time)
{
    if (m_shapeDirty)
        updateShapePositions();

    float particleTimeS = time - sd.startTime;
    if (shouldAffect(sd, particleTimeS)) {
        float at = particleTime(particleTimeS);

        float duration = m_duration < 0 ? sd.lifetime : (m_duration / 1000.0);
        float durationVariation = m_durationVariation == 0
                ? 0.0f
                : (m_durationVariation / 1000.0) - 2.0f * QPRand::get(sd.index, QPRand::AttractorDurationV) * (m_durationVariation / 1000.0);
        duration = std::max(duration + durationVariation, MIN_DURATION);
        float pEnd = std::min(1.0f, std::max(0.0f, at / duration));
        // TODO: Should we support easing?
        //pEnd = easeInOutQuad(pEnd);

        if (m_hideAtEnd && pEnd >= 1.0f)
            d->color.a = 0;

        float pStart = 1.0f - pEnd;
        if (m_shapeNode) {
            d->position = (pStart * d->position) + (pEnd * (m_shapeNode->scenePosition() + m_shapePositionList[sd.index]));
        } else {
            if (m_positionVariation.isNull()) {
                d->position =  (pStart * d->position) + (pEnd * m_position);
            } else {
                QVector3D pos = m_position;
                if (!m_positionVariation.isNull()) {
                    pos.setX(pos.x() + m_positionVariation.x() - 2.0f * QPRand::get(sd.index, QPRand::AttractorPosVX) * m_positionVariation.x());
                    pos.setY(pos.y() + m_positionVariation.y() - 2.0f * QPRand::get(sd.index, QPRand::AttractorPosVY) * m_positionVariation.y());
                    pos.setZ(pos.z() + m_positionVariation.z() - 2.0f * QPRand::get(sd.index, QPRand::AttractorPosVZ) * m_positionVariation.z());
                }
                d->position =  (pStart * d->position) + (pEnd * pos);
            }
        }
    }
}

QT_END_NAMESPACE
