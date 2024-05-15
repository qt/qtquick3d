// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qquick3dxritem_p.h"
#include "qopenxrview_p.h"
#include <QQuickWindow>
#include <QQuickItem>

#if defined(Q_NO_TEMPORARY_DISABLE_XR_API)
#include "qopenxrinputmanager_p.h"
#endif // Q_NO_TEMPORARY_DISABLE_XR_API

QT_BEGIN_NAMESPACE

QOpenXRView::QOpenXRView()
    : m_openXRRuntimeInfo(&m_openXRManager)
{
    init();
}

QOpenXRView::~QOpenXRView()
{
    m_inDestructor = true;
}

QQuick3DXrOrigin *QOpenXRView::xrOrigin() const
{
    return m_openXRManager.m_xrOrigin;
}

QQuick3DSceneEnvironment *QOpenXRView::environment() const
{
    return m_openXRManager.m_vrViewport ? m_openXRManager.m_vrViewport->environment() : nullptr;
}

QOpenXRHandInput *QOpenXRView::leftHandInput() const
{
#if !defined(Q_OS_VISIONOS)
    return m_openXRManager.m_inputManager ? m_openXRManager.m_inputManager->leftHandInput() : nullptr;
#else
    return nullptr;
#endif
}

QOpenXRHandInput *QOpenXRView::rightHandInput() const
{
#if !defined(Q_OS_VISIONOS)
    return m_openXRManager.m_inputManager ? m_openXRManager.m_inputManager->rightHandInput() : nullptr;
#else
    return nullptr;
#endif
}

QOpenXRHandTrackerInput *QOpenXRView::leftHandTrackerInput() const
{
#if !defined(Q_OS_VISIONOS)
    return m_openXRManager.m_inputManager->leftHandTrackerInput();
#else
    return nullptr;
#endif
}

QOpenXRHandTrackerInput *QOpenXRView::rightHandTrackerInput() const
{
#if !defined(Q_OS_VISIONOS)
    return m_openXRManager.m_inputManager->rightHandTrackerInput();
#else
    return nullptr;
#endif
}

QOpenXRGamepadInput *QOpenXRView::gamepadInput() const
{
#if !defined(Q_OS_VISIONOS)
    return m_openXRManager.m_inputManager ? m_openXRManager.m_inputManager->gamepadInput() : nullptr;
#else
    return nullptr;
#endif
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
    if (environment != m_sceneEnvironment)
        m_sceneEnvironment = environment;

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
#if !defined(Q_OS_VISIONOS)
    if (!m_openXRManager.isValid())
        return NoFoveation;

    return FoveationLevel(m_openXRManager.m_foveationLevel);
#else
    // Foveation is not configurable on VisionOS
    return QOpenXRView::HighFoveation;
#endif
}

