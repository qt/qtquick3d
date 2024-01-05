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
#include <QtQuick3DXr/private/qabstractopenxrgraphics_p.h>
#include <QtQuick3DXr/private/qopenxranimationdriver_p.h>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlComponent>

#ifdef DEBUG_RENDER_DOC
#include "renderdoc_app.h"
#endif

#ifdef XR_USE_PLATFORM_ANDROID
# include <QtCore/QJniObject>
#endif

QT_BEGIN_NAMESPACE

class QQuickWindow;
class QQuickRenderControl;
class QQuick3DNode;
class QQuick3DViewport;
class QOpenXRCamera;
class QOpenXRView;
class QOpenXRActor;
class QOpenXRInputManager;
class QOpenXRSpaceExtension;

class QOpenXRManager : public QObject
{
    Q_OBJECT
public:
    explicit QOpenXRManager(QObject *parent = nullptr);
    ~QOpenXRManager();

    bool initialize();
    void teardown();

    bool isValid() const { return m_graphics != nullptr; }

    void setPassthroughEnabled(bool enabled);

    QString errorString() const { return m_errorString; }

private Q_SLOTS:
    void update();

Q_SIGNALS:
    void sessionEnded();
    void actorChanged();

protected:
    bool event(QEvent *e) override;

private:
    void setErrorString(XrResult result, const char *callName);
    void checkXrExtensions(const char* layerName, int indent = 0);
    void checkXrLayers();

    XrResult createXrInstance();
    void checkXrInstance();

    XrResult initializeSystem();

    void checkViewConfiguration();
    bool checkXrResult(const XrResult &result);
    void checkEnvironmentBlendMode(XrViewConfigurationType type);
    bool setupGraphics();

    void checkReferenceSpaces();

    //void setupActions();
    void setupVisualizedSpace();

    void createSwapchains();

    void processXrEvents();
    void pollEvents(bool *exitRenderLoop, bool *requestRestart);
    //void pollActions();
    void renderFrame();
    bool renderLayer(XrTime predictedDisplayTime,
                     XrDuration predictedDisplayPeriod,
                     QVector<XrCompositionLayerProjectionView> &projectionLayerViews,
                     XrCompositionLayerProjection &layer);
    void doRender(const XrCompositionLayerProjectionView& layerView,
                  const XrSwapchainImageBaseHeader* swapchainImage,
                  quint64 swapchainFormat);

    void setupQuickScene();
    void updateQuickSceneEye(int eye, const XrCompositionLayerProjectionView &layerView);
    void checkActor();

    bool supportsPassthrough() const;

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

#ifdef DEBUG_RENDER_DOC
    // RenderDoc API
    void initRenderDocAPI();
    void startRenderDocCapture();
    void endRenderDocCapture();
    RENDERDOC_API_1_1_2 *rdoc_api = NULL;
    bool m_renderDocCaptureEnabled = false;
#endif

    XrInstance m_instance{XR_NULL_HANDLE};
    XrSession m_session{XR_NULL_HANDLE};
    XrSpace m_appSpace{XR_NULL_HANDLE};
    XrFormFactor m_formFactor{XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY};
    XrViewConfigurationType m_viewConfigType{XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO};
    XrEnvironmentBlendMode m_environmentBlendMode{XR_ENVIRONMENT_BLEND_MODE_OPAQUE};
    XrSystemId m_systemId{XR_NULL_SYSTEM_ID};

    QAbstractOpenXRGraphics *m_graphics = nullptr;

    QQuickWindow *m_quickWindow = nullptr;
    QQuickRenderControl *m_renderControl = nullptr;
    QQmlComponent *m_qmlContent = nullptr;
    QQuick3DViewport *m_vrViewport = nullptr;
    QQuick3DNode *m_vrCameraContainer = nullptr;
    QOpenXRActor *m_xrActor = nullptr;
    QOpenXRInputManager *m_inputManager = nullptr;

    QOpenXRAnimationDriver *m_animationDriver = nullptr;
    XrTime m_previousTime = 0;

    struct Swapchain {
        XrSwapchain handle;
        quint32 width;
        quint32 height;
    };

    QVector<XrViewConfigurationView> m_configViews;
    QVector<Swapchain> m_swapchains;
    QMap<XrSwapchain, QVector<XrSwapchainImageBaseHeader*>> m_swapchainImages;
    QVector<XrView> m_views;
    int64_t m_colorSwapchainFormat{-1};

    // Application's current lifecycle state according to the runtime
    XrSessionState m_sessionState{XR_SESSION_STATE_UNKNOWN};
    bool m_sessionRunning{false};

    XrEventDataBuffer m_eventDataBuffer;

    bool m_enablePassthrough = false;
    bool m_passthroughSupported = false;
    bool m_spaceExtensionSupported = false;
    QOpenXRSpaceExtension *m_spaceExtension = nullptr;
    bool m_colorspaceExtensionSupported = false;
    bool m_displayRefreshRateExtensionSupported = false;
    bool m_foveationExtensionSupported = false;

#ifdef XR_USE_PLATFORM_ANDROID
    QJniObject m_androidActivity;
#endif // XR_USE_PLATFORM_ANDROID

    QString m_errorString;
    QString m_runtimeName;
    QVersionNumber m_runtimeVersion;
    QStringList m_enabledExtensions;

    friend class QOpenXRView;
    friend class QOpenXRRuntimeInfo;
};

QT_END_NAMESPACE

#endif // QOPENXRMANAGER_H