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

int QQuick3DParticleEmitBurst::time() const
{
    return m_time;
}

int QQuick3DParticleEmitBurst::amount() const
{
    return m_amount;
}

int QQuick3DParticleEmitBurst::duration() const
{
    return m_duration;
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

    m_amount = amount;
    Q_EMIT amountChanged();
}

void QQuick3DParticleEmitBurst::setDuration(int duration)
{
    if (m_duration == duration)
        return;

    m_duration = duration;
    Q_EMIT durationChanged();
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
