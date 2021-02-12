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

QQuick3DParticleSystemLogging::QQuick3DParticleSystemLogging(QObject *parent)
    : QObject(parent)
{
}

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

int QQuick3DParticleSystemLogging::updates() const
{
    return m_updates;
}

int QQuick3DParticleSystemLogging::particlesMax() const
{
    return m_particlesMax;
}

int QQuick3DParticleSystemLogging::particlesUsed() const
{
    return m_particlesUsed;
}

float QQuick3DParticleSystemLogging::time() const
{
    return m_time;
}

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
    m_time = 0;
    m_timeAverage = 0;
    m_totalTimesList.clear();
    Q_EMIT updatesChanged();
    Q_EMIT particlesMaxChanged();
    Q_EMIT particlesUsedChanged();
    Q_EMIT timeChanged();
    Q_EMIT timeAverageChanged();
}

QT_END_NAMESPACE
