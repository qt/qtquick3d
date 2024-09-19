// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qtquick3dxrglobal_p.h"
#include "qquick3dxritem_p.h"
#include "qquick3dxrview_p.h"
#include <QQuickWindow>
#include <QQuickItem>

#include "qquick3dxrinputmanager_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype XrView
    \inherits Node
    \inqmlmodule QtQuick3D.Xr
    \brief Sets up the view for an Xr application.

    An XrView sets up the view for an XR application.
    The following snippet is from the \l{\qxr Simple Example} and shows
    how to use the type.

    \quotefromfile xr_simple/main.qml
    \printto XrOrigin
*/

QQuick3DXrView::QQuick3DXrView()
    : m_xrRuntimeInfo(&m_xrManager)
{
    init();
}

QQuick3DXrView::~QQuick3DXrView()
{
    m_inDestructor = true;
}

/*!
    \qmlproperty XrOrigin QtQuick3D.Xr::XrView::xrOrigin
    \brief Holds the active XR origin.

    The XR origin is the point in the scene that is considered the origin of the
    XR coordinate system. The XR origin is used to position tracked objects like
    the camera and controllers in the scene. An application can have multiple XrOrigins
    but only one can be active at a time.

    \note This property must be set for the scene to be rendered in XR.

    \sa XrOrigin
*/

QQuick3DXrOrigin *QQuick3DXrView::xrOrigin() const
{
    return m_xrOrigin;
}

/*!
    \qmlproperty SceneEnvironment QtQuick3D.Xr::XrView::environment
    \summary Holds the SceneEnvironment for the XR view.
*/

QQuick3DSceneEnvironment *QQuick3DXrView::environment() const
{
    return m_xrManager.m_vrViewport ? m_xrManager.m_vrViewport->environment() : nullptr;
}

QQuick3DViewport *QQuick3DXrView::view3d() const
{
    return m_xrManager.m_vrViewport;
}

/*!
    \qmlproperty bool QtQuick3D.Xr::XrView::passthroughEnabled
    \summary Holds whether passthrough is enabled for the XR view.
*/
bool QQuick3DXrView::passthroughEnabled() const
{
    return m_xrManager.isPassthroughEnabled();
}

/*!
    \qmlproperty QQuick3DXrRuntimeInfo QtQuick3D.Xr::XrView::runtimeInfo
    \summary Provides information about the XR runtime for the XR view.
*/

QQuick3DXrRuntimeInfo *QQuick3DXrView::runtimeInfo() const
{
    return &m_xrRuntimeInfo;
}

void QQuick3DXrView::setEnvironment(QQuick3DSceneEnvironment *environment)
{
    if (environment != m_sceneEnvironment)
        m_sceneEnvironment = environment;

    if (!m_xrManager.m_vrViewport)
        return;

    auto oldEnvironment = m_xrManager.m_vrViewport->environment();
    if (oldEnvironment == environment)
        return;

    if (oldEnvironment) {
        disconnect(oldEnvironment, &QQuick3DSceneEnvironment::backgroundModeChanged, this, &QQuick3DXrView::handleClearColorChanged);
        disconnect(oldEnvironment, &QQuick3DSceneEnvironment::clearColorChanged, this, &QQuick3DXrView::handleClearColorChanged);
        disconnect(oldEnvironment, &QQuick3DSceneEnvironment::antialiasingModeChanged, this, &QQuick3DXrView::handleAAChanged);
        disconnect(oldEnvironment, &QQuick3DSceneEnvironment::antialiasingQualityChanged, this, &QQuick3DXrView::handleAAChanged);
    }

    m_xrManager.m_vrViewport->setEnvironment(environment);
    handleClearColorChanged();
    handleAAChanged();

    if (environment) {
        connect(environment, &QQuick3DSceneEnvironment::backgroundModeChanged, this, &QQuick3DXrView::handleClearColorChanged);
        connect(environment, &QQuick3DSceneEnvironment::clearColorChanged, this, &QQuick3DXrView::handleClearColorChanged);
        connect(environment, &QQuick3DSceneEnvironment::antialiasingModeChanged, this, &QQuick3DXrView::handleAAChanged);
        connect(environment, &QQuick3DSceneEnvironment::antialiasingQualityChanged, this, &QQuick3DXrView::handleAAChanged);
    }

    emit environmentChanged(m_xrManager.m_vrViewport->environment());
}

/*!
    \qmlproperty bool QtQuick3D.Xr::XrView::passthroughSupported
    \summary Indicates whether passthrough is supported for the XR view.
*/

