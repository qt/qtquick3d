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
    connect(environment(), &QQuick3DSceneEnvironment::antialiasingModeChanged, this, &QOpenXRView::handleAAChanged);
    connect(environment(), &QQuick3DSceneEnvironment::antialiasingQualityChanged, this, &QOpenXRView::handleAAChanged);

    connect(&m_openXRManager, &QOpenXRManager::sessionEnded, this, &QOpenXRView::handleSessionEnded);
    connect(&m_openXRManager, &QOpenXRManager::frameReady, this, &QOpenXRView::frameReady);
    connect(&m_openXRManager, &QOpenXRManager::referenceSpaceChanged, this, &QOpenXRView::referenceSpaceChanged);

    m_openXRManager.update();
}

QOpenXRView::~QOpenXRView()
{
    m_inDestructor = true;
}

QOpenXROrigin *QOpenXRView::xrOrigin() const
{
    return m_openXRManager.m_xrOrigin;
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

QOpenXRHandTrackerInput *QOpenXRView::leftHandTrackerInput() const
{
    return m_openXRManager.m_inputManager->leftHandTrackerInput();
}

QOpenXRHandTrackerInput *QOpenXRView::rightHandTrackerInput() const
{
    return m_openXRManager.m_inputManager->rightHandTrackerInput();
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

void QOpenXRView::setEnvironment(QQuick3DSceneEnvironment *environment)
{
    if (!m_openXRManager.m_vrViewport)
        return;

    auto oldEnvironment = m_openXRManager.m_vrViewport->environment();
    if (oldEnvironment == environment)
        return;

    disconnect(oldEnvironment, &QQuick3DSceneEnvironment::backgroundModeChanged, this, &QOpenXRView::handleClearColorChanged);
    disconnect(oldEnvironment, &QQuick3DSceneEnvironment::clearColorChanged, this, &QOpenXRView::handleClearColorChanged);
    disconnect(oldEnvironment, &QQuick3DSceneEnvironment::antialiasingModeChanged, this, &QOpenXRView::handleAAChanged);
    disconnect(oldEnvironment, &QQuick3DSceneEnvironment::antialiasingQualityChanged, this, &QOpenXRView::handleAAChanged);

    m_openXRManager.m_vrViewport->setEnvironment(environment);
    handleClearColorChanged();
    handleAAChanged();

    connect(environment, &QQuick3DSceneEnvironment::backgroundModeChanged, this, &QOpenXRView::handleClearColorChanged);
    connect(environment, &QQuick3DSceneEnvironment::clearColorChanged, this, &QOpenXRView::handleClearColorChanged);
    connect(environment, &QQuick3DSceneEnvironment::antialiasingModeChanged, this, &QOpenXRView::handleAAChanged);
    connect(environment, &QQuick3DSceneEnvironment::antialiasingQualityChanged, this, &QOpenXRView::handleAAChanged);

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

QOpenXRView::FoveationLevel QOpenXRView::fixedFoveation() const
{
    if (!m_openXRManager.isValid())
        return NoFoveation;

    return FoveationLevel(m_openXRManager.m_foveationLevel);
}

void QOpenXRView::setFixedFoveation(FoveationLevel level)
{
    if (!m_openXRManager.isValid())
        return;

    const XrFoveationLevelFB xrLevel = XrFoveationLevelFB(level);
    if (m_openXRManager.m_foveationLevel == xrLevel)
        return;

    m_openXRManager.m_foveationLevel = xrLevel;
    m_openXRManager.setupMetaQuestFoveation();

    emit fixedFoveationChanged();
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

QQuick3DRenderStats *QOpenXRView::renderStats() const
{
    return m_openXRManager.m_vrViewport ? m_openXRManager.m_vrViewport->renderStats() : nullptr;
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

void QOpenXRView::handleAAChanged()
{
    auto env = environment();
    int samples = 1;
    if (env && env->antialiasingMode() == QQuick3DSceneEnvironment::MSAA) {
        switch (env->antialiasingQuality()) {
        case QQuick3DSceneEnvironment::Medium:
            samples = 2;
            break;
        case QQuick3DSceneEnvironment::High:
            samples = 4;
            break;
        case QQuick3DSceneEnvironment::VeryHigh:
            samples = 8;
            break;
        }
    }
    m_openXRManager.setSamples(samples);
}

/*!
    \qmlmethod PickResult XrView::rayPick(vector3d origin, vector3d direction)

    This method will "shoot" a ray into the scene starting at \a origin and in
    \a direction and return information about the nearest intersection with an
    object in the scene.

    This can, for instance, be called with the position and forward vector of
    any object in a scene to see what object is in front of an item. This
    makes it possible to do picking from any point in the scene.
 */
QQuick3DPickResult QOpenXRView::rayPick(const QVector3D &origin, const QVector3D &direction) const
{
    return m_openXRManager.m_vrViewport->rayPick(origin, direction);
}

/*!
    \qmlmethod List<PickResult> XrView::rayPickAll(vector3d origin, vector3d direction)

    This method will "shoot" a ray into the scene starting at \a origin and in
    \a direction and return a list of information about the nearest intersections with
    objects in the scene.
    The list is presorted by distance from the origin along the direction
    vector with the nearest intersections appearing first and the furthest
    appearing last.

    This can, for instance, be called with the position and forward vector of
    any object in a scene to see what objects are in front of an item. This
    makes it possible to do picking from any point in the scene.
 */
QList<QQuick3DPickResult> QOpenXRView::rayPickAll(const QVector3D &origin, const QVector3D &direction) const
{
    return m_openXRManager.m_vrViewport->rayPickAll(origin, direction);
}

namespace {

XrReferenceSpaceType getXrReferenceSpaceType(QOpenXRView::ReferenceSpace referenceSpace)
{
    switch (referenceSpace) {
    case QOpenXRView::ReferenceSpace::ReferenceSpaceLocal:
        return XR_REFERENCE_SPACE_TYPE_LOCAL;
    case QOpenXRView::ReferenceSpace::ReferenceSpaceStage:
        return XR_REFERENCE_SPACE_TYPE_STAGE;
    case QOpenXRView::ReferenceSpace::ReferenceSpaceLocalFloor:
        return XR_REFERENCE_SPACE_TYPE_LOCAL_FLOOR_EXT;
    default:
        return XR_REFERENCE_SPACE_TYPE_LOCAL;
    }
}

QOpenXRView::ReferenceSpace getReferenceSpaceType(XrReferenceSpaceType referenceSpace)
{
    switch (referenceSpace) {
    case XR_REFERENCE_SPACE_TYPE_LOCAL:
        return QOpenXRView::ReferenceSpace::ReferenceSpaceLocal;
    case XR_REFERENCE_SPACE_TYPE_STAGE:
        return QOpenXRView::ReferenceSpace::ReferenceSpaceStage;
    case XR_REFERENCE_SPACE_TYPE_LOCAL_FLOOR_EXT:
        return QOpenXRView::ReferenceSpace::ReferenceSpaceLocalFloor;
    default:
        return QOpenXRView::ReferenceSpace::ReferenceSpaceUnknown;
    }
}

}


QOpenXRView::ReferenceSpace QOpenXRView::referenceSpace() const
{
    return getReferenceSpaceType(m_openXRManager.m_referenceSpace);
}

void QOpenXRView::setReferenceSpace(ReferenceSpace newReferenceSpace)
{
    XrReferenceSpaceType referenceSpace = getXrReferenceSpaceType(newReferenceSpace);
    if (m_openXRManager.m_referenceSpace == referenceSpace)
        return;

    m_openXRManager.m_requestedReferenceSpace = referenceSpace;

    // we do not emit a changed signal here because it hasn't
    // changed yet.
}

QT_END_NAMESPACE
