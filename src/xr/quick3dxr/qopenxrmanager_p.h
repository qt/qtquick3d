// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QOPENXRMANAGER_H
#define QOPENXRMANAGER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/QObject>
#include <QtCore/QMap>
#include <QtCore/QVersionNumber>
#if defined(Q_NO_TEMPORARY_DISABLE_XR_API)
#include <QtQuick3DXr/private/qabstractopenxrgraphics_p.h>
#endif // Q_NO_TEMPORARY_DISABLE_XR_API
#include <QtQuick3DXr/private/qopenxranimationdriver_p.h>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlComponent>

#include <QtGui/private/qtgui-config_p.h>
#if QT_CONFIG(graphicsframecapture)
#include <QtGui/private/qgraphicsframecapture_p.h>
#endif

#ifdef XR_USE_PLATFORM_ANDROID
# include <QtCore/QJniObject>
#endif

#if defined(Q_OS_VISIONOS)
# include <QtQuick3DXr/private/qquick3dxrvisionosrendermanager_p.h>
#endif

QT_BEGIN_NAMESPACE

class QQuickWindow;
class QQuickRenderControl;
class QQuick3DNode;
class QQuick3DViewport;
class QQuick3DXrEyeCamera;
class QOpenXRView;
class QQuick3DXrOrigin;
class QOpenXRInputManager;
class QOpenXRSpaceExtension;
class QRhiTexture;

class QOpenXRManager : public QObject
{
    Q_OBJECT
public:
    explicit QOpenXRManager(QObject *parent = nullptr);
    ~QOpenXRManager();

    bool isReady() const;

    bool initialize();
    void teardown();

    bool isValid() const;

    void setPassthroughEnabled(bool enabled);

    QString errorString() const { return m_errorString; }

    void setSamples(int samples);

private Q_SLOTS:
    void update();

Q_SIGNALS:
    void initialized();
    void sessionEnded();
    void xrOriginChanged();
    void frameReady(QRhiTexture *colorBuffer);
    void referenceSpaceChanged();

protected:
    bool event(QEvent *e) override;

private:
#if defined(Q_NO_TEMPORARY_DISABLE_XR_API)
    void destroySwapchain();
    void setErrorString(XrResult result, const char *callName);
    void checkXrExtensions(const char* layerName, int indent = 0);
    void checkXrLayers();

    XrResult createXrInstance();
    void checkXrInstance();

    void setupDebugMessenger();

    XrResult initializeSystem();

    void checkViewConfiguration();
    bool checkXrResult(const XrResult &result);
    void checkEnvironmentBlendMode(XrViewConfigurationType type);
#endif // Q_NO_TEMPORARY_DISABLE_XR_API
    bool setupGraphics();

#if defined(Q_NO_TEMPORARY_DISABLE_XR_API)
    void checkReferenceSpaces();
    bool isReferenceSpaceAvailable(XrReferenceSpaceType type);

    bool setupAppSpace();
    void updateAppSpace(XrTime predictedDisplayTime);
    bool setupViewSpace();
    bool resetEmulatedFloorHeight(XrTime predictedDisplayTime);

    void createSwapchains();
#endif // Q_NO_TEMPORARY_DISABLE_XR_API

    void processXrEvents();
#if defined(Q_NO_TEMPORARY_DISABLE_XR_API)
    void pollEvents(bool *exitRenderLoop, bool *requestRestart);
    void pollActions();
#endif // Q_NO_TEMPORARY_DISABLE_XR_API
    void renderFrame();
#if defined(Q_NO_TEMPORARY_DISABLE_XR_API)
    bool renderLayer(XrTime predictedDisplayTime,
                     XrDuration predictedDisplayPeriod,
                     XrCompositionLayerProjection &layer);
    void doRender(const XrSwapchainSubImage &subImage,
                  const XrSwapchainImageBaseHeader *swapchainImage,
                  const XrSwapchainImageBaseHeader *depthSwapchainImage = nullptr);
#endif // Q_NO_TEMPORARY_DISABLE_XR_API

    void preSetupQuickScene();
    bool setupQuickScene();
#if defined(Q_NO_TEMPORARY_DISABLE_XR_API)
    void updateCameraHelper(QQuick3DXrEyeCamera *camera, const XrCompositionLayerProjectionView &layerView);
    void updateCameraNonMultiview(int eye, const XrCompositionLayerProjectionView &layerView);
    void updateCameraMultiview(int projectionLayerViewStartIndex, int count);
#endif // Q_NO_TEMPORARY_DISABLE_XR_API
    void checkOrigin();

    bool supportsPassthrough() const;

#if defined(Q_NO_TEMPORARY_DISABLE_XR_API)
    void setupMetaQuestColorSpaces();
    void setupMetaQuestRefreshRates();
    void setupMetaQuestFoveation();

    // Passthrough
    XrPassthroughFB m_passthroughFeature{XR_NULL_HANDLE};
    void createMetaQuestPassthrough();
    void destroyMetaQuestPassthrough();
    void startMetaQuestPassthrough();
    void pauseMetaQuestPassthrough();

    XrPassthroughLayerFB m_passthroughLayer{XR_NULL_HANDLE};
    void createMetaQuestPassthroughLayer();
    void destroyMetaQuestPassthroughLayer();
    void pauseMetaQuestPassthroughLayer();
    void resumeMetaQuestPassthroughLayer();

