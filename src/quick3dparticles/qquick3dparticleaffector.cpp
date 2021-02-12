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

#include "qquick3dparticleaffector_p.h"

QT_BEGIN_NAMESPACE

QQuick3DParticleAffector::QQuick3DParticleAffector(QObject *parent)
    : QObject(parent)
{
}

QQuick3DParticleAffector::~QQuick3DParticleAffector()
{
    for (const auto &connection : qAsConst(m_connections))
        QObject::disconnect(connection);
    if (m_system)
        m_system->unRegisterParticleAffector(this);
}

QQuick3DParticleSystem* QQuick3DParticleAffector::system() const
{
    return m_system;
}

void QQuick3DParticleAffector::setSystem(QQuick3DParticleSystem* system)
{
    if (m_system == system)
        return;

    if (m_system)
        m_system->unRegisterParticleAffector(this);

    m_system = system;
    if (m_system)
        m_system->registerParticleAffector(this);

    Q_EMIT systemChanged();
    update();
}

int QQuick3DParticleAffector::startTime() const
{
    return m_startTime;
}

void QQuick3DParticleAffector::setStartTime(int startTime)
{
    if (m_startTime == startTime)
        return;

    m_startTime = startTime;
    Q_EMIT startTimeChanged();

    // endTime must always be >= starttime
    if (m_startTime > m_endTime)
        setEndTime(m_startTime);
}

int QQuick3DParticleAffector::endTime() const
{
    return m_endTime;
}

void QQuick3DParticleAffector::setEndTime(int endTime)
{
    if (m_endTime == endTime)
        return;

    m_endTime = endTime;
    Q_EMIT endTimeChanged();

    // endTime must always be >= starttime
    if (m_endTime < m_startTime)
        setStartTime(m_endTime);
}

void QQuick3DParticleAffector::componentComplete()
{
    if (!m_system && qobject_cast<QQuick3DParticleSystem*>(parent()))
        setSystem(qobject_cast<QQuick3DParticleSystem*>(parent()));
}

// Returns true if particle is currently alive
bool QQuick3DParticleAffector::shouldAffect(const QQuick3DParticleData &sd, float time)
{
    Q_UNUSED(sd);
    if (time < 0.0f)
        return false;

    int timeMs = time * 1000.0;

    if (m_startTime > timeMs)
        return false;

    return true;
}

// Returns current particle time local to this affector
// If affector endTime has been reached, return time at the end
float QQuick3DParticleAffector::particleTime(float time)
{
    float at = time - (m_startTime / 1000.0);
    at = std::min(at, float((m_endTime - m_startTime) / 1000.0));
    return at;
}

// Particles

QQmlListProperty<QQuick3DParticle> QQuick3DParticleAffector::particles()
{
    return {this, this,
             &QQuick3DParticleAffector::appendParticle,
             &QQuick3DParticleAffector::particleCount,
             &QQuick3DParticleAffector::particle,
             &QQuick3DParticleAffector::clearParticles,
             &QQuick3DParticleAffector::replaceParticle,
             &QQuick3DParticleAffector::removeLastParticle};
}

void QQuick3DParticleAffector::appendParticle(QQuick3DParticle *n) {
    m_particles.append(n);
    m_connections.insert(n, QObject::connect(n, &QObject::destroyed, [this](QObject *obj) {
        QQuick3DParticle *particle = qobject_cast<QQuick3DParticle *>(obj);
        m_particles.removeAll(particle);
        QObject::disconnect(m_connections[particle]);
        m_connections.remove(particle);
    }));
}

qsizetype QQuick3DParticleAffector::particleCount() const
{
    return m_particles.count();
}

QQuick3DParticle *QQuick3DParticleAffector::particle(qsizetype index) const
{
    return m_particles.at(index);
}

void QQuick3DParticleAffector::clearParticles() {
    m_particles.clear();
}

void QQuick3DParticleAffector::replaceParticle(qsizetype index, QQuick3DParticle *n)
{
    QQuick3DParticle *remove = m_particles[index];
    QObject::disconnect(m_connections[remove]);
    m_connections.remove(remove);
    m_particles[index] = n;
    m_connections.insert(n, QObject::connect(n, &QObject::destroyed, [this](QObject *obj) {
        QQuick3DParticle *particle = qobject_cast<QQuick3DParticle *>(obj);
        m_particles.removeAll(particle);
        QObject::disconnect(m_connections[particle]);
        m_connections.remove(particle);
    }));
}

void QQuick3DParticleAffector::removeLastParticle()
{
    QQuick3DParticle *last = m_particles.last();
    QObject::disconnect(m_connections[last]);
    m_connections.remove(last);
    m_particles.removeLast();
}

// Particles - static
void QQuick3DParticleAffector::appendParticle(QQmlListProperty<QQuick3DParticle> *list, QQuick3DParticle *p) {
    reinterpret_cast<QQuick3DParticleAffector *>(list->data)->appendParticle(p);
}

void QQuick3DParticleAffector::clearParticles(QQmlListProperty<QQuick3DParticle> *list) {
    reinterpret_cast<QQuick3DParticleAffector *>(list->data)->clearParticles();
}

void QQuick3DParticleAffector::replaceParticle(QQmlListProperty<QQuick3DParticle> *list, qsizetype i, QQuick3DParticle *p)
{
    reinterpret_cast<QQuick3DParticleAffector *>(list->data)->replaceParticle(i, p);
}

void QQuick3DParticleAffector::removeLastParticle(QQmlListProperty<QQuick3DParticle> *list)
{
    reinterpret_cast<QQuick3DParticleAffector *>(list->data)->removeLastParticle();
}

QQuick3DParticle *QQuick3DParticleAffector::particle(QQmlListProperty<QQuick3DParticle> *list, qsizetype i) {
    return reinterpret_cast<QQuick3DParticleAffector *>(list->data)->particle(i);
}

qsizetype QQuick3DParticleAffector::particleCount(QQmlListProperty<QQuick3DParticle> *list) {
    return reinterpret_cast<QQuick3DParticleAffector *>(list->data)->particleCount();
}

QT_END_NAMESPACE
