/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QSSGSCENERENDERER_H
#define QSSGSCENERENDERER_H

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

#include <QtQuick3DRuntimeRender/private/qssgrendercontextcore_p.h>

#include <qsgtextureprovider.h>
#include <qsgrendernode.h>
#include <QSGSimpleTextureNode>

#include <QtQuick3D/private/qquick3dviewport_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderlayer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrhieffectsystem_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderer_p.h>

#include "qquick3dsceneenvironment_p.h"

QT_BEGIN_NAMESPACE


class QQuick3DSceneManager;
class QQuick3DViewport;
struct QSSGRenderLayer;

class QQuick3DSceneRenderer
{
public:
    QQuick3DSceneRenderer(QWindow *window);
    ~QQuick3DSceneRenderer();

protected:
    QRhiTexture *renderToRhiTexture();
    void rhiPrepare(const QRect &viewport, qreal displayPixelRatio);
    void rhiRender();
    void cleanupResources();
    void synchronize(QQuick3DViewport *item, const QSize &size, float dpr, bool useFBO = true);
    void update();
    void invalidateFramebufferObject();
    QSize surfaceSize() const { return m_surfaceSize; }
    QSSGRenderPickResult pick(const QPointF &pos);
    QSSGRenderPickResult syncPick(const QPointF &pos);
    QQuick3DRenderStats *renderStats();

private:
    void releaseAaDependentRhiResources();
    void updateLayerNode(QQuick3DViewport *view3D);
    void addNodeToLayer(QSSGRenderNode *node);
    void removeNodeFromLayer(QSSGRenderNode *node);
    QSSGRef<QSSGRenderContextInterface> m_sgContext;
    QQuick3DSceneManager *m_sceneManager = nullptr;
    QQuick3DSceneManager *m_importSceneManager = nullptr;
    QSSGRenderLayer *m_layer = nullptr;
    QSize m_surfaceSize;
    void *data = nullptr;
    bool m_aaIsDirty = true;
    QWindow *m_window = nullptr;

    // RHI
    QRhiTexture *m_texture = nullptr;
    // the rt is set up to output into m_texture or m_ssaaTexture or m_msaaRenderBuffer(+resolve into m_texture)
    QRhiTextureRenderTarget *m_textureRenderTarget = nullptr;
    QRhiRenderPassDescriptor *m_textureRenderPassDescriptor = nullptr;
    // used by the draw quad that does m_ssaaTexture -> m_texture
    QRhiTextureRenderTarget *m_ssaaTextureToTextureRenderTarget = nullptr;
    QRhiRenderPassDescriptor *m_ssaaTextureToTextureRenderPassDescriptor = nullptr;
    QRhiRenderBuffer *m_msaaRenderBuffer = nullptr;
    QRhiTexture *m_ssaaTexture = nullptr;
    QRhiTexture *m_temporalAATexture = nullptr;
    QRhiTexture *m_prevTempAATexture = nullptr;
    QRhiTextureRenderTarget *m_temporalAARenderTarget = nullptr;
    QRhiRenderPassDescriptor *m_temporalAARenderPassDescriptor = nullptr;
    QRhiRenderBuffer *m_depthStencilBuffer = nullptr;
    bool m_textureNeedsFlip = true;
    QSSGRenderLayer::Background m_backgroundMode;
    QColor m_backgroundColor;
    int m_samples = 1;
    QSSGRhiEffectSystem *m_effectSystem = nullptr;

    QQuick3DRenderStats *m_renderStats = nullptr;

    QSSGRenderNode *m_sceneRootNode = nullptr;
    QSSGRenderNode *m_importRootNode = nullptr;

    float m_ssaaMultiplier = 1.5f;

    bool m_prepared = false;

    QMetaObject::Connection m_renderContextConnection;
    QMetaObject::Connection m_cleanupResourceConnection;

    friend class SGFramebufferObjectNode;
    friend class QQuick3DSGRenderNode;
    friend class QQuick3DSGDirectRenderer;
    friend class QQuick3DViewport;
};

namespace QQuick3DRenderLayerHelpers {
Q_QUICK3D_EXPORT void updateLayerNodeHelper(const QQuick3DViewport &view3D, QSSGRenderLayer &layerNode, bool &aaIsDirty, bool &temporalIsDirty, float &ssaaMultiplier);
}

class SGFramebufferObjectNode final : public QSGTextureProvider, public QSGSimpleTextureNode
{
    Q_OBJECT

public:
    SGFramebufferObjectNode();
    ~SGFramebufferObjectNode() override;

    void scheduleRender();

    QSGTexture *texture() const override;

    void preprocess() override;

public Q_SLOTS:
    void render();

    void handleScreenChange();

public:
    QQuickWindow *window;
    QQuick3DSceneRenderer *renderer;
    QQuick3DViewport *quickFbo;

    bool renderPending;
    bool invalidatePending;

    qreal devicePixelRatio;
    int requestedFramesCount;
};

class QQuick3DSGRenderNode final : public QSGRenderNode
{
public:
    ~QQuick3DSGRenderNode();
    void prepare() override;
    StateFlags changedStates() const override;
    void render(const RenderState *state) override;
    void releaseResources() override;
    RenderingFlags flags() const override;
public:
    QQuickWindow *window = nullptr;
    QQuick3DSceneRenderer *renderer = nullptr;
};

class QQuick3DSGDirectRenderer : public QObject
{
    Q_OBJECT
public:
    enum QQuick3DSGDirectRendererMode {
        Underlay,
        Overlay
    };
    QQuick3DSGDirectRenderer(QQuick3DSceneRenderer *renderer, QQuickWindow *window, QQuick3DSGDirectRendererMode mode = Underlay);
    ~QQuick3DSGDirectRenderer();

    QQuick3DSceneRenderer *renderer() { return m_renderer; }
    void setViewport(const QRectF &viewport);

    void requestRender();
    void setVisibility(bool visible);

private Q_SLOTS:
    void prepare();
    void render();

private:
    QQuick3DSceneRenderer *m_renderer = nullptr;
    QQuickWindow *m_window = nullptr;
    QQuick3DSGDirectRendererMode m_mode;
    QRectF m_viewport;
    bool m_isVisible = true;
};

QT_END_NAMESPACE

#endif // QSSGSCENERENDERER_H
