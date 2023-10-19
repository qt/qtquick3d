// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef QSSGRENDERER_H
#define QSSGRENDERER_H

//
//  W A R N I N G
//  -------------
//
// This file is part of the QtQuick3D API, with limited compatibility guarantees.
// Usage of this API may make your code source and binary incompatible with
// future versions of Qt.
//

#include <QtQuick3DRuntimeRender/qtquick3druntimerenderexports.h>

QT_BEGIN_NAMESPACE

struct QSSGRenderLayer;
class QSSGLayerRenderData;
class QSSGRhiQuadRenderer;
class QSSGRhiCubeRenderer;
class QSSGRenderContextInterface;
class QSSGRenderGraphObject;

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRenderer
{
    Q_DISABLE_COPY(QSSGRenderer)
public:
    QSSGRenderer();
    ~QSSGRenderer();

    // Returns true if this layer or a sibling was dirty.
    bool prepareLayerForRender(QSSGRenderLayer &inLayer);

    void rhiPrepare(QSSGRenderLayer &inLayer);
    void rhiRender(QSSGRenderLayer &inLayer);

    // Clients need to call this every frame in order for various subsystems to release
    // temporary per-frame allocated objects.
    void beginFrame(QSSGRenderLayer &layer, bool allowRecursion = true);

    // When allowRecursion is true, the cleanup is only done when all
    // beginFrames got their corresponding endFrame. This is indicated by the
    // return value (false if nothing's been done due to pending "frames")
    bool endFrame(QSSGRenderLayer &layer, bool allowRecursion = true);

    // Get the number of times EndFrame has been called
    quint32 frameCount() const { return m_frameCount; }

    void setViewport(QRect inViewport) { m_viewport = inViewport; }
    QRect viewport() const { return m_viewport; }

    void setDpr(float dpr) { m_dpr = dpr; }
    float dpr() const { return m_dpr; }

    void setScissorRect(QRect inScissorRect) { m_scissorRect = inScissorRect; }
    QRect scissorRect() const { return m_scissorRect; }

    const std::unique_ptr<QSSGRhiQuadRenderer> &rhiQuadRenderer();
    const std::unique_ptr<QSSGRhiCubeRenderer> &rhiCubeRenderer();

    QSSGRenderContextInterface *contextInterface() const { return m_contextInterface; }

protected:
    void cleanupResources(QList<QSSGRenderGraphObject*> &resources);
    void cleanupResources(QSet<QSSGRenderGraphObject*> &resources);

private:
    friend class QSSGRendererPrivate;
    friend class QSSGLayerRenderData;
    friend class QSSGRenderContextInterface;
    friend class QQuick3DSceneRenderer;
    friend class QQuick3DWindowAttachment;

    QSSGLayerRenderData *getOrCreateLayerRenderData(QSSGRenderLayer &layer);
    void beginLayerRender(QSSGLayerRenderData &inLayer);
    void endLayerRender();
    void addMaterialDirtyClear(QSSGRenderGraphObject *material);
    void cleanupUnreferencedBuffers(QSSGRenderLayer *inLayer);
    void resetResourceCounters(QSSGRenderLayer *inLayer);
    void releaseCachedResources();

    QSSGRenderContextInterface *m_contextInterface = nullptr; //  We're own by the context interface

    bool m_globalPickingEnabled = false;

    // Temporary information stored only when rendering a particular layer.
    QSSGLayerRenderData *m_currentLayer = nullptr;
    QByteArray m_generatedShaderString;

    QSet<QSSGRenderGraphObject *> m_materialClearDirty;

    std::unique_ptr<QSSGRhiQuadRenderer> m_rhiQuadRenderer;
    std::unique_ptr<QSSGRhiCubeRenderer> m_rhiCubeRenderer;

    quint32 m_activeFrameRef = 0;
    quint32 m_frameCount = 0;

    // Viewport that this render context should use
    QRect m_viewport;
    float m_dpr = 1.0;
    QRect m_scissorRect;
};

QT_END_NAMESPACE

#endif // QSSGRENDERER_H
