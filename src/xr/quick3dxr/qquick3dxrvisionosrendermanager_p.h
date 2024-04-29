// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QQUICK3DXRVISIONOSRENDERMANAGER_P_H
#define QQUICK3DXRVISIONOSRENDERMANAGER_P_H

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
class QOpenXROrigin;
class QQuick3DViewport;

class QQuick3DXRVisionOSRenderManager : public QObject
{
    Q_OBJECT
public:
    enum class RenderState {
        Paused,
        Running,
        Invalidated
    };

    QQuick3DXRVisionOSRenderManager(QObject *parent = nullptr);
    ~QQuick3DXRVisionOSRenderManager();

    bool initialize();
    bool finalizeGraphics(QRhi *rhi);
    bool isReady() const;

    void setupWindow(QQuickWindow *window);

    void createSwapchains();

    void teardown();

    RenderState getRenderState();

    cp_layer_renderer_t layerRenderer() const;

    void runWorldTrackingARSession();
    ar_device_anchor_t createPoseForTiming(cp_frame_timing_t timing);

    void renderFrame(QQuickWindow *quickWindow,
                     QQuickRenderControl *renderControl,
                     QOpenXROrigin *xrOrigin,
                     QQuick3DViewport *xrViewport);

Q_SIGNALS:
    void initialized();

private:
    QRhiTexture *m_rhiDepthTexture = nullptr;
    ar_session_t m_arSession;
    ar_world_tracking_provider_t m_worldTrackingProvider;
};

QT_END_NAMESPACE

#endif // QQUICK3DXRVISIONOSRENDERMANAGER_P_H
