// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qopenxrruntimeinfo_p.h"
#include <QtQuick3DXr/private/qopenxrmanager_p.h>
#include <QtQuick/QQuickWindow>
#include <rhi/qrhi.h>

QT_BEGIN_NAMESPACE

QOpenXRRuntimeInfo::QOpenXRRuntimeInfo(QOpenXRManager *manager, QObject *parent)
    : QObject(parent),
      m_openXRManager(manager)
{
}

QStringList QOpenXRRuntimeInfo::enabledExtensions() const
{
    return m_openXRManager->m_enabledExtensions;
}

QString QOpenXRRuntimeInfo::runtimeName() const
{
    return m_openXRManager->m_runtimeName;
}

QString QOpenXRRuntimeInfo::runtimeVersion() const
{
    return m_openXRManager->m_runtimeVersion.toString();
}

QString QOpenXRRuntimeInfo::graphicsApiName() const
{
    // This matches what Qt Quick's GraphicsInfo would expose to QML, but that
    // does not provide a string. We have seen way too many switch statements
    // in JS for this. So have a string property here and call it a day.
    if (m_openXRManager->isValid() && m_openXRManager->m_quickWindow) {
        QRhi *rhi = m_openXRManager->m_quickWindow->rhi();
        if (rhi)
            return QString::fromLatin1(rhi->backendName());
    }
    return QLatin1String("Unknown");
}

bool QOpenXRRuntimeInfo::multiViewRendering() const
{
    return m_openXRManager->m_multiviewRendering;
}

QT_END_NAMESPACE
