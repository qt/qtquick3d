// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qtquick3dxrglobal_p.h"
#include "qquick3dxritem_p.h"
#include "qquick3dxrview_p.h"
#include <QQuickWindow>
#include <QQuickItem>

#if defined(USE_OPENXR)
#include "qopenxrinputmanager_p.h"
#endif // USE_OPENXR

QT_BEGIN_NAMESPACE

/*!
    \qmltype XrView
    \inherits Node
    \inqmlmodule QtQuick3D.Xr
    \brief Sets up the view for an Xr application.

    An XrView sets up the view for an XR application.
    \quotefromfile xr_simple/main.qml
    \printto XrOrigin
*/

QQuick3DXrView::QQuick3DXrView()
    : m_openXRRuntimeInfo(&m_openXRManager)
{
    init();
}

QQuick3DXrView::~QQuick3DXrView()
{
    m_inDestructor = true;
}
/*!
    \qmlproperty XrOrigin QtQuick3D.Xr::XrView::xrOrigin
    \brief Holds the XR origin.
*/


QQuick3DXrOrigin *QQuick3DXrView::xrOrigin() const
{
    return m_openXRManager.m_xrOrigin;
}

/*!
    \qmlproperty SceneEnvironment QtQuick3D.Xr::XrView::environment
    \brief Holds the SceneEnvironment for the XR view.
*/

QQuick3DSceneEnvironment *QQuick3DXrView::environment() const
{
    return m_openXRManager.m_vrViewport ? m_openXRManager.m_vrViewport->environment() : nullptr;
}

/*!
    \qmlproperty XrHandInput QtQuick3D.Xr::XrView::leftHandInput
    \brief Provides access to the left hand input for the XR view.
*/

QOpenXRHandInput *QQuick3DXrView::leftHandInput() const
{
#if USE_OPENXR
    QQuick3DXrInputManager *inputManager = m_openXRManager.getInputManager();
    return inputManager ? inputManager->leftHandInput() : nullptr;
#else
    return nullptr;
#endif
}

/*!
    \qmlproperty XrHandInput QtQuick3D.Xr::XrView::rightHandInput
    \brief Provides access to the right hand input for the XR view.
*/

QOpenXRHandInput *QQuick3DXrView::rightHandInput() const
{
#if USE_OPENXR
    QQuick3DXrInputManager *inputManager = m_openXRManager.getInputManager();
    return inputManager ? inputManager->rightHandInput() : nullptr;
#else
    return nullptr;
#endif
}

/*!
    \qmlproperty XrHandTrackerInput QtQuick3D.Xr::XrView::leftHandTrackerInput
    \brief Provides access to the left hand tracker input for the XR view.
*/

QOpenXRHandTrackerInput *QQuick3DXrView::leftHandTrackerInput() const
{
#if USE_OPENXR
    QQuick3DXrInputManager *inputManager = m_openXRManager.getInputManager();
    return inputManager ? inputManager->leftHandTrackerInput() : nullptr;
#else
    return nullptr;
#endif
}

/*!
    \qmlproperty XrHandTrackerInput QtQuick3D.Xr::XrView::rightHandTrackerInput
    \brief Provides access to the right hand tracker input for the XR view.
*/

QOpenXRHandTrackerInput *QQuick3DXrView::rightHandTrackerInput() const
{
#if USE_OPENXR
    QQuick3DXrInputManager *inputManager = m_openXRManager.getInputManager();
    return inputManager ? inputManager->rightHandTrackerInput() : nullptr;
#else
    return nullptr;
#endif
}

/*!
    \qmlproperty XrGamepadInput QtQuick3D.Xr::XrView::gamepadInput
    \brief Provides access to the gamepad input for the XR view.
*/

QQuick3DXrGamepadInput *QQuick3DXrView::gamepadInput() const
{
#if USE_OPENXR
    QQuick3DXrInputManager *inputManager = m_openXRManager.getInputManager();
    return inputManager ? inputManager->gamepadInput() : nullptr;
#else
    return nullptr;
#endif
}

QQuick3DViewport *QQuick3DXrView::view3d() const
{
    return m_openXRManager.m_vrViewport;
}

/*!
    \qmlproperty bool QtQuick3D.Xr::XrView::isPassthroughEnabled
    \brief Holds whether passthrough is enabled for the XR view.
*/
bool QQuick3DXrView::isPassthroughEnabled() const
{
    return m_openXRManager.isPassthroughEnabled();
}

/*!
    \qmlproperty QOpenXRRuntimeInfo QtQuick3D.Xr::XrView::runtimeInfo
    \brief Provides information about the XR runtime for the XR view.
*/

QOpenXRRuntimeInfo *QQuick3DXrView::runtimeInfo() const
{
    return &m_openXRRuntimeInfo;
}

