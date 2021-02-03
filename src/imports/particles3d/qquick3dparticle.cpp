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

#include "qquick3dparticle_p.h"

QT_BEGIN_NAMESPACE

QQuick3DParticle::QQuick3DParticle(QQuick3DObject *parent)
    : QQuick3DObject(parent)
    , m_color(255, 255, 255, 255)
    , m_colorVariation(0, 0, 0, 0)
{
}

QQuick3DParticle::~QQuick3DParticle()
{
    if (m_system)
        m_system->unRegisterParticle(this);
}

QQuick3DParticleSystem* QQuick3DParticle::system() const
{
    return m_system;
}

void QQuick3DParticle::setSystem(QQuick3DParticleSystem* system)
{
    if (m_system == system)
        return;

    if (m_system)
        m_system->unRegisterParticle(this);

    m_system = system;
    if (m_system)
        m_system->registerParticle(this);
    Q_EMIT systemChanged();
}

int QQuick3DParticle::maxAmount() const
{
    return m_maxAmount;
}

void QQuick3DParticle::setMaxAmount(int maxAmount)
{
    if (m_maxAmount == maxAmount)
        return;

    m_maxAmount = maxAmount;
    Q_EMIT maxAmountChanged();
}

QColor QQuick3DParticle::color() const
{
    return m_color;
}

float QQuick3DParticle::opacity() const
{
    return m_color.alphaF();
}


void QQuick3DParticle::setColor(QColor color)
{
    if (m_color == color)
        return;

    m_color = color;
    Q_EMIT colorChanged();
}

// When setting color to undefined, reset particle
// to use its own color instead
void QQuick3DParticle::resetColor()
{
    m_color = QColor(255, 255, 255, 255);
    m_colorVariation = QVector4D(0, 0, 0, 0);
}

QVector4D QQuick3DParticle::colorVariation() const
{
    return m_colorVariation;
}

void QQuick3DParticle::setColorVariation(QVector4D colorVariation)
{
    if (m_colorVariation == colorVariation)
        return;

    m_colorVariation = colorVariation;
    Q_EMIT colorVariationChanged();
}

QQuick3DParticle::FadeType QQuick3DParticle::fadeInEffect() const
{
    return m_fadeInEffect;
}

void QQuick3DParticle::setFadeInEffect(FadeType fadeInEffect)
{
    if (m_fadeInEffect == fadeInEffect)
        return;

    m_fadeInEffect = fadeInEffect;
    Q_EMIT fadeInEffectChanged();
}

QQuick3DParticle::FadeType QQuick3DParticle::fadeOutEffect() const
{
    return m_fadeOutEffect;
}

void QQuick3DParticle::setFadeOutEffect(FadeType fadeOutEffect)
{
    if (m_fadeOutEffect == fadeOutEffect)
        return;

    m_fadeOutEffect = fadeOutEffect;
    Q_EMIT fadeOutEffectChanged();
}

int QQuick3DParticle::fadeInDuration() const
{
    return m_fadeInDuration;
}

void QQuick3DParticle::setFadeInDuration(int fadeInDuration)
{
    if (m_fadeInDuration == fadeInDuration)
        return;

    m_fadeInDuration = fadeInDuration;
    Q_EMIT fadeInDurationChanged();
}

int QQuick3DParticle::fadeOutDuration() const
{
    return m_fadeOutDuration;
}

void QQuick3DParticle::setFadeOutDuration(int fadeOutDuration)
{
    if (m_fadeOutDuration == fadeOutDuration)
        return;

    m_fadeOutDuration = fadeOutDuration;
    Q_EMIT fadeOutDurationChanged();
}

QQuick3DParticle::AlignMode QQuick3DParticle::alignMode() const
{
    return m_alignMode;
}

QQuick3DNode *QQuick3DParticle::target() const
{
    return m_target;
}

void QQuick3DParticle::setAlignMode(AlignMode alignMode)
{
    if (m_alignMode == alignMode)
        return;

    m_alignMode = alignMode;
    Q_EMIT alignModeChanged();
}

void QQuick3DParticle::setTarget(QQuick3DNode *target)
{
    if (m_target == target)
        return;

    m_target = target;
    Q_EMIT targetChanged();
}

void QQuick3DParticle::updateBurstIndex(int amount) {
    m_lastBurstIndex += amount;
}

int QQuick3DParticle::nextCurrentIndex() {
    m_currentIndex = (m_currentIndex < m_maxAmount - 1) ? m_currentIndex + 1 : m_lastBurstIndex;
    return m_currentIndex;
}

QT_END_NAMESPACE
