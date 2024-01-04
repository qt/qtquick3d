// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qopenxrview_p.h"
#include <QQuickWindow>
#include <QQuickItem>

#include "qopenxrinputmanager_p.h"

QT_BEGIN_NAMESPACE

QOpenXRView::QOpenXRView()
    : m_openXRRuntimeInfo(&m_openXRManager)
{
    if (!m_openXRManager.initialize()) {
        QString errorString = m_openXRManager.errorString();
        if (errorString.isEmpty())
            errorString = tr("Failed to initialize OpenXR");
        qWarning("\n%s\n", qPrintable(errorString));
        QMetaObject::invokeMethod(this, "initializeFailed", Qt::QueuedConnection, errorString);
        return;
    }

    // Create View3D
    auto viewport = new QQuick3DViewport();
    viewport->setRenderMode(QQuick3DViewport::Underlay);
    auto contentItem = m_openXRManager.m_quickWindow->contentItem();
    viewport->setParentItem(contentItem);
    m_openXRManager.m_vrViewport = viewport;
    viewport->setImportScene(this);

    contentItem->forceActiveFocus(Qt::MouseFocusReason);

    connect(contentItem, &QQuickItem::heightChanged, this, &QOpenXRView::updateViewportGeometry);
    connect(contentItem, &QQuickItem::widthChanged, this, &QOpenXRView::updateViewportGeometry);
    connect(contentItem, &QQuickItem::xChanged, this, &QOpenXRView::updateViewportGeometry);
    connect(contentItem, &QQuickItem::yChanged, this, &QOpenXRView::updateViewportGeometry);
    connect(environment(), &QQuick3DSceneEnvironment::backgroundModeChanged, this, &QOpenXRView::handleClearColorChanged);
    connect(environment(), &QQuick3DSceneEnvironment::clearColorChanged, this, &QOpenXRView::handleClearColorChanged);

    connect(&m_openXRManager, &QOpenXRManager::sessionEnded, this, &QOpenXRView::handleSessionEnded);
    m_openXRManager.update();
}

QOpenXRView::~QOpenXRView()
{
    m_inDestructor = true;
}

QOpenXRActor *QOpenXRView::xrActor() const
{
    return m_openXRManager.m_xrActor;
}

QQuick3DSceneEnvironment *QOpenXRView::environment() const
{
    return m_openXRManager.m_vrViewport ? m_openXRManager.m_vrViewport->environment() : nullptr;
}

QOpenXRHandInput *QOpenXRView::leftHandInput() const
{
    return m_openXRManager.m_inputManager ? m_openXRManager.m_inputManager->leftHandInput() : nullptr;
}

QOpenXRHandInput *QOpenXRView::rightHandInput() const
{
    return m_openXRManager.m_inputManager ? m_openXRManager.m_inputManager->rightHandInput() : nullptr;
}

QOpenXRGamepadInput *QOpenXRView::gamepadInput() const
{
    return m_openXRManager.m_inputManager ? m_openXRManager.m_inputManager->gamepadInput() : nullptr;
}

QQuick3DViewport *QOpenXRView::view3d() const
{
    return m_openXRManager.m_vrViewport;
}

bool QOpenXRView::enablePassthrough() const
{
    return m_openXRManager.m_enablePassthrough;
}

QOpenXRRuntimeInfo *QOpenXRView::runtimeInfo() const
{
    return &m_openXRRuntimeInfo;
}

QOpenXRInputManager *QOpenXRView::inputManager() const
{
    return m_openXRManager.m_inputManager;
}

void QOpenXRView::setEnvironment(QQuick3DSceneEnvironment *environment)
{
    if (!m_openXRManager.m_vrViewport)
        return;

    if (m_openXRManager.m_vrViewport->environment() == environment)
        return;

    disconnect(m_openXRManager.m_vrViewport->environment(), &QQuick3DSceneEnvironment::backgroundModeChanged, this, &QOpenXRView::handleClearColorChanged);
    disconnect(m_openXRManager.m_vrViewport->environment(), &QQuick3DSceneEnvironment::clearColorChanged, this, &QOpenXRView::handleClearColorChanged);

    m_openXRManager.m_vrViewport->setEnvironment(environment);
    handleClearColorChanged();

    connect(environment, &QQuick3DSceneEnvironment::backgroundModeChanged, this, &QOpenXRView::handleClearColorChanged);
    connect(environment, &QQuick3DSceneEnvironment::clearColorChanged, this, &QOpenXRView::handleClearColorChanged);

    emit environmentChanged(m_openXRManager.m_vrViewport->environment());
}

bool QOpenXRView::isPassthroughSupported() const
{
    if (!m_openXRManager.isValid())
        return false;

    return m_openXRManager.supportsPassthrough();
}

void QOpenXRView::setEnablePassthrough(bool enable)
{
    if (!m_openXRManager.isValid())
        return;

    if (m_openXRManager.m_enablePassthrough == enable)
        return;

    // bail if passthrough is not supported
    if (enable && !m_openXRManager.supportsPassthrough()) {
        qWarning("Enabling Passthrough is not supported.");
        return;
    }

    m_openXRManager.setPassthroughEnabled(enable);

    emit enablePassthroughChanged(enable);
}

bool QOpenXRView::isQuitOnSessionEndEnabled() const
{
    return m_quitOnSessionEnd;
}

void QOpenXRView::setQuitOnSessionEnd(bool enable)
{
    if (m_quitOnSessionEnd == enable)
        return;

    m_quitOnSessionEnd = enable;
    emit quitOnSessionEndChanged();
}

void QOpenXRView::updateViewportGeometry()
{
    auto contentItem = m_openXRManager.m_quickWindow->contentItem();
    auto viewport = m_openXRManager.m_vrViewport;
    if (viewport->height() != contentItem->height())
        viewport->setHeight(contentItem->height());
    if (viewport->width() != contentItem->width())
        viewport->setWidth(contentItem->width());
    if (viewport->x() != contentItem->x())
        viewport->setX(contentItem->x());
    if (viewport->y() != contentItem->y())
        viewport->setY(contentItem->y());
}

void QOpenXRView::handleSessionEnded()
{
    emit sessionEnded();
    if (m_quitOnSessionEnd)
        QCoreApplication::quit();
}

void QOpenXRView::handleClearColorChanged()
{
    auto env = environment();

    if (env) {
        if (env->backgroundMode() == QQuick3DSceneEnvironment::Color)
            m_openXRManager.m_quickWindow->setColor(env->clearColor());
        else if (env->backgroundMode() == QQuick3DSceneEnvironment::Transparent)
            m_openXRManager.m_quickWindow->setColor(Qt::transparent);
    }
}

QT_END_NAMESPACE