void QQuick3DXrView::setEnvironment(QQuick3DSceneEnvironment *environment)
{
    if (environment != m_sceneEnvironment)
        m_sceneEnvironment = environment;

    if (!m_openXRManager.m_vrViewport)
        return;

    auto oldEnvironment = m_openXRManager.m_vrViewport->environment();
    if (oldEnvironment == environment)
        return;

    if (oldEnvironment) {
        disconnect(oldEnvironment, &QQuick3DSceneEnvironment::backgroundModeChanged, this, &QQuick3DXrView::handleClearColorChanged);
        disconnect(oldEnvironment, &QQuick3DSceneEnvironment::clearColorChanged, this, &QQuick3DXrView::handleClearColorChanged);
        disconnect(oldEnvironment, &QQuick3DSceneEnvironment::antialiasingModeChanged, this, &QQuick3DXrView::handleAAChanged);
        disconnect(oldEnvironment, &QQuick3DSceneEnvironment::antialiasingQualityChanged, this, &QQuick3DXrView::handleAAChanged);
    }

    m_openXRManager.m_vrViewport->setEnvironment(environment);
    handleClearColorChanged();
    handleAAChanged();

    if (environment) {
        connect(environment, &QQuick3DSceneEnvironment::backgroundModeChanged, this, &QQuick3DXrView::handleClearColorChanged);
        connect(environment, &QQuick3DSceneEnvironment::clearColorChanged, this, &QQuick3DXrView::handleClearColorChanged);
        connect(environment, &QQuick3DSceneEnvironment::antialiasingModeChanged, this, &QQuick3DXrView::handleAAChanged);
        connect(environment, &QQuick3DSceneEnvironment::antialiasingQualityChanged, this, &QQuick3DXrView::handleAAChanged);
    }

    emit environmentChanged(m_openXRManager.m_vrViewport->environment());
}

/*!
    \qmlproperty bool QtQuick3D.Xr::XrView::isPassthroughSupported
    \brief Indicates whether passthrough is supported for the XR view.
*/

bool QQuick3DXrView::isPassthroughSupported() const
{
    if (!m_openXRManager.isValid())
        return false;

    return m_openXRManager.supportsPassthrough();
}

void QQuick3DXrView::setEnablePassthrough(bool enable)
{
    if (!m_openXRManager.isValid()) {
        qWarning("Attempted to set passthrough mode without a valid XR manager");
        return;
    }

    // bail if passthrough is not supported
    if (enable && !m_openXRManager.supportsPassthrough()) {
        qWarning("Enabling Passthrough is not supported.");
        return;
    }

    const bool orgPassthroughEnabled = m_openXRManager.isPassthroughEnabled();
    m_openXRManager.setPassthroughEnabled(enable);

    if (orgPassthroughEnabled != m_openXRManager.isPassthroughEnabled())
        emit enablePassthroughChanged(enable);
}

/*!
    \qmlproperty enumeration QtQuick3D.Xr::XrView::fixedFoveation
    \brief Controls the level of fixed foveated rendering for the XrView.

    Foveated rendering reduces GPU load by reducing image quality (resolution)
    in areas where the difference is less perceptible to the eye. With fixed
    foveated rendering the areas with reduced visual fidelity are fixed and do
    not change. On some platforms there is no concept of, or there is no control
    over, fixed foveated rendering. For example, VisionOS-based devices perform
    dynamic, eye-tracked foveation, and thus the value of this property is
    ignored in practice. Whereas other devices, such as the Meta Quest 3, only
    have support for fixed foveation, in which case this property becomes
    relevant.

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
    return FoveationLevel(m_openXRManager.getFixedFoveationLevel());
}

void QQuick3DXrView::setFixedFoveation(FoveationLevel level)
{
    const auto orgFoviationLevel = m_openXRManager.getFixedFoveationLevel();
    m_openXRManager.setFixedFoveationLevel(QtQuick3DXr::FoveationLevel(level));
    if (orgFoviationLevel != m_openXRManager.getFixedFoveationLevel())
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
    \brief Holds rendering statistics for the XR view.
*/

QQuick3DRenderStats *QQuick3DXrView::renderStats() const
{
    return m_openXRManager.m_vrViewport ? m_openXRManager.m_vrViewport->renderStats() : nullptr;
}

void QQuick3DXrView::updateViewportGeometry()
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
            m_openXRManager.m_quickWindow->setColor(env->clearColor());
        else if (env->backgroundMode() == QQuick3DSceneEnvironment::Transparent)
            m_openXRManager.m_quickWindow->setColor(Qt::transparent);
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
    m_openXRManager.setSamples(samples);
}

