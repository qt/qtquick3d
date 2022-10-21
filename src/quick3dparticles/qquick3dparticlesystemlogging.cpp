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

#include "qquick3dparticlesystemlogging_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype ParticleSystem3DLogging
    \inherits QtObject
    \inqmlmodule QtQuick3D.Particles3D
    \brief Provides information of the particle system.
    \since 6.2

    The \c ParticleSystem3DLogging type provides information about particle system statistics.
    This element cannot be created directly, but can be retrieved from a \l ParticleSystem3D.
*/

QQuick3DParticleSystemLogging::QQuick3DParticleSystemLogging(QObject *parent)
    : QObject(parent)
{
}

/*!
    \qmlproperty int ParticleSystem3DLogging::loggingInterval

    This property defines in milliseconds how often the logging data is updated.
    Longer update time increases the accuracy of \l {ParticleSystem3DLogging::time}{time} and
    \l {ParticleSystem3DLogging::timeAverage}{timeAverage}, while shorter update times keep
    the data more up to date.

    The default value is \c 1000.
*/

int QQuick3DParticleSystemLogging::loggingInterval() const
{
    return m_loggingInterval;
}

void QQuick3DParticleSystemLogging::setLoggingInterval(int interval)
{
    if (m_loggingInterval == interval)
        return;

    m_loggingInterval = interval;
    Q_EMIT loggingIntervalChanged();
}

/*!
    \qmlproperty int ParticleSystem3DLogging::updates
    \readonly

    This property holds the amount of particle system updates since the last logging.
    When \a loggingInterval is 1000 (default), this can be considered to match the fps.
*/
int QQuick3DParticleSystemLogging::updates() const
{
    return m_updates;
}

/*!
    \qmlproperty int ParticleSystem3DLogging::particlesMax
    \readonly

    This property holds the maximum amount of particles in this system.
    Maximum amount is the sum of system particles \l {Particle3D::maxAmount}{maxAmount} properties.
*/
int QQuick3DParticleSystemLogging::particlesMax() const
{
    return m_particlesMax;
}

/*!
    \qmlproperty int ParticleSystem3DLogging::particlesUsed
    \readonly

    This property holds the amount of particles currently in use in this system.
    This value should be close to \l particlesMax at some point of particle system
    animation. If it is much smaller, consider decreasing \l {Particle3D::maxAmount}{maxAmount} values.
    If it reaches \l particlesMax, particles are used effectively but it can also mean that
    particles are reused before they reach the end of their \l {ParticleEmitter3D::lifeSpan}{lifeSpan}.
    In this case, consider increasing the \l {Particle3D::maxAmount}{maxAmount} values.
*/
int QQuick3DParticleSystemLogging::particlesUsed() const
{
    return m_particlesUsed;
}

/*!
    \qmlproperty real ParticleSystem3DLogging::time
    \readonly

    This property holds the time in milliseconds used for emitting and animating particles
    in each frame.
*/
float QQuick3DParticleSystemLogging::time() const
{
    return m_time;
}

/*!
    \qmlproperty real ParticleSystem3DLogging::timeAverage
    \readonly

    This property holds the average time in milliseconds used for emitting and animating
    particles in each frame. Average is calculated from the past 100 logging updates.
    So when \l loggingInterval is 1000, this represents an average \l time in past 100 seconds.
    This can be used for measuring the performance of current particle system.
*/
float QQuick3DParticleSystemLogging::timeAverage() const
{
    return m_timeAverage;
}

void QQuick3DParticleSystemLogging::updateTimes(qint64 time)
{
    m_time = float(time / 1000000.0) / m_updates;

    // Keep max amount of times stored
    m_totalTimesList.append(m_time);
    const int MAX_TIMES = 100;
    if (m_totalTimesList.size() > MAX_TIMES)
        m_totalTimesList.removeFirst();

    // Calculate average from stored times
    double totalTime = 0;
    for (auto time : m_totalTimesList)
        totalTime += time;
    m_timeAverage = float(totalTime / m_totalTimesList.size());

    Q_EMIT timeChanged();
    Q_EMIT timeAverageChanged();
}

void QQuick3DParticleSystemLogging::resetData()
{
    m_updates = 0;
    m_particlesMax = 0;
    m_particlesUsed = 0;
    m_time = 0.0f;
    m_timeAverage = 0.0f;
    m_totalTimesList.clear();
    Q_EMIT updatesChanged();
    Q_EMIT particlesMaxChanged();
    Q_EMIT particlesUsedChanged();
    Q_EMIT timeChanged();
    Q_EMIT timeAverageChanged();
}

QT_END_NAMESPACE
