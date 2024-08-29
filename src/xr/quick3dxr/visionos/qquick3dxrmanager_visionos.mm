// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qquick3dxrmanager_visionos_p.h"
#include "qquick3dxrorigin_p.h"
#include "qquick3dxrmanager_p.h"
#include "qquick3dxrinputmanager_visionos_p.h"
#include "qquick3dxranchormanager_visionos_p.h"

#include "../qquick3dxrinputmanager_p.h"
#include "../qquick3dxranimationdriver_p.h"

#include <QtQuick3D/private/qquick3dviewport_p.h>
#include <QtQuick3D/private/qquick3dnode_p_p.h>

#include <QtQuick3DUtils/private/qssgassert_p.h>

#include <QQuickGraphicsDevice>
#include <rhi/qrhi.h>

#include <CompositorServices/CompositorServices.h>
#include <QtGui/qguiapplication_platform.h>

#include <QtCore/qoperatingsystemversion.h>

QT_BEGIN_NAMESPACE

class CompositorLayer : public QObject, public QNativeInterface::QVisionOSApplication::ImmersiveSpaceCompositorLayer
{
    Q_OBJECT
public:
    void configure(cp_layer_renderer_capabilities_t capabilities, cp_layer_renderer_configuration_t configuration) const override
    {
        // NOTE: foveation is disabled for now
        const bool supportsFoveation = false && cp_layer_renderer_capabilities_supports_foveation(capabilities);

        cp_layer_renderer_configuration_set_layout(configuration, cp_layer_renderer_layout_dedicated);
        cp_layer_renderer_configuration_set_foveation_enabled(configuration, supportsFoveation);
        cp_layer_renderer_configuration_set_color_format(configuration, MTLPixelFormatRGBA16Float);
        simd_float2 depthRange = cp_layer_renderer_configuration_get_default_depth_range(configuration);
        // NOTE: The depth range is inverted for VisionOS (x = far, y = near)
        m_depthRange[0] = depthRange.y;
        m_depthRange[1] = depthRange.x;
    }

    void render(cp_layer_renderer_t renderer) override
    {
        if (m_layerRenderer != renderer) {
            m_layerRenderer = renderer;
            emit layerRendererChanged();
        }

        emit renderingRequested();
    }

    void handleSpatialEvents(const QJsonObject &events) override
    {
        emit handleSpatialEventsRequested(events);
    }

    void setActive() { m_active = true; };
    bool isActive() const { return m_active; }

    cp_layer_renderer_t layerRenderer() const
    {
        return m_layerRenderer;
    }

    void getDefaultDepthRange(float &near, float &far) const
    {
        near = m_depthRange[0];
        far = m_depthRange[1];
    }

Q_SIGNALS:
    void renderingRequested();
    void layerRendererChanged();
    void handleSpatialEventsRequested(const QJsonObject &jsonString);

private:
    cp_layer_renderer_t m_layerRenderer = nullptr;
    mutable float m_depthRange[2] {1.0f, 10000.0f}; // NOTE: Near, Far
    bool m_active = false;
};

Q_GLOBAL_STATIC(CompositorLayer, s_compositorLayer)

QQuick3DXrManagerPrivate::QQuick3DXrManagerPrivate(QQuick3DXrManager &manager)
    : q_ptr(&manager)
{
}

QQuick3DXrManagerPrivate::~QQuick3DXrManagerPrivate()
{
    ar_session_stop(m_arSession);
}

QQuick3DXrManagerPrivate *QQuick3DXrManagerPrivate::get(QQuick3DXrManager *manager)
{
    QSSG_ASSERT(manager != nullptr, return nullptr);
    return manager->d_func();
}

bool QQuick3DXrManagerPrivate::initialize()
{
    Q_Q(QQuick3DXrManager);

    if (!m_worldTrackingProvider)
        runWorldTrackingARSession();

    // NOTE: Check if the compository layer proxy is already active.
    if (!s_compositorLayer->isActive()) {
        if (auto *visionOSApplicaton = qGuiApp->nativeInterface<QNativeInterface::QVisionOSApplication>()) {
            visionOSApplicaton->setImmersiveSpaceCompositorLayer(&(*s_compositorLayer));
            s_compositorLayer->setActive();
            // FIXME: We don't actually handle the case where the rendere changes or we get multiple calls should do something.

            QObject::connect(s_compositorLayer, &CompositorLayer::renderingRequested, q, &QQuick3DXrManager::initialized, Qt::ConnectionType(Qt::SingleShotConnection | Qt::QueuedConnection));

            // Listen for spatial events (these are native gestures like pinch click/drag coming from SwiftUI)
            QObject::connect(s_compositorLayer, &CompositorLayer::handleSpatialEventsRequested, q, &QQuick3DXrManager::processSpatialEvents);
        }
        return s_compositorLayer->layerRenderer() == nullptr ? false : true;
    }

    cp_layer_renderer_t renderer = layerRenderer();

    if (!renderer) {
        qWarning("QQuick3DXrManagerPrivate: Layer renderer is not available.");
        return false;
    }

    return true;
}