bool QQuick3DXrView::passthroughSupported() const
{
    if (!m_xrManager.isValid())
        return false;

    return m_xrManager.supportsPassthrough();
}

void QQuick3DXrView::setPassthroughEnabled(bool enable)
{
    if (!m_xrManager.isValid()) {
        qWarning("Attempted to set passthrough mode without a valid XR manager");
        return;
    }

    // bail if passthrough is not supported
    if (enable && !m_xrManager.supportsPassthrough()) {
        qWarning("Enabling Passthrough is not supported.");
        return;
    }

    const bool orgPassthroughEnabled = m_xrManager.isPassthroughEnabled();
    m_xrManager.setPassthroughEnabled(enable);

    if (orgPassthroughEnabled != m_xrManager.isPassthroughEnabled())
        emit passthroughEnabledChanged();
}

/*!
    \qmlproperty enumeration QtQuick3D.Xr::XrView::fixedFoveation
    \brief Controls the level of fixed foveated rendering for the XrView.
    \default XrView.HighFoveation

    Foveated rendering reduces GPU load by reducing image quality (resolution)
    in areas where the difference is less perceptible to the eye. With fixed
    foveated rendering, the areas with reduced visual fidelity are fixed and do
    not change. On some platforms, there is no concept of fixed foveated
    rendering or control over it. For example, VisionOS-based devices perform
    dynamic, eye-tracked foveation; thus, the value of this property is
    ignored in practice. Other devices, such as the Meta Quest 3, only
    support fixed foveation, which makes this property relevant.

    The value can be one of:
    \value XrView.NoFoveation 0, no foveation.
    \value XrView.LowFoveation 1, low foveation.
    \value XrView.MediumFoveation 2, medium foveation.
    \value XrView.HighFoveation 3, high foveation.

    Where supported, the default is \c HighFoveation. Therefore, changing this
    value in applications should be rarely needed in practice.
*/

QQuick3DXrView::FoveationLevel QQuick3DXrView::fixedFoveation() const
{
    return FoveationLevel(m_xrManager.getFixedFoveationLevel());
}

void QQuick3DXrView::setFixedFoveation(FoveationLevel level)
{
    const auto orgFoviationLevel = m_xrManager.getFixedFoveationLevel();
    m_xrManager.setFixedFoveationLevel(QtQuick3DXr::FoveationLevel(level));
    if (orgFoviationLevel != m_xrManager.getFixedFoveationLevel())
        emit fixedFoveationChanged();
}

/*!
    \qmlproperty bool QtQuick3D.Xr::XrView::isQuitOnSessionEndEnabled
    \brief Holds whether the application should quit when the XR session ends.
*/

bool QQuick3DXrView::isQuitOnSessionEndEnabled() const
{
    return m_quitOnSessionEnd;
}

void QQuick3DXrView::setQuitOnSessionEnd(bool enable)
{
    if (m_quitOnSessionEnd == enable)
        return;

    m_quitOnSessionEnd = enable;
    emit quitOnSessionEndChanged();
}
/*!
    \qmlproperty RenderStats QtQuick3D.Xr::XrView::renderStats
    \summary Holds rendering statistics for the XR view.
*/

QQuick3DRenderStats *QQuick3DXrView::renderStats() const
{
    return m_xrManager.m_vrViewport ? m_xrManager.m_vrViewport->renderStats() : nullptr;
}

void QQuick3DXrView::updateViewportGeometry()
{
    auto contentItem = m_xrManager.m_quickWindow->contentItem();
    auto viewport = m_xrManager.m_vrViewport;
    if (viewport->height() != contentItem->height())
        viewport->setHeight(contentItem->height());
    if (viewport->width() != contentItem->width())
        viewport->setWidth(contentItem->width());
    if (viewport->x() != contentItem->x())
        viewport->setX(contentItem->x());
    if (viewport->y() != contentItem->y())
        viewport->setY(contentItem->y());
}

void QQuick3DXrView::handleSessionEnded()
{
    emit sessionEnded();
    if (m_quitOnSessionEnd)
        QCoreApplication::quit();
}

void QQuick3DXrView::handleClearColorChanged()
{
    auto env = environment();

    if (env) {
        if (env->backgroundMode() == QQuick3DSceneEnvironment::Color)
            m_xrManager.m_quickWindow->setColor(env->clearColor());
        else if (env->backgroundMode() == QQuick3DSceneEnvironment::Transparent)
            m_xrManager.m_quickWindow->setColor(Qt::transparent);
    }
}

void QQuick3DXrView::handleAAChanged()
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
    m_xrManager.setSamples(samples);
}