bool QQuick3DXrView::init()
{
    if (m_isInitialized) {
        qWarning("Already initialized!");
        return false;
    }

    if (!m_openXRManager.isReady() && !m_openXRManager.initialize()) {
        qDebug() << "Waiting for XR platform to be initialized...";
        connect(&m_openXRManager, &QQuick3DXrManager::initialized, this, &QQuick3DXrView::init, Qt::UniqueConnection);
        return false;
    }

    if (!m_openXRManager.initialize()) {
        QString errorString = m_openXRManager.errorString();
        if (errorString.isEmpty())
            errorString = tr("Failed to initialize XR platform");
        qWarning("\n%s\n", qPrintable(errorString));
        QMetaObject::invokeMethod(this, "initializeFailed", Qt::QueuedConnection, errorString);
        return false;
    }

    // Create View3D
    QSSG_CHECK_X(m_openXRManager.m_vrViewport == nullptr, "View3D already created!");
    auto viewport = new QQuick3DViewport();
    viewport->setRenderMode(QQuick3DViewport::Underlay);
    auto contentItem = m_openXRManager.m_quickWindow->contentItem();
    viewport->setParentItem(contentItem);
    m_openXRManager.m_vrViewport = viewport;
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

    connect(&m_openXRManager, &QQuick3DXrManager::sessionEnded, this, &QQuick3DXrView::handleSessionEnded);
    connect(&m_openXRManager, &QQuick3DXrManager::frameReady, this, &QQuick3DXrView::frameReady);
    connect(&m_openXRManager, &QQuick3DXrManager::referenceSpaceChanged, this, &QQuick3DXrView::referenceSpaceChanged);

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
QQuick3DPickResult QQuick3DXrView::rayPick(const QVector3D &origin, const QVector3D &direction) const
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
QList<QQuick3DPickResult> QQuick3DXrView::rayPickAll(const QVector3D &origin, const QVector3D &direction) const
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

    This method will search for an XrItem near \a position, and send a virtual
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
    \li The distance from the plane to the touch point. Will be \c 0 if \c pressed is \c true.
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
    return ReferenceSpace(m_openXRManager.getReferenceSpace());
}

void QQuick3DXrView::setReferenceSpace(ReferenceSpace newReferenceSpace)
{
    m_openXRManager.setReferenceSpace(QtQuick3DXr::ReferenceSpace(newReferenceSpace));
}

bool QQuick3DXrView::isDepthSubmissionEnabled() const
{
    if (!m_openXRManager.isValid()) {
        qWarning("Attempted to check depth submission mode without a valid XR manager");
        return false;
    }

    return m_openXRManager.isDepthSubmissionEnabled();
}

/*!
    \qmlproperty bool QtQuick3D.Xr::XrView::multiViewRenderingSupported

    \brief This read-only property reports the availability of \l{Multiview Rendering}.

    \sa enableMultiViewRendering
 */
bool QQuick3DXrView::isMultiViewRenderingSupported() const
{
    if (!m_openXRManager.isValid())
        return false;

    return m_openXRManager.isMultiViewRenderingSupported();
}