void QQuick3DXrManagerPrivate::setupWindow(QQuickWindow *window)
{
    if (!window) {
        qWarning("QQuick3DXrManagerPrivate: Window is null!");
        return;
    }

    cp_layer_renderer_t renderer = layerRenderer();
    if (!renderer) {
        qWarning("QQuick3DXrManagerPrivate: Layer renderer is not available.");
        return;
    }

    auto device = cp_layer_renderer_get_device(renderer);
    auto commandQueue = [device newCommandQueue];

    auto qqGraphicsDevice = QQuickGraphicsDevice::fromDeviceAndCommandQueue(static_cast<MTLDevice*>(device), static_cast<MTLCommandQueue *>(commandQueue));

    window->setGraphicsDevice(qqGraphicsDevice);
}

bool QQuick3DXrManagerPrivate::finalizeGraphics(QRhi *rhi)
{
    Q_UNUSED(rhi);
    m_isGraphicsInitialized = true;
    return m_isGraphicsInitialized;
}

bool QQuick3DXrManagerPrivate::isReady() const
{
    return (s_compositorLayer->layerRenderer() != nullptr);
}

bool QQuick3DXrManagerPrivate::isGraphicsInitialized() const
{
    return m_isGraphicsInitialized;
}

bool QQuick3DXrManagerPrivate::setupGraphics(QQuickWindow *window)
{
    // FIXME:
    Q_UNUSED(window);
    Q_UNIMPLEMENTED(); qWarning() << Q_FUNC_INFO;
    return true;
}

void QQuick3DXrManagerPrivate::createSwapchains()
{
    // cp_layer_renderer_t renderer = getLayerRenderer();
    // if (!renderer) {
    //     qWarning("QQuick3DXrManagerPrivate: Layer renderer is not available.");
    //     return;
    // }

    //cp_layer_renderer_configuration_t layerConfiguration = cp_layer_renderer_get_configuration(renderer);
    //cp_layer_renderer_layout layout = cp_layer_renderer_configuration_get_layout(layerConfiguration);
}

QQuick3DXrManagerPrivate::RenderState QQuick3DXrManagerPrivate::getRenderState()
{
    if (!isReady())
        return RenderState::Paused;

    cp_layer_renderer_t renderer = layerRenderer();
    switch (cp_layer_renderer_get_state(renderer)) {
        case cp_layer_renderer_state_paused:
            return RenderState::Paused;
        case cp_layer_renderer_state_running:
            return RenderState::Running;
        case cp_layer_renderer_state_invalidated:
            return RenderState::Invalidated;
    }
    return RenderState::Invalidated;
}

void QQuick3DXrManagerPrivate::getDefaultClipDistances(float &nearClip, float &farClip) const
{
    s_compositorLayer->getDefaultDepthRange(nearClip, farClip);
}

void QQuick3DXrManagerPrivate::teardown()
{
    Q_UNIMPLEMENTED(); qWarning() << Q_FUNC_INFO;
}

void QQuick3DXrManagerPrivate::setMultiViewRenderingEnabled(bool enable)
{
    Q_UNUSED(enable);
    Q_UNIMPLEMENTED(); qWarning() << Q_FUNC_INFO;
}

void QQuick3DXrManagerPrivate::setPassthroughEnabled(bool enable)
{
    Q_UNUSED(enable);
    Q_UNIMPLEMENTED(); qWarning() << Q_FUNC_INFO;
}

QtQuick3DXr::ReferenceSpace QQuick3DXrManagerPrivate::getReferenceSpace() const
{
    // FIXME: Not sure exactly what reference space is default or what is supported etc.
    return QtQuick3DXr::ReferenceSpace::ReferenceSpaceLocalFloor;
}

