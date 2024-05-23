// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qopenxrruntimeinfo_p.h"
#include "qquick3dxrmanager_p.h"
#include <QtQuick/QQuickWindow>
#include <rhi/qrhi.h>

#if defined(USE_OPENXR)
#include "openxr/qquick3dxrmanager_openxr_p.h"
#endif

// FIXME: This whole class needs to be revisited.

QT_BEGIN_NAMESPACE

QOpenXRRuntimeInfo::QOpenXRRuntimeInfo(QQuick3DXrManager *manager, QObject *parent)
    : QObject(parent),
      m_openXRManager(manager)
{
}

QStringList QOpenXRRuntimeInfo::enabledExtensions() const
{
#if USE_OPENXR
    QQuick3DXrManagerPrivate *manager = QQuick3DXrManagerPrivate::get(m_openXRManager);
    return manager ? manager->m_enabledExtensions : QStringList{};
#else
    return QStringList{};
#endif
}

QString QOpenXRRuntimeInfo::runtimeName() const
{
#if USE_OPENXR
    QQuick3DXrManagerPrivate *manager = QQuick3DXrManagerPrivate::get(m_openXRManager);
    return manager ? manager->m_runtimeName : QString{};
#else
    return QString{};
#endif
}

QString QOpenXRRuntimeInfo::runtimeVersion() const
{
#if USE_OPENXR
    QQuick3DXrManagerPrivate *manager = QQuick3DXrManagerPrivate::get(m_openXRManager);
    return manager ? manager->m_runtimeVersion.toString() : QString{};
#else
    return QString{};
#endif
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

QT_END_NAMESPACE
