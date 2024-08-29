// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QQUICK3DXRVISIONOSRENDERMANAGER_P_H
#define QQUICK3DXRVISIONOSRENDERMANAGER_P_H

#include <QtQuick3DXr/private/qtquick3dxrglobal_p.h>
#include <QObject>
#import <CompositorServices/CompositorServices.h>
#import <Spatial/Spatial.h>
#import <ARKit/ARKit.h>

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

QT_BEGIN_NAMESPACE

class QQuickWindow;
class QQuick3DXrOrigin;
class QQuick3DXrManager;
class QQuick3DViewport;
class QQuick3DXrInputManager;
class QQuick3DXrAnchorManager;

class QQuick3DXrManagerPrivate
{
    Q_DECLARE_PUBLIC(QQuick3DXrManager)
public:
    enum class RenderState {
        Paused,
        Running,
        Invalidated
    };

    explicit QQuick3DXrManagerPrivate(QQuick3DXrManager &manager);
    ~QQuick3DXrManagerPrivate();

    static QQuick3DXrManagerPrivate *get(QQuick3DXrManager *manager);

    bool initialize();
    bool finalizeGraphics(QRhi *rhi);
    bool isReady() const;

    bool isGraphicsInitialized() const;
    bool setupGraphics(QQuickWindow *window);

    void setupWindow(QQuickWindow *window);

    void createSwapchains();

    void teardown();

    void setMultiViewRenderingEnabled(bool enable);
    bool isMultiViewRenderingEnabled() const { return false; }
    bool isMultiViewRenderingSupported() const { return false; }

    void setPassthroughEnabled(bool enable);
    bool isPassthroughEnabled() const { return false; }
    bool supportsPassthrough() const { return false; }

    QtQuick3DXr::ReferenceSpace getReferenceSpace() const;
    void setReferenceSpace(QtQuick3DXr::ReferenceSpace newReferenceSpace);

    void setDepthSubmissionEnabled(bool enable);
    bool isDepthSubmissionEnabled() const { return true; }

    [[nodiscard]] bool isValid() const { return true; }

    RenderState getRenderState();

    void getDefaultClipDistances(float &nearClip, float &farClip) const;

    cp_layer_renderer_t layerRenderer() const;

    void runWorldTrackingARSession();
    ar_device_anchor_t createPoseForTiming(cp_frame_timing_t timing);

    void processXrEvents();

    void setSamples(int samples);

    QStringList enabledExtensions() const { return {}; }
    QString runtimeName() const;
    QVersionNumber runtimeVersion() const;

    QString errorString() const;

    void doRenderFrame();

Q_SIGNALS:
    void initialized();

private:
    QQuick3DXrManager *q_ptr = nullptr;
    QRhiTexture *m_rhiDepthTexture = nullptr;
    QPointer<QQuick3DXrInputManager> m_inputManager;
    QPointer<QQuick3DXrAnchorManager> m_anchorManager;
    ar_session_t m_arSession;
    ar_world_tracking_provider_t m_worldTrackingProvider = nullptr;
    qint64 m_previousTime = 0;
    bool m_isGraphicsInitialized = false;
};

QT_END_NAMESPACE

#endif // QQUICK3DXRVISIONOSRENDERMANAGER_P_H
