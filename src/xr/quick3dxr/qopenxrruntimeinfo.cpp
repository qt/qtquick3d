// Copyright (C) 2024 The Qt Company Ltd.
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

/*!
    \qmltype XrRuntimeInfo
    \inherits Item
    \inqmlmodule QtQuick3D.Xr
    \brief Displays information about the OpenXR runtime.

    This type provides information about the OpenXR runtime, including enabled
    extensions, runtime name, version, graphics API name, and whether multi-view
    rendering is supported.

    \note This type is automatically created by a \l XrView and it can not be
    manually created.
*/

QOpenXRRuntimeInfo::QOpenXRRuntimeInfo(QQuick3DXrManager *manager, QObject *parent)
    : QObject(parent),
      m_openXRManager(manager)
{
}

/*!
    \qmlproperty QStringList XrRuntimeInfo::enabledExtensions
    \brief A list of enabled OpenXR extensions.

    This property holds a QStringList containing the names of the
    OpenXR extensions that are currently enabled for the runtime.
*/

QStringList QOpenXRRuntimeInfo::enabledExtensions() const
{
#if USE_OPENXR
    QQuick3DXrManagerPrivate *manager = QQuick3DXrManagerPrivate::get(m_openXRManager);
    return manager ? manager->m_enabledExtensions : QStringList{};
#else
    return QStringList{};
#endif
}

/*!
    \qmlproperty QString XrRuntimeInfo::runtimeName
    \brief The name of the OpenXR runtime.

    This property provides the human-readable name of the OpenXR runtime being
    used.
*/

QString QOpenXRRuntimeInfo::runtimeName() const
{
#if USE_OPENXR
    QQuick3DXrManagerPrivate *manager = QQuick3DXrManagerPrivate::get(m_openXRManager);
    return manager ? manager->m_runtimeName : QString{};
#else
    return QString{};
#endif
}

/*!
    \qmlproperty QString XrRuntimeInfo::runtimeVersion
    \brief The version of the OpenXR runtime.

    This property returns the version string of the OpenXR runtime
    (for example, "1.0.0").
*/

QString QOpenXRRuntimeInfo::runtimeVersion() const
{
#if USE_OPENXR
    QQuick3DXrManagerPrivate *manager = QQuick3DXrManagerPrivate::get(m_openXRManager);
    return manager ? manager->m_runtimeVersion.toString() : QString{};
#else
    return QString{};
#endif
}

/*!
    \qmlproperty QString XrRuntimeInfo::graphicsApiName
    \brief The name of the graphics API used by the OpenXR runtime.
    This property specifies the name of the graphics API (for example, "Vulkan") that the
    OpenXR runtime is utilizing.
*/

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
