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

#include "qquick3dparticleemitburst_p.h"
#include "qquick3dparticleemitter_p.h"
#include <qdebug.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype EmitBurst3D
    \inherits QtObject
    \inqmlmodule QtQuick3D.Particles3D
    \brief Declarative emitter bursts.
    \since 6.2

    This element defines particle bursts in the \l ParticleEmitter3D.

    For example, to emit 100 particles at the beginning, and 50 particles at 2 seconds,
    so that both bursts take 200 milliseconds:

    \qml
    ParticleEmitter3D {
        ...
        emitBursts: [
            EmitBurst3D {
                time: 0
                amount: 100
                duration: 200
            },
            EmitBurst3D {
                time: 2000
                amount: 50
                duration: 200
            }
        ]
    }
    \endqml
*/

QQuick3DParticleEmitBurst::QQuick3DParticleEmitBurst(QObject *parent)
    : QObject(parent)
{
    m_parentEmitter = qobject_cast<QQuick3DParticleEmitter *>(parent);
}

QQuick3DParticleEmitBurst::~QQuick3DParticleEmitBurst()
{
    if (m_parentEmitter)
        m_parentEmitter->unRegisterEmitBurst(this);
}

/*!
    \qmlproperty int EmitBurst3D::time

    This property defines the time in milliseconds when emitting the burst starts.

    The default value is \c 0.
*/
int QQuick3DParticleEmitBurst::time() const
{
    return m_time;
}

/*!
    \qmlproperty int EmitBurst3D::amount

    This property defines the amount of particles emitted during the burst.

    The default value is \c 0.
*/
int QQuick3DParticleEmitBurst::amount() const
{
    return m_amount;
}

/*!
    \qmlproperty int EmitBurst3D::duration

    This property defines the duration of the burst. The default value is 0,
    meaning all particles will burst at the beginning of \l time.
    If the duration is set, particles emitting is distributed between \c time
    and \c time + \c duration.

    For example, to have emit rate of 400 between 1000 and 1200 milliseconds:
    \qml
    EmitBurst3D {
        time: 1000
        amount: 80
        duration: 1200
    }
    \endqml

    The default value is \c 0.
*/
int QQuick3DParticleEmitBurst::duration() const
{
    return m_duration;
}

/*!
    \qmlproperty bool EmitBurst3D::repeatDelay
    \since 6.3

    This property defines the delay between repeating bursts.

    For example, to have the burst emit 5 particles every 1000 milliseconds
    \qml
    EmitBurst3D {
        amount: 5
        repeat: true
        repeatDelay: 1000
    }
    \endqml

    The default value is \c false.
*/
int QQuick3DParticleEmitBurst::repeatDelay() const
{
    return m_repeatDelay;
}

/*!
    \qmlproperty bool EmitBurst3D::repeat
    \since 6.3

    This property defines if the burst will keep repeating after its initial burst.

    The default value is \c false.
*/
bool QQuick3DParticleEmitBurst::repeat() const
{
    return m_repeat;
}

/*!
    \qmlproperty int EmitBurst3D::amountVariation
    \since 6.3

    This property defines the random variation in particle emit amount.

    For example, to have a random range between 0 and 10
    \qml
    EmitBurst3D {
        time: 1000
        amount: 5
        amountVariation: 5
    }
    \endqml

    The default value is \c 0.
*/
int QQuick3DParticleEmitBurst::amountVariation() const
{
    return m_amountVariation;
}

/*!
    \qmlproperty bool EmitBurst3D::endTrigger
    \since 6.3

    This property defines if the burst should be emitted when the followed particle is destroyed.
    \note This property is restricted to only work with trail emitters.

    The default value is \c false.
*/
bool QQuick3DParticleEmitBurst::endTrigger() const
{
    return m_triggerType & TriggerEnd;
}

/*!
    \qmlproperty bool EmitBurst3D::startTrigger
    \since 6.3

    This property defines if the burst should be emitted when the followed particle is created.
    \note This property is restricted to only work with trail emitters.

    The default value is \c false.
*/
bool QQuick3DParticleEmitBurst::startTrigger() const
{
    return m_triggerType & TriggerStart;
}

/*!
    \qmlproperty bool EmitBurst3D::triggerOnly
    \since 6.3

    This property defines if the burst should only be triggered by creation or deletion.
    \note This property is restricted to only work with trail emitters.

    The default value is \c false.
*/
bool QQuick3DParticleEmitBurst::triggerOnly() const
{
    return !(m_triggerType & TriggerEmit);
}

void QQuick3DParticleEmitBurst::setTime(int time)
{
    if (m_time == time)
        return;

    m_time = time;
    Q_EMIT timeChanged();
}

void QQuick3DParticleEmitBurst::setAmount(int amount)
{
    if (m_amount == amount)
        return;
    if (amount < 0) {
        qWarning () << "EmitBurst3D: Amount must be positive.";
        return;
    }
    m_amount = amount;
    Q_EMIT amountChanged();
}

void QQuick3DParticleEmitBurst::setDuration(int duration)
{
    if (m_duration == duration)
        return;
    if (duration < 0) {
        qWarning () << "EmitBurst3D: Duration must be positive.";
        return;
    }
    m_duration = duration;
    Q_EMIT durationChanged();
}

void QQuick3DParticleEmitBurst::setRepeatDelay(int delay)
{
    if (m_repeatDelay == delay)
        return;

    if (delay < 0) {
        qWarning () << "EmitBurst3D: Repeat delay must be positive.";
        return;
    }
    m_repeatDelay = delay;
    Q_EMIT repeatDelayChanged();
}

void QQuick3DParticleEmitBurst::setRepeat(bool repeat)
{
    if (m_repeat == repeat)
        return;

    m_repeat = repeat;
    Q_EMIT repeatChanged();
}

void QQuick3DParticleEmitBurst::setAmountVariation(int value)
{
    if (m_amountVariation == value)
        return;

    if (value < 0) {
        qWarning () << "EmitBurst3D: Amount variation must be positive.";
        return;
    }
    m_amountVariation = value;
    Q_EMIT amountVariationChanged();
}

void QQuick3DParticleEmitBurst::setTriggerOnly(bool value)
{
    if (triggerOnly() == value)
        return;

    m_triggerType = (TriggerType)(m_triggerType ^ TriggerEmit);
    Q_EMIT triggerOnlyChanged();
}

void QQuick3DParticleEmitBurst::setEndTrigger(bool value)
{
    if (endTrigger() == value)
        return;

    m_triggerType = (TriggerType)(m_triggerType ^ TriggerEnd);
    Q_EMIT endTriggerChanged();
}

void QQuick3DParticleEmitBurst::setStartTrigger(bool value)
{
    if (startTrigger() == value)
        return;

    m_triggerType = (TriggerType)(m_triggerType ^ TriggerStart);
    Q_EMIT startTriggerChanged();
}

void QQuick3DParticleEmitBurst::componentComplete()
{
    m_parentEmitter = qobject_cast<QQuick3DParticleEmitter *>(parent());
    if (m_parentEmitter)
        m_parentEmitter->registerEmitBurst(this);
    else
        qWarning() << "EmitBurst requires parent Emitter to function correctly!";
}

QT_END_NAMESPACE
