// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qopenxranimationdriver_p.h"

QT_BEGIN_NAMESPACE

QOpenXRAnimationDriver::QOpenXRAnimationDriver()
{
}

void QOpenXRAnimationDriver::advance()
{
    m_elapsed += m_step;
    advanceAnimation();
}

qint64 QOpenXRAnimationDriver::elapsed() const
{
    return m_elapsed;
}

void QOpenXRAnimationDriver::setStep(int stepSize)
{
    m_step = stepSize;
}

QT_END_NAMESPACE
