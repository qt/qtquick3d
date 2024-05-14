// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#import "qquick3dxrvisionosmanager_p.h"

#include "qquick3dxrvisionosrendermanager_p.h"
#include "qopenxrorigin_p.h"
#include <QtQuick3D/private/qquick3dviewport_p.h>
#include <QtQuick3D/private/qquick3dnode_p_p.h>

#include <QQuickGraphicsDevice>
#include <rhi/qrhi.h>

#include <CompositorServices/CompositorServices.h>
#include <QtGui/qguiapplication_platform.h>

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
    }

    void render(cp_layer_renderer_t renderer) override
    {
        if (m_layerRenderer != renderer) {
            m_layerRenderer = renderer;
            emit layerRendererChanged();
        }

        emit renderingRequested();
    }

    void setActive() { m_active = true; };
    bool isActive() const { return m_active; }

    cp_layer_renderer_t layerRenderer() const
    {
        return m_layerRenderer;
    }

Q_SIGNALS:
    void renderingRequested();
    void layerRendererChanged();

private:
    cp_layer_renderer_t m_layerRenderer = nullptr;
    bool m_active = false;
};

Q_GLOBAL_STATIC(CompositorLayer, s_compositorLayer)

QQuick3DXRVisionOSRenderManager::QQuick3DXRVisionOSRenderManager(QObject *parent)
    : QObject(parent)
{
    runWorldTrackingARSession();
}

QQuick3DXRVisionOSRenderManager::~QQuick3DXRVisionOSRenderManager()
{
    ar_session_stop(m_arSession);
}

bool QQuick3DXRVisionOSRenderManager::initialize()
{
    // NOTE: Check if the compository layer proxy is already active.
    if (!s_compositorLayer->isActive()) {
        if (auto *visionOSApplicaton = qGuiApp->nativeInterface<QNativeInterface::QVisionOSApplication>()) {
            visionOSApplicaton->setImmersiveSpaceCompositorLayer(&(*s_compositorLayer));
            s_compositorLayer->setActive();
            // FIXME: We don't actually handle the case where the rendere changes or we get multiple calls should do something.
            connect(s_compositorLayer, &CompositorLayer::renderingRequested, this, &QQuick3DXRVisionOSRenderManager::initialized, Qt::ConnectionType(Qt::SingleShotConnection | Qt::QueuedConnection));
        }
        return s_compositorLayer->layerRenderer() == nullptr ? false : true;
    }

    cp_layer_renderer_t renderer = layerRenderer();
    if (!renderer) {
        qWarning("QQuick3DXRVisionOSRenderManager: Layer renderer is not available.");
        return false;
    }

    // Pre-setup Qt Quick

    // Setup Graphics

    return true;
}

void QQuick3DXRVisionOSRenderManager::setupWindow(QQuickWindow *window)
{
    if (!window) {
        qWarning("QQuick3DXRVisionOSRenderManager: Window is null!");
        return;
    }

    cp_layer_renderer_t renderer = layerRenderer();
    if (!renderer) {
        qWarning("QQuick3DXRVisionOSRenderManager: Layer renderer is not available.");
        return;
    }

    auto device = cp_layer_renderer_get_device(renderer);
    auto commandQueue = [device newCommandQueue];

    auto qqGraphicsDevice = QQuickGraphicsDevice::fromDeviceAndCommandQueue(static_cast<MTLDevice*>(device), static_cast<MTLCommandQueue *>(commandQueue));

    window->setGraphicsDevice(qqGraphicsDevice);
}

bool QQuick3DXRVisionOSRenderManager::finalizeGraphics(QRhi *rhi)
{
    Q_UNUSED(rhi);
    return true;
}

bool QQuick3DXRVisionOSRenderManager::isReady() const
{
    return (s_compositorLayer->layerRenderer() != nullptr);
}