/*!
    \qmlproperty bool QtQuick3D.Xr::XrView::enableMultiViewRendering

    \brief Gets or sets whether \l{Multiview Rendering} is enabled for the XR view.

    The default value is \c false. Changing the value to \c true has an effect
    only when \l multiViewRenderingSupported is \c true. See \l{Multiview
    Rendering} for details.

    This property can also be used to query whether multiview rendering is
    really in use at run time. When not supported, the value will flip back to
    \c false.

    \note Changing the value dynamically, while the scene is already up and
    running, is possible, but not recommended, because enabling or disabling
    multiview mode involves releasing and recreating certain graphics and XR
    resources. Depending on the platform and headset, this then may cause visual
    effects that are undesirable, for example the scene may disappear and
    reappear.

    \note Enabling multiview rendering is recommended, in general. It can
    improve performance, reduce CPU and GPU load, as well as reduce power
    consumption. It defaults to disabled in order to ensure maximum
    compatibility. Developers are encouraged to verify that their application
    renders as expected with enableMultiViewRendering set to \c true, and then
    leave it set afterwards.

    Certain Qt Quick and Quick 3D features that involve application-provided
    shader code, in particular custom 2D and 3D materials and postprocessing
    effects, require awareness from the developers in order to make the
    application-provided shader assets multiview-compatible. See \l{Multiview
    Rendering} for details.

     \sa multiViewRenderingSupported
*/
bool QQuick3DXrView::isMultiViewRenderingEnabled() const
{
    if (!m_openXRManager.isValid())
        return false;

    return m_openXRManager.isMultiViewRenderingEnabled();
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
    \qmlproperty bool QtQuick3D.Xr::XrView::enableDepthSubmission

    \brief Gets or sets whether submitting the depth buffer to the XR compositor
    is enabled.

    By default the value is \c false and the depth buffer used by the 3D scene
    in the XrView is not exposed to the XR compositor. However, in some
    platforms depth submission is implicit and cannot be disabled or controlled
    by the application. An example of this is VisionOS. Changing this property
    has no effect on those platforms. Elsewhere, with OpenXR in particular,
    support depends on the OpenXR implementation used at run time.

    It is always safe to set enableDepthSubmission to \c true. It will just have
    no effect when not support by the underlying stack. Inspect the debug output
    to see if depth submission is in use.

    Submitting the depth buffer may improve reprojections that may be performed
    by the XR compositor. This could happen for example when the system cannot
    maintain the target frame rate, and thus has to resort to predicting frame
    contents, in order to improve and stabilize the user's perception of the
    scene and reduce possibly nauseating effects. However, the application and
    Qt has no control over the usage of the data. It could also happen that
    submitting depth data has no practical effects and is simply ignored by the
    underlying XR runtime and compositor.

    In practice submitting the depth buffer implies rendering into a depth
    texture provided by the XR runtime, instead of the intermediate
    texture/renderbuffer created and managed by Qt. This has certain lower-level
    consequences that can have a performance impact:

    When \l{QtQuick3D::SceneEnvironment::antialiasingMode}{multisample
    antialiasing} (MSAA) is used, enabling depth submission implies rendering
    into a multisample depth texture and resolving the samples into the XR
    runtime provided non-multisample depth texture. Without depth submission,
    the resolve step would not be necessary at all. In addition, some 3D APIs
    have no support at all for resolving multisample depth-stencil data. (see
    the \l{QRhi::ResolveDepthStencil} flag for details) Attempts to enable depth
    submission in combination with MSAA will be gracefully ignored if this is
    the case.

    Even when MSAA is not used, enabling depth submission triggers writing out
    depth data with 3D APIs that have control over this. The store operation for
    depth/stencil data is normally indicated by Qt as not necessary, which can
    have positive effects for performance on tiled GPU architectures in
    particular. With depth submission this is not done, because depth data has
    to then be written out always from Qt's perspective.

    \note It is recommended that developers test their applications with depth
    submission enabled, evaluate the advantages and disadvantages, and make a
    conscious choice based on their testing if they wish to enable it or not.
*/

void QQuick3DXrView::setEnableDepthSubmission(bool enable)
{
    if (!m_openXRManager.isValid()) {
        qWarning("Attempted to set depth submission mode without a valid XR manager");
        return;
    }

    const bool orgDepthSubmission = m_openXRManager.isDepthSubmissionEnabled();

    m_openXRManager.setDepthSubmissionEnabled(enable);

    if (orgDepthSubmission != m_openXRManager.isDepthSubmissionEnabled())
        emit enableDepthSubmissionChanged();
}

void QQuick3DXrView::setEnableMultiViewRendering(bool enable)
{
    if (!m_openXRManager.isValid()) {
        qWarning("Attempted to set multiview rendering mode without having m_openXRManager initialized");
        return;
    }

    const bool orgMultiView = m_openXRManager.isMultiViewRenderingEnabled();
    m_openXRManager.setMultiviewRenderingEnabled(enable);

    if (orgMultiView != m_openXRManager.isMultiViewRenderingEnabled())
        emit enableMultiViewRenderingChanged();
}

/*!
    \qmlsignal XrView::initializeFailed(const QString &errorString)

    Emitted when initialization fails and there is new \a errorString
    describing the failure.
 */


/*!
    \qmlsignal XrView::sessionEnded()

    Emitted when the session ends.
 */


/*!
    \qmlsignal XrView::xrOriginChanged(QQuick3DXrOrigin* xrOrigin)

    Emitted when the XR origin changes to \a xrOrigin .
 */


/*!
    \qmlsignal XrView::environmentChanged(QQuick3DSceneEnvironment* environment)

    Emitted when the scene environment changes to \a environment .
 */


/*!
    \qmlsignal XrView::enablePassthroughChanged(bool enable)

    Emitted when passthrough mode is changed to the value of \a enable .
 */


/*!
    \qmlsignal XrView::quitOnSessionEndChanged()

    Emitted when the behavior of quitting on session end changes.
 */


/*!
    \qmlsignal XrView::fixedFoveationChanged()

    Emitted when the fixedFoveation property value changes.
 */


/*!
    \qmlsignal XrView::frameReady(QRhiTexture* colorBuffer)

    Emitted when a new frame is ready to provide \a colorBuffer.
 */


/*!
    \qmlsignal XrView::referenceSpaceChanged()

    Emitted when the referenceSpace property value changes.
 */


/*!
    \qmlsignal XrView::enableDepthSubmissionChanged()

    Emitted when the enableDepthSubmission property value changes.
 */


/*!
    \qmlsignal XrView::enableMultiViewRenderingChanged()

    Emitted when the enableMultiViewRendering property value changes.
 */

QT_END_NAMESPACE