void QQuick3DXrManagerPrivate::setReferenceSpace(QtQuick3DXr::ReferenceSpace newReferenceSpace)
{
    // FIXME: Not sure if it's possible to set a reference space on VisionOS
    Q_UNUSED(newReferenceSpace);
    Q_UNIMPLEMENTED(); qWarning() << Q_FUNC_INFO;
}

void QQuick3DXrManagerPrivate::setDepthSubmissionEnabled(bool enable)
{
    Q_UNUSED(enable);
    if (!enable)
        qWarning("Depth submission is required on VisionOS");
}

cp_layer_renderer_t QQuick3DXrManagerPrivate::layerRenderer() const
{
    return s_compositorLayer->layerRenderer();
}

ar_device_anchor_t QQuick3DXrManagerPrivate::createPoseForTiming(cp_frame_timing_t timing)
{
    ar_device_anchor_t outAnchor = ar_device_anchor_create();
    cp_time_t presentationTime = cp_frame_timing_get_presentation_time(timing);
    CFTimeInterval queryTime = cp_time_to_cf_time_interval(presentationTime);
    ar_device_anchor_query_status_t status = ar_world_tracking_provider_query_device_anchor_at_timestamp(m_worldTrackingProvider, queryTime, outAnchor);
    if (status != ar_device_anchor_query_status_success) {
        NSLog(@"Failed to get estimated pose from world tracking provider for presentation timestamp %0.3f", queryTime);
    }
    return outAnchor;
}

void QQuick3DXrManagerPrivate::processXrEvents()
{
    Q_Q(QQuick3DXrManager);

    enum RenderState : quint8 {
        Paused,
        Running,
        Invalidated
    };
    static bool logOnce[3] = {false, false, false};
    QQuick3DXrManagerPrivate::RenderState renderState = getRenderState();
    if (renderState == QQuick3DXrManagerPrivate::RenderState::Paused) {
        // Wait
        if (!logOnce[RenderState::Paused]) {
            qDebug() << "-- Wait --";
            logOnce[RenderState::Paused] = true;
            logOnce[RenderState::Running] = false;
            logOnce[RenderState::Invalidated] = false;
        }
    } else if (renderState == QQuick3DXrManagerPrivate::RenderState::Running) {
        q->renderFrame();
        if (!logOnce[RenderState::Running]) {
            qDebug() << "-- Running --";
            logOnce[RenderState::Paused] = false;
            logOnce[RenderState::Running] = true;
            logOnce[RenderState::Invalidated] = false;
        }
    } else if (renderState == QQuick3DXrManagerPrivate::RenderState::Invalidated) {
        if (!logOnce[RenderState::Invalidated]) {
            qDebug() << "-- Invalidated --";
            logOnce[RenderState::Paused] = false;
            logOnce[RenderState::Running] = false;
            logOnce[RenderState::Invalidated] = true;
        }
        emit q->sessionEnded();
    }
}

void QQuick3DXrManagerPrivate::runWorldTrackingARSession()
{
    ar_world_tracking_configuration_t worldTrackingConfiguration = ar_world_tracking_configuration_create();
    m_worldTrackingProvider = ar_world_tracking_provider_create(worldTrackingConfiguration);

    ar_data_providers_t dataProviders = ar_data_providers_create();
    ar_data_providers_add_data_provider(dataProviders, m_worldTrackingProvider);

    if (!m_inputManager)
        m_inputManager = QQuick3DXrInputManager::instance();

    if (!m_anchorManager)
        m_anchorManager = QQuick3DXrAnchorManager::instance();

    // 1. prepare
    QQuick3DXrInputManagerPrivate *pim = nullptr;
    if (QSSG_GUARD_X(m_inputManager != nullptr, "No InputManager available!")) {
        pim = QQuick3DXrInputManagerPrivate::get(m_inputManager);
        if (QSSG_GUARD(pim != nullptr))
            pim->prepareHandtracking(dataProviders);
    }

    if (QSSG_GUARD_X(m_anchorManager != nullptr, "No AnchorManager available!"))
        m_anchorManager->prepareAnchorManager(dataProviders);

    m_arSession = ar_session_create();
    ar_session_run(m_arSession, dataProviders);

    // 2. initialize
    if (pim)
        pim->initHandtracking();

    if (m_anchorManager != nullptr)
        m_anchorManager->initAnchorManager();
}

void QQuick3DXrManagerPrivate::setSamples(int samples)
{
    Q_UNUSED(samples);
    qWarning("Setting samples is not supported");
}

QString QQuick3DXrManagerPrivate::runtimeName() const
{
    return QStringLiteral("VisionOS");
}