bool QQuick3DXrView::init()
{
    if (m_isInitialized) {
        qWarning("Already initialized!");
        return false;
    }

    if (!m_xrManager.isReady() && !m_xrManager.initialize()) {
        qDebug() << "Waiting for XR platform to be initialized...";
        connect(&m_xrManager, &QQuick3DXrManager::initialized, this, &QQuick3DXrView::init, Qt::UniqueConnection);
        return false;
    }

    if (!m_xrManager.initialize()) {
        QString errorString = m_xrManager.errorString();
        if (errorString.isEmpty())
            errorString = tr("Failed to initialize XR platform");
        qWarning("\n%s\n", qPrintable(errorString));
        QMetaObject::invokeMethod(this, "initializeFailed", Qt::QueuedConnection, errorString);
        return false;
    }

    // Create View3D
    QSSG_CHECK_X(m_xrManager.m_vrViewport == nullptr, "View3D already created!");
    auto viewport = new QQuick3DViewport();
    viewport->setRenderMode(QQuick3DViewport::Underlay);
    auto contentItem = m_xrManager.m_quickWindow->contentItem();
    viewport->setParentItem(contentItem);
    m_xrManager.m_vrViewport = viewport;
    viewport->setImportScene(this);

    contentItem->forceActiveFocus(Qt::MouseFocusReason);

    connect(contentItem, &QQuickItem::heightChanged, this, &QQuick3DXrView::updateViewportGeometry);
    connect(contentItem, &QQuickItem::widthChanged, this, &QQuick3DXrView::updateViewportGeometry);
    connect(contentItem, &QQuickItem::xChanged, this, &QQuick3DXrView::updateViewportGeometry);
    connect(contentItem, &QQuickItem::yChanged, this, &QQuick3DXrView::updateViewportGeometry);

    QQuick3DSceneEnvironment *env = environment();
    if (env) {
        connect(env, &QQuick3DSceneEnvironment::backgroundModeChanged, this, &QQuick3DXrView::handleClearColorChanged);
        connect(env, &QQuick3DSceneEnvironment::clearColorChanged, this, &QQuick3DXrView::handleClearColorChanged);
        connect(env, &QQuick3DSceneEnvironment::antialiasingModeChanged, this, &QQuick3DXrView::handleAAChanged);
        connect(env, &QQuick3DSceneEnvironment::antialiasingQualityChanged, this, &QQuick3DXrView::handleAAChanged);
    }

    connect(&m_xrManager, &QQuick3DXrManager::sessionEnded, this, &QQuick3DXrView::handleSessionEnded);
    connect(&m_xrManager, &QQuick3DXrManager::frameReady, this, &QQuick3DXrView::frameReady);
    connect(&m_xrManager, &QQuick3DXrManager::referenceSpaceChanged, this, &QQuick3DXrView::referenceSpaceChanged);

    // NOTE: If we've called async, we need to make sure the environment, etc. is set again
    setEnvironment(m_sceneEnvironment);

    m_xrManager.update();

    m_isInitialized = true;

    return m_isInitialized;
}

/*!
    \qmlmethod pickResult XrView::rayPick(vector3d origin, vector3d direction)

    This method will \e shoot a ray into the scene starting at \a origin and in
    \a direction and return information about the nearest intersection with an
    object in the scene.

    For example, pass the position and forward vector of
    any object in a scene to see what object is in front of an item. This
    makes it possible to do picking from any point in the scene.
 */
QQuick3DPickResult QQuick3DXrView::rayPick(const QVector3D &origin, const QVector3D &direction) const
{
    return m_xrManager.m_vrViewport->rayPick(origin, direction);
}

/*!
    \qmlmethod List<pickResult> XrView::rayPickAll(vector3d origin, vector3d direction)

    This method will \e shoot a ray into the scene starting at \a origin and in
    \a direction and return a list of information about the nearest intersections with
    objects in the scene.
    The list is presorted by distance from the origin along the direction
    vector with the nearest intersections appearing first and the furthest
    appearing last.

    This can, for instance, be called with the position and forward vector of
    any object in a scene to see what objects are in front of an item. This
    makes it possible to do picking from any point in the scene.
 */
QList<QQuick3DPickResult> QQuick3DXrView::rayPickAll(const QVector3D &origin, const QVector3D &direction) const
{
    return m_xrManager.m_vrViewport->rayPickAll(origin, direction);
}

/*!
    \qmlmethod XrView::setTouchpoint(Item target, point position, int pointId, bool pressed)

    Sends a synthetic touch event to \a target, moving the touch point with ID \a pointId to \a position,
    with \a pressed determining if the point is pressed.
    Also sends the appropriate touch release event if \a pointId was previously active on a different
    item.
*/

