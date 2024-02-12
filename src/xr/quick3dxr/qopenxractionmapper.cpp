// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qopenxractionmapper_p.h"
#include "qopenxrcontroller_p.h"

QT_BEGIN_NAMESPACE

QOpenXRActionMapper::QOpenXRActionMapper(QObject *parent) : QObject { parent }
{
}

void QOpenXRActionMapper::classBegin()
{
}

void QOpenXRActionMapper::componentComplete()
{
    m_controller = qobject_cast<QOpenXRController *>(parent());
    if (m_controller && !m_controller->actionMapper())
        m_controller->setActionMapper(this);
}

QT_END_NAMESPACE
