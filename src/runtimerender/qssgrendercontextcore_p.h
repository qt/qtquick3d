// Copyright (C) 2008-2012 NVIDIA Corporation.
// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QSSG_RENDER_CONTEXT_CORE_H
#define QSSG_RENDER_CONTEXT_CORE_H

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

#include <QtQuick3DRuntimeRender/private/qssgrendershaderlibrarymanager_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrhicustommaterialsystem_p.h>
#include <QtQuick3DRuntimeRender/private/qtquick3druntimerenderglobal_p.h>
#include <QtQuick3DRuntimeRender/private/qssgperframeallocator_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercache_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderdefaultmaterialshadergenerator_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderbuffermanager_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderer_p.h>

#include <QtCore/QPair>
#include <QtCore/QSize>

QT_BEGIN_NAMESPACE

class QSSGCustomMaterialSystem;
class QSSGRendererInterface;
class QQuickWindow;

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRenderContextInterface
{
    Q_DISABLE_COPY(QSSGRenderContextInterface)
public:
    QAtomicInt ref;

    static QSSGRenderContextInterface *renderContextForWindow(const QQuickWindow &window);

    // The commonly used version (from QQuick3DSceneRenderer). There is one
    // rendercontext per QQuickWindow (and so scenegraph render thread).
    QSSGRenderContextInterface(QQuickWindow *window, const QSSGRef<QSSGRhiContext> &ctx);

    // This overload must only be used in special cases, e.g. by the genshaders tool.
    QSSGRenderContextInterface(const QSSGRef<QSSGRhiContext> &ctx,
                               const QSSGRef<QSSGBufferManager> &bufferManager,
                               const QSSGRef<QSSGRenderer> &renderer,
                               const QSSGRef<QSSGShaderLibraryManager> &shaderLibraryManager,
                               const QSSGRef<QSSGShaderCache> &shaderCache,
                               const QSSGRef<QSSGCustomMaterialSystem> &customMaterialSystem,
                               const QSSGRef<QSSGProgramGenerator> &shaderProgramGenerator);

    ~QSSGRenderContextInterface();

    const QSSGRef<QSSGRenderer> &renderer() const;
    const QSSGRef<QSSGBufferManager> &bufferManager() const;
    const QSSGRef<QSSGRhiContext> &rhiContext() const;
    const QSSGRef<QSSGShaderCache> &shaderCache() const;
    const QSSGRef<QSSGShaderLibraryManager> &shaderLibraryManager() const;
    const QSSGRef<QSSGCustomMaterialSystem> &customMaterialSystem() const;
    const QSSGRef<QSSGProgramGenerator> &shaderProgramGenerator() const;

    // The memory used for the per frame allocator is released as the first step in BeginFrame.
    // This is useful for short lived objects and datastructures.
    QSSGPerFrameAllocator &perFrameAllocator() { return m_perFrameAllocator; }

    // Get the number of times EndFrame has been called
    quint32 frameCount() { return m_frameCount; }

    void setSceneColor(const QColor &inSceneColor) { m_sceneColor = inSceneColor; }

    void setViewport(QRect inViewport) { m_viewport = inViewport; }
    QRect viewport() const { return m_viewport; }

    void setDpr(float dpr) { m_dpr = dpr; }
    float dpr() const { return m_dpr; }

    void setScissorRect(QRect inScissorRect) { m_scissorRect = inScissorRect; }
    QRect scissorRect() const { return m_scissorRect; }

    void cleanupResources(QList<QSSGRenderGraphObject*> &resources);
    void cleanupUnreferencedBuffers(QSSGRenderLayer *inLayer);
    void resetResourceCounters(QSSGRenderLayer *inLayer);

    // Steps needed to render:
    // 1.  beginFrame - Reset the per-frame allocator
    // 2.  Make sure state is set (viewport, windowDimensions, dpr, sceneColor, scissorRect)
    // 3.  prepareLayerForRenderer - process the layer scene (prepare the scene for rendering)
    // 4.  rhiPrepare - start rendering necessary sub-scenes and prepare resources
    // 5.  rhiRender - render the layer scene
    // 6.  endFrame

    // Clients need to call this every frame in order for various subsystems to release
    // temporary per-frame allocated objects.
    void beginFrame(QSSGRenderLayer *layer, bool allowRecursion = true);

    bool prepareLayerForRender(QSSGRenderLayer &inLayer);

    void rhiPrepare(QSSGRenderLayer &inLayer);
    void rhiRender(QSSGRenderLayer &inLayer);

    // When allowRecursion is true, the cleanup is only done when all
    // beginFrames got their corresponding endFrame. This is indicated by the
    // return value (false if nothing's been done due to pending "frames")
    bool endFrame(QSSGRenderLayer *layer, bool allowRecursion = true);

private:
    void init();

    const QSSGRef<QSSGRhiContext> m_rhiContext;
    const QSSGRef<QSSGShaderCache> m_shaderCache;
    const QSSGRef<QSSGBufferManager> m_bufferManager;
    const QSSGRef<QSSGRenderer> m_renderer;
    const QSSGRef<QSSGShaderLibraryManager> m_shaderLibraryManager;
    const QSSGRef<QSSGCustomMaterialSystem> m_customMaterialSystem;
    const QSSGRef<QSSGProgramGenerator> m_shaderProgramGenerator;

    QSSGPerFrameAllocator m_perFrameAllocator;
    quint32 m_activeFrameRef = 0;
    quint32 m_frameCount = 0;

    // Viewport that this render context should use
    QRect m_viewport;
    float m_dpr = 1.0;
    QRect m_scissorRect;
    QColor m_sceneColor;

    QMetaObject::Connection m_beforeFrameConnection;
    QMetaObject::Connection m_afterFrameConnection;
};
QT_END_NAMESPACE

#endif