void QQuick3DXrView::setTouchpoint(QQuickItem *target, const QPointF &position, int pointId, bool pressed)
{
    view3d()->setTouchpoint(target, position, pointId, pressed);
}

// TODO: Maybe do a proper QQuick3DXrViewPrivate instead
struct QQuick3DXrView::XrTouchState
{
    QHash<int, QQuick3DXrItem::TouchState> points;
};

/*!
    \qmlmethod vector3d XrView::processTouch(vector3d position, int pointId)

    This method will search for an XrItem near \a position and send a virtual
    touch event with touch point ID \a pointId if \a position maps to a point
    on the surface.

    The return value is the offset between \a position and the touched point on
    the surface. This can be used to prevent a hand model from passing through
    an XrItem.

    \sa XrHandModel

*/

QVector3D QQuick3DXrView::processTouch(const QVector3D &pos, int pointId)
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
    \li The distance from the plane to the touch point. It will be \c 0 if \c pressed is \c true.
    \endtable

 */

#define Q_TOUCHPOINT_STATE(prop) { QStringLiteral(#prop), QVariant::fromValue(it->prop) }
QVariantMap QQuick3DXrView::touchpointState(int pointId) const
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

/*!
    \qmlproperty enumeration QtQuick3D.Xr::XrView::referenceSpace
    \brief Gets or sets the reference space for the XR view.

     It can be one of:
     \value XrView.ReferenceSpaceUnknown
     \value XrView.ReferenceSpaceLocal
     \value XrView.ReferenceSpaceStage
     \value XrView.ReferenceSpaceLocalFloor
*/

QQuick3DXrView::ReferenceSpace QQuick3DXrView::referenceSpace() const
{
    return ReferenceSpace(m_xrManager.getReferenceSpace());
}

void QQuick3DXrView::setReferenceSpace(ReferenceSpace newReferenceSpace)
{
    m_xrManager.setReferenceSpace(QtQuick3DXr::ReferenceSpace(newReferenceSpace));
}

bool QQuick3DXrView::depthSubmissionEnabled() const
{
    if (!m_xrManager.isValid()) {
        qWarning("Attempted to check depth submission mode without a valid XR manager");
        return false;
    }

    return m_xrManager.isDepthSubmissionEnabled();
}

/*!
    \qmlproperty bool QtQuick3D.Xr::XrView::multiViewRenderingSupported

    \brief This read-only property reports the availability of \l{Multiview Rendering}.

    \sa multiViewRenderingEnabled
 */
bool QQuick3DXrView::isMultiViewRenderingSupported() const
{
    if (!m_xrManager.isValid())
        return false;

    return m_xrManager.isMultiViewRenderingSupported();
}

/*!
    \qmlproperty bool QtQuick3D.Xr::XrView::multiViewRenderingEnabled

    \brief Gets or sets whether \l{Multiview Rendering} is enabled for the XR view.

    \default false

    Changing the value to \c true has an effect only when
    \l multiViewRenderingSupported is \c true.

    This property tells you if multiview rendering is actually in use at run time.
    When not supported, the value will flip back to \c false.

    Enabling multiview rendering is recommended. It can improve performance and reduce
    CPU and GPU power usage. It defaults to disabled to ensure maximum
    compatibility. Developers are encouraged to verify that their application
    renders as expected with multiViewRenderingEnabled set to \c true and then
    leave it set afterward.

    Certain Qt Quick and Quick 3D features that involve shader code that is
    provided by the application may need this code to be modified to be multiview
    compatible.  Examples of these are custom 2D and 3D materials and
    postprocessing effects.

    \note Changing the value dynamically while the scene is already up and
    running is possible but not recommended because enabling or disabling
    multiview mode involves releasing and recreating certain graphics and XR
    resources. Depending on the platform and headset, this may cause
    undesirable visual effects. For example, the scene may disappear and reappear.

    \sa multiViewRenderingSupported {Multiview Rendering}
*/
bool QQuick3DXrView::multiViewRenderingEnabled() const
{
    if (!m_xrManager.isValid())
        return false;

    return m_xrManager.isMultiViewRenderingEnabled();
}

void QQuick3DXrView::registerXrItem(QQuick3DXrItem *newXrItem)
{
    m_xrItems.append(newXrItem);
}

void QQuick3DXrView::unregisterXrItem(QQuick3DXrItem *xrItem)
{
    m_xrItems.removeAll(xrItem);
}