void QOpenXRView::setFixedFoveation(FoveationLevel level)
{
#if !defined(Q_OS_VISIONOS)
    if (!m_openXRManager.isValid())
        return;

    const XrFoveationLevelFB xrLevel = XrFoveationLevelFB(level);
    if (m_openXRManager.m_foveationLevel == xrLevel)
        return;

    m_openXRManager.m_foveationLevel = xrLevel;
    m_openXRManager.setupMetaQuestFoveation();

    emit fixedFoveationChanged();
#else
    // Foveation is not configurable on VisionOS
    Q_UNUSED(level);
#endif
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

bool QOpenXRView::init()
{
    if (m_isInitialized) {
        qWarning("Already initialized!");
        return false;
    }

    if (!m_openXRManager.isReady() && !m_openXRManager.initialize()) {
        qDebug() << "Waiting for OpenXR to be initialized...";
        connect(&m_openXRManager, &QOpenXRManager::initialized, this, &QOpenXRView::init, Qt::UniqueConnection);
        return false;
    }

    if (!m_openXRManager.initialize()) {
        QString errorString = m_openXRManager.errorString();
        if (errorString.isEmpty())
            errorString = tr("Failed to initialize OpenXR");
        qWarning("\n%s\n", qPrintable(errorString));
        QMetaObject::invokeMethod(this, "initializeFailed", Qt::QueuedConnection, errorString);
        return false;
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

    // NOTE: If we're called async we need to make sure the environment etc. is set again
    setEnvironment(m_sceneEnvironment);

    m_openXRManager.update();

    m_isInitialized = true;

    return m_isInitialized;
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

/*!
    \qmlmethod XrView::setTouchpoint(Item target, point position, int pointId, bool pressed)

    Sends a synthetic touch event to \a target, moving the touch point with ID \a pointId to \a position,
    with \a pressed determining if the point is pressed.
    Also sends the appropriate touch release event if \a pointId was previously active on a different
    item.
*/

void QOpenXRView::setTouchpoint(QQuickItem *target, const QPointF &position, int pointId, bool pressed)
{
    view3d()->setTouchpoint(target, position, pointId, pressed);
}

// TODO: Maybe do a proper QOpenXRViewPrivate instead
struct QOpenXRView::XrTouchState
{
    QHash<int, QQuick3DXrItem::TouchState> points;
};

/*!
    \qmlmethod vector3d XrView::processTouch(vector3d position, int pointId)

    This method will search for an XrItem near \a position, and send a virtual
    touch event with touch point ID \a pointId if \a position maps to a point
    on the surface.

    The return value is the offset between \a position and the touched point on
    the surface. This can be used to prevent a hand model from passing through
    an XrItem.

    \sa XrHandModel

*/

QVector3D QOpenXRView::processTouch(const QVector3D &pos, int pointId)
{
    QVector3D offset;
    if (m_xrItems.isEmpty())
        return offset;

    if (!m_touchState)
        m_touchState = new XrTouchState;
    QQuick3DXrItem::TouchState &state = m_touchState->points[pointId];
    state.pointId = pointId; // in case it's a new point that was default-constructed

    auto *prevTarget = state.target;
    bool grabbed = false;
    if (prevTarget) {
        grabbed = prevTarget->handleVirtualTouch(this, pos, &state, &offset);
    }
    for (auto *item : std::as_const(m_xrItems)) {
        if (grabbed)
            break;
        if (item != prevTarget)
            grabbed = item->handleVirtualTouch(this, pos, &state, &offset);
    }

    return offset;
}

/*!
    \qmlmethod object XrView::touchpointState(int pointId)

    This method returns the state of the touch point with ID \a pointId.
    The state is represented by a map from property names to values:

    \table
    \header
    \li Key
    \li Type
    \li Description
    \row
    \li \c grabbed
    \li \c bool
    \li Is the point grabbed by an item? If \c false, all other values are \c undefined.
    \row
    \li \c target
    \li XrItem
    \li The item that is grabbing the touch point.
    \row
    \li \c pressed
    \li \c bool
    \li Is the touch point pressed?
    \row
    \li \c cursorPos
    \li \c point
    \li The 2D position of the touch point within \c target
    \row
    \li \c touchDistance
    \li \c real
    \li The distance from the plane to the touch point. Will be \c 0 if \c pressed is \c true.
    \endtable

 */

#define Q_TOUCHPOINT_STATE(prop) { QStringLiteral(#prop), QVariant::fromValue(it->prop) }
QVariantMap QOpenXRView::touchpointState(int pointId) const
{
    auto constexpr end = QHash<int, QQuick3DXrItem::TouchState>::const_iterator();
    auto it = m_touchState ? m_touchState->points.constFind(pointId) : end;

    if (it == end)
        return { { QStringLiteral("grabbed"), QVariant::fromValue(false) } };

    return { Q_TOUCHPOINT_STATE(target),
             Q_TOUCHPOINT_STATE(grabbed),
             Q_TOUCHPOINT_STATE(pressed),
             Q_TOUCHPOINT_STATE(cursorPos),
             Q_TOUCHPOINT_STATE(touchDistance) };
}
#undef Q_TOUCHPOINT_STATE

#if defined(Q_NO_TEMPORARY_DISABLE_XR_API)
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

#endif // Q_NO_TEMPORARY_DISABLE_XR_API

QOpenXRView::ReferenceSpace QOpenXRView::referenceSpace() const
{
#if !defined(Q_OS_VISIONOS)
    return getReferenceSpaceType(m_openXRManager.m_referenceSpace);
#else
    // I am not sure exactly what reference space is default
    // or what is supported etc.
    return ReferenceSpace::ReferenceSpaceLocalFloor;
#endif
}

void QOpenXRView::setReferenceSpace(ReferenceSpace newReferenceSpace)
{
#if !defined(Q_OS_VISIONOS)
    XrReferenceSpaceType referenceSpace = getXrReferenceSpaceType(newReferenceSpace);
    if (m_openXRManager.m_referenceSpace == referenceSpace)
        return;

    m_openXRManager.m_requestedReferenceSpace = referenceSpace;

    // we do not emit a changed signal here because it hasn't
    // changed yet.
#else
    // I'm not sure if it's possible to set a reference space on VisionOS
    Q_UNUSED(newReferenceSpace);
#endif
}

bool QOpenXRView::isDepthSubmissionEnabled() const
{
#if !defined(Q_OS_VISIONOS)
    if (!m_openXRManager.isValid())
        return false;

    return m_openXRManager.m_submitLayerDepth;
#else
    // Depth submission is required on VisionOS
    return true;
#endif
}

void QOpenXRView::registerXrItem(QQuick3DXrItem *newXrItem)
{
    m_xrItems.append(newXrItem);
}

void QOpenXRView::unregisterXrItem(QQuick3DXrItem *xrItem)
{
    m_xrItems.removeAll(xrItem);
}

void QOpenXRView::setEnableDepthSubmission(bool enable)
{
#if !defined(Q_OS_VISIONOS)
    if (!m_openXRManager.isValid()) {
        qWarning("Attempted to set depth submission mode without having m_openXRManager initialized");
        return;
    }

    if (m_openXRManager.m_submitLayerDepth == enable)
        return;

    if (m_openXRManager.m_compositionLayerDepthSupported) {
        if (enable)
            qDebug("Enabling submitLayerDepth");
        m_openXRManager.m_submitLayerDepth = enable;
        emit enableDepthSubmissionChanged();
    }
#else
    // VisionOS requires depth submission to be enabled
    Q_UNUSED(enable);
#endif
}

QT_END_NAMESPACE