    XrInstance m_instance{XR_NULL_HANDLE};
    XrSession m_session{XR_NULL_HANDLE};
    XrSpace m_appSpace{XR_NULL_HANDLE};
    XrReferenceSpaceType m_requestedReferenceSpace = XR_REFERENCE_SPACE_TYPE_STAGE;
    XrReferenceSpaceType m_referenceSpace = XR_REFERENCE_SPACE_TYPE_LOCAL;
    bool m_isEmulatingLocalFloor = false;
    bool m_isFloorResetPending = false;
    XrSpace m_viewSpace{XR_NULL_HANDLE};
    XrFormFactor m_formFactor{XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY};
    XrViewConfigurationType m_viewConfigType{XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO};
    XrEnvironmentBlendMode m_environmentBlendMode{XR_ENVIRONMENT_BLEND_MODE_OPAQUE};
    XrSystemId m_systemId{XR_NULL_SYSTEM_ID};

    QVector<XrReferenceSpaceType> m_availableReferenceSpace;

#ifdef XR_EXT_debug_utils
    XrDebugUtilsMessengerEXT m_debugMessenger = XR_NULL_HANDLE;
    PFN_xrDestroyDebugUtilsMessengerEXT m_xrDestroyDebugUtilsMessengerEXT = nullptr;
#endif

    QAbstractOpenXRGraphics *m_graphics = nullptr;
#endif // Q_NO_TEMPORARY_DISABLE_XR_API

    QQuickWindow *m_quickWindow = nullptr;
    QQuickRenderControl *m_renderControl = nullptr;
    QQuick3DViewport *m_vrViewport = nullptr;
    QQuick3DXrOrigin *m_xrOrigin = nullptr;
#if defined(Q_NO_TEMPORARY_DISABLE_XR_API)
    QOpenXRInputManager *m_inputManager = nullptr;
#endif // Q_NO_TEMPORARY_DISABLE_XR_API

    QOpenXRAnimationDriver *m_animationDriver = nullptr;
#if defined(Q_NO_TEMPORARY_DISABLE_XR_API)
    XrTime m_previousTime = 0;

    struct Swapchain {
        XrSwapchain handle;
        quint32 width;
        quint32 height;
        quint32 arraySize;
    };
#endif // Q_NO_TEMPORARY_DISABLE_XR_API

    bool m_multiviewRendering = false;
#if defined(Q_NO_TEMPORARY_DISABLE_XR_API)
    QVector<XrViewConfigurationView> m_configViews;
    QVector<XrCompositionLayerProjectionView> m_projectionLayerViews;
    QVector<XrCompositionLayerDepthInfoKHR> m_layerDepthInfos;
    QVector<Swapchain> m_swapchains;
    QVector<Swapchain> m_depthSwapchains;
    QMap<XrSwapchain, QVector<XrSwapchainImageBaseHeader*>> m_swapchainImages;
    QMap<XrSwapchain, QVector<XrSwapchainImageBaseHeader*>> m_depthSwapchainImages;
    QVector<XrView> m_views;
#endif // Q_NO_TEMPORARY_DISABLE_XR_API
    int64_t m_colorSwapchainFormat = -1;
    int64_t m_depthSwapchainFormat = -1;
    int m_samples = 1;

#if defined(Q_NO_TEMPORARY_DISABLE_XR_API)
    // Application's current lifecycle state according to the runtime
    XrSessionState m_sessionState{XR_SESSION_STATE_UNKNOWN};
#endif // Q_NO_TEMPORARY_DISABLE_XR_API
    bool m_sessionRunning{false};

#if defined(Q_NO_TEMPORARY_DISABLE_XR_API)
    XrEventDataBuffer m_eventDataBuffer;
#endif // Q_NO_TEMPORARY_DISABLE_XR_API

    bool m_enablePassthrough = false;
    bool m_passthroughSupported = false;
#if defined(Q_NO_TEMPORARY_DISABLE_XR_API)
    bool m_spaceExtensionSupported = false;
    QOpenXRSpaceExtension *m_spaceExtension = nullptr;
    bool m_colorspaceExtensionSupported = false;
    bool m_displayRefreshRateExtensionSupported = false;
    bool m_foveationExtensionSupported = false;
    XrFoveationLevelFB m_foveationLevel = XR_FOVEATION_LEVEL_HIGH_FB;
    bool m_compositionLayerDepthSupported = false;
    bool m_submitLayerDepth = false;
    bool m_handtrackingExtensionSupported = false;
    bool m_handtrackingAimExtensionSupported = false;

#ifdef XR_USE_PLATFORM_ANDROID
    QJniObject m_androidActivity;
#endif // XR_USE_PLATFORM_ANDROID
#endif // Q_NO_TEMPORARY_DISABLE_XR_API

    QString m_errorString;
    QString m_runtimeName;
    QVersionNumber m_runtimeVersion;
    QStringList m_enabledApiLayers;
    QStringList m_enabledExtensions;

#if defined(Q_NO_TEMPORARY_DISABLE_XR_API)
#if QT_CONFIG(graphicsframecapture)
    std::unique_ptr<QGraphicsFrameCapture> m_frameCapture;
#endif
#endif // Q_NO_TEMPORARY_DISABLE_XR_API

#if defined(Q_OS_VISIONOS)
    QQuick3DXRVisionOSRenderManager *m_visionOSRenderManager = nullptr;
#endif

    friend class QOpenXRView;
    friend class QOpenXRRuntimeInfo;
};

QT_END_NAMESPACE

#endif // QOPENXRMANAGER_H