/*!
    \qmlproperty bool QtQuick3D.Xr::XrView::depthSubmissionEnabled
    \brief Controls whether submitting the depth buffer to the XR compositor
    is enabled.
    \default false

    By default, the depth buffer used by the 3D scene in the XrView is not exposed
    to the XR compositor. However, in some platforms, depth submission is implicit
    and cannot be disabled or controlled by the application. An example of this is
    VisionOS. Changing this property has no effect on those platforms. Elsewhere,
    with OpenXR in particular, support depends on the OpenXR implementation used
    at run time.

    It is always safe to set depthSubmissionEnabled to \c true. It will just have
    no effect when not supported by the underlying stack. To be sure, you can
    inspect the debug output to see if depth submission is in use.
    Submitting the depth buffer may improve reprojections that the XR compositor
    may perform. Reprojection could happen, for example, when the system cannot
    maintain the target frame rate and thus has to resort to predicting frame
    contents to improve and stabilize the user's perception of the
    scene and reduce possible motion sickness. However, the application and
    Qt have no control over data usage. It could also happen that
    submitting depth data has no practical effects and is ignored by the
    underlying XR runtime and compositor.

    In practice, submitting the depth buffer implies rendering into a depth
    texture provided by the XR runtime instead of the intermediate texture/render buffer
    created and managed by Qt. Rendering into a depth texture has certain lower-level
    consequences that can have a performance impact:

    When using \l{QtQuick3D::SceneEnvironment::antialiasingMode}{multisample antialiasing}
    (MSAA), enabling depth submission implies rendering into a multisample depth
    texture and resolving the samples into the non-multisample depth texture provided by
    the XR runtime. Without depth submission,
    the resolve step would not be necessary. In addition, some 3D APIs
    do not support resolving multisample depth-stencil data (see
    the \l{QRhi::ResolveDepthStencil} flag for details). Without this support,
    attempts to  enable depth submission in combination with MSAA are gracefully ignored.

    Even when MSAA is not used, enabling depth submission triggers writing out
    depth data with 3D APIs that have control over this. The store operation for
    depth/stencil data is typically indicated by Qt as unnecessary, which can
    have positive performance impacts on tiled GPU architectures. This is not
    done with depth submission because depth data must always be written out
    from Qt's perspective.

    \note We recommended that developers test their applications with depth
    submission enabled, evaluate the advantages and disadvantages, and make a
    conscious choice based on their testing if they wish to enable it or not.
*/

void QQuick3DXrView::setDepthSubmissionEnabled(bool enable)
{
    if (!m_xrManager.isValid()) {
        qWarning("Attempted to set depth submission mode without a valid XR manager");
        return;
    }

    const bool orgDepthSubmission = m_xrManager.isDepthSubmissionEnabled();

    m_xrManager.setDepthSubmissionEnabled(enable);

    if (orgDepthSubmission != m_xrManager.isDepthSubmissionEnabled())
        emit depthSubmissionEnabledChanged();
}

void QQuick3DXrView::setMultiViewRenderingEnabled(bool enable)
{
    if (!m_xrManager.isValid()) {
        qWarning("Attempted to set multiview rendering mode without having m_openXRManager initialized");
        return;
    }

    const bool orgMultiView = m_xrManager.isMultiViewRenderingEnabled();
    m_xrManager.setMultiViewRenderingEnabled(enable);

    if (orgMultiView != m_xrManager.isMultiViewRenderingEnabled())
        emit multiViewRenderingEnabledChanged();
}

void QQuick3DXrView::setXROrigin(QQuick3DXrOrigin *newXrOrigin)
{
    if (m_xrOrigin == newXrOrigin)
        return;

    QQuick3DObjectPrivate::attachWatcher(this, &QQuick3DXrView::setXROrigin, newXrOrigin, m_xrOrigin);

    m_xrOrigin = newXrOrigin;

    // Make sure the XrOrigin has a parent item, if it hasn't, we're it.
    if (m_xrOrigin && !m_xrOrigin->parentItem())
        m_xrOrigin->setParentItem(this);

    m_xrManager.setXROrigin(m_xrOrigin);

    emit xrOriginChanged();
}

/*!
    \qmlsignal XrView::initializeFailed(const QString &errorString)

    Emitted when initialization fails, and there is a new \a errorString
    describing the failure.
 */

/*!
    \qmlsignal XrView::sessionEnded()

    Emitted when the session ends.
 */

/*!
    \qmlsignal XrView::frameReady()
    \internal

    Emitted when a new frame is ready.
 */

QT_END_NAMESPACE