void QQuick3DXRVisionOSRenderManager::createSwapchains()
{
    // cp_layer_renderer_t renderer = getLayerRenderer();
    // if (!renderer) {
    //     qWarning("QQuick3DXRVisionOSRenderManager: Layer renderer is not available.");
    //     return;
    // }

    //cp_layer_renderer_configuration_t layerConfiguration = cp_layer_renderer_get_configuration(renderer);
    //cp_layer_renderer_layout layout = cp_layer_renderer_configuration_get_layout(layerConfiguration);
}

QQuick3DXRVisionOSRenderManager::RenderState QQuick3DXRVisionOSRenderManager::getRenderState()
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

void QQuick3DXRVisionOSRenderManager::teardown()
{

}

cp_layer_renderer_t QQuick3DXRVisionOSRenderManager::layerRenderer() const
{
    return s_compositorLayer->layerRenderer();
}

ar_device_anchor_t QQuick3DXRVisionOSRenderManager::createPoseForTiming(cp_frame_timing_t timing)
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

void QQuick3DXRVisionOSRenderManager::runWorldTrackingARSession()
{
    ar_world_tracking_configuration_t worldTrackingConfiguration = ar_world_tracking_configuration_create();
    m_worldTrackingProvider = ar_world_tracking_provider_create(worldTrackingConfiguration);

    ar_data_providers_t dataProviders = ar_data_providers_create();
    ar_data_providers_add_data_provider(dataProviders, m_worldTrackingProvider);

    m_isHandTrackingSupported = ar_hand_tracking_provider_is_supported();
    if (m_isHandTrackingSupported) {
        ar_hand_tracking_configuration_t handTrackingConfiguration = ar_hand_tracking_configuration_create();
        m_handTrackingProvider = ar_hand_tracking_provider_create(handTrackingConfiguration);
        ar_data_providers_add_data_provider(dataProviders, m_handTrackingProvider);
    } else {
        qWarning("Hand tracking is not supported on this device.");
    }


    m_arSession = ar_session_create();
    ar_session_run(m_arSession, dataProviders);

    // Create hand anchors now
    if (m_isHandTrackingSupported) {
        m_leftHandAnchor = ar_hand_anchor_create();
        m_rightHandAnchor = ar_hand_anchor_create();
    }
}

void QQuick3DXRVisionOSRenderManager::renderFrame(QQuickWindow *quickWindow, QQuickRenderControl *renderControl, QOpenXROrigin *xrOrigin, QQuick3DViewport *xrViewport)
{
    auto layerRenderer = this->layerRenderer();
    cp_frame_t frame = cp_layer_renderer_query_next_frame(layerRenderer);
    if (frame == nullptr) {
        return;
    }

    cp_frame_timing_t timing = cp_frame_predict_timing(frame);
    if (timing == nullptr)
        return;

    cp_frame_start_update(frame);

    // TODO do input update here

    cp_frame_end_update(frame);

    cp_time_wait_until(cp_frame_timing_get_optimal_input_time(timing));

    cp_frame_start_submission(frame);
    cp_drawable_t drawable = cp_frame_query_drawable(frame);
    if (drawable == nullptr)
        return;

    cp_frame_timing_t actualTiming = cp_drawable_get_frame_timing(drawable);
    ar_device_anchor_t anchor = createPoseForTiming(actualTiming);
    cp_drawable_set_device_anchor(drawable, anchor);

    // Get the pose transform from the anchor
    simd_float4x4 headTransform = ar_anchor_get_origin_from_anchor_transform(anchor);

    // Update the hands
    if (m_isHandTrackingSupported) {
        ar_hand_tracking_provider_get_latest_anchors(m_handTrackingProvider, m_leftHandAnchor, m_rightHandAnchor);

        if (ar_trackable_anchor_is_tracked(m_leftHandAnchor)) {

        }

        if (ar_trackable_anchor_is_tracked(m_rightHandAnchor)) {

        }
    }

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
        if (xrOrigin) {
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

#include "qquick3dxrvisionosrendermanager.moc"