QVersionNumber QQuick3DXrManagerPrivate::runtimeVersion() const
{
    static const auto versionNumber = QOperatingSystemVersion::current().version();
    return versionNumber;
}

QString QQuick3DXrManagerPrivate::errorString() const
{
    return QString();
}

void QQuick3DXrManagerPrivate::doRenderFrame()
{
    Q_Q(QQuick3DXrManager);

    QQuickWindow *quickWindow = q->m_quickWindow;
    QQuickRenderControl *renderControl = q->m_renderControl;
    QQuick3DXrOrigin *xrOrigin = q->m_xrOrigin;
    QQuick3DViewport *xrViewport = q->m_vrViewport;
    QQuick3DXrAnimationDriver *animationDriver = q->m_animationDriver;

    QSSG_ASSERT_X(quickWindow && renderControl && xrViewport && xrOrigin && animationDriver, "Invalid state, rendering aborted", return);

    auto layerRenderer = this->layerRenderer();
    cp_frame_t frame = cp_layer_renderer_query_next_frame(layerRenderer);
    if (frame == nullptr) {
        qWarning("Failed to get next frame");
        return;
    }

    cp_frame_timing_t timing = cp_frame_predict_timing(frame);
    if (timing == nullptr) {
        qWarning("Failed to get timing for frame");
        return;
    }

    cp_frame_start_update(frame);

    cp_frame_end_update(frame);

    cp_time_t optimalInputTime = cp_frame_timing_get_optimal_input_time(timing);
    cp_time_wait_until(optimalInputTime);

    cp_frame_start_submission(frame);
    cp_drawable_t drawable = cp_frame_query_drawable(frame);
    if (drawable == nullptr) {
        qWarning("Failed to get drawable for frame");
        return;
    }

    cp_frame_timing_t actualTiming = cp_drawable_get_frame_timing(drawable);
    ar_device_anchor_t anchor = createPoseForTiming(actualTiming);
    cp_drawable_set_device_anchor(drawable, anchor);

    // Get the pose transform from the anchor
    simd_float4x4 headTransform = ar_anchor_get_origin_from_anchor_transform(anchor);

    // NOTE: We need to convert from meters to centimeters here
    QMatrix4x4 qtHeadTransform{headTransform.columns[0].x, headTransform.columns[1].x, headTransform.columns[2].x, headTransform.columns[3].x * 100,
                                 headTransform.columns[0].y, headTransform.columns[1].y, headTransform.columns[2].y, headTransform.columns[3].y * 100,
                                 headTransform.columns[0].z, headTransform.columns[1].z, headTransform.columns[2].z, headTransform.columns[3].z * 100,
                                 0.0f, 0.0f, 0.0f, 1.0f};
    xrOrigin->updateTrackedCamera(qtHeadTransform);

    // Update the hands
    if (QSSG_GUARD(m_inputManager != nullptr))
        QQuick3DXrInputManagerPrivate::get(m_inputManager)->updateHandtracking();

    // Animation driver
    // Convert the cp_frame_timing_t ticks to milliseconds
    const qint64 displayPeriodMS = qint64(cp_time_to_cf_time_interval(optimalInputTime) * 1000.0);
    const qint64 displayDeltaMS = ((qint64(cp_time_to_cf_time_interval(cp_frame_timing_get_optimal_input_time(actualTiming)) * 1000.0)) - m_previousTime);

    if (m_previousTime == 0)
        animationDriver->setStep(displayPeriodMS);
    else {
        if (displayDeltaMS > displayPeriodMS)
            animationDriver->setStep(displayPeriodMS);
        else
            animationDriver->setStep(displayDeltaMS);
        animationDriver->advance();
    }
    m_previousTime = displayPeriodMS;

    QRhi *rhi = renderControl->rhi();

    for (size_t i = 0, end = cp_drawable_get_view_count(drawable); i != end ; ++i) {
        // Setup the RenderTarget based on the current drawable
        id<MTLTexture> colorMetalTexture = cp_drawable_get_color_texture(drawable, i);
        auto textureSize = QSize([colorMetalTexture width], [colorMetalTexture height]);
        auto renderTarget = QQuickRenderTarget::fromMetalTexture(static_cast<MTLTexture*>(colorMetalTexture), [colorMetalTexture pixelFormat], textureSize);

        auto depthMetalTexture = cp_drawable_get_depth_texture(drawable, i);
        auto depthTextureSize = QSize([depthMetalTexture width], [depthMetalTexture height]);
        MTLPixelFormat depthTextureFormat = [depthMetalTexture pixelFormat];
        static const auto convertFormat = [](MTLPixelFormat format) -> QRhiTexture::Format {
            switch (format) {
            case MTLPixelFormatDepth16Unorm:
                return QRhiTexture::D16;
            case MTLPixelFormatDepth32Float:
                return QRhiTexture::D32F;
            default:
                qWarning("Unsupported depth texture format");
                return QRhiTexture::UnknownFormat;
            }
        };
        auto depthFormat = convertFormat(depthTextureFormat);
        if (depthFormat != QRhiTexture::UnknownFormat) {
            if (m_rhiDepthTexture && (m_rhiDepthTexture->format() != depthFormat || m_rhiDepthTexture->pixelSize() != depthTextureSize)) {
                delete m_rhiDepthTexture;
                m_rhiDepthTexture = nullptr;
            }

            if (!m_rhiDepthTexture)
                m_rhiDepthTexture = rhi->newTexture(depthFormat, depthTextureSize, 1, QRhiTexture::RenderTarget);


            m_rhiDepthTexture->createFrom({ quint64(static_cast<MTLTexture*>(depthMetalTexture)), 0});
            renderTarget.setDepthTexture(m_rhiDepthTexture);
        }

        quickWindow->setRenderTarget(renderTarget);

        // Update the window size and content item size using the texture size
        quickWindow->setGeometry(0,
                                0,
                                textureSize.width(),
                                textureSize.height());
        quickWindow->contentItem()->setSize(QSizeF(textureSize.width(),
                                                    textureSize.height()));

        // Update the camera pose
        if (QSSG_GUARD(xrOrigin)) {
            cp_view_t view = cp_drawable_get_view(drawable, i);
            simd_float4 tangents = cp_view_get_tangents(view);
            const float tangentLeft = tangents[0];
            const float tangentRight = tangents[1];
            const float tangentUp = tangents[2];
            const float tangentDown = tangents[3];
            //qDebug() << "Left: " << tangentLeft << " Right: " << tangentRight << " Up: " << tangentUp << " Down: " << tangentDown;
            simd_float2 depth_range = cp_drawable_get_depth_range(drawable);
            const float clipNear = depth_range[1];
            const float clipFar = depth_range[0];
            //qDebug() << "Near: " << clipNear << " Far: " << clipFar;
            xrOrigin->eyeCamera(i)->setLeftTangent(tangentLeft);
            xrOrigin->eyeCamera(i)->setRightTangent(tangentRight);
            xrOrigin->eyeCamera(i)->setUpTangent(tangentUp);
            xrOrigin->eyeCamera(i)->setDownTangent(tangentDown);
            xrOrigin->eyeCamera(i)->setClipNear(clipNear);
            xrOrigin->eyeCamera(i)->setClipFar(clipFar);

            simd_float4x4 localEyeTransform = cp_view_get_transform(view);
            simd_float4x4 eyeCameraTransform = simd_mul(headTransform, localEyeTransform);
            // NOTE: We need to convert from meters to centimeters here
            QMatrix4x4 transform{eyeCameraTransform.columns[0].x, eyeCameraTransform.columns[1].x, eyeCameraTransform.columns[2].x, eyeCameraTransform.columns[3].x * 100,
                                 eyeCameraTransform.columns[0].y, eyeCameraTransform.columns[1].y, eyeCameraTransform.columns[2].y, eyeCameraTransform.columns[3].y * 100,
                                 eyeCameraTransform.columns[0].z, eyeCameraTransform.columns[1].z, eyeCameraTransform.columns[2].z, eyeCameraTransform.columns[3].z * 100,
                                 0.0f, 0.0f, 0.0f, 1.0f};
            QQuick3DNodePrivate::get(xrOrigin->eyeCamera(i))->setLocalTransform(transform);
            xrViewport->setCamera(xrOrigin->eyeCamera(i));
        }

        renderControl->polishItems();
        renderControl->beginFrame();
        renderControl->sync();
        renderControl->render();
        renderControl->endFrame();
    }

    id<MTLCommandBuffer> commandBuffer = [static_cast<const QRhiMetalNativeHandles*>(renderControl->rhi()->nativeHandles())->cmdQueue commandBuffer];

    cp_drawable_encode_present(drawable, commandBuffer);
    [commandBuffer commit];

    cp_frame_end_submission(frame);
}

QT_END_NAMESPACE

#include "qquick3dxrmanager_visionos.moc"
